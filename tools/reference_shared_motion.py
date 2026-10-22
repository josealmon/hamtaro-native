"""Private shared actor geometry, motion and screen-effect family comparisons."""
from __future__ import annotations
import ctypes as C
import itertools
import random
import struct


def check_shared_motion(*, entries, rom, memory, bind, call, both, fresh,
                        equal, compare, native_bytes, command):
    sources = {"src/actors/leader_position_velocity_callbacks.c", "src/actors/actor_approach_render_slot.c",
               "src/scripts/script_callback_class_xz_directions.c", "src/actors/render_screen_cell.c",
               "src/scripts/script_callback_class_position.c", "src/scripts/script_callback_flag_projected_slot_distance.c",
               "src/scripts/script_callback_store_secondary_class_screen_y_side.c", "src/scripts/script_callback_render_pair.c"}
    rows = [row for row in entries if row["source"] in sources]
    equal(len(rows), 11, "complete eight-family shared geometry inventory")
    main_rows = [row for row in rows if row["name"].startswith("ScriptCallback")]
    equal(len(main_rows), 10, "shared geometry Main contracts")
    task_rows = [row for row in rows if row["name"] in {"ScriptCallbackStoreSecondaryClassScreenYSide",
                 "ScriptCallbackInitializeRenderPair", "ScriptCallbackSetVideoMode1"}]
    equal(len(task_rows), 3, "shared geometry argument-free task contracts")
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    clamp = bind("hn_motion_clamp_coordinate", C.c_int32, C.c_int32, C.c_int32, C.c_int32)
    direction = bind("hn_render_direction_raw", C.c_uint32, C.c_void_p, C.c_uint8, C.c_uint8, C.c_bool)
    rng = random.Random(0x534841524D4F54)
    base, classes, controls, args = 0x02010000, 0x0201DC40, 0x02004520, 0x02028000
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000))

    def fixture(case, render_base=base):
        fresh();both(0x02000000, rom[0x8FE018:0x900EC8])
        both(render_base, rng.randbytes((40 if render_base >> 24 == 3 else 256) * 104))
        both(classes, rng.randbytes(512));both(controls, rng.randbytes(512));both(args, rng.randbytes(32))
        both(0x03002180, rng.randbytes(96));both(0x03002C90, rng.randbytes(256))
        both(0x03002D98, struct.pack("<I", render_base));both(0x03002DC0, b"\x28\x00\x27")
        both(0x03002C0C, bytes([case % 40]));both(0x03001F50, bytes([case * 7 % 40]))
        both(controls + 0x12, bytes([case * 11 % 256]))
        both(controls + 0x60, bytes([case % 4, case % 4]))
        both(controls + 0x66, bytes([case % 256, case * 3 % 256]))
        both(0x03002BB0, struct.pack("<II", rng.getrandbits(32), rng.getrandbits(32)))
        both(0x0201DCAE, struct.pack("<H", (0, 1, 2, 7, 32767, 32768, 65535)[case % 7]))

    def payload(name, case):
        if name == "ScriptCallbackApproachRenderSlot":
            return bytes([(0x60, 0xF0, 255)[case % 3], case % 9, case * 7 % 256,
                          (0, 1, 127, 128, 255)[case % 5], (0, 1, 127, 128, 255)[case // 5 % 5],
                          case % 256, case % 256, case * 3 % 256, (0, 1, 2, 255)[case % 4], case % 3])
        if name == "ScriptCallbackFindRenderScreenCell":
            # The CPU probe deliberately rejects BIOS division by zero. The
            # empty-grid host contract has a separate analytic public check.
            return bytes([case % 256, 1 + case % 16])
        return bytes([case % 256, case * 7 % 256, case % 256, case * 13 % 256])

    def check(row, label, arguments=args):
        consumed = C.c_uint32(99)
        equal(main(memory, row["address"] | 1, arguments, C.byref(consumed)), 1, row["name"] + " " + str(label))
        equal(consumed.value, call(row["name"], arguments, limit=2000000), "shared geometry consumed")
        compare(row["name"] + " " + str(label), ranges)

    for row in main_rows:
        for case in range(128):
            fixture(case);both(args, payload(row["name"], case));check(row, case)
        for case in range(16):
            fixture(case);both(args, struct.pack("<I", row["address"] | 1) + payload(row["name"], case))
            both(0x03000010, b"\x1C" + bytes(15))
            equal(command(memory, 0x1C, args), 1, "shared geometry Main ABI")
            call("MainScriptInvokeCallback", args, limit=2000000);compare("shared geometry Main ABI", ranges)
    for row in task_rows:
        for case in range(16):
            fixture(case);both(0x02019000, rng.randbytes(52));both(0x02019010, struct.pack("<I", args))
            both(args, b"\x01\xE1");both(0x03001FC4, struct.pack("<I", row["address"] | 1))
            report = (C.c_uint32 * 4)()
            equal(task(memory, 0x02019000, 0xF4, report), 1, "shared geometry F4 ABI")
            equal(report[3], call("TaskCommandF4", 0x02019000), "shared geometry F4 continuation")
            compare("shared geometry F4 ABI", ranges + ((0x02019000, 52),))
    for case in range(4096):
        values = [rng.randrange(-2147483648, 2147483648) for _ in range(3)]
        equal(clamp(*values) & 0xFFFFFFFF, call("ClampRenderApproachCoordinate", *[v & 0xFFFFFFFF for v in values]), "coordinate clamp signed/reversed bounds")
    for case in range(256):
        fixture(case);both(0x03002DC0, b"\x01")
        for eight in (False, True):
            name = "GetRenderXZDirectionBetweenSlots8" if eight else "GetRenderXZDirectionBetweenSlots4"
            equal(direction(memory, case, 255 - case, eight), call(name, case, 255 - case), "mapped inactive raw direction selectors")
    approach = next(row for row in rows if row["name"] == "ScriptCallbackApproachRenderSlot")
    coordinates = (0, 1, 0x7FFF, 0x8000, 0xFFFF)
    for case, (x, z, prefer) in enumerate(itertools.product(coordinates, coordinates, (0, 1))):
        fixture(case);both(controls + 0x60, b"\x00\x00")
        both(base + 5 * 104 + 0x18, struct.pack("<III", (x << 16) | 0xFFFF, 0, (z << 16) | 0xFFFF))
        both(base + 5 * 104 + 0x24, struct.pack("<III", 1, 0, 1))
        both(base + 6 * 104 + 0x18, struct.pack("<III", 0x8000, 0, 0x8000))
        both(args, bytes([0x60, case % 9, 5, 255, 128, 6, 0, 0, 255, prefer]));check(approach, "projected signed boundaries")
    for case in range(256):
        fixture(case);both(controls + 0x60, bytes([case % 2, case % 4]))
        both(base + 5 * 104 + 0x18, struct.pack("<III", case * 65536 + 32768, 0, case * 32768))
        both(base + 5 * 104 + 0x24, bytes(12))
        both(base + 6 * 104 + 0x18, native_bytes(base + 5 * 104 + 0x18, 12))
        both(args, bytes([0x60, case % 9, 5, 0, 0, 6, 0, 0, 1, 0]));check(approach, "completion and live idle state")
    for offset in (0x08, 0x18, 0x20, 0x24, 0x4C, 0x60, 0x63):
        for case in range(4):
            alias = controls + 0x61 - 16 * 104 - offset
            fixture(case, alias);both(controls + 0x60, bytes([1, case]))
            both(args, bytes([0x60, 0, 5, 0, 0, 16, 0, 0, 1, 0]));check(approach, "live render/control alias")
    for phase, delta, requested in itertools.product((0, 1, 2, 255), (-1, 0, 1), (0, 1, 255)):
        fixture(phase);both(controls + 255, bytes([phase, 0]));both(controls, b"\xA5")
        both(base + 5 * 104 + 0x18, struct.pack("<III", ((40 + delta) << 16) | 32768, 0, 0x8000))
        both(base + 5 * 104 + 0x24, bytes(12));both(base + 6 * 104 + 0x18, struct.pack("<III", 0x00288000, 0, 0x8000))
        both(args, bytes([255, 0, 5, 0, 0, 6, 0, 0, requested, 0]));check(approach, "selector 255/one-pixel/zero-step")
    distance = next(row for row in rows if row["name"] == "ScriptCallbackFlagProjectedSlotDistance")
    for x, z, threshold in itertools.product((-32768, -1, 0, 1, 32767), (-32768, -1, 0, 1, 32767), (0, 1, 255)):
        fixture(0);both(base + 5 * 104 + 0x18, bytes(12))
        both(base + 6 * 104 + 0x18, struct.pack("<III", (x << 16) & 0xFFFFFFFF, (z << 16) & 0xFFFFFFFF, 0))
        both(args, bytes([5, 6, threshold, threshold]));both(controls + 0x60, b"\xA5");check(distance, "projected absolute signed-half overflow")
    cell = next(row for row in rows if row["name"] == "ScriptCallbackFindRenderScreenCell")
    for divisions in (1, 2, 15, 16, 128, 160, 240, 255):
        fixture(divisions);both(args, bytes([0, divisions]));both(base + 0x4E, b"\xFF\xFF\xFF\xFF");check(cell, "empty/zero-width screen cells")
    for offset in (0x4E, 0x4F, 0x50, 0x51):
        fixture(offset, controls + 0x60 - 5 * 104 - offset)
        both(args, b"\x05\x03");check(cell, "screen cell/control alias")
    position = next(row for row in rows if row["name"] == "ScriptCallbackSetRenderPositionFromClasses")
    for offset in (-4, -2, 0, 2, 4, 6):
        fixture(offset, classes + 0x44 - 16 * 104 - 0x18 + offset)
        both(classes + 0x44, struct.pack("<HHH", 0x1234, 0x5678, 0x9ABC));both(args, b"\x10");check(position, "sequential class coordinate alias")
    leader_rows = [row for row in rows if row["name"] in {"ScriptCallbackCopyLeaderPositionWithScaledVelocity", "ScriptCallbackOffsetLeaderPositionAndApproach"}]
    for row in leader_rows:
        for offset in (0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C):
            fixture(offset, 0x03002BB0 - 5 * 104 - offset)
            both(args, b"\x05");check(row, "captured slot/offset alias")
    print("Eleven shared actor geometry functions, ten Main contracts and three task targets passed; full scene integration remains unverified.")


def check_scene_effects(*, entries, rom, memory, bind, call, both, fresh,
                        equal, compare, native_bytes, command):
    sources = {"src/graphics/display_mode_clear_callback.c", "src/graphics/screen_window_callbacks.c",
               "src/scripts/script_callback_configure_shake.c", "src/scripts/script_callback_render_shake.c",
               "src/scripts/script_callback_reset_background_shake.c"}
    rows = [row for row in entries if row["source"] in sources]
    equal(len(rows), 8, "complete five-family screen/shake inventory")
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    rng = random.Random(0x5348414B45)
    base, controls, args = 0x02010000, 0x02004520, 0x02028000
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x04000040, 8))

    def fixture(case, render_base=base):
        fresh();both(render_base, rng.randbytes((40 if render_base >> 24 == 3 else 256) * 104))
        both(controls, rng.randbytes(512));both(0x0201DC40, rng.randbytes(128))
        both(0x03002C90, rng.randbytes(256));both(0x03002E40, rng.randbytes(176))
        both(0x03002D98, struct.pack("<I", render_base));both(0x03002DC0, b"\x01")
        both(controls + 0x60, bytes([case % 2, case % 256, (0, 1, 128, 255)[case % 4],
                                   (0, 1, 2, 255)[case // 4 % 4], case % 256, case * 7 % 256]))
        both(args, rng.randbytes(8));both(0x04000040, rng.randbytes(8))

    def check(row, label):
        consumed = C.c_uint32(99)
        equal(main(memory, row["address"] | 1, args, C.byref(consumed)), 1, "native screen/shake " + label)
        equal(consumed.value, call(row["name"], args), "screen/shake zero-argument Main result")
        compare(row["name"] + " " + label, ranges)

    for row in rows:
        for case in range(256):
            fixture(case);check(row, f"byte domain {case}")
        for case in range(16):
            fixture(case);both(args, struct.pack("<I", row["address"] | 1) + rng.randbytes(4))
            both(0x03000010, b"\x1C" + bytes(15))
            equal(command(memory, 0x1C, args), 1, "screen/shake Main ABI")
            call("MainScriptInvokeCallback", args);compare("screen/shake Main ABI", ranges)
            fixture(case);both(0x02019000, rng.randbytes(52));both(0x02019010, struct.pack("<I", args))
            both(args, b"\x01\xE1");both(0x03001FC4, struct.pack("<I", row["address"] | 1))
            report = (C.c_uint32 * 4)()
            equal(task(memory, 0x02019000, 0xF4, report), 1, "screen/shake F4 ABI")
            equal(report[3], call("TaskCommandF4", 0x02019000), "screen/shake F4 continuation")
            compare("screen/shake F4 ABI", ranges)
    shake = next(row for row in rows if row["name"] == "ScriptCallbackUpdateRenderShake")
    for mode, amplitude, rate in itertools.product((0, 1, 255), (0, 1, 128, 255), (0, 1, 2, 255)):
        for case in (0, 1, 62, 63, 64, 254, 255):
            fixture(case);both(controls + 0x60, bytes([mode, case, amplitude, rate, case, case]))
            check(shake, "phase/amplitude/decay boundaries")
    for layers in range(256):
        fixture(layers);both(controls + 0x60, bytes([1, layers, 128, 1, 63, 0]))
        both(0x03002C90 + 0x3A + 48, struct.pack("<HH", 65530, 7))
        both(0x0201DC84, struct.pack("<HH", 65535, 32768));check(shake, "captured background-1 anchor/layer mask")
    # Position writes can overlap control bytes, classes or the render-base pointer.
    # Preserve the captured destination and read the decay state after those writes.
    for target in (controls + 0x5C, controls + 0x60, controls + 0x64, controls + 0x68,
                   0x0201DC80, 0x0201DC84, 0x0201DC88, 0x03002D98):
        for case in range(16):
            fixture(case, target - 5 * 104 - 0x18)
            both(controls + 0x60, bytes([0, 5, case * 17, case, case, case * 17]))
            check(shake, "captured destination/live decay alias")
    for mode, rate in itertools.product((0, 1), (0, 1, 2, 255)):
        fixture(0);both(controls + 0x60, bytes([mode, 15, 128, rate, 62, 254]))
        for frame in range(320):
            check(shake, f"continuous phase/decay {mode}/{rate}/{frame}")
    print("Eight shared screen/shake functions and their Main/F4 contracts passed; timed story integration remains unverified.")
