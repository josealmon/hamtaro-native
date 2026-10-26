"""Private whole-family presentation checks against the player's reference ROM."""
from __future__ import annotations
import ctypes as C
from pathlib import Path
import random
import re
import struct


def check_presentation(*,entries,rom,memory,cpu,bind,call,both,fresh,equal,compare,native_bytes,command):
    from inventory_reference import body
    root=Path(__file__).resolve().parents[1]
    modules=("presentation","presentation_background","presentation_window")
    texts={n:(root/f"src/game/{n}.c").read_text() for n in modules}
    def addresses(module,suffix):
        return {int(a,16) for a in re.findall(r"case (0x[0-9A-Fa-f]+):",body(texts[module],"hn_"+module+suffix))}
    mains=set().union(*(addresses(n,"_callback") for n in modules))|{0x080CEE10,0x080CEEA8}
    tasks=set().union(*(addresses(n,"_task") for n in modules))
    animations=addresses("presentation","_animation")|addresses("presentation_background","_animation")
    sources={"src/actors/map_bank7_render_presets.c","src/graphics/render_actor_region_motion.c",
             "src/graphics/render_graphics_background_callbacks.c","src/scripts/script_callbacks_affine_background.c",
             "src/scripts/script_callbacks_background_actors.c","src/scripts/script_callbacks_background_scroll.c",
             "src/scripts/script_callbacks_window_presets.c","src/graphics/background_scroll_text_row.c",
             "src/core/transfer_dma_tilemap.c"}
    rows=[r for r in entries if r["source"] in sources];by_name={r["name"]:r for r in rows}
    helpers={0x080B9118,0x080B9164,0x080B9188,0x08021F88,0x080CC4F0,0x0800A028}
    equal((len(rows),len(mains),len(tasks),len(animations),len(helpers)),(56,28,40,6,6),"complete nine-family presentation census")
    equal({r["address"] for r in rows},mains|tasks|animations|helpers,"every source entry has a reviewed contract")
    def native(name,result,*types):return bind(name,result,C.c_void_p,*types)
    u=C.c_uint32;ptr=C.POINTER(u)
    main=native("hn_scene_callback",C.c_int,u,u,ptr)
    task=native("hn_presentation_task",C.c_int,u)
    animate=native("hn_scene_animation_callback",C.c_int,u,u,u,ptr,ptr)
    task_command=native("hn_probe_task_command",C.c_int,u,C.c_uint,ptr)
    animation_command=native("hn_probe_animation",C.c_int,u,C.c_uint,u,ptr)
    heap=native("hn_heap_init",None);task_init=native("hn_task_contexts_init",None)
    allocate=native("hn_task_allocate",u);map_init=native("hn_map_initialize",C.c_bool,C.c_uint8,C.c_uint8)
    height=native("hn_presentation_actor_height",C.c_bool,u,C.c_uint)
    motion=native("hn_presentation_region_motion",u,u,u,u,u,u)
    scroll=native("hn_presentation_scroll_row",C.c_bool,u,u,u)
    tilemap=native("hn_transfer_tilemap_rows",None,u,u,u,u,u)
    rng=random.Random(0x50524553454E54)
    base,controls,classes,args,out=0x02008000,0x02004520,0x0201DC40,0x0203F000,0x0203FFF0
    ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x05000000,0x400),
            (0x06000000,0x18000),(0x07000000,0x400),(0x0E000000,0x8000),(0x04000000,0x58),(0x040000D8,8))
    # DMA3's source points to a transient guest stack word in the original.
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack("<H",v&65535))
    def word(a,v):both(a,struct.pack("<I",v&0xFFFFFFFF))
    def ctrl(o,v):byte(controls+o,v)
    def fixture(case):
        fresh();both(0x02000000,rom[0x8FE018:0x900EC8]);both(controls,rng.randbytes(512))
        # Heap metadata aliases the control block; initialize after randomization.
        heap(memory);call("InitializeHeap");task_init(memory);call("InitializeTaskContexts")
        both(base,rng.randbytes(256*104));word(0x03002D98,base);byte(0x03002DC0,64)
        word(0x03002D9C,0x02018000);word(0x03002E2C,0x02018400)
        both(0x02018000,bytes(range(256)));both(0x02018400,rng.randbytes(256))
        both(classes,rng.randbytes(1024));both(0x03002180,rng.randbytes(96));both(0x03000000,rng.randbytes(12))
        word(0x03002E08,0x0201A000);byte(0x03002C80,8);both(0x0201A000,rng.randbytes(8*80))
        word(0x03003070,0x02016000);byte(0x03002E10,16);both(0x02016000,rng.randbytes(16*44))
        both(0x03002C90,rng.randbytes(0x104));both(0x03002E40,rng.randbytes(176))
        # VideoState randomization crosses render globals: restore typed pointers.
        word(0x03002D98,base);word(0x03002D9C,0x02018000);word(0x03002E2C,0x02018400);byte(0x03002DC0,64)
        word(0x03002E08,0x0201A000);byte(0x03002C80,8)
        word(0x02000398,0x02024000);both(0x02024000,rng.randbytes(64))
        byte(0x03002C0C,10);byte(0x03001F50,13);byte(0x03002C04,19);byte(0x03002170,0xA5)
        both(0x03003480,bytes([1 if case%3==0 else 0 for _ in range(32)]))
        ctrl(0x60,case&255);ctrl(0x61,case%20);ctrl(0x67,case%19);ctrl(0x68,case%7+1);ctrl(0x69,(1,2,0,255)[case%4]);ctrl(255,case%32)
        for i in range(5):byte(base+rom[0x6E9696+i*6]*104+0x60,0 if i==case%6 else 1)
        both(0x030037D0,rng.randbytes(1280));both(0x03003DD0,rng.randbytes(260))
        # Timer data must not overwrite the task sentinel or its links.
        index=case%256;both(args,bytes([index,case%8,case&255])+rng.randbytes(13))
        both(0x030037D0+index*5,bytes([0 if (case+i)%3==0 else (case+i)&255 for i in range(5)]))
        byte(0x03003DD0+index,12 if case&1 else 0);byte(base+19*104+0x60,case&255)
        if case%7==0:half(0x02024005,case%42)
        # Sequence removal requires a real member of the linked task list.
        a=allocate(memory);equal(a,call("AllocateTaskContext"),"window-removal task fixture")
        word(0x03003D10+((case%7+1-2)&255)*4,a)
    def main_check(row,label,arguments=args,opcode=False):
        if opcode:
            both(arguments,struct.pack("<I",row["address"]|1)+native_bytes(arguments,16));byte(0x03000010,0x1C)
            equal(command(memory,0x1C,arguments),1,"presentation Main VM")
            call("MainScriptInvokeCallback",arguments,limit=4000000)
        else:
            consumed=u(99);equal(main(memory,row["address"]|1,arguments,C.byref(consumed)),1,row["name"]+" Main "+str(label))
            equal(consumed.value,call(row["name"],arguments,limit=4000000),row["name"]+" consumed")
        compare(row["name"]+" "+str(label),ranges)
    def anim_check(row,slot,label):
        consumed=u(99);yielded=u(0xA5B6C7D8);word(out,yielded.value)
        equal(animate(memory,row["address"]|1,slot,args,C.byref(consumed),C.byref(yielded)),1,row["name"]+" animation")
        equal(consumed.value,call(row["name"],slot,args,out,limit=4000000),row["name"]+" consumed "+str(label))
        equal(yielded.value,int.from_bytes(cpu.mem_read(out,4),"little"),row["name"]+" full-word yield");word(out,yielded.value)
        compare(row["name"]+" "+str(label),ranges)
    modal={0x080CEE10,0x080CEEA8}
    for row in rows:
        if row["address"] in modal:continue
        for case in range(32):
            fixture(case)
            if row["address"] in mains:
                if row["address"]==0x080CBDF4:ctrl(0x60,case%3)
                main_check(row,case)
            elif row["address"] in tasks:
                equal(task(memory,row["address"]|1),1,row["name"]+" task");call(row["name"],limit=4000000);compare(row["name"]+" task",ranges)
            elif row["address"] in animations:anim_check(row,base+19*104,case)
        fixture(1)
        if row["address"] in mains:main_check(row,"real opcode",opcode=True)
        else:
            consumed=u(99);equal(main(memory,row["address"]|1,args,C.byref(consumed)),0,"unreviewed Main ABI rejected");compare("rejected Main",ranges)
        fixture(1)
        if row["address"] in tasks:
            both(0x0202F000,rng.randbytes(52));word(0x0202F010,args);both(args,b"\x01\xE1");word(0x03001FC4,row["address"]|1);report=(u*4)()
            equal(task_command(memory,0x0202F000,0xF4,report),1,"presentation F4 VM")
            equal(report[3],call("TaskCommandF4",0x0202F000,limit=4000000),"F4 continuation");compare(row["name"]+" F4",ranges)
        else:equal(task(memory,row["address"]|1),0,"unreviewed F4 ABI rejected");compare("rejected task",ranges)
        fixture(1)
        if row["address"] in animations:
            both(args,b"\x8D"+struct.pack("<I",row["address"]|1)+native_bytes(args,16));word(0x03007010,args);report=(u*4)()
            equal(animation_command(memory,base+19*104,0x8D,0x03007010,report),1,"presentation animation VM")
            equal(report[3],call("AnimationInvokeCallback",base+19*104,0x03007010,limit=4000000),"animation continuation");compare(row["name"]+" animation VM",ranges+((0x03007010,4),))
        else:
            consumed=u(99);yielded=u(0xA5B6C7D8);equal(animate(memory,row["address"]|1,base,args,C.byref(consumed),C.byref(yielded)),0,"unreviewed animation ABI rejected");compare("rejected animation",ranges)
    print("Presentation: every new Main/F4/animation entry and VM contract passed.")
    for operation,name in enumerate(("UpdateMapBank7ActorHeight","LowerMapBank7ActorHeight","RestoreMapBank7ActorHeight")):
        for y in (0,1,0x7FFF,0x8000,0xFFFFFFFF,0xFFF38000,0xFFF38001,0xFFF18001,0x7FFFFFFF,0x80000000):
            for x in (0x507FFF,0x508000,0xD08000,0xD08001):
                fixture(y);word(base+0x1C,y);word(base+0x18,x);word(base+0x20,0x1E8000);byte(0x030021B5,32)
                equal(height(memory,base,operation),True,"height helper");call(name,base);compare(name,ranges)
    images=(0x088AC2BA,0x088AC359,0x088AC0C6,0x088AC0FD)
    for x in (0,0x100000,0x100001,0x600000,0x600001,0x71FFFF,0x720000,0xB00000,0xB00001,0xFFFFFFFF,0x80000000):
        for z in (0x6BFFFF,0x6C0000,0xFFFFFFFF,0x80000000):
            for flags in range(4):
                fixture(x+z);word(base+0x18,x);word(base+0x20,z);byte(base+0x60,flags);word(base+4,images[flags]);byte(0x0300218F,64 if flags&1 else 0)
                equal(motion(memory,base,*images),call("UpdateActorRegionRenderMotion",base,*images),"mixed signed/unsigned region");compare("region helper",ranges)
    for width in (0,1,31,32,255,256,0xFFFFFFFF):
        for rows_count in (0,1,2,255,256):
            for wide in (0,1,256,0xFFFFFFFF):
                fixture(width);byte(0x03002E34,252 if width&1 else 255)
                tilemap(memory,args,0x06000000,width,rows_count,wide);call("QueueDmaTilemapRows",args,0x06000000,width,rows_count,wide)
                compare("raw tilemap words and queue wrap",ranges)
    for bank in (0,1,2):
        for position in (0,1,7,8,247,248,255,256,257,504,511,512,65535,0xFFFF0000):
            fixture(position);word(args,0x086E6B17)
            equal(scroll(memory,args,bank|0x123400,position),True,"scroll-row helper");call("ConfigureBackgroundScrollTextRow",args,bank|0x123400,position)
            compare("scroll row two pages and word normalization",ranges)
    for case in range(256):
        fixture(case);byte(args,case);byte(args+1,case%8)
        main_check(by_name["ScriptCallbackConfigureObjectWindowGrid"],"raw wrapped first slot")
        fixture(case);ctrl(0x60,case);main_check(by_name["ScriptCallbackConfigureSelectionWindowSlots"],"raw selection")
        fixture(case);ctrl(255,case);half(classes+82,(0,1,32767,32768,65535)[case%5])
        main_check(by_name["ScriptCallbackAdvanceWindowSequenceStage"],"raw stage and counter")
        fixture(case);ctrl(0x69,case);main_check(by_name["ScriptCallbackSpawnBackgroundActorAtTableX"],"byte constructor versus word velocity index")
        fixture(case);byte(0x03003580,case);main_check(by_name["RecordBackgroundScrollPosition"],"byte postincrement")
    for angle in (0,63,64,127,128,191,192,255,256,511,32768,65535):
        for scale in (0,1,2,255,256,32767,32768,65535):
            fixture(angle);half(0x03002E40+88+28,angle);half(0x03002E40+88+24,scale);half(0x03002E40+88+26,scale^65535)
            half(0x03002E40+88+30,32768);half(0x03002E40+88+32,32767)
            main_check(by_name["ScriptCallbackApplyBackground2Affine"],"full angle, signed reciprocal and anchors")
    for x in (0x27FFFF,0x280000,0x280001,0x7FFFFFFF,0x80000000,0xFFFFFFFF):
        for z in (0x27FFFF,0x280000,0x280001,0x7FFFFFFF,0x80000000,0xFFFFFFFF):
            fixture(x);word(base+10*104+24,0);word(base+13*104+24,x);word(base+10*104+32,0);word(base+13*104+32,z)
            main_check(by_name["ScriptCallbackStoreSlot10To13Separation"],"wrapped signed absolute separation")
    # Clearing effect[9] changes the global pointer before the next reload.
    fixture(0);word(0x02000398,0x02000390);half(0x02000395,39)
    main_check(by_name["ScriptCallbackClampRasterEffectWindow"],"effect writes reload the global pointer")
    fixture(0);word(0x03002D98,0x03002C04-15*104-0x60);byte(0x03002C04,19)
    row=by_name["SetMapBank7RenderPresetB8FA8"]
    equal(task(memory,row["address"]|1),1,"live class after pair clear");call(row["name"]);compare("preset live class alias",ranges)
    fixture(0);word(0x03002D98,controls+0x69-25*104-0x60);ctrl(0x69,255)
    main_check(by_name["ScriptCallbackSpawnBackgroundActorAtTableX"],"velocity reloads constructor-altered control")
    for timers in (bytes(5),b"\x01"*5,b"\xFF"*5):
        for direction in (0,1,2,3,4,5,6,7,8,255):
            fixture(direction);byte(args,0);both(0x030037D0,timers);byte(base+19*104+0x60,direction)
            anim_check(by_name["AnimationCallbackUpdateDirectionalBackgroundActors"],base+19*104,"five timers, raw direction and shared offset")
    # Both modal roots in the window family already have scene snapshot adapters.
    for case in range(16):
        fresh();heap(memory);call("InitializeHeap");equal(map_init(memory,10,10),True,"modal map setup");call("InitializeMapDescriptor",10,10)
        slots=int.from_bytes(native_bytes(0x03002D98,4),"little");both(slots,rng.randbytes(16*104));both(0x02002EB0,rng.randbytes(624))
        byte(0x03001F60,case&1);byte(0x03001F90,(case>>1)&1);byte(0x03002182,128|(case&127))
        for i,index in enumerate((1,0 if case&8 else 2,3,4)):
            byte((0x03002C0C,0x03002274,0x03001F50,0x03002270)[i],index)
            byte(slots+index*104+96,((case+i)%4)+1 if i==0 or case&(1<<(i-1)) else 0)
        for name in ("ScriptCallbackSaveActorsAndChangeMap10_17","ScriptCallbackRestoreActorsAndPreviousMapCEEA8"):
            word(args,by_name[name]["address"]|1);byte(0x03000010,0x1C);word(0x03002C60,0x02020111)
            equal(command(memory,0x1C,args),1,"window modal Main VM");call("MainScriptInvokeCallback",args,limit=4000000)
            compare(name,ranges[:2])
    print("All 56 presentation functions across nine source families passed.")
