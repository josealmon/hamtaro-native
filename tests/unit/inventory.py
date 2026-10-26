"""Synthetic inventory guards; no game resources or reference checkout needed."""
import hashlib
import json
from pathlib import Path
import runpy
import tempfile
import unittest
from unittest.mock import patch

MODULE = runpy.run_path(str(Path(__file__).resolve().parents[2] / "tools/inventory_reference.py"))
fields = MODULE["background_fields"]
decode = MODULE["background_commands"]
inventory = MODULE["background_inventory"]
task_stores = MODULE["task_callback_stores"]
task_inventory = MODULE["task_callback_inventory"]
BASE = 0x08000000


class BackgroundInventoryTests(unittest.TestCase):
    def test_fixed_fields_ignore_comments_and_strings_and_keep_macro_boundaries(self):
        text = '''#define B_WAIT(n) 0x81, LE16(n)
        #define B_RETURN 0x80
        /* B_FAKE(0) */
        .text = { "B_FAKE" },
        .script_800000 = { B_WAIT(0x9C), B_RETURN },
        '''
        self.assertEqual(fields(text), [dict(field="script_800000", address=0x08800000, opcodes=[0x81, 0x80])])

    def test_positional_byte_layout_retains_end_and_low_byte_yield(self):
        text = '''typedef struct { u8 first[2], second[7], third[1]; } Fixture;
        #define SCRIPT_ADDRESS(member) (0x08000000u + (u32)&((Fixture *)0)->member)
        #define END 0x80
        #define B_WRITE(target,value) \\
            0x93, LE32(target), (value)
        #define B_YIELD 0
        const Fixture example = {
            { END },
            { B_WRITE(0,0), END },
            { B_YIELD }
        };
        '''
        self.assertEqual(fields(text), [dict(field="first", address=BASE, opcodes=[0x80]),
                                       dict(field="second", address=BASE + 2, opcodes=[0x93, 0x80]),
                                       dict(field="third", address=BASE + 9, opcodes=[0])])

    def test_unclassified_or_pointer_layouts_are_not_silently_skipped(self):
        for text in ("#define B_RETURN 0x80\nconst u8 example[] = { B_RETURN };",
                     "#define B_RETURN 0x80\n.script_unknown = { B_RETURN };",
                     ".script_800000 = { B_UNKNOWN };",
                     "#define B_INVALID 0xFF\n.script_800000 = { B_INVALID };",
                     '''typedef struct { const u8 *pointer; } Fixture;
                     #define SCRIPT_ADDRESS(member) (0x08000000u + (u32)&((Fixture *)0)->member)
                     #define B_RETURN 0x80
                     const Fixture x = { { B_RETURN } };'''):
            with self.subTest(text=text), self.assertRaises(ValueError):
                fields(text)

    def test_jump_does_not_decode_operands_or_unreachable_queue_bytes(self):
        base = BASE + 0x90
        data = bytes([0x82]) + (base + 12).to_bytes(4, "little") + bytes([0x9C] * 7) + bytes([0x80])
        self.assertEqual(decode(data, [base], base), {base: 0x82, base + 12: 0x80})
        loop = bytes([0x82]) + BASE.to_bytes(4, "little")
        self.assertEqual(decode(loop, [BASE]), {BASE: 0x82})

    def test_call_includes_callee_queue_and_return_continuation(self):
        data = bytearray(19)
        data[:5] = bytes([0xA0]) + (BASE + 12).to_bytes(4, "little")
        data[5] = data[18] = 0x80
        data[12] = 0x9C
        self.assertEqual(decode(bytes(data), [BASE]),
                         {BASE: 0xA0, BASE + 5: 0x80, BASE + 12: 0x9C, BASE + 18: 0x80})

    def test_each_conditional_visits_both_outcomes(self):
        for opcode in (0xB0, 0xB1, 0xB2):
            data = bytearray(26)
            data[0] = opcode
            data[6:10] = (BASE + 12).to_bytes(4, "little")
            data[10] = data[25] = 0x80
            data[12] = 0x9E
            with self.subTest(opcode=opcode):
                self.assertEqual(set(decode(bytes(data), [BASE])), {BASE, BASE + 10, BASE + 12, BASE + 25})

    def test_repeats_cover_exit_and_data_does_not_become_an_opcode(self):
        data = bytes([0x90, 3, 0x93, 0x9C, 0, 0, 0, 0, 0x91, 0x80])
        self.assertEqual(decode(data, [BASE]),
                         {BASE: 0x90, BASE + 2: 0x93, BASE + 8: 0x91, BASE + 9: 0x80})

    def test_original_stops_do_not_advance_into_data(self):
        for opcode in (0, 0x7F, 0x80, 0x8C, 0x8D, 0x8E, 0x8F, 0x96, 0x98, 0x9B):
            with self.subTest(opcode=opcode):
                self.assertEqual(decode(bytes([opcode, 0x9C]), [BASE]), {BASE: opcode})

    def test_unknown_truncated_and_external_targets_fail(self):
        for data, roots in ((b"\xff", [BASE]), (b"\x9f", [BASE]), (b"\x80", [BASE - 1]),
                            (bytes([0x82]) + (BASE + 20).to_bytes(4, "little"), [BASE])):
            with self.subTest(data=data, roots=roots), self.assertRaises(ValueError):
                decode(data, roots)

    def test_rom_identity_and_source_disagreement_cannot_produce_a_report(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "config").mkdir()
            source = root / "src/data/graphics"
            source.mkdir(parents=True)
            (source / "fixture.c").write_text("#define B_RETURN 0x80\n.script_000000 = { B_RETURN };", encoding="utf-8")
            rom = root / "input.rom"
            rom.write_bytes(b"\x80")
            specification = root / "config/rom.json"
            specification.write_text(json.dumps(dict(sha256=hashlib.sha256(rom.read_bytes()).hexdigest())), encoding="utf-8")
            with patch.dict(inventory.__globals__, ROOT=root):
                with patch.object(inventory.__globals__["subprocess"], "check_output", side_effect=["fixture\n", b""]):
                    report = inventory(root, rom)
                self.assertEqual((report["command_count"], report["cpu_queue_sites"]), (1, []))
                rom.write_bytes(b"\x81")
                with self.assertRaisesRegex(ValueError, "supported AH3P"):
                    inventory(root, rom)
                specification.write_text(json.dumps(dict(sha256=hashlib.sha256(rom.read_bytes()).hexdigest())), encoding="utf-8")
                with self.assertRaisesRegex(ValueError, "Source/ROM"):
                    inventory(root, rom)


class TaskCallbackInventoryTests(unittest.TestCase):
    SOURCE = ".store_000000 = { 0x25, 7, (void (*)(void))FixtureTask };"
    ROM = bytes((0x25, 7)) + (BASE + 9).to_bytes(4, "little")
    SYMBOLS = {"FixtureTask": BASE + 8}

    def test_typed_store_verifies_slot_and_thumb_pointer(self):
        self.assertEqual(task_stores(self.SOURCE, self.ROM, self.SYMBOLS),
                         [dict(field="store_000000", site="0x08000000", slot=7,
                               name="FixtureTask", address="0x08000008")])

    def test_f4_index_and_other_opcodes_are_not_pointer_stores(self):
        self.assertEqual(task_stores(".text_000000 = { 0xF4, 7, FixtureTask };", self.ROM, self.SYMBOLS), [])
        self.assertEqual(task_stores(".callback_000000 = { 0x1C, FixtureTask };", self.ROM, self.SYMBOLS), [])

    def test_corrupt_slots_targets_and_unresolved_stores_fail(self):
        for data in (self.ROM[:5], bytes((0x25, 8)) + self.ROM[2:],
                     bytes((0x25, 7)) + (BASE + 11).to_bytes(4, "little")):
            with self.subTest(data=data), self.assertRaisesRegex(ValueError, "Source/ROM"):
                task_stores(self.SOURCE, data, self.SYMBOLS)
        for source in (self.SOURCE.replace("FixtureTask", "Unknown"),
                       self.SOURCE.replace("FixtureTask", "0"), self.SOURCE.replace("000000", "unknown")):
            with self.subTest(source=source), self.assertRaisesRegex(ValueError, "Unclassified"):
                task_stores(source, self.ROM, self.SYMBOLS)

    def test_main_membership_does_not_count_as_a_task_binding(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "src/data/graphics"
            source.mkdir(parents=True)
            (source / "fixture.c").write_text(self.SOURCE, encoding="utf-8")
            original = dict(reference_revision="fixture", reference_dirty=False,
                            sources=[dict(functions=[dict(name="FixtureTask", address="0x08000008",
                                                          native_dispatch_abis=["main"])])])
            with patch.dict(task_inventory.__globals__, private_rom=lambda _: (self.ROM, "fixture"),
                            inventory=lambda *_: original):
                report = task_inventory(root, root / "input.rom")
            self.assertEqual(len(report["missing_task_bindings"]), 1)
            self.assertEqual(report["unique_targets"], 1)


if __name__ == "__main__":
    unittest.main()
