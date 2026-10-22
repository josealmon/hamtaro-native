"""Private scanline wave function and typed dispatch comparisons."""
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
