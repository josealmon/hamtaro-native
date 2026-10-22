"""Private complete-family checks for bank-3 scenes and animation contracts."""
from __future__ import annotations
import ctypes as C
import random
import re
import struct
from unicorn.arm_const import UC_ARM_REG_R4


def check_bank3_games(*, entries, rom, memory, cpu, bind, call, both, fresh, equal,
                     compare, native_bytes, command):
    from inventory_reference import body
    from pathlib import Path
    root = Path(__file__).resolve().parents[1]
    source = (root / "src/game/bank3_games.c").read_text()
    addresses = lambda name: {int(x, 16) for x in re.findall(r"case (0x[0-9A-Fa-f]+):", body(source, name))}
    main_addresses, animation_addresses = addresses("hn_bank3_games_callback"), addresses("hn_bank3_games_animation")
    task_addresses = addresses("hn_bank3_games_task")
    main_rows = [r for r in entries if r["address"] in main_addresses]
    animation_rows = [r for r in entries if r["address"] in animation_addresses]
    equal((len(main_rows), len(animation_rows), len(task_addresses)), (28, 7, 25), "reviewed bank-3 game contracts")
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    animation = bind("hn_scene_animation_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32,
                     C.c_uint32, C.POINTER(C.c_uint32), C.POINTER(C.c_uint32))
    apply = bind("hn_bank3_task", C.c_int, C.c_void_p, C.c_uint32)
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    phase = bind("hn_bank3_phase_kind", C.c_uint32, C.c_void_p, C.c_uint32, C.c_uint32)
    overlap = bind("hn_bank3_boxes_overlap", C.c_uint32, *([C.c_uint8] * 8))
    square_root = bind("hn_bios_sqrt", C.c_uint32, C.c_uint32)
    entry_configure = bind("hn_bank3_entry_configure", C.c_bool, C.c_void_p, C.c_uint32)
    entry_spawn = bind("hn_bank3_entry_spawn", C.c_bool, C.c_void_p, C.c_uint32)
    entry_motion = bind("hn_bank3_entry_motion", C.c_bool, C.c_void_p, C.c_uint8)
    audio_init = bind("hn_audio_driver_init", C.c_bool, C.c_void_p)
    heap_init = bind("hn_heap_init", None, C.c_void_p)
    tasks_init = bind("hn_task_contexts_init", None, C.c_void_p)
    rng = random.Random(0x3347414D4553)
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x05000000, 0x400),
              (0x06000000, 0x18000), (0x07000000, 0x400), (0x0E000000, 0x8000))
    base, classes, controls, args, slot, output = 0x02010000, 0x0201DC40, 0x02004520, 0x02028000, 0x02010A28, 0x0203FFF0

    def fixture(case):
        fresh();both(0x02000000, rom[0x8FE018:0x900EC8])
        heap_init(memory);call("InitializeHeap");tasks_init(memory);call("InitializeTaskContexts")
        both(base, rng.randbytes(256 * 104));both(0x02018000, rng.randbytes(16 * 44))
        both(controls, rng.randbytes(0x200));both(classes, rng.randbytes(1024))
        both(0x03000000, struct.pack("<III", rng.randrange(1, 30000), rng.randrange(1, 30000), case % 227 + 1))
        both(0x03002D98, struct.pack("<I", base));both(0x03002DC0, b"\x28")
        both(0x03003070, struct.pack("<I", 0x02018000));both(0x03002E10, b"\x10")
        both(0x02020000, rng.randbytes(16 * 80));both(0x03002E08, struct.pack("<I", 0x02020000));both(0x03002C80, b"\x10")
        both(0x03002180, rng.randbytes(96));both(0x03002182, bytes([128 | (case & 127)]))
        for address, value in zip((0x03002C0C, 0x03001F50, 0x03002C04, 0x03002C00), (10, 13, 16, 19)):both(address, bytes([value]))
        both(controls + 0x63, bytes([case % 6, (case + 1) % 6, (case + 2) % 6, case % 2]))
        both(controls + 0x60, bytes([(0, 1, 2, 3, 254, 255)[case % 6]]))
        both(controls + 0x93, bytes([4 if case % 2 else 0]));both(controls + 0x1D0, bytes([case % 2, case % 256]))
        both(controls + 0xEB, bytes([case % 5]))
        both(classes + 66 * 2, struct.pack("<H", 0 if case % 2 else case * 1021 % 65536))
        both(classes + 224 * 2, struct.pack("<H", (0, 1, 59, 60, 3599, 3600, 3601, 3659, 3660, 65535)[case % 10]))
        both(classes + 225 * 2, struct.pack("<H", (0, 1, 9, 10, 99, 100, 999, 1000, 9999, 65535)[case % 10]))
        both(classes + 11 * 2, struct.pack("<H", (0, 1, 49, 50, 51, 65535)[case % 6]))
        both(classes + 6 * 2, struct.pack("<H", (0, 1, 49, 50, 51, 65535)[case // 6 % 6]))
        both(controls + 0x1A0, bytes([case % 3, case % 4, case // 4 % 4, case % 256, case * 7 % 256]))
        both(controls + 0x1E0, bytes([(0, 19, 20, 255)[case % 4], (0, 4, 5, 255)[case // 4 % 4]]))
        both(0x03003DB4, struct.pack("<H", (0, 64, 112, 160, 199, 65535)[case % 6]))
        both(0x03001FBC, struct.pack("<H", (0, 1, 2, 3, 65535)[case % 5]))
        both(0x03001F64, struct.pack("<H", 1023));both(0x03000822, struct.pack("<H", case // 4 & 1))
        for index in range(3):
            both(0x03003EA0 + index * 8, bytes([(case + index) % 4, 1 + (case // 4 + index) % 4,
                 (case + index) % 4, case % 3, (case // 4 + index) % 6, 0xA5]) +
                 struct.pack("<H", (0, 1, 20, 21, 24, 40, 41, 45, 46, 65535)[(case + index) % 10]))
        for index in range(10):
            both(0x03003DE0 + index * 8, bytes([1 if case % 8 == 7 or (case + index) % 3 else 0,
                 1 + (case // 4 + index) % 4, (case + index) % 4, case % 3,
                 (0, 1, 2, 16, 255)[(case + index) % 5], 0x5A]) + struct.pack("<H", case * 257 % 65536))
            x = (0x2FFFFF, 0x300000, 0xD00000, 0xD00001, 0x748000)[case % 5]
            both(base + (index + 26) * 104 + 0x18, struct.pack("<I", x))
            both(base + (index + 26) * 104 + 0x20, struct.pack("<I", (0x608000, 0xA00000, 0xA00001)[case // 5 % 3]))
        both(base + 10 * 104 + 0x18, struct.pack("<I", 0x608000));both(base + 10 * 104 + 0x20, struct.pack("<I", 0x808000))
        both(args, bytes([case % 256]) + rng.randbytes(15))
        equal(audio_init(memory), True, "bank-3 game audio initialization");call("InitializeAudioDriver", limit=2000000)
        both(0x03004E98, b"\x0A");both(0x03004EBC, struct.pack("<I", 0x02030000))
        both(0x03004EC4, struct.pack("<I", 0 if case % 16 == 15 else 0x68736D53))
        both(0x02030000, bytes(10 * 80))
        for track in range(10):
            both(0x02030000 + track * 80, bytes([128 if (case + track) % 4 else 0]))
            both(0x02030000 + track * 80 + 0x13, b"\x55")

    def prepare(row):
        setup = {"ScriptCallbackSelectMapBank3PairBoardPosition": (0x0804CCDD, "ScriptCallbackInitializeMapBank3PairBoard"),
                 "ScriptCallbackRemoveMapBank3RecordMessages": (0x0804CA25, "ScriptCallbackConfigureMapBank3RecordMessages")}.get(row["name"])
        if setup:
            consumed = C.c_uint32(99)
            equal(main(memory, setup[0], args, C.byref(consumed)), 1, "bank-3 game dependency setup")
            equal(call(setup[1], args, limit=2000000), consumed.value, "bank-3 setup consumed")
            compare("initialized game dependency fixture", ranges)

    def main_check(row, label, opcode=False):
        prepare(row)
        if opcode:
            both(args, struct.pack("<I", row["address"] | 1) + native_bytes(args, 16))
            both(0x03000010, b"\x1C" + bytes(15))
            equal(command(memory, 0x1C, args), 1, row["name"] + " Main opcode")
            call("MainScriptInvokeCallback", args, limit=2000000)
        else:
            consumed = C.c_uint32(99)
            equal(main(memory, row["address"] | 1, args, C.byref(consumed)), 1, row["name"] + " native")
            equal(consumed.value, call(row["name"], args, limit=2000000), row["name"] + " consumed")
        compare(row["name"] + " " + label, ranges)

    for row in main_rows:
        for case in range(64):
            fixture(case);main_check(row, str(case))
        for case in range(4):
            fixture(case);main_check(row, "typed Main cursor", opcode=True)
            fixture(case);both(0x03002DC0, b"\x01");both(0x03002E10, b"\x01");both(0x03002C80, b"\x01")
            main_check(row, "inactive mapped records")
        fixture(0)
        if row["address"] in task_addresses:
            prepare(row)
            both(0x02019000, rng.randbytes(52));both(0x02019010, struct.pack("<I", args))
            both(args, b"\x01\xE1");both(0x03001FC4, struct.pack("<I", row["address"] | 1))
            report = (C.c_uint32 * 4)()
            equal(task(memory, 0x02019000, 0xF4, report), 1, "reviewed game F4 target")
            equal(report[3], call("TaskCommandF4", 0x02019000, limit=2000000), "game F4 continuation")
            compare(row["name"] + " F4", ranges)
        else:
            equal(apply(memory, row["address"] | 1), 0, "argument-dependent game rejected by F4")
            compare("rejected game task preserves memory", ranges)

    def animation_check(row, label):
        consumed, yielded = C.c_uint32(99), C.c_uint32(rng.getrandbits(32))
        both(output, struct.pack("<I", yielded.value))
        equal(animation(memory, row["address"] | 1, slot, args, C.byref(consumed), C.byref(yielded)), 1, row["name"] + " native")
        equal(consumed.value, call(row["name"], slot, args, output, limit=2000000), row["name"] + " consumed")
        equal(yielded.value, int.from_bytes(cpu.mem_read(output, 4), "little"), row["name"] + " yield")
        both(output, struct.pack("<I", yielded.value));compare(row["name"] + " " + label, ranges)

    angles = (0, 1, 4, 5, 7, 8, 127 * 8, 128 * 8, 129 * 8, 174 * 8, 175 * 8, 255 * 8, 65535)
    for row in animation_rows:
        for case in range(256):
            fixture(case)
            both(controls + 0x1DB, bytes([case]));both(classes + 0x57 * 2, struct.pack("<H", case * 257))
            value = angles[case % len(angles)];both(classes + 0xE3 * 2, struct.pack("<H", value))
            both(classes + (case + 0xE3) * 2, struct.pack("<H", value))
            animation_check(row, str(case))
        fixture(0);equal(apply(memory, row["address"] | 1), 0, "animation game rejected by F4")
        consumed = C.c_uint32(99)
        equal(main(memory, row["address"] | 1, args, C.byref(consumed)), 0, "animation game rejected by Main")
        compare("rejected animation contract preserves memory", ranges)
        fixture(0)
        for update in range(160):animation_check(row, "continuous sequence")

    collisions = next(r for r in main_rows if r["name"] == "ScriptCallbackUpdateMapBank3SpawnEntries")
    for kind in range(1, 5):
        for timer in (0, 1, 599, 600, 601, 3599, 3600, 65535):
            for score in (0, 1, 65535):
                fixture(kind);both(controls + 0x1A1, b"\x01")
                for index in range(10):both(0x03003DE0 + index * 8, b"\x00")
                both(0x03003DE0, bytes([1, kind, 0, 0, 2, 0x5A]))
                both(base + 26 * 104 + 0x18, struct.pack("<I", 0x748000));both(base + 26 * 104 + 0x20, struct.pack("<I", 0x608000))
                both(classes + 224 * 2, struct.pack("<HH", timer, score))
                main_check(collisions, "four collision effects, score/time wrapping")

    for mode in range(256):
        for repeat in range(8):
            fixture(mode + repeat);retained = rng.getrandbits(32);cpu.reg_write(UC_ARM_REG_R4, retained)
            equal(phase(memory, mode | (repeat << 8), retained), call("PickMapBank3PhaseKind", mode | (repeat << 8)), "phase mode/R4 contract")
            compare("phase PRNG and retained incoming word", ranges)
    for case in range(64):
        for index in (0, 1, 2, 9, 10, 254, 255):
            fixture(case)
            both(0x03003EA0 + index * 8, bytes([case % 4, 1 + case % 4, case % 4, case % 3, case % 5, 0xA5]) + struct.pack("<H", case * 1021))
            word = (case << 16) | index
            equal(entry_configure(memory, word), True, "source-entry byte selector")
            call("ConfigureMapBank3SourceEntry", word, limit=2000000);compare("source-entry configure", ranges)
            fixture(case)
            both(0x03003DE0 + index * 8, bytes([1, 1 + case % 4, case % 4, case % 3, case % 256, 0x5A]) + struct.pack("<H", 65535))
            equal(entry_motion(memory, index), True, "spawn-motion byte selector")
            call("SetMapBank3SpawnMotion", index, limit=2000000);compare("spawn-motion configure", ranges)
            fixture(case)
            both(0x03003EA0 + index * 8, bytes([1, 1 + case % 4, case % 4, case % 3, 0, 0xA5]) + struct.pack("<H", 24))
            if case % 2:
                for active in range(10):both(0x03003DE0 + active * 8, b"\x01")
                both(0x03003E30, b"\xB5")
            equal(entry_spawn(memory, word), True, "spawn source byte selector and full pool")
            call("SpawnMapBank3Entry", word, limit=2000000);compare("spawn entry and mapped pool-10 alias", ranges)
    for case in range(4096):
        vector = tuple(rng.randrange(256) for _ in range(8))
        equal(overlap(*vector), call("MapBank3BoxEndpointsOverlap", *vector), "strict byte endpoint overlap")
    for value in list(range(65536)) + [65536, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFE, 0xFFFFFFFF]:
        equal(square_root(value), call("BiosSqrt", value), "native integer square root against BIOS result model")
    fixture(0);consumed = C.c_uint32()
    equal(main(memory, 0x0804CCDD, args, C.byref(consumed)), 1, "pair-board continuous setup")
    call("ScriptCallbackInitializeMapBank3PairBoard", args)
    for update in range(320):
        equal(main(memory, 0x0804CDBD, args, C.byref(consumed)), 1, "pair-board continuous selection")
        equal(consumed.value, call("ScriptCallbackSelectMapBank3PairBoardPosition", args), "pair-board selection return")
        compare("pair-board continuous selection", ranges)
    print("Bank-3 source/spawn entries, pair-board, phase/orbit, record and endpoint contracts passed; full activity progression remains unverified.")


def check_bank3_chain(*, entries, rom, memory, cpu, bind, call, both, fresh, equal,
                     compare, native_bytes, command):
    from inventory_reference import body
    from pathlib import Path
    source = (Path(__file__).resolve().parents[1] / "src/game/bank3_chain.c").read_text()
    addresses = lambda name: {int(x, 16) for x in re.findall(r"case (0x[0-9A-Fa-f]+):", body(source, name))}
    main_addresses = addresses("hn_bank3_chain_callback")
    task_addresses = addresses("hn_bank3_chain_task")
    rows = {r["address"]: r for r in entries}
    equal((len(main_addresses), len(task_addresses)), (12, 19), "reviewed bank-3 chain dispatch contracts")
    # A complete source family includes its typed helpers and shared condition,
    # not only callback-named entries reported by the planning inventory.
    from inventory_reference import dispatches
    dispatch = dispatches()
    helper_addresses = {0x0804BD60, 0x0804BF34, 0x0804C044, 0x0804C848, 0x0804D09C,
                        0x0804E1BC, 0x0804E8FC, 0x0804E934, 0x0804E978, 0x0804E990,
                        0x0804ED08, 0x0804ED2C, 0x0804EE64, 0x08050724}
    family = {r["address"] for r in entries if r["source"] == "src/scripts/map_bank3_callbacks.c"}
    equal(len(family), 282, "complete original bank-3 main family inventory")
    equal(family - (set(dispatch) | helper_addresses), set(), "all bank-3 main entry points accounted for")
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    apply = bind("hn_bank3_task", C.c_int, C.c_void_p, C.c_uint32)
    animation = bind("hn_scene_animation_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32,
                     C.c_uint32, C.POINTER(C.c_uint32), C.POINTER(C.c_uint32))
    find = bind("hn_bank3_chain_find", C.c_uint32, C.c_void_p, C.c_uint32, C.c_uint32)
    flags = bind("hn_bank3_chain_flags", C.c_bool, C.c_void_p, C.c_uint32)
    variant = bind("hn_bank3_chain_variant", C.c_bool, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
    effect = bind("hn_bank3_chain_effect", C.c_bool, C.c_void_p, C.c_uint32, C.c_uint32, C.c_uint32)
    helpers = tuple((bind(native, result, C.c_void_p, *([C.c_uint32] * argc)), rows[address], result, argc)
                    for native, address, result, argc in (
                        ("hn_bank3_chain_set_flag", 0x0804ED08, None, 2),
                        ("hn_bank3_chain_clear", 0x0804E8FC, None, 2),
                        ("hn_bank3_chain_prepare", 0x0804E934, None, 1),
                        ("hn_bank3_chain_consume", 0x0804E978, None, 1),
                        ("hn_bank3_chain_mark", 0x0804EE64, C.c_bool, 1)))
    audio_init = bind("hn_audio_driver_init", C.c_bool, C.c_void_p)
    rng = random.Random(0x33434841494E)
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x05000000, 0x400),
              (0x06000000, 0x18000), (0x07000000, 0x400), (0x0E000000, 0x8000))
    base, controls, args, pool, flag_base, available = 0x02010000, 0x02004520, 0x02028000, 0x030035D0, 0x03003570, 0x03003B40
    byte_edges = (0, 1, 2, 8, 9, 10, 95, 96, 127, 254, 255)
    def byte(address, value):both(address, bytes([value & 255]))
    def control(offset, value):byte(controls + offset, value)
    def actor(index, offset, value):both(base + index * 104 + offset, struct.pack("<I", value & 0xFFFFFFFF))
    def fixture(case):
        fresh();both(0x02000000, rom[0x8FE018:0x900EC8])
        both(base, rng.randbytes(256 * 104));both(0x02018000, rng.randbytes(256 * 44))
        both(0x02020000, rng.randbytes(256 * 80));both(controls, rng.randbytes(0x200))
        both(0x0201DC40, rng.randbytes(1024));both(pool, rng.randbytes(256 * 8))
        both(0x03000000, struct.pack("<III", rng.randrange(1, 30000), rng.randrange(1, 30000), case % 227 + 1))
        both(0x03002D98, struct.pack("<I", base));byte(0x03002DC0, 40)
        both(0x03003070, struct.pack("<I", 0x02018000));byte(0x03002E10, 40)
        both(0x03002E08, struct.pack("<I", 0x02020000));byte(0x03002C80, 16)
        both(0x03002180, bytes(96));byte(0x03002182, 128)
        for address, value in zip((0x03002C0C, 0x03001F50, 0x03002C04, 0x03002C00), (10, 13, 16, 19)):byte(address, value)
        both(0x03000820, struct.pack("<HH", 1023, (0, 1, 2, 0x100, 0x200, 0x300, 0x303, 1023)[case % 8]))
        control(0x1B3, case & 1);control(0x1B4, byte_edges[case % len(byte_edges)]);control(0x1B5, case % 3)
        control(0x1B6, byte_edges[case // 3 % len(byte_edges)])
        for offset in (0x1B7, 0x1B8, 0x1B9):control(offset, case % 4)
        control(0x1BA, byte_edges[case % len(byte_edges)]);control(0x1BB, case % 8);control(0x1BC, byte_edges[case % len(byte_edges)])
        control(0x1BD, case % 3);control(0x1BE, case % 18);control(0x1BF, (0, 1, 11, 12, 254, 255)[case % 6])
        control(0x1C0, 15 if case % 2 else 255);control(0x1C1, 0 if case % 2 else 255);control(0x1C2, case % 9)
        for offset in (0x1C3, 0x1C4, 0x1C5):control(offset, byte_edges[case % len(byte_edges)])
        control(0x1D6, case % 4);control(0x60, case);control(0x61, byte_edges[case % len(byte_edges)])
        byte(0x030021D4, ((case // 8) & 1) * 32 + ((case // 16) & 1) * 64)
        order = list(range(9));rng.shuffle(order);both(available, bytes(order))
        both(flag_base, bytes(value + 1 if case >> (i % 6) & 1 else 0 for i, value in enumerate(order)))
        for index in range(16):
            kind = (0, 255, 1, 9, 10, 11, 12, 13, 14, 15, 16, 17)[(case + index) % 12]
            both(pool + index * 8, bytes([kind, 1 + index % 8, 2 + index % 8,
                                         index - 1 if index else 255, index + 1 if index < 15 else 255]))
            actor(index + 22, 0x18, (-0x100001, -0x100000, -0xFFFFF, 0, 0xE87FFF, 0xE88000, 0x1000000)[(case + index) % 7])
            actor(index + 22, 0x20, 6 << 16)
        both(pool + 255 * 8, bytes([1, 1, 2, 255, 255]))
        actor(10, 0x18, (0x688000, 0x688001, -1, 0x800000)[case % 4]);actor(10, 0x24, case % 2)
        for index in (15, 16, 17):
            actor(index, 4, 0x088E8DBE if case % 2 else 0x088E8DE4)
            actor(index, 0x18, (40 if index == 15 else 200 if index == 17 else 120) << 16)
            actor(index, 0x20, (-0x180001, -0x180000, -0x17FFFF, 0)[case % 4])
        both(args, rng.randbytes(16))
        equal(audio_init(memory), True, "chain audio initialization");call("InitializeAudioDriver", limit=2000000)

    def main_check(address, label, opcode=False):
        row = rows[address]
        if opcode:
            both(args, struct.pack("<I", address | 1) + native_bytes(args, 16));byte(0x03000010, 0x1C)
            equal(command(memory, 0x1C, args), 1, row["name"] + " Main opcode")
            call("MainScriptInvokeCallback", args, limit=2000000)
        else:
            consumed = C.c_uint32(99)
            equal(main(memory, address | 1, args, C.byref(consumed)), 1, row["name"] + " native " + label)
            equal(consumed.value, call(row["name"], args, limit=2000000), row["name"] + " return " + label)
        compare(row["name"] + " " + label, ranges)

    for address in sorted(task_addresses):
        for case in range(64):
            fixture(case)
            if address in main_addresses:main_check(address, str(case))
            else:
                equal(apply(memory, address | 1), 1, rows[address]["name"] + " task")
                call(rows[address]["name"], limit=2000000);compare(rows[address]["name"] + " task", ranges)
        for case in range(4):
            fixture(case);byte(0x03002DC0, 1);byte(0x03002E10, 1);byte(0x03002C80, 1)
            if address in main_addresses:main_check(address, "inactive records and Main cursor", opcode=True)
            else:
                equal(apply(memory, address | 1), 1, "mapped inactive chain task")
                call(rows[address]["name"], limit=2000000);compare("mapped inactive chain records", ranges)
            fixture(case);both(0x02019000, rng.randbytes(52));both(0x02019010, struct.pack("<I", args))
            both(args, b"\x01\xE1");both(0x03001FC4, struct.pack("<I", address | 1))
            report = (C.c_uint32 * 4)()
            equal(task(memory, 0x02019000, 0xF4, report), 1, "reviewed chain F4 target")
            equal(report[3], call("TaskCommandF4", 0x02019000, limit=2000000), "chain F4 continuation")
            compare(rows[address]["name"] + " F4", ranges)
        if address not in main_addresses:
            fixture(0);consumed = C.c_uint32(99)
            equal(main(memory, address | 1, args, C.byref(consumed)), 0, "void chain task rejected by Main")
            compare("rejected chain Main preserves memory", ranges)

    for native, row, result_type, argc in helpers:
        for case in range(256):
            fixture(case);word = case | (rng.getrandbits(24) << 8)
            arguments = (word,) if argc == 1 else (word, rng.getrandbits(32))
            result = native(memory, *arguments)
            if result_type:equal(result, True, row["name"] + " native")
            call(row["name"], *arguments, limit=2000000);compare(row["name"] + " byte/high-word contract", ranges)
        fixture(0);consumed = C.c_uint32(99)
        equal(apply(memory, row["address"] | 1), 0, "argumentful chain helper rejected by F4")
        equal(main(memory, row["address"] | 1, args, C.byref(consumed)), 0, "argumentful chain helper rejected by Main")
        compare("rejected chain helper preserves memory", ranges)
    for case in range(256):
        fixture(case);word = case | (rng.getrandbits(24) << 8);retained = rng.getrandbits(32)
        cpu.reg_write(UC_ARM_REG_R4, retained)
        equal(find(memory, word, retained), call("FindMapBank3ChainKind", word), "chain kind lookup retains incoming R4 when absent")
        compare("chain lookup preserves memory", ranges)
        result = C.c_uint32(99);equal(variant(memory, word, C.byref(result)), True, "chain pair variant native")
        equal(result.value, call("ChooseMapBank3ChainVariant", word), "chain pair variant return")
        compare("chain sorted pair, PRNG order and byte selector", ranges)

    # The original flag initializer retains &oldFlags[i] in R4 on an invalid-kind
    # lookup. Its explicit native adapter receives that guest frame address.
    for case in range(256):
        fixture(case);both(flag_base, bytes([case] * 9))
        equal(flags(memory, 0x03007BD4), True, "flag initializer with explicit original stack frame")
        call("InitializeMapBank3ChainFlags", limit=2000000);compare("flag restoration and missing-kind stack-byte alias", ranges)
    for mask in range(512):
        fixture(mask);both(flag_base, bytes(i + 1 if mask >> i & 1 else 0 for i in range(9)))
        equal(apply(memory, 0x0804ED69), 1, "all grid line combinations")
        call("RefreshMapBank3ChainLines", limit=2000000);compare("rows, columns, diagonals and changed-line signal", ranges)
    for kind in range(256):
        for capture in (1, 2):
            fixture(kind);word = (rng.getrandbits(24) << 8) | 7
            byte(pool + 7 * 8, kind);control(0x1D6, kind % 4)
            equal(effect(memory, word, capture | 0xCAFE00, kind | 0xABCD00), True, "chain effect native")
            call("ApplyMapBank3ChainEffect", word, capture | 0xCAFE00, kind | 0xABCD00, limit=2000000)
            compare("all effect bytes, capture modes, linked mutation and sound priority", ranges)

    for index in (0, 1, 7, 15, 254, 255):
        for kind in range(18):
            for capture in (0, 1, 2, 255):
                fixture(kind + capture);word = index | 0xCAFE0000
                both(pool + index * 8, bytes([kind, 1, 9, 255, 255]))
                equal(effect(memory, word, capture, kind), True, "effect byte selectors and empty links")
                call("ApplyMapBank3ChainEffect", word, capture, kind, limit=2000000)
                compare("effect on mapped byte-edge entry and all capture modes", ranges)
    for previous in (0, 6, 7, 8, 15, 16, 255):
        for next_index in (0, 6, 7, 8, 15, 16, 255):
            fixture(previous + next_index);both(pool + 7 * 8, bytes([12, 1, 9, previous, next_index]))
            if effect(memory, 7, 1, 12):
                call("ApplyMapBank3ChainEffect", 7, 1, 12, limit=2000000)
                compare("live links, self references and out-of-chain neighbors", ranges)
            else:
                # Some deliberately inconsistent graphs recurse until the GBA
                # stack overwrites the chain pool itself. Check that diagnosis
                # before accepting the host's explicit corrupt-state refusal;
                # these paths are not successful state-parity comparisons.
                from unicorn import UC_HOOK_CODE
                from unicorn.arm_const import UC_ARM_REG_SP
                minimum_sp = [cpu.reg_read(UC_ARM_REG_SP)]
                def enter_effect(machine, address, size, data):
                    minimum_sp[0] = min(minimum_sp[0], machine.reg_read(UC_ARM_REG_SP))
                hook = cpu.hook_add(UC_HOOK_CODE, enter_effect, begin=0x0804E1BC, end=0x0804E1BC)
                try:call("ApplyMapBank3ChainEffect", 7, 1, 12, limit=2000000)
                finally:cpu.hook_del(hook)
                equal(minimum_sp[0] <= pool + 16 * 8, True,
                      f"corrupt chain {previous}/{next_index} overflows original stack into game records")

    for case in range(256):
        fixture(case);control(0x1B7, 3);control(0x1B8, 3);control(0x1B9, 3)
        for index in range(16):
            byte(pool + index * 8, (case + index) % 18)
            actor(index + 22, 0x18, (40, 120, 200)[index % 3] << 16);actor(index + 22, 0x20, 100 << 16)
        for index, x in ((15, 40), (16, (40, 120, 200)[case % 3]), (17, 200)):
            actor(index, 0x18, (x + 1) << 16);actor(index, 0x20, 100 << 16)
        main_check(0x0804DF44, "paddle captures, enemy overlap and linked effects")

    tile_callback = rows[0x0804EE88]
    for case in range(256):
        fixture(case);consumed, yielded = C.c_uint32(99), C.c_uint32(rng.getrandbits(32))
        both(0x0203FFF0, struct.pack("<I", yielded.value))
        equal(animation(memory, 0x0804EE89, base, args, C.byref(consumed), C.byref(yielded)), 1, "chain tile animation")
        equal(consumed.value, call(tile_callback["name"], base, args, 0x0203FFF0), "chain tile consumed")
        equal(yielded.value, int.from_bytes(cpu.mem_read(0x0203FFF0, 4), "little"), "chain tile yield")
        both(0x0203FFF0, struct.pack("<I", yielded.value));compare("chain tile byte wrapping", ranges)
    for address in (0x0804E1BC, 0x0804E990, 0x0804ED2C, 0x0804EE88):
        fixture(0);consumed = C.c_uint32(99)
        equal(apply(memory, address | 1), 0, "chain non-task contract rejected by F4")
        equal(main(memory, address | 1, args, C.byref(consumed)), 0, "chain non-Main contract rejected")
        compare("rejected effect/lookup/variant/animation preserves memory", ranges)
    fixture(0);main_check(0x0804D4E0, "continuous chain initialization")
    for update in range(512):
        main_check(0x0804DA88, "continuous counter, removal and spawn")
        main_check(0x0804EA1C, "continuous background phases")
    print("28 chain functions passed: Main/F4/animation contracts, byte selectors, retained R4, linked effects, grid lines and continuous spawning.")


def check_bank3_presets(*, entries, reference, rom, memory, cpu, bind, call, both,
                       fresh, equal, compare, native_bytes):
    from inventory_reference import body
    from pathlib import Path
    root = Path(__file__).resolve().parents[1]
    dispatch = body((root / "src/game/bank3.c").read_text(), "main_presets")
    addresses = {int(x, 16) for x in re.findall(r"case (0x[0-9A-Fa-f]+):", dispatch)}
    rows = [r for r in entries if r["address"] in addresses or
            r["source"] == "src/actors/actor_animation_presets.c"]
    equal((len(addresses), len(rows)), (212, 235), "bank-3 scene presets and complete base-actor family")
    scalar = set()
    for row in rows:
        text = (reference / row["source"]).read_text(encoding="utf-8")
        declaration = re.search(r"\b(void|u32)\s+" + re.escape(row["name"]) + r"\((?:void|const u8 \*arguments)\)", text)
        equal(bool(declaration), True, "reviewed argument-free bank-3 dependency contract")
        if declaration[1] == "u32":scalar.add(row["address"])
    equal(len(scalar), 24, "defined-zero Main contracts")
    apply = bind("hn_bank3_task", C.c_int, C.c_void_p, C.c_uint32)
    actor = bind("hn_preset_apply", C.c_int, C.c_void_p, C.c_uint32)
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    audio_init = bind("hn_audio_driver_init", C.c_bool, C.c_void_p)
    rng = random.Random(0x334D41494E505245)
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x05000000, 0x400),
              (0x06000000, 0x18000), (0x07000000, 0x400), (0x0E000000, 0x8000))
    base, context, args = 0x02010000, 0x02019000, 0x02028000

    def fixture(case):
        fresh();both(0x02000000, rom[0x8FE018:0x900EC8])
        both(0x03000000, struct.pack("<III", rng.randrange(1, 30000), rng.randrange(1, 30000), case % 227 + 1))
        both(base, rng.randbytes(256 * 104));both(0x02018000, rng.randbytes(16 * 44))
        both(0x03002D98, struct.pack("<I", base));both(0x03002DC0, b"\x28")
        both(0x03003070, struct.pack("<I", 0x02018000));both(0x03002E10, b"\x10")
        both(0x02004520, rng.randbytes(0x200));both(0x0201DC40, rng.randbytes(0x200))
        for offset in (0xA7, 0x1D3, 0x93):both(0x02004520 + offset, bytes([(0, 1, 4, 5, 6, 7, 254, 255)[case % 8]]))
        for i, address in enumerate((0x03002C0C, 0x03001F50, 0x03002C04, 0x03002C00)):
            both(address, bytes([(10, 13, 16, 19)[(case + i) % 4]]))
        both(0x03002182, bytes([128 | (case & 127)]));both(0x03002170, b"\xA5")
        equal(audio_init(memory), True, "bank-3 preset audio initialization")
        call("InitializeAudioDriver", limit=2000000)

    def direct(row, label):
        function = apply if row["address"] in addresses else actor
        equal(function(memory, row["address"] | 1), 1, row["name"] + " " + label)
        call(row["name"], limit=2000000);compare(row["name"] + " " + label, ranges)

    for row in rows:
        for case in range(32):
            fixture(case);direct(row, str(case))
        for case in range(4):
            fixture(case);both(context, rng.randbytes(52));both(context + 16, struct.pack("<I", args))
            both(args, b"\x01\xE1");both(0x03001FC4, struct.pack("<I", row["address"] | 1))
            report = (C.c_uint32 * 4)()
            equal(task(memory, context, 0xF4, report), 1, "bank-3 preset F4 dispatch")
            equal(report[3], call("TaskCommandF4", context, limit=2000000), "bank-3 preset F4 continuation")
            compare("bank-3 preset F4 " + row["name"], ranges)
        fixture(0);consumed = C.c_uint32(99)
        is_scalar = row["address"] in scalar
        equal(main(memory, row["address"] | 1, args, C.byref(consumed)), int(is_scalar), "bank-3 preset Main ABI")
        if is_scalar:equal(consumed.value, call(row["name"], args, limit=2000000), "bank-3 preset defined return")
        compare("bank-3 preset Main " + row["name"], ranges)
        for case in (0, 1, 254, 255):
            fixture(case);both(0x03002DC0, b"\x01");both(0x03002E10, b"\x01")
            for address in (0x03002C0C, 0x03001F50, 0x03002C04, 0x03002C00):both(address, bytes([case]))
            direct(row, "inactive mapped byte selectors")
    for name in ("SetMapBank3Class2Indicator", "ConfigureMapBank3AuxiliaryByAnimation",
                 "SetMapBank3Class3ControlDirection"):
        row = next(r for r in rows if r["name"] == name)
        for selector, index in ((0x03002C04, 16), (0x03002C00, 19)):
            fixture(0);both(selector, bytes([index]))
            both(0x03002D98, struct.pack("<I", selector - index * 104 - 4))
            direct(row, "animation overwrites live class selector")
    print("212 bank-3 scene presets and all 23 base-actor functions passed; full area integration remains unverified.")


def check_bank3_late(*, entries, reference, rom, memory, cpu, bind, call, both,
                     fresh, equal, compare, native_bytes, command):
    rows = [r for r in entries if r["source"] == "src/scripts/map_bank3_late_callbacks.c"]
    equal(len(rows), 31, "complete bank-3 late family inventory")
    text = (reference / "src/scripts/map_bank3_late_callbacks.c").read_text(encoding="utf-8")
    presets = [r for r in rows if re.search(r"void\s+" + re.escape(r["name"]) + r"\((?:void|const u8 \*arguments)\)", text)]
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
