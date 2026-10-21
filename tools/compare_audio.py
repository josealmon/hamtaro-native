"""ROM-free timer/PSG/Direct Sound component checks with a separate mGBA oracle."""
import argparse
import ctypes as ct
import os
from pathlib import Path
import random
import struct
import subprocess

STATE_WORDS = 47
NAMES = (["cycles"] + [f"timer{i}.count" for i in range(4)]
         + [f"timer{i}.reload" for i in range(4)] + [f"timer{i}.control" for i in range(4)]
         + ["IF", "SOUNDCNT_H", "SOUNDCNT_X", "SOUNDBIAS"]
         + [f"fifo{i}.{field}" for i in range(2) for field in ("read", "write", "remaining", "shifter")]
         + [f"fifo{i}.word{j}" for i in range(2) for j in range(8)]
         + ["dma1.source", "dma2.source", "fifo0.sample", "fifo1.sample", "dac.left", "dac.right"])
PSG_NAMES = (["cycles", "PSG status", "PSG volume/routing", "frame next"]
             + [f"psg{i}.{field}" for field in ("length", "frequency", "volume", "sample", "active") for i in range(4)]
             + ["pulse0.phase", "pulse1.phase", "sweep.shadow", "sweep.clock"]
             + [f"wave.byte{i}" for i in range(32)] + [f"register {0x60+i:#x}" for i in range(34)]
             + ["PSG left", "PSG right"])

class Comparison:
    def __init__(self, native, oracle, runtime):
        self.dll = ct.CDLL(str(native))
        self.dll.hn_probe_hardware_create.restype = ct.c_void_p
        self.dll.hn_probe_hardware_destroy.argtypes = [ct.c_void_p]
        self.dll.hn_probe_hardware_write.argtypes = [ct.c_void_p, ct.c_uint, ct.c_uint32, ct.c_uint32]
        self.dll.hn_probe_hardware_advance.argtypes = [ct.c_void_p, ct.c_uint32]
        self.dll.hn_probe_hardware_state.argtypes = [ct.c_void_p, ct.POINTER(ct.c_uint32)]
        self.dll.hn_probe_hardware_psg_state.argtypes = [ct.c_void_p, ct.POINTER(ct.c_uint32)]
        env = os.environ.copy()
        if runtime:
            env["PATH"] = str(runtime) + os.pathsep + env.get("PATH", "")
        self.process = subprocess.Popen([str(oracle)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, env=env)
        self.native = None
        self.checks = self.snapshots = 0
        self.state = [0] * STATE_WORDS
        self.reset()

    def command(self, op, address=0, value=0):
        self.process.stdin.write(struct.pack("<4I", op, address, value, 0))

    def reset(self):
        if self.native:
            self.dll.hn_probe_hardware_destroy(self.native)
        self.native = self.dll.hn_probe_hardware_create()
        if not self.native:
            raise RuntimeError("Could not create native hardware probe.")
        self.command(0)

    def write(self, width, offset, value):
        if not self.dll.hn_probe_hardware_write(self.native, width, 0x04000000 + offset, value):
            raise AssertionError(f"Native IO write fault at {offset:#x}.")
        self.command(width, offset, value)

    def seed(self, offset, value):
        if not self.dll.hn_probe_hardware_write(self.native, 4, 0x02000000 + offset, value):
            raise AssertionError("Native synthetic RAM seed failed.")
        self.command(8, offset, value)

    def advance(self, cycles):
        if not self.dll.hn_probe_hardware_advance(self.native, cycles):
            raise AssertionError("Native hardware clock fault.")
        self.command(16, value=cycles)

    def compare(self, label, dac=True, dma=False):
        native = (ct.c_uint32 * STATE_WORDS)()
        self.dll.hn_probe_hardware_state(self.native, native)
        self.command(32)
        self.process.stdin.flush()
        data = bytearray()
        while len(data) < STATE_WORDS * 4:
            block = self.process.stdout.read(STATE_WORDS * 4 - len(data))
            if not block:
                raise RuntimeError(f"Audio oracle terminated ({self.process.poll()}).")
            data += block
        reference = struct.unpack(f"<{STATE_WORDS}I", data)
        for i, name in enumerate(NAMES):
            if not dac and i >= 45 or not dma and i in (41, 42):
                continue
            self.checks += 1
            if native[i] != reference[i]:
                raise AssertionError(f"{label}, {name}: native {native[i]:#x}, reference {reference[i]:#x}")
        self.state = list(native)
        self.snapshots += 1

    def compare_psg(self, label, fields=None):
        native = (ct.c_uint32 * 96)()
        self.dll.hn_probe_hardware_psg_state(self.native, native)
        self.command(64)
        self.process.stdin.flush()
        data = bytearray()
        while len(data) < 96 * 4:
            block = self.process.stdout.read(96 * 4 - len(data))
            if not block:
                raise RuntimeError(f"PSG oracle terminated ({self.process.poll()}).")
            data += block
        reference = struct.unpack("<96I", data)
        self.psg_native, self.psg_reference = list(native), list(reference)
        for i in range(96) if fields is None else fields:
            # The oracle stops advancing silent pulse phases; native dividers
            # keep running after their first trigger. Compare audible phases.
            if i in (24, 25) and (not native[20+i-24] or not native[12+i-24]):
                continue
            if i == 27 and not native[20]:
                continue  # The oracle stops its sweep countdown on overflow.
            self.checks += 1
            if native[i] != reference[i]:
                raise AssertionError(f"{label}, {PSG_NAMES[i]}: native {native[i]:#x}, reference {reference[i]:#x}")
        self.snapshots += 1

    def close(self):
        if self.native:
            self.dll.hn_probe_hardware_destroy(self.native)
            self.native = None
        self.process.stdin.close()
        try:
            self.process.wait(timeout=10)
        finally:
            if self.process.poll() is None:
                self.process.kill()
                self.process.wait()
            self.process.stdout.close()
        if self.process.returncode:
            raise RuntimeError(f"Audio oracle exit status {self.process.returncode}.")

def timers(c, rng):
    for case in range(48):
        c.reset()
        c.advance(rng.randrange(32))
        for i in range(4):
            control = 0xC0 | rng.randrange(4) | (4 if i and case & 1 else 0)
            reload = rng.randrange(0xFF00, 0x10000) if case & 2 else rng.choice((0, 0x1234, 0xFFFE, 0xFFFF))
            c.write(4, 0x100 + i * 4, reload | control << 16)
        for step in range(48):
            if step % 7 == 0:
                c.write(2, 0x100 + rng.randrange(4) * 4, rng.randrange(0xFF00, 0x10000))
            if step % 13 == 0:
                i = rng.randrange(4)
                c.write(2, 0x102 + i * 4, 0x40)
                c.advance(rng.randrange(16))
                c.write(2, 0x102 + i * 4, 0xC0 | rng.randrange(4))
            if step % 11 == 0:
                i = rng.randrange(4)
                c.write(2, 0x102 + i * 4, 0xC0 | rng.randrange(4) | (4 if i and step & 1 else 0))
            c.advance(rng.randrange(1, 2048))
            c.compare(f"timers case {case} step {step}")

def fifos(c, rng):
    for case in range(32):
        c.reset()
        c.write(2, 0x84, 0x80)
        resolution = case & 3
        c.write(2, 0x88, 0x200 | resolution << 14)
        control = (rng.randrange(1, 4) << 8 | rng.randrange(1, 4) << 12
                   | rng.randrange(4) << 2 | (0x400 if case & 4 else 0) | (0x4000 if case & 8 else 0))
        c.write(2, 0x82, control | 0x8800)
        c.write(4, 0x100, (0x10000 - rng.choice((128, 256, 512, 1024))) | 0x800000)
        c.write(4, 0x104, (0x10000 - rng.choice((192, 384, 768))) | 0x800000)
        c.compare(f"FIFO case {case} init")
        for step in range(96):
            for i in range(2):
                read, write = c.state[17 + i * 4:19 + i * 4]
                if ((write - read) & 7) < 4:
                    for _ in range(3):
                        c.write(4, 0xA0 + i * 4, rng.getrandbits(32))
            if step % 23 == 0:
                c.write(2, 0x82, control | (0x800 if step & 1 else 0x8000))
            if step == 57:
                c.write(2, 0x84, 0)
            if step == 59:
                c.write(2, 0x84, 0x80)
                c.write(2, 0x82, control)
            c.advance(rng.randrange(1, 1536))
            # mGBA 0.10.5 keeps routing held PCM while master power is off.
            # The native unit contract checks the documented mute separately.
            c.compare(f"FIFO case {case} step {step}", dac=not 57 <= step <= 60)

def dma(c, rng):
    for case in range(12):
        c.reset()
        for i in range(1024):
            c.seed(i * 4, rng.getrandbits(32))
        c.write(2, 0x84, 0x80)
        c.write(2, 0x82, 0xA90E)
        for i in range(2):
            mode = case % 3
            source = 0x02000000 + i * 2048 + (1792 if mode == 1 else 0)
            c.write(4, 0xBC + i * 12, source)
            c.write(4, 0xC0 + i * 12, 0x040000A0 + i * 4)
            c.write(4, 0xC4 + i * 12, 0xF2000000 | mode << 23 | rng.randrange(1, 0x100))
        # Inspect 128 cycles after timer edges, after each DMA burst has settled.
        c.write(4, 0x100, 0x0080FC00)
        c.advance(128)
        for step in range(192):
            c.advance(1024)
            if step % 8 == 0:
                c.compare(f"DMA case {case} step {step}", dma=True)

def psg(c, rng):
    for case in range(32):
        c.reset()
        c.write(2, 0x84, 0x80)
        c.write(2, 0x80, 0xBB77)
        c.write(2, 0x82, 2)
        # Initial trigger at clock zero, then steady periods. First-step and
        # retrigger divider behavior have independent analytic native tests.
        for i in range(2):
            duty = rng.randrange(4)
            envelope = rng.randrange(1, 16) << 4 | rng.randrange(8) | (8 if case & 1 else 0)
            c.write(2, 0x62 if i == 0 else 0x68, envelope << 8 | duty << 6 | rng.randrange(64))
            c.write(2, 0x64 if i == 0 else 0x6C, 0x8000 | rng.randrange(0x400, 0x7F0) | (0x4000 if case & 2 else 0))
        c.write(2, 0x78, (rng.randrange(1, 16) << 12) | (rng.randrange(1, 8) << 8) | rng.randrange(64))
        noise = rng.randrange(0xD0)
        c.write(2, 0x7C, 0x8000 | noise | (0x4000 if case & 4 else 0))
        # Align the controlled polynomial seed, not the stock oracle's startup.
        c.command(128, value=0x40 if noise & 8 else 0x4000)
        for step in range(64):
            c.advance(rng.randrange(32, 128) * 512)
            c.compare_psg(f"PSG case {case} step {step}")

def waves(c, rng):
    for case in range(24):
        c.reset()
        c.write(2, 0x84, 0x80)
        c.write(2, 0x80, 0x4477)
        # Independent synthetic content in both banks; no extracted wave data.
        for bank in range(2):
            c.write(1, 0x70, bank << 6)
            for i in range(4):
                c.write(4, 0x90+i*4, rng.getrandbits(32))
        two_banks = bool(case & 2)
        bank = 0 if two_banks else case & 1
        c.write(1, 0x70, 0x80 | (0x20 if two_banks else 0) | bank << 6)
        c.write(2, 0x72, (case % 8) << 13 | rng.randrange(128))
        frequency = rng.randrange(0x600, 0x7FC)
        period = 8 * (2048-frequency)
        c.write(2, 0x74, 0x8000 | frequency)
        # Stock mGBA inserts an unexplained 24-cycle startup delay. Inspect
        # after both models have crossed the same oscillator edge, before the
        # next one; this verifies steady wave ordering, not trigger latency.
        c.advance(28)
        for step in range(48):
            c.advance(period * rng.randrange(1, 4))
            c.compare_psg(f"wave case {case} step {step}")

def sweeps(c, rng):
    # mGBA changes an in-flight period differently from a reload divider.
    # Compare sweep/length/register state; native phase reload has analytic tests.
    fields = [i for i in range(96) if i not in (16, 24, 94, 95)]
    for case in range(32):
        c.reset()
        c.write(2, 0x84, 0x80)
        c.write(1, 0x60, rng.randrange(1, 8) << 4 | (8 if case & 1 else 0) | rng.randrange(1, 8))
        c.write(2, 0x62, 0xA080 | rng.randrange(64))
        c.write(2, 0x64, 0x8000 | rng.randrange(256, 1792))
        for step in range(48):
            c.advance(32768)
            c.compare_psg(f"sweep case {case} step {step}", fields)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path, default=Path("build/Release/hn_reference_probe.dll"))
    parser.add_argument("--oracle", required=True, type=Path)
    parser.add_argument("--oracle-runtime-dir", type=Path)
    args = parser.parse_args()
    c = Comparison(args.native.resolve(), args.oracle.resolve(), args.oracle_runtime_dir)
    try:
        rng = random.Random(0xA0A4100)
        timers(c, rng)
        fifos(c, rng)
        dma(c, rng)
        psg(c, rng)
        waves(c, rng)
        sweeps(c, rng)
        print(f"{c.checks} timer/PSG/FIFO/DMA/PWM checks matched across {c.snapshots} synthetic snapshots.")
        print("Component-level hardware evidence; game output, analog filtering and startup quirks remain unverified.")
    finally:
        c.close()

if __name__ == "__main__":
    main()
