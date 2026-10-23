"""Private scanline wave/VCOUNT contracts and independent timed pixel checks."""
from __future__ import annotations
import ctypes as C
import random
import struct


def check_wave(*, entries, rom, memory, bind, call, both, fresh, equal, compare, command):
    rows = [row for row in entries if row["source"] == "src/graphics/scanline_wave_blend.c"]
    equal(len(rows), 3, "complete scanline-wave source family")
    wave = bind("hn_raster_wave", C.c_bool, C.c_void_p)
    hblank = bind("hn_raster_hblank", C.c_bool, C.c_void_p)
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    rng = random.Random(0x57415645424C)
    ranges = ((0x03000000, 0x7000), (0x04000000, 0x60))

    def fixture(case):
        fresh()
        both(0x02000000, rom[0x8FE018:0x900EC8])
        both(0x03002C90, rng.randbytes(0x100))
        both(0x03002DD4, rng.randbytes(2))
        both(0x03002C54, struct.pack("<H", case * 997 % 65536))
        both(0x03001FB4, bytes([case % 256, case * 7 % 256]))
        both(0x04000006, struct.pack("<H", case % 228))

    for line in range(228):
        for position in (-32768, -1, 0, 55, 56, 64, 65, 72, 73, 80, 81, 88, 89, 96, 97, 104, 105, 112, 113, 32767):
            fixture(line)
            both(0x04000006, struct.pack("<H", line))
            both(0x03002C54, struct.pack("<H", (position - line) & 65535))
            equal(wave(memory), True, "scanline wave direct")
            call("UpdateScanlineWaveBlend")
            compare(f"scanline wave thresholds/wrap {line}/{position}", ranges)
    for case in range(512):
        fixture(case)
        both(0x04000006, struct.pack("<H", 227))
        both(0x03001F6C, b"\x01")
        equal(hblank(memory), True, "scanline wave typed HBlank")
        call("UpdateScanlineWaveBlend")
        compare("scanline wave phase/frame-end/IRQ", ranges)
    for row in rows:
        if not row["name"].startswith("ScriptCallback"):
            continue
        for case in range(128):
            fixture(case)
            consumed = C.c_uint32(99)
            equal(main(memory, row["address"] | 1, 0, C.byref(consumed)), 1, "scanline-wave Main callback")
            equal(consumed.value, call(row["name"], 0), "scanline-wave callback consumed")
            compare("scanline-wave enable/disable", ranges)
        for case in range(16):
            fixture(case)
            both(0x02020000, struct.pack("<I", row["address"] | 1))
            both(0x03000010, b"\x1C" + bytes(15))
            equal(command(memory, 0x1C, 0x02020000), 1, "scanline-wave Main ABI")
            call("MainScriptInvokeCallback", 0x02020000)
            compare("scanline-wave Main ABI", ranges)
            fixture(case)
            both(0x02019000, rng.randbytes(52))
            both(0x02019010, struct.pack("<I", 0x02020000))
            both(0x02020000, b"\x01\xE1")
            both(0x03001FC4, struct.pack("<I", row["address"] | 1))
            report = (C.c_uint32 * 4)()
            equal(task(memory, 0x02019000, 0xF4, report), 1, "scanline-wave F4 ABI")
            equal(report[3], call("TaskCommandF4", 0x02019000), "scanline-wave task continuation")
            compare("scanline-wave F4 ABI", ranges + ((0x02019000, 52),))
    print("Three scanline-wave functions and typed Main/F4/HBlank calls passed; full raster timing remains unverified.")


def check_vcount(*,entries,rom,memory,bind,call,both,fresh,equal,compare,command):
    names={"SetVcountIrqEnabled","HandleVcountIrq","VcountNoop","ConfigureVcountAlternatingBlend",
           "UpdateVcountAlternatingBlend","ConfigureVcountClassValueIncrement","IncrementVcountClassValue",
           "ScriptCallbackDisableVcountInterruptCF248","ScriptCallbackDisableVcountInterrupt","ScriptCallbackEnableVcountInterrupt"}
    rows={r['name']:r for r in entries if r['name'] in names}
    equal(set(rows),names,"all ten connected VCOUNT functions")
    families={"src/graphics/vcount_blend_setup.c","src/graphics/vcount_blend.c",
              "src/graphics/vcount_increment_setup.c","src/graphics/vcount_increment.c"}
    equal({r['name'] for r in entries if r['source'] in families},
          names-{"SetVcountIrqEnabled","HandleVcountIrq","VcountNoop","ScriptCallbackDisableVcountInterrupt","ScriptCallbackEnableVcountInterrupt"},
          "complete blend/increment source families")
    p,u=C.c_void_p,C.c_uint32
    enable=bind("hn_video_vcount",None,p,u,u);blend=bind("hn_raster_vcount_blend",C.c_bool,p)
    increment=bind("hn_raster_vcount_increment",C.c_bool,p);irq=bind("hn_raster_vcount",C.c_bool,p)
    main=bind("hn_scene_callback",C.c_int,p,u,u,C.POINTER(u))
    task=bind("hn_probe_task_command",C.c_int,p,u,C.c_uint,C.POINTER(u))
    rng=random.Random(0x56434F554E54);status=0x03002DD4;phase=0x03002C08;selector=0x03002178;value=0x0201DC92
    ranges=((0x03000000,0x7000),(0x04000000,0x60),(0x04000200,12),(0x0201DC40,512))
    def half(a,v):both(a,struct.pack('<H',v&65535))
    def fixture(case):
        fresh();both(0x02000000,rom[0x8FE018:0x900EC8]);both(0x03002C90,rng.randbytes(0x108))
        both(status,rng.randbytes(4));both(phase,bytes([case&255,0xA5]));half(value,case)
        half(0x04000004,rng.getrandbits(16));half(0x04000200,case);half(0x04000208,case&1)
    # Every byte selector/scanline pair with both pending-state paths; caller
    # words also exercise the original u8 entry normalization.
    fixture(0)
    for case in range(65536):
        for toggled in (0,32):
            both(status,struct.pack('<I',0xA55A0000|((case*977+0xC0)^toggled)&65535))
            enabled=(case&255)|0xFACE0000;line=(case>>8)|0xBEEF0000
            enable(memory,enabled,line);call("SetVcountIrqEnabled",enabled,line)
            compare("VCOUNT selector/scanline/pending/full status domain",((status,4),(selector,1),(phase,2)))
    # Full halfword status and counter domains retain reserved/read-only bits,
    # arbitrary shared phases and the u16 increment wrap.
    fixture(0)
    for case in range(65536):
        half(0x04000004,case);both(phase,bytes([case&255,0xA5]))
        equal(blend(memory),True,"VCOUNT alternating blend");call("UpdateVcountAlternatingBlend")
        compare("VCOUNT blend status/phase domain",((phase,2),(0x04000000,0x58)))
        half(value,case);equal(increment(memory),True,"VCOUNT class increment");call("IncrementVcountClassValue")
        compare("VCOUNT counter full u16 domain",((value-2,6),))
    mains={"ConfigureVcountAlternatingBlend","ConfigureVcountClassValueIncrement",
           "ScriptCallbackDisableVcountInterruptCF248","ScriptCallbackDisableVcountInterrupt","ScriptCallbackEnableVcountInterrupt"}
    for name in sorted(mains):
        row=rows[name]
        for case in range(256):
            fixture(case);consumed=u(99)
            equal(main(memory,row['address']|1,0,C.byref(consumed)),1,name+" Main")
            equal(consumed.value,call(name,0),name+" consumed");compare(name,ranges)
        fixture(1);both(0x02020000,struct.pack('<I',row['address']|1));both(0x03000010,b'\x1C'+bytes(15))
        equal(command(memory,0x1C,0x02020000),1,name+" actual Main command")
        call("MainScriptInvokeCallback",0x02020000);compare(name+" Main VM",ranges)
        fixture(1);both(0x02019000,rng.randbytes(52));both(0x02019010,struct.pack('<I',0x02020000))
        both(0x02020000,b'\x01\xE1');both(0x03001FC4,struct.pack('<I',row['address']|1));report=(u*4)()
        equal(task(memory,0x02019000,0xF4,report),1,name+" actual F4 command")
        equal(report[3],call("TaskCommandF4",0x02019000),name+" F4 continuation")
        compare(name+" F4 VM",ranges+((0x02019000,52),))
    for selected in (0,1,2):
        for callback in (rows['VcountNoop']['address'],rows['UpdateVcountAlternatingBlend']['address'],rows['IncrementVcountClassValue']['address']):
            for case in range(256):
                fixture(case);both(selector,bytes([selected]));both(0x0200004C+selected*4,struct.pack('<I',callback|1))
                equal(irq(memory),True,"VCOUNT typed selector/table");call("HandleVcountIrq");compare("VCOUNT IRQ/noop/phase",ranges)
    fixture(0)
    for selected in (3,4,254,255):
        both(selector,bytes([selected]));equal(irq(memory),False,"unsupported VCOUNT selector rejected")
    both(selector,b'\x01');both(0x02000050,struct.pack('<I',0x08012345));equal(irq(memory),False,"unknown VCOUNT callback rejected")
    for name in ('VcountNoop','HandleVcountIrq','UpdateVcountAlternatingBlend','IncrementVcountClassValue','SetVcountIrqEnabled'):
        consumed=u(99);equal(main(memory,rows[name]['address']|1,0,C.byref(consumed)),0,"non-Main VCOUNT ABI rejected")
    print("Ten VCOUNT functions matched: full byte selector/line, halfword status/counter domains, five Main/F4 contracts and typed IRQ table; shared phase and original dirty mask retained.")


def check_vcount_video(*,rom_buffer,rom_size,bind,equal,oracle,runtime_dir):
    from compare_video import VideoOracle,fixture,word
    p,u=C.c_void_p,C.c_uint32;create=bind('hn_probe_game_create',p,p,C.c_size_t,p)
    destroy=bind('hn_probe_game_destroy',None,p);memory_of=bind('hn_probe_game_memory',p,p)
    seed=bind('hn_probe_seed',C.c_int,p,u,p,C.c_size_t);pixels=bind('hn_probe_game_pixels',p,p)
    advance=bind('hn_probe_game_advance',C.c_int,p,u);rng=random.Random(0x56434F554E545050)
    with VideoOracle(oracle,runtime_dir) as reference:
        for case,initial_phase in enumerate((0,1,255)):
            parts=fixture(2+case*6,rng);io=parts[0]
            word(io,0,0x0300);word(io,8,0x1001);word(io,10,0x1100)
            word(io,4,0x6820);word(io,0x200,4);word(io,0x208,1)
            settled=[]
            for control,alpha in ((0x1342,0x0E03),(0x1D42,0x0A06),(0x1D42,0x040C)):
                word(io,0x50,control);word(io,0x52,alpha);settled.append(reference.render(parts))
            word(io,0x50,0x1342);word(io,0x52,0x0E03)
            game=create(rom_buffer,rom_size,None);equal(bool(game),True,"timed VCOUNT native game")
            try:
                memory=memory_of(game)
                for address,payload in zip((0x04000000,0x05000000,0x06000000,0x07000000),parts):
                    equal(seed(memory,address,bytes(payload),len(payload)),1,"synthetic timed PPU fixture")
                for address,payload in ((0x03002C08,bytes([initial_phase])),(0x03002178,b'\x01'),
                                        (0x02000050,struct.pack('<I',0x080CD4E1))):
                    equal(seed(memory,address,payload,len(payload)),1,"timed VCOUNT selector metadata")
                split=104*240*4
                for frame in range(3):
                    equal(advance(game,280896),1,"runtime clock VCOUNT without Main")
                    first=settled[0] if frame==0 else settled[1]
                    second=settled[1] if frame==0 and initial_phase==0 else settled[2]
                    equal(C.string_at(pixels(game),240*160*4),first[:split]+second[split:],
                          f"VCOUNT frame/phase {frame}/{initial_phase}: all pixels and line104 boundary")
            finally:destroy(game)
    print("Nine clocked VCOUNT frames matched independently rendered mGBA bands, including initial phase0/1/255 and line104; this is not full original CPU timing parity.")
