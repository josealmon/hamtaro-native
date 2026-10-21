"""Run input-only scenarios through the compiled native runtime with a private ROM.

No emulator, captured resource, save output or screenshot is involved. The optional
test DLL is not shipped with the application. A scenario fault always fails.
"""
from __future__ import annotations
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import time

ROOT=Path(__file__).resolve().parents[1]
BUTTONS={name:1<<i for i,name in enumerate(("A","B","Select","Start","Right","Left","Up","Down","R","L"))}

class NativeGame:
    def __init__(self,library: Path,rom: bytes,save: bytes|None):
        self.library=C.CDLL(str(library.resolve()))
        self.rom=C.create_string_buffer(rom)
        self.save=C.create_string_buffer(save) if save is not None else None
        def bind(name,result,*arguments):
            function=getattr(self.library,name);function.restype=result;function.argtypes=list(arguments);return function
        pointer=C.c_void_p;word=C.c_uint32
        self.create=bind("hn_probe_game_create",pointer,pointer,C.c_size_t,pointer)
        self.destroy=bind("hn_probe_game_destroy",None,pointer)
        self.advance=bind("hn_probe_game_frame",C.c_int,pointer,C.c_uint16)
        self.report=bind("hn_probe_game_report",None,pointer,C.POINTER(word))
        self.get_memory=bind("hn_probe_game_memory",pointer,pointer)
        self.read=bind("hn_probe_read",C.c_int,pointer,word,pointer,C.c_size_t)
        self.game=self.create(self.rom,len(rom),self.save)
        if not self.game:raise MemoryError("Native runtime allocation failed.")
        self.memory=self.get_memory(self.game)
        try:self.check()
        except BaseException:self.close();raise
    def close(self):
        if self.game:self.destroy(self.game);self.game=None
    def check(self):
        report=(C.c_uint32*7)();self.report(self.game,report)
        if report[0] or not report[6]:
            raise RuntimeError(f"Native fault after {report[4]} completed updates: subsystem={report[0]}, detail={report[1]}, opcode=0x{report[2]:X}, address=0x{report[3]:08X}.")
        return report
    def value(self,address,size=4,signed=False):
        out=C.create_string_buffer(size)
        if not self.read(self.memory,address,out,size):raise RuntimeError("Native checkpoint read failed.")
        return int.from_bytes(out.raw,"little",signed=signed)
    def checkpoint(self):
        report=self.check();slot=self.value(0x03002D98)+self.value(0x03002C0C,1)*104
        return dict(frame=report[4],map_bank=self.value(0x03002C58,1),map_index=self.value(0x03001F68,1),
                    opcode=self.value(0x03000010,1),main_cursor=f"0x{self.value(0x03002C60):08X}",leader_x=self.value(slot+0x18,signed=True)/65536,
                    leader_z=self.value(slot+0x20,signed=True)/65536,learned_words=self.value(0x03003E30,1),seeds=self.value(0x0201DC52,2))
    def frame(self,buttons):
        if not self.advance(self.game,buttons):self.check();raise RuntimeError("Native update failed without a diagnostic.")

def validate_steps(steps,depth=0):
    if not isinstance(steps,list) or depth>8:raise ValueError("Invalid scenario nesting.")
    total=0
    for step in steps:
        if not isinstance(step,dict):raise ValueError("Each scenario step must be an object.")
        if "repeat" in step:
            if set(step)!={"repeat","steps"} or type(step["repeat"]) is not int or not 1<=step["repeat"]<=10000:
                raise ValueError("Invalid repeat step.")
            total+=step["repeat"]*validate_steps(step["steps"],depth+1)
        elif "expect" in step:
            if set(step)-{"expect","label"} or not isinstance(step["expect"],dict):raise ValueError("Invalid checkpoint.")
            fields={"frame","map_bank","map_index","opcode","main_cursor","leader_x","leader_z","learned_words","seeds"}
            if not step["expect"] or set(step["expect"])-fields:raise ValueError("Unknown or empty checkpoint fields.")
        else:
            if set(step)-{"frames","buttons"} or type(step.get("frames")) is not int or not 1<=step["frames"]<=1000000:
                raise ValueError("Invalid input duration.")
            buttons=step.get("buttons",[])
            if not isinstance(buttons,list) or any(not isinstance(b,str) or b not in BUTTONS for b in buttons):
                raise ValueError("Unknown scenario button.")
            total+=step["frames"]
        if total>1000000:raise ValueError("A scenario must contain at most one million updates.")
    return total

def run_steps(game,steps):
    for step in steps:
        if "repeat" in step:
            for _ in range(step["repeat"]):run_steps(game,step["steps"])
        elif "expect" in step:
            actual=game.checkpoint()
            for key,expected in step["expect"].items():
                if actual[key]!=expected:raise RuntimeError(f"Checkpoint {step.get('label','')} {key}: expected {expected}, got {actual[key]}.")
            print(f"Checkpoint {step.get('label','')}: {actual}",flush=True)
        else:
            buttons=0
            for name in step.get("buttons",[]):buttons|=BUTTONS[name]
            for _ in range(step["frames"]):game.frame(buttons)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--rom",type=Path,required=True)
    parser.add_argument("--scenario",type=Path,required=True)
    parser.add_argument("--save",type=Path,help="Optional read-only 32 KiB SRAM input")
    parser.add_argument("--library",type=Path,default=ROOT/"build/Release/hn_reference_probe.dll")
    args=parser.parse_args()
    spec=json.loads((ROOT/"config/rom.json").read_text())
    if args.rom.stat().st_size!=spec["size"]:raise ValueError("A compatible AH3P ROM is required.")
    rom=args.rom.read_bytes()
    if hashlib.sha256(rom).hexdigest()!=spec["sha256"]:raise ValueError("ROM SHA-256 mismatch.")
    saved=None
    if args.save:
        if args.save.stat().st_size!=32768:raise ValueError("Expected 32 KiB SRAM input.")
        saved=args.save.read_bytes()
        if len(saved)!=32768:raise ValueError("SRAM input changed while reading.")
    scenario=json.loads(args.scenario.read_text(encoding="utf-8"))
    if not isinstance(scenario,dict) or set(scenario)!={"name","steps"}:raise ValueError("Invalid scenario document.")
    count=validate_steps(scenario["steps"])
    start=time.perf_counter();game=NativeGame(args.library,rom,saved)
    try:run_steps(game,scenario["steps"])
    finally:game.close()
    print(f"{scenario['name']}: {count} native updates passed in {time.perf_counter()-start:.2f}s. Scenario coverage only; full gameplay remains unverified.")

if __name__=="__main__":main()
