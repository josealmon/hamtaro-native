"""Private differential checks of complete cutscene actor/panel source families."""
from __future__ import annotations
import ctypes as C
import random
import re
import struct
from inventory_reference import body


def check_actor_mirror(*,entries,rom,memory,cpu,bind,call,both,fresh,equal,compare,native_bytes):
    rows=[r for r in entries if r['source']=='src/graphics/actor_animation_mirror.c']
    equal(len(rows),1,'complete actor animation reflection family')
    row=rows[0];equal(row['name'],'AnimationCallbackMirrorActorZAndAnimation','reflection source contract')
    animate=bind('hn_scene_animation_callback',C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32),C.POINTER(C.c_uint32))
    main=bind('hn_scene_callback',C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
    task=bind('hn_scene_actor_task',C.c_int,C.c_void_p,C.c_uint32)
    rng=random.Random(0x4D495252);base=0x02010000;output=0x0202F000
    ranges=((0x02000000,0x40000),(0x03000000,0x7000))
    def word(a,v):both(a,struct.pack('<I',v&0xFFFFFFFF))
    def fixture(case,render_base=base):
        fresh();both(render_base-104,rng.randbytes(258*104))
        for table,count,start in ((0x02002D60,23,0x08100000),(0x02002DBC,18,0x08200000),
                                  (0x02002E04,23,0x08300000),(0x02002E60,18,0x08400000)):
            both(table,struct.pack(f'<{count}I',*(start+i*8 for i in range(count))))
        word(0x03002D98,render_base);both(0x03002C0C,b'\x0A');both(0x03001F50,b'\x0D')
        both(0x02004543,bytes([case%256]));both(0x03002182,bytes([case&1]))
        both(0x0201DCA8,struct.pack('<H',(0,1,0x7FFF,0x8000,0xFFFF)[case%5]))
        both(0x03002DC0,b'\x01')  # Mapped inactive slots remain writable.
    def check(label):
        consumed=C.c_uint32(99);yielded=C.c_uint32(0xA5B6C7D8);word(output,yielded.value)
        equal(animate(memory,row['address']|1,0,0,C.byref(consumed),C.byref(yielded)),1,label+' native ABI')
        equal(consumed.value,call(row['name'],0,0,output),label+' consumed')
        equal(yielded.value,int.from_bytes(cpu.mem_read(output,4),'little'),label+' yield')
        word(output,yielded.value);compare(label,ranges)
    for primary in range(24):
        for secondary in range(19):
            for mode in range(3):
                case=primary*57+secondary*3+mode;fixture(case)
                target=base+(case%256)*104
                word(base+10*104+4,0x08100000+primary*8)
                word(base+13*104+4,0x08200000+secondary*8)
                if mode==1:
                    word(target+4,0x08300000+primary*8);word(target-104+4,0x08400000+secondary*8)
                if mode==2:both(0x03001F50,b'\x0A')
                check(f'mirror lookups/fallback/same animation {primary}/{secondary}/{mode}')
    for index in range(256):
        for enabled in (0,1):
            fixture(index);both(0x03002C0C,bytes([index]));both(0x03001F50,bytes([255-index]))
            both(0x03002182,bytes([enabled]));both(0x02004543,bytes([255-index]))
            word(base+index*104+0x20,(0,1,0x7FFFFFFF,0x80000000,0xFFFFFFFF)[index%5])
            check(f'mirror full-byte slots and wrapped Z {index}/{enabled}')
    for render_base in (0x02002D60-10*104-4,0x02002E04-7*104-0x20,
                        0x02004543-7*104-0x20,0x0201DCA8-7*104-0x20):
        for case in range(32):
            fixture(case,render_base);both(0x02004543,b'\x07')
            word(render_base+10*104+4,0x08100000+(case%23)*8)
            word(render_base+13*104+4,0x08200000+(case%18)*8)
            check('mirror mutable tables/control/class aliases and captured destination')
    fixture(0);consumed=C.c_uint32(99)
    equal(main(memory,row['address']|1,0,C.byref(consumed)),0,'reflection rejected through Main ABI')
    equal(task(memory,row['address']|1),0,'reflection rejected through task ABI')
    print('Actor reflection animation family matched: both tables/fallbacks, retained offsets, byte slots, wrapped Z, aliases and typed yield.')


def check_scene_actors(*, entries, reference, rom, memory, cpu, bind, call, both,
                       fresh, equal, compare, native_bytes, command):
    sources = {
        "src/scripts/script_callback_backup_render_pair.c", "src/actors/actor_restore_clear.c",
        "src/scripts/script_callback_render_acceleration.c", "src/graphics/slot_acceleration_z_callback.c",
        "src/scripts/script_callback_render_info_x.c", "src/scripts/script_callback_render_slot_bytes.c",
        "src/scripts/script_callbacks_adjust_class_turn_target.c", "src/actors/actor_approach_class_coordinates.c",
        "src/graphics/screen_pair_presets.c", "src/graphics/screen_pair_state_presets.c",
        "src/graphics/screen_pair_more_presets.c", "src/actors/actor_restore_slots.c",
        "src/scripts/script_callbacks_direction_slots.c", "src/scripts/script_callbacks_direction_variants.c",
        "src/scripts/script_callback_render_shake15248.c", "src/scripts/script_callback_render_selection.c"}
    rows = [row for row in entries if row["source"] in sources or row["name"] == "ScriptCallbackJumpUnlessMapBank3ValueEquals"]
    equal(len(rows), 46, "complete scene actor families and shared cross-bank condition")
    task_rows = [row for row in rows if row["name"] == "ScriptCallbackStoreClassScreenYSide" or not re.search(
        r"\barguments\b", body((reference / row["source"]).read_text(encoding="utf-8"), row["name"]))]
    equal(len(task_rows), 36, "reviewed argument-free scene actor task contracts")
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    rng = random.Random(0x53434E414354)
    args, base, classes, controls, snapshots = 0x02020000, 0x02010000, 0x0201DC40, 0x02004520, 0x02002EB0
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000))

    def value(address, size=4):
        return int.from_bytes(native_bytes(address, size), "little")

    def fixture(case, render_base=base):
        fresh()
        both(0x02000000, rom[0x8FE018:0x900EC8])
        both(render_base, rng.randbytes(256 * 104))
        both(0x02028000, rng.randbytes(16 * 44))
        both(snapshots, rng.randbytes(6 * 104))
        both(classes, rng.randbytes(512))
        both(controls, rng.randbytes(512))
        both(controls + 0x61, bytes([case % 40, case % 256, case % 256]))
        both(controls + 0x12, bytes([case % 40]))
        both(controls + 0x14, bytes([255 if case & 1 else case % 40,
                                    255 if case & 2 else (case * 7) % 40]))
        both(controls + 0x74, bytes([(0, 1, 2, 3, 4, 5, 255)[case % 7],
                                    (0, 1, 2, 3, 4, 5, 255)[(case // 7) % 7]]))
        both(0x03002180, rng.randbytes(96))
        both(0x03002182, bytes([128 | (case & 1)]))
        both(0x03002170, b"\xA5")
        both(0x03002C0C, bytes([(10, 13, 16, 19)[case % 4]]))
        both(0x03001F50, bytes([(0, 10, 13, 16)[case % 4]]))
        both(0x03002D98, struct.pack("<I", render_base))
        both(0x03002DC0, b"\x28\x00\x27")
        both(0x03003070, struct.pack("<I", 0x02028000))
        both(0x03002E10, b"\x10")

    def payload(name, case):
        if name == "ScriptCallbackConfigureRenderSelection":
            return bytes([(0, 1, 2, 255)[case % 4], (0, 0x33, 0x72, 255)[case % 4], case % 256])
        if name == "ScriptCallbackJumpUnlessMapBank3ValueEquals":
            mode, index = case % 3, case * 257 % 65536
            current = (int.from_bytes(native_bytes(classes + index * 2, 2), "little") if mode
                       else native_bytes(controls + index, 1)[0])
            return bytes([mode]) + struct.pack("<HHI", index, current if case & 1 else (current + 1) & 65535,
                                               0x02030000 + case * 4)
        if name == "ScriptCallbackBackupTwoRenderSlots":
            return bytes([255 if case & 1 else case % 40, 255 if case & 2 else (case * 7) % 40])
        if name == "ScriptCallbackApproachClassCoordinates":
            selector = (0x60, 0x70, 0, 255)[case % 4]
            both(controls + selector, bytes([(0, 1, 2, 3, 255)[case % 5],
                                           (0, 1, 2, 3, 128, 255)[case % 6]]))
            return bytes([selector, case % 256, (case * 3) % 256, case % 9, case % 256,
                          (0, 1, 127, 128, 255)[case % 5], (0, 1, 127, 128, 255)[(case // 5) % 5],
                          (0, 1, 2, 255)[case % 4], (0, 1, 255)[case % 3]])
        return bytes([case % 40, (case * 7) % 40, (0, 1, 127, 128, 255)[case % 5]]) + rng.randbytes(13)

    def check_main(row, case, argument=args):
        consumed = C.c_uint32(99)
        equal(main(memory, row["address"] | 1, argument, C.byref(consumed)), 1,
              f"native scene actor {row['name']}/{case}")
        equal(consumed.value, call(row["name"], argument), "scene actor consumed count")
        compare(f"scene actor {row['name']}/{case}", ranges)

    for row in rows:
        for case in range(128):
            fixture(case)
            both(args, payload(row["name"], case))
            check_main(row, case)
        for case in range(16):
            fixture(case)
            both(args, struct.pack("<I", row["address"] | 1) + payload(row["name"], case))
            both(0x03000010, b"\x1C" + bytes(15))
            equal(command(memory, 0x1C, args), 1, "scene actor Main ABI")
            call("MainScriptInvokeCallback", args)
            compare(f"scene actor Main ABI {row['name']}/{case}", ranges)
    for row in task_rows:
        for case in range(16):
            fixture(case)
            both(0x0201A000, rng.randbytes(52))
            both(0x0201A010, struct.pack("<I", args))
            index = case * 5 % 104
            both(args, bytes([index, 0xE1]))
            both(0x03001FC0 + index * 4, struct.pack("<I", row["address"] | 1))
            report = (C.c_uint32 * 4)()
            equal(task(memory, 0x0201A000, 0xF4, report), 1, "scene actor task ABI")
            equal(report[3], call("TaskCommandF4", 0x0201A000), "scene actor task continuation")
            compare(f"scene actor F4 ABI {row['name']}/{case}", ranges)
    approach = next(row for row in rows if row["name"] == "ScriptCallbackApproachClassCoordinates")
    # Signed-half deltas wrap before selecting an axis. One-pixel distances use
    # step one; zero requested step and invalid states retain source behavior.
    for case in range(1024):
        fixture(case)
        x, z = (0, 1, 32767, 32768, 65535)[case % 5], (0, 32767, 32768, 65535)[case % 4]
        dx, dz = (-32768, -1, 0, 1, 32767)[(case // 5) % 5], (-1, 0, 1)[case % 3]
        index = case % 256
        both(base + index * 104 + 0x18, struct.pack("<III", (x << 16) | case % 65536,
                                                    rng.getrandbits(32), (z << 16) | (65535 - case)))
        both(classes, struct.pack("<HH", (x + dx) & 65535, (z + dz) & 65535))
        both(controls + 0x60, bytes([case & 1, (0, 1, 2, 3, 128)[case % 5]]))
        both(args, bytes([0x60, 0, 1, case % 9, index, 0, 0, (0, 1, 2, 255)[case % 4], case & 1]))
        check_main(approach, f"signed boundary {case}")
    for case in range(256):
        fixture(case)
        both(base + 16 * 104 + 0x18, struct.pack("<III", 0x000A1234, 0xABCDEF00, 0x0014FEDC))
        both(classes, struct.pack("<HH", 10, 20))
        both(controls + 0x60, bytes([0, 128]))
        both(args, bytes([0x60, 0, 1, case % 9, 16, 0, 0, 1, case & 1]))
        for frame in range(4):
            check_main(approach, f"completion {case}/{frame}")
    for offset in (0x18, 0x1C, 0x20, 0x24, 0x4C, 0x60, 0x61):
        for direction in (0, 1, 2, 3, 128):
            fixture(direction)
            new_base = controls + 0x61 - 16 * 104 - offset
            both(new_base, native_bytes(base, 40 * 104))
            both(0x03002D98, struct.pack("<I", new_base))
            actor = new_base + 16 * 104
            both(controls + 0x60, bytes([1, direction]))
            both(classes, struct.pack("<HH", value(actor + 0x1A, 2), value(actor + 0x22, 2)))
            both(args, bytes([0x60, 0, 1, 0, 16, 0, 0, 1, 0]))
            check_main(approach, f"live alias {offset:X}/{direction}")
    snapshots_rows = [row for row in rows if row["name"] in {"ScriptCallbackBackupTwoRenderSlots",
                      "ScriptCallbackRestoreActorRenderSlotsAndClearFlag", "ScriptCallbackRestoreSavedRenderSlotPair"}]
    for row in snapshots_rows:
        for case in range(64):
            # Equal/disjoint slot intervals can overwrite an index or data
            # consumed by the next copy. These are forward memcpy contracts.
            fixture(case, render_base=snapshots + (4 - case % 8) * 104)
            both(args, bytes([case % 8, (case * 7) % 8]))
            both(controls + 0x14, bytes([case % 8, (case * 7) % 8]))
            check_main(row, f"snapshot alias {case}")
    for row in [r for r in rows if r["source"] in {"src/graphics/screen_pair_more_presets.c", "src/actors/actor_restore_slots.c"}]:
        for case in (0, 1, 254, 255):
            fixture(case);both(0x03002DC0, b"\x01");both(0x03002E10, b"\x01")
            both(controls + 0x14, bytes([case, 255 - case]))
            check_main(row, "mapped inactive slots and optional restore byte selectors")
    saved_pair = next(r for r in rows if r["name"] == "ScriptCallbackRestoreSavedRenderSlotPair")
    for offset in (0, 1, 4, 16, 63, 99):
        fixture(offset, render_base=controls + 0x14 - offset)
        both(controls + 0x14, bytes([0, 1]));check_main(saved_pair, "first copy overwrites live restore indices")
    adjust = next(row for row in rows if row["name"] == "ScriptCallbackAdjustClassTurnTarget")
    for shift in range(256):
        fixture(shift)
        both(controls + 0x63, bytes([shift]))
        check_main(adjust, f"ARM shift count {shift}")
    selection = next(row for row in rows if row["name"] == "ScriptCallbackConfigureRenderSelection")
    for secondary in (0, 1, 255):
        for choice in range(256):
            fixture(choice)
            both(args, bytes([secondary, (0, 0x33, 0x72, 255)[choice % 4], choice]))
            check_main(selection, f"selection byte domain {secondary}/{choice}")
    for secondary in (0, 1):
        for offset in (4, 0x18, 0x20, 0x60, 0x62):
            fixture(offset)
            argument = base + (13 if secondary else 10) * 104 + offset
            both(argument, bytes([secondary, 0x33, 3]))
            both(0x03002DC0, b"\x01");both(0x03002E10, b"\x01")
            check_main(selection, f"selection arguments captured before slot reset {secondary}/{offset}", argument)
    print("45 scene actor/panel functions across 16 families and one shared condition passed.")
