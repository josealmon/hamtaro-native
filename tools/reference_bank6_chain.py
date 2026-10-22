"""Private complete bank-6 chain family and calling-contract checks."""
from __future__ import annotations
import ctypes as C
from pathlib import Path
import random
import re
import struct


def check_bank6_chain(*,entries,rom,memory,cpu,bind,call,both,fresh,equal,compare,native_bytes,command):
    from inventory_reference import body
    source=(Path(__file__).resolve().parents[1]/"src/game/bank6_chain.c").read_text()
    def addresses(name):return {int(a,16) for a in re.findall(r"case (0x[0-9A-Fa-f]+):",body(source,name))}
    mains=addresses("hn_bank6_chain_callback");tasks=addresses("hn_bank6_chain_task");animations=addresses("hn_bank6_chain_animation")
    rows=[r for r in entries if r["source"]=="src/scripts/map_bank6_chain_callbacks.c"]
    equal(len(rows),12,"complete bank-6 chain source family")
    equal((len(mains),len(tasks),len(animations)),(5,2,7),"reviewed bank-6 chain calling contracts")
    equal({r['address'] for r in rows},mains|animations,"every chain entry has a reviewed adapter")
    main=bind("hn_scene_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
    task=bind("hn_bank6_chain_task",C.c_int,C.c_void_p,C.c_uint32)
    animate=bind("hn_scene_animation_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32),C.POINTER(C.c_uint32))
    task_command=bind("hn_probe_task_command",C.c_int,C.c_void_p,C.c_uint32,C.c_uint,C.POINTER(C.c_uint32))
    animation_command=bind("hn_probe_animation",C.c_int,C.c_void_p,C.c_uint32,C.c_uint,C.c_uint32,C.POINTER(C.c_uint32))
    rng=random.Random(0x42414E4B3643)
    # The original null SecondChain call performs two discarded BIOS reads.
    # Synthetic bytes vary independently; no BIOS image or runtime read is used.
    mapped_null=not any(start==0 for start,_,_ in cpu.mem_regions())
    if mapped_null:cpu.mem_map(0,0x1000)
    base,classes,position,args,out=0x02008000,0x0201DC40,0x03002BB0,0x0203F000,0x0203FFF0
    ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x05000000,0x400),(0x06000000,0x18000),(0x07000000,0x400),(0x0E000000,0x8000))
    by_name={r['name']:r for r in rows}
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack('<H',v&65535))
    def word(a,v):both(a,struct.pack('<I',v&0xFFFFFFFF))
    def fixture(case):
        fresh();cpu.mem_write(0,rng.randbytes(2));both(0x02000000,rom[0x8FE018:0x900EC8]);both(base,rng.randbytes(256*104));word(0x03002D98,base);byte(0x03002DC0,64)
        both(classes,rng.randbytes(1024));both(0x02004520,rng.randbytes(512));both(0x03002180,rng.randbytes(96));both(position,rng.randbytes(12))
        byte(0x03002C0C,10);byte(0x03001F50,13);byte(0x03002C00,19);byte(0x03002170,0xA5)
        both(0x020018D8,bytes(range(22,38)));half(classes+84,case&65535)
        for i in range(16):byte(base+(22+i)*104+0x64,(0,1,31,32,255)[(case+i)%5])
        both(0x03003480,bytes([255 if case%4==0 else 1 if case%4==1 else 255 if i%3 else 0 for i in range(288)]))
        both(args,bytes([case%256,0 if case%3==0 else (case*7)%256])+rng.randbytes(14))
    def main_check(row,label,arguments=args,opcode=False):
        if opcode:
            both(arguments,struct.pack('<I',row['address']|1)+native_bytes(arguments,16));byte(0x03000010,0x1C)
            equal(command(memory,0x1C,arguments),1,'chain Main command accepted');call('MainScriptInvokeCallback',arguments,limit=4000000)
        else:
            consumed=C.c_uint32(99);equal(main(memory,row['address']|1,arguments,C.byref(consumed)),1,row['name']+' Main '+str(label))
            equal(consumed.value,call(row['name'],arguments,limit=4000000),row['name']+' consumed')
        compare(row['name']+' Main '+str(label),ranges)
    def animation_check(row,slot,label):
        consumed=C.c_uint32(99);yielded=C.c_uint32(0x12345678);word(out,yielded.value)
        equal(animate(memory,row['address']|1,slot,args,C.byref(consumed),C.byref(yielded)),1,row['name']+' animation '+str(label))
        equal(consumed.value,call(row['name'],slot,args,out,limit=4000000),row['name']+' consumed')
        equal(yielded.value,int.from_bytes(cpu.mem_read(out,4),'little'),row['name']+' full-word yield');word(out,yielded.value)
        compare(row['name']+' animation '+str(label),ranges)
    for row in rows:
        if row['address'] in mains:
            for case in range(256):fixture(case);main_check(row,case)
            fixture(1);main_check(row,'real opcode',opcode=True)
        else:
            fixture(0);consumed=C.c_uint32(99);equal(main(memory,row['address']|1,args,C.byref(consumed)),0,'wrong Main ABI rejected');compare('rejected chain Main',ranges)
        if row['address'] in tasks:
            for case in range(32):
                fixture(case);equal(task(memory,row['address']|1),1,'reviewed chain task');call(row['name']);compare(row['name']+' task',ranges)
            fixture(1);both(0x0202F000,rng.randbytes(52));word(0x0202F010,args);both(args,b'\x01\xE1');word(0x03001FC4,row['address']|1);report=(C.c_uint32*4)()
            equal(task_command(memory,0x0202F000,0xF4,report),1,'chain F4 accepted');equal(report[3],call('TaskCommandF4',0x0202F000,limit=4000000),'chain F4 continuation');compare(row['name']+' F4',ranges)
        else:
            fixture(0);equal(task(memory,row['address']|1),0,'wrong task ABI rejected');compare('rejected chain task',ranges)
        if row['address'] in animations:
            for case in range(256):fixture(case);animation_check(row,base+(22 if case%4==0 else case%256)*104,case)
            fixture(1);both(args,b'\x8D'+struct.pack('<I',row['address']|1)+native_bytes(args,16));word(0x03007010,args);report=(C.c_uint32*4)()
            equal(animation_command(memory,base+22*104,0x8D,0x03007010,report),1,'chain animation VM')
            equal(report[3],call('AnimationInvokeCallback',base+22*104,0x03007010,limit=4000000),'chain VM continuation');compare(row['name']+' animation VM',ranges+((0x03007010,4),))
        else:
            fixture(0);consumed=C.c_uint32(99);yielded=C.c_uint32(0x12345678)
            equal(animate(memory,row['address']|1,base,args,C.byref(consumed),C.byref(yielded)),0,'wrong animation ABI rejected');compare('rejected chain animation',ranges)
    print('All twelve bank-6 chain entries and reviewed Main/F4/animation contracts passed.')
    for phase in (0,1,54,55,57,58,64,65,66,255,256,257,511,32768,65535):
        for name in ('UpdateMapBank6FirstChain','UpdateMapBank6SecondChain','UpdateMapBank6ThirdChain'):
            for arguments in (0,args):
                fixture(phase);main_check(by_name[name],('full halfword phase',phase,arguments),arguments)
    for name in ('AnimationCallbackCheckMapBank6RectContainment','AnimationCallbackCheckMapBank6RectOverlap'):
        for x in (-32768,-11,-10,-9,9,10,11,49,50,51,32767):
            for z in (-32768,-11,-10,-9,9,10,11,49,50,51,32767):
                fixture(x+z);half(base+10*104+0x1A,x);half(base+10*104+0x22,z)
                for i,v in enumerate((-10,50,-10,50)):half(classes+76+i*2,v)
                byte(base+10*104+0x60,(x+z)%6);animation_check(by_name[name],base+13*104,('strict signed rectangle',x,z))
    for y in (0,0x7FFF,0x8000,0xFFFFFFFF,0x7FFFFFFF,0x80000000):
        for velocity in (0,1,0x8000,0xFFFFFFFF,0x80000000):
            for index in (10,13):
                fixture(index);word(base+index*104+0x1C,y);word(base+index*104+0x28,velocity)
                animation_check(by_name['AnimationCallbackLandMapBank6AtClassPosition'],base+index*104,'wrapped landing threshold and paired classes')
    for old in (0,1,47,48,63,64,65,255,256,383,384,65535):
        for flags in range(4):
            for index in (10,22,25,63,64,255):
                fixture(old);actual=index if index<64 else 255;half(classes+(actual+205)*2,old)
                byte(0x030021D5,flags<<6);animation_check(by_name['AnimationCallbackUpdateMapBank6SineFlight'],base+index*104,('flight counter',old,flags,index))
    wrapped=[int.from_bytes(rom[0x6BF9F4+i*4:0x6BF9F8+i*4],'little') for i in range(4)]
    for value in (0x7FFFF,0x80000,0x80001,0x7FFFFFFF,0x80000000,0xFFFFFFFF):
        fixture(value)
        for index in wrapped:word(base+index*104+0x1C,value);word(base+index*104+0x20,0)
        animation_check(by_name['AnimationCallbackWrapMapBank6ChainY'],base,'strict wrapped Y/Z sum')
    for name in ('UpdateMapBank6FirstChain','UpdateMapBank6SecondChain','UpdateMapBank6ThirdChain'):
        for case in range(48):
            fixture(case);indices=0x020018D8 if name.endswith('FirstChain') else 0x020018DD if name.endswith('SecondChain') else 0x020018E2
            both(indices,bytes((0,1,254,255,0,1)));byte(0x03002C0C,255);byte(0x03001F50,254)
            main_check(by_name[name],'raw and duplicate slot bytes')
        fixture(65);word(0x03002D98,position-22*104-0x18);main_check(by_name[name],'chain overwrites the captured shared position')
    fixture(1);alias=base+27*104+0x64;byte(alias,0);byte(alias+1,13)
    main_check(by_name['UpdateMapBank6SecondChain'],'argument bytes captured before affine writes',alias)
    fixture(0);word(0x03002D98,0x03002C00-(18*104+0x60));byte(0x03002C00,19)
    main_check(by_name['ConfigureMapBank6PreviousClassIndicator'],'selector reloaded after field write')
    fixture(0);word(0x03002D98,position+8-5*104-0x18);byte(args,5)
    main_check(by_name['ScriptCallbackPlaceMapBank6AtSharedPosition'],'X store aliases the later Z read')
    fixture(1);both(0x03003480,b'\xFF'*32)
    for index in range(22,27):byte(base+index*104+0x64,0 if index==22 else 1)
    for repeat in range(3):main_check(by_name['UpdateMapBank6FirstChain'],('zero matrix is allocated again',repeat),0)
    for name,index in (('UpdateMapBank6FirstChain',22),('UpdateMapBank6SecondChain',27),('UpdateMapBank6ThirdChain',32)):
        fixture(0);word(0x03002D98,classes+84-(index*104+0x64));both(0x03003480,b'\x01'*32)
        main_check(by_name[name],'affine allocation changes the phase before it is read')
    fixture(65);word(0x03002D98,0x03002C0C-(10*104+0x18));word(position,0x100000)
    main_check(by_name['UpdateMapBank6FirstChain'],'actor X store overwrites the live leader selector')
    for name in ('AnimationCallbackUpdateMapBank6Chains','AnimationCallbackUpdateMapBank6SineFlight','AnimationCallbackLandMapBank6AtClassPosition'):
        for count in (0,1,64,255):
            fixture(count);byte(0x03002DC0,count);animation_check(by_name[name],base+22*104+1,'unrecognized unaligned slot')
    print('Bank-6 chains: full-word phases, zero affine sentinel/exhaustion, raw slots, captured/live aliases, strict rectangles, landing, flight and wrap boundaries passed; area/story integration remains unverified.')
    if mapped_null:cpu.mem_unmap(0,0x1000)
