"""Private whole-family checks of argument-free actor render presets."""
from __future__ import annotations
import ctypes as C
import random
import re
import struct


def check_actor_presets(*, entries, reference, rom, memory, cpu, bind, call,
                        both, fresh, equal, compare, native_bytes):
    sources = {"src/actors/actor_animation_linked_presets.c",
               "src/actors/actor_animation_neighbor_presets.c",
               "src/actors/actor_class_animation_presets.c",
               "src/actors/actor_secondary_animation_presets.c",
               "src/actors/actor_render_setup_presets.c"}
    rows = [row for row in entries if row["source"] in sources]
    equal(len(rows), 476, "complete five-family actor preset inventory")
    for row in rows:
        source = (reference / row["source"]).read_text(encoding="utf-8")
        equal(bool(re.search(r"void\s+" + re.escape(row["name"]) + r"\(void\)", source)), True,
              "reviewed actor preset void/argument-free source contract")
    apply = bind("hn_preset_apply", C.c_int, C.c_void_p, C.c_uint32)
    main = bind("hn_scene_callback", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
    task = bind("hn_probe_task_command", C.c_int, C.c_void_p, C.c_uint32, C.c_uint, C.POINTER(C.c_uint32))
    rng = random.Random(0x414354505245)
    ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000))

    def fixture(case):
        fresh()
        both(0x02000000, rom[0x8FE018:0x900EC8])
        both(0x02010000, rng.randbytes(40 * 104))
        for i in range(40):
            both(0x02010000 + i * 104 + 0x18, struct.pack("<III", rng.getrandbits(32),
                                                                         rng.getrandbits(32), rng.getrandbits(32)))
        both(0x02015000, rng.randbytes(16 * 44))
        both(0x03002D98, struct.pack("<I", 0x02010000))
        both(0x03002DC0, b"\x28\x00\x27")
        both(0x03003070, struct.pack("<I", 0x02015000))
        both(0x03002E10, b"\x10")
        both(0x03002180, rng.randbytes(96))
        both(0x03002182, bytes([128 | (case & 1)]))
        both(0x03002170, b"\xA5")
        for i, address in enumerate((0x03002C0C, 0x03001F50, 0x03002C04, 0x03002C00)):
            both(address, bytes([(10, 13, 16, 19)[(i + case) % 4]]))
        if case & 8:
            both(0x03001F50, native_bytes(0x03002C0C, 1))

    for row in rows:
        for case in range(48):
            fixture(case)
            equal(apply(memory, row["address"] | 1), 1, f"actor preset {row['name']}/{case}")
            call(row["name"])
            compare(f"actor preset {row['name']}/{case}", ranges)
        for case in range(8):
            fixture(case)
            both(0x02019000, rng.randbytes(52))
            both(0x02019010, struct.pack("<I", 0x02020000))
            index = case * 13 % 104
            both(0x02020000, bytes([index, 0xE1]))
            both(0x03001FC0 + index * 4, struct.pack("<I", row["address"] | 1))
            report = (C.c_uint32 * 4)()
            equal(task(memory, 0x02019000, 0xF4, report), 1, "actor preset F4 dispatch")
            equal(report[3], call("TaskCommandF4", 0x02019000), "actor preset F4 continuation")
            compare(f"actor preset F4 ABI {row['name']}/{case}", ranges)
        # A void source has no defined Main consumed count. Reject it without
        # mutating guest state; do not call the original through an invalid ABI.
        fixture(0)
        consumed = C.c_uint32(99)
        equal(main(memory, row["address"] | 1, 0, C.byref(consumed)), 0, "void actor preset rejected by Main")
        compare(f"void actor preset leaves Main RAM unchanged {row['name']}", ranges)
    selectors = (("SetFollowerAnimationTowardActor13A5C", 0x03001F50),
                 ("SelectActorAnimationForXZDirection17B18", 0x03002C00),
                 ("SelectActorAnimationForXZDirection18784", 0x03002C04),
                 ("SelectActorAnimationForXZDirection187F8", 0x03002C00))
    for name, selector in selectors:
        row = next(row for row in rows if row["name"] == name)
        for dx, dz in ((0, 0), (-1, 0), (0, -1), (1, 0), (0, 1), (-1, -1),
                       (32767, 32767), (-32768, -32768), (1, 32767)):
            fixture(0)
            index = native_bytes(selector, 1)[0]
            leader = native_bytes(0x03002C0C, 1)[0]
            both(0x02010000 + index * 104 + 0x18, struct.pack("<I", 0x00808000))
            both(0x02010000 + index * 104 + 0x20, struct.pack("<I", 0x00808000))
            both(0x02010000 + leader * 104 + 0x18, struct.pack("<I", (0x00808000 + (dx << 16)) & 0xFFFFFFFF))
            both(0x02010000 + leader * 104 + 0x20, struct.pack("<I", (0x00808000 + (dz << 16)) & 0xFFFFFFFF))
            equal(apply(memory, row["address"] | 1), 1, "actor preset direction sector")
            call(name)
            compare(f"actor preset direction sector {name}/{dx}/{dz}", ranges)
    # Wrapper arguments are captured once, while two separate source calls
    # reload selectors. ConfigureRenderAuxiliarySlot must keep its old parent
    # even when render initialization overwrites the selector's guest byte.
    for name, selector in (("ConfigureActorNeighborSlots13038", 0x03002C0C),
                            ("ConfigureFollowerNeighborSlots13910", 0x03001F50)):
        row = next((row for row in rows if row["name"] == name), None)
        if row is None:
            # Source names are metadata, never guessed. Fail if the requested
            # alias fixture no longer maps to a reviewed source function.
            raise RuntimeError(f"Actor auxiliary source function missing: {name}")
        for case in range(16):
            fixture(case)
            index = native_bytes(selector, 1)[0]
            new_base = selector - (index - 1) * 104 - 0x60
            payload = native_bytes(0x02010000, 40 * 104)
            both(new_base, payload)
            both(0x03002D98, struct.pack("<I", new_base))
            both(0x03002DC0, b"\x28\x00\x27")
            for i, address in enumerate((0x03002C0C, 0x03001F50, 0x03002C04, 0x03002C00)):
                both(address, bytes([(10, 13, 16, 19)[(i + case) % 4]]))
            both(0x03002182, bytes([128 | (case & 1)]))
            equal(apply(memory, row["address"] | 1), 1, "actor auxiliary aliased selector")
            call(name)
            compare(f"actor auxiliary aliased selector {name}/{case}", ranges)
    print("476 argument-free actor presets across five source families passed; full area integration remains unverified.")
