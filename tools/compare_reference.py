"""Private function-level checks against a player-supplied AH3P ROM.

Requires Unicorn in the calling Python environment. No ROM, save or captured
state is written by this tool; the reference checkout is read-only.
"""
from __future__ import annotations
import argparse
import ctypes as C
import hashlib
import json
import math
from pathlib import Path
import random
import struct
from contextlib import nullcontext
from unicorn import (Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_INTR,
                     UC_HOOK_MEM_WRITE, UC_HOOK_CODE)
from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2,
                              UC_ARM_REG_R3, UC_ARM_REG_SP, UC_ARM_REG_LR,
                              UC_ARM_REG_PC)

ROOT = Path(__file__).resolve().parents[1]
STOP = 0x01000000

class Keys(C.Structure):
    _fields_ = [(name, C.c_uint16) for name in
                ("held", "pressed", "repeat", "released", "previous",
                 "held_frames", "first_delay", "next_delay", "counter")]

class PRNG(C.Structure):
    _fields_ = [("seed", C.c_uint32 * 3)]

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--rom", required=True, type=Path)
    parser.add_argument("--reference", required=True, type=Path,
                        help="Read-only decompilation checkout containing build/function-symbols.json")
    parser.add_argument("--library", type=Path, default=ROOT / "build/Release/hn_reference_probe.dll")
    parser.add_argument("--video-oracle", type=Path, help="Optional separate RGB555 mGBA PPU oracle")
    parser.add_argument("--oracle-runtime-dir", type=Path, help="Optional runtime DLL directory for the PPU oracle")
    args = parser.parse_args()
    spec = json.loads((ROOT / "config/rom.json").read_text())
    rom = args.rom.read_bytes()
    if len(rom) != spec["size"] or hashlib.sha256(rom).hexdigest() != spec["sha256"]:
        raise SystemExit("A matching European AH3P revision 0 ROM is required.")
    entries = json.loads((args.reference / "build/function-symbols.json").read_text())
    symbols = {row["name"]: row["address"] for row in entries}
    cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    for address, size in ((0x02000000, 0x40000), (0x03000000, 0x8000),
                          (0x04000000, 0x1000), (0x05000000, 0x1000),
                          (0x06000000, 0x20000), (0x07000000, 0x1000),
                          (0x08000000, len(rom)), (0x0E000000, 0x10000), (STOP, 0x1000)):
        cpu.mem_map(address, size)
    cpu.mem_write(0x08000000, rom)
    cpu.reg_write(UC_ARM_REG_SP, 0x03007C00)

    def bios(uc: Uc, number: int, _context) -> None:
        pc = uc.reg_read(UC_ARM_REG_PC)
        instruction = bytes(uc.mem_read(pc - 2, 2))
        if number != 2 or instruction[1] != 0xDF:
            raise RuntimeError("Unexpected BIOS service in the selected function checks.")
        service = instruction[0]
        if service == 0x0F:
            source, destination, count, stride = (uc.reg_read(r) for r in
                (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3))
            def f32(value):
                return struct.unpack("<f", struct.pack("<f", value))[0]
            for _ in range(count):
                sx, sy, angle = struct.unpack("<hhH", bytes(uc.mem_read(source, 6)))
                theta = f32((angle >> 8) / 128 * math.pi)
                sine, cosine = f32(math.sin(theta)), f32(math.cos(theta))
                values = (f32(cosine * f32(sx / 256)), f32(-sine * f32(sx / 256)),
                          f32(sine * f32(sy / 256)), f32(cosine * f32(sy / 256)))
                for i, value in enumerate(values):
                    uc.mem_write(destination + i * stride, struct.pack("<H", int(f32(value * 256)) & 0xFFFF))
                source += 8
                destination += stride * 4
            return
        if service in (0x11, 0x12):
            source, destination = (uc.reg_read(r) for r in (UC_ARM_REG_R0, UC_ARM_REG_R1))
            header = struct.unpack("<I", bytes(uc.mem_read(source, 4)))[0]
            source += 4
            if header & 255 != 0x10 or header >> 8 > 0x40000:
                raise RuntimeError("Invalid private LZ77 fixture.")
            output = bytearray()
            while len(output) < header >> 8:
                flags = uc.mem_read(source, 1)[0]
                source += 1
                for bit in range(8):
                    if len(output) == header >> 8:
                        break
                    if flags & (128 >> bit):
                        hi, lo = uc.mem_read(source, 2)
                        source += 2
                        distance = ((hi & 15) << 8) + lo + 1
                        for _ in range((hi >> 4) + 3):
                            output.append(output[-distance])
                    else:
                        output += uc.mem_read(source, 1)
                        source += 1
            uc.mem_write(destination, bytes(output))
            return
        if service in (0x0B, 0x0C):
            source, destination, control = (uc.reg_read(r) for r in
                (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2))
            width = 4 if service == 0x0C or control & 0x04000000 else 2
            count = control & 0x1FFFFF
            if service == 0x0C:
                count = (count + 7) & ~7
            source &= ~(width - 1)
            destination &= ~(width - 1)
            for i in range(count):
                value = bytes(uc.mem_read(source + (0 if control & 0x01000000 else i * width), width))
                uc.mem_write(destination + i * width, value)
            return
        if service != 6:
            raise RuntimeError(f"Unmodeled BIOS service {service:02X} in private function checks.")
        def signed(value: int) -> int:
            return value - 0x100000000 if value & 0x80000000 else value
        numerator, denominator = (signed(uc.reg_read(r)) for r in (UC_ARM_REG_R0, UC_ARM_REG_R1))
        if not denominator:
            raise RuntimeError("This private probe does not model BIOS division by zero.")
        quotient = abs(numerator) // abs(denominator)
        if (numerator < 0) != (denominator < 0):
            quotient = -quotient
        uc.reg_write(UC_ARM_REG_R0, quotient & 0xFFFFFFFF)
        uc.reg_write(UC_ARM_REG_R1, (numerator - quotient * denominator) & 0xFFFFFFFF)
        uc.reg_write(UC_ARM_REG_R3, abs(quotient))
    cpu.hook_add(UC_HOOK_INTR, bios)

    # Unicorn supplies the CPU only. These selected functions also need immediate
    # DMA and read-only IO masks. Deferred DMA, scanline and audio timing are not
    # represented by this function-level oracle.
    pending_io = []
    def io_write(uc, _access, address, size, value, _context):
        pending_io.append((address, size, value, bytes(uc.mem_read(address, size))))

    def finish_io(uc, _address=0, _size=0, _context=None):
        while pending_io:
            address, size, value, old = pending_io.pop(0)
            for i in range(size):
                a, byte = address + i, (value >> (8 * i)) & 255
                if a == 0x04000000:
                    byte &= 0xF7
                elif a == 0x04000004:
                    byte = (byte & 0xF8) | (old[i] & 7)
                elif a in (0x04000006, 0x04000007):
                    byte = old[i]
                elif a in (0x04000202, 0x04000203):
                    byte = old[i] & ~byte
                uc.mem_write(a, bytes([byte]))
            for channel in range(4):
                base = 0x040000B0 + channel * 12
                if not address <= base + 11 < address + size:
                    continue
                source, destination, control = struct.unpack("<III", bytes(uc.mem_read(base, 12)))
                if not control & 0x80000000 or control & 0x30000000:
                    continue
                width = 4 if control & 0x04000000 else 2
                source &= ~(width - 1)
                destination &= ~(width - 1)
                count = control & (0xFFFF if channel == 3 else 0x3FFF)
                count = count or (65536 if channel == 3 else 16384)
                source_mode, dest_mode = (control >> 23) & 3, (control >> 21) & 3
                for _ in range(count):
                    uc.mem_write(destination, bytes(uc.mem_read(source, width)))
                    if source_mode < 2:
                        source += -width if source_mode == 1 else width
                    if dest_mode != 2:
                        destination += -width if dest_mode == 1 else width
                uc.mem_write(base + 8, struct.pack("<I", control & 0x7FFFFFFF))
    cpu.hook_add(UC_HOOK_MEM_WRITE, io_write, begin=0x04000000, end=0x040003FF)
    cpu.hook_add(UC_HOOK_CODE, finish_io)

    def call(name: str, *arguments: int, limit=200000) -> int:
        for register, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), arguments):
            cpu.reg_write(register, value)
        if len(arguments) > 4:
            cpu.mem_write(cpu.reg_read(UC_ARM_REG_SP), struct.pack(f"<{len(arguments)-4}I", *arguments[4:]))
        cpu.reg_write(UC_ARM_REG_LR, STOP | 1)
        cpu.emu_start(symbols[name] | 1, STOP, count=limit)
        finish_io(cpu)
        if cpu.reg_read(UC_ARM_REG_PC) != STOP:
            raise RuntimeError(f"Reference function did not return: {name}")
        return cpu.reg_read(UC_ARM_REG_R0)

    library = C.CDLL(str(args.library.resolve()))
    def bind(name, result, *arguments):
        fn = getattr(library, name)
        fn.restype, fn.argtypes = result, list(arguments)
        return fn
    create = bind("hn_probe_create", C.c_void_p, C.c_void_p, C.c_size_t)
    destroy = bind("hn_probe_destroy", None, C.c_void_p)
    read = bind("hn_probe_read", C.c_int, C.c_void_p, C.c_uint32, C.c_void_p, C.c_size_t)
    heap_init = bind("hn_heap_init", None, C.c_void_p)
    allocate = bind("hn_heap_allocate", C.c_uint32, C.c_void_p, C.c_uint32, C.c_bool)
    free = bind("hn_heap_free", C.c_uint32, C.c_void_p, C.c_uint32)
    collect = bind("hn_heap_coalesce", None, C.c_void_p)
    random_init = bind("hn_random_init", None, C.POINTER(PRNG), C.c_uint8)
    random_next = bind("hn_random_next", C.c_uint32, C.POINTER(PRNG))
    keys_init = bind("hn_keys_init", None, C.POINTER(Keys))
    keys_step = bind("hn_keys_step", None, C.POINTER(Keys), C.c_uint16)
    command = bind("hn_probe_command", C.c_int, C.c_void_p, C.c_uint, C.c_uint32)
    seed = bind("hn_probe_seed", C.c_int, C.c_void_p, C.c_uint32, C.c_void_p, C.c_size_t)
    reset_memory = bind("hn_memory_init", None, C.c_void_p, C.c_void_p, C.c_size_t)
    buffer = C.create_string_buffer(rom)
    memory = create(buffer, len(rom))
    if not memory:
        raise MemoryError("Native memory allocation failed.")
    rng = random.Random(0x483350)
    checks = 0

    def equal(left, right, label: str) -> None:
        nonlocal checks
        checks += 1
        if left != right:
            difference = ""
            if isinstance(left, bytes) and isinstance(right, bytes):
                offset = next((i for i, (a, b) in enumerate(zip(left, right)) if a != b), min(len(left), len(right)))
                difference = f" (first different byte +{offset:X})"
            raise RuntimeError(f"Native/reference mismatch: {label}{difference}")

    def native_bytes(address: int, size: int) -> bytes:
        out = C.create_string_buffer(size)
        if not read(memory, address, out, size):
            raise RuntimeError("Native guest-memory read failed.")
        return out.raw

    try:
        heap_init(memory)
        call("InitializeHeap", 0)
        active = []
        for i in range(180):
            if active and rng.randrange(3) == 0:
                pointer = active.pop(rng.randrange(len(active)))
                equal(free(memory, pointer), call("FreeHeap", pointer), f"free {i}")
            else:
                size = rng.randrange(1, 513)
                pointer = call("AllocateHeapRaw", size)
                equal(allocate(memory, size, False), pointer, f"allocate {i}")
                active.append(pointer)
            if i % 17 == 0:
                collect(memory)
                call("CoalesceHeap")
            equal(native_bytes(0x02022000, 0x1E000), bytes(cpu.mem_read(0x02022000, 0x1E000)), f"heap RAM {i}")
            for address in (0x030010C8, 0x03002C84, 0x03002E04):
                equal(native_bytes(address, 12 if address == 0x030010C8 else 4),
                      bytes(cpu.mem_read(address, 12 if address == 0x030010C8 else 4)), f"heap root {i}")
        native_random = PRNG()
        random_init(C.byref(native_random), 23)
        cpu.mem_write(0x03000000, struct.pack("<III", 0x3AC2, 0x4DF3, 24))
        for i in range(1000):
            equal(random_next(C.byref(native_random)), call("NextRandom"), f"random result {i}")
            equal(tuple(native_random.seed), struct.unpack("<III", bytes(cpu.mem_read(0x03000000, 12))), f"random state {i}")
        keys = Keys()
        keys_init(C.byref(keys))
        cpu.mem_write(0x03000820, bytes(26))
        call("InitializeKeys")
        sequence = [1] * 180 + [0] * 20 + [0x50] * 100 + [rng.randrange(1024) for _ in range(200)]
        for i, buttons in enumerate(sequence):
            keys_step(C.byref(keys), buttons)
            cpu.mem_write(0x04000130, struct.pack("<H", (~buttons) & 1023))
            call("ReadKeys")
            original = struct.unpack("<12H", bytes(cpu.mem_read(0x03000820, 24)))
            equal((keys.held, keys.pressed, keys.repeat, keys.released,
                   keys.first_delay, keys.next_delay, keys.counter, keys.held_frames),
                  (original[0], original[1], original[2], original[3],
                   original[6], original[7], original[8], original[9]), f"input {i}")
        def both(address: int, payload: bytes) -> None:
            cpu.mem_write(address, payload)
            if not seed(memory, address, payload, len(payload)):
                raise RuntimeError("Private fixture injection failed.")
        operations = (
            (0, "AdvanceScript", b"\x04\x01\x02\x03"),
            (1, "EndScriptFrame", b""), (2, "ScriptWaitFrames", b"\x02\x00"),
            (3, "ScriptWaitInput", b"\x05\x00"),
            (4, "ResumeScriptHandler", b""), (5, "StopScriptHandler", b""),
            (0x0C, "MainScriptSetCursor", struct.pack("<I", 0x02021000)),
            (0x1E, "MainScriptCall", struct.pack("<I", 3)),
            (0x1F, "MainScriptReturn", b""), (0x27, "MainScriptStopAndYield", b""),
            (0x28, "MainScriptReadPointerByte", struct.pack("<I", 3)),
            (0x29, "MainScriptReadControlByte", b"\x03\x00"),
            (0x2A, "MainScriptReadClassHalfword", b"\x03\x00"),
            (0x2B, "MainScriptSetResultHalfword", b"\x56\x12"),
            (0x2D, "MainScriptWriteControlByte", b"\x03\x00\x71"),
            (0x2E, "MainScriptWriteClassHalfword", b"\x03\x00\x56\x12"),
            (0x53, "MainScriptWriteVariableWord", struct.pack("<HI", 3, 0x12345678)),
        )
        for repeat in range(4):
            for opcode, name, payload in operations:
                both(0x03000010, bytes(16)); both(0x030005E8, bytes(12))
                both(0x03002BB0, bytes(0xB4)); both(0x03001FBC, b"\0\0")
                both(0x02020000, payload + bytes(64 - len(payload)))
                both(0x02021000, bytes(range(64)))
                both(0x02004520, bytes(range(128))); both(0x0201DC40, bytes(range(128)))
                both(0x03000010, bytes([opcode])); both(0x03000018, bytes([repeat & 1]))
                both(0x0300001A, struct.pack("<H", repeat))
                both(0x03000822, struct.pack("<H", repeat & 1))
                both(0x03002C60, struct.pack("<I", 0x02020000))
                both(0x03002BF0, b"\x01")
                both(0x03002BBC, struct.pack("<I", 0x02021000))
                both(0x03002C10, struct.pack("<I", 0x02021004))
                equal(command(memory, opcode, 0x02020000), 1, f"native command {name}")
                call(name, 0x02020000)
                for address, length in ((0x03000010, 16), (0x030005E8, 12), (0x03001FBC, 2),
                                        (0x03002BB0, 0xB4), (0x02004520, 128), (0x0201DC40, 128)):
                    equal(native_bytes(address, length), bytes(cpu.mem_read(address, length)), f"script {name} RAM {address:08X}")

        def fresh() -> None:
            reset_memory(memory, buffer, len(rom))
            for address, size in ((0x02000000, 0x40000), (0x03000000, 0x8000),
                                  (0x04000000, 0x400), (0x05000000, 0x400),
                                  (0x06000000, 0x18000), (0x07000000, 0x400)):
                cpu.mem_write(address, bytes(size))
            cpu.mem_write(0x04000130, b"\xFF\x03")
            cpu.mem_write(0x0E000000, b"\xFF" * 0x8000)
            cpu.reg_write(UC_ARM_REG_SP, 0x03007C00)

        def compare(label: str, ranges) -> None:
            for address, size in ranges:
                equal(native_bytes(address, size), bytes(cpu.mem_read(address, size)), f"{label} RAM {address:08X}")

        def native(name: str, arguments=0, result=None):
            return bind(name, result, C.c_void_p, *([C.c_uint32] * arguments))

        resource_cases = (
            (0x0F, "MainScriptConfigureRenderSlot", struct.pack("<BBBHHHI", 3, 1, 0x30, 64, 80, 16, 4)),
            (0x10, "MainScriptSetRenderAnimation", struct.pack("<BI", 3, 4)),
            (0x11, "MainScriptSetRenderField60AndAnimation", struct.pack("<BBI", 3, 1, 4)),
            (6, "MainScriptDecompressStaged", struct.pack("<II", 0x02022000, 0x06000000)),
            (6, "MainScriptDecompressStaged", struct.pack("<II", 0x02022000, 0x05000020)),
            (6, "MainScriptDecompressStaged", struct.pack("<II", 0x02022000, 0x02024000)),
            (7, "MainScriptDecompressPointer", struct.pack("<II", 0x02022000, 3)),
            (7, "MainScriptDecompressPointer", struct.pack("<II", 0x02022000, 0x06000020)),
            (8, "MainScriptCopyDma", struct.pack("<IIH", 4, 3, 32)),
            (8, "MainScriptCopyDma", struct.pack("<IIH", 0x02022002, 0x05000020, 14)),
            (9, "MainScriptCopyDmaQueued", struct.pack("<IIH", 4, 0x06000020, 32)),
            (0x12, "MainScriptSetBackgroundScript", struct.pack("<BBI", 1, 5, 4)),
            (0x12, "MainScriptSetBackgroundScript", struct.pack("<BBI", 1, 0x85, 0)),
            (0x13, "MainScriptSetGraphicsScript", struct.pack("<BBI", 1, 2, 4)),
            (0x13, "MainScriptSetGraphicsScript", struct.pack("<BBI", 1, 1, 0)),
            (0x14, "MainScriptConfigureVideoBackground", bytes([2, 1, 2, 1, 0, 25, 1, 3])),
            (0x14, "MainScriptConfigureVideoBackground", bytes([2, 255, 255, 255, 255, 255, 255, 255])),
            (0x15, "MainScriptSetVideoBackgroundOffset", struct.pack("<BHH", 3, 511, 479)),
            (0x16, "MainScriptEnableDisplayLayers", b"\x13"),
            (0x17, "MainScriptDisableDisplayLayers", b"\x05"),
            (0x18, "MainScriptFillMemory", struct.pack("<BHHI", 0, 0xABCD, 32, 3)),
            (0x18, "MainScriptFillMemory", struct.pack("<BHHI", 1, 0xABCD, 32, 0x06004000)),
            (0x18, "MainScriptFillMemory", struct.pack("<BHHI", 1, 0xABCD, 14, 0x05000020)),
            (0x1C, "MainScriptInvokeCallback", struct.pack("<IH", 0x0801C709, 0xF5A5)),
            (0x1C, "MainScriptInvokeCallback", struct.pack("<I", 0x0801C72D)),
            (0x1D, "MainScriptJumpByHalf1FBC", struct.pack("<BIIII", 4, 0x02021000, 0x02022000, 0x02023000, 0x02024000)),
            (0x22, "MainScriptConditionalCursor084C0", struct.pack("<II", 0x02023000, 3)),
            (0x23, "MainScriptInvoke086AC", struct.pack("<II", 0x02023000, 0x02024000)),
            (0x2C, "MainScriptSetBackgroundBounds", struct.pack("<BBhhhh", 3, 15, -32, -16, 512, 256)),
            (0x2F, "MainScriptCopyValuesByWidth", struct.pack("<BHHH", 0, 3, 8, 16)),
            (0x2F, "MainScriptCopyValuesByWidth", struct.pack("<BHHH", 1, 3, 8, 16)),
            (0x30, "MainScriptJumpValuesEqual", struct.pack("<BHHI", 0, 3, 0x7F, 3)),
            (0x30, "MainScriptJumpValuesEqual", struct.pack("<BHHI", 1, 3, 0x7F, 0x02024000)),
            (0x31, "MainScriptJumpValuesAtLeast", struct.pack("<BHHI", 0, 3, 0x7F, 3)),
            (0x31, "MainScriptJumpValuesAtLeast", struct.pack("<BHHI", 1, 3, 0x7F, 3)),
            (0x32, "MainScriptJumpValuesBelow", struct.pack("<BHHI", 0, 3, 0x7F, 3)),
            (0x32, "MainScriptJumpValuesBelow", struct.pack("<BHHI", 1, 3, 0x7F, 3)),
            (0x33, "MainScriptJumpGameFlag", struct.pack("<BHI", 0, 140, 3)),
            (0x33, "MainScriptJumpGameFlag", struct.pack("<BHI", 1, 141, 3)),
            (0x39, "MainScriptSetRenderPriorityBits", bytes([3, 2])),
            (0x3A, "MainScriptSetRenderPosition", struct.pack("<BHHH", 3, 32768, 65535, 0)),
            (0x3B, "MainScriptSetRenderField60", bytes([3, 255])),
            (0x3C, "MainScriptJumpRenderAnimationBase", struct.pack("<BII", 3, 4, 3)),
            (0x3E, "MainScriptSetRenderField62", bytes([3, 1, 2, 5])),
            (0x3E, "MainScriptSetRenderField62", bytes([3, 2, 1, 5])),
            (0x44, "MainScriptSetRenderVector24", struct.pack("<BHHH", 3, 32768, 65535, 32767)),
            (0x46, "MainScriptSetGameFlag", struct.pack("<HB", 141, 1)),
            (0x46, "MainScriptSetGameFlag", struct.pack("<HB", 140, 0)),
            (0x47, "MainScriptConfigureSquareMosaic", b"\x1F\x0A"),
            (0x48, "MainScriptConfigureSharedMosaic", b"\x1F\x0A\x0B"),
            (0x49, "MainScriptSetSaveGroup01Bytes", b"\x12\x34"),
            (0x4A, "MainScriptRepositionRenderOrder", bytes([3, 10, 0])),
            (0x4A, "MainScriptRepositionRenderOrder", bytes([10, 3, 1])),
            (0x4B, "MainScriptSetRenderModeBits", bytes([3, 255])),
            (0x4C, "MainScriptSetVideoMode", b"\x04"),
            (0x4C, "MainScriptSetVideoMode", b"\x07"),
            (0x4D, "MainScriptStoreDestinationWord21F0", struct.pack("<BI", 3, 4)),
            (0x4F, "MainScriptSetRenderBit5", bytes([3, 255])),
            (0x52, "MainScriptSetForcedBlankAndYield", b"\x00"),
            (0x52, "MainScriptSetForcedBlankAndYield", b"\x01"),
            (0x55, "MainScriptSetBootGateAndByte1FB8", b"\x0A"),
            (0x59, "MainScriptSetRenderClasses", b"\x03\x04\x05\x06"),
            (0x5D, "MainScriptChangeSaveByte21E0Bits", b"\x0A\x00"),
            (0x5D, "MainScriptChangeSaveByte21E0Bits", b"\x0A\x01"),
        )
        for repeat in range(4):
            for opcode, name, payload in resource_cases:
                fresh()
                both(0x02020000, payload + bytes(64 - len(payload)))
                # Synthetic compressed literals; never a ROM resource fixture.
                both(0x02022000, b"\x10\x10\0\0\0ABCDEFGH\0IJKLMNOP" + bytes(64))
                both(0x02023000, struct.pack("<HH", 0x7405, 0xBE2A))
                both(0x03002BB0, bytes(64))
                both(0x03002BBC, struct.pack("<II", 0x02024000, 0x02022000))
                both(0x03000010, bytes([opcode]))
                both(0x03001FBC, struct.pack("<H", repeat))
                both(0x03000820, struct.pack("<H", repeat * 0x123 & 1023))
                both(0x03001F64, b"\xFF\x03")
                both(0x03002C60, struct.pack("<I", 0x02020000))
                both(0x03002E08, struct.pack("<I", 0x02018000))
                both(0x03003070, struct.pack("<I", 0x02019000))
                both(0x03002E40, rng.randbytes(176))
                both(0x03002C90, rng.randbytes(0x108))
                both(0x03002D98, struct.pack("<I", 0x02017000))
                both(0x03002D9C, struct.pack("<I", 0x02016800))
                both(0x03002E2C, struct.pack("<I", 0x02016000))
                both(0x03002DC0, b"\x10")
                both(0x03001F50, b"\x03")
                both(0x02016800, bytes(range(16)))
                both(0x03002180, rng.randbytes(96))
                both(0x02004520, rng.randbytes(128))
                both(0x0201DC40, rng.randbytes(128))
                both(0x02004523, bytes([0x7E + repeat]))
                both(0x0201DC46, struct.pack("<H", 0x7E + repeat))
                both(0x04000000, struct.pack("<H", 0x80 if repeat & 1 else 0))
                equal(command(memory, opcode, 0x02020000), 1, f"native resources {name}")
                call(name, 0x02020000)
                compare(f"resources {name} {repeat}", ((0x02000000, 0x40000), (0x03000000, 0x7000),
                        (0x05000000, 0x400), (0x06000000, 0x18000), (0x04000000, 0x58)))

        for opcode, original in ((0x3F, "MainScriptFadePhaseToCenter"), (0x40, "MainScriptFadePhaseByStep")):
            fresh()
            for phase in range(256):
                for step in (0, 1, 127, 128, 255):
                    both(0x03000010, bytes([opcode]) + bytes(14))
                    both(0x03002C90, rng.randbytes(0x108))
                    both(0x03002D96, bytes([phase]))
                    both(0x02020000, bytes([step]))
                    both(0x04000000, b"\x80\0")
                    equal(command(memory, opcode, 0x02020000), 1, f"native fade {opcode} {phase} {step}")
                    call(original, 0x02020000)
                    compare(f"fade {opcode} {phase} {step}", ((0x03000010, 16), (0x03002C90, 0x108), (0x04000000, 2)))

        evaluate = bind("hn_expression_evaluate", C.c_bool, C.c_void_p, C.c_uint32, C.c_bool, C.POINTER(C.c_uint32))
        def expression(update: bool, depth=0) -> list[int]:
            tag = rng.choice((0x2000, 0x4000, 0x6000, 0x8000, 0xA000, 0xC000, 0xE000))
            selector = rng.randrange(512)
            tail = [rng.randrange(65536)] if tag == 0xC000 else []
            operations = (0x1A00, 0x1C00, 0x1E00, 0x0A00, 0x0C00, 0x0400, 0x0600, 0x0800)
            operations += (0x1400, 0x1600, 0x1800) if update else (0x1000, 0x1200, 0x0E00)
            operation = rng.choice(operations) if depth < 4 else 0x1E00
            words = [tag | operation | selector] + tail
            if operation in (0x0A00, 0x0C00, 0x0400, 0x0600, 0x0800, 0x1400, 0x1600, 0x1800,
                             0x1000, 0x1200, 0x0E00):
                words += expression(update, depth + 1)
            if depth < 2 and rng.randrange(5) == 0:
                # A grouped expression reads its following operation from the
                # shared cursor, including short-circuit cursor side effects.
                words = [0xA000] + words + [0x1E00]
            return words

        for update in (False, True):
            fresh()
            for i in range(600):
                both(0x02004520, rng.randbytes(512))
                both(0x0201DC40, rng.randbytes(1024))
                both(0x03002180, rng.randbytes(64))
                words = expression(update)
                both(0x02023000, struct.pack(f"<{len(words)}H", *words) + b"\0" * 512)
                result = C.c_uint32()
                equal(evaluate(memory, 0x02023000, update, C.byref(result)), True, f"expression success {update} {i}")
                original = "UpdateWordExpression" if update else "EvaluateWordExpression"
                equal(result.value, call(original, 0x02023000), f"expression value {update} {i}")
                compare(f"expression state {update} {i}", ((0x03000798, 8),
                    (0x02004520, 512), (0x0201DC40, 1024), (0x03002180, 64)))

        video_ranges = ((0x03002C90, 0x108), (0x03002DD4, 4), (0x04000000, 0x58))
        video_cases = (
            ("hn_video_state_init", "InitializeVideoState", ()),
            ("hn_video_set_mode", "SetVideoMode", (0xFF,)),
            ("hn_video_set_layers", "SetDisplayLayers", (0, 0xA5)),
            ("hn_video_set_layers", "SetDisplayLayers", (1, 0xA5)),
            ("hn_video_forced_blank", "SetForcedBlank", (1,)),
            ("hn_video_forced_blank", "SetForcedBlank", (0,)),
            ("hn_video_mark_backgrounds", "MarkBackgroundsDirty", (0xA5,)),
            ("hn_video_blend_effect", "SetBlendEffect", (0xFF,)),
            ("hn_video_blend_targets", "SetBlendTargets", (0xFF, 0xA5)),
            ("hn_video_blend_targets", "SetBlendTargets", (0xA5, 0xFF)),
            ("hn_video_blend_coefficients", "SetBlendCoefficients", (0xA5, 0x61, 0x7F)),
            ("hn_mosaic_init", "InitializeMosaic", ()),
            ("hn_mosaic_update", "UpdateMosaic", ()),
            ("hn_video_commit", "CommitVideoRegisters", ()),
        )
        for repeat in range(24):
            for function, original, values in video_cases:
                fresh()
                both(0x03002C90, rng.randbytes(0x108))
                both(0x03002DD4, rng.randbytes(4))
                both(0x04000000, rng.randbytes(0x58))
                native(function, len(values))(memory, *values)
                call(original, *values)
                compare(f"{original} {repeat}", video_ranges)
        for function, original in (("hn_video_blend_scale", "SetBlendScale"),
                                   ("hn_video_fade_phase", "SetFadePhase")):
            for value in range(256):
                native(function, 1)(memory, value)
                call(original, value)
                compare(f"{original} {value}", video_ranges)

        transfer_reset = native("hn_transfer_reset")
        queue = native("hn_transfer_queue", 3)
        queue_fill = native("hn_transfer_queue_fill", 3)
        flush = native("hn_transfer_flush")
        fresh()
        both(0x03000840, rng.randbytes(0x884))
        transfer_reset(memory)
        call("ResetTransferBuffers")
        transfer_ranges = ((0x03000840, 0x884), (0x03002DA0, 1), (0x03002E34, 1),
                           (0x03002E20, 1), (0x030034A0, 1))
        compare("transfer reset", transfer_ranges)
        both(0x02020000, rng.randbytes(512))
        for i in range(120):
            # Exercise ring wrap, zero-size queue requests, halfword/word transfers,
            # delayed flushing, fill-value wrap and palette/VRAM destinations.
            destination = (0x05000000, 0x06004000, 0x02021000)[i % 3] + (i % 8) * 32
            size = (0, 2, 4, 6, 16, 32)[i % 6]
            if i & 1:
                value = rng.getrandbits(32)
                queue_fill(memory, value, destination, size)
                call("QueueDmaFill", value, destination, size)
            else:
                source = 0x02020000 + (i % 8) * 2
                queue(memory, source, destination, size)
                call("QueueDmaTransfer", source, destination, size)
            compare(f"queue {i}", transfer_ranges)
            if i % 7 == 0:
                both(0x04000006, struct.pack("<H", 220 if i % 14 else 160))
                flush(memory)
                call("FlushDmaTransfers")
                compare(f"flush {i}", transfer_ranges + ((0x02021000, 512),
                    (0x05000000, 0x400), (0x06004000, 512)))
        both(0x04000006, b"\xA0\0")
        flush(memory)
        call("FlushDmaTransfers")
        compare("final flush", transfer_ranges + ((0x02021000, 512), (0x05000000, 0x400), (0x06004000, 512)))

        fresh()
        both(0x03000600, rng.randbytes(0x40))
        native("hn_task_contexts_init")(memory)
        call("InitializeTaskContexts")
        compare("task sentinel", ((0x03000600, 0x40), (0x03002BA0, 9), (0x03001F70, 9)))
        for selector in range(4):
            fresh()
            both(0x03002180, rng.randbytes(0x80))
            both(0x03002BB0, rng.randbytes(0x40))
            both(0x02004520, rng.randbytes(0x200))
            both(0x0201DC40, rng.randbytes(0x200))
            native("hn_core_storage_clear", 1)(memory, selector)
            call("ClearCoreScriptStorage", selector)
            compare(f"clear storage {selector}", ((0x03002180, 0x80), (0x03002BB0, 0x40),
                                                    (0x02004520, 0x200), (0x0201DC40, 0x200)))

        fresh()
        heap_init(memory)
        call("InitializeHeap", 0)
        initialize_map = native("hn_map_initialize", 2, C.c_bool)
        for bank in (*range(11), 0, 10, 1):
            equal(initialize_map(memory, bank, 0), True, f"native map {bank}")
            call("InitializeMapDescriptor", bank, 0)
            # The reference's transient stack and DMA pointers to stack locals
            # are not persistent game state and have no native address equivalent.
            compare(f"map {bank}", ((0x02000000, 0x40000), (0x03000000, 0x7000)))
        for mask in range(16):
            both(0x03002E40, rng.randbytes(176))
            native("hn_background_layers_reset", 1)(memory, mask)
            call("ResetBackgroundLayers", mask)
            compare(f"background layer reset {mask}", ((0x03002E40, 176),))

        sram_init = native("hn_sram_init", result=C.c_bool)
        produce = bind("hn_save_produce", C.c_bool, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
        consume = native("hn_save_consume", 2, C.c_bool)
        checksum = native("hn_save_checksum", 2, C.c_uint16)
        def prepare_save() -> None:
            fresh()
            equal(native("hn_runtime_tables_init", result=C.c_bool)(memory), True, "native runtime tables")
            cpu.mem_write(0x02000000, rom[0x8FE018:0x900EC8])
            heap_init(memory)
            call("InitializeHeap", 0)
            equal(sram_init(memory), True, "native SRAM init")
            call("InitializeSram")

        save_ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x0E000000, 0x8000), (0x04000204, 2))
        prepare_save()
        compare("SRAM adapters", save_ranges)
        for opcode, original in ((0x34, "MainScriptDecompressRows"), (0x35, "MainScriptDecompressRowsCpu")):
            for blank in (False, True):
                for rows in (0, 1, 2):
                    prepare_save()
                    both(0x02020000, b"\x10\x10\0\0\0ABCDEFGH\0IJKLMNOP")
                    both(0x02021000, struct.pack("<IIHHH", 0x02020000, 0x06004000, 8, rows, 32))
                    both(0x04000000, struct.pack("<H", 0x80 if blank else 0))
                    equal(command(memory, opcode, 0x02021000), 1, f"native row decompress {opcode} {blank} {rows}")
                    call(original, 0x02021000)
                    compare(f"row decompress {opcode} {blank} {rows}", save_ranges + ((0x06000000, 0x18000),))
        for length in (0, 1, 2, 3, 7, 64, 255, 1024, 32768):
            both(0x02010000, rng.randbytes(length))
            equal(checksum(memory, 0x02010000, length), call("CalculateChecksum", 0x02010000, length), f"checksum {length}")
        for i in range(32):
            length = rng.randrange(256)
            both(0x02010000, rng.randbytes(256))
            native("hn_sram_write", 3, C.c_bool)(memory, 0x02010000, 0x0E000100, length)
            call("WriteSram", 0x02010000, 0x0E000100, length)
            native("hn_sram_read", 3, C.c_bool)(memory, 0x0E000100, 0x02011000, length)
            call("ReadSram", 0x0E000100, 0x02011000, length)
            if i & 1 and length:
                both(0x02010000 + length - 1, b"\xA5")
            equal(native("hn_sram_verify", 3, C.c_uint32)(memory, 0x02010000, 0x0E000100, length),
                  call("VerifySram", 0x02010000, 0x0E000100, length), f"SRAM verify {i}")
            compare(f"SRAM access {i}", save_ranges)
        producer_names = ("ProduceSaveDataGroups0And1", "ProduceSaveDataGroups0And1", "ProduceSaveDataGroup2",
                          "ProduceSaveDataGroup3", "CopyCommunicationRecordForSave")
        consumer_names = ("ConsumeSaveDataGroups0And1", "ConsumeSaveDataGroups0And1", "ConsumeSaveDataGroup2",
                          "ConsumeSaveDataGroup3", "ConsumeCommunicationSaveRecords")
        for group in range(5):
            prepare_save()
            for address, size in ((0x02004520, 0x200), (0x0201CB20, 0x320), (0x0201DC40, 0x200),
                                  (0x0201EE40, 0x20), (0x03001F60, 0x40), (0x03001FC0, 0x1A0),
                                  (0x03002180, 0x70), (0x03002BA0, 0x90), (0x03002C10, 0x50),
                                  (0x02002EB0, 0x270), (0x030035CC, 1), (0x03003730, 0x115), (0x03003C50, 0x1E1)):
                both(address, rng.randbytes(size))
            size = C.c_uint32()
            equal(produce(memory, group, 0x02010000, C.byref(size)), True, f"produce {group}")
            equal(size.value, call(producer_names[group], 0x02010000, 0), f"payload size {group}")
            compare(f"payload produce {group}", save_ranges)
            both(0x02010000, rng.randbytes(0x2BD4))
            both(0x03002BB0, struct.pack("<I", 0x02012000))
            equal(consume(memory, group, 0x02010000), True, f"consume {group}")
            call(consumer_names[group], 0x02010000, 0x2BD4)
            compare(f"payload consume {group}", save_ranges)

        prepare_save()
        native("hn_save_default_fields")(memory)
        call("InitializeSaveDataGroup3Fields")
        compare("default save fields", save_ranges)
        equal(native("hn_save_initialize", result=C.c_bool)(memory), True, "native blank SRAM initialization")
        call("InitializeSaveStorage", limit=6000000)
        compare("blank SRAM initialization", save_ranges)
        initialized_sram = native_bytes(0x0E000000, 0x8000)
        for case in range(4):
            prepare_save()
            both(0x0E000000, initialized_sram)
            if case == 1:
                both(0x0E000000, b"\xFF")
            if case == 2:
                both(0x0E004000, b"\xFF")
            if case == 3:
                both(0x03002BF4, b"\x01")
            equal(native("hn_save_initialize", result=C.c_bool)(memory), True, f"SRAM identity repair {case}")
            call("InitializeSaveStorage", limit=6000000)
            compare(f"SRAM identity repair {case}", save_ranges)
        for group in range(1, 5):
            prepare_save()
            both(0x0E000000, initialized_sram)
            equal(native("hn_save_write_record", 2, C.c_bool)(memory, group, 1 if group == 4 else 0), True,
                  f"native write record {group}")
            call("WriteSaveRecord", group, 1 if group == 4 else 0)
            compare(f"write record {group}", save_ranges)
            for case in range(3):
                both(0x0E000000, initialized_sram)
                offset = (0, 0xEE, 0x6B8, 0xDC2, 0x142C)[group]
                if case:
                    both(0x0E000000 + offset, b"\xFF")
                if case == 2:
                    both(0x0E004000 + offset, b"\xFF")
                both(0x03002BB0, struct.pack("<I", 0x02010000))
                equal(native("hn_save_load_group", 1, C.c_bool)(memory, group), True, f"native load group {group} {case}")
                call("LoadSaveDataGroup", group, limit=1000000)
                compare(f"load group {group} {case}", save_ranges)
        for case in range(4):
            prepare_save()
            both(0x0E000000, initialized_sram)
            if case in (1, 2):
                both(0x0E0006B8, b"\xFF")
            if case == 2:
                both(0x0E004DC2, b"\xFF")
            if case == 3:
                equal(native("hn_save_write_groups", result=C.c_bool)(memory), True, "native write triplet")
                call("WriteSaveDataGroups1To3")
                compare("write triplet", save_ranges)
            equal(native("hn_save_load_groups", result=C.c_bool)(memory), True, f"native load triplet {case}")
            call("LoadSaveDataGroups1To3")
            compare(f"load triplet {case}", save_ranges)
        def prepare_core():
            prepare_save()
            native("hn_video_state_init")(memory)
            call("InitializeVideoState")
            native("hn_mosaic_init")(memory)
            call("InitializeMosaic")
            native("hn_task_contexts_init")(memory)
            call("InitializeTaskContexts")
            equal(native("hn_core_initialize", result=C.c_bool)(memory), True, "native core initialization")
            call("InitializeCoreState", limit=6000000)
            compare("core initialization", save_ranges)
        prepare_core()
        frame = bind("hn_probe_main_frame", C.c_int, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        report = (C.c_uint32 * 3)()
        for index in range(88):
            equal(frame(memory, 4096, report), 1, f"ROM Main-only frame {index}")
            call("RunMainScriptFrame")
            compare(f"ROM Main-only frame {index}", save_ranges + ((0x05000000, 0x400),
                (0x06000000, 0x18000), (0x04000000, 0x58)))
        for opcode, name, payload in ((0x0E, "MainScriptChangeMapBank", b"\x0A"),
                                      (0x0D, "MainScriptChangeMapIndexAndSave", b"\0"),
                                      (0x58, "MainScriptWriteSaveRecord", b"\x02\0"),
                                      (0x5F, "MainScriptWriteSaveDataGroups", b"")):
            both(0x02020000, payload + bytes(16))
            equal(command(memory, opcode, 0x02020000), 1, f"native map/save command {name}")
            call(name, 0x02020000)
            compare(f"map/save command {name}", save_ranges)

        # Opcode names are metadata. Every bytecode fixture below is synthetic.
        animation_names = (
            "Return SetScript Jump SetOffset BranchEqualByte BranchLessByte BranchGreaterByte BeginRepeat EndRepeat Call "
            "BranchEqualHalfword BranchLessHalfword BranchGreaterHalfword InvokeCallback BranchClassLess BranchClassGreater "
            "Advance Reserved WriteByte WriteHalfword WriteWord IncrementByte IncrementHalfword IncrementWord "
            "DecrementByte DecrementHalfword DecrementWord AddByte AddHalfword AddWord AddClassValue Reserved "
            "SetFlags1 SetActive SetFlags2 SetX SetY SetZ SetRelativeX SetRelativeY SetRelativeZ AddX AddY AddZ "
            "SetVelocityX SetVelocityY SetVelocityZ SetAccelerationX SetAccelerationY SetAccelerationZ DisableAffine "
            "SetScaleVelocityX SetScaleVelocityY SetScaleX SetScaleY OverrideFlags2 RestoreFlags2 SetXFraction SetYFraction "
            "SetZFraction SetRotation SetRotationVelocity SetAffineField5E SetAffineField5F SetStateField38 SetAttr52 "
            "SetAttr54 SetOtherActive EnableBit6 DisableBit6 SetFrame SetDelay QueueDma QueueTileRows DecompressWram "
            "DecompressTileSet QueueTileSet SetTileSetFrame Reserved Reserved StartSong StopSong"
        ).split()
        animation = bind("hn_probe_animation", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
        render_animation = bind("hn_probe_render_animation", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
        render_ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x03007010, 4))
        slot = 0x02017000 + 10 * 0x68
        index_patterns = ((0, 0, 0, 0), (2, 0, 0, 0), (0x8010, 3, 0, 0),
                          (2, 3, 4, 5), (0, 0, 0, 0x8030), (0x8001, 0, 0, 0))
        for repeat in range(6):
            for opcode, suffix in enumerate(animation_names, 0x80):
                if opcode == 0x8D:
                    continue  # Unbound callback addresses must fault explicitly.
                fresh()
                both(0x03002D98, struct.pack("<I", 0x02017000))
                both(0x03002DC0, b"\x20")
                both(0x03002E08, struct.pack("<I", 0x02018000))
                both(0x03003480, b"\xFF" * 32)
                both(0x03002182, b"\xC0")  # Original suppression branch, not an audio stub.
                both(0x03001F50, bytes([10 if repeat & 1 else 255]))
                both(0x0201DC40, rng.randbytes(1024))
                both(0x02017000, rng.randbytes(32 * 0x68))
                both(slot + 4, struct.pack("<I", 0x02020000))
                both(slot + 8, struct.pack("<4I", *(0x02020000 + i * 32 for i in range(4))))
                both(slot + 0x56, struct.pack("<4H", *index_patterns[repeat]))
                both(slot + 0x4C, struct.pack("<H", 0xFFFD if repeat == 5 else 0x40))
                both(slot + 0x64, bytes([255 if repeat & 1 else 3]))
                both(0x03007010, struct.pack("<I", 0x02021000))
                both(0x02020100, bytes([repeat * 50]) + b"\x80\xFE\x7F")
                both(0x02022000, b"\x10\x10\0\0\0ABCDEFGH\0IJKLMNOP")
                target = 0x02030000 if repeat == 5 else 0x02020020
                payload = struct.pack("<H", 0xFE03)
                if opcode in (0x81, 0x82, 0x89):
                    payload = struct.pack("<I", target)
                elif opcode in (0x84, 0x85, 0x86):
                    payload = struct.pack("<IBI", 0x02020100, 127, target)
                elif opcode in (0x8A, 0x8B, 0x8C):
                    payload = struct.pack("<IHI", 0x02020100, 0x807F, target)
                elif opcode in (0x8E, 0x8F):
                    payload = struct.pack("<HI", 0x807F, target)
                elif 0x92 <= opcode <= 0x9D:
                    payload = struct.pack("<II", 0x02020100, 0xFEDCBA98)
                elif opcode in (0xA6, 0xA7, 0xA8):
                    payload = struct.pack("<BH", 3, 0xFE80)
                elif opcode in (0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1):
                    payload = struct.pack("<I", 0xFEDCBA98)
                elif opcode in (0xC0, 0xC3):
                    payload = bytes([3, 0x80])
                elif opcode in (0xC8, 0xC9):
                    payload = struct.pack("<IIH", 0x06004000, 0x02022000, 0x0210)
                elif opcode == 0xCA:
                    payload = struct.pack("<II", 0x02024000, 0x02022000)
                elif opcode == 0xCB:
                    payload = struct.pack("<I", 0x02022000)
                elif opcode == 0xCD:
                    payload = bytes([3, 2])
                elif opcode == 0xC6:
                    payload = struct.pack("<I", 0x02025000)
                first = 5 if opcode == 0xC6 else opcode
                both(0x02021000, bytes([first]) + payload + bytes(32))
                report4 = (C.c_uint32 * 4)()
                equal(animation(memory, slot, opcode, 0x03007010, report4), 1, f"animation success {opcode:02X} {repeat}")
                equal(report4[3], call("Animation" + suffix, slot, 0x03007010), f"animation yield {opcode:02X} {repeat}")
                compare(f"animation {opcode:02X} {repeat}", render_ranges)

        fresh()
        both(0x03002D98, struct.pack("<I", 0x02017000))
        both(0x03002DC0, b"\x20")
        both(0x03002D9C, struct.pack("<I", 0x02016000))
        both(0x03002E2C, struct.pack("<I", 0x02015000))
        for i in range(300):
            both(0x02017000, rng.randbytes(32 * 0x68))
            both(0x03002DB0, rng.randbytes(16))
            both(0x03002E40, rng.randbytes(176))
            both(slot + 0x65, bytes([i % 32]))
            if i % 3 == 0:
                both(slot + 0x18, struct.pack("<III", rng.randrange(512) << 16, rng.randrange(128) << 16, rng.randrange(128) << 16))
                both(0x03002DB0, struct.pack("<II", 0x400000, 0x400000))
            native("hn_render_physics", 1)(memory, slot)
            call("UpdateRenderPhysics", slot)
            compare(f"render physics {i}", ((slot, 0x68),))
            native("hn_render_screen_position", 1)(memory, slot)
            call("CalculateRenderScreenPosition", slot)
            compare(f"render screen position {i}", render_ranges)
            equal(native("hn_render_find_slot", 1, C.c_uint8)(memory, slot), call("FindRenderSlot", slot), f"find render slot {i}")
            equal(native("hn_render_class", 1, C.c_int32)(memory, slot) & 0xFFFFFFFF,
                  call("FindRenderClass", slot), f"find render class {i}")
        for i in range(100):
            order = list(range(32))
            rng.shuffle(order)
            both(0x02016000, bytes(order))
            both(0x02015000, rng.randbytes(256))
            moving, anchor, mode = rng.randrange(32), rng.randrange(32), i & 1
            equal(native("hn_render_reorder", 3, C.c_bool)(memory, moving, anchor, mode), True, f"native render order {i}")
            call("RepositionRenderOrder", moving, anchor, mode)
            compare(f"render order {i}", render_ranges)
        for i in range(100):
            both(0x02017000, rng.randbytes(32 * 0x68))
            both(0x03003480, rng.randbytes(32))
            for index in range(32):
                both(0x02017000 + index * 0x68 + 0x64, bytes([255 if index % 4 == 0 else rng.randrange(32)]))
                if index % 5 == 0:
                    both(0x02017000 + index * 0x68 + 0x3C, struct.pack("<HHH", 256, 256, 0))
            native("hn_render_mark_affine")(memory)
            call("MarkUsedAffineMatrices")
            compare(f"mark affine {i}", render_ranges)
            equal(native("hn_render_assign_affine", result=C.c_bool)(memory), True, f"native assign affine {i}")
            call("AssignRenderAffineMatrices")
            compare(f"assign affine {i}", render_ranges)
        for i in range(300):
            layer = i % 4
            both(0x03002E40, rng.randbytes(176))
            both(0x030034B0, rng.randbytes(48))
            x, y = rng.getrandbits(32), rng.getrandbits(32)
            native("hn_probe_background_offset", 4)(memory, layer, x, y, 0x02020200)
            call("CalculateBackgroundOffset", layer, x, y, 0x02020200, 0x02020204)
            compare(f"background offset {i}", render_ranges)

        fresh()
        equal(native("hn_runtime_tables_init", result=C.c_bool)(memory), True, "animation runtime table")
        cpu.mem_write(0x02000000, rom[0x8FE018:0x900EC8])
        both(0x03002D98, struct.pack("<I", 0x02017000))
        both(0x03002DC0, b"\x20")
        native("hn_render_slot_reset", 1)(memory, 10)
        call("ResetRenderSlot", 10)
        both(slot + 0x60, b"\x01")
        both(slot + 0x24, struct.pack("<III", 0x10000, 0, 0))
        both(slot + 4, struct.pack("<I", 0x02020000))
        # Position, repeat a moving frame, call a delayed subroutine, then stop.
        program = bytes([0xA3, 12, 0, 0x87, 3, 0xA9, 1, 0, 5]) + struct.pack("<I", 0x02025000)
        program += b"\x88\x89" + struct.pack("<I", 0x02020100) + b"\x01" + struct.pack("<I", 0x02025000) + b"\x80"
        both(0x02020000, program)
        both(0x02020100, bytes([0xA4, 20, 0, 0xC7, 2, 0x80]))
        report3 = (C.c_uint32 * 3)()
        for i in range(40):
            if not native_bytes(slot + 0x60, 1)[0]:
                break
            native("hn_render_physics", 1)(memory, slot)
            call("UpdateRenderPhysics", slot)
            equal(render_animation(memory, slot, 4096, report3), 1, f"native animation frame {i}")
            call("ProcessRenderAnimation", slot)
            compare(f"animation frame {i}", render_ranges)
        equal(native_bytes(slot + 0x60, 1), b"\0", "animation completes after nested flow")

        render = bind("hn_probe_render", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
        for case in range(120):
            fresh()
            count = 16 if case else 0
            both(0x03002D98, struct.pack("<I", 0x02017000))
            both(0x03002E2C, struct.pack("<I", 0x02015000))
            both(0x03002C90, rng.randbytes(0x108))
            both(0x03002DB0, struct.pack("<II4H4B", 0x400000, 0x400000, 0x1C0, 0xC0, 105, 65,
                                      count, case % 3, count - 1 if count else 0, 0))
            both(0x03002E40, rng.randbytes(176))
            both(0x030034B0, rng.randbytes(48))
            both(0x03003080, rng.randbytes(1024))
            order = list(range(count))
            rng.shuffle(order)
            for index in range(count):
                entry = 0x02015000 + index * 8
                both(entry, rng.randbytes(8))
                both(entry + 5, bytes([order[index]]))
                current = 0x02017000 + index * 0x68
                both(current, rng.randbytes(0x68))
                both(current, struct.pack("<I", 0x02026000 + index * 32))
                both(current + 0x18, struct.pack("<III", rng.randrange(256) << 16, rng.randrange(160) << 16, rng.randrange(64) << 16))
                both(current + 0x3C, struct.pack("<HHH", rng.randrange(64, 513), rng.randrange(64, 513), rng.randrange(65536)))
                both(current + 0x60, bytes([int(index % 5 != 0), rng.randrange(256), rng.randrange(256), 0,
                                          255 if index % 3 == 0 else 0, rng.randrange(count), 0, rng.randrange(256)]))
                parts = bytearray([index % 5, 0])
                for _ in range(index % 5):
                    attr0 = rng.randrange(3) << 14 | rng.randrange(0x4000)
                    parts += struct.pack("<HHH", attr0, rng.randrange(65536), rng.randrange(65536))
                both(0x02026000 + index * 32, bytes(parts))
            equal(render(memory, 4096, 0, report3), 1, f"native render draw {case}")
            call("DrawRenderSlots", limit=1000000)
            compare(f"render draw {case}", ((0x02000000, 0x40000), (0x03000000, 0x7000)))

        background_command = bind("hn_probe_background_command", C.c_int, C.c_void_p, C.c_uint32,
                                  C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
        background = bind("hn_probe_background", C.c_int, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        background_handlers = (
            "Return", "WaitMotion", "Jump", "SetField38", "SetAccelerationY", "SetAccelerationX",
            "SetStepY", "SetStepX", "AddOffsetY", "AddOffsetX", "SetPositionY", "SetPositionX",
            "Reserved8C", "Reserved8D", "Reserved8E", "Reserved8F", "BeginRepeat", "EndRepeat",
            "SetDelay", "WriteByte", "WriteHalfword", "Decompress", "Reserved96", "DmaCopyRows64",
            "Stop98", "BiosCopy", "BiosCopyOffset", "Stop9B", "QueueByteWrite", "QueueHalfwordWrite",
            "QueueCpuCopy", "QueueCpuCopyOffset", "Call", "DmaCopyTilemapRectangle", "SetRotation",
            "SetField3E", "SetField3F", "AddRotation", "SetScaleY", "SetScaleX", "SetField44",
            "SetField40", "SetField46", "SetField42", "AddScaleY", "AddScaleX", "SetAnchorX",
            "SetAnchorY", "JumpIfByteEqual", "JumpIfByteLess", "JumpIfByteGreater", "SkipWord",
            "SkipFiveBytes", "SkipByteB5", "SkipByteB6", "SkipByteB7", "SkipHalfword", "SetField48",
            "SetField4A", "SetField4C", "SetField4E")
        background_ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x05000000, 0x400),
                             (0x06000000, 0x18000), (0x04000000, 0x58))
        state = 0x02018000
        def prepare_background(case):
            fresh()
            both(0x03002C90, rng.randbytes(0x108))
            both(0x03002CC4, bytes([case % 3, 0]))
            both(0x03002E08, struct.pack("<I", state))
            both(0x03002C80, b"\x02")
            both(0x03002E40, rng.randbytes(176))
            both(0x030034B0, rng.randbytes(48))
            both(state, rng.randbytes(80))
            both(state + 0x14, struct.pack("<I", 0x02021000))
            both(state + 0x38, bytes([(1, 4, 8, 15)[case % 4]]))
            for layer in range(4):
                both(0x03002E40 + layer * 44 + 0x18, struct.pack("<HHH", 128 + case, 384 - case, case * 17 % 256))
            counts = ((0, 0, 0, 0), (3, 0, 0, 0), (2, 1, 0, 0), (2, 2, 0, 0), (1, 1, 1, 0), (1, 3, 1, 0))
            both(state + 0x39, bytes(counts[case % 6]))
            both(state + 0x3D, bytes([case % 4]))
            for i in range(8):
                both(state + 0x18 + i * 4, struct.pack("<I", 0x02021800 + i * 16))
            both(0x03007010, struct.pack("<I", 0x02021000))
            both(0x02022000, rng.randbytes(512))
            both(0x02024000, b"\x10\x08\0\0\0ABCDEFGH")
        for case in range(6):
            for opcode, suffix in enumerate(background_handlers, 0x80):
                prepare_background(case)
                payload = struct.pack("<I", 0x123480FF)
                if opcode in (0x82, 0xA0):
                    payload = struct.pack("<I", 0x02021800)
                elif opcode == 0x90:
                    payload = b"\x04"
                elif opcode in (0x93, 0x94, 0x9C, 0x9D):
                    payload = struct.pack("<IH", 0x02025000, 0xFE80)
                elif opcode == 0x95:
                    payload = struct.pack("<III", 0x06000000, 0x02024000, 0x4000)
                elif opcode == 0x97:
                    payload = struct.pack("<IIBB", 0x02022000, 0x06004000, 6, 3)
                elif opcode in (0x99, 0x9E):
                    payload = struct.pack("<III", 0x02022000, 0x06004000, 14 if case & 1 else 32)
                elif opcode in (0x9A, 0x9F):
                    payload = struct.pack("<IIII", 0x02022000, 2, 0x06004000, 14)
                elif opcode == 0xA1:
                    # Text copies span screen blocks; affine maps retain the original size-bit stride.
                    payload = struct.pack("<IBBBB", 0x02022000, 28, 30, 35, 33)
                    if case % 3:
                        payload = struct.pack("<IBBBB", 0x02022000, 2, 3, 9, 6)
                elif opcode in (0xB0, 0xB1, 0xB2):
                    both(0x02025000, bytes([127 + case % 3]))
                    payload = struct.pack("<IBI", 0x02025000, 128, 0x02021800)
                both(0x02021000, payload)
                equal(background_command(memory, state, opcode, 0x03007010, report4), 1,
                      f"native background command {opcode:02X} {case}")
                equal(report4[3], call("BackgroundCommand" + suffix, state, 0x03007010, limit=1000000),
                      f"background yield {opcode:02X} {case}")
                compare(f"background command {opcode:02X} {case}", background_ranges)
        for case in range(300):
            prepare_background(case % 6)
            layer = case % 4
            p = 0x03002E40 + layer * 44
            for axis in range(4):
                coordinate = p + (axis & 1) * 4
                native("hn_background_clamp", 3)(memory, layer, coordinate, axis)
                call("ClampBackgroundCoordinate", layer, coordinate, axis)
                compare(f"background clamp {case} {axis}", background_ranges[:2])
            equal(native("hn_background_apply", 1, C.c_bool)(memory, layer), True, f"background apply success {case}")
            call("ApplyBackgroundLayer", layer)
            compare(f"background apply {case}", background_ranges[:2])
            # Keep the affine reciprocal away from zero in this CPU-only oracle.
            for i in range(4):
                both(state + 0x40 + i * 4, struct.pack("<HH", 0xFFF0, 0x0020))
            equal(native("hn_background_motion", 1, C.c_bool)(memory, state), True, f"background motion success {case}")
            call("UpdateBackgroundMotion", state)
            compare(f"background motion {case}", background_ranges[:2])

        prepare_core()
        memory_copy = bind("hn_memory_copy", C.c_bool, C.c_void_p, C.c_uint32, C.c_uint32, C.c_size_t)
        video_render = bind("hn_video_render", None, C.c_void_p, C.c_void_p)
        if args.video_oracle:
            from compare_video import VideoOracle
            oracle_context = VideoOracle(args.video_oracle, args.oracle_runtime_dir)
        else:
            oracle_context = nullcontext(None)
        video_frames, nonuniform = 0, 0
        with oracle_context as video_oracle:
            for index in range(180):
                equal(frame(memory, 4096, report), 1, f"coupled Main {index}")
                call("RunMainScriptFrame")
                equal(background(memory, 4096, report3), 1, f"coupled native background {index}: {list(report3)}")
                call("UpdateBackgroundStates", limit=1000000)
                equal(render(memory, 4096, 1, report3), 1, f"coupled native render {index}")
                call("UpdateRenderSlots", limit=1000000)
                both(0x04000006, b"\xA0\0")
                equal(memory_copy(memory, 0x07000000, 0x03003080, 1024), True, "OAM shadow transfer")
                cpu.mem_write(0x07000000, bytes(cpu.mem_read(0x03003080, 1024)))
                native("hn_video_commit")(memory)
                call("CommitVideoRegisters")
                native("hn_transfer_flush")(memory)
                call("FlushDmaTransfers")
                compare(f"coupled Main/background/render {index}", save_ranges + ((0x05000000, 0x400),
                    (0x06000000, 0x18000), (0x07000000, 0x400), (0x04000000, 0x58)))
                if video_oracle and index in (0, 24, 32, 64, 87, 120, 179):
                    pixels = (C.c_uint32 * (240 * 160))()
                    video_render(memory, pixels)
                    parts = [native_bytes(address, size) for address, size in
                        ((0x04000000, 1024), (0x05000000, 1024), (0x06000000, 98304), (0x07000000, 1024))]
                    equal(bytes(pixels), video_oracle.render(parts), f"private ROM PPU snapshot {index}")
                    video_frames += 1
                    nonuniform += len(set(pixels)) > 1
        if args.video_oracle:
            print(f"{video_frames} private ROM PPU snapshots matched; {nonuniform} contained multiple colors.")
    finally:
        destroy(memory)
    print(f"{checks} private native/reference checks passed (heap, input, scripts, expressions, rendering, maps, saves).")
    print("Function-level evidence only; boot and gameplay remain unverified.")

if __name__ == "__main__":
    main()
