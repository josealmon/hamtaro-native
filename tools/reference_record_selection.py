"""Private complete counter-display, record-selection and transaction checks."""
from __future__ import annotations
import ctypes as C
from pathlib import Path
import random
import re
import struct


def check_record_selection(*,entries,rom,memory,cpu,bind,call,both,fresh,equal,compare,native_bytes,command):
    from inventory_reference import body
    source=(Path(__file__).resolve().parents[1]/"src/game/record_selection.c").read_text(encoding="utf-8")
    addresses=lambda name:{int(v,16) for v in re.findall(r"case (0x[0-9A-Fa-f]+):",body(source,name))}
    mains=addresses("hn_record_selection_callback");tasks=addresses("hn_record_selection_task")
    families={"src/save/game_counter_display.c","src/save/game_record_selection.c","src/save/game_record_transactions.c"}
    rows=[r for r in entries if r["source"] in families];by_name={r['name']:r for r in rows}
    helpers={by_name[n]['address'] for n in ('WriteRecordCounterStatusPalette','LoadGameCounterDisplayRows','SelectUnusedGameRecordCandidate','HalveGameRecordPaletteChannels','RefreshGameRecordSelectionPalettes')}
    equal((len(rows),len(mains),len(tasks),len(helpers)),(23,18,14,5),"complete record selection families and contracts")
    equal({r['address'] for r in rows},mains|helpers,"all original source entries accounted for")
    ptr,u=C.c_void_p,C.c_uint32
    main=bind('hn_scene_callback',C.c_int,ptr,u,u,C.POINTER(u));task=bind('hn_record_selection_task',C.c_int,ptr,u)
    palette=bind('hn_record_counter_palette',C.c_bool,ptr,u,u);halve=bind('hn_record_palette_halve',C.c_bool,ptr,u)
    load=bind('hn_record_counter_rows',C.c_bool,ptr,u);refresh=bind('hn_record_selection_refresh',C.c_bool,ptr,u)
    pick=bind('hn_record_pick_candidate',C.c_bool,ptr,u,u,u,C.POINTER(u));heap=bind('hn_heap_init',None,ptr)
    task_command=bind('hn_probe_task_command',C.c_int,ptr,u,C.c_uint,C.POINTER(u))
    rng=random.Random(0x52454353454C)
    base,controls,classes,args,origin,list_,cache=0x02016000,0x02004520,0x0201DC40,0x02020400,0x02008000,0x02020800,0x02021000
    ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x05000000,0x400),(0x06000000,0x18000),(0x07000000,0x400),(0x04000000,0x58),(0x040000D8,8))
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack('<H',v&65535))
    def word(a,v):both(a,struct.pack('<I',v&0xFFFFFFFF))
    def word_rom(a):return struct.unpack_from('<I',rom,a-0x08000000)[0]
    def half_rom(a):return struct.unpack_from('<H',rom,a-0x08000000)[0]
    def flag_value(index,value):
        a=0x03002180+(index>>3);v=native_bytes(a,1)[0];byte(a,(v|1<<(index&7)) if value else (v&~(1<<(index&7))))
    valid=[r for r in range(1,81) if all(0x08000000<=word_rom(0x0858D344+r*24+off)<0x09000000 for off in (16,20))]
    equal(len(valid)>0,True,'selection fixtures use real mapped record resources')
    def fixture(case):
        fresh();both(0x02000000,rom[0x8FE018:0x900EC8]);both(0x02004520,rng.randbytes(512))
        both(0x02004720,rng.randbytes(0x18000));both(base,rng.randbytes(256*104));both(classes,rng.randbytes(640))
        for i in range(320):half(classes+i*2,valid[(case+i)%len(valid)])
        both(0x03002C90,rng.randbytes(256));both(0x03002180,rng.randbytes(96));both(0x03000000,rng.randbytes(12))
        word(0x03002D98,base);byte(0x03002DC0,32);byte(0x03002C0C,10);byte(0x03001F50,13)
        both(list_,bytes(1+(case+i)%30 for i in range(256)));both(cache,rng.randbytes(1024))
        word(0x03002BB0,list_);word(0x03002BB4,cache);word(args,origin);both(args+4,rng.randbytes(12))
        byte(controls+0xEB,case%5);byte(controls+0x60,1+case%31);byte(controls+0x61,case%(1+case%31))
        byte(controls+0x1A0,case%3);byte(controls+0x1A5,1+case%30);byte(controls+0x1A6,case%256)
        byte(0x020046C5,case%32);half(classes+34*2,1+case%3)
        both(0x0201CC20,bytes((0,255,1,10,127,254)[(case+i)%6] for i in range(256)))
        # Counter 31 has no display descriptor; retail lists contain 0..30.
        byte(0x0201CC20+31,0)
        both(0x0201EE40,bytes(0 if (case+i)%17==0 else 1+(case+i)%80 for i in range(256)))
        flag_value(0x86,case&2);flag_value(0xC2,case&4)
        heap(memory);call('InitializeHeap')
    def prepare(row,case):
        fixture(case);name=row['name']
        if name=='WriteRecordCounterStatusFromArguments':byte(args,case%256);word(args+1,origin)
        if name=='ScriptCallbackPrepareCounterGameRecordSelection':
            counter=1+case%30;desired=1+case%3
            candidates=[]
            for record in range(1,81):
                if desired==3 and rom[0x58D344+record*24+6]==2:continue
                if record==40 and not case&2:continue
                for f in range(3):
                    if (half_rom(0x0858D34E+record*24+f*2)>>7)&127==counter:candidates.append(record)
            desired=min(desired,len(set(candidates)));half(classes+34*2,desired)
            both(cache+counter*4,bytes([1,valid[0],valid[1],valid[2]]) if case&1 else bytes(4))
    def main_check(row,label,vm=False):
        if vm:
            both(args-4,struct.pack('<I',row['address']|1));byte(0x03000010,0x1C)
            equal(command(memory,0x1C,args-4),1,'record selection Main VM');call('MainScriptInvokeCallback',args-4,limit=8000000)
        else:
            expected=call(row['name'],args,limit=8000000)
            consumed=u(99);equal(main(memory,row['address']|1,args,C.byref(consumed)),1,row['name']+' native '+str(label))
            equal(consumed.value,expected,row['name']+' Main result')
        compare(row['name']+' '+str(label),ranges)
    for row in rows:
        if row['address'] not in mains:continue
        for case in range(64):prepare(row,case);main_check(row,case)
        prepare(row,7);main_check(row,'Main VM',True)
        prepare(row,9)
        if row['address'] in tasks:
            both(0x02020500,rng.randbytes(52));word(0x02020510,args);both(args,b'\x01\xE1');word(0x03001FC4,row['address']|1);report=(u*4)()
            equal(task_command(memory,0x02020500,0xF4,report),1,'record selection F4 VM')
            equal(report[3],call('TaskCommandF4',0x02020500,limit=8000000),'record selection F4 continuation');compare('record selection F4 '+row['name'],ranges)
        else:equal(task(memory,row['address']|1),0,'argument-aware F4 rejected');compare('rejected argument-aware F4',ranges)
    for row in rows:
        if row['address'] in helpers:
            fixture(0);consumed=u(99);equal(main(memory,row['address']|1,args,C.byref(consumed)),0,'typed helper Main rejected');equal(task(memory,row['address']|1),0,'typed helper F4 rejected');compare('rejected typed helper',ranges)
    print('All 23 counter-display/selection/transaction entries and their Main/F4 contracts passed.')
    for case in range(256):
        fixture(case);word(args,origin)
        record=(0,1,40,80,81,255,256,65535)[case%8]
        target=(origin,0x0201CC1E,0x0201DC80)[case%3]
        equal(palette(memory,target,record|0xABCD0000),True,'status palette raw word selector')
        call('WriteRecordCounterStatusPalette',target,record|0xABCD0000);compare('counter status and live counter aliases',ranges)
        fixture(case);colors=origin+(case%4)
        equal(halve(memory,colors),True,'packed palette halving');call('HalveGameRecordPaletteChannels',colors);compare('packed channel boundaries and marker bits',ranges)
        fixture(case);equal(load(memory,case|0xABCD0000),True,'two counter display rows');call('LoadGameCounterDisplayRows',case|0xABCD0000,limit=4000000);compare('two display rows and byte index',ranges)
        fixture(case);equal(refresh(memory,args),True,'selection palette helper');call('RefreshGameRecordSelectionPalettes',args,limit=4000000);compare('selected palette and three status rows',ranges)
    for case in range(128):
        fixture(case);both(origin,rng.randbytes(2048));selected=(0,1,3,255,256,65535)[case%6];count=(0,1,2,16,256,65535)[case//6%6];result=u()
        equal(pick(memory,selected|0x43210000,count|0xABCD0000,origin,C.byref(result)),True,'candidate picker typed word arguments')
        equal(result.value,call('SelectUnusedGameRecordCandidate',selected|0x43210000,count|0xABCD0000,origin,limit=4000000),'candidate picker result');compare('candidate picker PRNG and duplicate rejection',ranges)
    for case in range(256):
        for name in ('ScriptCallbackPreparePreviousGameRecordSelection','ScriptCallbackPrepareNextGameRecordSelection'):
            row=by_name[name];prepare(row,case);byte(controls+0x1A0,case);main_check(row,'all byte selectors')
        for name in ('ScriptCallbackSelectNextGameCounter','ScriptCallbackSelectPreviousGameCounter'):
            row=by_name[name];prepare(row,case);byte(controls+0x60,(0,1,32,255)[case%4]);byte(controls+0x61,case);main_check(row,'raw counter selection edges')
    for language in range(5):
        for record in valid:
            row=by_name['ScriptCallbackBuildGameRecordSelectionGraphics'];prepare(row,record);byte(controls+0xEB,language)
            half(classes+34*2,3)
            for i in range(3):half(classes+(35+i)*2,valid[(valid.index(record)+i)%len(valid)])
            main_check(row,('three records, five languages',language,record))
    for case in range(16):
        row=by_name['ScriptCallbackCaptureSelectedGameCounter'];prepare(row,case);word(0x03002BB0,controls+9);byte(controls+0x61,0);main_check(row,'source aliases first result')
        row=by_name['ScriptCallbackClearRecordBuffer40Words'];prepare(row,case);word(0x03002BB4,0x03002BB4);main_check(row,'capture clear pointer before writes')
        row=by_name['ScriptCallbackInitializeGameCounterDisplay'];prepare(row,case);word(0x03002BB0,controls+0x60);main_check(row,'list overlaps selection and count')
        fixture(case);word(args,controls+0x1A0)
        equal(refresh(memory,args),True,'palette overlaps captured selection');call('RefreshGameRecordSelectionPalettes',args,limit=4000000);compare('palette selection captured before BIOS output',ranges)
        for name in ('ScriptCallbackSelectPreviousGameRecordCandidate','ScriptCallbackSelectNextGameRecordCandidate'):
            row=by_name[name];prepare(row,case);selected=case%3;index=rom[0x5CC004+selected*6]
            word(0x03002D98,controls+0x1A0-index*104-0x24);main_check(row,'velocity changes live selection before increment')
    row=by_name['ScriptCallbackBuildGameRecordSelectionGraphics']
    for case in range(15):
        prepare(row,case);word(args,0x0201E000);half(classes+34*2,3)
        # Use a valid isolated free-list node and arena: the output deliberately
        # spans the usual descriptor pool while exercising forward label copies.
        word(0x03002E04,0x0203F000);both(0x0203F000,struct.pack('<3I',0xE000,0x02030000,0x030010C8))
        main_check(row,'forward label copies overlap their decompressed source')
    for record in range(1,81):
        row=by_name['ScriptCallbackCheckSelectedGameRecordRequirements']
        for field in range(3):
            requirement=half_rom(0x0858D34E+record*24+field*2);key=(requirement>>7)&127
            for value in (0,max(0,(requirement&127)-1),requirement&127,255):
                prepare(row,record);half(classes+224*2,record);byte(0x0201CC20+key,value)
                main_check(row,('counter requirement boundary',record,field,value))
    # Isolated function fixtures, not gameplay: choose one initial state for each
    # outcome using the independent ARM routine, then compare complete payment /
    # outcome chains. No HNGame or player save is loaded or modified here.
    roll_seeds={}
    for candidate in range(1,4097):
        seeds=struct.pack('<3I',0x3AC2,0x4DF3,candidate);both(0x03000000,seeds)
        roll=call('RandomRange',1,5);roll_seeds.setdefault(roll,seeds)
        if len(roll_seeds)==4:break
    equal(sorted(roll_seeds),[1,2,3,4],'transaction fixtures cover all four original rolls')
    outcomes={'success':0,'ordinary_failure':0,'special_refund':0};refunded_keys=set()
    payment=by_name['ScriptCallbackStoreGameRecordWithCounterCosts']
    outcome=by_name['ScriptCallbackResolveGameRecordCounterOutcome']
    for record in range(1,81):
        requirements=[half_rom(0x0858D34E+record*24+field*2) for field in range(3)]
        costs={}
        for requirement in requirements:
            key=(requirement>>7)&127;costs[key]=costs.get(key,0)+(requirement&127)
        equal(all(key<32 for key in costs),True,'retail transaction counter bounds')
        special={key for key in costs if rom[0x58D0D8+key*20+4]==5}
        initial=bytearray([255]*32)
        for key,cost in costs.items():
            if key:initial[key]=cost if key in special else cost+3
        paid=bytearray(initial)
        for requirement in requirements:
            key=(requirement>>7)&127;paid[key]=(paid[key]-(requirement&127))&255
        threshold=rom[0x58D344+record*24+8]//25
        for roll,seeds in sorted(roll_seeds.items()):
            prepare(payment,record);half(classes+224*2,record);byte(controls+0x1A0,record)
            selected=record%30;byte(controls+0x1A6,selected);byte(0x0201EE40+selected,0)
            byte(controls+0x60,0xA5);byte(controls+0x61,0x71)
            both(0x0201CC20,bytes(initial));both(0x03000000,seeds)
            slots=bytearray(native_bytes(0x0201EE40,30));slots[selected]=record
            main_check(payment,('all-recipe payment',record,roll))
            equal(native_bytes(0x0201CC20,32),bytes(paid),'recipe charges each material exactly once')
            equal(native_bytes(0x0201EE40,30),bytes(slots),'payment stores only the selected accessory')
            main_check(outcome,('all-recipe outcome',record,roll))
            expected=bytearray(paid)
            if roll>=threshold:
                outcomes['success']+=1;status,detail=1,0
            else:
                slots[selected]=0;status,detail=3,0x71
                for requirement in requirements:
                    key=(requirement>>7)&127
                    if key in special:
                        expected[key]=(expected[key]+(requirement&127))&255
                        status,detail=2,key;refunded_keys.add(key)
                outcomes['special_refund' if special else 'ordinary_failure']+=1
            equal(native_bytes(0x0201CC20,32),bytes(expected),'failure returns special materials and retains ordinary charges')
            equal(native_bytes(0x0201EE40,30),bytes(slots),'outcome preserves unrelated accessory slots')
            equal(native_bytes(controls+0x60,2),bytes([status,detail]),'success/failure/refund result contract')
    equal(all(outcomes.values()),True,'all three retail transaction outcomes exercised')
    equal(refunded_keys,{1,2,3,4,5,6},'all six special material refunds compared')
    print(f'All 80 retail recipes / four original rolls matched payment and outcome chains: {outcomes}.')
    print('Packed palettes, cached/random candidates, raw selectors, five-language graphics and counter-source aliases passed.')
