"""Private complete bank-6 main/final family and calling-contract checks."""
from __future__ import annotations
import ctypes as C
from pathlib import Path
import random
import re
import struct
from unicorn.arm_const import UC_ARM_REG_R5


def check_bank6(*,entries,rom,memory,cpu,bind,call,both,fresh,equal,compare,native_bytes,command,
                presets_only=False,dynamics_only=False):
    from inventory_reference import body
    root=Path(__file__).resolve().parents[1]
    texts={n:(root/f"src/game/{n}.c").read_text() for n in ("bank6","bank6_presets","bank6_targets")}
    def addresses(module,name):return {int(a,16) for a in re.findall(r"case (0x[0-9A-Fa-f]+):",body(texts[module],name))}
    presets=addresses("bank6_presets","hn_bank6_presets")
    mains=addresses("bank6_presets","hn_bank6_preset_callback")|addresses("bank6","hn_bank6_callback")|addresses("bank6_targets","hn_bank6_targets_callback")
    tasks=presets|addresses("bank6","hn_bank6_task")
    animations=addresses("bank6","hn_bank6_animation")|addresses("bank6_targets","hn_bank6_targets_animation")
    rows=[r for r in entries if r["source"] in ("src/scripts/map_bank6_callbacks.c","src/scripts/map_bank6_final_callbacks.c")]
    equal(len(rows),243,"complete bank-6 main and final inventories")
    equal((len(presets),len(mains),len(tasks),len(animations)),(200,30,223,6),"reviewed bank-6 contracts")
    equal({r["name"] for r in rows if r["address"] not in mains|tasks|animations},
          {"UpdateMapBank6OrbitPosition","SetMapBank6TargetMotion","ConfigureMapBank6TargetPosition","ApplyMapBank6PlanarTransform"},"complete typed helper census")
    dependency=[r for r in entries if r["source"]=="src/actors/actor_neighbor_animation_pair.c"]
    equal(len(dependency),1,"complete neighbor-pair dependency")
    rows+=dependency;tasks.add(dependency[0]["address"])
    main=bind("hn_scene_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
    apply=bind("hn_bank6_task",C.c_int,C.c_void_p,C.c_uint32)
    actor=bind("hn_actor_presets_apply",C.c_int,C.c_void_p,C.c_uint32)
    animate=bind("hn_scene_animation_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32),C.POINTER(C.c_uint32))
    task=bind("hn_probe_task_command",C.c_int,C.c_void_p,C.c_uint32,C.c_uint,C.POINTER(C.c_uint32))
    animation_command=bind("hn_probe_animation",C.c_int,C.c_void_p,C.c_uint32,C.c_uint,C.c_uint32,C.POINTER(C.c_uint32))
    heap_init=bind("hn_heap_init",None,C.c_void_p);audio_init=bind("hn_audio_driver_init",C.c_bool,C.c_void_p)
    orbit=bind("hn_bank6_orbit",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint32)
    target_motion=bind("hn_bank6_target_motion",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32)
    target_position=bind("hn_bank6_target_position",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint32)
    restore=bind("hn_bank6_restore_targets",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint32)
    transform=bind("hn_bank6_planar_transform",None,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.c_uint32,C.c_uint32)
    rng=random.Random(0x42414E4B36)
    base,controls,classes,args,out=0x02008000,0x02004520,0x0201DC40,0x0203F000,0x0203FFF0
    ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x05000000,0x400),(0x06000000,0x18000),(0x07000000,0x400),(0x0E000000,0x8000))
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack("<H",v&65535))
    def word(a,v):both(a,struct.pack("<I",v&0xFFFFFFFF))
    def ctrl(o,v):byte(controls+o,v)
    def rom16(a):return int.from_bytes(rom[a-0x08000000:a-0x08000000+2],"little")
    def rom32(a):return int.from_bytes(rom[a-0x08000000:a-0x08000000+4],"little")
    selectors=[rom16(0x086BF94C+i*16+8) for i in range(7)]
    equal(255 not in selectors,True,"unmatched selector exercises the retained-R5 path")
    def fixture(case):
        fresh();cpu.reg_write(UC_ARM_REG_R5,0x030005E8);both(0x02000000,rom[0x8FE018:0x900EC8]);heap_init(memory);call("InitializeHeap")
        both(base,rng.randbytes(256*104));word(0x03002D98,base);byte(0x03002DC0,64)
        both(controls,rng.randbytes(512));both(classes,rng.randbytes(1024));both(0x0201EE40,rng.randbytes(32))
        both(0x02016000,rng.randbytes(32*44));word(0x03003070,0x02016000);byte(0x03002E10,8)
        both(0x02020000,rng.randbytes(8*80));word(0x03002E08,0x02020000);byte(0x03002C80,8)
        both(0x03002180,rng.randbytes(96));byte(0x03002182,128|(case&127));byte(0x03002170,0xA5)
        for j,a in enumerate((0x03002C0C,0x03001F50,0x03002C04,0x03002C00)):
            byte(a,(10,13,16,19)[j] if case%8<4 else (0,1,254,255)[(case+j)%4])
        both(0x03000000,struct.pack("<III",(1+case*17)&0xFFFFFFFF,(3+case*37)&0xFFFFFFFF,1+case%223))
        both(0x03000820,rng.randbytes(26));half(0x03001F64,1023)
        ctrl(0x13,case%5);ctrl(0x1A0,case%6);ctrl(0x1A2,case%3);ctrl(0x1A3,case);ctrl(0x1A4,case%3);ctrl(0x1A5,case%3);ctrl(0x1A8,case&1);ctrl(0xEB,case%5)
        both(0x03002BA0,rng.randbytes(16));both(0x03002BB0,rng.randbytes(64));both(0x03003480,bytes([case&1])*288)
        both(args,bytes([case%256])+rng.randbytes(19));half(0x030005F4,0x100);half(0x030005F6,0x101);ctrl(0x100,0);ctrl(0x101,0)
        equal(audio_init(memory),True,"bank-6 audio fixture");call("InitializeAudioDriver",limit=2000000)
    def prepare(row,case):
        name=row["name"]
        if name=="ScriptCallbackConfigureMapBank6ProjectedMotion":both(args,struct.pack("<BBBH",case%256,(case*7)%256,(case*13)%256,case%512)+rng.randbytes(6))
        if name=="ScriptCallbackSelectMapBank6Targets":
            set=0x086BF94C+(case%7)*16;count=rom16(set+10);r=rom32(set+4)+(case%count)*8
            ctrl(0x28,rom16(set+8));index=rom[r-0x08000000+4];ctrl(0x12,index)
            word(base+index*104+4,0x088E4BFF if case&1 else 0x12345678)
        if name=="ScriptCallbackRestoreMapBank6Targets":byte(args,selectors[case%7])
    def main_check(row,label,opcode=False):
        cpu.reg_write(UC_ARM_REG_R5,0x030005E8)
        if opcode:
            both(args,struct.pack("<I",row["address"]|1)+native_bytes(args,20));byte(0x03000010,0x1C)
            equal(command(memory,0x1C,args),1,row["name"]+" Main opcode");call("MainScriptInvokeCallback",args,limit=4000000)
        else:
            consumed=C.c_uint32(99);equal(main(memory,row["address"]|1,args,C.byref(consumed)),1,row["name"]+f" native Main/{label}")
            equal(consumed.value,call(row["name"],args,limit=4000000),row["name"]+" consumed")
        compare(row["name"]+f" Main/{label}",ranges)
    def animation_check(row,slot,label):
        consumed=C.c_uint32(99);yielded=C.c_uint32(0x12345678);word(out,yielded.value)
        equal(animate(memory,row["address"]|1,slot,args,C.byref(consumed),C.byref(yielded)),1,row["name"]+f" animation/{label}")
        equal(consumed.value,call(row["name"],slot,args,out,limit=4000000),row["name"]+" consumed")
        equal(yielded.value,int.from_bytes(cpu.mem_read(out,4),"little"),row["name"]+" yield");word(out,yielded.value);compare(row["name"]+f" animation/{label}",ranges)
    fixture(0);byte(args,255)
    main_check(next(r for r in rows if r["name"]=="ScriptCallbackRestoreMapBank6Targets"),"unmatched Main caller initializes R5",True)
    for row in rows:
        address=row["address"];is_preset=address in presets or row in dependency
        if presets_only and not is_preset:continue
        if dynamics_only and is_preset:continue
        if address in tasks:
            for case in range(32):
                fixture(case);prepare(row,case);equal((actor if row in dependency else apply)(memory,address|1),1,row["name"]+f" task/{case}")
                call(row["name"],limit=4000000);compare(row["name"]+f" task/{case}",ranges)
            fixture(1);prepare(row,1);both(0x0202F000,rng.randbytes(52));word(0x0202F010,args);both(args,b"\x01\xE1");word(0x03001FC4,address|1);report=(C.c_uint32*4)()
            equal(task(memory,0x0202F000,0xF4,report),1,"bank-6 F4 accepted");equal(report[3],call("TaskCommandF4",0x0202F000,limit=4000000),"bank-6 F4 continuation");compare(row["name"]+" F4",ranges)
        if address in mains:
            for case in range(8 if address in presets else 128):fixture(case);prepare(row,case);main_check(row,case)
            fixture(0);prepare(row,0);main_check(row,0,True)
        else:
            fixture(0);consumed=C.c_uint32(99);equal(main(memory,address|1,args,C.byref(consumed)),0,"wrong Main ABI rejected");compare("rejected Main preserves memory",ranges)
        if address in animations:
            for case in range(256):fixture(case);animation_check(row,base+(case%256)*104,case)
            fixture(1);both(args,b"\x8D"+struct.pack("<I",address|1)+native_bytes(args,20));word(0x03007010,args);report=(C.c_uint32*4)()
            equal(animation_command(memory,base+16*104,0x8D,0x03007010,report),1,"bank-6 animation VM")
            equal(report[3],call("AnimationInvokeCallback",base+16*104,0x03007010,limit=4000000),"bank-6 VM yield");compare("bank-6 animation VM",ranges+((0x03007010,4),))
    if presets_only:
        print("All 200 bank-6 main/final presets and the complete neighbor-pair dependency passed; other logic and chains are separate checks.")
        return
    by_name={r["name"]:r for r in rows}
    for direction in range(16):
        for x in (0,8,9,207,208,65535):
            for z in (0,8,9,127,128,65535):
                fixture(direction);half(0x03000820,direction<<4);half(0x03000822,direction&3)
                half(base+12*104+0x4E,x);half(base+12*104+0x50,z)
                main_check(by_name["ScriptCallbackMoveMapBank6Cursor"],"direction and screen-edge limits")
    for name in ("ScriptCallbackHitMapBank6CursorRect","ScriptCallbackHitMapBank6Slot11Rect"):
        for dx in (0,1,2,19,20,21,65535):
            for dz in (0,2,3,29,30,31,65535):
                fixture(dx+dz);both(args,bytes([14,1,2,20,30]));byte(base+14*104+0x60,1)
                word(base+14*104+0x18,100<<16);word(base+14*104+0x20,200<<16)
                index=12 if "Cursor" in name else 11;offset=16 if index==12 else 0
                word(base+index*104+0x18,((100+dx-offset)&65535)<<16);word(base+index*104+0x20,((200+dz-offset)&65535)<<16)
                main_check(by_name[name],"strict positive/negative rectangle boundaries")
        fixture(0);both(controls+0x60,bytes([14,1,2,20,30]));payload=native_bytes(controls+0x60,5)
        both(args,payload);saved_args=args;args=controls+0x60
        main_check(by_name[name],"result store precedes aliased payload reads");args=saved_args
    for reverse in (0,1):
        for x in (0x3BFFFF,0x3C0000,0x580000,0x580001,0xA7FFFF,0xA80000,0xC3FFFF,0xC40000,0x80000000):
            for trigger in (0x3C0000,0x3C0001,0xC3FFFF,0xC40000):
                fixture(x);a=0x03002180+(0x2B7>>3);old=native_bytes(a,1)[0];byte(a,(old&127)|(reverse<<7))
                word(base+16*104+0x18,x);word(base+22*104+0x18,trigger)
                main_check(by_name["ScriptCallbackUpdateMapBank6Slot16Motion"],"signed motion thresholds and direction flag")
    for language in range(5):
        for kind in range(3):
            pattern=rom32(0x086BED68+kind*20+language*4);data=bytearray()
            while rom[pattern-0x08000000+len(data)] and len(data)<128:data.append(rom[pattern-0x08000000+len(data)])
            equal(len(data)<128,True,"bounded original task pattern")
            for suffix in (b"\0",b"\x7F\0"):
                fixture(language);ctrl(0xEB,language);both(0x03002BA0,bytes(data)+suffix)
                main_check(by_name["ScriptCallbackChooseMapBank6TaskPattern"],"each language and prefix-match precedence")
    for value in (0,1,63,64,65,255,256,32768,65535):
        for matrix in (0,31,32,255):
            fixture(value);half(classes+110,value);byte(base+16*104+0x64,matrix)
            animation_check(by_name["AnimationCallbackScaleMapBank6Auxiliary"],base+16*104,"wrapped curve and allocation failure index")
    for case in range(256):
        fixture(case);ctrl(0x28,case);ctrl(0x12,255)
        main_check(by_name["ScriptCallbackSelectMapBank6Targets"],"missing set/range paths clear both targets")
    for phase in (0,1,1023,1024,0xEFFF,0xF000,65535):
        fixture(phase);index=225;aliased_base=classes+index*2-(25*104+0x1A);word(0x03002D98,aliased_base)
        half(classes+index*2,phase);equal(orbit(memory,25,index),True,"phase aliases the orbit center high word")
        call("UpdateMapBank6OrbitPosition",25,index);compare("orbit phase and center alias",ranges)
    print("Bank-6 cursor, rectangles, target/pattern selection, motion and scale boundaries passed.")
    for case in range(2048):
        fixture(case);index=case%256;class_word=(0xABCD0000|case%512);half(classes+(case%512)*2,case*37)
        equal(orbit(memory,0xABCD0000|index,class_word),True,"raw byte orbit selector and halfword class selector");call("UpdateMapBank6OrbitPosition",0xABCD0000|index,class_word)
        compare("orbit phase clamp and signed coordinates",ranges)
        fixture(case);slot=base+(case%256)*104;terms=[rng.getrandbits(32) for _ in range(4)]
        transform(memory,slot,*terms);call("ApplyMapBank6PlanarTransform",slot,*terms);compare("wrapped planar transform",ranges)
    for case in range(512):
        fixture(case);moving=case%256;target=(case*7)%256;coordinates=base+moving*104+0x18 if case&1 else args
        both(coordinates,struct.pack("<HH",case*127%65536,case*257%65536));word(base+target*104+4,0x088E4C34 if case&2 else 0x12345678)
        equal(target_motion(memory,coordinates,moving,target),True,"target motion with aliased coordinates");call("SetMapBank6TargetMotion",coordinates,moving,target);compare("target motion",ranges)
        fixture(case);coordinates=base+(case%256)*104+0x18 if case&1 else args
        both(coordinates,struct.pack("<HH",case*127%65536,case*257%65536));equal(target_position(memory,coordinates,0xABCD0000|case%256),True,"raw target position")
        call("ConfigureMapBank6TargetPosition",coordinates,0xABCD0000|case%256);compare("target position snapshots",ranges)
        fixture(case);set=0x02030000;positions=0x02031000;both(positions,rng.randbytes(1024));word(set,positions);half(set+12,0x100);half(set+14,0x101)
        ctrl(0x100,case%256);ctrl(0x101,(case+1)%256);byte(args,255);cpu.reg_write(UC_ARM_REG_R5,set)
        equal(restore(memory,args,set),True,"incoming R5 descriptor on unmatched selector");call("ScriptCallbackRestoreMapBank6Targets",args);compare("explicit retained descriptor",ranges)
    print("Complete bank-6 main/final families (243 functions), typed helpers and one neighbor-pair dependency passed; chain callbacks and full area/story integration remain unverified.")
