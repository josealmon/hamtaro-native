"""Private function-level checks against a player-supplied AH3P ROM.

Requires Unicorn in the calling Python environment. No ROM, save or captured
state is written by this tool; the reference checkout is read-only.
"""
from __future__ import annotations
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import random
import struct
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_INTR
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
    args = parser.parse_args()
    spec = json.loads((ROOT / "config/rom.json").read_text())
    rom = args.rom.read_bytes()
    if len(rom) != spec["size"] or hashlib.sha256(rom).hexdigest() != spec["sha256"]:
        raise SystemExit("A matching European AH3P revision 0 ROM is required.")
    entries = json.loads((args.reference / "build/function-symbols.json").read_text())
    symbols = {row["name"]: row["address"] for row in entries}
    cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
    for address, size in ((0x02000000, 0x40000), (0x03000000, 0x8000),
                          (0x04000000, 0x1000), (0x08000000, len(rom)), (STOP, 0x1000)):
        cpu.mem_map(address, size)
    cpu.mem_write(0x08000000, rom)
    cpu.reg_write(UC_ARM_REG_SP, 0x03007C00)

    def bios(uc: Uc, number: int, _context) -> None:
        pc = uc.reg_read(UC_ARM_REG_PC)
        instruction = bytes(uc.mem_read(pc - 2, 2))
        if number != 2 or instruction != b"\x06\xdf":
            raise RuntimeError("Unexpected BIOS service in the selected function checks.")
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

    def call(name: str, *arguments: int) -> int:
        for register, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), arguments):
            cpu.reg_write(register, value)
        cpu.reg_write(UC_ARM_REG_LR, STOP | 1)
        cpu.emu_start(symbols[name] | 1, STOP, count=200000)
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
    write8 = bind("hn_write8", None, C.c_void_p, C.c_uint32, C.c_uint8)
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
            raise RuntimeError(f"Native/reference mismatch: {label}")

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
            for offset, value in enumerate(payload):
                write8(memory, address + offset, value)
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
    finally:
        destroy(memory)
    print(f"{checks} private native/reference checks passed (heap, PRNG, input, script control).")
    print("Function-level evidence only; boot and gameplay remain unverified.")

if __name__ == "__main__":
    main()
