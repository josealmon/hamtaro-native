"""ROM-free angular BIOS checks against separately compiled mGBA math helpers."""
import argparse
import ctypes as C
import os
from pathlib import Path
import random
import struct
import subprocess

class BiosOracle:
    def __init__(self, executable, runtime=None):
        environment=os.environ.copy()
        if runtime:environment["PATH"]=str(runtime)+os.pathsep+environment.get("PATH", "")
        self.process=subprocess.Popen([str(executable.resolve())],stdin=subprocess.PIPE,stdout=subprocess.PIPE,env=environment)
        self.calls=0
    def call(self, service, x, y=0):
        self.calls+=1
        self.process.stdin.write(struct.pack("<3I",service,x&0xFFFFFFFF,y&0xFFFFFFFF));self.process.stdin.flush()
        data=self.process.stdout.read(16)
        if len(data)!=16:raise RuntimeError("BIOS arithmetic oracle did not return a complete response.")
        return struct.unpack("<4I",data)
    def close(self):
        self.process.stdin.close()
        try:status=self.process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.process.kill();self.process.wait();raise RuntimeError("BIOS arithmetic oracle did not stop after EOF.")
        if status:raise RuntimeError(f"BIOS arithmetic oracle exited with status {status}.")
    def __enter__(self):return self
    def __exit__(self,*_args):self.close()

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--oracle",required=True,type=Path)
    parser.add_argument("--oracle-runtime-dir",type=Path)
    parser.add_argument("--native",type=Path,default=Path(__file__).resolve().parents[1]/"build/Release/hn_reference_probe.dll")
    args=parser.parse_args();library=C.CDLL(str(args.native.resolve()))
    atan=library.hn_bios_arctan;atan.argtypes=[C.c_int32];atan.restype=C.c_int16
    atan2=library.hn_bios_arctan2;atan2.argtypes=[C.c_int32,C.c_int32];atan2.restype=C.c_uint16
    checks=0;rng=random.Random(0xB105)
    with BiosOracle(args.oracle,args.oracle_runtime_dir) as oracle:
        for value in range(-16384,16385):
            expected=oracle.call(9,value)[0]&65535
            if atan(value)&65535!=expected:raise AssertionError(f"ArcTan mismatch at {value}.")
            checks+=1
        edges=(-32768,-32767,-1024,-255,-2,-1,0,1,2,255,1024,32767)
        pairs=[(x,y) for x in edges for y in edges]+[(rng.randrange(-32768,32768),rng.randrange(-32768,32768)) for _ in range(4096)]
        for x,y in pairs:
            if atan2(x,y)!=oracle.call(10,x,y)[0]:raise AssertionError(f"ArcTan2 mismatch at {x}, {y}.")
            checks+=1
    print(f"{checks} ROM-free BIOS angular comparisons passed.")

if __name__=="__main__":main()
