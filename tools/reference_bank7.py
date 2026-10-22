"""Private complete bank-7 tile-board and scene-preset source-family checks."""
from __future__ import annotations
import ctypes as C
from pathlib import Path
import random
import re
import struct


def check_bank7(*,entries,rom,memory,cpu,bind,call,both,fresh,equal,compare,native_bytes,command):
    from inventory_reference import body
    source=(Path(__file__).resolve().parents[1]/"src/game/bank7.c").read_text()
    def addresses(name):return {int(v,16) for v in re.findall(r"case (0x[0-9A-Fa-f]+):",body(source,name))}
    mains=addresses("hn_bank7_callback");tasks=addresses("hn_bank7_task")
    rows=[r for r in entries if r["source"] in ("src/scripts/tile_board_callbacks.c","src/actors/scene_render_presets.c")]
    helpers={r["address"] for r in rows if r["name"]=="MoveTileBoardIntoEmptyCell"}
    equal((len(rows),len(mains),len(tasks),len(helpers)),(32,5,30,1),"complete bank-7 board/preset inventories and ABIs")
    equal({r["address"] for r in rows},mains|tasks|helpers,"every source entry accounted for")
    u=C.c_uint32;ptr=C.POINTER(u)
    main=bind("hn_scene_callback",C.c_int,C.c_void_p,u,u,ptr)
    task=bind("hn_bank7_task",C.c_int,C.c_void_p,u)
    task_command=bind("hn_probe_task_command",C.c_int,C.c_void_p,u,C.c_uint,ptr)
    move=bind("hn_bank7_move_tile",C.c_bool,C.c_void_p,u,u)
    rng=random.Random(0x42414E4B37)
    base,controls,args,offsets=0x02008000,0x02004520,0x0203F000,0x020018E8
    ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x05000000,0x400),(0x06000000,0x18000),(0x07000000,0x400),(0x0E000000,0x8000),(0x04000000,0x58),(0x040000D8,8))
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack("<H",v&65535))
    def word(a,v):both(a,struct.pack("<I",v&0xFFFFFFFF))
    def fixture(case):
        fresh();both(0x02000000,rom[0x8FE018:0x900EC8]);both(base,rng.randbytes(256*104));both(controls,rng.randbytes(1536));both(0x03002180,rng.randbytes(96))
        both(0x03002C90,rng.randbytes(0x104));both(0x03002E40,rng.randbytes(176))
        word(0x03002D98,base);byte(0x03002DC0,(0,1,64,255)[case%4]);word(0x03002D9C,0x02018000);word(0x03002E2C,0x02018400)
        both(0x02018000,bytes(range(256)));both(0x02018400,rng.randbytes(256));both(0x02016000,rng.randbytes(32*44));both(0x0201A000,rng.randbytes(8*80))
        word(0x03003070,0x02016000);byte(0x03002E10,32);word(0x03002E08,0x0201A000);byte(0x03002C80,8)
        byte(0x03002C0C,10);byte(0x03001F50,(10,13,255)[case%3]);byte(0x03002C04,case&255);byte(0x03002C00,(255-case)&255)
        both(args,bytes([case&255])+rng.randbytes(15));both(controls+0x1A0,bytes(10) if case%3==0 else bytes(range(10)) if case%3==1 else rng.randbytes(10));byte(controls+0x1AA,case&255)
        for i,value in enumerate((3,-3,-1,1)):half(offsets+i*2,value)
    def main_check(row,label,arguments=args,opcode=False):
        if opcode:
            both(arguments,struct.pack("<I",row["address"]|1)+native_bytes(arguments,16));byte(0x03000010,0x1C)
            equal(command(memory,0x1C,arguments),1,"bank-7 actual Main command");call("MainScriptInvokeCallback",arguments,limit=4000000)
        else:
            consumed=u(99);equal(main(memory,row["address"]|1,arguments,C.byref(consumed)),1,row["name"]+" Main "+str(label))
            equal(consumed.value,call(row["name"],arguments,limit=4000000),row["name"]+" consumed")
        compare(row["name"]+" "+str(label),ranges)
    by_name={r["name"]:r for r in rows}
    for row in rows:
        for case in range(256):
            fixture(case)
            if row["address"] in mains:main_check(row,case)
            elif row["address"] in tasks:
                equal(task(memory,row["address"]|1),1,row["name"]+" task");call(row["name"],limit=4000000);compare(row["name"]+" raw selectors",ranges)
        fixture(1)
        if row["address"] in mains:main_check(row,"Main VM",opcode=True)
        else:
            consumed=u(99);equal(main(memory,row["address"]|1,args,C.byref(consumed)),0,"void/helper Main rejected");compare("rejected Main",ranges)
        fixture(1)
        if row["address"] in tasks:
            both(0x0202F000,rng.randbytes(52));word(0x0202F010,args);both(args,b"\x01\xE1");word(0x03001FC4,row["address"]|1);report=(u*4)()
            equal(task_command(memory,0x0202F000,0xF4,report),1,"bank-7 actual F4 command")
            equal(report[3],call("TaskCommandF4",0x0202F000,limit=4000000),"F4 continuation");compare(row["name"]+" F4 VM",ranges)
        else:equal(task(memory,row["address"]|1),0,"argument-dependent/helper F4 rejected");compare("rejected F4",ranges)
    print("All 32 bank-7 tile-board/preset entries and reviewed Main/F4 contracts passed.")
    for position in range(256):
        for direction in range(4):
            for empty in (0,1):
                fixture(position);byte(controls+0x1AA,position);byte(args,direction)
                target=position+0x1A0+(3,-3,-1,1)[direction];byte(controls+target,empty)
                main_check(by_name["ScriptCallbackMoveTileBoardCursor"],("all byte cursors/directions",position,direction,empty))
    for position in (0,1,3,4,6,7,9,10,127,255,256,0xFFFFFFFF):
        for direction in range(256):
            fixture(position);half(offsets+direction*2,(-4096,-670,-415,-1,0,1,255,4096)[direction%8])
            first=((position&255)+0x1A0+(-4096,-670,-415,-1,0,1,255,4096)[direction%8])
            byte(controls+first,0 if direction&1 else 1)
            equal(move(memory,position,direction|0x123400),True,"typed move helper");call("MoveTileBoardIntoEmptyCell",position,direction|0x123400)
            compare("raw helper words and signed mapped offsets",ranges)
    fixture(0);byte(controls+0x1AA,5);byte(controls,3)
    main_check(by_name["ScriptCallbackMoveTileBoardCursor"],"argument reread after output clear",controls)
    fixture(0);byte(controls+0x1AA,5);byte(args,0);word(0x03002D98,controls+0x1AA-10*104-0x24)
    main_check(by_name["ScriptCallbackMoveTileBoardCursor"],"cursor overwritten by moving actor")
    fixture(0);byte(controls+0x1AA,5);word(0x03002D98,controls+0x1AA-34*104-0x60)
    main_check(by_name["ScriptCallbackRefreshTileBoardArrows"],"capture coordinates but reload the selector")
    fixture(0);byte(controls+0x1A0,2);byte(controls+0x1A1,8);word(0x03002D98,controls+0x1A1-26*104-0x60)
    main_check(by_name["ScriptCallbackConfigureTileBoardSlots"],"constructor changes a later cell")
    fixture(0);byte(controls+0x1A4,1);byte(controls+0x1A7,0);word(0x03002D98,offsets-25*104-0x24)
    equal(move(memory,4,0),True,"offset table alias helper");call("MoveTileBoardIntoEmptyCell",4,0);compare("offset reread after velocity store",ranges)
    for name in ("ConfigureSceneClass2AnimationAndIndicatorB98BC","ConfigureSceneClass2AnimationAndIndicatorB9924","ConfigureSceneClass2AnimationAndNeighborB998C","ConfigureSceneClass2AnimationAndNeighborB99DC","ConfigureSceneClass2NeighborB982C"):
        fixture(19);word(0x03002D98,0x03002C04-18*104-0x60);byte(0x03002C04,19);row=by_name[name]
        equal(task(memory,row["address"]|1),1,"live class alias preset");call(name);compare(name+" constructor changes class",ranges)
    for name in ("ConfigureSceneClass2AnimationAndIndicatorB98BC","ConfigureSceneClass2AnimationAndNeighborB99DC"):
        fixture(19);word(0x03003070,0x03002C04-4*44);row=by_name[name]
        equal(task(memory,row["address"]|1),1,"live graphics class alias");call(name);compare(name+" graphics mode changes class",ranges)
    print("Bank-7 cursor transitions, all byte selectors/directions, signed helper offsets, live board/class aliases and captured arrow coordinates passed; complete puzzle/area integration remains pending.")
