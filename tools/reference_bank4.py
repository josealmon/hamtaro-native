"""Private whole-family checks for bank-4 presets, games, audio and path contracts."""
from __future__ import annotations
import ctypes as C
from pathlib import Path
import random
import re
import struct
from unicorn.arm_const import UC_ARM_REG_R10


def check_bank4(*,entries,rom,memory,cpu,bind,call,both,fresh,equal,compare,native_bytes,command,presets_only=False,dynamics_only=False):
    from inventory_reference import body
    root=Path(__file__).resolve().parents[1]
    sources={name:(root/f"src/game/{name}.c").read_text() for name in ("bank4","bank4_presets","bank4_path")}
    def addresses(module,name):
        return {int(x,16) for x in re.findall(r"case (0x[0-9A-Fa-f]+):",body(sources[module],name))}
    presets=addresses("bank4_presets","hn_bank4_presets")
    main_addresses=addresses("bank4_presets","hn_bank4_preset_callback")|addresses("bank4","hn_bank4_callback")
    task_addresses=presets|addresses("bank4","hn_bank4_task")
    animation_addresses=addresses("bank4","hn_bank4_animation")|addresses("bank4_path","hn_bank4_path_animation")
    rows=[r for r in entries if r["source"]=="src/scripts/map_bank4_callbacks.c"]
    helpers={"ClearMapBank4AdjacentSlotsField60","ClearMapBank4Neighbors6A88C","ClearMapBank4Neighbors6B850",
             "ClearMapBank4Neighbors6BEF8","ClearMapBank4Neighbors6C1A4","ClearMapBank4PathPresetNeighbors",
             "SetMapBank4DistanceSongVolume","CheckMapBank4ProjectedPosition","AdvanceMapBank4Path",
             "UpdateMapBank4PathAnimation","AdvanceMapBank4ReversiblePath","UpdateMapBank4DirectionalPathAnimation",
             "UpdateMapBank4BlockedPathAnimation","AccumulateMapBank4PathStep","GetMapBank4PathBlockingBounds","CheckMapBank4PathBlocked"}
    equal(len(rows),483,"complete bank-4 source inventory")
    shared=[r for r in entries if r["source"]=="src/graphics/screen_indicator_only.c"]
    equal(len(shared),1,"complete shared indicator dependency family");rows+=shared
    equal((len(presets),len(main_addresses),len(task_addresses),len(animation_addresses)),(428,220,445,17),"bank-4 and dependency typed dispatch census")
    equal({r["name"] for r in rows if r["address"] not in main_addresses|task_addresses|animation_addresses},helpers,"all bank-4 helper contracts accounted for")
    main=bind("hn_scene_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
    apply=bind("hn_bank4_task",C.c_int,C.c_void_p,C.c_uint32)
    animate=bind("hn_scene_animation_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32),C.POINTER(C.c_uint32))
    task=bind("hn_probe_task_command",C.c_int,C.c_void_p,C.c_uint32,C.c_uint,C.POINTER(C.c_uint32))
    heap_init=bind("hn_heap_init",None,C.c_void_p)
    audio_init=bind("hn_audio_driver_init",C.c_bool,C.c_void_p)
    rng=random.Random(0x42414E4B34)
    base,controls,args,output=0x02008000,0x02004520,0x0203F000,0x0203FFF0
    ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x05000000,0x400),(0x06000000,0x18000),(0x07000000,0x400),(0x0E000000,0x8000))
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack("<H",v&65535))
    def word(a,v):both(a,struct.pack("<I",v&0xFFFFFFFF))
    def ctrl(o,v):byte(controls+o,v)
    def fixture(case):
        fresh();cpu.reg_write(UC_ARM_REG_R10,0);both(0x02000000,rom[0x8FE018:0x900EC8])
        heap_init(memory);call("InitializeHeap")
        both(base,rng.randbytes(256*104));word(0x03002D98,base);byte(0x03002DC0,1)
        both(controls,rng.randbytes(512));both(0x0201DC40,rng.randbytes(2048))
        both(0x02016000,rng.randbytes(32*44));word(0x03003070,0x02016000);byte(0x03002E10,1)
        both(0x02020000,rng.randbytes(8*80));word(0x03002E08,0x02020000);byte(0x03002C80,8)
        both(0x03002180,rng.randbytes(96));byte(0x03002182,128|case&127);byte(0x03002170,0xA5)
        for address,value in zip((0x03002C0C,0x03001F50,0x03002C04,0x03002C00),(10,13,16,19)):
            byte(address,255 if case%16==15 else value)
        both(0x03000000,struct.pack("<III",rng.randrange(1,30000),rng.randrange(1,30000),case%227+1))
        ctrl(0x82,case%12);ctrl(0x89,case);ctrl(0x1A0,case%6);ctrl(0x1A1,case%5)
        ctrl(0x1A3,case%11);ctrl(0x69,case%6);ctrl(0xEB,case%5)
        ctrl(0,case%7);ctrl(0x1A5,case%6);ctrl(0x1A6,case%8);ctrl(0x1A7,case%4);ctrl(0x1A9,case);ctrl(0x1AA,case)
        ctrl(0x1A4,case&1);ctrl(0x60,case);half(0x03000822,case&1)
        both(args,bytes([case%256])+rng.randbytes(19))
        equal(audio_init(memory),True,"bank-4 audio fixture");call("InitializeAudioDriver",limit=2000000)
    def prepare(row,case):
        name=row["name"]
        if name=="ScriptCallbackWriteMapBank4Halfword":both(args,struct.pack("<IH",args+2 if case&1 else args+100,case*1021%65536))
        if name in ("ScriptCallbackPanMapBank4Background","ScriptCallbackReturnMapBank4Background"):byte(args,1+case%255)
        if name.startswith("AnimationCallbackScaleMapBank4SongBy"):
            both(args,struct.pack("<BBHHHHH",case%256,case%6,36,case*701%65536,1+case%256,case*601%65536,case*257%65536))
    def main_check(row,case,opcode=False):
        if opcode:
            both(args,struct.pack("<I",row["address"]|1)+native_bytes(args,20));byte(0x03000010,0x1C)
            equal(command(memory,0x1C,args),1,row["name"]+" Main opcode");call("MainScriptInvokeCallback",args,limit=3000000)
        else:
            consumed=C.c_uint32(99);result=main(memory,row["address"]|1,args,C.byref(consumed))
            equal(result,1,row["name"]+f" native Main/{case} result {result}")
            equal(consumed.value,call(row["name"],args,limit=3000000),row["name"]+" consumed")
        compare(row["name"]+f" Main/{case}",ranges)
    for row in rows:
        if presets_only and row["address"] not in presets:continue
        if dynamics_only and row["address"] in presets:continue
        if row["address"] in task_addresses:
            for case in range(48):
                fixture(case);prepare(row,case);equal(apply(memory,row["address"]|1),1,row["name"]+f" task/{case}")
                call(row["name"],limit=3000000);compare(row["name"]+f" task/{case}",ranges)
            fixture(1);prepare(row,1);both(0x0202F000,rng.randbytes(52));word(0x0202F010,args)
            both(args,b"\x01\xE1");word(0x03001FC4,row["address"]|1);report=(C.c_uint32*4)()
            equal(task(memory,0x0202F000,0xF4,report),1,"bank-4 F4 accepted")
            equal(report[3],call("TaskCommandF4",0x0202F000,limit=3000000),"bank-4 F4 continuation")
            compare(row["name"]+" F4",ranges)
        if row["address"] in main_addresses:
            for case in range(8 if row["address"] in presets else 64):
                fixture(case);prepare(row,case);main_check(row,case)
            fixture(0);prepare(row,0);main_check(row,0,True)
        else:
            fixture(0);consumed=C.c_uint32(99)
            equal(main(memory,row["address"]|1,args,C.byref(consumed)),0,"bank-4 non-Main contract rejected")
            compare("bank-4 rejected Main preserves memory",ranges)
    if presets_only:
        print("All 428 bank-4 presentation presets passed; path/game helpers and area integration are separate checks.")
        return
    by_name={r["name"]:r for r in rows}
    for count in range(256):
        for row in (by_name["ScriptCallbackDrawMapBank4Counter"],by_name["ScriptCallbackAdvanceMapBank4Counter"]):
            fixture(count);prepare(row,count);ctrl(0x1A3,count);main_check(row,"byte counter and wrapped slot selectors")
    timer=by_name["ScriptCallbackUpdateMapBank4JumpWithLimit"]
    for count in range(256):
        for limit in (0,1,96,255):
            fixture(count);prepare(timer,count);ctrl(0x60,count);byte(args,limit);main_check(timer,"all timer bytes and limit edges")
    for row in (by_name["ScriptCallbackPanMapBank4Background"],by_name["ScriptCallbackReturnMapBank4Background"]):
        for target in (0x0300113C,0x03001144,0x03001148,0x03002E40,0x03002E6C):
            fixture(target);prepare(row,0);word(0x03002E08,target-5*80)
            main_check(row,"background reset and step destinations alias saved/live coordinates")
    # Typed animation and helper comparisons follow the same whole-family census.
    for row in rows:
        if row["address"] not in animation_addresses:continue
        if row["name"].startswith("AnimationCallbackFollow"):continue
        for case in range(128):
            fixture(case);prepare(row,case);slot=base+(case%256)*104
            consumed=C.c_uint32(99);yielded=C.c_uint32(case*0x1020304&0xFFFFFFFF);word(output,yielded.value)
            equal(animate(memory,row["address"]|1,slot,args,C.byref(consumed),C.byref(yielded)),1,row["name"]+" animation")
            equal(consumed.value,call(row["name"],slot,args,output,limit=3000000),row["name"]+" consumed")
            equal(yielded.value,int.from_bytes(cpu.mem_read(output,4),"little"),row["name"]+" yield")
            word(output,yielded.value);compare(row["name"]+f" animation/{case}",ranges)
    check_bank4_helpers(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,fresh=fresh,
                        equal=equal,compare=compare,native_bytes=native_bytes,fixture=fixture,prepare=prepare,rows=rows,
                        base=base,controls=controls,args=args,output=output,ranges=ranges,rng=rng)
    if not dynamics_only:print("All 483 bank-4 functions and the complete shared indicator family passed.")


def check_bank4_helpers(*,entries,rom,memory,cpu,bind,call,both,fresh,equal,compare,native_bytes,
                        fixture,prepare,rows,base,controls,args,output,ranges,rng):
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack("<H",v&65535))
    def word(a,v):both(a,struct.pack("<I",v&0xFFFFFFFF))
    neighbors=bind("hn_bank4_clear_neighbors",C.c_bool,C.c_void_p,C.c_uint32)
    projected=bind("hn_bank4_projected_position",C.c_uint32,C.c_void_p,C.c_uint32)
    wander_bounds=bind("hn_wander_bounds",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint8,C.POINTER(C.c_uint32))
    volume=bind("hn_bank4_distance_volume",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint32,C.c_int32,C.c_int32,C.c_uint32,C.c_uint32)
    advance=bind("hn_bank4_advance_path",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint32)
    reverse=bind("hn_bank4_advance_reversible",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint16)
    image=bind("hn_bank4_path_image",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint8,C.c_uint8,C.c_uint8)
    directional=bind("hn_bank4_directional_image",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint8,C.c_uint8,C.c_uint8,C.c_uint32,C.c_bool)
    accumulate=bind("hn_bank4_accumulate",C.c_uint16,C.c_void_p,C.c_uint32,C.c_uint16)
    bounds=bind("hn_bank4_path_bounds",None,C.c_void_p,C.c_uint8,C.c_uint16,C.c_uint16,*([C.c_uint32]*4))
    blocked=bind("hn_bank4_path_blocked",C.c_uint8,*([C.c_uint16]*8),C.c_uint8)
    follow=bind("hn_bank4_follow_path",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32)
    animate=bind("hn_scene_animation_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32),C.POINTER(C.c_uint32))
    neighbor_names=("ClearMapBank4AdjacentSlotsField60","ClearMapBank4Neighbors6A88C","ClearMapBank4Neighbors6B850",
                    "ClearMapBank4Neighbors6BEF8","ClearMapBank4Neighbors6C1A4","ClearMapBank4PathPresetNeighbors")
    for name in neighbor_names:
        for index in range(256):
            fixture(index);equal(neighbors(memory,0xABCD0000|index),True,"byte neighbor selector")
            call(name,0xABCD0000|index);compare(name,ranges)
    for case in range(128):
        fixture(case);slot=base+(case%256)*104
        for axis,delta in ((0x18,-0x288001),(0x20,0x887FFF)):
            word(slot+axis,delta);word(slot+axis+12,0)
        if case&1:word(slot+0x18,(0x388000,0x388001,0x7FFFFFFF,0xFFFFFFFF)[case%4])
        equal(projected(memory,slot),call("CheckMapBank4ProjectedPosition",slot),"projected bounds result and save-side flag")
        compare("projected bounds",ranges)
    for case in range(128):
        fixture(case);context=0x030010E0;slot=base+16*104
        both(context,struct.pack("<IIII",0x80000000,0x80000000,0x7FFFFFFF,0x7FFFFFFF)+bytes(20))
        byte(context+30,16);byte(controls+0xAD,1);word(0x03002BB0,0x0806BA65)
        word(slot+0x18,(0x388000,0x388001,-0x288001,-0x288000)[case%4]);word(slot+0x20,(0x887FFF,0x888000,0xFFFFFFFF)[case//4%3])
        word(slot+0x24,(0,1,0xFFFFFFFF,0x10000)[case//12%4]);word(slot+0x2C,0)
        result=C.c_uint32(99);equal(wander_bounds(memory,context,slot,255,C.byref(result)),True,"bank-4 prospective predicate accepted")
        equal(result.value,call("CheckRenderMotionBounds",context,slot,255),"bank-4 prospective result")
        compare("native wandering invokes the reviewed bank-4 predicate",ranges)
    for case in range(256):
        fixture(case);both(0x020016BC,struct.pack("<256I",*([0x03004E90]*256)))
        distance=(0,1,0x10000,0x7FFFFFFF,-1,-2147483648)[case%6];limit=(1,0x18000,0x7FFFFFFF)[case%3]
        start,end=case*257,(255-case)*257
        equal(volume(memory,0xABCD0000|case,0xCAFE0024,distance,limit,0xBEEF0000|start,0xFACE0000|end),True,"distance volume")
        call("SetMapBank4DistanceSongVolume",0xABCD0000|case,0xCAFE0024,distance&0xFFFFFFFF,limit,0xBEEF0000|start,0xFACE0000|end,limit=3000000)
        compare("distance volume wrapping and byte player selector",ranges)
    # Exhaust every speed halfword against the original, retaining arbitrary carry.
    fixture(0);accumulator=0x0202F000
    for speed in range(65536):
        half(accumulator,(0,255,256,65535)[speed%4])
        equal(accumulate(memory,accumulator,speed),call("AccumulateMapBank4PathStep",accumulator,speed),"path accumulator return")
        equal(native_bytes(accumulator,2),bytes(cpu.mem_read(accumulator,2)),"path accumulator halfword carry")
    compare("exhaustive path speed side effects",ranges)
    for direction in range(256):
        fixture(direction);x=(0,1,27,28,34,65535)[direction%6];z=(0,1,23,24,36,65535)[direction//6%6]
        outputs=[0x0202F000+i*2 for i in range(4)]
        if direction&4:outputs=[outputs[0]]*4
        bounds(memory,direction,x,z,*outputs);call("GetMapBank4PathBlockingBounds",direction,x,z,*outputs)
        compare("bounds direction, wrapping, invalid output retention and aliases",ranges)
    for case in range(2048):
        values=[rng.randrange(65536) for _ in range(8)]
        if case%4==0:values[4:]=[0,65535,0,65535]
        if case%4==1:values[4:]=[values[0],values[0],values[1],values[1]]
        equal(blocked(*values,case%256),call("CheckMapBank4PathBlocked",*values,case%256),"inclusive wrapped blocking bounds")
    path=0x0202E000;state=controls+0x100
    for index in range(256):
        for reverse_flag in (False,True):
            fixture(index);both(path,b"\x11\x11"*256);half(path+(index%3+18)*2,65535)
            current_state=path+12 if index&4 else state;byte(current_state,index)
            byte(0x0300218C,2 if reverse_flag else 0)
            equal(reverse(memory,current_state,path,97),True,"reversible path helper")
            call("AdvanceMapBank4ReversiblePath",current_state,path,97);compare("reverse scan, byte wrapping and live path alias",ranges)
        fixture(index);both(path,b"\x11\x11"*256);byte(state,index)
        if index&1:half(path+((index+2)&255)*2,65535)
        equal(advance(memory,state,path),True,"forward path helper");call("AdvanceMapBank4Path",state,path)
        compare("two-word advance and sentinel",ranges)
    image_rows=("UpdateMapBank4PathAnimation","UpdateMapBank4DirectionalPathAnimation","UpdateMapBank4BlockedPathAnimation")
    for name in image_rows:
        for case in range(256):
            fixture(case);both(0x020016D4,struct.pack("<320I",*(0x08100000+i*4 for i in range(320))))
            slot=0x0202F800;both(slot,rng.randbytes(104));animation=case;overlay=(0,1,2,15,255)[case%5];direction=case%4
            selected=animation if name==image_rows[0] else rom[(0x670558 if name==image_rows[2] else 0x670540)+animation*4+direction]
            if case&1:word(slot+4,int.from_bytes(native_bytes(0x020016D4+selected*4,4),"little"))
            if name==image_rows[0]:equal(image(memory,slot,animation,overlay,case),True,"path pointer setter");call(name,slot,animation,overlay,case)
            else:
                equal(directional(memory,slot,animation,direction,overlay,0xABCD0000|case,name==image_rows[2]),True,"directional pointer setter")
                call(name,slot,animation,direction,overlay,0xABCD0000|case)
            compare(name+" arbitrary mapped actor, live tables and overlay byte",ranges)

    path_rows=[r for r in rows if r["name"].startswith("AnimationCallbackFollow")]
    position=0x0201DC40+200
    def path_fixture(row,case):
        fixture(case);byte(0x03002C0C,10);byte(0x03001F50,13)
        stride=2 if row["address"]==0x0806D38C else 3
        operation=case%(8 if stride==2 else 9)
        both(args,struct.pack("<BHHHI BHH",16,0x100,100,96,path,case%256,110,(0,1,255,256,257,65535)[case%6]))
        both(path,bytes(512));half(path,operation<<12|case%6);half(path+2,128 if operation in (2,3,5,6,8) else 160)
        half(path+4,160);half(path+stride*2,0x4001);half(path+stride*2+2,3);half(path+stride*4,65535)
        byte(state,0);byte(state+1,(0,1,2,255)[case//9%4]);byte(0x0300218C,(1 if case&1 else 0)|(2 if case&2 else 0))
        half(position,136);half(position+2,168);half(0x0201DC40+220,case*257)
        word(base+16*104+0x18,136*65536+32768);word(base+16*104+0x20,168*65536+32768);byte(base+16*104+0x60,1+case%4)
        for index in (10,13):
            word(base+index*104+0x18,(104 if case&4 and operation==8 else 128 if case&4 else 512)*65536+32768)
            word(base+index*104+0x20,(168 if case&4 and operation==8 else 160 if case&4 else 512)*65536+32768)
        half(0x0300114C,0x8000)
    def path_check(row,label,retained=0,normal=False):
        cpu.reg_write(UC_ARM_REG_R10,retained);word(output,0xA5A5A5A5)
        if normal:
            consumed=C.c_uint32(99);yielded=C.c_uint32(0xA5A5A5A5)
            equal(animate(memory,row["address"]|1,base,args,C.byref(consumed),C.byref(yielded)),1,row["name"]+" normal adapter")
        else:equal(follow(memory,row["address"]|1,args,retained),True,row["name"]+" explicit legacy SL")
        returned=call(row["name"],base,args,output,limit=3000000)
        equal(returned,11 if row["address"]==0x0806D38C else 12 if row["address"]==0x0806D794 else 16,"path consumed bytes")
        equal(int.from_bytes(cpu.mem_read(output,4),"little"),1,"path yield")
        if normal:equal((consumed.value,yielded.value),(returned,1),"normal typed path contract")
        word(output,1);compare(row["name"]+" "+label,ranges)
    for row in path_rows:
        for case in range(144):
            for retained in (0,1,0x12345678):path_fixture(row,case);path_check(row,f"operation/wait/flags/{case}",retained)
        for case in range(18):
            path_fixture(row,case)
            for frame in range(64):path_check(row,f"continuous {case}/{frame}",normal=True)
    print("Bank-4 dynamic Main/task/animation/helper contracts passed; presentation presets are a separate group.")
