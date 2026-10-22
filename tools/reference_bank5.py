"""Private complete bank-5 family checks against the player's original ROM."""
from __future__ import annotations
import ctypes as C
from pathlib import Path
import random
import re
import struct


def check_bank5(*,entries,rom,memory,cpu,bind,call,both,fresh,equal,compare,native_bytes,command,
                presets_only=False,dynamics_only=False):
    from inventory_reference import body
    root=Path(__file__).resolve().parents[1]
    texts={n:(root/f"src/game/{n}.c").read_text() for n in ("bank5","bank5_presets")}
    def addresses(module,name):
        return {int(a,16) for a in re.findall(r"case (0x[0-9A-Fa-f]+):",body(texts[module],name))}
    presets=addresses("bank5_presets","hn_bank5_presets")
    mains=addresses("bank5","hn_bank5_callback")|addresses("bank5_presets","hn_bank5_preset_callback")
    tasks=addresses("bank5","hn_bank5_task")|presets
    animations=addresses("bank5","hn_bank5_animation")
    irqs=addresses("bank5","hn_bank5_irq")
    rows=[r for r in entries if r["source"]=="src/scripts/map_bank5_callbacks.c"]
    equal(len(rows),207,"complete bank-5 source family")
    equal((len(presets),len(mains),len(tasks),len(animations),len(irqs)),(172,34,191,13,1),"reviewed bank-5 dispatch contracts")
    equal({r["name"] for r in rows if r["address"] not in mains|tasks|animations|irqs},
          {"UpdateMapBank5MechanismPrimaryLane","UpdateMapBank5MechanismPartnerLane"},"complete bank-5 typed helper census")
    main=bind("hn_scene_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
    apply=bind("hn_bank5_task",C.c_int,C.c_void_p,C.c_uint32)
    animate=bind("hn_scene_animation_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32),C.POINTER(C.c_uint32))
    irq=bind("hn_bank5_irq",C.c_int,C.c_void_p,C.c_uint32)
    primary=bind("hn_bank5_primary_lane",C.c_bool,C.c_void_p,C.c_uint32)
    partner=bind("hn_bank5_partner_lane",C.c_bool,C.c_void_p,C.c_uint32)
    task=bind("hn_probe_task_command",C.c_int,C.c_void_p,C.c_uint32,C.c_uint,C.POINTER(C.c_uint32))
    animation_command=bind("hn_probe_animation",C.c_int,C.c_void_p,C.c_uint32,C.c_uint,C.c_uint32,C.POINTER(C.c_uint32))
    heap_init=bind("hn_heap_init",None,C.c_void_p)
    audio_init=bind("hn_audio_driver_init",C.c_bool,C.c_void_p)
    rng=random.Random(0x42414E4B35)
    base,controls,classes,args,out=0x02008000,0x02004520,0x0201DC40,0x0203F000,0x0203FFF0
    ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x05000000,0x400),(0x06000000,0x18000),(0x07000000,0x400),(0x0E000000,0x8000))
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack("<H",v&65535))
    def word(a,v):both(a,struct.pack("<I",v&0xFFFFFFFF))
    def ctrl(o,v):byte(controls+o,v)
    def fixture(case):
        fresh();both(0x02000000,rom[0x8FE018:0x900EC8]);heap_init(memory);call("InitializeHeap")
        both(base,rng.randbytes(256*104));word(0x03002D98,base);byte(0x03002DC0,64)
        word(0x03002D9C,0x02038000);both(0x02038000,bytes(range(64)))
        word(0x03002E2C,0x02039000);both(0x02039000,rng.randbytes(64*8))
        both(controls,rng.randbytes(512));both(classes,rng.randbytes(256))
        both(0x02016000,rng.randbytes(32*44));word(0x03003070,0x02016000);byte(0x03002E10,8)
        both(0x02020000,rng.randbytes(8*80));word(0x03002E08,0x02020000);byte(0x03002C80,8)
        both(0x03002180,rng.randbytes(96));byte(0x03002182,128|(case&127));byte(0x03002170,0xA5)
        for a,v in ((0x03002C0C,10),(0x03001F50,13),(0x03002C04,16),(0x03002C00,19)):byte(a,v)
        for i in range(256):byte(base+i*104+0x60,1+case%4)
        both(0x03000000,struct.pack("<III",(1+case*17)&0xFFFFFFFF,(3+case*37)&0xFFFFFFFF,1+case%223))
        ctrl(0x1BD,case%9);ctrl(0x60,case%6);ctrl(0x1AA,case%4);ctrl(0x23,case%64)
        ctrl(0x1B0,case&1);ctrl(0x1AD,case%6);ctrl(0x1AE,case%6);ctrl(0x1B2,case)
        for i in range(6):ctrl(0x1A4+i,case%8);byte(0x020046D7+i,case%4)
        for i in range(4):byte(0x020046D3+i,case%3)
        half(classes+56*2,case%64);half(classes+11*2,case%100)
        word(0x03002BB0,(0,59,60,3599,3600,65535,65536,0x587C6,0x587C7,0xFFFFFFFF)[case%10])
        word(0x03001150,case);word(0x03001154,case+1)
        byte(0x03001F60,case&1);byte(0x03001F90,(case>>1)&1);byte(0x03002270,0);byte(0x03002274,0)
        for a in (0x020017F8,0x02001858,0x0200187C,0x02001884,0x02001880,0x02001881,0x02001882):byte(a,case)
        both(args,bytes([case%256])+rng.randbytes(19))
        equal(audio_init(memory),True,"bank-5 audio fixture");call("InitializeAudioDriver",limit=2000000)
    def main_check(row,label,opcode=False):
        if opcode:
            both(args,struct.pack("<I",row["address"]|1)+native_bytes(args,20));byte(0x03000010,0x1C)
            equal(command(memory,0x1C,args),1,row["name"]+" Main opcode");call("MainScriptInvokeCallback",args,limit=3000000)
        else:
            consumed=C.c_uint32(99);equal(main(memory,row["address"]|1,args,C.byref(consumed)),1,row["name"]+f" native Main/{label}")
            equal(consumed.value,call(row["name"],args,limit=3000000),row["name"]+" consumed")
        compare(row["name"]+f" Main/{label}",ranges)
    def animation_check(row,slot,label):
        consumed=C.c_uint32(99);yielded=C.c_uint32(0x12345678);word(out,yielded.value)
        equal(animate(memory,row["address"]|1,slot,args,C.byref(consumed),C.byref(yielded)),1,row["name"]+f" animation/{label}")
        equal(consumed.value,call(row["name"],slot,args,out,limit=3000000),row["name"]+" consumed")
        equal(yielded.value,int.from_bytes(cpu.mem_read(out,4),"little"),row["name"]+" yield")
        word(out,yielded.value);compare(row["name"]+f" animation/{label}",ranges)
    def vm_contracts():
        residual=[r for r in entries if r["source"]=="src/scripts/script_callbacks_stairs_residual.c"]
        for row in residual:
            if not row["name"].startswith("Animation"):
                fixture(1);both(0x0202F000,rng.randbytes(52));word(0x0202F010,args)
                both(args,b"\x01\xE1");word(0x03001FC4,row["address"]|1);report=(C.c_uint32*4)()
                equal(task(memory,0x0202F000,0xF4,report),1,"shared idle/direction F4 contract")
                equal(report[3],call("TaskCommandF4",0x0202F000,limit=3000000),"shared F4 continuation");compare(row["name"]+" F4",ranges)
        for row in rows+residual:
            if row["address"] not in animations and not row["name"].startswith("Animation"):continue
            fixture(1);both(args,b"\x8D"+struct.pack("<I",row["address"]|1)+native_bytes(args,20));word(0x03007010,args)
            report=(C.c_uint32*4)();equal(animation_command(memory,base+16*104,0x8D,0x03007010,report),1,"bank-5 animation VM dispatch")
            equal(report[3],call("AnimationInvokeCallback",base+16*104,0x03007010,limit=3000000),"bank-5 animation VM yield")
            compare(row["name"]+" animation VM",ranges+((0x03007010,4),))
    for row in rows:
        address=row["address"]
        if presets_only and address not in presets:continue
        if dynamics_only and address in presets:continue
        if address in tasks:
            for case in range(32):
                fixture(case);equal(apply(memory,address|1),1,row["name"]+f" task/{case}");call(row["name"],limit=3000000);compare(row["name"]+f" task/{case}",ranges)
            fixture(1);both(0x0202F000,rng.randbytes(52));word(0x0202F010,args)
            both(args,b"\x01\xE1");word(0x03001FC4,address|1);report=(C.c_uint32*4)()
            equal(task(memory,0x0202F000,0xF4,report),1,"bank-5 F4 accepted")
            equal(report[3],call("TaskCommandF4",0x0202F000,limit=3000000),"bank-5 F4 continuation");compare(row["name"]+" F4",ranges)
        if address in mains:
            for case in range(8 if address in presets else 128):fixture(case);main_check(row,case)
            fixture(0);main_check(row,0,True)
        else:
            fixture(0);consumed=C.c_uint32(99);equal(main(memory,address|1,args,C.byref(consumed)),0,"bank-5 wrong Main ABI rejected");compare("rejected Main preserves memory",ranges)
        if address in animations:
            for case in range(256):fixture(case);animation_check(row,base+(case%256)*104,case)
    if presets_only:
        vm_contracts()
        print("All 172 bank-5 presentation presets and their reviewed calling contracts passed; dynamics and area progression are separate checks.")
        return
    by_name={r["name"]:r for r in rows}
    for fn,name in ((primary,"UpdateMapBank5MechanismPrimaryLane"),(partner,"UpdateMapBank5MechanismPartnerLane")):
        for index in (0,1,2,3,4,16,200,255,256):
            for case in range(64):
                fixture(case);byte(0x020046D7+index,case%4);byte(0x020046DA+index,(case//4)%4)
                byte(base+(index+18)*104+0x60,(case>>4)&1);byte(base+(index+21)*104+0x60,(case>>5)&1)
                word(0x03001150,(0,1,0x7FFFFFFF,0x80000000,0xFFFFFFFF)[case%5])
                word(0x03001154,(0,1,0x7FFFFFFF,0x80000000,0xFFFFFFFF)[(case//5)%5])
                word(base+(index+18)*104+0x20,(0,0xBFFFF,0xC0000,0xFFFFFFFF)[case%4])
                word(base+(index+21)*104+0x20,(0,0x2FFFFF,0x300000,0x80000000)[(case//4)%4])
                equal(fn(memory,index),True,name+" helper");call(name,index,limit=3000000);compare(name+f" lane {index}/{case}",ranges)
    print("Bank-5 typed Main/task/animation and lane helper checks passed.")
    for case in range(256):
        fixture(case);ctrl(0x1BD,case);equal(irq(memory,0x080864B5),1,"reviewed VBlank callback");call("UpdateMapBank5Mechanism",limit=3000000);compare("mechanism VBlank callback",ranges)
        equal(irq(memory,0x08086551),0,"helper is not an IRQ contract")
        fixture(case);word(args,case if case<16 else (0xFFFFFFFF if case&1 else 0x080864B5))
        equal(command(memory,0x4E,args),1,"set VBlank callback command");call("MainScriptSetIrqUserCallback",args);compare("IRQ pointer command",ranges)
    for source in (0x030005E7,0x030005E8,0x030005E9,args):
        for value in (0,15,16,0xFFFFFFFF,0x080864B5):
            fixture(value);word(source,value);equal(command(memory,0x4E,source),1,"aliased IRQ argument")
            call("MainScriptSetIrqUserCallback",source);compare("forward IRQ argument snapshot and sentinel",ranges)
    mirror_row=by_name["AnimationCallbackMirrorMapBank5Actors"]
    boundaries=((64,128),(65,128),(143,128),(144,128),(65,129),(65,0),(65535,0),(65,65535))
    for selected in range(24):
        for mode,(x,z) in enumerate(boundaries):
            fixture(selected*8+mode);destination=(0,10,13,16,17,18,19,63)[mode];ctrl(0x23,destination)
            for id,value in ((16,mode&1),(0x15A,(mode>>1)&1)):
                a=0x03002180+(id>>3);b=native_bytes(a,1)[0];byte(a,(b&~(1<<(id&7)))|((1<<(id&7)) if value else 0))
            word(base+10*104+0x18,(x<<16)|0x8000);word(base+10*104+0x20,(z<<16)|0x8000)
            word(base+13*104+0x18,(z<<16)|0x8000);word(base+13*104+0x20,(x<<16)|0x8000)
            word(base+10*104+4,int.from_bytes(native_bytes(0x02002D60+selected*4,4),"little") if selected<23 else 0x1234ABCD)
            secondary=(selected*7+mode)%19
            word(base+13*104+4,int.from_bytes(native_bytes(0x02002DBC+secondary*4,4),"little") if secondary<18 else 0xFEDC1234)
            animation_check(mirror_row,base+7*104,"every source image, coordinate boundary and overlapping mirror actor")
    for name in ("ScriptCallbackDrawMapBank5GridCover","ScriptCallbackDrawMapBank5SelectedGridChoice","ScriptCallbackDrawMapBank5Grid"):
        for case in range(256):
            fixture(case);ctrl(0x60,case)
            for i in range(6):ctrl(0x1A4+i,(case+i*37)&255)
            main_check(by_name[name],"all raw cell/kind bytes and wrapped output slots")
    for case in range(256):
        fixture(case)
        for i in range(6):ctrl(0x1A4+i,0 if (case>>i)&1 else (case%7)+1)
        main_check(by_name["ScriptCallbackPlaceMapBank5GridChoices"],"mixed occupied cells without retry")
        main_check(by_name["ScriptCallbackReplaceMapBank5GridChoice"],"mixed occupied replacement without retry")
    print("Bank-5 mirror, raw-grid and VBlank registration boundaries passed.")
    for name in ("ScriptCallbackClampMapBank5Slot13Depth","ScriptCallbackStoreMapBank5RoundTime"):
        row=by_name[name]
        for value in range(65536):
            if value%256==0:fixture(value)
            if name.endswith("Depth"):word(base+13*104+0x20,(value<<16)|0x7123)
            else:
                word(0x03002BB0,value)
                for i,t in enumerate((1000,5000,60000)):half(classes+(63+i)*2,t)
            main_check(row,"complete halfword domain")
        print(name+": all 65,536 halfword values passed.")
    residual=[r for r in entries if r["source"]=="src/scripts/script_callbacks_stairs_residual.c"]
    equal(len(residual),4,"complete stairs/idle dependency family")
    for row in residual:
        for case in range(128):
            fixture(case);ctrl(0x75,case%6)
            byte(0x03001F50,0 if case%8==0 else 13)
            byte(0x03002270,0 if case%3==0 else 14);byte(0x03002274,0 if case%3==0 else 15)
            if row["name"].startswith("Animation"):animation_check(row,base+16*104,case)
            else:main_check(row,case)
    vm_contracts()
    print("All 207 bank-5 functions, VBlank registration and the complete four-function stairs/idle dependency family passed; full area/story integration remains unverified.")
