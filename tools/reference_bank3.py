"""Private complete-family checks for bank-3 scenes and animation contracts."""
from __future__ import annotations
import ctypes as C
import random
import re
import struct


def check_bank3_late(*, entries, reference, rom, memory, cpu, bind, call, both,
                     fresh, equal, compare, native_bytes, command):
    rows = [r for r in entries if r["source"] == "src/scripts/map_bank3_late_callbacks.c"]
    equal(len(rows), 31, "complete bank-3 late family inventory")
    text = (reference / "src/scripts/map_bank3_late_callbacks.c").read_text(encoding="utf-8")
    presets = [r for r in rows if re.search(r"void\s+" + re.escape(r["name"]) + r"\(void\)", text)]
    animations = [r for r in rows if r["name"].startswith("AnimationCallback")]
    modals = [r for r in rows if r["name"].startswith("ScriptCallback")]
    equal((len(presets), len(animations), len(modals)), (25, 4, 2), "reviewed bank-3 calling contracts")
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    apply = bind("hn_bank3_task", C.c_int, C.c_void_p, C.c_uint32)
    animation = bind("hn_scene_animation_callback", C.c_int, C.c_void_p, C.c_uint32,
                     C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32), C.POINTER(C.c_uint32))
    audio_init = bind("hn_audio_driver_init", C.c_bool, C.c_void_p)
    heap_init = bind("hn_heap_init", None, C.c_void_p)
    map_init = bind("hn_map_initialize", C.c_bool, C.c_void_p, C.c_uint8, C.c_uint8)
    rng = random.Random(0x42414E4B334C)
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x05000000, 0x400),
              (0x06000000, 0x18000), (0x07000000, 0x400), (0x0E000000, 0x8000))
    base, args, context, yield_address = 0x02010000, 0x02028000, 0x02019000, 0x0203FFF0

    def value(address, size=4):
        return int.from_bytes(native_bytes(address, size), "little")

    def fixture(case):
        fresh();both(0x02000000, rom[0x8FE018:0x900EC8])
        both(base, rng.randbytes(256 * 104));both(0x02018000, rng.randbytes(16 * 44))
        both(0x03002D98, struct.pack("<I", base));both(0x03002DC0, b"\x28")
        both(0x03003070, struct.pack("<I", 0x02018000));both(0x03002E10, b"\x10")
        for i, address in enumerate((0x03002C0C, 0x03001F50, 0x03002C04, 0x03002C00)):
            both(address, bytes([(10, 13, 16, 19)[(case + i) % 4]]))
        both(0x03002182, bytes([128 | (case & 127)]));both(0x03002170, b"\xA5")
        both(0x02004577, bytes([case % 256]));both(0x02001658, bytes([case % 256, case % 256]))
        both(0x03003480, rng.randbytes(256))
        equal(audio_init(memory), True, "bank-3 scene audio initialization")
        call("InitializeAudioDriver", limit=2000000)

    def main_check(row, label, opcode=False):
        if opcode:
            both(args, struct.pack("<I", row["address"] | 1));both(0x03000010, b"\x1C" + bytes(15))
            equal(command(memory, 0x1C, args), 1, "bank-3 Main dispatch " + label)
            call("MainScriptInvokeCallback", args, limit=2000000)
        else:
            consumed = C.c_uint32(99)
            equal(main(memory, row["address"] | 1, args, C.byref(consumed)), 1, "bank-3 modal " + label)
            equal(consumed.value, call(row["name"], args, limit=2000000), "bank-3 modal consumed count")
        compare(row["name"] + " " + label, ranges)

    def task_check(row):
        both(context, rng.randbytes(52));both(context + 16, struct.pack("<I", args))
        both(args, b"\x01\xE1");both(0x03001FC4, struct.pack("<I", row["address"] | 1))
        report = (C.c_uint32 * 4)()
        equal(task(memory, context, 0xF4, report), 1, "bank-3 F4 dispatch " + row["name"])
        equal(report[3], call("TaskCommandF4", context, limit=2000000), "bank-3 F4 continuation")
        compare("bank-3 F4 " + row["name"], ranges)

    for row in presets:
        for case in range(48):
            fixture(case)
            equal(apply(memory, row["address"] | 1), 1, "bank-3 late preset " + row["name"])
            call(row["name"], limit=2000000);compare(row["name"], ranges)
        for case in range(8):
            fixture(case);task_check(row)
        fixture(0);consumed = C.c_uint32(99)
        equal(main(memory, row["address"] | 1, args, C.byref(consumed)), 0, "void late preset rejected as Main")
        compare("void late preset does not change Main state", ranges)
        for case in (0, 1, 254, 255):
            fixture(case);both(0x03002DC0, b"\x01");both(0x03002E10, b"\x01")
            both(0x03002C04, bytes([case]));both(0x03002C00, bytes([case]));both(0x03001F50, bytes([case]))
            equal(apply(memory, row["address"] | 1), 1, "bank-3 inactive mapped preset")
            call(row["name"], limit=2000000);compare("bank-3 inactive mapped preset", ranges)

    def animation_check(row, label):
        consumed, yielded = C.c_uint32(99), C.c_uint32(rng.getrandbits(32))
        both(yield_address, struct.pack("<I", yielded.value))
        equal(animation(memory, row["address"] | 1, 0, 0, C.byref(consumed), C.byref(yielded)), 1,
              "bank-3 animation " + label)
        equal(consumed.value, call(row["name"], 0, 0, yield_address), "bank-3 animation consumed count")
        equal(yielded.value, int.from_bytes(cpu.mem_read(yield_address, 4), "little"), "bank-3 animation yield")
        both(yield_address, struct.pack("<I", yielded.value));compare(row["name"] + " " + label, ranges)

    def active_audio_tracks(case):
        players = []
        for song in (0x36, 0x37):
            record = 0x086EAE34 + song * 8
            index = int.from_bytes(rom[record - 0x08000000 + 4:record - 0x08000000 + 6], "little")
            player = int.from_bytes(rom[0x6EADEC + index * 12:0x6EADEC + index * 12 + 4], "little")
            if player not in players:players.append(player)
        for i, player in enumerate(players):
            tracks = 0x02030000 + i * 1024
            both(player + 8, b"\x0A");both(player + 0x2C, struct.pack("<I", tracks))
            both(player + 0x34, struct.pack("<I", 0 if case % 16 == 15 else 0x68736D53))
            both(player + 0x1C, struct.pack("<HHH", case * 257 % 65536, 0x1234, 0x5678))
            both(tracks, rng.randbytes(10 * 80))
            for track in range(10):
                both(tracks + track * 80, bytes([128 if (case + track) % 4 else 0]))
                both(tracks + track * 80 + 0x15, b"\x55")

    for row in animations:
        for case in range(256):
            fixture(case)
            if "Song" in row["name"]:active_audio_tracks(case)
            for i in range(29, 32):both(base + i * 104 + 0x64, bytes([255 if (case + i) % 3 else case % 32]))
            animation_check(row, "byte counters/mirror/affine allocation")
        fixture(0)
        equal(apply(memory, row["address"] | 1), 0, "animation-only late function rejected by F4")
        consumed = C.c_uint32(99)
        equal(main(memory, row["address"] | 1, args, C.byref(consumed)), 0, "animation-only late function rejected by Main")
        compare("rejected late animation leaves RAM unchanged", ranges)
    scales = next(r for r in animations if r["name"] == "AnimationCallbackCopyMapBank3AffineScales")
    for case in (0, 1, 255, 256, 32767, 32768, 65535):
        for mirror in (0, 1, 254, 255):
            fixture(case);both(base + 27 * 104 + 0x3C, struct.pack("<HH", case, 65535 - case))
            both(0x02004577, bytes([mirror]));both(0x03003480, bytes([0]) * 256)
            for i in range(29, 32):both(base + i * 104 + 0x64, b"\xFF")
            animation_check(scales, "signed-half mirroring/all matrices allocated")
    for row in animations[:2]:
        fixture(0);active_audio_tracks(0);both(0x02001658, b"\x16\x80")
        for frame in range(80):animation_check(row, "continuous audio counter updates")

    save = next(r for r in modals if "Save" in r["name"])
    restore = next(r for r in modals if "Restore" in r["name"])

    def modal_fixture(case, restoring):
        fresh();heap_init(memory);call("InitializeHeap", 0)
        equal(map_init(memory, 10, 10), True, "bank-3 modal source map setup")
        call("InitializeMapDescriptor", 10, 10)
        slots = value(0x03002D98);count = value(0x03002DC0, 1)
        both(slots, rng.randbytes(count * 104));both(0x02002EB0, rng.randbytes(416))
        both(0x03001F60, bytes([case % 2]));both(0x03001F90, bytes([case // 2 % 2]))
        both(0x03002182, bytes([128 | (case & 127)]))
        for i, (selector, index) in enumerate(zip((0x03002C0C, 0x03002274, 0x03001F50, 0x03002270),
                                                 (1, 0 if case & 8 else 2, 0 if case & 16 else 3, 0 if case & 32 else 4))):
            both(selector, bytes([index]));both(slots + index * 104 + 0x60,
                bytes([1 + (case + i) % 4 if i == 0 or case & (1 << (i - 1)) else 0]))
        if restoring:main_check(save, "restore preparation")
        both(0x03002C60, struct.pack("<I", 0x02028111))

    for row in modals:
        for case in range(64):
            modal_fixture(case, row == restore);main_check(row, str(case))
        for case in range(8):
            modal_fixture(case, row == restore);main_check(row, "Main ABI", opcode=True)
            modal_fixture(case, row == restore);task_check(row)
    print("31 late bank-3 functions, 27 F4 targets, four animation and two Main contracts passed; full area integration remains unverified.")
