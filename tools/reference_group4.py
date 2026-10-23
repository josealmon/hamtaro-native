"""Private local music/record presentation contracts; no link transport."""
from __future__ import annotations
import ctypes as C
from pathlib import Path
import random
import re
import struct


def check_group4_records(*,entries,rom,memory,bind,call,both,fresh,equal,compare,native_bytes,command):
    from inventory_reference import body
    text=(Path(__file__).resolve().parents[1]/"src/game/save_group4.c").read_text()
    def addresses(name):return {int(v,16) for v in re.findall(r"case (0x[0-9A-Fa-f]+):",body(text,name))}
    mains=addresses("hn_group4_callback");tasks={0x080C9000}
    families={"src/scripts/communication_record_callbacks.c","src/scripts/communication_record_selection.c"}
    rows=[r for r in entries if r["source"] in families]
    equal(len(rows),29,"complete local record/presentation/selection source families")
    equal({r['address'] for r in rows},(mains-{0x080C6EDC})|tasks,"all local record entries and typed ABIs")
    u=C.c_uint32;main=bind("hn_scene_callback",C.c_int,C.c_void_p,u,u,C.POINTER(u))
    task=bind("hn_group4_task",C.c_int,C.c_void_p,u);task_command=bind("hn_probe_task_command",C.c_int,C.c_void_p,u,C.c_uint,C.POINTER(u))
    heap=bind("hn_heap_init",None,C.c_void_p);sram=bind("hn_sram_init",C.c_bool,C.c_void_p)
    audio=bind("hn_audio_driver_init",C.c_bool,C.c_void_p)
    rng=random.Random(0x47525034524543);controls=0x02004520;args=0x0201F801;base=0x02008000;context=0x02010000;received=0x03003D30
    ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x0E000000,0x8000),(0x05000000,0x400),(0x06000000,0x18000),(0x04000000,0x58),(0x04000204,2))
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack("<H",v&65535))
    def word(a,v):both(a,struct.pack("<I",v&0xFFFFFFFF))
    def fixture(case):
        fresh();both(0x02000000,rom[0x8FE018:0x900EC8]);both(base,rng.randbytes(320*104));both(context,rng.randbytes(0x5000))
        both(controls,rng.randbytes(1536));both(0x03002180,rng.randbytes(96));both(received,rng.randbytes(512))
        both(0x03002BA0,rng.randbytes(9));word(0x03002BB0,context);word(0x03002D98,base);byte(0x03002DC0,(0,1,64,255)[case%4])
        byte(0x03001F50,(0,6,255)[case%3]);both(0x020021E8,rng.randbytes(2048))
        both(args,bytes([case&255])+struct.pack("<I",0x088F3ABE));byte(controls+0x180,case%14);byte(controls+0x181,case&255)
        byte(controls+0x187,case);byte(controls+0x18C,case);byte(controls+0x191,case%10);byte(controls+0x185,case)
        byte(controls+0x184,case);byte(controls+0x18B,case)
        byte(0x020046AB,case);byte(0x020046B3,case);byte(0x020046A0,case%10)
        both(0x0201DC40,rng.randbytes(512))
    def exercise(row,label,arguments=args,vm=False):
        if vm:
            both(arguments,struct.pack("<I",row['address']|1)+native_bytes(arguments,16));byte(0x03000010,0x1C)
            equal(command(memory,0x1C,arguments),1,"local group4 Main command");call("MainScriptInvokeCallback",arguments,limit=4000000)
        else:
            consumed=u(99);equal(main(memory,row['address']|1,arguments,C.byref(consumed)),1,row['name']+" native "+str(label))
            equal(consumed.value,call(row['name'],arguments,limit=4000000),row['name']+" consumed")
        compare(row['name']+" "+str(label),ranges)
    def prepare_saved_record(case):
        fixture(case);heap(memory);call("InitializeHeap",0);equal(sram(memory),True,"group4 SRAM adapters");call("InitializeSram")
        byte(controls+0x180,case%10);byte(controls+0x181,case%4);byte(received,3);byte(received+2,case%10)
    by_name={r['name']:r for r in rows}
    special={"ScriptCallbackUpdateSelectedCommunicationRecord","ScriptCallbackStartCommunicationResourceSong"}
    for row in rows:
        if row['name'] in special:continue
        for case in range(256):
            fixture(case)
            if row['address'] in mains:exercise(row,case)
            else:equal(task(memory,row['address']|1),1,"local layout task");call(row['name']);compare(row['name'],ranges)
        fixture(1)
        if row['address'] in mains:exercise(row,"Main VM",vm=True);equal(task(memory,row['address']|1),0,"Main-only interface rejected as void")
        else:
            consumed=u(99);equal(main(memory,row['address']|1,args,C.byref(consumed)),0,"void layout Main rejected")
            both(0x0201F000,rng.randbytes(52));word(0x0201F010,args);both(args,b'\x01\xE1');word(0x03001FC4,row['address']|1)
            report=(u*4)();equal(task_command(memory,0x0201F000,0xF4,report),1,"local layout actual F4 command")
            equal(report[3],call("TaskCommandF4",0x0201F000),"layout F4 continuation");compare("layout F4",ranges)
    row=by_name['ScriptCallbackUpdateSelectedCommunicationRecord']
    for case in range(256):
        prepare_saved_record(case);exercise(row,("record numbering/depth and both SRAM copies",case))
    for number in (0,1,8,9,10,49,50,51,254,255):
        for depth in (0,1,254,255):
            for kind in (0,2,3):
                prepare_saved_record(number);byte(received,kind);byte(received+2,3);both(received+5,b'\x07\x08\x09')
                both(0x03002BA0,b'\x07\x08\x09\0'+bytes(5));records=context+(number%10)*1100
                for i in range(4):
                    byte(records+i*220,1);byte(records+i*220+1,number+i);byte(records+i*220+16,depth-(i&1));both(records+i*220+5,b'\x07\x08\x09')
                exercise(row,("duplicate suffix and depth wrap",number,depth,kind))
    prepare_saved_record(1);exercise(row,"saved record Main VM",vm=True)
    # Resource descriptors and song data are read from the player's ROM.
    row=by_name['ScriptCallbackStartCommunicationResourceSong']
    for selected in range(14):
        for repeated in (False,True):
            fixture(selected);equal(audio(memory),True,"local song audio driver");call("InitializeAudioDriver",limit=2000000)
            # InitializeAudioDriver's oracle models its scanline wait; begin
            # the selected callback with the same read-only VCOUNT on both sides.
            half(0x04000006,160);compare("resource-song initialized baseline",ranges)
            byte(controls+0x180,selected);exercise(row,("resource song",selected,repeated))
            if repeated:exercise(row,"song restart")
    fixture(13);equal(audio(memory),True,"credits song audio driver");call("InitializeAudioDriver",limit=2000000)
    half(0x04000006,160);compare("credits-song initialized baseline",ranges)
    byte(controls+0x180,13);exercise(row,"song Main VM",vm=True)
    for name,offset,slot in (("ScriptCallbackRefreshCommunicationResourceBoundaryIndicators",0x187,30),
                             ("ScriptCallbackRefreshCommunicationRecordBoundaryIndicators",0x18C,32),
                             ("ScriptCallbackPositionCommunicationRenderSlots",0x193,9)):
        fixture(1);word(0x03002D98,controls+offset-slot*104-0x60);exercise(by_name[name],"live control/base alias")
    fixture(1);word(0x03002D98,0x03002D98-104-0x18);byte(args,1)
    exercise(by_name['ScriptCallbackSetCommunicationRenderPosition'],"captured slot base during position stores")
    for free in range(33):
        fixture(free);both(0x03003480,bytes(33));byte(base+26*104+0x64,255)
        if free<32:byte(0x03003480+free,255)
        exercise(by_name['ScriptCallbackConfigureCommunicationRenderSlot26'],("affine allocation/exhaustion",free))
    fixture(0);word(0x03002D98,controls+0x18C-26*104-0x64);byte(controls+0x18C,255)
    exercise(by_name['ScriptCallbackConfigureCommunicationRenderSlot26'],"affine allocation changes live scale selector")
    for flags in range(4):
        fixture(flags);byte(0x03002180+(0x281>>3),1 if flags&1 else 0);byte(0x03002180+(0x1E9>>3),2 if flags&2 else 0)
        first=rom[0x6E83D8];word(0x03002D98,0x03002D98-first*104-0x60)
        exercise(by_name['ScriptCallbackResetCommunicationRenderSlots'],"hidden-slot store aliases render base")
    print("All 29 local record/music/presentation/selection entries and typed Main/F4 contracts matched, including complete SRAM writes, suffix/depth boundaries, raw resource readers and the credits song. Link transport remains outside this scope.")
