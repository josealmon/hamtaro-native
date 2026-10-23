"""Private pair-list modal comparisons; assets are read only from the caller's ROM."""
from __future__ import annotations
import ctypes as C
import random
import struct


def check_pair_list(*,entries,rom,memory,bind,call,both,fresh,equal,compare,native_bytes,command):
    from inventory_reference import dispatches
    rows={r['name']:r for r in entries if r['source']=='src/scripts/script_callbacks_pair_list.c'}
    mains={n:r for n,r in rows.items() if n.startswith('ScriptCallback')}
    animations={n:r for n,r in rows.items() if n.startswith('PairListAnimation')}
    helpers={
        'UpdatePairListPaletteSlots':('hn_pair_list_palette_slots',0),
        'CopyPairListVariationPalette':('hn_pair_list_variation_palette',2),
        'RenderPairListRemainingCount':('hn_pair_list_remaining',0),
        'RenderPairListValue':('hn_pair_list_value',0),
        'RenderPairListSelectedValue':('hn_pair_list_selected_value',0),
        'RenderPairListNumber':('hn_render_number_raw',4),
        'ConfigurePairListSelectedIcon':('hn_pair_list_icon',0),
        'ClassifyPairListFlags':('hn_pair_list_classify',2),
        'RefreshPairListSelectedDisplay':('hn_pair_list_display',2),
        'PositionPairListVariantCursor':('hn_pair_list_cursor',1),
        'SetPairListRowHighlight':('hn_pair_list_highlight',2),
        'SelectFirstUnsetPairListVariant':('hn_pair_list_first_unset',1),
        'ReturnZeroCE0DC':('hn_pair_list_return_zero',0)}
    equal(len(rows),40,'complete pair-list family')
    equal(set(rows),set(mains)|set(animations)|set(helpers),'all pair-list helpers and calling contracts')
    equal((len(mains),len(animations),len(helpers)),(18,9,13),'pair-list ABI partition')
    bindings=dispatches()
    equal(all('main' in bindings[r['address']] for r in mains.values()),True,'all eighteen pair-list Main bindings')
    equal(all('text-task' in bindings[r['address']] for r in animations.values()),True,'nine void task bindings')
    p,u=C.c_void_p,C.c_uint32;main=bind('hn_scene_callback',C.c_int,p,u,u,C.POINTER(u))
    task=bind('hn_pair_list_task',C.c_int,p,u);task_vm=bind('hn_probe_task_command',C.c_int,p,u,C.c_uint,C.POINTER(u))
    audio=bind('hn_audio_driver_init',C.c_bool,p);base=0x02008000;args=0x0201F801;ctx=0x0201F000
    controls=0x02004520;records=0x03001170;selections=0x03001188;fields=0x03003C50
    ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x04000000,0x60),
            (0x04000200,12),(0x05000000,0x400),(0x06000000,0x18000))
    rng=random.Random(0x504149524C495354)
    moving={'ScriptCallbackSelectPreviousPairListEntry','ScriptCallbackSelectNextPairListEntry',
            'ScriptCallbackSelectPreviousPairListVariant','ScriptCallbackSelectNextPairListVariant'}
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack('<H',v&65535))
    def word(a,v):both(a,struct.pack('<I',v&0xFFFFFFFF))
    def fixture(case,with_audio=False):
        fresh();both(0x02000000,rom[0x8FE018:0x900EC8]);both(base,rng.randbytes(320*104))
        word(0x03002D98,base);byte(0x03002DC0,(0,1,40,255)[case%4]);byte(0x03001F50,case)
        both(controls,rng.randbytes(512));byte(controls+0x21,case%6+1);byte(controls+0x60,case%6);byte(controls+0xEB,case%5)
        both(records,b''.join(struct.pack('<BBH',i%30,1+i%5,rng.randrange(65536)) for i in range(256)))
        both(selections,bytes(i%5 for i in range(256)));word(0x03001168,6)
        both(fields,rng.randbytes(1024));byte(0x030035CC,case);both(0x0201DC40,rng.randbytes(512))
        half(0x0201DC52,case%1000);half(0x0201DDC0,case&1);both(args,bytes([case&255,case&255,255-(case&255)])+bytes(13))
        both(0x03002C90,rng.randbytes(0x108));word(0x03002D98,base);byte(0x03002DC0,(0,1,40,255)[case%4])
        half(0x04000200,case);half(0x04000208,case&1)
        for i in range(6):word(0x02002230+i*4,0x02018000+i*0x4000)
        if with_audio:
            equal(audio(memory),True,'pair-list sound driver');call('InitializeAudioDriver',limit=2000000);half(0x04000006,160)
    def invoke(name,label,vm=False):
        row=rows[name]
        if vm:
            both(args,struct.pack('<I',row['address']|1)+native_bytes(args,16));byte(0x03000010,0x1C)
            equal(command(memory,0x1C,args),1,name+' actual Main command');call('MainScriptInvokeCallback',args,limit=4000000)
        else:
            consumed=u(99);equal(main(memory,row['address']|1,args,C.byref(consumed)),1,name+' native '+str(label))
            equal(consumed.value,call(name,args,limit=4000000),name+' consumed')
        compare(name+' '+str(label),ranges)
    def direct(name,*arguments):
        fn,count=helpers[name]
        if name=='ReturnZeroCE0DC':
            native=bind(fn,u);equal(native(),call(name,*arguments),'unknown incoming leaf return');return
        result=bind(fn,C.c_uint8 if name=='ClassifyPairListFlags' else C.c_bool,p,*([u]*count))(memory,*arguments)
        expected=call(name,*arguments,limit=4000000)
        equal(result,expected if name=='ClassifyPairListFlags' else True,name+' direct contract')
        compare(name,ranges)
    # Actual ROM lists and all five language banks: no original table/resource
    # bytes are stored in the repository. Exercise both completed and open icons.
    for group in range(1,7):
        for language in range(5):
            for known in (0,0xA8,0xF8):
                fixture(group);byte(controls+0x21,group);byte(controls+0xEB,language)
                both(fields,bytes([known,0,0,0])*30)
                invoke('ScriptCallbackPreparePairList',('source',group,language,known))
                invoke('ScriptCallbackRenderPairList',('render',group,language,known))
    for name in mains:
        if name in ('ScriptCallbackPreparePairList','ScriptCallbackRenderPairList'):continue
        for case in range(256):
            fixture(case,name in moving);invoke(name,case)
    # Every Main entry is also called through its real VM command. Argument-free
    # entries retain F4; the three-byte number callback is deliberately excluded.
    for name,row in mains.items():
        fixture(1,name in moving);invoke(name,'Main VM',vm=True)
        fixture(1,name in moving)
        if name=='ScriptCallbackRenderPairList':invoke('ScriptCallbackPreparePairList','before render F4')
        if name=='ScriptCallbackRenderPairListControlNumber':
            equal(task(memory,row['address']|1),0,'argument-dependent callback has no void ABI');continue
        both(ctx,rng.randbytes(52));word(ctx+0x10,args);both(args,b'\x01\xE1');word(0x03001FC4,row['address']|1);report=(u*4)()
        equal(task_vm(memory,ctx,0xF4,report),1,name+' F4 VM')
        equal(report[3],call('TaskCommandF4',ctx,limit=4000000),name+' F4 continuation');compare(name+' F4',ranges)
    for name,row in animations.items():
        for case in range(128):
            fixture(case);equal(task(memory,row['address']|1),1,name+' void task');call(name);compare(name,ranges)
        fixture(1);consumed=u(99);equal(main(memory,row['address']|1,args,C.byref(consumed)),0,'void animation rejected as Main')
        both(ctx,rng.randbytes(52));word(ctx+0x10,args);both(args,b'\x01\xE1');word(0x03001FC4,row['address']|1);report=(u*4)()
        equal(task_vm(memory,ctx,0xF4,report),1,name+' actual F4')
        equal(report[3],call('TaskCommandF4',ctx),name+' F4 continuation');compare(name+' F4',ranges)
    fixture(0);classify=bind('hn_pair_list_classify',C.c_uint8,p,u,u)
    for case in range(65536):
        category=(case*17)&255;mask=case>>8;value=case&255;byte(fields+category*4,value)
        equal(classify(memory,category|0xFACE0000,mask|0xBEEF0000),call('ClassifyPairListFlags',category|0xFACE0000,mask|0xBEEF0000),'all flag/mask pairs and byte entry truncation')
        compare('classification is read-only',((fields,1024),))
    for case in range(256):
        for name in helpers:
            if name in ('ClassifyPairListFlags','ReturnZeroCE0DC','CopyPairListVariationPalette'):continue
            fixture(case);index=case
            arguments=({'RenderPairListNumber':(index,(0,9,10,99,100,999,1000,65535)[case%8]|0xFACE0000,0xFFFF0000|case,0x8000FFFF-case),
                        'RefreshPairListSelectedDisplay':(index,(0,8,0x80,0xF8,255)[case%5]),
                        'PositionPairListVariantCursor':(index,),'SetPairListRowHighlight':(index,(0,1,2,255)[case%4]),
                        'SelectFirstUnsetPairListVariant':(index,)}).get(name,())
            if name=='SelectFirstUnsetPairListVariant':byte(fields+(index%30)*4,case)
            if name=='UpdatePairListPaletteSlots':byte(records+(case%6)*4+1,case)
            direct(name,*arguments)
        for variation in (0,1,4,5,127,255):fixture(case);direct('CopyPairListVariationPalette',case,variation)
    # Missing-entry sentinels, byte wrap and captured/live aliases are preserved.
    for count in (0,1,6,0xFFFFFFFF):
        for selected in (0,1,5,254,255):
            fixture(selected,True);word(0x03001168,count);byte(controls+0x60,selected)
            invoke('ScriptCallbackSelectNextPairListEntry',('raw entry boundary',count,selected))
    for count in range(8):
        for selected in (0,1,4,7,31,32,255):
            fixture(selected,True);byte(records+(selected%6)*4+1,count);byte(selections+selected%6,selected)
            invoke('ScriptCallbackSelectNextPairListVariant',('variant boundary',count,selected))
    for name,index,target in (('RenderPairListRemainingCount',13,0x030035CC),('RenderPairListSelectedValue',21,records)):
        fixture(1);byte(controls+0x60,0);word(0x03002D98,target-index*104);direct(name)
    fixture(1);byte(controls+0x60,0);byte(records+1,3);word(0x03002D98,controls+0x60-31*104-0x18)
    direct('PositionPairListVariantCursor',0)
    for group in range(1,7):
        fixture(group);both(0x02016000,b'\xFF\0\0\0');word(0x02002ADC+(group-1)*4,0x02016000);byte(controls+0x21,group)
        invoke('ScriptCallbackPreparePairList','empty sentinel');invoke('ScriptCallbackCheckPairListCompletion','empty completion')
    fixture(0);direct('ReturnZeroCE0DC',0xFACE1234,0xBEEF4321)
    for name in helpers:
        consumed=u(99);equal(main(memory,rows[name]['address']|1,args,C.byref(consumed)),0,name+' helper has no Main ABI')
        equal(task(memory,rows[name]['address']|1),0,name+' helper has no F4 ABI')
    print('All40 pair-list functions accounted for:18Main,9void animations,13typed helpers; real ROM lists/languages, raw byte slots, full flag/mask domain, sentinel/counter boundaries, palette queues, aliases and Main/F4 commands matched. Full activity/save integration remains separate.')
