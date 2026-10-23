"""Private six/eight-choice room contracts against the original ROM functions."""
from __future__ import annotations
import ctypes as C
from pathlib import Path
import random
import re
import struct


def check_scene_selection(*,entries,rom,memory,bind,call,both,fresh,equal,compare,native_bytes,command):
    from inventory_reference import body
    source=(Path(__file__).resolve().parents[1]/"src/game/scene_selection.c").read_text()
    def addresses(name):return {int(v,16) for v in re.findall(r"case (0x[0-9A-Fa-f]+):",body(source,name))}
    mains=addresses("hn_scene_selection_callback");tasks=addresses("hn_scene_selection_task")
    rows=[r for r in entries if r["source"]=="src/scripts/scene_selection_callbacks.c" and 0x080BF770<=r["address"]<0x080BFD34]
    equal((len(rows),len(mains),len(tasks)),(13,10,3),"complete room-choice source family and typed ABIs")
    equal({r["address"] for r in rows},mains|tasks,"every room-choice entry accounted for")
    by_name={r["name"]:r for r in rows};u=C.c_uint32
    main=bind("hn_scene_callback",C.c_int,C.c_void_p,u,u,C.POINTER(u))
    task=bind("hn_scene_selection_task",C.c_int,C.c_void_p,u)
    task_command=bind("hn_probe_task_command",C.c_int,C.c_void_p,u,C.c_uint,C.POINTER(u))
    rng=random.Random(0x43484F494345);controls=0x02004520;base=0x02008000;args=0x0203F000
    ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x05000000,0x400),(0x06000000,0x18000),(0x07000000,0x400),(0x04000000,0x58))
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack("<H",v&65535))
    def word(a,v):both(a,struct.pack("<I",v&0xFFFFFFFF))
    def fixture(case):
        fresh();both(0x02000000,rom[0x8FE018:0x900EC8]);both(base,rng.randbytes(256*104));both(controls,rng.randbytes(1536))
        both(0x03002180,rng.randbytes(96));both(0x03002C90,rng.randbytes(0x104));both(0x03002E40,rng.randbytes(176))
        word(0x03002D98,base);byte(0x03002DC0,(0,1,64,255)[case%4]);byte(0x03001F50,(16,19,255)[case%3]);byte(0x03002C04,case)
        both(0x02016000,rng.randbytes(32*44));word(0x03003070,0x02016000);byte(0x03002E10,32)
        both(0x03000000,struct.pack("<III",0x3AC2,0x4DF3,case+1));byte(controls+0x7F,case)
        both(0x020018F0,struct.pack("<I",0x02017000)*264);both(0x02017000,bytes(range(8)))
        both(controls+0x3E,bytes((case+i)%4 for i in range(6)));both(controls+0x45,bytes((case+i)%4 for i in range(8)))
        both(args,struct.pack("<H",0x44)+bytes(14));byte(controls+0x44,case)
        both(0x0201CB20,rng.randbytes(256));both(0x0201CC20,rng.randbytes(256))
    def exercise(row,label,arguments=args,vm=False):
        if vm:
            both(arguments,struct.pack("<I",row["address"]|1)+native_bytes(arguments,16));byte(0x03000010,0x1C)
            equal(command(memory,0x1C,arguments),1,"room-choice actual Main command")
            call("MainScriptInvokeCallback",arguments,limit=4000000)
        else:
            consumed=u(99);equal(main(memory,row["address"]|1,arguments,C.byref(consumed)),1,row["name"]+" native "+str(label))
            equal(consumed.value,call(row["name"],arguments,limit=4000000),row["name"]+" consumed")
        compare(row["name"]+" "+str(label),ranges)
    for row in rows:
        for case in range(256):
            fixture(case)
            if row["address"] in mains:exercise(row,case)
            else:
                equal(task(memory,row["address"]|1),1,"room-choice void preset");call(row["name"]);compare(row["name"],ranges)
        fixture(1)
        if row["address"] in mains:
            exercise(row,"Main VM",vm=True);equal(task(memory,row["address"]|1),0,"Main-only contract rejected as void")
        else:
            consumed=u(99);equal(main(memory,row["address"]|1,args,C.byref(consumed)),0,"void Main contract rejected")
            both(0x0202F000,rng.randbytes(52));word(0x0202F010,args);both(args,b"\x01\xE1");word(0x03001FC4,row["address"]|1)
            report=(u*4)();equal(task_command(memory,0x0202F000,0xF4,report),1,"room-choice actual F4 command")
            equal(report[3],call("TaskCommandF4",0x0202F000),"F4 continuation");compare(row["name"]+" F4 VM",ranges)
    # All selected bytes, including the no-random fixed eighth choice, and both
    # probability groups. Seeds vary rather than replacing RandomRange in either side.
    for name in ("ScriptCallbackChooseSceneChoiceResult","ScriptCallbackChooseSceneChoiceResult8"):
        seen=set()
        for case in range(512):
            for side in (0,1,255):
                fixture(case);byte(controls+0x44,case);byte(controls+0x60,side)
                exercise(by_name[name],("probability groups",case,side));seen.add(native_bytes(0x0201DC84,2))
        for value in (30,54,57):equal(struct.pack("<H",value) in seen,True,"random substitution branch observed")
    # Counts occupy five bits in four-byte records. A matching empty record must
    # not hide a later occupied duplicate, including the final array element.
    for key in range(256):
        for count in (0,1,31):
            fixture(key);both(0x0201CB20,bytes(256));byte(args,key)
            half(0x0201CB20,(key&127));half(0x0201CB20+63*4,(key&127)|(count<<7))
            exercise(by_name["ScriptCallbackCheckSceneChoiceRecord"],("last duplicate",key,count))
        fixture(key);byte(args,key);byte(0x0201CC20+key,key)
        exercise(by_name["ScriptCallbackCheckSceneChoiceCounter"],"all counter values")
        fixture(key);byte(args,0);byte(controls+0x2E,key)
        exercise(by_name["ScriptCallbackCheckSceneChoiceSpecial"],"all special candidates")
    # Raw constructor writes can change a later live state or value selector.
    for name,offset,slot in (("ScriptCallbackConfigureSceneChoices6",0x3F,16),("ScriptCallbackConfigureSceneChoices6",0x2F,16),
                             ("ScriptCallbackConfigureSceneChoices8",0x46,15)):
        fixture(0);byte(controls+(0x45 if name.endswith("8") else 0x3E),0)
        word(0x03002D98,controls+offset-slot*104-0x60);exercise(by_name[name],"live control alias")
    for name in ("ScriptCallbackConfigureSceneChoices6","ScriptCallbackConfigureSceneChoices8"):
        for value in (20,25,27,22,0,255):
            for state in (0,1,2,255):
                fixture(value);both(controls+0x2E,bytes([value])*6)
                both(controls+(0x45 if name.endswith("8") else 0x3E),bytes([state])*8)
                exercise(by_name[name],("all slot states and special graphics",value,state))
    print("All 13 room-choice entries, Main/F4 contracts, shuffle banks, byte selectors, probability branches, record strides and live constructor aliases matched the original.")
