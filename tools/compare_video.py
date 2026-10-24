"""Compare ROM-free video fixtures with an optional external mGBA PPU.

No captures or game assets are written. Build the separate test oracle as
documented in docs/testing.md; it is never a dependency of the Windows app.
The default protocol is static; scanline callers supply six registers per row.
"""
from __future__ import annotations
import argparse
import ctypes as C
import os
from pathlib import Path
import random
import struct
import subprocess
ROOT = Path(__file__).resolve().parents[1]
PIXELS = 240 * 160
def word(data, offset, value):
    struct.pack_into("<H", data, offset, value & 0xFFFF)
class VideoOracle:
    def __init__(self, executable: Path, runtime_dir: Path | None = None, *, scanlines=False):
        environment = os.environ.copy()
        if runtime_dir:
            environment["PATH"] = str(runtime_dir.resolve()) + os.pathsep + environment.get("PATH", "")
        self.scanlines=scanlines
        self.process = subprocess.Popen([str(executable.resolve())]+(["--scanlines"] if scanlines else []), stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, env=environment)
    def __enter__(self):
        return self
    def render(self, parts, line_registers=None) -> bytes:
        if tuple(map(len, parts)) != (1024, 1024, 98304, 1024):
            raise ValueError("A PPU snapshot requires IO, palette, VRAM and OAM.")
        if self.scanlines:
            if not isinstance(line_registers,bytes) or len(line_registers)!=160*12:
                raise ValueError("Scanline PPU input requires six halfword registers per visible row.")
        elif line_registers is not None:raise ValueError("The static PPU protocol has no per-line registers.")
        self.process.stdin.write(b"".join(parts)+(line_registers or b""))
        self.process.stdin.flush()
        reference = bytearray()
        while len(reference) < PIXELS * 4:
            block = self.process.stdout.read(PIXELS * 4 - len(reference))
            if not block:
                raise RuntimeError("PPU oracle terminated while rendering a snapshot.")
            reference += block
        return bytes(reference)
    def __exit__(self, error_type, error, traceback):
        try:
            self.process.stdin.close()
            self.process.wait(timeout=10)
        finally:
            if self.process.poll() is None:
                self.process.kill()
                self.process.wait()
            self.process.stdout.close()
        if not error_type and self.process.returncode:
            raise RuntimeError(f"PPU oracle failed: {self.process.returncode}.")
def fixture(index, rng):
    io, palette, vram, oam = bytearray(1024), bytearray(1024), bytearray(98304), bytearray(1024)
    for i in range(128):
        word(oam, i * 8, 0x200)
    for i in range(512):
        word(palette, i * 2, rng.randrange(32768))
    if index == 0:
        word(io, 0, 0x80)
        return io, palette, vram, oam
    if index == 1:
        return io, palette, vram, oam
    mode = (index - 2) % 6
    # Window clipping through a mosaic block has known carry-pixel artifacts in
    # mGBA 0.10.5. Test mosaic and windows independently here; native unit tests
    # retain the combined global mosaic phase contract.
    mosaic = bool(index & 4) and index < 16
    display = mode | (0x40 if index & 1 else 0)
    layers = (0, 1, 2, 3) if mode == 0 else (0, 1, 2) if mode == 1 else (2, 3) if mode == 2 else (2,)
    for layer in layers:
        display |= 0x100 << layer
    # Synthetic tiles, maps and bitmap words, with no extracted resource data.
    vram[:] = rng.randbytes(len(vram))
    for layer in range(4):
        control = rng.randrange(4) | rng.randrange(4) << 2 | (16 + layer) << 8
        if index & 2:
            control |= 0x80
        if mosaic:
            control |= 0x40
        if layer >= 2 and mode in (1, 2):
            control |= rng.randrange(4) << 14 | 0x2000
        word(io, 8 + layer * 2, control)
        word(io, 0x10 + layer * 4, rng.randrange(512))
        word(io, 0x12 + layer * 4, rng.randrange(512))
        # Text map tile selectors stay inside the selected character block.
        if mode == 0 or mode == 1 and layer < 2:
            for entry in range(1024):
                word(vram, (16 + layer) * 2048 + entry * 2, rng.randrange(128) | rng.randrange(16) << 12 | rng.randrange(4) << 10)
    for offset in (0x20, 0x30):
        scale = 128 if mode == 5 and offset == 0x20 else 256
        word(io, offset, scale)
        word(io, offset + 6, scale)
    if mode >= 4 and index & 8:
        display |= 16
    display |= 0x1000
    for obj in range(4):
        offset = obj * 8
        affine = index & 8 and obj & 1
        attr0 = rng.randrange(160) | rng.randrange(3) << 14
        attr1 = rng.randrange(240) | rng.randrange(2) << 14
        if affine:
            attr0 |= 0x100
            attr1 |= obj << 9
            matrix = obj * 32
            for delta, value in ((6, 256), (14, 0), (22, 0), (30, 256)):
                word(oam, matrix + delta, value)
        else:
            attr1 |= rng.randrange(4) << 12
        if index & 2:
            attr0 |= 0x2000
        if mosaic:
            attr0 |= 0x1000
        word(oam, offset, attr0)
        word(oam, offset + 2, attr1)
        word(oam, offset + 4, rng.randrange(256) | rng.randrange(4) << 10 | rng.randrange(16) << 12)
    word(io, 0x4C, rng.randrange(65536) if mosaic else 0)
    if index >= 16:
        display |= 0x6000
        word(io, 0x40, rng.randrange(65536))
        word(io, 0x42, rng.randrange(65536))
        word(io, 0x44, rng.randrange(65536))
        word(io, 0x46, rng.randrange(65536))
        word(io, 0x48, rng.randrange(0x4000))
        word(io, 0x4A, rng.randrange(0x4000))
        word(io, 0x50, rng.randrange(0x4000))
        word(io, 0x52, rng.randrange(17) | rng.randrange(17) << 8)
        word(io, 0x54, rng.randrange(17))
    word(io, 0, display)
    return io, palette, vram, oam
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--oracle", required=True, type=Path)
    parser.add_argument("--runtime-dir", type=Path, help="Optional directory containing the oracle's runtime DLLs")
    parser.add_argument("--library", type=Path, default=ROOT / "build/Release/hn_reference_probe.dll")
    parser.add_argument("--cases", type=int, default=64)
    args = parser.parse_args()
    if not 2 <= args.cases <= 10000:
        parser.error("--cases must be between 2 and 10000")
    library = C.CDLL(str(args.library.resolve()))
    def bind(name, result, *arguments):
        fn = getattr(library, name)
        fn.restype, fn.argtypes = result, list(arguments)
        return fn
    create = bind("hn_probe_create", C.c_void_p, C.c_void_p, C.c_size_t)
    destroy = bind("hn_probe_destroy", None, C.c_void_p)
    seed = bind("hn_probe_seed", C.c_int, C.c_void_p, C.c_uint32, C.c_void_p, C.c_size_t)
    render = bind("hn_video_render", None, C.c_void_p, C.c_void_p)
    memory = create(None, 0)
    if not memory:
        raise MemoryError("Native memory allocation failed.")
    rng = random.Random(0x505055)
    output = (C.c_uint32 * PIXELS)()
    try:
        with VideoOracle(args.oracle, args.runtime_dir) as oracle:
            for index in range(args.cases):
                parts = fixture(index, rng)
                for address, data in zip((0x04000000, 0x05000000, 0x06000000, 0x07000000), parts):
                    payload = bytes(data)
                    if not seed(memory, address, payload, len(payload)):
                        raise RuntimeError("Native fixture injection failed.")
                render(memory, output)
                reference = oracle.render(parts)
                native = bytes(output)
                if native != reference:
                    pixel = next(i for i in range(PIXELS) if native[i*4:i*4+4] != reference[i*4:i*4+4])
                    actual, expected = struct.unpack_from("<I", native, pixel*4)[0], struct.unpack_from("<I", reference, pixel*4)[0]
                    raise RuntimeError(f"PPU mismatch in synthetic case {index} at ({pixel%240}, {pixel//240}): {actual:08X} != {expected:08X}.")
    finally:
        destroy(memory)
    print(f"{args.cases} static PPU frames matched mGBA ({args.cases * PIXELS:,} pixels).")
    print("Synthetic static snapshots only; game-scene and raster timing parity remain pending.")
if __name__ == "__main__":
    main()
