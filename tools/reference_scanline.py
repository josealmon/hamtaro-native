"""Private HBlank/window contracts; original resources stay in the caller's ROM."""
from __future__ import annotations
import ctypes as C
import random
import struct
from unicorn import UC_HOOK_MEM_READ


def check_scanlines(*,entries,rom,memory,bind,call,both,fresh,equal,compare,native_bytes,command,cpu,
                    video_oracle=None,runtime_dir=None,rom_buffer=None):
    from inventory_reference import dispatches
    p,u=C.c_void_p,C.c_uint32
    hblank=bind('hn_raster_hblank',C.c_bool,p)
    main=bind('hn_scene_callback',C.c_int,p,u,u,C.POINTER(u))
    profile=bind('hn_raster_scanline_profile',C.c_bool,p,C.c_uint8)
    reset_phase=bind('hn_raster_scanline_phase_reset',None,p)
    initialize=bind('hn_heap_init',None,p);allocate=bind('hn_heap_allocate',u,p,u,C.c_bool)
    enable=bind('hn_video_hblank',None,p,u)
    task=bind('hn_probe_task_command',C.c_int,p,u,C.c_uint,C.POINTER(u))
    rows={r['name']:r for r in entries};names={r['address']&~1:r['name'] for r in entries}
    table=[struct.unpack_from('<I',rom,0x8FE018+i*4)[0]&~1 for i in range(19)]
    equal(len(set(table)),18,'all nineteen HBlank selectors and eighteen distinct targets')
    bindings=dispatches()
    equal(all('hblank' in bindings.get(a,[]) for a in table),True,'all original HBlank table targets bound')
    mains=('ScriptCallbackAdjustScanlineWindowParameters','ScriptCallbackConfigureScanlineWindow',
           'ScriptCallbackDisableScanlineWindow','ConfigureScanlineWaveScroll')
    equal(all('main' in bindings[rows[n]['address']] for n in mains),True,'four reviewed window Main contracts')
    source_families={
        'src/graphics/scanline_windows.c':4,'src/graphics/scanline_steps.c':2,
        'src/graphics/scanline_circle.c':1,'src/graphics/scanline_radius.c':1,
        'src/graphics/scanline_wave_scroll.c':1,'src/graphics/scanline_wave_setup.c':1,
        'src/graphics/scanline_secondary_noop.c':1,'src/scripts/script_callbacks_scanline_window.c':4,
        'src/graphics/graphics_script_raster_bands.c':1,'src/graphics/graphics_script_raster_scrolled_bands.c':1,
        'src/graphics/graphics_script_raster_steps.c':1,'src/graphics/graphics_script_raster_scrolled_steps.c':1,
        'src/graphics/graphics_script_raster_alternating.c':1,'src/graphics/graphics_script_raster_edge.c':1}
    for source,count in source_families.items():equal(sum(r['source']==source for r in entries),count,'complete family '+source)
    controls=0x02004520;phase=0x03001FB4;transition=0x03002C08;windows=0x03002164
    radius_root=0x03003DB0;accumulator=0x03002C54;effect_root=0x02000398;args=0x0201F801;ctx=0x0201F000
    rng=random.Random(0x5343414E4C494E45)
    ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x04000000,0x400))
    def byte(a,v):both(a,bytes([v&255]))
    def half(a,v):both(a,struct.pack('<H',v&65535))
    def word(a,v):both(a,struct.pack('<I',v&0xFFFFFFFF))
    def stack():
        # The radius helper leaves local byte0 unwritten. Supply defined zero
        # inactive ARM scratch, never claim arbitrary stack-history identity.
        cpu.mem_write(0x03007000,bytes(0xB80))
    def fixture(case):
        fresh();both(0x02000000,rom[0x8FE018:0x900EC8]);initialize(memory);call('InitializeHeap',0)
        effect=allocate(memory,258,True);equal(effect,call('AllocateHeap',258),'scanline effect allocation')
        radial=allocate(memory,512,True);equal(radial,call('AllocateHeap',512),'scanline profile allocation')
        both(effect,rng.randbytes(258));word(effect_root,effect);word(radius_root,radial);both(radial,rng.randbytes(512))
        both(controls,rng.randbytes(512));both(phase,rng.randbytes(4));both(transition,rng.randbytes(4))
        both(windows,rng.randbytes(4));half(accumulator,case*977)
        both(0x03002C90,rng.randbytes(0x108));half(0x04000004,case*313);half(0x04000006,case%228)
        half(0x04000000,case*97);half(0x04000200,case);half(0x04000208,case&1)
        both(args,bytes([case&255,0xA5])+bytes(14))
        # Valid circular graphics-effect state; remaining bytes retain noise.
        half(effect+5,20);byte(effect+7,40);byte(effect+9,1);half(effect+10,0);byte(effect+12,case%160);half(effect+13,0)
        return effect,radial
    def irq(label):
        stack();equal(hblank(memory),True,'HBlank native '+str(label));call('HandleHblankIrq',limit=400000)
        compare('HBlank '+str(label),ranges)
    def invoke(name,label,vm=False):
        stack();address=rows[name]['address']|1
        if vm:
            payload=struct.pack('<I',address)+native_bytes(args,16);both(args,payload);byte(0x03000010,0x1C)
            equal(command(memory,0x1C,args),1,name+' actual Main command');call('MainScriptInvokeCallback',args,limit=400000)
        else:
            consumed=u(99);equal(main(memory,address,args,C.byref(consumed)),1,name+' native '+str(label))
            equal(consumed.value,call(name,args,limit=400000),name+' consumed bytes')
        compare(name+' '+str(label),ranges)

    # Every real scanline, including 159/160/227, reaches the live ROM-backed
    # dispatcher. Zero clears the real status bit rather than invoking a noop.
    for selector in range(19):
        for line in range(228):
            fixture(selector*228+line);byte(0x03001F6C,selector);half(0x04000006,line)
            byte(phase,line);byte(phase+1,(0,1,2,3,7,159,160,255)[line%8]);byte(phase+2,line*13)
            byte(transition,line*7);byte(transition+1,line*3);byte(transition+2,line%200);byte(transition+3,1)
            irq((selector,names[table[selector]],line))
    # Full phase-byte domains at both active and frame boundaries, with zero
    # periods, high widths and byte wraps retained by the original callbacks.
    for selector in (5,6,7,8,9,10,11,12,13,14,15,17,18):
        for case in range(256):
            for line in (0,159,160,227):
                effect,_=fixture(case);byte(0x03001F6C,selector);half(0x04000006,line)
                both(phase,bytes([case,case,case,0xA5]));both(transition,bytes([case,case,case,case]))
                if selector==11 and line==160:byte(transition+2,case%200)
                byte(effect+3,case);byte(effect+4,case);byte(effect+5,case);byte(effect+6,case)
                if selector in (14,15,18):half(effect+7,(0,239,240,241,65535)[case%5])
                irq(('byte boundary',selector,case,line))
    # The compiler's actual division leaves return zero on a zero divisor.
    for name in ('__divsi3','__modsi3'):
        for value in (0,1,160,0xFFFFFFFF):equal(call(name,value,0),0,name+' zero divisor')

    _,radial=fixture(0)
    for old in range(256):
        for increment in (0,1,2,127,128,129,254,255):
            for fraction in (0,1,127,255):
                both(transition,bytes([120,80,old,increment]));half(windows+2,fraction<<8);half(accumulator,0)
                both(radial,b'\xCC'*300);stack()
                equal(profile(memory,0),True,'scanline radius profile');call('UpdateScanlineRadiusProfile',limit=400000)
                compare('radius domains/forward copy including last byte',((radial,300),(transition,4),(accumulator,2)))
    for case in range(256):
        fixture(case);stack();reset_phase(memory);call('ResetScanlineWindowPhase');compare('typed phase reset',ranges)
        for name in ('ScriptCallbackAdjustScanlineWindowParameters','ScriptCallbackDisableScanlineWindow','ConfigureScanlineWaveScroll'):
            fixture(case);invoke(name,case)
    for mode in range(9):
        for case in range(256):
            fixture(case);byte(controls+0x61,mode);invoke('ScriptCallbackConfigureScanlineWindow',(mode,case))
    # Defined BIOS-store site before allocation. The synthetic oracle page holds
    # no firmware and is never executed; its ignored writes are not state inputs.
    cpu.mem_map(0,0x4000)
    try:
        for mode in (6,7,8):
            fixture(mode);word(radius_root,0);byte(controls+0x61,mode)
            invoke('ScriptCallbackConfigureScanlineWindow',('initial null profile',mode))
    finally:cpu.mem_unmap(0,0x4000)
    for name in mains:
        fixture(1);byte(controls+0x61,1);invoke(name,'Main VM',True)
        fixture(1);byte(controls+0x61,1)
        if name in ('ScriptCallbackDisableScanlineWindow','ConfigureScanlineWaveScroll'):
            both(ctx,rng.randbytes(52));word(ctx+0x10,args);both(args,b'\x01\xE1');word(0x03001FC4,rows[name]['address']|1)
            report=(u*4)();stack();equal(task(memory,ctx,0xF4,report),1,name+' actual F4')
            equal(report[3],call('TaskCommandF4',ctx,limit=400000),name+' F4 continuation');compare(name+' F4',ranges)
        else:
            both(ctx,bytes(52));word(ctx+0x10,args);word(0x03001FC4,rows[name]['address']|1)
            equal(task(memory,ctx,0xF4,(u*4)()),0,name+' argument callback rejected as void F4')
    # Raw aliases exercise captured pointers, live selectors and source reset order.
    for argument in (phase,transition,windows,controls+0x61):
        fixture(1);byte(controls+0x61,4);byte(argument,5)
        consumed=u();stack();equal(main(memory,rows['ScriptCallbackConfigureScanlineWindow']['address']|1,argument,C.byref(consumed)),1,'live argument alias')
        equal(consumed.value,call('ScriptCallbackConfigureScanlineWindow',argument),'alias consumed')
        equal(consumed.value,2,'alias consumes two bytes')
        compare('window configure argument alias',ranges)
    for selector in (12,13,14,15,17,18):
        for target in (0x03002C70-5,0x03002C70-6):
            fixture(1);both(target,bytes([0,15,15,1,8,0,40,80,0])+bytes(16))
            word(effect_root,target);byte(0x03001F6C,selector);half(0x04000006,0)
            irq(('effect aliases live scanline word',selector,target))
    for selector in (12,13,17):
        fixture(1);both(0x02000300,bytes(24));target=effect_root-6
        both(target,bytes(24));byte(target+3,1);word(effect_root,target);byte(0x03001F6C,selector);half(0x04000006,160)
        irq(('effect store changes its root',selector))
    for destination in (transition,windows,radius_root):
        fixture(1);word(radius_root,destination);both(transition,bytes([120,80,12,0]));half(windows+2,0);half(accumulator,0)
        stack();equal(profile(memory,0),True,'profile destination aliases live globals');call('UpdateScanlineRadiusProfile')
        compare('profile captured destination and final radius store',ranges)
    for retained in (1,0xA5,255):
        _,radial=fixture(1);both(transition,bytes([120,80,12,0]));half(windows+2,0);half(accumulator,0);stack();read=[]
        def supply_retained(uc,access,address,size,value,context):
            if size==1 and not read:
                read.append(address);uc.mem_write(address,bytes([retained]))
        hook=cpu.hook_add(UC_HOOK_MEM_READ,supply_retained,begin=0x03007000,end=0x03007B7F)
        try:
            equal(profile(memory,retained),True,'explicit retained scratch byte');call('UpdateScanlineRadiusProfile')
        finally:cpu.hook_del(hook)
        equal(len(read),1,'original profile reads one defined retained byte')
        compare('explicit retained positive-radius byte',((radial,32),(transition,4),(accumulator,2)))
    for argument_kind in ('payload','root'):
        _,radial=fixture(1);byte(controls+0x61,8);argument=radial if argument_kind=='payload' else radius_root
        if argument_kind=='payload':byte(argument,5)
        consumed=u();stack();equal(main(memory,rows['ScriptCallbackConfigureScanlineWindow']['address']|1,argument,C.byref(consumed)),1,'profile live argument')
        equal(consumed.value,call('ScriptCallbackConfigureScanlineWindow',argument),'profile update/allocation argument order')
        compare('profile update before argument read before allocation',ranges)
    fixture(0)
    for selector in range(256):
        for status in (0,16,64,128,240,255):
            byte(0x03002DD4,status);enable(memory,selector|0xFACE0000);call('SetHblankIrqEnabled',selector|0xFACE0000)
            compare('HBlank raw selector and pending status',((0x03002DD4,2),(0x03001F6C,1)))
    print('All 19 HBlank selectors and 26 connected functions compared, including complete window/step/circle/wave setup families, radius scratch adaptation, Main/F4 contracts and source heap/DMA order; CPU timing remains separate.')
    if video_oracle is None:
        print('Clocked scanline pixels skipped; supply --video-oracle to include them.');return
    from compare_video import VideoOracle,fixture as video_fixture,word as video_word
    create=bind('hn_probe_game_create',p,p,C.c_size_t,p);destroy=bind('hn_probe_game_destroy',None,p)
    memory_of=bind('hn_probe_game_memory',p,p);seed=bind('hn_probe_seed',C.c_int,p,u,p,C.c_size_t)
    pixels=bind('hn_probe_game_pixels',p,p);advance=bind('hn_probe_game_advance',C.c_int,p,u)
    read=bind('hn_probe_read',C.c_int,p,u,p,C.c_size_t)
    write16=bind('hn_write16',None,p,u,C.c_uint16);write32=bind('hn_write32',None,p,u,u)
    mutable=(0x18,0x1C,0x40,0x42,0x50,0x52)
    def source_half(address):return struct.unpack('<H',bytes(cpu.mem_read(address,2)))[0]
    def source_word(address):return struct.unpack('<I',bytes(cpu.mem_read(address,4)))[0]
    # The callback oracle supplies per-line register values; an independent mGBA
    # PPU draws those rows. DMA precedes the IRQ at each visible HBlank boundary.
    # This checks clock/IRQ/DMA/PPU integration, not original ARM instruction timing.
    with VideoOracle(video_oracle,runtime_dir,scanlines=True) as renderer:
        for case in range(23):
            combined=case==19
            selector=7 if combined else (14,15,18)[case-20] if case>=20 else case
            effect,radial=fixture(selector);parts=video_fixture(20,rng);io=parts[0]
            video_word(io,0,0x2F00)
            for layer,priority in enumerate((2,0,1,3)):video_word(io,8+layer*2,((16+layer)<<8)|priority)
            video_word(io,0x40,0x28D0);video_word(io,0x42,0);video_word(io,0x44,160)
            video_word(io,0x48,0x2F);video_word(io,0x4A,0x28);video_word(io,0x4C,0)
            video_word(io,0x50,0x1D42);video_word(io,0x52,0x0808)
            if case>=20:
                # mGBA 0.10.5's 4bpp flipped-tile suffix selects the wrong
                # nibble when a window clips before the tile's right edge.
                # Keep the 40 earlier flipped frames; these six extra completion
                # frames use unflipped maps. Native PPU unit tests retain the
                # one-pixel flipped boundary with analytically known colors.
                for at in range(16*2048,20*2048,2):
                    video_word(parts[2],at,struct.unpack_from('<H',parts[2],at)[0]&~0x400)
            for address,payload in zip((0x04000000,0x05000000,0x06000000,0x07000000),parts):both(address,bytes(payload))
            both(phase,b'\0\x08\x04\0');both(transition,bytes([40,4,32,1]));half(windows,0);half(windows+2,180);half(accumulator,0)
            if 5<=selector<=11:
                byte(args,5);byte(controls+0x61,selector-5);byte(controls+0x63,4);byte(controls+0x64,8);byte(controls+0x65,8)
                invoke('ScriptCallbackConfigureScanlineWindow',('timed setup',selector))
                if selector<=8:byte(transition,40)
                if selector in (9,10):half(windows+2,180)
            elif selector==4:
                byte(controls+0x60,8);invoke('ConfigureScanlineWaveScroll','timed setup')
            elif selector>=12:
                both(effect,bytes(258));byte(effect,0);byte(effect+1,15);byte(effect+2,15)
                byte(effect+3,8);byte(effect+4,8);byte(effect+5,0);byte(effect+6,232);half(effect+7,250)
                if selector in (14,15,18):
                    byte(effect+5,4);byte(effect+6,0);half(effect+7,315 if case>=20 else 180)
                if selector==16:
                    byte(effect+3,120);byte(effect+4,80);half(effect+5,40);byte(effect+7,41);byte(effect+9,1)
                    stack();equal(bind('hn_raster_profile',C.c_bool,p)(memory),True,'timed raster circle profile')
                    call('UpdateRasterRadiusProfile');compare('timed raster circle profile',ranges)
                word(0x040000B0,0x03002C70);word(0x040000B4,0x04000040);half(0x040000B8,1);half(0x040000BA,0xA340)
            byte(0x03001F6C,selector);half(0x04000004,16);half(0x04000006,0);half(0x04000200,2);half(0x04000208,1)
            if combined:
                byte(0x03002178,1);word(0x02000050,rows['UpdateVcountAlternatingBlend']['address']|1)
                half(0x04000004,0x6830);half(0x04000200,6)
            # Seed the compiled runtime with synthetic fixtures and the original
            # source's setup; no game code is executed by the PPU process.
            initial_parts=tuple(bytes(cpu.mem_read(a,n)) for a,n in ((0x04000000,1024),(0x05000000,1024),(0x06000000,98304),(0x07000000,1024)))
            game=create(rom_buffer,len(rom),None);equal(bool(game),True,'clocked window native game')
            try:
                game_memory=memory_of(game)
                for address,size in ((0x02000000,0x40000),(0x03000000,0x7000),(0x04000000,1024),(0x05000000,1024),(0x06000000,98304),(0x07000000,1024)):
                    payload=bytes(cpu.mem_read(address,size));equal(seed(game_memory,address,payload,len(payload)),1,'clocked window fixture')
                dma=source_half(0x040000BA)
                if dma&0x8000:
                    write16(game_memory,0x040000BA,0);write32(game_memory,0x040000B0,source_word(0x040000B0))
                    write32(game_memory,0x040000B4,source_word(0x040000B4));write16(game_memory,0x040000B8,1);write16(game_memory,0x040000BA,dma)
                initial_registers=b''.join(initial_parts[0][a:a+2] for a in mutable)*160
                baseline_pixels=renderer.render(initial_parts,initial_registers);changed=False
                for frame in range(2):
                    line_registers=bytearray()
                    for line in range(228):
                        cpu.mem_write(0x04000006,struct.pack('<H',line))
                        if combined and source_half(0x04000004)&32 and line==source_half(0x04000004)>>8:
                            stack();call('HandleVcountIrq')
                        if line<160:
                            registers=b''.join(bytes(cpu.mem_read(0x04000000+a,2)) for a in mutable)
                            observed=bytearray()
                            for address in mutable:
                                value=C.create_string_buffer(2)
                                equal(read(game_memory,0x04000000+address,value,2),1,'clocked window register read')
                                observed+=value.raw
                            equal(bytes(observed),registers,f'per-line registers: case{case}/selector{selector}/frame{frame}/line{line}')
                            line_registers+=registers
                            control=source_half(0x040000BA)
                            if control&0x8000:
                                equal(control,0xA340,'reviewed fixed-source repeating HBlank DMA')
                                source=source_word(0x040000B0);destination=source_word(0x040000B4)
                                cpu.mem_write(destination,bytes(cpu.mem_read(source,2)))
                        if source_half(0x04000004)&16:
                            stack();call('HandleHblankIrq',limit=400000)
                        equal(advance(game,1232),1,'runtime clock scanline IRQs')
                    expected=renderer.render(initial_parts,bytes(line_registers))
                    changed|=expected!=baseline_pixels
                    if selector in (9,10,14,15,18) and frame==0 and case<20:
                        equal(len({bytes(line_registers[y*12+4:y*12+6]) for y in range(160)})>2,True,'timed stepped windows contain multiple visible widths')
                    if selector==1 and frame==0:
                        fixed_alpha=bytearray(line_registers)
                        for y in range(160):struct.pack_into('<H',fixed_alpha,y*12+10,0x0808)
                        equal(expected!=renderer.render(initial_parts,bytes(fixed_alpha)),True,'timed wave fixture visibly exercises per-line alpha')
                    actual=C.string_at(pixels(game),240*160*4)
                    equal(actual,expected,f'all pixels: case{case}/selector{selector}/frame{frame}/VCOUNT{combined}, source callbacks plus independent PPU')
                equal(changed,selector not in (0,2,3),'each non-noop effect changes visible pixels')
            finally:destroy(game)
    print('46 clocked native frames matched per-line source state and independent mGBA PPU output across all 19 selectors, including HBlank DMA, wrap, scrolling, circle initialization, stepped widths, effect completion and combined VCOUNT/window phases; original CPU latency remains unverified.')
