"""Private whole-family counter/sequence comparisons; no captured fixtures are saved."""
from __future__ import annotations
import ctypes as C
import itertools
import random
import struct


def check_counter_scene(*, entries, rom, memory, bind, call, both, fresh,
                        equal, compare, native_bytes, command):
    rows = [r for r in entries if r["source"] == "src/scripts/map10_4_counter_sequence.c"]
    equal(len(rows), 13, "complete counter/sequence source inventory")
    apply = bind("hn_counter_scene_apply", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32)
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    initialize = bind("hn_audio_driver_init", C.c_bool, C.c_void_p)
    rng = random.Random(0x434E545253434E)
    classes, controls, tiles = 0x0201DC40, 0x02004520, 0x02001B9C
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x03007FF0, 4), (0x04000000, 0x60))
    fresh()
    both(0x02000000, rom[0x8FE018:0x900EC8])
    equal(initialize(memory), True, "counter scene audio fixture")
    # Driver behavior is compared separately; here both machines receive the same
    # initialized private state, including its nonexecuted callback data image.
    baseline = [(a, native_bytes(a, size)) for a, size in ranges[:3]]

    def fixture(case, base=0x02010000, graphics=0x02018000):
        fresh()
        for address, data in baseline:
            both(address, data)
        both(base, rng.randbytes(40 * 104))
        both(graphics, rng.randbytes(16 * 44))
        both(tiles, rng.randbytes(100))
        both(classes, rng.randbytes(128))
        both(controls, rng.randbytes(256))
        both(classes + 24, struct.pack("<H", (0, 1, 2, 3, 4, 5, 6, 65535)[case % 8]))
        both(classes + 26, struct.pack("<H", (0, 4, 7, 8, 13, 14, 51, 52, 53, 54, 55, 56, 57, 68, 69, 128, 65535)[case % 17]))
        both(classes + 28, struct.pack("<H", case * 513 % 65536))
        both(classes + 0x44, struct.pack("<H", case * 997 % 65536))
        both(classes + 0x48, struct.pack("<H", (0, 1, 15, 16, 47, 48, 95, 96, 65535)[case % 9]))
        both(controls + 0x60, bytes([case % 5, case % 12]))
        both(controls + 0x79, bytes([case % 256]))
        both(0x03002C90, rng.randbytes(0x100))
        both(0x03002182, bytes([128 | (case & 127)]))
        both(0x03002170, b"\xA5")
        both(0x03001F50, bytes([case % 40]))
        both(0x03002D98, struct.pack("<I", base))
        both(0x03002DC0, b"\x28\x00\x27")
        both(0x03003070, struct.pack("<I", graphics))
        both(0x03002E10, b"\x10")
        both(base + 23 * 104 + 4, struct.pack("<I", 0x088F81A3 if case & 1 else 0x088F80F1))
        both(base + 28 * 104 + 4, struct.pack("<I", 0x088F7AF3 if case & 2 else 0x088F7A36))

    def direct(row, word=0x02020000):
        equal(apply(memory, row["address"] | 1, word), 1, row["name"] + " native")
        equal(call(row["name"], word), 0, row["name"] + " reference return")
        compare(row["name"], ranges)

    for row in rows:
        for case in range(128):
            fixture(case)
            try:
                direct(row, case if row["name"] == "UpdateMap10_4StageIndicators" else 0x02020000)
            except RuntimeError as error:
                raise RuntimeError(f"Counter scene direct case {case}: {error}") from error
    draw = next(r for r in rows if r["name"] == "ScriptCallbackDrawMap10_4Counter")
    fixture(0)
    for number in range(65536):
        both(classes + 0x44, struct.pack("<H", number))
        both(controls + 0x60, bytes([number % 5]))
        equal(apply(memory, draw["address"], 0), 1, "counter complete u16 domain")
        call(draw["name"], 0)
        compare("counter digits/byte truncation", ((tiles, 100),))
    advance = next(r for r in rows if r["name"] == "ScriptCallbackAdvanceMap10_4Sequence")
    cases = itertools.product((0, 2, 4, 5, 6, 7, 9, 10), (0, 1, 2, 3, 4, 5, 65535),
                              (0, 4, 7, 8, 13, 14, 51, 52, 53, 54, 55, 56, 57, 68, 69, 128, 65535),
                              (0, 1, 15, 47, 95, 65535))
    for case, (phase, stage, scroll, counter) in enumerate(cases):
        fixture(case)
        both(controls + 0x61, bytes([phase]))
        both(classes + 24, struct.pack("<H", stage))
        both(classes + 26, struct.pack("<H", scroll))
        both(classes + 0x48, struct.pack("<H", counter))
        direct(advance)
    for base in (classes - 23 * 104, controls - 23 * 104, tiles - 23 * 104):
        for row in rows:
            for case in range(8):
                fixture(case, base)
                direct(row, case if row["name"] == "UpdateMap10_4StageIndicators" else 0x02020000)
    for row in rows:
        if row["name"].startswith("ScriptCallback") or row["name"] == "Map10_4NoopC2C38":
            for case in range(16):
                fixture(case)
                consumed = C.c_uint32(99)
                equal(main(memory, row["address"] | 1, 0, C.byref(consumed)), 1, "counter scene Main adapter")
                equal(consumed.value, call(row["name"], 0), "counter scene Main consumed")
                compare("counter scene Main", ranges)
                fixture(case)
                both(0x02020000, struct.pack("<I", row["address"] | 1))
                both(0x03000010, b"\x1C" + bytes(15))
                equal(command(memory, 0x1C, 0x02020000), 1, "counter scene Main ABI")
                call("MainScriptInvokeCallback", 0x02020000)
                compare("counter scene Main ABI", ranges)
        if row["name"] != "UpdateMap10_4StageIndicators":
            for case in range(8):
                fixture(case)
                both(0x02019000, rng.randbytes(52))
                both(0x02019010, struct.pack("<I", 0x02020000))
                both(0x02020000, b"\x01\xE1")
                both(0x03001FC4, struct.pack("<I", row["address"] | 1))
                report = (C.c_uint32 * 4)()
                equal(task(memory, 0x02019000, 0xF4, report), 1, "counter scene F4 ABI")
                equal(report[3], call("TaskCommandF4", 0x02019000), "counter scene F4 continuation")
                compare("counter scene F4", ranges + ((0x02019000, 52),))
    fixture(0)
    both(controls + 0x60, b"\x05")
    before = native_bytes(tiles, 100)
    equal(apply(memory, draw["address"], 0), 2, "unreviewed incoming R6 selector rejected")
    equal(native_bytes(tiles, 100), before, "invalid counter selector preserves tile banks")
    print("Thirteen counter/sequence functions and typed contracts passed; complete Select menu integration remains unverified.")
