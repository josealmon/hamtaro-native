"""Asset-free scenario format and bounded input-schedule checks."""
import contextlib
import io
import json
from pathlib import Path
import runpy
import tempfile
from types import SimpleNamespace
import unittest

MODULE=runpy.run_path(str(Path(__file__).resolve().parents[2]/"tools/run_scenario.py"))
parse=MODULE["input_schedule"]
validate=MODULE["validate_steps"]
run=MODULE["run_steps"]


class ScenarioTests(unittest.TestCase):
    def test_schedule_preserves_input_order_and_observes_state(self):
        document=parse("# synthetic\n0 0 initial\n2 0 -\n0 0 -\n1 312 checkpoint # comment\n", "synthetic")
        class Game:
            def __init__(self):self.keys=[];self.observed=[]
            def frame(self,keys):self.keys.append(keys)
            def checkpoint(self):self.observed.append(len(self.keys));return dict(frame=len(self.keys))
        game=Game()
        with contextlib.redirect_stdout(io.StringIO()) as output:run(game,document["steps"])
        self.assertEqual(game.keys,[0,0,0x312])
        self.assertEqual(game.observed,[0,3])
        self.assertIn("Observation checkpoint",output.getvalue())

    def test_malformed_or_ambiguous_schedules_fail(self):
        for text in ("", "# empty", "1 0", "1 0 - extra", "x 0 -", "1 nope -",
                     "0 0 -", "-1 0 -", "1000001 0 -", "1 -1 -", "1 400 -",
                     "1 0 same\n1 1 same", "1000000 0 -\n1000000 0 -\n1 0 -"):
            with self.subTest(text=text),self.assertRaises(ValueError):parse(text,"bad")

    def test_full_story_size_and_repeat_budget(self):
        document=parse("1000000 0 -\n224925 0 end", "long")
        self.assertEqual(validate(document["steps"]),1224925)
        self.assertEqual(validate([dict(repeat=2,steps=[dict(frames=1000000)])]),2000000)
        with self.assertRaises(ValueError):validate([dict(repeat=3,steps=[dict(frames=1000000)])])

    def test_observations_cannot_weaken_semantic_expectations(self):
        for step in (dict(observe=""),dict(observe=True),dict(observe="x",expect={}),
                     dict(expect={}),dict(expect={"controls":{"0x1":True}}),
                     dict(expect={"flags":{"0x1":1}}),
                     dict(expect={"flags":{"1":True,"0x1":False}}),
                     dict(expect={"controls":{"512":0}})):
            with self.subTest(step=step),self.assertRaises(ValueError):validate([step])

    def test_versioned_scenarios_remain_valid(self):
        paths=list((Path(__file__).resolve().parents[1]/"scenarios").glob("*.json"))
        self.assertTrue(paths)
        for path in paths:
            with self.subTest(path=path.name):
                self.assertGreater(validate(json.loads(path.read_text(encoding="utf-8"))["steps"]),0)

    def test_class_checkpoints_reject_truncation_and_aliases(self):
        self.assertEqual(validate([dict(expect={"classes":{"0":0,"255":65535}})]),0)
        for values in ({"0":True},{"0":-1},{"0":65536},{"256":0},
                       {"-1":0},{"1":1,"0x1":2},{}):
            with self.subTest(values=values),self.assertRaises(ValueError):
                validate([dict(expect={"classes":values})])

    def test_class_checkpoints_read_halfwords_and_enforce_scores(self):
        calls=[]
        def value(address,size=4,signed=False):
            calls.append((address,size))
            return {0x0201DE00:3600,0x0201DE02:600}.get(address,0)
        game=SimpleNamespace(check=lambda:[0,0,0,0,9],value=value)
        actual=MODULE["NativeGame"].checkpoint(game,classes={"0xE0":3600,"0xE1":600})
        self.assertEqual(actual["classes"],{"0xE0":3600,"0xE1":600})
        self.assertEqual(calls[-2:],[(0x0201DE00,2),(0x0201DE02,2)])
        game.checkpoint=lambda flags,controls,classes: {"classes":{"0xE1":599}}
        with contextlib.redirect_stdout(io.StringIO()),self.assertRaisesRegex(RuntimeError,"expected.*600.*got.*599"):
            run(game,[dict(label="score",expect={"classes":{"0xE1":600}})])


    def test_saved_profile_requires_every_sdk_byte(self):
        expected=bytes(32768);verify=MODULE["verify_profile_sram"]
        with tempfile.TemporaryDirectory() as directory:
            profile=Path(directory);save=profile/"game.sav"
            with self.assertRaises(FileNotFoundError):verify(profile,expected)
            for actual in (expected[:-1],expected+b'\0',expected[:17000]+b'\1'+expected[17001:]):
                save.write_bytes(actual)
                with self.subTest(size=len(actual)),self.assertRaises(RuntimeError):verify(profile,expected)
            save.write_bytes(expected)
            self.assertEqual(verify(profile,expected),MODULE['hashlib'].sha256(expected).hexdigest())
            self.assertEqual(save.read_bytes(),expected)
            with self.assertRaises(ValueError):verify(profile,b'')


if __name__=="__main__":unittest.main()
