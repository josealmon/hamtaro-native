"""Private comparisons of the connected actor-wandering source families."""
from __future__ import annotations
import ctypes as C
import random
import struct


def check_wander(*, entries, rom, memory, cpu, bind, call, both, fresh,
                 equal, compare, native_bytes, command):
    names = {"AnimationCallbackInitializeRenderMotion", "AnimationCallbackUpdateRenderMotion",
             "CheckRenderMotionBounds", "SetRenderMotionVelocity", "ScriptCallbackResetRenderMotionAnimation",
             "AnimationCallbackInitializeRenderTurn", "AnimationCallbackUpdateRenderTurn"}
    selected = [row for row in entries if row["name"] in names]
    equal(len(selected), 7, "complete connected wandering/turning family inventory")
    initialize = bind("hn_wander_initialize", C.c_bool, C.c_void_p, C.c_uint32)
    velocity = bind("hn_wander_velocity", None, C.c_void_p, C.c_uint32, C.c_int32, C.c_uint8)
    bounds = bind("hn_wander_bounds", C.c_bool, C.c_void_p, C.c_uint32, C.c_uint32, C.c_uint8, C.POINTER(C.c_uint32))
    update = bind("hn_wander_update", C.c_bool, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    reset = bind("hn_wander_reset", C.c_bool, C.c_void_p, C.c_uint32)
    main_callback = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    turn_initialize = bind("hn_wander_turn_initialize", C.c_bool, C.c_void_p, C.c_uint32)
    turn_update = bind("hn_wander_turn_update", C.c_bool, C.c_void_p)
    animation = bind("hn_scene_animation_callback", C.c_int, C.c_void_p, C.c_uint32,
                     C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32), C.POINTER(C.c_uint32))
    rng = random.Random(0x57414E444552)
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000))
    context, slot, args = 0x030010E0, 0x02010000 + 16 * 104, 0x02020000
    stop_flags = struct.unpack("<HH", rom[0x5A431C:0x5A4320])

    def value(address, size=4):
        return int.from_bytes(native_bytes(address, size), "little")

    def flag(identifier, enabled):
        address = 0x03002180 + (identifier >> 3)
        previous = value(address, 1)
        mask = 1 << (identifier & 7)
        both(address, bytes([(previous | mask) if enabled else previous & ~mask]))

    def fixture(case):
        fresh()
        both(0x02000000, rom[0x8FE018:0x900EC8])
        both(0x02010000, rng.randbytes(40 * 104))
        both(0x03002D98, struct.pack("<I", 0x02010000))
        both(0x03002DC0, b"\x28\x00\x27")
        both(0x03002C0C, b"\x0A")
        both(0x03002180, rng.randbytes(96))
        both(0x03000000, struct.pack("<III", *[rng.randrange(1, 30000) for _ in range(3)]))
        both(0x020045CD, b"\0")
        both(0x03001FA8, bytes([case & 8]))
        for i in range(2):
            record = struct.pack("<6IH7B3x", 0, 0, 0x01000000, 0x01000000,
                (0, 1, 0x10000, 0x8000, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF)[case % 7],
                (0, 1, 15, 16, 0x10000, 0x80000000, 0xFFFFFFFF)[case % 7],
                (0, 1, 2, 65535)[case % 4], 0, 1 + case % 254, 0,
                (0, 1, 10, 255)[case % 4], 16 + i, case % 4, case % 3)
            both(context + i * 36, record)
            actor = 0x02010000 + (16 + i) * 104
            both(actor + 0x18, struct.pack("<III", 0x00808000, rng.getrandbits(32), 0x00808000))
            both(actor + 0x60, bytes([(0, 1, 2, 3, 4, 5, 15, 255)[case % 8]]))
            flag(stop_flags[i], False)
        both(args, rng.randbytes(32))

    for case in range(512):
        fixture(case)
        payload = rng.randbytes(14) + bytes([(16, 17, 0, 255)[case % 4], case % 256])
        both(args, payload)
        equal(initialize(memory, args), True, "wandering initialization")
        call("AnimationCallbackInitializeRenderMotion", slot, args, 0x0203FFF0)
        both(0x0203FFF0, bytes(4))
        compare(f"wandering initialization {case}", ranges)
    for offset in range(32):
        for record in range(2):
            fixture(offset)
            aliased = context + record * 36 + offset
            both(aliased, rng.randbytes(16))
            equal(initialize(memory, aliased), True, "wandering aliased initialization")
            call("AnimationCallbackInitializeRenderMotion", slot, aliased, 0x0203FFF0)
            both(0x0203FFF0, bytes(4))
            compare(f"wandering initialized arguments/context alias {offset}/{record}", ranges)
    for rate in (0, 1, 0x10000, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF):
        for direction in range(256):
            fixture(direction)
            velocity(memory, slot, C.c_int32(rate), direction)
            call("SetRenderMotionVelocity", slot, rate, direction)
            compare(f"wandering velocity {rate:X}/{direction}", ranges)
    for case in range(2048):
        fixture(case)
        direction = (0, 1, 2, 3, 4, 15, 254, 255)[case % 8]
        if case & 16:
            both(context, struct.pack("<4I", 0x80000000, 0x80000000, 0x7FFFFFFF, 0x7FFFFFFF))
        both(slot + 0x18, struct.pack("<III", rng.getrandbits(32), 0, rng.getrandbits(32)))
        both(slot + 0x24, struct.pack("<III", rng.getrandbits(32), 0, rng.getrandbits(32)))
        if case & 32:
            distance = (0, 0x27FFFF, 0x280000, 0x280001, 0x80000000)[case % 5]
            leader = 0x02010000 + 10 * 104
            both(leader + 0x18, struct.pack("<I", (value(slot + 0x18) + distance) & 0xFFFFFFFF))
            both(leader + 0x20, struct.pack("<I", value(slot + 0x20)))
        if case & 64:
            selector = (1, 16, 255)[case % 3]
            both(0x020045CD, bytes([selector]))
            both(0x03002BB0 + (selector - 1) * 4, struct.pack("<I", 0x08032C81))
        blocked = C.c_uint32(99)
        equal(bounds(memory, context, slot, direction, C.byref(blocked)), True, "wandering bound check")
        equal(blocked.value, call("CheckRenderMotionBounds", context, slot, direction), "wandering bound result")
        compare(f"wandering bounds {case}", ranges)

    def update_case(label, callback_slot):
        yielded = C.c_uint32(rng.getrandbits(32))
        both(0x0203FFF0, struct.pack("<I", yielded.value))
        equal(update(memory, callback_slot, args, C.byref(yielded)), True, label)
        equal(call("AnimationCallbackUpdateRenderMotion", callback_slot, args, 0x0203FFF0), 1, "wandering update consumed")
        equal(yielded.value, struct.unpack("<I", cpu.mem_read(0x0203FFF0, 4))[0], "wandering update yield")
        both(0x0203FFF0, struct.pack("<I", yielded.value))
        compare(label, ranges)

    for case in range(1024):
        fixture(case)
        index = (16, 17, 18)[case % 3]
        both(args, bytes([index]))
        if case & 32:
            flag(stop_flags[index == 17], True)
        if case & 64:
            both(0x020045CD, b"\x01")
            both(0x03002BB0, struct.pack("<I", 0x08032C81))
        update_case(f"wandering update {case}", slot if case & 1 else slot + 104)
    fixture(2)
    both(context, struct.pack("<6IH7B3x", 0x00648000, 0x00648000, 0x00C88000, 0x00C88000,
                             0x10000, 0, 0, 1, 6, 0, 3, 16, 0, 0))
    both(args, b"\x10")
    for frame in range(2400):
        update_case(f"wandering component schedule {frame}", slot)
        # Physics integration is separately compared by the render suite. Here
        # advance the agreed coordinates to exercise complete pause/move cycles.
        for coordinate, speed in ((0x18, 0x24), (0x20, 0x2C)):
            both(slot + coordinate, struct.pack("<I", (value(slot + coordinate) + value(slot + speed)) & 0xFFFFFFFF))
    for case in range(512):
        fixture(case)
        argument = args if case < 256 else slot + 0x60
        both(argument, bytes([case % 40, case % 4]))
        equal(reset(memory, argument), True, "wandering animation reset")
        equal(call("ScriptCallbackResetRenderMotionAnimation", argument), 2, "wandering reset consumed")
        compare(f"wandering animation reset {case}", ranges)
    for callback, name, count in ((0x0801E45D, "AnimationCallbackInitializeRenderMotion", 16),
                                  (0x0801E4E5, "AnimationCallbackUpdateRenderMotion", 1)):
        for case in range(64):
            fixture(case)
            both(args + 14, b"\x10\x00")
            both(args, b"\x10")
            consumed, yielded = C.c_uint32(99), C.c_uint32(0xA5B6C7D8)
            both(0x0203FFF0, struct.pack("<I", yielded.value))
            equal(animation(memory, callback, slot, args, C.byref(consumed), C.byref(yielded)), 1, "wandering animation dispatch")
            equal(consumed.value, count, "wandering animation native consumed")
            equal(call(name, slot, args, 0x0203FFF0), count, "wandering animation source consumed")
            equal(yielded.value, struct.unpack("<I", cpu.mem_read(0x0203FFF0, 4))[0], "wandering animation ABI yield")
            both(0x0203FFF0, struct.pack("<I", yielded.value))
            compare("wandering animation ABI", ranges)
    for case in range(32):
        fixture(case)
        both(args, struct.pack("<I", 0x0801DA39) + bytes([case % 40, case % 4]))
        both(0x03000010, b"\x1C" + bytes(15))
        equal(command(memory, 0x1C, args), 1, "wandering reset Main ABI")
        call("MainScriptInvokeCallback", args)
        compare("wandering reset Main ABI", ranges)
    for case in range(512):
        fixture(case)
        argument = args if case < 256 else 0x02004577 + case % 5
        both(argument, rng.randbytes(3))
        equal(turn_initialize(memory, argument), True, "turn initialization")
        equal(call("AnimationCallbackInitializeRenderTurn", 0, argument, 0x0203FFF0), 3, "turn initialize consumed")
        both(0x0203FFF0, bytes(4))
        compare(f"turn initialize {case}", ranges)
    for case in range(2048):
        fixture(case)
        both(0x02004577, bytes([(0, 1, 2, 3, 255)[case % 5], 16, 10, case % 4, case % 256]))
        both(slot + 0x60, bytes([case % 256]))
        leader = 0x02010000 + 10 * 104
        x, z = ((0, 0x00808000), (0x00808000, 0), (0x01008000, 0x00808000),
                (0x00808000, 0x01008000), (0x00808000, 0x00808000))[case % 5]
        both(leader + 0x18, struct.pack("<I", x))
        both(leader + 0x20, struct.pack("<I", z))
        equal(turn_update(memory), True, "turn update")
        equal(call("AnimationCallbackUpdateRenderTurn", 0, 0, 0x0203FFF0), 0, "turn update consumed")
        both(0x0203FFF0, struct.pack("<I", 1))
        compare(f"turn update {case}", ranges)
    for field in (0, 1, 2, 3, 4):
        fixture(field)
        new_base = 0x02004577 + field - 16 * 104 - 0x60
        both(new_base, native_bytes(0x02010000, 40 * 104))
        both(0x03002D98, struct.pack("<I", new_base))
        both(0x02004577, bytes([0, 16, 10, 0, 1]))
        equal(turn_update(memory), True, "turn aliased context")
        call("AnimationCallbackUpdateRenderTurn", 0, 0, 0x0203FFF0)
        both(0x0203FFF0, struct.pack("<I", 1))
        compare(f"turn aliased context {field}", ranges)
    for callback, name, count in ((0x0801E719, "AnimationCallbackInitializeRenderTurn", 3),
                                  (0x0801E741, "AnimationCallbackUpdateRenderTurn", 0)):
        for case in range(32):
            fixture(case)
            both(args, bytes([16, 10, case % 4]))
            both(0x02004577, bytes([case % 3, 16, 10, case % 4, case % 5]))
            both(slot + 0x60, bytes([case % 4 + 1]))
            consumed, yielded = C.c_uint32(99), C.c_uint32(0xA5B6C7D8)
            both(0x0203FFF0, struct.pack("<I", yielded.value))
            equal(animation(memory, callback, slot, args, C.byref(consumed), C.byref(yielded)), 1, "turn animation dispatch")
            equal(consumed.value, call(name, slot, args, 0x0203FFF0), "turn animation consumed")
            equal(yielded.value, struct.unpack("<I", cpu.mem_read(0x0203FFF0, 4))[0], "turn animation yield")
            both(0x0203FFF0, struct.pack("<I", yielded.value))
            compare("turn animation ABI", ranges)
    bridges = {"ScriptCallbackReadRenderXZIntegers", "ScriptCallbackSetRenderBaselineXZWithOffsets",
               "ScriptCallbackReadRenderYHighHalf", "ScriptCallbackReadActorDirectionPair",
               "ScriptCallbackStoreMutualRenderDirections", "ScriptCallbackStoreDirectionsAgainstControl12",
               "ScriptCallbackStoreDirectionsAgainstControl17", "ScriptCallbackConfigurePairActorTurns",
               "ScriptCallbackFaceRenderClassesTowardSlot", "ScriptCallbackDecrementTurnDirection",
               "ScriptCallbackStepActorTurnAnimation", "ScriptCallbackSetRenderBit7",
               "ScriptCallbackMarkBackgroundsDirty", "ScriptCallbackCopySignedSlotVelocity",
               "ScriptCallbackStepClassTurnSequence", "ScriptCallbackSetClassTurnVelocity"}
    bridge_rows = [row for row in entries if row["name"] in bridges]
    equal(len(bridge_rows), 16, "complete connected actor coordinate/direction families")

    def bridge_fixture(case):
        fixture(case)
        both(args, bytes([case % 40, (case * 7) % 40, case % 256]))
        both(0x02004520, bytes([(case * 3) % 40, (case * 11) % 40]))
        both(0x02004532, bytes([(case * 13) % 40]))
        both(0x02004537, bytes([(case * 19) % 40]))
        both(0x03001F50, bytes([13 if case & 1 else 0]))
        both(0x02000868, bytes([0x60, 0x80]))
        both(0x0200086B, bytes([0x64, 0x81]))
        both(0x0200086E, bytes([0x64, 0x81, 1]))
        both(0x02004580, bytes([case % 8, case % 40, case % 256, case % 5,
                              case % 8, case * 3 % 8, case % 3, case % 5,
                              (0, 1, 2, 3, 4, 5, 255)[case % 7], case % 256]))

    for row in bridge_rows:
        for case in range(192):
            bridge_fixture(case)
            if row["name"] == "ScriptCallbackStepActorTurnAnimation":
                both(args, bytes([0x60, (16, 0x80, 0x81, 39)[case % 4], case % 4]))
            argument = args if case < 128 else 0x03001134 + case % 3
            consumed = C.c_uint32(99)
            equal(main_callback(memory, row["address"] | 1, argument, C.byref(consumed)), 1, "actor direction/coordinate callback")
            equal(consumed.value, call(row["name"], argument), "actor direction/coordinate consumed")
            compare(f"actor direction/coordinate {row['name']}/{case}", ranges)
        for case in range(16):
            bridge_fixture(case)
            both(args, struct.pack("<I", row["address"] | 1) + bytes([case % 40, (case * 7) % 40, case % 256]))
            both(0x03000010, b"\x1C" + bytes(15))
            equal(command(memory, 0x1C, args), 1, "actor direction/coordinate Main ABI")
            call("MainScriptInvokeCallback", args)
            compare("actor direction/coordinate Main ABI", ranges)
        if row["name"] in {"ScriptCallbackStoreMutualRenderDirections", "ScriptCallbackConfigurePairActorTurns",
                           "ScriptCallbackStepClassTurnSequence", "ScriptCallbackSetClassTurnVelocity"}:
            for case in range(16):
                bridge_fixture(case)
                both(0x02019000, rng.randbytes(52))
                both(0x02019010, struct.pack("<I", args))
                both(args, b"\x01\xE1")
                both(0x03001FC4, struct.pack("<I", row["address"] | 1))
                report = (C.c_uint32 * 4)()
                equal(task(memory, 0x02019000, 0xF4, report), 1, "actor direction task ABI")
                equal(report[3], call("TaskCommandF4", 0x02019000), "actor direction task continuation")
                compare("actor direction task ABI", ranges)
    for case in range(256):
        bridge_fixture(case)
        both(args, bytes([case, (case * 13) % 256]))
        consumed = C.c_uint32(99)
        equal(main_callback(memory, 0x0801CB49, args, C.byref(consumed)), 1, "signed actor velocity copy")
        equal(consumed.value, call("ScriptCallbackCopySignedSlotVelocity", args), "signed velocity copy consumed")
        compare(f"signed actor velocity copy {case}", ranges)
    for case in range(128):
        bridge_fixture(case)
        # Render field 0x4C aliases the live turn timer/control block. Both
        # animation setter calls must run and reload their table operands.
        new_base = 0x02004580 - 16 * 104 - 0x4C
        both(new_base, native_bytes(0x02010000, 40 * 104))
        both(0x03002D98, struct.pack("<I", new_base))
        both(0x02004580, bytes([case % 8, (case * 3) % 8, case % 3, 0]))
        both(args, bytes([0x60, 16, case % 4]))
        consumed = C.c_uint32(99)
        equal(main_callback(memory, 0x0801CA75, args, C.byref(consumed)), 1, "actor turn animation alias")
        equal(consumed.value, call("ScriptCallbackStepActorTurnAnimation", args), "actor turn alias consumed")
        compare(f"actor turn animation alias {case}", ranges)
    print("Twenty-three connected actor motion/direction functions and typed contracts passed; complete gameplay remains unverified.")
