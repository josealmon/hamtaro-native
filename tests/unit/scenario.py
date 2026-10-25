"""Asset-free scenario format and bounded input-schedule checks."""
import contextlib
import io
import json
from pathlib import Path
import runpy
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

MODULE=runpy.run_path(str(Path(__file__).resolve().parents[2]/"tools/run_scenario.py"))
parse=MODULE["input_schedule"]
validate=MODULE["validate_steps"]
run=MODULE["run_steps"]
select=MODULE["select_scenario"]


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
                document=json.loads(path.read_text(encoding="utf-8"))
                self.assertGreater(validate(select(document)["steps"]),0)
                for route in document.get("routes",{}):
                    self.assertGreater(validate(select(document,route)["steps"]),0)

    def test_routes_keep_independent_inputs_and_checkpoint_boundaries(self):
        document=dict(name="first",steps=[dict(frames=2,buttons=["A"]),dict(expect={"frame":2})],
                      routes={"later":dict(name="continuation",steps=[dict(frames=3,buttons=["B"]),dict(expect={"frame":3})])})
        original=json.dumps(document)
        class Game:
            def __init__(self):self.keys=[]
            def frame(self,keys):self.keys.append(keys)
            def checkpoint(self,*args,**kwargs):return dict(frame=len(self.keys))
        for route,keys in ((None,[1,1]),("later",[2,2,2])):
            game=Game();selected=select(document,route)
            with self.subTest(route=route),contextlib.redirect_stdout(io.StringIO()):run(game,selected["steps"])
            self.assertEqual(game.keys,keys)
        self.assertEqual(json.dumps(document),original)

    def test_unknown_or_malformed_routes_cannot_fall_back_to_a_passing_route(self):
        base=dict(name="first",steps=[dict(frames=1)])
        with self.assertRaisesRegex(ValueError,"Unknown scenario route"):select(base,"missing")
        for routes in (None,[],"later",{"":base},{" later":base},{1:base},{None:base},
                       {"later":None},{"later":{"steps":[dict(frames=1)]}},
                       {"later":dict(base,name="")},{"later":dict(base,extra=True)},
                       {"later":dict(base,steps=[dict(frames=0)])},
                       {"later":dict(base,steps=[dict(write=1)])}):
            with self.subTest(routes=routes),self.assertRaises(ValueError):select(dict(base,routes=routes))
        with self.assertRaisesRegex(ValueError,"Unknown scenario route"):
            select(dict(base,routes={"later":base}),"missing")

    def test_selected_route_retains_semantic_failures(self):
        document=dict(name="first",steps=[dict(frames=1)],routes={"later":dict(name="later",steps=[dict(expect={"learned_words":2})])})
        game=SimpleNamespace(checkpoint=lambda *args,**kwargs:{"learned_words":1})
        with contextlib.redirect_stdout(io.StringIO()),self.assertRaisesRegex(RuntimeError,"expected 2, got 1"):
            run(game,select(document,"later")["steps"])

    def test_route_cannot_be_combined_with_an_external_schedule(self):
        arguments=["run_scenario.py","--rom","missing.gba","--input-schedule","missing.txt","--route","later"]
        with patch("sys.argv",arguments),contextlib.redirect_stderr(io.StringIO()) as error,self.assertRaises(SystemExit) as raised:
            MODULE["main"]()
        self.assertEqual(raised.exception.code,2)
        self.assertIn("--route requires --scenario",error.getvalue())

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
        game.checkpoint=lambda flags,controls,classes,record_counters,rock_counts: {"classes":{"0xE1":599}}
        with contextlib.redirect_stdout(io.StringIO()),self.assertRaisesRegex(RuntimeError,"expected.*600.*got.*599"):
            run(game,[dict(label="score",expect={"classes":{"0xE1":600}})])

    def test_inventory_checkpoints_reject_truncation_and_aliases(self):
        for field,limit,maximum in (("record_counters",32,255),("rock_counts",64,31),("rock_keys",64,127),("accessory_records",30,255)):
            self.assertEqual(validate([dict(expect={field:{"0":0,str(limit-1):maximum}})]),0)
            for values in ({"0":True},{"0":-1},{"0":maximum+1},{str(limit):0},
                           {"-1":0},{"1":1,"0x1":2},{},[]):
                with self.subTest(field=field,values=values),self.assertRaises(ValueError):
                    validate([dict(expect={field:values})])

    def test_inventory_checkpoints_read_packed_counts_and_enforce_gems(self):
        calls=[]
        def value(address,size=4,signed=False):
            calls.append((address,size))
            return {0x0201CC20:255,0x0201CC3F:2,0x0201CB20:0xF07C,
                    0x0201CC1C:0xFFFF}.get(address,0)
        game=SimpleNamespace(check=lambda:[0,0,0,0,9],value=value)
        actual=MODULE["NativeGame"].checkpoint(game,record_counters={"0":255,"31":2},
                                               rock_counts={"0":0,"63":31})
        self.assertEqual(actual["record_counters"],{"0":255,"31":2})
        self.assertEqual(actual["rock_counts"],{"0":0,"63":31})
        self.assertEqual(calls[-4:],[(0x0201CC20,1),(0x0201CC3F,1),
                                    (0x0201CB20,2),(0x0201CC1C,2)])
        def checkpoint(flags,controls,classes,record_counters,rock_counts):
            self.assertEqual(record_counters,{"29":2})
            self.assertEqual(rock_counts,{"6":0})
            return inventory
        game.checkpoint=checkpoint
        for counters,rocks,message in ((1,0,"record_counters"),(2,1,"rock_counts")):
            inventory={"record_counters":{"29":counters},"rock_counts":{"6":rocks}}
            with self.subTest(field=message),contextlib.redirect_stdout(io.StringIO()),self.assertRaisesRegex(RuntimeError,message):
                run(game,[dict(label="gems",expect={"record_counters":{"29":2},"rock_counts":{"6":0}})])

    def test_rock_types_are_independent_of_counts_and_high_flags(self):
        def value(address,size=4,signed=False):
            return {0x0201CB20:0xF07C,0x0201CB24:5<<7,0x0201CC1C:0xFFFF}.get(address,0)
        game=SimpleNamespace(check=lambda:[0,0,0,0,9],value=value)
        actual=MODULE["NativeGame"].checkpoint(game,rock_counts={"1":5},rock_keys={"0":124,"1":0,"63":127})
        self.assertEqual(actual["rock_counts"],{"1":5})
        self.assertEqual(actual["rock_keys"],{"0":124,"1":0,"63":127})
        game.checkpoint=lambda *args,**kwargs:actual
        with contextlib.redirect_stdout(io.StringIO()),self.assertRaisesRegex(RuntimeError,"rock_keys"):
            run(game,[dict(expect={"rock_counts":{"1":5},"rock_keys":{"1":1}})])


    def test_accessory_checkpoints_detect_lost_or_unremoved_crafts(self):
        calls=[]
        def value(address,size=4,signed=False):
            calls.append((address,size));return {0x0201EE40:18,0x0201EE5D:0}.get(address,0)
        game=SimpleNamespace(check=lambda:[0,0,0,0,9],value=value)
        actual=MODULE["NativeGame"].checkpoint(game,accessory_records={"0":0,"29":18})
        self.assertEqual(actual["accessory_records"],{"0":18,"29":0})
        self.assertEqual(calls[-2:],[(0x0201EE40,1),(0x0201EE5D,1)])
        def checkpoint(*args,**extra):
            self.assertEqual(extra,{"accessory_records":{"0":18,"29":0}})
            return {"accessory_records":observed}
        game.checkpoint=checkpoint
        for observed in ({"0":0,"29":0},{"0":18,"29":18}):
            with self.subTest(observed=observed),contextlib.redirect_stdout(io.StringIO()),self.assertRaisesRegex(RuntimeError,"accessory_records"):
                run(game,[dict(label="craft",expect={"accessory_records":{"0":18,"29":0}})])

    def test_catalog_checkpoints_reject_truncation_and_aliases(self):
        self.assertEqual(validate([dict(expect={"catalog_flags":{"0":0,"109":255},"catalog_count":255})]),0)
        for values in ({"0":True},{"0":-1},{"0":256},{"110":0},{"-1":0},{"1":1,"0x1":2},{},[]):
            with self.subTest(values=values),self.assertRaises(ValueError):validate([dict(expect={"catalog_flags":values})])
        for value in (True,-1,256,1.0,"30",None):
            with self.subTest(value=value),self.assertRaises(ValueError):validate([dict(expect={"catalog_count":value})])

    def test_catalog_checkpoints_read_saved_flags_and_enforce_acquisition(self):
        calls=[]
        def value(address,size=4,signed=False):
            calls.append((address,size));return {0x03003C50:191,0x03003E04:255,0x030035CC:1}.get(address,0)
        game=SimpleNamespace(check=lambda:[0,0,0,0,9],value=value)
        actual=MODULE["NativeGame"].checkpoint(game,catalog_flags={"0":255,"109":255},catalog_count=30)
        self.assertEqual(actual["catalog_flags"],{"0":191,"109":255})
        self.assertEqual(actual["catalog_count"],1)
        self.assertEqual(calls[-3:],[(0x03003C50,1),(0x03003E04,1),(0x030035CC,1)])
        def checkpoint(*args,**extra):
            self.assertEqual(extra,{"catalog_flags":{"0":255},"catalog_count":1});return observed
        game.checkpoint=checkpoint
        for flags,count,field in ((191,1,"catalog_flags"),(255,0,"catalog_count")):
            observed={"catalog_flags":{"0":flags},"catalog_count":count}
            with self.subTest(field=field),contextlib.redirect_stdout(io.StringIO()),self.assertRaisesRegex(RuntimeError,field):
                run(game,[dict(label="clothes",expect={"catalog_flags":{"0":255},"catalog_count":1})])

    def test_photo_records_reject_malformed_layouts_and_aliases(self):
        photo=dict(background=1,outfits=[[0,0],[255,0],[255,0],[0,1],[255,0],[255,0]])
        self.assertEqual(validate([dict(expect={"photo_records":{"0":photo,"5":photo}})]),0)
        for photos in ({},[],{"6":photo},{"-1":photo},{1:photo},{"1":photo,"0x1":photo}):
            with self.subTest(photos=photos),self.assertRaises(ValueError):validate([dict(expect={"photo_records":photos})])
        for invalid in ({},dict(photo,extra=0),dict(photo,background=True),dict(photo,background=256),
                        dict(photo,background=-1),dict(photo,background=1.0),dict(photo,outfits=[]),
                        dict(photo,outfits=photo["outfits"][:-1]),dict(photo,outfits=[[0,0]]*7),
                        dict(photo,outfits=[(0,0)]*6),dict(photo,outfits=[[0,True]]*6),
                        dict(photo,outfits=[[256,0]]*6),dict(photo,outfits=[[0,-1]]*6)):
            with self.subTest(photo=invalid),self.assertRaises(ValueError):validate([dict(expect={"photo_records":{"0":invalid}})])

    def test_photo_records_read_both_characters_without_record_padding(self):
        expected=dict(background=2,outfits=[[0,0],[255,0],[255,0],[0,1],[255,0],[255,0]])
        payload=[2,0,0,255,0,255,0,0,1,255,0,255,0];base=0x03003730+5*16;calls=[]
        def value(address,size=4,signed=False):
            calls.append((address,size));return payload[address-base] if base<=address<base+13 else 0
        game=SimpleNamespace(check=lambda:[0,0,0,0,9],value=value)
        actual=MODULE["NativeGame"].checkpoint(game,photo_records={"5":dict(expected,background=1)})
        self.assertEqual(actual["photo_records"],{"5":expected})
        self.assertEqual(calls[-13:],[(base+i,1) for i in range(13)])
        def checkpoint(*args,**extra):
            self.assertEqual(extra,{"photo_records":{"5":expected}});return observed
        game.checkpoint=checkpoint
        for changed in (dict(expected,background=1),dict(expected,outfits=[[0,1]]+expected['outfits'][1:]),
                        dict(expected,outfits=expected['outfits'][:3]+[[0,0]]+expected['outfits'][4:])):
            observed={"photo_records":{"5":changed}}
            with self.subTest(photo=changed),contextlib.redirect_stdout(io.StringIO()),self.assertRaisesRegex(RuntimeError,'photo_records'):
                run(game,[dict(expect={"photo_records":{"5":expected}})])

    def test_outfit_indices_reject_truncation_and_read_actual_colors(self):
        expected=[0,0,0,1,0,0]
        self.assertEqual(validate([dict(expect={"outfit_indices":expected})]),0)
        for invalid in ([],expected[:-1],expected+[0],[0]*5+[True],[0]*5+[256],[0]*5+[-1],tuple(expected)):
            with self.subTest(indices=invalid),self.assertRaises(ValueError):validate([dict(expect={"outfit_indices":invalid})])
        observed=[1,0,0,0,0,0];calls=[]
        def value(address,size=4,signed=False):
            calls.append((address,size));return observed[address-0x03003790] if 0x03003790<=address<0x03003796 else 0
        game=SimpleNamespace(check=lambda:[0,0,0,0,9],value=value)
        actual=MODULE["NativeGame"].checkpoint(game,outfit_indices=expected)
        self.assertEqual(actual['outfit_indices'],observed)
        self.assertEqual(calls[-6:],[(0x03003790+i,1) for i in range(6)])
        game.checkpoint=lambda *args,**extra:actual
        with contextlib.redirect_stdout(io.StringIO()),self.assertRaisesRegex(RuntimeError,'outfit_indices'):
            run(game,[dict(expect={"outfit_indices":expected})])

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
