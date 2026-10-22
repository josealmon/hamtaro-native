"""Private bank-2 family comparisons; fixtures are synthetic and never saved."""
from __future__ import annotations
import ctypes as C
import random
import re
import struct
from inventory_reference import body


def check_bank2(*, entries, reference, rom, memory, cpu, bind, call, both,
                fresh, equal, compare, native_bytes, command):
    files = {"src/scripts/map_bank2_scene_control.c",
             "src/scripts/map_bank2_scene_presets.c",
             "src/scripts/map_bank2_height_callbacks.c",
             "src/actors/map_bank2_render_callbacks.c"}
    rows = [row for row in entries if row["source"] in files]
    equal(len(rows), 120, "complete bank-2 family inventory")
    signatures, argument_free = {}, []
    for row in rows:
        source = (reference / row["source"]).read_text(encoding="utf-8")
        match = re.search(r"^(void|u32)\s+" + re.escape(row["name"]) +
                          r"\(([^;{}]*)\)\s*\{", source, re.M)
        if not match:
            raise RuntimeError(f"Bank-2 signature missing: {row['name']}")
        signatures[row["name"]] = match[1], match[2]
        if not row["name"].startswith("AnimationCallback") and not re.search(
                r"\barguments\b", body(source, row["name"])):
            argument_free.append(row)
    main_rows = [row for row in rows if signatures[row["name"]][0] == "u32"
                 and not row["name"].startswith("AnimationCallback")]
    equal(len(main_rows), 70, "bank-2 defined Main return contracts")
    equal(len(argument_free), 116, "bank-2 argument-free task contracts")
    apply = bind("hn_bank2_apply", C.c_int, C.c_void_p, C.c_uint32)
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32,
                C.c_uint32, C.POINTER(C.c_uint32))
    animation = bind("hn_scene_animation_callback", C.c_int, C.c_void_p,
                     C.c_uint32, C.c_uint32, C.c_uint32,
                     C.POINTER(C.c_uint32), C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32,
                C.c_uint, C.POINTER(C.c_uint32))
    rng = random.Random(0x42414E4B32)
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x03007FF0, 4))

    def fixture(case, base=0x02010000):
        fresh()
        both(0x02000000, rom[0x8FE018:0x900EC8])
        both(base, rng.randbytes(40 * 104))
        for i in range(40):
            x, z = ((64 + (i + case) * 31 % 128) << 16) | (case * 997 % 65536), rng.getrandbits(32)
            both(base + i * 104 + 0x18, struct.pack("<III", x, rng.getrandbits(32), z))
            both(base + i * 104 + 0x24, struct.pack("<I", (
                0x3FFFF, 0x40000, 0x40001, 0xFFFC0001, 0xFFFC0000,
                0xFFFBFFFF, 0x80000000, 0x7FFFFFFF)[(case + i) % 8]))
            both(base + i * 104 + 0x60, bytes([(case + i) % 4]))
        both(0x02015000, rng.randbytes(16 * 44))
        both(0x02004520, rng.randbytes(512))
        both(0x02004580, bytes([(0, 1, 19, 20, 21, 254, 255)[case % 7], 0, 255, 16, 240, case & 1]))
        both(0x0200458D, bytes([(0, 4, 5, 255)[case % 4]]))
        both(0x0200455A, bytes([(0, 1, 3, 5, 255)[case % 5]]))
        both(0x0201DC40, rng.randbytes(0x400))
        both(0x0201DC84, struct.pack("<H", (0, 1, 31, 32, 65535)[case % 5]))
        both(0x0201DF8C, struct.pack("<H", (0, 1, 2, 3, 4, 65535)[case % 6]))
        both(0x03000000, struct.pack("<III", *[rng.randrange(1, 30000) for _ in range(3)]))
        both(0x03000820, rng.randbytes(26))
        both(0x03001F64, struct.pack("<H", (0, 1, 1023, 65535)[case % 4]))
        both(0x03002180, rng.randbytes(96))
        both(0x03002170, b"\xA5")
        for i, address in enumerate((0x03002C0C, 0x03001F50, 0x03002C04, 0x03002C00)):
            both(address, bytes([(10, 13, 16, 19)[(i + case) % 4]]))
        # Seed shared pointers last: alternate render bases deliberately overlap
        # class values, so test initialization must not corrupt its own metadata.
        both(0x03002D98, struct.pack("<II", base, 0x02014000))
        both(0x03002DC0, b"\x28\x00\x27")
        both(0x03003070, struct.pack("<I", 0x02015000))
        both(0x03002E10, b"\x10")

    def payload(name, case):
        if name == "ScriptCallbackConfigureScenePanelSlots":
            return bytes([case % 32]) + struct.pack("<HH", (
                0, 15, 16, 32767, 32768, 65535)[case % 6], case * 1999 % 65536)
        if name == "CopyRenderSlotFromBytePairE8A8":
            return bytes([case % 40, (case * 7) % 40])
        return bytes([case & 1]) + rng.randbytes(7)

    for row in argument_free:
        for case in range(32):
            fixture(case)
            equal(apply(memory, row["address"] | 1), 1, f"bank-2 task leaf {row['name']}/{case}")
            call(row["name"], 0)
            compare(f"bank-2 task leaf {row['name']}/{case}", ranges)
    for row in main_rows:
        for case in range(32):
            fixture(case)
            both(0x02020000, payload(row["name"], case))
            consumed = C.c_uint32(99)
            equal(main(memory, row["address"] | 1, 0x02020000, C.byref(consumed)), 1,
                  f"bank-2 Main leaf {row['name']}/{case}")
            equal(consumed.value, call(row["name"], 0x02020000), "bank-2 Main return")
            compare(f"bank-2 Main leaf {row['name']}/{case}", ranges)
    for row in argument_free:
        for case in range(8):
            fixture(case)
            index = case * 13 % 104
            both(0x02019000, rng.randbytes(52))
            both(0x02019010, struct.pack("<I", 0x02020000))
            both(0x02020000, bytes([index, 0xE1]))
            both(0x03001FC0 + index * 4, struct.pack("<I", row["address"] | 1))
            report = (C.c_uint32 * 4)()
            equal(task(memory, 0x02019000, 0xF4, report), 1, "bank-2 task dispatch")
            equal(report[3], call("TaskCommandF4", 0x02019000), "bank-2 task continuation")
            compare(f"bank-2 task ABI {row['name']}/{case}", ranges)
    for row in main_rows:
        for case in range(8):
            fixture(case)
            both(0x02020000, struct.pack("<I", row["address"] | 1) + payload(row["name"], case))
            both(0x03000010, b"\x1C" + bytes(15))
            equal(command(memory, 0x1C, 0x02020000), 1, "bank-2 Main dispatch")
            call("MainScriptInvokeCallback", 0x02020000)
            compare(f"bank-2 Main ABI {row['name']}/{case}", ranges)
    fixture(0)
    both(0x02004580, b"\x00")
    both(0x0201DC84, b"\x00\x00")
    for frame in range(480):
        equal(apply(memory, 0x0802D031), 1, "bank-2 repeated motion update")
        call("ScriptCallbackUpdateSceneSlotMotion", 0)
        compare(f"bank-2 motion counters/random sequence {frame}", ranges)
    # Raised-slot tests exercise equality, subpixels, wrapped sums, and every
    # early-break slot; a hide must retain unrelated render bytes.
    for height in (0xFFE7FFFF, 0xFFE80000, 0xFFE80001, 0x80000000, 0x7FFFFFFF):
        for first_break in range(6):
            fixture(first_break)
            for i in range(5):
                y = 0xFFE80000 if i != first_break else height
                both(0x02010000 + (i + 23) * 104 + 0x1C, struct.pack("<II", y, 0))
            consumed = C.c_uint32(99)
            equal(main(memory, 0x0804B361, 0, C.byref(consumed)), 1, "bank-2 raised-slot boundary")
            equal(consumed.value, call("ScriptCallbackHideMapBank2RaisedSlots", 0), "bank-2 raised return")
            compare("bank-2 raised-slot boundary", ranges)
    for case in range(128):
        fixture(case)
        index = native_bytes(0x03002C04, 1)[0]
        both(0x02010000 + index * 104 + 0x20, struct.pack("<I", (
            0xFFD7FFFF, 0xFFD80000, 0xFFD80001, 0x80000000, 0x7FFFFFFF)[case % 5]))
        consumed, yielded = C.c_uint32(99), C.c_uint32(rng.getrandbits(32))
        both(0x0203FFF0, struct.pack("<I", yielded.value))
        equal(animation(memory, 0x0804B43D, 0, 0, C.byref(consumed), C.byref(yielded)), 1, "bank-2 animation")
        equal(consumed.value, call("AnimationCallbackLatchMapBank2Class2Z", 0, 0, 0x0203FFF0), "bank-2 animation return")
        equal(yielded.value, struct.unpack("<I", cpu.mem_read(0x0203FFF0, 4))[0], "bank-2 full-word yield")
        both(0x0203FFF0, struct.pack("<I", yielded.value))
        compare("bank-2 animation RAM", ranges)
    # Stage aliases a field reset by render initialization; reload it after the
    # four configure calls instead of incrementing its cached entry value.
    for stage in range(4):
        fixture(stage, base=0x0201DF8C - 0x4C)
        both(0x0201DF8C, struct.pack("<H", stage))
        both(0x02020000, bytes([0]) + struct.pack("<HH", 65535, 32768))
        consumed = C.c_uint32(99)
        equal(main(memory, 0x0802D839, 0x02020000, C.byref(consumed)), 1, "bank-2 aliased stage")
        equal(consumed.value, call("ScriptCallbackConfigureScenePanelSlots", 0x02020000), "bank-2 aliased stage return")
        compare("bank-2 aliased panel stage", ranges)
    noops = [row for row in entries if row["source"] == "asm/map_bank2_script_noops.s"]
    equal(len(noops), 3, "bank-2 reviewed assembly leaves")
    for row in noops:
        for case in range(16):
            fixture(case)
            both(0x02020000, struct.pack("<I", row["address"] | 1))
            both(0x03000010, b"\x51" + bytes(15))
            equal(command(memory, 0x51, 0x02020000), 1, "bank-2 Main51 dispatch")
            call("MainScriptDispatchArgumentWord", 0x02020000)
            compare("bank-2 Main51 no-op", ranges)
    print("Complete bank-2 family comparisons passed: 120 C functions, 3 Main51 leaves; story integration pending.")
