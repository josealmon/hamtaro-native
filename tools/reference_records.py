"""Private record/counter comparisons with native 32-bit result layouts."""
from __future__ import annotations
import ctypes as C
import random
import struct


def check_records(*, entries, rom, memory, cpu, bind, call, both, fresh,
                  equal, compare, native_bytes, command):
    families = {"src/save/game_records.c", "src/save/game_record_allocator.c",
                "src/save/game_record_allocate_callback.c", "src/scripts/script_callback_allocate_class_game_record.c",
                "src/scripts/script_callback_allocate_record_batch.c"}
    rows = [row for row in entries if row["source"] in families]
    equal(len(rows), 9, "complete record allocation/query/consume/counter families")
    count = bind("hn_records_count", C.c_uint8, C.c_void_p)
    allocate = bind("hn_record_allocate", C.c_int32, C.c_void_p, C.c_uint16)
    query = bind("hn_record_query", C.c_int32, C.c_void_p, C.c_uint8, C.c_uint32)
    consume = bind("hn_record_consume", C.c_int32, C.c_void_p, C.c_uint16, C.c_uint8, C.c_uint32)
    adjust = bind("hn_record_adjust_counter", C.c_uint32, C.c_void_p, C.c_uint8, C.c_int32)
    counter = bind("hn_record_counter", C.c_uint8, C.c_void_p, C.c_uint8)
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    rng = random.Random(0x5245434F5244)
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x05000000, 0x400))
    slots, args = 0x0201CB20, 0x02020000

    def fixture(case):
        fresh()
        both(0x02000000, rom[0x8FE018:0x900EC8])
        both(slots, rng.randbytes(4096))
        both(0x0201CC20, rng.randbytes(256))
        both(0x02010000, rng.randbytes(40 * 104))
        both(0x03002D98, struct.pack("<I", 0x02010000))
        both(0x03002DC0, b"\x28\x00\x27")
        both(0x03001F50, bytes([2 if case & 1 else 13]))
        both(0x03002180, rng.randbytes(96))
        both(0x0201DC40, rng.randbytes(128))
        both(args, rng.randbytes(16))

    def rom_word(address):
        return struct.unpack_from("<I", rom, address - 0x08000000)[0]

    valid_keys = []
    for key in range(128):
        descriptor = rom_word(0x0858CFE8 + key * 4)
        bank = rom_word(0x0858F25C + (descriptor & 3) * 4)
        pointer = bank + ((descriptor >> 2) & 127) * 4
        if 0x08000000 <= pointer <= 0x08FFFFFC or 0x02000000 <= pointer <= 0x0203FFFC:
            valid_keys.append(key)
    equal(bool(valid_keys), True, "record result table fixtures have mapped bank views")
    for case in range(512):
        fixture(case)
        for i in range(64):
            raw = (rng.getrandbits(32) & ~0xF80) | (0 if i < case % 65 else (1 + i % 31) << 7)
            both(slots + i * 4, struct.pack("<I", raw))
        equal(count(memory), call("CountOccupiedGameRecords"), "occupied record count")
        compare("occupied record count RAM", ranges)
        key = (0, 1, 59, 60, 127, 128, 511, 512, 65535)[case % 9]
        equal(allocate(memory, key) & 0xFFFFFFFF, call("AllocateGameRecord", key), "record allocation return")
        compare(f"record allocation {case}", ranges)
    for index in range(256):
        for occupied in (0, 1, 31):
            fixture(index)
            key = valid_keys[index % len(valid_keys)]
            both(slots + index * 4, struct.pack("<I", (rng.getrandbits(32) & ~0xFFF) | key | occupied << 7))
            result = (0, args, slots + index * 4)[index % 3]
            both(args, rng.randbytes(12))
            equal(query(memory, index, result), call("QueryGameRecord", index, result), "record query")
            compare("record query/result layout", ranges)
    for case in range(2048):
        fixture(case)
        index = (0, 1, 63, 64, 127, 255, 1023)[case % 7]
        key = valid_keys[case % len(valid_keys)]
        occupied = (0, 1, 2, 30, 31)[case % 5]
        both(slots + index * 4, struct.pack("<I", (rng.getrandbits(32) & ~0xFFF) | key | occupied << 7))
        both(0x030021B2, bytes([(case & 1) << 1]))
        result = (0, args, slots + index * 4)[case % 3]
        comparison = (0, 1, 2, 30, 31, 32, 255)[case % 7]
        equal(consume(memory, index, comparison, result) & 0xFFFFFFFF,
              call("ConsumeGameRecord", index, comparison, result), "record consume")
        compare(f"record consume/layout/aliases {case}", ranges)
    for index in range(256):
        fixture(index)
        equal(counter(memory, index), call("QueryGameCounter", index), "game counter index bounds")
        for delta in (0, 1, -1, 127, 128, 255, -255, 2147483647, -2147483648):
            both(0x0201CC20 + index, bytes([(0, 1, 127, 128, 254, 255)[index % 6]]))
            equal(adjust(memory, index, delta), call("AdjustGameCounter", index, delta & 0xFFFFFFFF), "game counter adjust")
            compare("game counter sentinel/wrap/limit", ranges)
    callbacks = [row for row in rows if row["name"].startswith("ScriptCallback")]
    for row in callbacks:
        for case in range(128):
            fixture(case)
            for i in range(64):
                both(slots + i * 4, struct.pack("<I", (rng.getrandbits(32) & ~0xFFF) |
                                              valid_keys[i % len(valid_keys)] | ((0 if i == case % 65 else 1) << 7)))
            key = (0, 1, 59, 60, 127, 511, 65535)[case % 7]
            both(0x0201DC84, struct.pack("<H", key))
            both(args, struct.pack("<H", key))
            argument = args if case < 96 else 0x03001137 + case % 3
            consumed = C.c_uint32(99)
            equal(main(memory, row["address"] | 1, argument, C.byref(consumed)), 1, "record allocate callback")
            equal(consumed.value, call(row["name"], argument), "record allocate callback consumed")
            compare("record allocate callback counter/palette/state", ranges)
        for case in range(16):
            fixture(case)
            both(args, struct.pack("<I", row["address"] | 1) + struct.pack("<H", case))
            both(0x03000010, b"\x1C" + bytes(15))
            equal(command(memory, 0x1C, args), 1, "record allocation Main ABI")
            call("MainScriptInvokeCallback", args)
            compare("record allocation Main ABI", ranges)
    for case in range(16):
        fixture(case)
        both(0x02019000, rng.randbytes(52))
        both(0x02019010, struct.pack("<I", args))
        both(args, b"\x01\xE1")
        both(0x03001FC4, struct.pack("<I", 0x0801E00D))
        report = (C.c_uint32 * 4)()
        equal(task(memory, 0x02019000, 0xF4, report), 1, "class record allocation F4 ABI")
        equal(report[3], call("TaskCommandF4", 0x02019000), "class record task continuation")
        compare("class record allocation F4 ABI", ranges)
    batch = next(r for r in rows if r["name"] == "ScriptCallbackAllocateGameRecordBatch")
    for occupied in range(65):
        for amount in (0, 1, 2, 30, 31, 32, 33, 63, 64, 65, 255, 65535):
            fixture(occupied + amount)
            for index in range(64):
                both(slots + index * 4, struct.pack("<I", valid_keys[index % len(valid_keys)] | (128 if index < occupied else 0)))
            both(0x0201DC4C, struct.pack("<H", amount));both(args, struct.pack("<H", (0, 6, 11, 63, 65535)[amount % 5]))
            both(0x03002DC0, b"\x01")
            consumed = C.c_uint32(99)
            equal(main(memory, batch["address"] | 1, args, C.byref(consumed)), 1, "batch allocation occupancy/amount edges")
            equal(consumed.value, call(batch["name"], args), "batch allocation consumed")
            compare("batch capacity, zero/full branch, countdown and division boundaries", ranges)
    apply = bind("hn_preset_apply", C.c_int, C.c_void_p, C.c_uint32)
    fixture(0);equal(apply(memory, batch["address"] | 1), 0, "argument-dependent batch rejected by F4")
    compare("rejected record batch preserves memory", ranges)
    print("Nine record allocation/query/consume/counter functions across five families passed; full inventory/save UI integration remains unverified.")
