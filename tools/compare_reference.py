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
import os
from pathlib import Path
import random
import re
import struct
import subprocess
import sys
import time
from functools import wraps
from contextlib import nullcontext
from unicorn import (Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_INTR,
                     UC_HOOK_MEM_WRITE, UC_HOOK_MEM_READ, UC_HOOK_CODE)
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
    parser.add_argument("--bus-oracle", type=Path, help="Optional separate mGBA floating-bus halfword oracle")
    parser.add_argument("--oracle-runtime-dir", type=Path, help="Optional runtime DLL directory for the PPU oracle")
    parser.add_argument("--audio-only", action="store_true", help="Run just the audio function comparisons")
    parser.add_argument("--scenes-only", action="store_true", help="Run scene menus, input, text and transition comparisons")
    parser.add_argument("--actors-only", action="store_true", help="Run actor contexts, motion setup/routes and supporting Main commands")
    parser.add_argument("--tasks-only", action="store_true", help="Run text-task lifecycle, graphics and script comparisons")
    parser.add_argument("--choices-only", action="store_true", help="Run word-choice dialogs and supporting script comparisons")
    parser.add_argument("--motion-only", action="store_true", help="Run motion setup, routes and live follower comparisons")
    parser.add_argument("--collisions-only", action="store_true", help="Run leader motion, collision and wait-handler comparisons")
    parser.add_argument("--scripts-only", action="store_true", help="Run Main memory commands and scene-transition comparisons")
    parser.add_argument("--book-only", action="store_true", help="Run Ham-Chat acquisition, page and decoration comparisons")
    parser.add_argument("--indicators-only", action="store_true", help="Run destination progress, indicators, navigation and unlock comparisons")
    parser.add_argument("--presets-only", action="store_true", help="Run scene-render presets, actor dependencies and typed task/Main contracts")
    parser.add_argument("--map-choice-only", action="store_true", help="Run complete map-choice, particle and dependency comparisons (requires BIOS oracle)")
    parser.add_argument("--bank2-only", action="store_true", help="Run complete bank-2 actor/control/preset/height families (requires BIOS oracle)")
    parser.add_argument("--bank3-late-only", action="store_true", help="Run complete late bank-3 scenes/music/modal family (requires BIOS oracle)")
    parser.add_argument("--bank3-presets-only", action="store_true", help="Run bank-3 scene presets and base-actor dependency family (requires BIOS oracle)")
    parser.add_argument("--bank3-games-only", action="store_true", help="Run current bank-3 phase/orbit and minigame contracts (requires BIOS oracle)")
    parser.add_argument("--bank3-chain-only", action="store_true", help="Run complete bank-3 chain family and calling contracts (requires BIOS oracle)")
    parser.add_argument("--bank4-only", action="store_true", help="Run complete bank-4 family and typed contracts (requires BIOS oracle)")
    parser.add_argument("--bank4-presets-only", action="store_true", help="Run all bank-4 presentation presets (requires BIOS oracle)")
    parser.add_argument("--bank4-dynamics-only", action="store_true", help="Run bank-4 jumps, counters, audio, paths and helpers (requires BIOS oracle)")
    parser.add_argument("--bank5-only", action="store_true", help="Run complete bank-5 family and dependencies (requires BIOS oracle)")
    parser.add_argument("--bank5-presets-only", action="store_true", help="Run all bank-5 presentation presets (requires BIOS oracle)")
    parser.add_argument("--bank5-dynamics-only", action="store_true", help="Run bank-5 grid, mirror, mechanism and animation contracts (requires BIOS oracle)")
    parser.add_argument("--bank6-only", action="store_true", help="Run bank-6 main/final families and dependencies (requires BIOS oracle)")
    parser.add_argument("--bank6-presets-only", action="store_true", help="Run bank-6 main/final presentation presets (requires BIOS oracle)")
    parser.add_argument("--bank6-dynamics-only", action="store_true", help="Run bank-6 main/final motion, cursor and target contracts (requires BIOS oracle)")
    parser.add_argument("--bank6-chain-only", action="store_true", help="Run the complete bank-6 chain family (requires BIOS oracle)")
    parser.add_argument("--presentation-only", action="store_true", help="Run complete shared presentation families (requires BIOS oracle)")
    parser.add_argument("--bank7-only", action="store_true", help="Run complete bank-7 board/preset families (requires BIOS oracle)")
    parser.add_argument("--record-selection-only", action="store_true", help="Run complete record counter/selection/transaction families (requires BIOS oracle)")
    parser.add_argument("--wander-only", action="store_true", help="Run connected actor-wandering families (requires BIOS oracle)")
    parser.add_argument("--scene-actors-only", action="store_true", help="Run complete cutscene actor/state/panel families")
    parser.add_argument("--actor-presets-only", action="store_true", help="Run all five actor-preset families (requires BIOS oracle)")
    parser.add_argument("--actor-animations-only", action="store_true", help="Run complete graphics/directional/extended actor animation families (requires BIOS oracle)")
    parser.add_argument("--actor-sequences-only", action="store_true", help="Run complete actor render-sequence family and typed task contracts")
    parser.add_argument("--records-only", action="store_true", help="Run game record allocation/query/consume/counter families")
    parser.add_argument("--group3-only", action="store_true", help="Run SaveGroup3, record/item selection and strided transfer families")
    parser.add_argument("--group3-scenes-only", action="store_true", help="Run SaveGroup3 Main/task contracts")
    parser.add_argument("--record-menus-only", action="store_true", help="Run complete record selection/menu family")
    parser.add_argument("--selection-scenes-only", action="store_true", help="Run complete shared map-10/5 item selection family")
    parser.add_argument("--wave-only", action="store_true", help="Run complete scanline-wave family and typed IRQ dispatch")
    parser.add_argument("--counter-scenes-only", action="store_true", help="Run complete map-10 counter/sequence family")
    parser.add_argument("--shared-motion-only", action="store_true", help="Run shared actor geometry/motion families (requires BIOS oracle)")
    parser.add_argument("--scene-effects-only", action="store_true", help="Run complete background/render shake and screen-window families (requires BIOS oracle)")
    parser.add_argument("--bios-oracle",type=Path,help="Separate mGBA angular BIOS oracle, required for full/collision comparisons")
    parser.add_argument("--timings-json",type=Path,help="Optional private per-phase timing report outside the checkout")
    args = parser.parse_args()
    if args.timings_json:
        args.timings_json=args.timings_json.resolve()
        if args.timings_json==ROOT or ROOT in args.timings_json.parents:
            parser.error("Timing reports must be outside the checkout.")
    started=time.perf_counter();phases=[]
    if not args.bios_oracle and (args.collisions_only or args.map_choice_only or args.bank2_only or args.bank3_late_only or args.bank3_presets_only or args.bank3_games_only or args.bank3_chain_only or args.bank4_only or args.bank4_presets_only or args.bank4_dynamics_only or args.bank5_only or args.bank5_presets_only or args.bank5_dynamics_only or args.bank6_only or args.bank6_presets_only or args.bank6_dynamics_only or args.wander_only or args.actor_presets_only or args.actor_animations_only or args.shared_motion_only or args.scene_effects_only or not any((args.audio_only,args.scenes_only,args.actors_only,args.tasks_only,args.choices_only,args.motion_only,args.collisions_only,args.scripts_only,args.book_only,args.indicators_only,args.presets_only,args.scene_actors_only,args.records_only,args.wave_only,args.counter_scenes_only,args.group3_only,args.group3_scenes_only,args.actor_sequences_only,args.record_menus_only,args.selection_scenes_only))):
        parser.error("Full/collision comparisons require --bios-oracle PATH_TO_HN_BIOS_ORACLE.")
    bios_oracle=None
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
        if service==8:
            # This function probe models only Sqrt's integer result, not BIOS
            # timing or volatile scratch registers. Callers compare memory.
            uc.reg_write(UC_ARM_REG_R0,math.isqrt(uc.reg_read(UC_ARM_REG_R0)))
            return
        if service==0x0A:
            if bios_oracle is None:raise RuntimeError("ArcTan2 comparison requires --bios-oracle.")
            result=bios_oracle.call(10,uc.reg_read(UC_ARM_REG_R0),uc.reg_read(UC_ARM_REG_R1))
            for register,value in zip((UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R3),(result[0],result[1],result[3])):uc.reg_write(register,value)
            return
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
            if header & 0xF0 != 0x10 or header >> 8 > 0x40000:
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

    audio_wait, audio_scanline_reads = False, 0
    def audio_scanline(uc, _access, _address, _size, _value, _context):
        nonlocal audio_scanline_reads
        if audio_wait:
            # Model only the frequency setter's explicit boundary wait. This is
            # not elapsed CPU time, FIFO sampling, or a complete hardware clock.
            audio_scanline_reads += 1
            uc.mem_write(0x04000006, bytes([0 if audio_scanline_reads & 1 else 159, 0]))
    cpu.hook_add(UC_HOOK_MEM_READ, audio_scanline, begin=0x04000006, end=0x04000006)

    def call(name: str, *arguments: int, limit=200000) -> int:
        nonlocal audio_wait, audio_scanline_reads
        for register, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), arguments):
            cpu.reg_write(register, value)
        if len(arguments) > 4:
            cpu.mem_write(cpu.reg_read(UC_ARM_REG_SP), struct.pack(f"<{len(arguments)-4}I", *arguments[4:]))
        cpu.reg_write(UC_ARM_REG_LR, STOP | 1)
        audio_wait = name in {"InitializeAudioDriver", "InitializeAudioCore", "SetAudioSampleFrequency", "SetAudioCoreMode"}
        audio_scanline_reads = 0
        try:
            cpu.emu_start(symbols[name] | 1, STOP, count=limit)
        finally:
            audio_wait = False
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

    def timed(function):
        @wraps(function)
        def run(*arguments,**keywords):
            begin=time.perf_counter();before=checks;status="failed"
            try:
                result=function(*arguments,**keywords);status="passed";return result
            finally:
                row=dict(phase=function.__name__,seconds=round(time.perf_counter()-begin,3),checks=checks-before,status=status)
                phases.append(row)
                print(f"Timing {row['phase']}: {row['seconds']:.3f}s, {row['checks']} checks, {status}.",flush=True)
        return run

    buffers={}
    def native_bytes(address: int, size: int) -> bytes:
        out=buffers.get(size)
        if out is None:out=buffers.setdefault(size,C.create_string_buffer(size))
        if not read(memory, address, out, size):
            raise RuntimeError("Native guest-memory read failed.")
        return out.raw

    def both(address: int, payload: bytes) -> None:
        cpu.mem_write(address, payload)
        if not seed(memory, address, payload, len(payload)):
            raise RuntimeError("Private fixture injection failed.")

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

    @timed
    def scene_checks():
        fresh()
        both(0x03000820, rng.randbytes(26))
        native("hn_input_initialize")(memory);call("InitializeKeys")
        compare("guest input initialization", ((0x03000820, 26),))
        held = [0x91]*110+[0]*12+[rng.randrange(1024) for _ in range(134)]
        for case, buttons in enumerate(held):
            if case == 100:
                both(0x03000832, b"\xFF\xFF")
            if case == 150:
                native("hn_input_repeat", 2)(memory, 16, 8);call("SetKeyRepeatDelays", 16, 8)
            both(0x04000130, struct.pack("<H", (~buttons)&1023))
            equal(native("hn_input_update", 1, C.c_bool)(memory, buttons), True, f"guest input {case}")
            call("ReadKeys");compare(f"input cache {case}", ((0x03000820, 26), (0x04000130, 2)))
            for mode in range(4):
                mask = rng.randrange(65536)
                equal(native("hn_input_query", 2, C.c_uint16)(memory, mask, mode), call("QueryKeys", mask, mode),
                      f"key query {case}/{mode}")
        both(0x03000000, struct.pack("<3I", 0x3AC2, 0x4DF3, 24))
        for case in range(128):
            lower, upper = ((0, 6), (0, 0), (5, 2), (65535, 65535), (0, 65535), (32768, 65535))[case%6]
            equal(native("hn_random_range", 2, C.c_uint16)(memory, lower, upper), call("RandomRange", lower, upper),
                  f"guest random range {case}")
            compare("guest random seeds", ((0x03000000, 12),))
        equal(native("hn_audio_driver_init", result=C.c_bool)(memory), True, "scene audio init")
        call("InitializeAudioDriver", limit=2000000)
        both(0x03002D98, struct.pack("<I", 0x02010000))
        both(0x03002E08, struct.pack("<I", 0x02018000))
        both(0x03002DC0, bytes([32]))
        both(0x03002C80, bytes([9]))
        both(0x02010000, rng.randbytes(32*104))
        both(0x02018000, rng.randbytes(9*80))
        both(0x03001F50, bytes([27]))
        both(0x03000000, struct.pack("<3I", 0x3AC2, 0x4DF3, 24))
        ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x03007FF0, 4), (0x04000060, 0x74), (0x04000100, 4))
        def invoke(name, payload=b""):
            both(0x02030000, struct.pack("<I", symbols[name] | 1) + payload + bytes(8))
            equal(command(memory, 0x1C, 0x02030000), 1, f"native scene {name}")
            call("MainScriptInvokeCallback", 0x02030000, limit=2000000)
            compare(name, ranges)
        both(0x030034B0,rng.randbytes(48))
        invoke("ScriptCallbackSetBackground2RightBottom1024")
        configure = native("hn_render_configure", 7, C.c_bool)
        for case in range(60):
            index = case % 32
            x, y, z = (rng.getrandbits(32) for _ in range(3))
            values = (index, rng.randrange(256), rng.randrange(256), x, y, z, 0x02031000)
            both(0x03002170, b"\xFF")
            both(0x03002182, bytes([rng.randrange(256)]))
            equal(configure(memory, *values), True, f"configure word-choice slot {case}")
            call("ConfigureWordChoiceRenderSlot", *values)
            compare(f"word-choice render {case}", ranges[:2])
        for selected in range(5):
            for mode in range(2):
                both(0x0200453E, bytes([selected]));both(0x0200460C, bytes([mode]))
                invoke("ScriptCallbackRenderSceneRecordMenu")
                invoke("ScriptCallbackShowSceneRecordMenuOption")
                for keys in (0, 0x40, 0x80, 0xC0):
                    both(0x0200453E, bytes([selected]));both(0x03000822, struct.pack("<H", keys))
                    invoke("ScriptCallbackMoveSceneRecordMenu")
        for enabled in (15, 3, 5, 1):
            for language in range(5):
                for selected in range(6):
                    both(0x0200453B, bytes([enabled]));both(0x0200460B, bytes([language]))
                    both(0x02004539, bytes([selected]))
                    invoke("ScriptCallbackRefreshSceneMenu")
                    invoke("ScriptCallbackShowSceneMenuOption")
                    invoke("ScriptCallbackReadSceneMenuSelection")
                    for keys in (0x40, 0x80, 0xC0):
                        both(0x02004539, bytes([selected]));both(0x03000822, struct.pack("<H", keys))
                        invoke("ScriptCallbackMoveSceneMenu")
        for case in range(32):
            both(0x03002180, rng.randbytes(64))
            both(0x02004539, bytes([0 if case & 1 else 3]));both(0x0200460B, bytes([case % 5]))
            invoke("ScriptCallbackInitializeSceneMenu")
            invoke("ScriptCallbackSaveSceneRecordLanguage")
            both(0x0200460B, rng.randbytes(2));invoke("ScriptCallbackRestoreSceneRecordLanguage")
            invoke("ScriptCallbackSetSceneRecordKeyRepeat")
            both(0x0200453F, bytes([255 if case % 3 == 0 else case]))
            both(0x03004ED4, struct.pack("<I", 0x80000000 if case & 1 else 0))
            invoke("ScriptCallbackPollSceneMenuAudio")
            both(0x0200453D, bytes([case % 5]));both(0x0200460B, bytes([case % 8]))
            invoke("ScriptCallbackSetSceneMenuStatusBackground")
            both(0x03003730, bytes(96))
            for i in range(6):
                if case & 1 << i:
                    both(0x03003730+i*16, bytes([i+1]))
            invoke("ScriptCallbackChooseSceneSaveRecord")
        for case in range(256):
            both(0x03000820, rng.randbytes(8));both(0x03001F64, struct.pack("<H", rng.randrange(1024)))
            both(0x03002180, rng.randbytes(2));both(0x020046C1, bytes([case]))
            for name in ("ScriptCallbackReadPressedKeyMask", "ScriptCallbackReadRepeatKeyMask",
                         "ScriptCallbackReadDirectionalHeldAndPressedKeyMask", "ScriptCallbackReadSceneRecordDialogInput"):
                invoke(name)
        for case in range(512):
            both(0x03000820, rng.randbytes(8))
            payload = struct.pack("<H", rng.getrandbits(16) if case > 32 else (1 << (case % 10)))
            for name in ("ScriptCallbackReadHeldKeyOrdinal", "ScriptCallbackReadPressedKeyOrdinal",
                         "ScriptCallbackReadRepeatKeyOrdinal", "ScriptCallbackReadDirectionalHeldAndPressedKeyOrdinal"):
                invoke(name, payload)
        for case in range(160):
            for suffix,half in (("AddIndirectByte",False),("SubtractIndirectByte",False),
                                ("AddIndirectHalfword",True),("SubtractIndirectHalfword",True)):
                target=(0x02028000,0x03002182,0x0300113A if half else 0x0300113B,0x0300113C)[case%4]
                both(0x03001134,rng.randbytes(12));both(0x03002182,bytes([rng.randrange(256)]))
                if target==0x02028000:both(target,rng.randbytes(2))
                amount=(0,1,2,127,128,254,255,256,32768,65535)[case%10]
                payload=struct.pack("<HI",amount,target) if half else struct.pack("<BI",amount&255,target)
                invoke("ScriptCallback"+suffix,payload)
        for case in range(32):
            both(0x03002C0C,bytes([case]));both(0x03001F50,bytes([31-case]))
            invoke("ScriptCallbackSetPrimaryRenderSlots",bytes([255 if case&1 else case,255 if case&2 else 31-case]))
            invoke("ScriptCallbackReadPrimaryRenderDirectionNibbles")
            invoke("ScriptCallbackReadRenderDirectionNibble",bytes([case]))
        for case in range(48):
            payload = struct.pack("<B3H", case & 1, case % 32, case % 7, case % 13)
            both(0x02030000, payload)
            equal(command(memory, 0x50, 0x02030000), 1, f"native random script {case}")
            call("MainScriptStoreRandomRange", 0x02030000);compare("random script", ranges)
            both(0x02030000, struct.pack("<H", rng.randrange(65536)))
            equal(command(memory, 0x5B, 0x02030000), 1, "native choice flags")
            call("MainScriptSetWordChoiceOptionFlags", 0x02030000);compare("choice flags", ranges)
        equal(command(memory, 0x5A, 0x02030000), 1, "native script core reset")
        call("MainScriptResetCoreState", 0x02030000);compare("script core reset", ranges)

    @timed
    def text_checks():
        ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x06000000, 0x18000), (0x040000D4, 12))
        blit = native("hn_text_blit_glyph", 5, C.c_bool)
        offsets = (0,1,2,3,4,5,6,7,8,9,15,16,17,31,55,56,63,127,255,256,257,65535,65536,65537)
        for width in (0,1,2,7,255,256,257):
            for x in offsets:
                fresh();heap_init(memory);call("InitializeHeap", 0)
                shadow = allocate(memory, 0x4000, False)
                equal(shadow, call("AllocateHeap", 0x4000), "glyph shadow allocation")
                both(shadow, rng.randbytes(0x4000));both(0x02010000, rng.randbytes(64))
                palette = rng.getrandbits(32)
                equal(blit(memory, 0x02010000, shadow, width, x, palette), True, f"glyph success {width}/{x}")
                call("BlitTaskGlyphToShadow", 0x02010000, shadow, width, x, palette)
                compare(f"glyph bytes/heap {width}/{x}", ranges[:2])
        fresh();heap_init(memory);call("InitializeHeap",0)
        both(0x03002D98, struct.pack("<I", 0x02010000));both(0x03002DC0, bytes([40]))
        both(0x03003070, struct.pack("<I",0x02018000));both(0x03002E10, bytes([8]))
        both(0x02010000, rng.randbytes(40*104));both(0x02018000, rng.randbytes(8*44))
        both(0x03001F50, bytes([29]));both(0x03001F64, struct.pack("<H",1023))
        def invoke(name, payload=b""):
            both(0x02030000,struct.pack("<I",symbols[name]|1)+payload+bytes(8))
            equal(command(memory,0x1C,0x02030000),1,f"native text callback {name}")
            call("MainScriptInvokeCallback",0x02030000,limit=2000000)
            compare(name,ranges)
        for language in range(5):
            both(0x0200460B,bytes([language]));invoke("ScriptCallbackResetBothSceneTexts")
        for mode in (0,1,2):
            both(0x020046C2,bytes([mode]))
            for position in range(8):
                both(0x020046C4,bytes([position]));both(0x03002BA0,rng.randbytes(9));both(0x03001F70,rng.randbytes(9))
                invoke("ScriptCallbackInitializeSceneTextFirst");invoke("ScriptCallbackInitializeSceneTextSecond")
                invoke("ScriptCallbackRefreshSceneTextCaret");invoke("ScriptCallbackTrimSceneTextPadding")
        for bank in range(3):
            for index in range(77):
                both(0x020046C3,bytes([index]));both(0x020046C5,bytes([bank]))
                invoke("ScriptCallbackRefreshSceneTextGrid")
                for direction in (0x10,0x20,0x40,0x80):
                    both(0x020046C3,bytes([index]));both(0x0201DC40,struct.pack("<H",direction))
                    invoke("ScriptCallbackMoveSceneTextGrid")
        for case in range(64):
            both(0x020046C2,bytes([1+case%2]));both(0x020046C3,bytes([case%77]))
            both(0x020046C4,bytes([case%8]));both(0x020046C5,bytes([case%3]))
            # Seven synthetic printable/spacer codes, with the original terminator.
            data = bytes([1 if case%4==0 else rng.randrange(1,240) for _ in range(7)])+bytes(2)
            both(0x03002BA0,data);both(0x03001F70,data)
            both(0x03000822,struct.pack("<H",rng.randrange(1024)));both(0x03000824,struct.pack("<H",rng.randrange(1024)))
            invoke("ScriptCallbackReadSceneTextInput")
            invoke("ScriptCallbackSelectSceneTextCharacter")
            invoke("ScriptCallbackEraseSceneTextCharacter")
            invoke("ScriptCallbackConfirmSceneText")
            invoke("ScriptCallbackAdvanceSceneTextPosition");invoke("ScriptCallbackRetreatSceneTextPosition")
            invoke("ScriptCallbackNextSceneTextBank");invoke("ScriptCallbackPreviousSceneTextBank")
            invoke("ScriptCallbackSelectSceneTextConfirm");invoke("ScriptCallbackSceneTextNoop")
        animation = bind("hn_probe_animation", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
        report_animation = (C.c_uint32*4)()
        for language in range(5):
            for variant in range(4):
                both(0x0200460B,bytes([language]));invoke("ScriptCallbackInitializeScenePanelTransition",bytes([variant]))
                row=language*92+variant*23
                count=max(0,struct.unpack("<b",rom[0x6E6B32+row:0x6E6B33+row])[0])
                def physics():
                    for i in range(count):
                        slot=0x02010000+i*104
                        native("hn_render_physics",1)(memory,slot);call("UpdateRenderPhysics",slot)
                for step in range(160):
                    invoke("ScriptCallbackUpdateScenePanelEntry",bytes([variant]));physics()
                    if native_bytes(0x020046C6,1)==b"\xFF":break
                equal(native_bytes(0x020046C6,1),b"\xFF","panel entry terminates")
                both(0x020046D9,bytes([variant]));both(0x020046D7,b"\x01\0")
                slot=0x02010000+39*104
                for step in range(96):
                    both(0x02030000,b"\x8D"+struct.pack("<I",symbols["UpdateScenePanelDepth"]|1))
                    both(0x03007010,struct.pack("<I",0x02030000))
                    equal(animation(memory,slot,0x8D,0x03007010,report_animation),1,"native depth animation callback")
                    equal(report_animation[3],call("AnimationInvokeCallback",slot,0x03007010),"depth animation yield")
                    physics();compare("panel depth state",ranges+((0x03007010,4),))
                invoke("ScriptCallbackScheduleScenePanelExit",bytes([variant]))
                for step in range(160):
                    invoke("ScriptCallbackUpdateScenePanelExit",bytes([variant]));physics()
                    if native_bytes(0x020046C6,1)==b"\xFF":break
                equal(native_bytes(0x020046C6,1),b"\xFF","panel exit terminates")

    @timed
    def counter_checks():
        ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x05000000,0x400))
        fresh()
        for bank in range(3):
            both(0x0201DC40,rng.randbytes(32))
            for number in (0,4,5,9,10,99,100,459,500,999,1000,65535,-1,-123,-2147483648,2147483647):
                equal(native("hn_text_counter",2,C.c_bool)(memory,number&0xFFFFFFFF,bank),True,"native counter bank")
                call("RenderGameCounterBank"+str(bank),number&0xFFFFFFFF);compare("counter bank/digits",ranges)
        animation=bind("hn_probe_animation",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
        report=(C.c_uint32*4)()
        fresh();both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,bytes([1]))
        for case in range(40):
            both(0x0201DC52,struct.pack("<H",rng.randrange(65536)))
            both(0x02020000,b"\x8D"+struct.pack("<I",symbols["AnimationCallbackRenderGameCounterBank0"]|1))
            both(0x03007010,struct.pack("<I",0x02020000))
            equal(animation(memory,0x02010000,0x8D,0x03007010,report),1,"native counter animation")
            equal(report[3],call("AnimationInvokeCallback",0x02010000,0x03007010),"counter animation yield")
            compare("counter animation",ranges+((0x03007010,4),))
        local=random.Random(0x434F554E)
        for name,index in (("ScriptCallbackAddGameCounter",9),("ScriptCallbackSubtractGameCounter",9),
                           ("ScriptCallbackAddGameCounterBank2",10),("ScriptCallbackSubtractGameCounterBank2",10)):
            for value in (0,1,31,32,49,50,51,998,999,1000,32768,65535):
                for amount in (0,1,2,10,30,31,32,33,49,50,999,65535):
                    fresh();both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x28")
                    both(0x02010000,local.randbytes(40*104));both(0x0201DC40,local.randbytes(32))
                    both(0x0201DC40+index*2,struct.pack("<H",value));both(0x0201DC4C,struct.pack("<H",amount))
                    both(0x03001F50,bytes([local.choice((0,2))]))
                    both(0x02020000,struct.pack("<I",symbols[name]|1));both(0x03000010,b"\x1C"+bytes(15))
                    equal(command(memory,0x1C,0x02020000),1,f"native counter change {name}")
                    call("MainScriptInvokeCallback",0x02020000);compare(f"counter change {name}",ranges)
        fresh();equal(native("hn_audio_driver_init",result=C.c_bool)(memory),True,"counter audio init")
        call("InitializeAudioDriver",limit=2000000)
        both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x28")
        animated_ranges=ranges+((0x03007FF0,4),(0x04000060,0x80),(0x04000100,4),(0x03007010,4))
        for case in range(120):
            countdown=(0,1,0xFFFFFFFF,0x80000000,0x7FFFFFFF)[case%5]
            delta=(-0x80000000,-99,-1,0,1,99,0x7FFFFFFF)[case%7]
            steps=(0,1,2,31,32,0xFFFFFFFF)[case%6]
            both(0x03001130,struct.pack("<I",countdown));both(0x03001128,struct.pack("<i",delta))
            both(0x030037EC,struct.pack("<I",steps));both(0x0300112C,struct.pack("<I",case%4))
            both(0x0201DC4E,struct.pack("<H",local.randrange(65536)));both(0x03003798,struct.pack("<I",case%3))
            for tick in range(8):
                both(0x02020000,b"\x8D"+struct.pack("<I",symbols["AnimationCallbackUpdateGameCounter"]|1))
                both(0x03007010,struct.pack("<I",0x02020000))
                equal(animation(memory,0x02010000,0x8D,0x03007010,report),1,"native counter ticking")
                equal(report[3],call("AnimationInvokeCallback",0x02010000,0x03007010),"counter ticking yield")
                compare(f"counter ticking {case} {tick}",animated_ranges)

    @timed
    def actor_checks():
        ranges=((0x02000000,0x40000),(0x03000000,0x7000))
        for index in range(17):
            for pointer in (0,1,15,0x02010000,0xFFFFFFFF):
                fresh();both(0x03000050,rng.randbytes(17*84))
                both(0x03002BB0,struct.pack("<16I",*[0xFFFFFFFF if i&1 else 0x02010000+i*64 for i in range(16)]))
                both(0x02020000,struct.pack("<BI",index,pointer))
                equal(command(memory,0x26,0x02020000),1,"native actor context setup")
                call("MainScriptSetActorContext",0x02020000);compare("actor context setup",ranges)
        for index in (0,1,7,255):
            for pointer in (0,15,0x08000101,0xFFFFFFFF):
                fresh();both(0x03002BB0,rng.randbytes(64));both(0x02020000,struct.pack("<BI",index,pointer))
                equal(command(memory,0x25,0x02020000),1,"native indirect callback storage")
                call("MainScriptStoreIndirectCallback1FC0",0x02020000);compare("indirect callback storage",ranges)
        actor_frames=bind("hn_probe_actor_frames",C.c_int,C.c_void_p,C.c_uint32,C.POINTER(C.c_uint32))
        report=(C.c_uint32*3)()
        for case in range(16):
            fresh()
            equal(native("hn_runtime_tables_init",result=C.c_bool)(memory),True,"actor runtime tables")
            cpu.mem_write(0x02000000,rom[0x8FE018:0x900EC8])
            both(0x03000010,rng.randbytes(16));both(0x03002C60,struct.pack("<I",0x02020000))
            both(0x03002BF0,bytes([case]));both(0x03002C10,rng.randbytes(64));both(0x03001FBC,rng.randbytes(2))
            both(0x03000050,rng.randbytes(17*84))
            for index in range(17):both(0x03000050+index*84,bytes(4))
            for index in range(1,17):
                if case==0 or (index+case)%4==0:continue
                base=0x02010000+index*64
                program=b"\x2B"+struct.pack("<H",index)+b"\x2D"+struct.pack("<HB",0x100+index,index)
                program+=b"\x1E"+struct.pack("<I",base+24)+b"\x02"+struct.pack("<H",1+case%4)+b"\x27"
                both(base,program.ljust(24,b"\0")+b"\x2B"+struct.pack("<H",index^0xBEEF)+b"\x1F")
                both(0x02020000,struct.pack("<BI",index,base))
                equal(command(memory,0x26,0x02020000),1,"native actor fixture setup")
                call("MainScriptSetActorContext",0x02020000)
            for frame_index in range(8):
                equal(actor_frames(memory,4096,report),1,f"native actor frames {case}/{frame_index}")
                call("RunActorScriptFrames",limit=2000000);compare("actor frame context switch/restore",ranges)
        fresh();equal(native("hn_runtime_tables_init",result=C.c_bool)(memory),True,"actor runtime tables")
        cpu.mem_write(0x02000000,rom[0x8FE018:0x900EC8])
        both(0x02010000,b"\x26\x10"+struct.pack("<I",0x02010100)+b"\x27")
        both(0x02010100,b"\x2B\xEF\xBE\x27")
        both(0x02020000,struct.pack("<BI",1,0x02010000))
        equal(command(memory,0x26,0x02020000),1,"native first actor setup")
        call("MainScriptSetActorContext",0x02020000)
        for frame_index in range(3):
            equal(actor_frames(memory,4096,report),1,"native same-frame actor activation")
            call("RunActorScriptFrames",limit=2000000);compare("same-frame actor activation",ranges)

    @timed
    def motion_checks():
        ranges=((0x02000000,0x40000),(0x03000000,0x7000))
        query=bind("hn_motion_route_obstacle",C.c_bool,C.c_void_p,*([C.c_uint32]*4),C.POINTER(C.c_uint32))
        result=C.c_uint32()
        def prepare(case):
            fresh()
            width,height=8+case%33,8+case%27
            both(0x02000138,struct.pack("<HH",width,height));both(0x03002BFC,struct.pack("<I",0x02010000))
            cells=[(0x1000 if rng.randrange(23)==0 else 0)|rng.randrange(4096) for _ in range(width*height)]
            both(0x02010000,struct.pack(f"<{len(cells)}H",*cells))
            both(0x03002D98,struct.pack("<I",0x02017000));both(0x03002DC0,bytes([32]))
            both(0x02017000,rng.randbytes(32*104));both(0x03002C0C,b"\x0A");both(0x03001F50,b"\x0D")
            both(0x03002A90,rng.randbytes(32))
        for case in range(400):
            prepare(case)
            coordinates=[rng.randrange(256) if case%5 else rng.getrandbits(32) for _ in range(4)]
            equal(query(memory,*coordinates,C.byref(result)),True,"native route obstacle query")
            equal(result.value,call("HasRenderXZRouteObstacle",*coordinates),f"route obstacle {case}")
            compare("route obstacle memory",ranges)
        for case in range(120):
            prepare(case)
            coordinates=[((rng.randrange(-64,256)<<16)|rng.randrange(65536))&0xFFFFFFFF for _ in range(4)]
            if case%4==0:coordinates[2]=coordinates[0]
            if case%4==1:coordinates[3]=coordinates[1]
            if case>=110:coordinates=[rng.getrandbits(32) for _ in range(4)]
            for slot,values in ((10,coordinates[:2]),(13,coordinates[2:])):
                both(0x02017000+slot*104+0x18,struct.pack("<I",values[0]))
                both(0x02017000+slot*104+0x20,struct.pack("<I",values[1]))
            equal(native("hn_motion_rebuild_route",result=C.c_bool)(memory),True,"native follower route rebuild")
            call("RebuildFollowerRenderXZRoute");compare("follower route",ranges)
        for case in range(80):
            prepare(case)
            both(0x030034B0,rng.randbytes(48));both(0x030021F0,rng.randbytes(128))
            both(0x0201EE60,rng.randbytes(0x1200));both(0x02012000,rng.randbytes(128))
            source=0x0201EE60 if case%3==0 else 0x02011000
            if case%3==1:both(source,b"\x11\x10\0\0\0ABCDEFGH\0IJKLMNOP")
            elif case%3==2:both(source,b"\0"+rng.randbytes(0x11FF))
            parameters=0x02012000 if case&1 else 0
            values=[rng.getrandbits(32) for _ in range(7)]+[source,parameters]
            equal(native("hn_motion_configure",9,C.c_bool)(memory,*values),True,"native collision scene setup")
            call("ConfigureRenderXZCollisionScene",*values,limit=1000000);compare("collision scene setup",ranges)
        for case in range(64):
            prepare(case)
            both(0x0201EE60,bytes(0x1200));both(0x02012000,rng.randbytes(128))
            both(0x03002BB0,struct.pack("<16I",0x0201EE60,0x02012000,*([0]*14)))
            flags=(case*37)&255
            payload=bytes([10,13,case,255-case,flags])+struct.pack("<HHII",256,512,0 if case&1 else 0x0201EE60,1 if case&2 else 0x02012000)
            both(0x02020000,payload)
            equal(command(memory,0x20,0x02020000),1,"native Main collision configuration")
            call("MainScriptConfigure065D8",0x02020000,limit=1000000);compare("Main collision configuration",ranges)
            equal(command(memory,0x3D,0x02020000),1,"native Main follower route")
            call("MainScriptRebuildFollowerRoute",0x02020000);compare("Main follower route",ranges)

    @timed
    def motion_live_checks():
        ranges=((0x02000000,0x40000),(0x03000000,0x7000))
        steps=bind("hn_probe_motion_steps",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
        result=C.c_uint32();leader,follower=0x02017000+10*104,0x02017000+13*104
        def fixture(case):
            fresh()
            both(0x03002D98,struct.pack("<I",0x02017000));both(0x03002DC0,b"\x20")
            both(0x02017000,rng.randbytes(32*104));both(0x03002C0C,b"\x0A");both(0x03001F50,b"\x0D")
            both(0x03002A90,rng.randbytes(256));both(0x03001F54,bytes([case%32]));both(0x03002BF8,bytes([case*7%32]))
            both(0x03001F60,bytes([case%2]));both(0x03001F90,bytes([case//2%2]));both(0x03002270,bytes([11 if case%3 else 0]))
            both(follower+0x60,bytes([case%8+1]));both(0x03000786,bytes([(0,1,2,3,255)[case%5]]))
            both(0x03000780,struct.pack("<i",case%9-3));both(0x03000788,rng.randbytes(12))
            both(0x02000138,struct.pack("<HH",64,64));both(0x03002BFC,struct.pack("<I",0x02010000))
            both(0x02010000,struct.pack("<4096H",*[(rng.randrange(4096)|(0x1000 if rng.randrange(97)==0 else 0)) for _ in range(4096)]))
            for slot in (leader,follower):
                for field in (0x18,0x20):both(slot+field,struct.pack("<I",rng.getrandbits(32)))
        for case in range(1024):
            fixture(case)
            velocities=(0,1,65535,65536,0xFFFF0000,0xFFFE0000,0xFFFD0000,0x20000,0x80000000,0x7FFFFFFF)
            x,z=velocities[case%10],velocities[case//10%10]
            if case>=1000:x,z=rng.getrandbits(32),rng.getrandbits(32)
            for slot in (leader,follower):
                both(slot+0x24,struct.pack("<I",x));both(slot+0x2C,struct.pack("<I",z))
            equal(native("hn_motion_direction",1,C.c_uint32)(memory,leader),call("GetRenderXZMotionDirection",leader),"motion direction")
            equal(native("hn_motion_classify",1,C.c_uint32)(memory,leader),call("ClassifyLeaderRenderXZMotion"),"motion classification")
            equal(steps(memory,0x02023000,0x02023002,C.byref(result)),1,"native follower step quantization")
            equal(result.value,call("QuantizeFollowerRenderXZSteps",0x02023000,0x02023002),"quantized follower class")
            compare("motion directions/quantization",ranges)
        for case in range(320):
            fixture(case);point=0x03002A90+(case*7%32)*8
            values=[rng.randrange(-32768,32768) for _ in range(4)]
            x,z=(rng.getrandbits(32) for _ in range(2))
            if case%5==0:x=((values[0]&65535)<<16)|rng.randrange(65536)
            if case%5==1:z=((values[1]&65535)<<16)|rng.randrange(65536)
            if case%5==2:
                x=((values[0]+values[2])&65535)<<16;z=((values[1]+values[3])&65535)<<16
            if case%5==3:
                values=[5,-7,8,4];x=9<<16;z=(-5<<16)&0xFFFFFFFF
            both(point,struct.pack("<4h",*values));both(leader+0x18,struct.pack("<I",x));both(leader+0x20,struct.pack("<I",z))
            equal(native("hn_motion_extend_route",result=C.c_bool)(memory),True,"native follower route extension")
            call("UpdateLeaderFollowerRouteSegment");compare(f"follower route extension {case}",ranges)
        for case in range(320):
            fixture(case);point=0x03002A90+(case%32)*8
            origin=(rng.randrange(-256,257),rng.randrange(-256,257));delta=(0,0) if case%8==0 else (rng.randrange(-8,9),rng.randrange(-8,9))
            if case%8==1:delta=(0,-2)
            if case%8==2:delta=(-2,0)
            both(point,struct.pack("<4h",*origin,*delta))
            for slot in (leader,follower):
                for axis,value in zip((0x18,0x20),origin):
                    both(slot+axis,struct.pack("<I",((value+rng.randrange(-80,81))<<16|rng.randrange(65536))&0xFFFFFFFF))
            if case%8 in (3,4):
                both(follower+0x18,native_bytes(leader+0x18,4));both(follower+0x20,native_bytes(leader+0x20,4))
            for tick in range(4):
                equal(native("hn_motion_follower",result=C.c_bool)(memory),True,f"native live follower {case}/{tick}")
                equal(call("UpdateFollowerRenderXZMotion"),0,"follower result")
                compare(f"live follower {case}/{tick}",ranges)
            both(0x03002170,bytes([case%2]));both(0x03002183,bytes([case//2%2]))
            equal(native("hn_motion_update_following",result=C.c_bool)(memory),True,"native gated following")
            call("UpdateRenderXZMotion");compare("gated follower integration",ranges)

    @timed
    def collision_checks():
        ranges=((0x02000000,0x40000),(0x03000000,0x7000))
        output=C.c_uint32();record=0x02023000;leader=0x02017000+10*104;obstacle=0x02017000+11*104
        contains=bind("hn_collision_contains",C.c_bool,C.c_void_p,C.c_uint32,C.c_int32,C.c_int32,C.POINTER(C.c_uint32))
        query=bind("hn_collision_query",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
        has=bind("hn_collision_has_record",C.c_bool,C.c_void_p,C.c_int32,C.c_int32,C.POINTER(C.c_uint32))
        moving=bind("hn_probe_motion_leader",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
        def fixture(case,shape=0):
            fresh()
            both(0x03002D98,struct.pack("<I",0x02017000));both(0x03002DC0,b"\x20");both(0x02017000,rng.randbytes(33*104))
            both(0x03002C0C,b"\x0A");both(0x03001F50,bytes([13 if case%3 else 0]))
            both(0x03001F60,bytes([case%2]));both(0x03001F90,bytes([case//2%2]));both(0x03002270,b"\x0E");both(0x03002274,b"\x0F")
            both(leader+0x60,bytes([case%4+1]));both(0x02017000+13*104+0x60,bytes([case%4+1]))
            both(0x02000138,struct.pack("<HH",64,64));both(0x03002BFC,struct.pack("<I",0x02010000))
            both(0x02010000,struct.pack("<4096H",*[rng.randrange(65536) for _ in range(4096)]))
            both(0x030021F0,bytes(128));both(0x03002170,bytes([case%2]));both(0x03000784,bytes([case%4]))
            both(0x03000785,bytes([8 if case%2 else 0]));both(0x03002C50,bytes([case%4]))
            both(0x03001FA8,bytes([case&15]));both(0x03001F8C,bytes([case%5]));both(0x03002174,bytes([case]))
            both(0x03002A90,bytes(256));both(0x03001F54,b"\0");both(0x03002BF8,b"\0")
            both(0x03000780,struct.pack("<i",case%4));both(0x030007A8,bytes([case%9]))
            for index,slot in enumerate((leader,obstacle,0x02017000+13*104)):
                both(slot+0x18,struct.pack("<I",(100+index*30)*65536+case*137%65536))
                both(slot+0x20,struct.pack("<I",(100+index*20)*65536+case*251%65536))
                both(slot+0x24,struct.pack("<I",((case%5-2)*65536)&0xFFFFFFFF))
                both(slot+0x2C,struct.pack("<I",((case%3-1)*65536)&0xFFFFFFFF))
            both(record,struct.pack("<8H",11,case*173&65535,shape,case%4,case%33,case%27,(0,1,24,64,255,65535)[case%6],(1,24,64,65535)[case%4]))
        fresh()
        for direction in range(4):
            for mask in range(256):
                equal(bind("hn_collision_response_direction",C.c_uint32,C.c_uint32,C.c_uint32)(mask,direction),call("ClassifyRenderCollisionMask",mask,direction),"collision-mask response")
        for case in range(128):
            fixture(case)
            x,z=rng.randrange(-16,512),rng.randrange(-16,512)
            for direction in range(4):
                for sample in (0,1,2,3,4,5,255):
                    values=(0x02010000,x&0xFFFFFFFF,z&0xFFFFFFFF,direction,sample)
                    equal(native("hn_collision_sample",5,C.c_uint32)(memory,*values),call("GetRenderCollisionSample",*values),"collision sample pointer")
                values=(0x02010000,x&0xFFFFFFFF,z&0xFFFFFFFF,direction)
                equal(native("hn_collision_ahead",4,C.c_uint32)(memory,*values),call("GetRenderDirectionalCollisionSample",*values),"directional collision sample")
            equal(native("hn_collision_mask",3,C.c_uint32)(memory,0x02010000,x&0xFFFFFFFF,z&0xFFFFFFFF),call("GetRenderXZCollisionMask",0x02010000,x&0xFFFFFFFF,z&0xFFFFFFFF),"collision edge mask")
            compare("collision sampling effects",ranges)
        response_hits=[0,0]
        for shape in (0,1):
            for case in range(192):
                fixture(case,shape)
                x,z=rng.randrange(-32768,65536),rng.randrange(-32768,65536)
                equal(contains(memory,record,x,z,C.byref(output)),True,"native record containment")
                equal(output.value,call("IsRenderXZWithinRecordRectangle" if shape else "IsRenderXZWithinRecordCircle",record,x&0xFFFFFFFF,z&0xFFFFFFFF),"record containment")
                equal(query(memory,10,record,C.byref(output)),True,"native record interaction")
                equal(output.value,call("QueryRenderXZRectangleRecord" if shape else "QueryRenderXZCircleRecord",10,record),"record interaction")
                if case%4<2:
                    both(leader+0x18,struct.pack("<I",(130-16+case%8)*65536+32768));both(leader+0x20,struct.pack("<I",(120-24+case%7)*65536+32768))
                    both(leader+0x24,bytes(4));both(leader+0x2C,bytes(4))
                both(0x03001FA8,b"\0")
                equal(native("hn_collision_response",2,C.c_bool)(memory,10,record),True,"native record response")
                call("ResolveRenderXZRecordRectangle" if shape else "ResolveRenderXZRecordCircle",10,record);compare("record response",ranges)
                response_hits[shape]+=bool(native_bytes(0x03001FA8,1)[0]&8)
                both(0x030021F0,struct.pack("<2I",record,0));both(record+6,b"\x02\0")
                equal(has(memory,x,z,C.byref(output)),True,"native record collision lookup")
                equal(output.value,call("HasRenderXZRecordCollision",x&0xFFFFFFFF,z&0xFFFFFFFF),"record collision lookup")
                for resolve,name in ((True,"UpdateRenderXZRecordCollision"),(False,"ProcessCoreRenderCollisionRecords")):
                    both(record+6,struct.pack("<H",2 if resolve else case%2))
                    equal(native("hn_collision_records",3,C.c_bool)(memory,case,10,resolve),True,"native collision records")
                    call(name,case,10);compare("collision record list",ranges)
        equal(response_hits[0]>0,True,"circle response exercised");equal(response_hits[1]>0,True,"rectangle response exercised")
        if args.bus_oracle:
            # Independent mGBA bus loads supply the original LDRH results.
            # The three reviewed ROM sites all stop before reading flags.
            sites=(0x08007D40,0x08007EC8,0x08008274)
            addresses=(0x10000000,0x20001235,0xFFFFFFFE,0xFFFFFFFF)
            requests=[]
            for site in sites:
                offset=site-0x08000000
                previous=int.from_bytes(rom[offset-2:offset],"little")
                prefetched=int.from_bytes(rom[offset:offset+2],"little")
                requests.extend((address,previous,prefetched) for address in addresses)
            environment=os.environ.copy()
            if args.oracle_runtime_dir:environment["PATH"]=str(args.oracle_runtime_dir)+os.pathsep+environment.get("PATH","")
            reply=subprocess.run([str(args.bus_oracle.resolve())],input=b"".join(struct.pack("<3I",*r) for r in requests),
                                 stdout=subprocess.PIPE,check=True,env=environment,timeout=30).stdout
            equal(len(reply),len(requests)*4,"complete floating-bus oracle response")
            values=struct.unpack(f"<{len(requests)}I",reply)
            fixture(0);both(record+6,b"\0\0");both(obstacle+0x60,b"\0")
            for site_index,site in enumerate(sites):
                for address_index,address in enumerate(addresses):
                    expected=values[site_index*len(addresses)+address_index]
                    equal(expected>255,True,"ROM-prefetched index terminates every u8 render count")
                    for count in range(256):
                        both(0x03002DC0,bytes([count]))
                        for first in (0,1):
                            both(0x030021F0,struct.pack("<3I",record if first else address,address if first else 0,0))
                            if site_index==0:
                                equal(has(memory,100,100,C.byref(output)),True,"native floating-bus collision lookup")
                                equal(output.value,0,"floating-bus lookup has no hit")
                            else:
                                equal(native("hn_collision_records",3,C.c_bool)(memory,0,10,site_index==1),True,"native floating-bus record scan")
                            compare("floating-bus terminating record preserves RAM",ranges)
            print("6144 collision-list scans matched independent floating-bus termination, including every render count.")
        else:
            print("Floating-bus collision checks skipped; supply --bus-oracle to include them.")
        for case in range(192):
            fixture(case,case%2)
            if case%3:both(0x030021F0,struct.pack("<2I",record,0))
            for fn,name in (("hn_collision_flags","UpdateRenderXZCollisionFlags"),("hn_collision_scripts","UpdateRenderXZCollisionScript")):
                equal(native(fn,2,C.c_bool)(memory,case,10),True,f"native {name}")
                call(name,case,10);compare(name,ranges)
        for case in range(256):
            fixture(case)
            both(0x03000822,struct.pack("<H",case*37&1023));both(0x030021E0,bytes([case&15]))
            native("hn_motion_control")(memory);call("UpdateCoreMotionControl");compare("motion action controls",ranges)
            native("hn_motion_reset")(memory);call("ResetCoreMotionFlags");compare("motion reset",ranges)
        for case in range(192):
            fixture(case,case%2)
            both(0x030021E0,bytes([case&15]));both(0x03000820,struct.pack("<H",case*13&255));both(0x03000822,struct.pack("<H",case*37&1023))
            both(0x02024000,bytes([case&128,case%2]))
            if case%4==0:both(0x030021F0,struct.pack("<2I",record,0))
            equal(moving(memory,0x02024000,0x02024001,C.byref(output)),1,"native leader motion")
            equal(output.value,call("UpdateLeaderRenderXZMotion",0x02024000,0x02024001),"leader motion mode")
            compare(f"leader motion {case}",ranges)
            equal(native("hn_motion_advance",result=C.c_bool)(memory),True,"native complete movement update")
            call("AdvanceCoreRenderMotion");compare(f"core motion {case}",ranges)
            both(0x03000018,bytes([case%2]));both(0x03000010,b"\x21")
            equal(command(memory,0x21,0x02025000),1,"native Main movement wait")
            call("MainScriptWaitForByte1FA8",0x02025000);compare(f"Main movement wait {case}",ranges)

    @timed
    def task_checks():
        ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x06000000,0x18000),(0x040000D8,8))
        fresh();heap_init(memory);call("InitializeHeap",0)
        native("hn_task_contexts_init")(memory);call("InitializeTaskContexts");compare("task contexts init",ranges[:2])
        active=[]
        for index in range(16):
            task=native("hn_task_allocate",result=C.c_uint32)(memory)
            equal(task,call("AllocateTaskContext"),"task allocation")
            active.append(task);compare("task allocation/list",ranges[:2])
        rng.shuffle(active)
        for task in active:
            equal(native("hn_task_remove",1,C.c_bool)(memory,task),True,"native task removal")
            call("RemoveTaskContext",task);compare("task removal/list",ranges[:2])
        dimensions=((1,2),(7,3),(28,4),(32,6),(28,8),(255,2),(1,255))
        for case in range(56):
            fresh();task=0x02020000;resource=0x02023000
            both(task,rng.randbytes(52));both(0x0200C720,rng.randbytes(0x4000))
            both(0x02011000,rng.randbytes(0x4000));both(0x06008000,rng.randbytes(0x4000))
            width,height=dimensions[case%len(dimensions)]
            values=(task,width+(case<<8),height+(case<<16),rng.getrandbits(32),rng.getrandbits(32),
                    rng.getrandbits(32),rng.getrandbits(32),(0,8,16,24,255)[case%5],0x06008000,0x02011000)
            equal(native("hn_task_graphics",10,C.c_bool)(memory,*values),True,"native task graphics constructor")
            call("InitializeTaskContextGraphics",*values,limit=2000000);compare("task graphics constructor",ranges)
            both(0x03002D98,struct.pack("<I",0x02017000));both(0x03002DC0,bytes([12]))
            both(0x02017000,rng.randbytes(12*104));both(resource,rng.randbytes(44))
            both(task+0x30,struct.pack("<I",resource if case%3 else 0));both(task+0xA,bytes([case&31]))
            both(0x03002170,b"\xFF");both(0x03001F50,bytes([case%8]))
            for clip,name in ((True,"ScrollTaskRenderSlotsUp"),(False,"StepTaskRenderSlotsUp")):
                equal(native("hn_task_render_scroll",2,C.c_bool)(memory,task,clip),True,"native task render scroll")
                call(name,task);compare(name,ranges[:2])
            equal(native("hn_task_render_disable",1,C.c_bool)(memory,task),True,"native task render disable")
            call("DisableTaskRenderSlots",task);compare("task render disable",ranges[:2])
            equal(native("hn_task_indicator",1,C.c_bool)(memory,task),True,"native task indicator")
            call("InitializeTaskRenderSlot0",task);compare("task indicator",ranges[:2])
            both(task,bytes([0x83 if case&1 else 3]));both(task+5,bytes([case%3]))
            both(task+0x1E,struct.pack("<H",rng.randrange(65536)));both(0x03000645,b"\xFF")
            equal(native("hn_task_row_transfer",1,C.c_bool)(memory,task),True,"native task row transfer")
            call("QueueTaskTextRowTransfer",task);compare("task row transfer",ranges[:2])
        measure=bind("hn_task_text_measure",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint8,C.c_bool,C.POINTER(C.c_uint32))
        result=C.c_uint32()
        fresh();native("hn_task_contexts_init")(memory);call("InitializeTaskContexts")
        for case in range(160):
            both(0x03000646,bytes([case%3]));both(0x0200460B,bytes([case%5]));both(0x02004529,bytes([case%4]))
            ordinary=bytes(rng.randrange(1,0xE0) for _ in range(case%16))
            if case<32:program=ordinary+bytes([0xE0+case%5])
            else:
                opcode=(0xE6,0xE7,0xF0,0xF1,0xF6,0xF7,0xF8,0xFA,0xFB,0xFC,0xFD,0xFE)[case%12]
                index=255 if case%7==0 else case%4
                program=ordinary+bytes([opcode])+(bytes([index]) if opcode>=0xF0 else b"")+b"\xE1"
            both(0x02010000,program+bytes(16))
            for items in (True,False):
                equal(measure(memory,0x02010000,case%3,items,C.byref(result)),True,"native task text measure")
                expected=call("CountTaskTextItems",0x02010000,limit=2000000) if items else call("MeasureTaskTextWidth",0x02010000,case%3,limit=2000000)
                equal(result.value,expected,f"task text measure {case}/{items}");compare("task text measure effects",ranges[:2])
            both(0x02020000,rng.randbytes(52));both(0x02020001,bytes([1+case%32]))
            both(0x02020004,bytes([(0,1,2,0x80,0x8C)[case%5]]));both(0x02020010,struct.pack("<I",0x02010000))
            offset=(0,254,255,256,65535,65536)[case%6]
            equal(native("hn_task_text_offset",2,C.c_bool)(memory,0x02020000,offset),True,"native task text offset")
            call("SetTaskTextHorizontalOffset",0x02020000,offset,limit=2000000);compare("task text offset",ranges[:2])
            both(0x02020003,bytes([255 if case&1 else 0]));both(0x0202001C,struct.pack("<H",case%4*100))
            equal(native("hn_task_set_script",2,C.c_bool)(memory,0x02020000,0x02010000),True,"native task script")
            call("SetTaskContextScript",0x02020000,0x02010000,limit=2000000);compare("task script/offset",ranges[:2])
            equal(native("hn_task_text_adjust",2,C.c_uint32)(memory,0x02010000,case%4),call("CopyTaskTextAdjustedInitial",0x02010000,case%4),"adjusted text pointer")
            compare("adjusted text buffer",ranges[:2])

        for case in range(24):
            fresh();heap_init(memory);call("InitializeHeap",0)
            native("hn_task_contexts_init")(memory);call("InitializeTaskContexts")
            both(0x03002E08,struct.pack("<I",0x02018000));both(0x03002C80,bytes([4]))
            both(0x03003070,struct.pack("<I",0x02019000));both(0x03002E10,bytes([4]))
            both(0x03002D98,struct.pack("<I",0x02017000));both(0x03002DC0,bytes([12]))
            both(0x0200460B,bytes([case%5]));both(0x02021000,b"\x01\x02\x03\xE1"+bytes(32))
            both(0x02022000,struct.pack("<5I",*([0x02021000]*5)))
            both(0x03002BB0,struct.pack("<I",0x02022000))
            slot=case%4
            def main_task(opcode,name,payload):
                both(0x02020000,payload)
                equal(command(memory,opcode,0x02020000),1,f"native task Main {name}")
                call(name,0x02020000,limit=2000000);compare(name,ranges)
            main_task(0x38,"MainScriptSetTaskResourceIndex",bytes([slot,case%4]))
            both(0x02018000+slot*80+0x38,b"\x01")
            main_task(0x19,"MainScriptSetTaskContextEnabled",bytes([slot,1]))
            both(0x02018000+slot*80+0x38,b"\0")
            main_task(0x19,"MainScriptSetTaskContextEnabled",bytes([slot,1]))
            main_task(0x54,"MainScriptSetTaskField5",bytes([slot,case]))
            both(0x03000018,b"\0")
            main_task(0x1A,"MainScriptConfigureTaskContextAndScript",bytes([slot])+struct.pack("<I",0))
            main_task(0x1A,"MainScriptConfigureTaskContextAndScript",bytes([slot])+struct.pack("<I",0))
            task=struct.unpack("<I",native_bytes(0x030005F0+slot*4,4))[0]
            both(task,b"\x01")
            main_task(0x1A,"MainScriptConfigureTaskContextAndScript",bytes([slot])+struct.pack("<I",0))
            both(0x03000018,b"\0")
            main_task(0x1B,"MainScriptSetTaskContextScript",bytes([slot])+struct.pack("<I",0))
            both(task,b"\x01")
            main_task(0x1B,"MainScriptSetTaskContextScript",bytes([slot])+struct.pack("<I",0))
            both(0x03000018,b"\0")
            main_task(0x19,"MainScriptSetTaskContextEnabled",bytes([slot,0]))
            main_task(0x19,"MainScriptSetTaskContextEnabled",bytes([slot,0]))

        task_command=bind("hn_probe_task_command",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
        tasks_update=bind("hn_probe_tasks",C.c_int,C.c_void_p,C.c_uint32,C.POINTER(C.c_uint32))
        report=(C.c_uint32*4)()
        task,script,resource=0x02020000,0x02018000,0x02023000
        def task_fixture(case, audio=False):
            fresh();heap_init(memory);call("InitializeHeap",0)
            if audio:
                equal(native("hn_audio_driver_init",result=C.c_bool)(memory),True,"task acknowledgement audio")
                call("InitializeAudioDriver",limit=2000000)
            native("hn_input_initialize")(memory);call("InitializeKeys")
            both(0x03002D98,struct.pack("<I",0x02017000));both(0x03002DC0,bytes([32]))
            both(0x02017000,rng.randbytes(32*104))
            both(0x03003070,struct.pack("<I",0x02019000));both(0x03002E10,bytes([8]))
            both(0x02019000,rng.randbytes(8*44))
            both(resource,rng.randbytes(44));both(resource+0x28,struct.pack("<I",0x02016000))
            both(task,bytes(52));both(task+0x30,struct.pack("<I",resource if case%3 else 0))
            both(0x0200C720,rng.randbytes(2048));both(0x02010000,rng.randbytes(512))
            both(0x06008000,rng.randbytes(2048))
            values=(task,(1,7,28,32)[case%4],(2,4,6,8)[case%4],case%13,(0,1,0x8C)[case%3],case%3,case%4,case%2*8,0x06008000,0x02010000)
            equal(native("hn_task_graphics",10,C.c_bool)(memory,*values),True,"task fixture graphics")
            call("InitializeTaskContextGraphics",*values,limit=2000000)
            both(task,b"\x03");both(task+0x10,struct.pack("<I",script));both(task+0x2C,struct.pack("<I",0x03000610))
            both(0x03000608,struct.pack("<I",task));both(0x03000645,bytes([case&1]))
            both(0x0200460B,bytes([case%5]));both(0x03000646,bytes([case%3]))
            both(0x03001F50,bytes([(16,17,31)[case%3]]));both(0x03002170,b"\xFF")

        for case in range(48):
            task_fixture(case)
            both(task+7,bytes([case*5&255]));both(task+0x1E,struct.pack("<H",(0,1,2,7,255,65535)[case%6]))
            for complete,name in ((False,"AdvanceTaskTextMapRows"),(True,"CompleteTaskTextMapScroll")):
                equal(native("hn_task_scroll_map",2,C.c_bool)(memory,task,complete),True,"native task map scroll")
                call(name,task,limit=2000000);compare(f"task map scroll {case}/{complete}",ranges)

        for opcode in (*range(0xE0,0xF4),*range(0xF5,0x100)):
            for variant in range(8):
                case=(opcode-0xE0)*8+variant;keys=3 if variant&1 else 0
                task_fixture(case,audio=bool(keys))
                both(script,bytes([variant%4,0xE1])+bytes(16))
                both(task+0x14,struct.pack("<I",script+8 if variant&2 else 0))
                both(task+0x1E,struct.pack("<H",variant%4));both(task+8,bytes([variant%2]))
                for index in range(5):both(0x02017000+(index+3)*104+0x60,bytes([int(index<variant%6)]))
                both(0x02004520,bytes([variant*31&255])*64);both(0x0201DC40,struct.pack("<H",variant*9362)*16)
                for index in range(16):both(0x03002BB0+index*4,struct.pack("<I",0x02024000))
                both(0x02024000,struct.pack("<5I",*([script+8]*5)))
                both(0x03001FB0,struct.pack("<H",keys));both(0x03001FAC,bytes([variant%32]))
                target=0x02017000+(variant%32)*104
                both(0x03001F7C,struct.pack("<I",struct.unpack("<I",native_bytes(target+4,4))[0] if variant&4 else 0))
                equal(task_command(memory,task,opcode,report),1,f"native task command {opcode:02X}/{variant} report {tuple(report)}")
                names=("TaskCommandCallSecondFlags","TaskCommandCallFirstFlags","TaskCommandSetFill3","TaskCommandSetFill2",
                       "TaskCommandSetFill1","TaskCommandSetDelay0","TaskCommandSetDelay6","TaskCommandSetDelay2")
                name=names[opcode-0xE6] if 0xE6<=opcode<=0xED else f"TaskCommand{opcode:02X}"
                equal(report[3],call(name,task,limit=2000000),f"task command continuation {opcode:02X}/{variant}")
                compare(f"task command {opcode:02X}/{variant}",ranges)

        fresh()
        for format_value in range(256):
            for value in (0,9,10,99,100,9999,65535,rng.getrandbits(32)):
                both(0x03000648,rng.randbytes(128))
                native("hn_task_text_number",2)(memory,format_value,value)
                call("FormatTaskNumericText",format_value,value)
                compare(f"numeric task text {format_value}/{value}",ranges[:2])
        for selected in range(3):
            for variant in range(3):
                for group in range(5):
                    family=struct.unpack("<I",native_bytes(0x0858CFD4+group*4,4))[0]
                    table=struct.unpack("<I",native_bytes(family+variant*4,4))[0]
                    for index in range(8):
                        bundle=native_bytes(table+index*4,4)
                        if struct.unpack("<I",bundle)[0]==0x0858C6B0:break
                        both(0x03000646,bytes([selected]));both(0x03002BB0,bundle)
                        equal(native("hn_task_text_retarget",1,C.c_bool)(memory,0x03002BB0),True,"native task variant retarget")
                        call("RetargetTaskTextBundle",0x03002BB0)
                        compare("task text variant retarget",ranges[:2])

        callbacks=[row for row in entries if row["source"]=="src/scripts/script_callbacks_slot16_presets.c"
                   and row["name"]!="ScriptCallbackSetBackground2RightBottom1024"]
        for callback in callbacks:
            for variant in range(3):
                task_fixture(variant)
                both(script,b"\x07\xE1");both(0x03001FC0+7*4,struct.pack("<I",callback["address"]|1))
                equal(task_command(memory,task,0xF4,report),1,f"native typed task callback {callback['name']}")
                equal(report[3],call("TaskCommandF4",task,limit=2000000),"task callback continuation")
                compare(f"typed task callback {callback['name']}/{variant}",ranges)

        programs=(b"\x01\x02\x03\xE1",b"\x01\xE4\x02\xE4\x03\xE4\x04\xE1",
                  b"\xE6\x01\xE7\xE1",b"\xFF\xF6\x00\xE1",b"\x5E\xF6\x01\xE1",
                  b"\xEC\x01\x02\xED\x03\xEB\x04\xE1",b"\xE8\x01\xE9\x02\xEA\x03\xE1")
        for case in range(56):
            task_fixture(case)
            both(script,programs[case%len(programs)]+bytes(16))
            both(0x03001F70,b"\x01\x02\xE1"+bytes(6));both(0x03002BA0,b"\x03\x04\xE1"+bytes(6))
            both(task+0xB,bytes([(0,1,4,8,12)[case%5]]))
            for frame in range(32):
                held=2 if case&1 and frame%4==0 else 0
                equal(native("hn_input_update",1,C.c_bool)(memory,held),True,"task frame input")
                both(0x04000130,struct.pack("<H",(~held)&1023));call("ReadKeys")
                equal(tasks_update(memory,4096,report),1,f"native task update {case}/{frame} report {tuple(report)}")
                call("UpdateTaskContexts",limit=4000000);compare(f"task update {case}/{frame}",ranges)

    @timed
    def choice_checks():
        for opcode,name in ((0x21,"MainScriptWaitForByte1FA8"),(0x24,"MainScriptAwaitWordChoice"),(0x45,"MainScriptConfigureWordChoice")):
            equal(struct.unpack_from("<I",rom,0x58E5D4+opcode*4)[0]&~1,symbols[name],f"original Main dispatch {opcode:02X}")
        ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x05000000,0x400),
                (0x06000000,0x18000),(0x04000060,0x74),(0x040000D8,8))
        dialog=bind("hn_choice_update",C.c_bool,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
        task_frames=bind("hn_probe_tasks",C.c_int,C.c_void_p,C.c_uint32,C.POINTER(C.c_uint32))
        task_report=(C.c_uint32*3)()
        selected=C.c_uint32()
        def fixture(case):
            fresh();heap_init(memory);call("InitializeHeap",0)
            native("hn_task_contexts_init")(memory);call("InitializeTaskContexts")
            equal(native("hn_audio_driver_init",result=C.c_bool)(memory),True,"choice audio initialization")
            call("InitializeAudioDriver",limit=2000000)
            native("hn_input_initialize")(memory);call("InitializeKeys")
            both(0x03002D98,struct.pack("<I",0x02017000));both(0x03002DC0,bytes([32]))
            both(0x02017000,rng.randbytes(32*104))
            both(0x03002E08,struct.pack("<I",0x02018000));both(0x03002C80,bytes([9]))
            both(0x02018000,rng.randbytes(9*80));both(0x02018038,b"\0")
            both(0x03003070,struct.pack("<I",0x02019000));both(0x03002E10,bytes([8]))
            both(0x02019000,rng.randbytes(8*44));both(0x0200FF20,rng.randbytes(512))
            both(0x0200460B,bytes([case%5]));both(0x03002160,b"\0")
            both(0x03001F58,bytes([(0,8,32,40)[case%4]]));both(0x030007A4,struct.pack("<I",case%9+1))
            both(0x03002C0C,b"\x0A");both(0x03001F50,b"\x0B")
            both(0x02017000+10*104+0x60,bytes([case%8+1]));both(0x02017000+11*104+0x60,bytes([case%8+1]))
            both(0x03001F60,bytes([case%2]));both(0x03001F90,bytes([case%2]));both(0x03002270,b"\x0C");both(0x03002274,b"\x0D")
            both(0x02020000,bytes([(0,1,86,87,90)[case%5]])*9)
            both(0x02021000,struct.pack("<9H",*([0xBE01]*9)))
            both(0x030021E4,struct.pack("<H",case*71&511))
        def update(count,label):
            equal(dialog(memory,count,0x02020000,0x02021000,C.byref(selected)),True,f"native choice {label}")
            equal(selected.value,call("UpdateWordChoiceDialog",count,0x02020000,0x02021000,limit=6000000),f"choice result {label}")
            compare(f"choice dialog {label}",ranges)
        for count in (10,16,17,31,32,33,64,127,255):
            for variant in range(4):
                fixture(variant);enabled=4 if count==10 else 9
                both(0x02020000,bytes([1])*count)
                both(0x02021000,struct.pack("<"+"H"*count,*([0xBE00]*(count-enabled)+[0xBE01]*enabled)))
                both(0x030021E4,struct.pack("<H",0xA55A))
                update(count|0xA5000000,"long filtered candidate list")
                both(0x02018038,b"\0");update(count,"long list initial selection")
        for case in range(96):
            fixture(case)
            for follower,name in ((False,"SetLeaderIdleRenderAnimations"),(True,"RefreshWordChoiceAnimations")):
                equal(native("hn_motion_idle_render",1,C.c_bool)(memory,follower),True,f"native idle animation {name}")
                call(name);compare(name,ranges)
                equal(native("hn_motion_idle_render",1,C.c_bool)(memory,follower),True,"native repeated idle")
                call(name);compare("repeated idle animation",ranges)
            descriptor=0x02022000;columns=(1,2,3)[case%3];rows=9//columns
            both(descriptor,rng.randbytes(24));both(descriptor+12,struct.pack("<I",0x088A31FE))
            both(descriptor+16,bytes([columns,rows,(8,10,12)[case%3],case%8,case,case*2&255]))
            for index in range(9):both(0x030007B0+index*12,struct.pack("<2I",index+1,int(index==case%9)))
            both(0x030007A4,struct.pack("<I",case%9+1));both(0x030007A8,b"\x09")
            equal(native("hn_choice_initial_selection",1,C.c_bool)(memory,descriptor),True,"native initial word selection")
            call("InitializeWordChoiceSelection",descriptor);compare("initial choice selection",ranges)
            equal(native("hn_choice_display",1,C.c_bool)(memory,descriptor),True,"native choice display")
            call("UpdateWordChoiceSelectionDisplay",descriptor);compare("choice selection display",ranges)
            equal(native("hn_choice_select_identifier",2,C.c_bool)(memory,descriptor,case%9+1),True,"native requested choice display")
            call("SetWordChoiceSelectionDisplay",descriptor,case%9+1);compare("requested choice display",ranges)

        for case in range(45):
            fixture(case);count=case%9+1
            # Filter one noninitial option; retain at least one available entry.
            if count>2:both(0x02021002,struct.pack("<H",0xBE00))
            both(0x030007A4,struct.pack("<I",1));update(count+((case+1)<<16),f"initialize {case}")
            both(0x02018038,b"\x88");update(count,f"opening wait {case}")
            both(0x02018038,b"\0");update(count,f"opening {case}")
            for tick in range(8):
                equal(task_frames(memory,4096,task_report),1,f"native choice text {case}/{tick}: {list(task_report)}")
                call("UpdateTaskContexts",limit=4000000);compare(f"choice text tasks {case}/{tick}",ranges)
            for step,keys in enumerate((16,128,32,64,16|128,32|64,0,2 if case%3 else 1)):
                both(0x03000822,struct.pack("<H",keys&3));both(0x03000824,struct.pack("<H",keys&240))
                if step==7 and case%3==1:both(0x03001F58,bytes([native_bytes(0x03001F58,1)[0]|1]))
                update(count,f"selection {case}/{step}")
            if case%3==1:
                native("hn_choice_request",1)(memory,1);call("RequestWordChoiceIdentifier",1);compare("requested choice",ranges)
                both(0x03000822,b"\0\0");both(0x03000824,b"\0\0");update(count,f"request close {case}")
            for step in range(10):
                both(0x02018038,bytes([0x88 if step<8 else 0]));update(count,f"closing {case}/{step}")

        for case in range(32):
            fixture(case)
            payload=bytes([case%4,case*7&255,case*5&255])+struct.pack("<H",(0,15,16,511,512,527,528,65535)[case%8])
            both(0x02020000,payload)
            equal(command(memory,0x45,0x02020000),1,"native Main choice configuration")
            call("MainScriptConfigureWordChoice",0x02020000);compare("Main choice configuration",ranges)
        for case in range(8):
            fixture(case);count=case%5+1;both(0x030007A4,struct.pack("<I",1))
            both(0x03002BB0,struct.pack("<2I",0x02020000,0x02021000))
            both(0x02024000,bytes([count])+struct.pack("<2I",0,1))
            for step in range(8):
                both(0x02018038,bytes([0x88 if step in (1,4) else 0]))
                both(0x03000822,struct.pack("<H",1 if step==3 else 0))
                equal(command(memory,0x24,0x02024000),1,"native Main word choice")
                call("MainScriptAwaitWordChoice",0x02024000,limit=6000000);compare("Main word-choice protocol",ranges)
                if step==5:break
        for state in range(7):
            fixture(state)
            update(3,"reset fixture initialization")
            both(0x03001F94,struct.pack("<H",state))
            equal(native("hn_choice_reset",result=C.c_bool)(memory),True,"native choice reset")
            call("ScriptCallbackResetWordChoiceDisplay",0);compare(f"choice reset state {state}",ranges)

    @timed
    def raster_checks():
        ranges = ((0x02000000,0x40000),(0x03000000,0x7000),(0x04000000,0x60),(0x040000B0,12))
        graphics_command = bind("hn_probe_graphics_command", C.c_int, C.c_void_p, C.c_uint32,
                                C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
        state, data = 0x02018000, 0x02020000
        report = (C.c_uint32*4)()
        def invoke(opcode, name, payload=b""):
            both(data,payload+bytes(8))
            equal(graphics_command(memory,state,opcode,data,report),1,f"native raster command {name}")
            equal(report[3],call(name,state,data,limit=1000000),f"raster continuation {name}")
            compare(name,ranges)
        fresh()
        for selector in (0,1,17,255,256,257):
            for status in range(256):
                both(0x03002DD4,bytes([status]))
                native("hn_video_hblank",1)(memory,selector);call("SetHblankIrqEnabled",selector)
                compare("HBlank selector/pending bits",ranges[1:2])
        for case in range(64):
            fresh();heap_init(memory);call("InitializeHeap",0)
            both(0x03002C90,rng.randbytes(0x108));both(0x03002DD4,rng.randbytes(4))
            both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,bytes([16]))
            both(0x02010000,rng.randbytes(16*104));both(0x0201DC40,rng.randbytes(128))
            both(0x030021AB,bytes([8 if case&1 else 0]));both(0x03000820,struct.pack("<H",case&2))
            both(state,rng.randbytes(44));both(state,bytes([1]))
            kind=case%8+1
            invoke(0xA7,"GraphicsAllocateRasterEffect",bytes([kind]))
            effect=struct.unpack("<I",native_bytes(0x02000398,4))[0]
            both(effect,rng.randbytes(258))
            invoke(0xA1,"GraphicsConfigureRasterWindow",struct.pack("<BHHBB",case%3,rng.getrandbits(16),rng.getrandbits(16),case,case&1))
            invoke(0xA8,"GraphicsSetRasterPosition",rng.randbytes(2))
            invoke(0xAD,"GraphicsSetRasterPositionAD",rng.randbytes(2))
            invoke(0xA9,"GraphicsSetRasterPositionAndByte5",rng.randbytes(3))
            invoke(0xA4,"GraphicsSetRasterBytes5To8",rng.randbytes(4))
            invoke(0xA5,"GraphicsSetRasterBytes9To11",rng.randbytes(3))
            for source in (23,0xFD,0xFE,0xFF):
                invoke(0xA3,"GraphicsSetRasterPositionFromSource",bytes([source,case]))
            if kind==5:
                both(effect+5,struct.pack("<H",8+case*3));both(effect+9,b"\x01\0\0")
            elif kind==8:
                both(effect+3,bytes([255 if case&16 else 3,case%3]))
            invoke(0xA2,"GraphicsBeginRasterEffect")
            if kind==8:
                for step in range(256):
                    invoke(0xAE,"GraphicsUpdateRasterSweep")
                    if native_bytes(0x02000398,4)==bytes(4):break
                equal(native_bytes(0x02000398,4),bytes(4),"raster sweep releases state")
            else:
                invoke(0xA6,"GraphicsReleaseRasterEffect")
            invoke(0xA7,"GraphicsAllocateRasterEffect",bytes([kind]))
            invoke(0xA7,"GraphicsAllocateRasterEffect",bytes([9-kind]))
        for case in range(160):
            fresh();heap_init(memory);call("InitializeHeap",0)
            effect=allocate(memory,258,False)
            equal(effect,call("AllocateHeap",258),"raster profile allocation")
            both(0x02000398,struct.pack("<I",effect));both(effect,rng.randbytes(258))
            both(effect+5,struct.pack("<H",4+case%200));both(effect+9,bytes([(0,1,2,255,254)[case%5]]))
            equal(native("hn_raster_profile",result=C.c_bool)(memory),True,"native radius profile")
            call("UpdateRasterRadiusProfile");compare(f"radius profile {case}",ranges[:2])
        for case in range(24):
            fresh();heap_init(memory);call("InitializeHeap",0)
            both(0x02000040,struct.pack("<I",symbols["UpdateRasterCircleWindow"]|1))
            both(0x03002C90,rng.randbytes(0x108));both(0x03002DD4,rng.randbytes(4))
            invoke(0xA7,"GraphicsAllocateRasterEffect",b"\x05")
            effect=struct.unpack("<I",native_bytes(0x02000398,4))[0]
            radius=24+case*7
            increment=255 if case&1 else 1
            limit=radius-2 if case&1 else radius+2
            both(effect,bytes([case%3,31,case&31,case*10,case*7,radius,0,limit,0,increment,0,0,0,0,0]))
            invoke(0xA2,"GraphicsBeginRasterEffect")
            for frame in range(4):
                for line in range(228):
                    both(0x04000006,struct.pack("<H",line))
                    equal(native("hn_raster_hblank",result=C.c_bool)(memory),True,f"native circle HBlank {case}/{frame}/{line}")
                    call("HandleHblankIrq");compare("circle IRQ state",ranges)
                    if native_bytes(0x02000398,4)==bytes(4):break
                if native_bytes(0x02000398,4)==bytes(4):break
            equal(native_bytes(0x02000398,4),bytes(4),"circle IRQ releases state")

    @timed
    def region_audio_checks():
        fresh();equal(native("hn_audio_driver_init",result=C.c_bool)(memory),True,"region audio initialization")
        call("InitializeAudioDriver",limit=2000000)
        both(0x03002D98,struct.pack("<I",0x02028000));both(0x03002DC0,b"\x01")
        animation=bind("hn_probe_animation",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
        report=(C.c_uint32*4)();case=0
        ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x03007FF0,4),(0x04000060,0x80),(0x04000100,4),(0x03007010,4))
        for record in range(3):
            for no_song in (0,128):
                for mode in range(3):
                    for factor in (0,1,2,255):
                        for bounds in ((-32,-16,128,96),(-32768,-32768,-32767,-32767),(128,96,-32,-16),(-32768,-32768,32767,32767),(7,-3,7,-3)):
                            low_x,low_z,high_x,high_z=[(v<<16)+0x8000 for v in bounds]
                            positions=((low_x-1,low_z),(low_x,low_z),(low_x+1,low_z+1),(high_x,high_z),
                                       (high_x+1,high_z),(high_x,high_z+1),((low_x+high_x)//2,(low_z+high_z)//2),(0x8000,0x8000))
                            for x,z in positions:
                                both(0x02028018,struct.pack("<I",x&0xFFFFFFFF));both(0x02028020,struct.pack("<I",z&0xFFFFFFFF))
                                flag=(0,1,0x60,0x180,0x1FF,65535)[case%6];case+=1
                                payload=struct.pack("<B4HBBH",record|no_song,*(v&65535 for v in bounds),mode,factor,flag)
                                both(0x02020000,b"\x8D"+struct.pack("<I",symbols["AnimationCallbackRegionAudio"]|1)+payload)
                                both(0x03007010,struct.pack("<I",0x02020000))
                                equal(animation(memory,0x02028000,0x8D,0x03007010,report),1,"native region audio")
                                equal(report[3],call("AnimationInvokeCallback",0x02028000,0x03007010),"region audio yield")
                                compare("region audio",ranges)

    @timed
    def audio_checks():
        audio_ranges = ((0x02000000, 0x40000), (0x03000000, 0x7000), (0x03007FF0, 4),
                        (0x04000060, 0x80), (0x04000100, 4))
        fresh()
        equal(native("hn_audio_driver_init", result=C.c_bool)(memory), True, "native audio driver init")
        call("InitializeAudioDriver", limit=2000000)
        compare("audio driver init", audio_ranges)
        for frequency in range(1, 13):
            equal(native("hn_audio_mode", 1, C.c_bool)(memory, frequency << 16), True, f"audio rate success {frequency}")
            call("SetAudioCoreMode", frequency << 16)
            compare(f"audio rate {frequency}", audio_ranges)
        for mode in (0, 0x7F, 0x93D800, 0x08FF, 0x100080, 0x800000):
            equal(native("hn_audio_mode", 1, C.c_bool)(memory, mode), True, f"audio mode success {mode:X}")
            call("SetAudioCoreMode", mode)
            compare(f"audio mode {mode:X}", audio_ranges)
        for case in range(100):
            player, tracks = 0x030050A0, 0x03001410
            both(player, rng.randbytes(64))
            both(player + 8, bytes([8]))
            both(player + 0x2C, struct.pack("<I", tracks))
            both(player + 0x34, struct.pack("<I", 0x68736D53 if case % 7 else 0))
            both(tracks, rng.randbytes(80 * 8))
            for i in range(8):
                both(tracks + i * 80 + 0x20, bytes(4))
            for parameter, original in enumerate(("SetAudioTrackVolume", "SetAudioTrackPitch", "SetAudioTrackPan",
                                                   "SetAudioPlayerTrackModulationDepth", "SetAudioPlayerTrackLfoSpeed")):
                mask, value = rng.getrandbits(32), rng.getrandbits(32)
                native("hn_audio_track_parameter", 4)(memory, player, mask, value, parameter)
                call(original, player, mask, value)
                compare(f"audio parameter {parameter} {case}", audio_ranges[:2])
            scale = rng.randrange(65536)
            native("hn_audio_tempo", 2)(memory, player, scale)
            call("SetAudioTempoScale", player, scale)
            compare(f"audio tempo {case}", audio_ranges[:2])
            for i in range(8):
                native("hn_audio_volume_pitch", 1)(memory, tracks + i * 80)
                call("UpdateAudioTrackVolumePitch", player, tracks + i * 80)
                compare(f"audio volume/pitch {case} {i}", audio_ranges[:2])
            for mode, original in enumerate(("ConfigureAudioPlayerFade", "FadeOutAudioPlayer", "FadeInAudioPlayer")):
                interval = case % 5
                native("hn_audio_fade", 3)(memory, player, interval, mode)
                call(original, player, interval)
                compare(f"audio fade setup {case} {mode}", audio_ranges[:2])
                for step in range(12):
                    equal(native("hn_audio_fade_update", 1, C.c_bool)(memory, player), True, "native audio fade update")
                    call("ProcessAudioPlayerFade", player)
                    compare(f"audio fade {case} {mode} {step}", audio_ranges[:2])
            equal(native("hn_audio_player_stop", 1, C.c_bool)(memory, player), True, "native stop player")
            call("StopAudioPlayer", player)
            native("hn_audio_player_resume", 1)(memory, player)
            call("ResumeAudioPlayerState", player)
            compare(f"audio player stop/resume {case}", audio_ranges)
        for case in range(600):
            channel = 0x03004FA0
            both(channel, rng.randbytes(64))
            native("hn_audio_psg_volume", 1)(memory, channel)
            call("UpdateAudioPsgVolume", channel)
            compare(f"audio PSG volume {case}", ((channel, 64),))
            control, key, fraction = case % 5, rng.randrange(256), rng.randrange(256)
            equal(native("hn_audio_psg_frequency", 3, C.c_uint32)(memory, control, key, fraction),
                  call("CalculateAudioPsgFrequency", control, key, fraction), f"PSG pitch {case}")
            both(0x02020004, struct.pack("<I", rng.getrandbits(32)))
            equal(native("hn_audio_wave_frequency", 3, C.c_uint32)(memory, 0x02020000, key, fraction),
                  call("CalculateAudioWaveFrequency", 0x02020000, key, fraction), f"wave pitch {case}")
        fresh()
        equal(native("hn_audio_driver_init", result=C.c_bool)(memory), True, "native song-control driver")
        call("InitializeAudioDriver", limit=2000000)
        song_functions = ("StartSong", "StartSongOrChange", "StartSongOrContinue", "StopSong", "ResumeSong")
        for song in range(300):
            record = rom[0x6EAE34 + song * 8:0x6EAE34 + song * 8 + 8]
            data, index, _ = struct.unpack("<IHH", record)
            if index >= 6:
                raise RuntimeError("Selected private song has an invalid player index.")
            player = struct.unpack_from("<I", rom, 0x6EADEC + index * 12)[0]
            if song % 3 == 0:
                both(player, struct.pack("<I", data))
                both(player + 4, struct.pack("<I", 1 | (0x80000000 if song & 1 else 0)))
            for action, original in enumerate(song_functions):
                equal(native("hn_audio_song", 2, C.c_bool)(memory, song, action), True, f"native song action {song} {action}")
                call(original, song)
                compare(f"song action {song} {action}", audio_ranges)
        for resume, original in ((False, "StopAllSongs"), (True, "ContinueAllSongs")):
            equal(native("hn_audio_all", 1, C.c_bool)(memory, resume), True, f"native all players {resume}")
            call(original)
            compare(f"all players {resume}", audio_ranges)
        sequence_names = (
            "StopAudioTrackCommandChannels", "JumpAudioTrackStream", "CallAudioTrackPatternAsm",
            "ReturnAudioTrackPatternAsm", "RepeatAudioTrackPatternAsm", "StopAudioTrackCommandChannels",
            "StopAudioTrackCommandChannels", "StopAudioTrackCommandChannels", "ExecuteAudioMemoryCommand",
            "SetAudioTrackPriorityAsm", "SetAudioPlayerTempoAsm", "SetAudioTrackKeyShiftAsm", "SetAudioTrackVoiceAsm",
            "SetAudioTrackVolumeAsm", "SetAudioTrackPanAsm", "SetAudioTrackBendAsm", "SetAudioTrackBendRangeAsm",
            "SetAudioTrackLfoSpeedAsm", "SetAudioTrackLfoDelayAsm", "SetAudioTrackModulationDepthAsm",
            "SetAudioTrackModulationTypeAsm", "StopAudioTrackCommandChannels", "StopAudioTrackCommandChannels",
            "SetAudioTrackTuningAsm", "StopAudioTrackCommandChannels", "StopAudioTrackCommandChannels",
            "StopAudioTrackCommandChannels", "WriteAudioTrackSoundRegisterAsm", "ExecuteAudioExtendedCommand", "EndAudioTrackTieAsm")
        audio_command = bind("hn_probe_audio_command", C.c_int, C.c_void_p, C.c_uint32,
                             C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
        report_audio = (C.c_uint32 * 3)()
        for case in range(24):
            for opcode, original in enumerate(sequence_names, 0xB1):
                both(0x030050A0, rng.randbytes(64))
                both(0x03001410, rng.randbytes(80))
                both(0x030050B8, struct.pack("<I", 0x02021000))
                both(0x030050D0, struct.pack("<I", 0x02022000))
                both(0x02021000, rng.randbytes(256))
                both(0x02022000, rng.randbytes(256 * 12))
                both(0x03001412, bytes([case % 4, case * 13 % 256]))
                both(0x03001430, bytes(4))
                for i in range(3):
                    both(0x03001454 + i * 4, struct.pack("<I", 0x02023000 + i * 16))
                if opcode in (0xB1, 0xB3, 0xCD, 0xCE):
                    both(0x03001430, struct.pack("<I", 0x03003F30))
                    for i in range(3):
                        channel = 0x03003F30 + i * 64
                        both(channel, rng.randbytes(64))
                        both(channel + 0x11, bytes([case & 127]))
                        both(channel + 0x2C, struct.pack("<III", 0x03001410,
                             channel - 64 if i else 0, channel + 64 if i < 2 else 0))
                payload = bytes([case * 13 % 256])
                if opcode in (0xB2, 0xB3):
                    payload = struct.pack("<I", 0x02023000)
                elif opcode == 0xB5:
                    payload = struct.pack("<BI", case % 4, 0x02023000)
                elif opcode == 0xB9:
                    payload = struct.pack("<BBBI", case % 18, 3, case, 0x02023000)
                elif opcode == 0xCC:
                    payload = bytes([case % 32, case * 13 % 256])
                elif opcode == 0xCD:
                    payload = bytes([case % 12]) + struct.pack("<I", 0x12345678)
                elif opcode == 0xCE:
                    payload = bytes([case & 127 if case & 1 else 128])
                both(0x02020000, payload)
                both(0x03001450, struct.pack("<I", 0x02020000))
                equal(audio_command(memory, 0x030050A0, 0x03001410, opcode, report_audio), 1,
                      f"native audio sequence {opcode:02X} {case}: {list(report_audio)}")
                call(original, 0x030050A0, 0x03001410)
                compare(f"audio sequence {opcode:02X} {case}", audio_ranges)
        note = bind("hn_probe_audio_note", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32,
                    C.c_uint32, C.POINTER(C.c_uint32))
        for case in range(240):
            player, track, core = 0x030050A0, 0x02010000, 0x03003EE0
            both(player + 9, bytes([rng.randrange(256)]))
            both(track, rng.randbytes(80))
            both(track + 5, bytes([rng.randrange(128)]))
            both(track + 0x20, bytes(4))
            both(track + 0x40, struct.pack("<I", 0x02020000))
            both(core + 6, bytes([case % 9]))
            both(core + 0x1C, struct.pack("<I", 0 if case % 13 == 0 else 0x03004FA0))
            for i in range(16):
                channel = core + 0x50 + i * 64 if i < 12 else 0x03004FA0 + (i - 12) * 64
                both(channel, rng.randbytes(64))
                status = 0 if case % 4 == 0 else 0x83 if case % 4 == 1 else 0x43 if case % 4 == 2 else rng.randrange(256)
                both(channel, bytes([status]))
                owner = (0x0200F000, 0x02010000, 0x02011000)[(case + i) % 3] if status else 0
                both(channel + 0x2C, struct.pack("<III", owner, 0, 0))
            # Explicit PCM/PSG, key-split and rhythm-bank paths; no sample bytes or captured state.
            kind, bank = case % 5, case % 3
            both(track + 0x24, struct.pack("<BBBBII", kind if bank == 0 else 0x40 if bank == 1 else 0x80,
                                          60, 0, 0, 0x02026000 if bank == 0 else 0x02023000,
                                          0x12345678 if bank == 0 else 0x02024000))
            both(0x02024000, bytes(rng.randrange(8) for _ in range(256)))
            for i in range(128):
                both(0x02023000 + i * 12, struct.pack("<BBBBII", kind, i & 127, i & 255,
                     rng.randrange(256), 0x02026000, rng.getrandbits(32)))
            both(0x02026004, struct.pack("<I", rng.getrandbits(32)))
            payload = bytes([rng.randrange(128), rng.randrange(128), rng.randrange(128)][:case % 4]) + b"\x81"
            both(0x02020000, payload)
            equal(note(memory, player, track, case % 49, report_audio), 1,
                  f"native audio note {case}: {list(report_audio)}")
            call("StartAudioTrackNoteAsm", case % 49, player, track)
            compare(f"audio note {case}", audio_ranges)
        player_step = bind("hn_probe_audio_player", C.c_int, C.c_void_p, C.c_uint32,
                           C.c_uint32, C.POINTER(C.c_uint32))
        for song in (1, 10, 40, 67, 128, 199, 200, 205, 220, 250, 270, 299):
            fresh()
            equal(native("hn_audio_driver_init", result=C.c_bool)(memory), True, f"audio sequencer driver {song}")
            call("InitializeAudioDriver", limit=2000000)
            equal(native("hn_audio_song", 2, C.c_bool)(memory, song, 0), True, f"audio sequencer song {song}")
            call("StartSong", song)
            index = struct.unpack_from("<H", rom, 0x6EAE34 + song * 8 + 4)[0]
            player = struct.unpack_from("<I", rom, 0x6EADEC + index * 12)[0]
            for frame_index in range(120):
                equal(player_step(memory, player, 4096, report_audio), 1,
                      f"native audio player {song} {frame_index}: {list(report_audio)}")
                call("ProcessAudioPlayerAsm", player, limit=1000000)
                compare(f"audio player {song} {frame_index}", audio_ranges)
        psg_step = bind("hn_probe_audio_psg", C.c_int, C.c_void_p, C.POINTER(C.c_uint32))
        mix = bind("hn_probe_audio_mix", C.c_int, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        fresh()
        equal(native("hn_audio_driver_init", result=C.c_bool)(memory), True, "PSG service driver")
        call("InitializeAudioDriver", limit=2000000)
        for case in range(240):
            both(0x03003EEA, bytes([case % 16]))
            both(0x04000060, rng.randbytes(64))
            both(0x02020000, rng.randbytes(16))
            for i in range(4):
                channel = 0x03004FA0 + i * 64
                both(channel, rng.randbytes(64))
                both(channel, bytes([(0, 0x80, 3, 2, 1, 0x43, 4, 0xC0)[(case + i) % 8]]))
                both(channel + 0x24, struct.pack("<II", 0x02020000, 0 if case & 1 else 0x02020000))
                both(channel + 4, bytes([case % 8, (case // 2) % 8, case % 16, (case // 3) % 8]))
                both(channel + 0x1C, bytes([0x11 << i]))
            for frame_index in range(5):
                equal(psg_step(memory, report_audio), 1, f"native PSG service {case} {frame_index}")
                call("UpdateAudioPsgChannels", limit=1000000)
                compare(f"PSG service {case} {frame_index}", audio_ranges)
        for case in range(160):
            fresh()
            equal(native("hn_audio_driver_init", result=C.c_bool)(memory), True, "PCM driver")
            call("InitializeAudioDriver", limit=2000000)
            core = 0x03003EE0
            both(core + 0x20, bytes(4))
            both(core + 0x28, struct.pack("<I", 0x080EC0B5))
            samples = (16, 20, 28, 32, 64, 132, 264)[case % 7]
            period = 1584 // samples
            both(core + 4, bytes([case % (period + 1), case * 7 % 128, case % 9, case % 16]))
            both(core + 11, bytes([period]))
            both(core + 0x10, struct.pack("<I", samples))
            both(core + 0x18, struct.pack("<I", 1))
            both(core + 0x350, rng.randbytes(3168))
            for i in range(12):
                channel, wave = core + 0x50 + i * 64, 0x02024000 + i * 1024
                both(channel, rng.randbytes(64))
                status = (0, 0x80, 0x83, 0x82, 0x81, 0x43, 0x44, 0x84, 0xC0, 0x13, 0x12, 0x14)[(case + i) % 12]
                both(channel, bytes([status, 8 if (case + i) & 1 else 0]))
                length = (1, 2, 3, 4, 5, 7, 8, 11, 16, 32, 64, 255)[(case + i) % 12]
                offset = 0 if case % 3 == 0 else length // 2
                loop_start = length if case % 19 == 0 else length // 2
                both(wave, struct.pack("<IIII", 0x40000000 if case & 2 else 0, 0, loop_start, length) + rng.randbytes(256))
                both(channel + 13, bytes([case % 4]))
                both(channel + 0x18, struct.pack("<II", length - offset, rng.randrange(1 << 23)))
                step = (0, 0x200000, 0x800000, 0xC00000, 0x1000000, 0x3000000, 0x8000000, 0x3F000000)[(case + i) % 8]
                both(channel + 0x20, struct.pack("<III", step, wave, wave + 16 + offset))
            equal(mix(memory, 4096, report_audio), 1, f"native PCM mix {case}: {list(report_audio)}")
            call("RunAudioMixerAsm", limit=2000000)
            compare(f"PCM mix {case}", audio_ranges)
        pcm_frames, nonzero_pcm_frames = 0, 0
        for song in (1, 10, 40, 67, 128, 199, 200, 205, 220, 250, 270, 299):
            fresh()
            equal(native("hn_audio_driver_init", result=C.c_bool)(memory), True, f"audio mixer driver {song}")
            call("InitializeAudioDriver", limit=2000000)
            equal(native("hn_audio_song", 2, C.c_bool)(memory, song, 0), True, f"audio mixer song {song}")
            call("StartSong", song)
            for frame_index in range(120):
                native("hn_audio_dma_vblank")(memory)
                call("SyncAudioDmaAtVblankAsm")
                equal(mix(memory, 4096, report_audio), 1,
                      f"native audio mixer {song} {frame_index}: {list(report_audio)}")
                call("RunAudioMixerAsm", limit=2000000)
                compare(f"audio mixer {song} {frame_index}", audio_ranges)
                core = 0x03003EE0
                samples = struct.unpack("<I", native_bytes(core + 0x10, 4))[0]
                counter, period = native_bytes(core + 4, 1)[0], native_bytes(core + 11, 1)[0]
                output = core + 0x350 + (samples * (period - counter + 1) if counter > 1 else 0)
                nonzero_pcm_frames += any(native_bytes(output, samples) + native_bytes(output + 0x630, samples))
                pcm_frames += 1
        print(f"{pcm_frames} private ROM mixer frames matched; {nonzero_pcm_frames} contained nonzero PCM.")
        for case in range(18):
            fresh()
            equal(native("hn_runtime_tables_init", result=C.c_bool)(memory), True, "audio Main runtime table")
            cpu.mem_write(0x02000000, rom[0x8FE018:0x900EC8])
            equal(native("hn_audio_driver_init", result=C.c_bool)(memory), True, "audio Main driver")
            call("InitializeAudioDriver", limit=2000000)
            song = (67, 200, 205, 250)[case % 4]
            index = struct.unpack_from("<H", rom, 0x6EAE34 + song * 8 + 4)[0]
            player = struct.unpack_from("<I", rom, 0x6EADEC + index * 12)[0]
            both(0x03000040 + index, bytes([case % 3]))
            both(0x03000020 + index * 2, struct.pack("<H", song if case & 1 else 0))
            both(0x0201DC5E, struct.pack("<H", 200))
            both(0x03002180, bytes([8 if case & 1 else 0]))
            cases = (
                (0x0A, "MainScriptStartSong", struct.pack("<HB", song, case % 4)),
                (0x0B, "MainScriptStopSong", struct.pack("<HB", song if case % 3 else 0, case % 4)),
                (0x56, "MainScriptChangeAudioPlayer", struct.pack("<HBB", song if case % 3 else 0, case & 1, case % 3)),
                (0x57, "MainScriptSelectAudioPlayers", bytes([case & 1, (0, 1, 5, 63, 255)[case % 5]])),
                (0x5C, "MainScriptSetAudioParameters", struct.pack("<HHHHHB", song, 0xFF, case * 2345,
                                                                 case * 3456, case * 3333, case * 13)),
                (0x5E, "MainScriptWaitForSong", struct.pack("<HB", song, case & 1)))
            for opcode, original, payload in cases:
                both(0x02020000, payload)
                both(0x03000010, bytes([opcode]))
                both(0x0300001D, bytes(1))
                if opcode == 0x5E:
                    both(player + 4, struct.pack("<I", (0x80000000, 0x80000001, 1)[case % 3]))
                equal(command(memory, opcode, 0x02020000), 1, f"native Main audio {opcode:02X} {case}")
                call(original, 0x02020000)
                compare(f"Main audio {opcode:02X} {case}", audio_ranges)
        animation_audio = bind("hn_probe_animation", C.c_int, C.c_void_p, C.c_uint32, C.c_uint32,
                               C.c_uint32, C.POINTER(C.c_uint32))
        report_animation = (C.c_uint32 * 4)()
        for case in range(12):
            for opcode, original in ((0xD0, "AnimationStartSong"), (0xD1, "AnimationStopSong")):
                slot = 0x02010000
                both(slot, rng.randbytes(104))
                both(slot + 0x61, bytes([64 if case & 2 else 0]))
                both(0x03002182, bytes([64 if case & 4 else 0]))
                both(0x02020000, struct.pack("<BH", opcode, (0, 67, 200)[case % 3]))
                both(0x03007010, struct.pack("<I", 0x02020000))
                equal(animation_audio(memory, slot, opcode, 0x03007010, report_animation), 1, f"native animation audio {case} {opcode}")
                equal(report_animation[3], call(original, slot, 0x03007010), f"animation audio yield {case} {opcode}")
                compare(f"animation audio {case} {opcode}", audio_ranges)

    @timed
    def indicator_checks():
        local=random.Random(0x494E4449)
        ranges=((0x02000000,0x40000),(0x03000000,0x7000))
        complete=native("hn_scene_indicator_complete",1,C.c_bool)
        visible=native("hn_scene_indicator_visible",1,C.c_bool)
        configure=native("hn_scene_indicator_configure",2,C.c_bool)
        update=native("hn_scene_indicator_update",1,C.c_bool)
        fresh()
        for missing in (-1,*range(0x47,0x9D)):
            flags=bytearray(b"\xFF"*96)
            if missing>=0:flags[missing//8]&=~(1<<(missing&7))
            both(0x03002180,bytes(flags))
            for index in range(1,8):
                equal(complete(memory,0xA5000000|index),bool(call("CheckSceneIndicatorComplete",index)),"destination completion flags")
        for index in range(256):
            equal(complete(memory,index),bool(call("CheckSceneIndicatorComplete",index)),"destination completion index")
            for mode in (0,1,2,3,4,5,255):
                both(0x02004536,bytes([mode]))
                equal(visible(memory,0xA5000000|index),bool(call("CheckSceneIndicatorVisible",index)),"destination visibility")
        def prepare():
            fresh();both(0x02004520,local.randbytes(512));both(0x03002180,local.randbytes(96))
            both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x28")
            both(0x02010000,local.randbytes(40*104));both(0x03002E08,struct.pack("<I",0x02018000))
            both(0x03002C80,b"\x09");both(0x02018000,local.randbytes(9*80))
            both(0x0200194C,struct.pack("<40I",*(0x08000100+i*16 for i in range(40))))
            both(0x03001F50,bytes([local.choice((0,15,26))]));both(0x03002170,b"\xA5")
        def invoke(name):
            both(0x02020000,struct.pack("<I",symbols[name]|1));both(0x03000010,b"\x1C"+bytes(15))
            equal(command(memory,0x1C,0x02020000),1,f"native destination {name}")
            call("MainScriptInvokeCallback",0x02020000);compare(f"destination {name}",ranges)
        for index in range(8):
            for state in (0,1,2,3,4,5,6,255):
                prepare()
                equal(configure(memory,index|0xA5000000,state|0xA5000000),True,"native destination indicator setup")
                call("ConfigureSceneIndicator",index,state);compare("destination indicator setup",ranges)
        for case in range(320):
            prepare();language=case%5;selected=case%8
            both(0x0200460B,bytes([language]));both(0x0200458C,bytes([selected]));both(0x02004605,bytes([(case//8)%8]))
            both(0x02004536,bytes([(case//40)%4]));both(0x02004533,bytes([case%2]))
            if case&1:both(0x03002180,b"\xFF"*96)
            equal(update(memory,selected),True,"native destination indicator update")
            call("UpdateSceneIndicator",selected);compare("destination indicator update",ranges)
            invoke("ScriptCallbackInitializeSceneIndicators")
            both(0x03001164,bytes([(case+3)%8,case%2]));invoke("ScriptCallbackRefreshSceneIndicators")
        for mask in range(256):
            prepare();both(0x030021B0,bytes([mask]));invoke("ScriptCallbackUnlockSceneIndicator")
            both(0x02004606,bytes([mask%8]));invoke("ScriptCallbackInitializeSceneDestinationIndicator")
        prepare();equal(native("hn_audio_driver_init",result=C.c_bool)(memory),True,"destination navigation audio init")
        call("InitializeAudioDriver",limit=2000000)
        for flags in range(256):
            for mode in (0,1):
                both(0x03002188,bytes([flags]));both(0x02004536,bytes([mode]))
                for index in range(-2,7):both(0x02001930+index*4,struct.pack("<I",0x02022000+(index+2)*64))
                for table in range(9):both(0x02022000+table*64,bytes(local.randrange(9) for _ in range(64)))
                for direction in range(16):
                    both(0x0200458C,bytes([direction%8]));both(0x03000822,struct.pack("<H",direction<<4))
                    both(0x03001164,local.randbytes(2));invoke("ScriptCallbackMoveSceneIndicator")
        compare("destination navigation audio",ranges+((0x03007FF0,4),(0x04000060,0x80),(0x04000100,4)))

    @timed
    def book_checks():
        local=random.Random(0x48414D)
        def prepare(sorted_mode=0,language=0):
            fresh();both(0x02004520,local.randbytes(512));both(0x03002180,local.randbytes(96))
            both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x28")
            both(0x02010000,local.randbytes(40*104));both(0x03003070,struct.pack("<I",0x02018000))
            both(0x03002E10,b"\x08");both(0x02018000,local.randbytes(8*44))
            both(0x03001F50,bytes([local.choice((10,13,25,29,32))]));both(0x03002170,b"\x73")
            both(0x0200460B,bytes([language]));both(0x0300219F,bytes([sorted_mode]))
            orders=[]
            for _ in range(5):
                order=list(range(86));local.shuffle(order);orders.extend(order)
            both(0x020019EC,bytes(orders));both(0x030036F0,local.randbytes(768))
            both(0x02002AB4,local.randbytes(2640))
            words=list(range(1,87));local.shuffle(words);both(0x030037F0,bytes(words))
            both(0x03003E30,b"\x56")
        def invoke(name,payload=b""):
            both(0x02020000,struct.pack("<I",symbols[name]|1)+payload)
            both(0x03000010,b"\x1C"+bytes(15))
            equal(command(memory,0x1C,0x02020000),1,f"native book {name}")
            call("MainScriptInvokeCallback",0x02020000,limit=1000000)
            compare(f"book {name}",((0x02000000,0x40000),(0x03000000,0x7000),(0x06000000,0x18000)))
        for mode in range(2):
            for selected in range(256):
                prepare(mode,selected%5);both(0x02004596,bytes([selected]))
                both(0x03003E30,bytes([(0,1,10,86,250)[selected%5]]))
                invoke("ScriptCallbackRecordTaskTextAcquisition")
                invoke("ScriptCallbackSelectTaskTextAcquiredPage")
            for selected in range(87):
                prepare(mode,selected%5);both(0x02004542,bytes([selected]));both(0x02004592,bytes([selected%5]))
                invoke("ScriptCallbackRestoreTaskTextSelectionPage")
            for case in range(160):
                prepare(mode,case%5);both(0x02004591,bytes([case%65]));both(0x02004592,bytes([case%7]))
                both(0x03003E30,bytes([case%87]));invoke("ScriptCallbackResolveTaskTextSelection")
                invoke("ScriptCallbackShowTaskTextSelectionMarkers")
        for case in range(256):
            prepare();both(0x02004580,bytes([case]));invoke("ScriptCallbackRenderTaskTextByteNumber",bytes([case%39,case,255-case]))
            invoke("ScriptCallbackRenderPairListControlNumber",bytes([case%38,case,255-case]))
            both(0x02004590,bytes([case]));invoke("ScriptCallbackConfigureTaskTextPageSlots")
            invoke("ScriptCallbackPositionTaskTextPairRenderSlot24")
            both(0x02004592,bytes([case]));invoke("ScriptCallbackConfigureTaskTextSelectionMarkers")
            invoke("ScriptCallbackPositionRenderSlot24FromRaster72")
            invoke("ScriptCallbackConfigureTaskTextPageDecorations")
            for mode in range(2):
                both(0x0300219F,bytes([mode]));both(0x03003E30,bytes([case]));both(0x0200460B,bytes([case%7]))
                invoke("ScriptCallbackStoreTaskTextPageCount")
        for language in range(5):
            for category in range(10):
                prepare(1,language);both(0x02004590,bytes([category]));invoke("ScriptCallbackSelectTaskTextCategoryPage")
                for offset in (0,1,2,255):invoke("ScriptCallbackTestTaskTextCategoryPage",bytes([offset]))
        for mode in range(2):
            for language in range(5):
                for case in range(36):
                    prepare(mode,language);heap_init(memory);call("InitializeHeap",0)
                    native("hn_task_contexts_init")(memory);call("InitializeTaskContexts")
                    both(0x03003E30,bytes([(0,1,4,10,85,86)[case%6]]))
                    both(0x02004591,bytes([(0,1,20,21,22,63)[case//6]]))
                    for name in ("ScriptCallbackConfigureTaskTextPairHeader","ScriptCallbackRemoveTaskTextPairHeader",
                                 "ScriptCallbackFillTaskTextPairFirst","ScriptCallbackFillTaskTextPairSecond",
                                 "ScriptCallbackRemoveTaskTextPairContexts"):
                        invoke(name)
        number=native("hn_render_number",4,C.c_bool)
        prepare()
        for case in range(384):
            value=(0,1,9,10,99,100,101,999,1000,65535)[case%10]|(local.randrange(65536)<<16)
            x=local.getrandbits(32);y=local.getrandbits(32);index=case%38
            equal(number(memory,index,value,x,y),True,f"native three-digit number {case}")
            call("RenderPairListNumber",index,value,x,y)
            compare(f"three-digit number {case}",((0x02000000,0x40000),(0x03000000,0x7000)))

    @timed
    def map_choice_checks():
        start_checks=checks;local=random.Random(0x4D434843)
        family=[e for e in entries if e["source"]=="src/scripts/map_choice_callbacks.c"]
        source=(args.reference/"src/scripts/map_choice_callbacks.c").read_text(encoding="utf-8")
        signatures={}
        groups={kind:[] for kind in ("apply","main","animation","slot")}
        for row in family:
            name=row["name"];match=re.search(r"^[ \t]*(u32|void)\s+"+re.escape(name)+r"\(([^;{}]*)\)\s*\{",source,re.M)
            if not match:raise RuntimeError(f"Map-choice source signature missing: {name}")
            signatures[name]=(match[1],match[2].strip())
            kind="animation" if name.startswith("AnimationCallback") else "main" if name.startswith("ScriptCallback") else "apply" if match[2].strip()=="void" or name=="FaceMapChoiceSlot10Toward13" else "slot"
            groups[kind].append(row)
        equal(tuple(len(groups[k]) for k in ("apply","main","animation","slot")),(233,31,5,4),"complete 273-function map-choice contracts")
        apply_map=bind("hn_map_choice_apply",C.c_int,C.c_void_p,C.c_uint32,C.POINTER(C.c_uint32))
        main_map=bind("hn_scene_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
        animation_map=bind("hn_scene_animation_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32),C.POINTER(C.c_uint32))
        slot_map=bind("hn_map_choice_slot",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
        preset=bind("hn_preset_apply",C.c_int,C.c_void_p,C.c_uint32)
        task_command=bind("hn_probe_task_command",C.c_int,C.c_void_p,C.c_uint32,C.c_uint,C.POINTER(C.c_uint32));report=(C.c_uint32*4)()
        directions=bind("hn_render_direction_between",C.c_uint32,C.c_void_p,C.c_uint8,C.c_uint8,C.c_bool)
        data=rom[0x8FE018:0x900EC8]
        ranges=((0x02000000,0x40000),(0x03000000,0x7000),(0x03007FF0,4),(0x04000060,0x80),(0x04000100,4))
        fresh();both(0x02000000,data)
        equal(native("hn_audio_driver_init",result=C.c_bool)(memory),True,"map-choice audio baseline")
        call("InitializeAudioDriver",limit=2000000);compare("map-choice audio baseline",ranges)
        audio_baseline=[(address,native_bytes(address,size)) for address,size in ((0x02000000,0x40000),(0x03000000,0x8000),(0x04000000,0x400))]
        audio_names={"StartMapChoiceSong23","ConfigureMapChoiceSlot14WithSong70","ScriptCallbackUpdateMapChoiceInputMotion"}
        def fixture(case,audio=False,base=0x02010000):
            fresh()
            if audio:
                for address,payload in audio_baseline:both(address,payload)
            else:both(0x02000000,data)
            both(base,local.randbytes(40*104));both(0x02015000,local.randbytes(16*44));both(0x02016000,local.randbytes(8*80))
            both(0x02014000,bytes(range(40)));both(0x03002D98,struct.pack("<II",base,0x02014000));both(0x03002DC0,b"\x28\x00\x27")
            both(0x03002E2C,struct.pack("<I",0x02014200));both(0x02014200,local.randbytes(40*8))
            both(0x03003070,struct.pack("<I",0x02015000));both(0x03002E10,b"\x10");both(0x03002E08,struct.pack("<I",0x02016000));both(0x03002C80,b"\x08")
            both(0x02000138,struct.pack("<HH",64,64));both(0x03002BFC,struct.pack("<I",0x02024000))
            both(0x02024000,bytes(8192) if case%8==0 else local.randbytes(8192))
            for index in range(40):
                x=(local.randrange(400)<<16)|local.randrange(65536);z=(local.randrange(400)<<16)|local.randrange(65536)
                both(base+index*104+0x18,struct.pack("<III",x,local.getrandbits(32),z))
                both(base+index*104+0x24,struct.pack("<III",local.choice((0,0x800,0x10000,0x18000,0xFFFFF800,0xFFFE8000)),0,local.choice((0,0x8000,0xFFFF8000))))
                both(base+index*104+0x60,bytes([(case+index)%4+1]))
            both(0x02004520,local.randbytes(512));both(0x03002180,local.randbytes(96))
            both(0x03002182,bytes([(case&1)|128]));both(0x03002170,b"\xA5")
            for i,address in enumerate((0x03002C0C,0x03001F50,0x03002C04,0x03002C00)):
                both(address,bytes([(10,13,16,19)[(i+case//8)%4]]))
            if case%16==1:both(0x03001F50,b"\x00")
            if case%8==2:both(0x03001F50,native_bytes(0x03002C0C,1))
            both(0x0201DC40,local.randbytes(128));both(0x0201DC8C,struct.pack("<HH",case%4+1,(case//4)%4+1))
            both(0x02004520+0x6E,bytes([case%4]));both(0x02004520+0x1A0,bytes([case%7+1]))
            both(0x02004520+0x12,bytes([case%40]))
            both(0x02004520+0x28,bytes([(0,22,23,255)[case%4]]));both(0x02004580,bytes([(0,1,2,255)[case%4]]))
            both(0x02004581,bytes([(0,1,255)[case%3]]));both(0x0200459B,bytes([case%3]));both(0x0200454A,bytes([case&1]))
            both(0x0200457C,bytes([(0,1,2,255)[case%4]]));both(0x03001140,bytes([(0,1,255)[case%3]]))
            both(0x03000820,struct.pack("<4H",(0,16,32,64,128,240,48,192)[case%8],0,0,0))
            both(0x02020000,bytes([case%40,case%14,0x6E])+local.randbytes(13))
            if base!=0x02010000:
                both(0x03002D98,struct.pack("<II",base,0x02014000));both(0x03002DC0,b"\x28\x00\x27")
                both(0x03002E2C,struct.pack("<I",0x02014200));both(0x03003070,struct.pack("<I",0x02015000))
                both(0x03002E10,b"\x10");both(0x03002E08,struct.pack("<I",0x02016000));both(0x03002C80,b"\x08")
        def payload(name,case):
            if name=="ScriptCallbackSkipIfMapValuesDiffer":return bytes([case%2])+struct.pack("<HH",case%32,(case//2)%32)
            if name=="ScriptCallbackConfigureMapChoiceRenderSlots":return bytes([(1,2,3,4,8)[case%5]])
            if name=="ScriptCallbackPrepareMapChoiceMotion":return bytes([case%4+1,case%256,0x6E])
            if name in {"ScriptCallbackOrientMapChoicePairToSlot","ScriptCallbackToggleMapChoiceRaisedPosition"}:return bytes([case%40,case%2])
            if name=="ScriptCallbackSelectPackedMapChoice":return bytes([(0,1,2,3,4,7,8,255)[case%8]])
            if name=="ScriptCallbackApproachMapChoiceLeaderPosition":return struct.pack("<HH",case*137%65536,case*193%65536)
            return bytes([case%40,case%14])+local.randbytes(14)
        for row in groups["apply"]:
            for case in range(64):
                name=row["name"];fixture(case,name in audio_names);value=C.c_uint32(99)
                equal(apply_map(memory,row["address"]|1,C.byref(value)),1,f"native map preset {name}/{case}")
                result=call(name,0) if name=="FaceMapChoiceSlot10Toward13" else call(name)
                if signatures[name][0]=="u32":equal(value.value,result,f"map preset return {name}")
                compare(f"map preset {name}/{case}",ranges)
        for row in groups["main"]:
            for case in range(64):
                name=row["name"];fixture(case,name in audio_names);both(0x02020000,payload(name,case));consumed=C.c_uint32(99)
                equal(main_map(memory,row["address"]|1,0x02020000,C.byref(consumed)),1,f"native map Main {name}/{case}")
                equal(consumed.value,call(name,0x02020000),f"map Main consumed {name}")
                compare(f"map Main {name}/{case}",ranges)
        for row in groups["animation"]:
            for case in range(128):
                fixture(case);slot=0x02010000+(case%40)*104;yielded=C.c_uint32(local.getrandbits(32));consumed=C.c_uint32(99)
                both(0x02020000,bytes([case%5,case%256])+local.randbytes(12));both(0x0203FFF0,struct.pack("<I",yielded.value))
                equal(animation_map(memory,row["address"]|1,slot,0x02020000,C.byref(consumed),C.byref(yielded)),1,f"native map animation {row['name']}")
                equal(consumed.value,call(row["name"],slot,0x02020000,0x0203FFF0),"map animation consumed")
                equal(yielded.value,struct.unpack("<I",cpu.mem_read(0x0203FFF0,4))[0],"map animation yield")
                both(0x0203FFF0,struct.pack("<I",yielded.value));compare(f"map animation {row['name']}/{case}",ranges)
        for row in groups["slot"]:
            for case in range(256):
                fixture(case);slot=0x02010000+(case%40)*104;value=C.c_uint32(99)
                both(slot+0x18,local.randbytes(36))
                equal(slot_map(memory,row["address"]|1,slot,C.byref(value)),1,f"native map slot helper {row['name']}")
                result=call(row["name"],slot)
                if signatures[row["name"]][0]=="u32":equal(value.value,result,"map slot predicate")
                compare(f"map slot helper {row['name']}/{case}",ranges)
        from inventory_reference import body as source_body
        unused=[row for row in groups["main"] if not re.search(r"\barguments\b",source_body(source,row["name"]))]
        equal(len(unused),20,"reviewed argument-free task/Main sharing")
        for row in groups["apply"]+unused:
            for case in range(16):
                name=row["name"];fixture(case,name in audio_names);task=0x02019000;script=0x02020000;index=(case*37)%104
                both(task,local.randbytes(52));both(task+0x10,struct.pack("<I",script));both(script,bytes([index,0xE1]))
                both(0x03001FC0+index*4,struct.pack("<I",row["address"]|1))
                equal(task_command(memory,task,0xF4,report),1,f"native map task ABI {name}/{case}")
                equal(report[3],call("TaskCommandF4",task),"map task continuation");compare(f"map task ABI {name}/{case}",ranges)
        main_rows=groups["main"]+[row for row in groups["apply"] if signatures[row["name"]][0]=="u32"]
        equal(len(main_rows),93,"reviewed 93 map Main contracts")
        for row in main_rows:
            for case in range(16):
                name=row["name"];fixture(case,name in audio_names)
                both(0x02020000,struct.pack("<I",row["address"]|1)+payload(name,case));both(0x03000010,b"\x1C"+bytes(15))
                equal(command(memory,0x1C,0x02020000),1,f"native map Main ABI {name}/{case}")
                call("MainScriptInvokeCallback",0x02020000);compare(f"map Main ABI {name}/{case}",ranges)
        # Signed/prospective region boundaries include subpixel edges and word
        # wrap, and distinguish the split at x=128 from the outer rectangle.
        for predicted_x in (0x47FFFF,0x480000,0x480001,0x7FFFFF,0x800000,0x800001,0xB7FFFF,0xB80000,0xB80001):
            for predicted_z in (0xCFFFFF,0xD00000,0xD00001):
                for vx in (-0x8000,-1,0,1,0x8000):
                    for vz in (-1,0,1):
                        fixture(predicted_x);slot=0x02010000
                        both(slot+0x18,struct.pack("<III",(predicted_x-vx)&0xFFFFFFFF,0,(predicted_z-vz)&0xFFFFFFFF))
                        both(slot+0x24,struct.pack("<III",vx&0xFFFFFFFF,0,vz&0xFFFFFFFF));value=C.c_uint32()
                        equal(slot_map(memory,symbols["IsMapChoiceProspectiveSlotRegion"]|1,slot,C.byref(value)),1,"native prospective map region")
                        equal(value.value,call("IsMapChoiceProspectiveSlotRegion",slot),"prospective map region predicate");compare("prospective map region",ranges)
        for previous in range(1,8):
            fixture(previous)
            for choice in range(256):
                both(0x020046C0,bytes([previous]));both(0x02020000,bytes([choice]));consumed=C.c_uint32()
                equal(main_map(memory,symbols["ScriptCallbackSelectPackedMapChoice"]|1,0x02020000,C.byref(consumed)),1,"native packed map choice")
                equal(consumed.value,call("ScriptCallbackSelectPackedMapChoice",0x02020000),"packed map choice consumed");compare("packed map choice",ranges)
        for name in ("ScriptCallbackSaveMapChoicePosition","ScriptCallbackRestoreMapChoicePosition"):
            for index in (0,1,10,13):
                for variable in (0,1,13,14,15,16,122,255):
                    base=0x03002BB0+variable*4-index*104-0x1C;fixture(variable,base)
                    both(0x02020000,bytes([index,variable]));consumed=C.c_uint32()
                    equal(main_map(memory,symbols[name]|1,0x02020000,C.byref(consumed)),1,"native overlapping map position variables")
                    equal(consumed.value,call(name,0x02020000),"overlapping map position consumed");compare("overlapping map position variables",ranges)
        for case in range(64):
            fixture(case);index=case%40;slot=0x02010000+index*104;resource=0x08000101+case*4
            both(slot+4,struct.pack("<I",resource));both(slot+0x60,bytes([case&1]));target=0x02020400+case*5
            stream=struct.pack("<IBII",symbols["ScriptCallbackJumpMapChoiceActiveAnimation"]|1,index,resource if case&2 else resource+4,target)
            both(0x02020000,stream);both(0x03000010,b"\x1C"+bytes(15))
            equal(command(memory,0x1C,0x02020000),1,"native active-animation Main cursor jump")
            call("MainScriptInvokeCallback",0x02020000);compare("active-animation Main cursor jump",ranges)
        positions=((0x687FFF,0x778000),(0x688000,0x778000),(0xC88000,0x808000),(0xC88001,0x808000),
                   (0x688000,0x777FFF),(0x688000,0x808001),(0,0),(0xA08000,0x7C8000))
        for phase in (0,1,2,255):
            for x,z in positions:
                for current in (0x088B503E,0x088B7318,0x08000101):
                    for leader in (1,13):
                        fixture(phase);both(0x03002C0C,bytes([leader]));both(0x0200457C,bytes([phase]))
                        both(0x02010000+leader*104+0x18,struct.pack("<III",x,0,z));both(0x02010000+13*104+4,struct.pack("<I",current))
                        consumed=C.c_uint32(99);yielded=C.c_uint32(0xA5B6C7D8);both(0x0203FFF0,struct.pack("<I",yielded.value))
                        equal(animation_map(memory,symbols["AnimationCallbackUpdateMapChoiceFollowerPhase"]|1,0x02010000,0,C.byref(consumed),C.byref(yielded)),1,"native follower-region phase")
                        equal(consumed.value,call("AnimationCallbackUpdateMapChoiceFollowerPhase",0x02010000,0,0x0203FFF0),"follower-region phase consumed")
                        equal(yielded.value,struct.unpack("<I",cpu.mem_read(0x0203FFF0,4))[0],"follower-region phase yield")
                        both(0x0203FFF0,struct.pack("<I",yielded.value));compare("follower-region phase",ranges)
        for name in ("ScriptCallbackConstrainMapChoiceMotion","ScriptCallbackConstrainMapChoiceZ3196C","ScriptCallbackConstrainMapChoiceUpperX31A20"):
            for x in (0x3C7FFF,0x3C8000,0x467FFF,0x468000,0x957FFF,0x958000,0x958001,0x7FFFFFFF,0x80000000):
                for z in (0x787FFF,0x788000,0x837FFF,0x838000,0x988000,0x988001,0x80000000):
                    for vx in (-0x10000,0x10000):
                        fixture(x+z);both(0x03002C0C,b"\x01");slot=0x02010000+104
                        both(slot+0x18,struct.pack("<III",(x-vx)&0xFFFFFFFF,0,(z-0x8000)&0xFFFFFFFF));both(slot+0x24,struct.pack("<III",vx&0xFFFFFFFF,0,0x8000))
                        consumed=C.c_uint32();equal(main_map(memory,symbols[name]|1,0,C.byref(consumed)),1,"native predicted map motion bounds")
                        equal(consumed.value,call(name,0),"predicted map motion bounds consumed");compare("predicted map motion bounds",ranges)
        for case in range(128):
            fixture(case);both(0x03002C0C,b"\x01");slot=0x02010000+104
            tx=(0,1,32767,32768,65535)[case%5];tz=(0,1,32767,32768,65535)[(case//5)%5]
            delta=(0,1,-1,0x7FFF,0x8000,0x8001,-0x8001,0x7FFFFFFF,0x80000000)
            both(slot+0x18,struct.pack("<III",((tx<<16)+0x8000+delta[case%9])&0xFFFFFFFF,0,((tz<<16)+0x8000+delta[(case//9)%9])&0xFFFFFFFF))
            both(0x02020000,struct.pack("<HH",tx,tz))
            for _ in range(3):
                consumed=C.c_uint32();equal(main_map(memory,symbols["ScriptCallbackApproachMapChoiceLeaderPosition"]|1,0x02020000,C.byref(consumed)),1,"native wrapped map approach")
                equal(consumed.value,call("ScriptCallbackApproachMapChoiceLeaderPosition",0x02020000),"wrapped map approach consumed");compare("wrapped map approach",ranges)
        actor_dependencies=("SetSecondaryAnimationPreset14034","ConfigureRenderSetupPreset1A3B0","ClearRenderSetupNeighborFields6018E2C",
            "ClearRenderSetupNeighborFields6018E90","ConfigureRenderSetupPreset19A0C","ConfigureRenderSetupPreset19FE4","ConfigureRenderSetupPreset1A028",
            "SetDirectionalAnimationPreset17694","SetDirectionalAnimationPreset176AC","SetDirectionalAnimationPreset176C4","SetDirectionalAnimationPreset178D0")
        for name in actor_dependencies:
            for case in range(64):
                fixture(case);equal(preset(memory,symbols[name]|1),1,f"native map actor dependency {name}")
                call(name);compare(f"map actor dependency {name}",ranges)
        for case in range(512):
            fixture(case);a=case%40;b=(case//8)%40
            for eight in (False,True):
                equal(directions(memory,a,b,eight),call("GetRenderXZDirectionBetweenSlots8" if eight else "GetRenderXZDirectionBetweenSlots4",a,b),"native pair direction")
                compare("pair direction",ranges)
            both(0x02004580,bytes([a,b,case%256]));consumed=C.c_uint32(99)
            equal(main_map(memory,symbols["ScriptCallbackSetSlotRelativePlanarVelocity"]|1,0,C.byref(consumed)),1,"native relative planar velocity")
            equal(consumed.value,call("ScriptCallbackSetSlotRelativePlanarVelocity",0),"relative planar velocity consumed");compare("relative planar velocity",ranges)
        particles=[e for e in entries if e["source"]=="src/graphics/map_particles.c"]
        equal(len(particles),4,"complete map-particle family")
        initialize_particles=native("hn_particles_initialize",result=C.c_bool)
        for row in particles:
            for case in range(64):
                fixture(case);both(0x02004580,bytes([case%2,(0,1,13,14)[case%4],0,0])+struct.pack("<I",0x085F4484 if case&1 else 0x085F42F0))
                if row["name"]=="InitializeMapParticleRenderSlots":
                    equal(initialize_particles(memory),True,"native particle initialization");call(row["name"])
                else:
                    consumed=C.c_uint32(99);equal(main_map(memory,row["address"]|1,0,C.byref(consumed)),1,f"native particle callback {row['name']}")
                    equal(consumed.value,call(row["name"],0),"particle callback consumed")
                compare(f"particle {row['name']}/{case}",ranges)
        for x in (0xFFDFFFFF,0xFFE00000,0xFFE00001,0,0x1600000,0x1600001,0x7FFFFFFF,0x80000000):
            for z in (0x1EFFFFF,0x1F00000,0x1F00001,0xFFFFFFFF,0x7FFFFFFF,0x80000000):
                fixture(x+z);consumed=C.c_uint32()
                equal(main_map(memory,symbols["ScriptCallbackStartMapParticleEffect"]|1,0,C.byref(consumed)),1,"native particle bounds setup")
                call("ScriptCallbackStartMapParticleEffect",0);compare("particle bounds setup",ranges)
                index=struct.unpack("<I",native_bytes(0x020012C4,4))[0];slot=0x02010000+index*104
                both(slot+0x18,struct.pack("<III",x,0,z));both(slot+0x30,local.randbytes(12))
                equal(main_map(memory,symbols["ScriptCallbackUpdateMapParticleEffect"]|1,0,C.byref(consumed)),1,"native particle reset boundaries")
                equal(consumed.value,call("ScriptCallbackUpdateMapParticleEffect",0),"particle reset boundaries consumed");compare("particle reset boundaries",ranges)
        print(f"{checks-start_checks} private map-choice/particle comparisons passed (273 map functions and reviewed dependencies).")

    @timed
    def preset_checks():
        start_checks=checks;local=random.Random(0x50525354)
        family=[e for e in entries if e["source"]=="src/scripts/script_callbacks_render_presets.c"]
        dependencies=("ConfigureRenderSetupPreset192A4","ConfigureRenderSetupPreset19584","ConfigureRenderSetupPreset195D8",
                      "ConfigureRenderSetupPreset197D0","ConfigureRenderSetupPreset19878","ConfigureRenderSetupPreset19BB0",
                      "ConfigureRenderSetupPreset1A458","ConfigureRenderSetupPreset1A4AC","ConfigureRenderSetupPreset1A500",
                      "ConfigureRenderSetupPreset1AA10","ConfigureRenderSetupPreset1AAB8","ConfigureRenderSetupPreset1B7A8",
                      "ConfigureRenderSetupPreset1BAFC","SetActorAnimationPreset13384","SetActorClassAnimationPreset136B8",
                      "SetDirectionalAnimationPreset16A6C","SetDirectionalAnimationPreset16BD8","SetDirectionalAnimationPreset1707C",
                      "SetDirectionalAnimationPreset1710C","SetExtendedAnimationPreset1499C","SetExtendedAnimationPreset149E4",
                      "SetGraphicsAnimationPreset1536C","SetGraphicsAnimationPreset1542C","SetGraphicsAnimationPreset1548C",
                      "SetRenderSetupAnimationPreset17C6C","SetRenderSetupAnimationPreset182FC","SetSecondaryAnimationPreset13FEC",
                      "SetSecondaryAnimationPreset14400")
        callbacks=[e for e in family if e["name"].startswith(("ScriptCallback","SceneRenderCallback"))]
        equal((len(family),len(dependencies),len(callbacks)),(77,28,35),"complete render-preset family and dependency inventory")
        preset=bind("hn_preset_apply",C.c_int,C.c_void_p,C.c_uint32)
        task_command=bind("hn_probe_task_command",C.c_int,C.c_void_p,C.c_uint32,C.c_uint,C.POINTER(C.c_uint32));report=(C.c_uint32*4)()
        def fixture(case,base=0x02010000):
            fresh();both(base,local.randbytes(40*104));both(0x02015000,local.randbytes(8*44))
            both(0x02004520,local.randbytes(512));both(0x03002D98,struct.pack("<I",base));both(0x03002DC0,b"\x28")
            both(0x03003070,struct.pack("<I",0x02015000));both(0x03002E10,b"\x08")
            classes=(1,2,10,13,16,19,22,25,30,38)
            for i,address in enumerate((0x03001F50,0x03002C04,0x03002C00)):both(address,bytes([classes[(case+i*3)%len(classes)]]))
            both(0x03002170,b"\xA5");both(0x03002182,bytes([(0,128,255)[case%3]]))
            both(0x020045E6,bytes([(0,6,7,8,255)[case%5]]));both(0x03004ED4,struct.pack("<I",local.getrandbits(32)))
            both(0x03001FAC,bytes([case%40]));both(0x03001F7C,struct.pack("<I",local.getrandbits(32)))
        names=[e["name"] for e in family]+list(dependencies)
        for name in names:
            for case in range(64):
                fixture(case);equal(preset(memory,symbols[name]|1),1,f"native render preset {name}/{case}")
                call(name);compare(f"render preset {name}/{case}",((0x02000000,0x40000),(0x03000000,0x7000)))
        # F4 ignores a callback's return, while Main1C must consume its defined
        # zero. Exercise the two native routes separately, including no-op entries.
        for row in family:
            for case in range(32):
                fixture(case);task=0x02019000;script=0x02020000;index=(case*37)%104
                both(task,local.randbytes(52));both(task+0x10,struct.pack("<I",script));both(script,bytes([index,0xE1]))
                both(0x03001FC0+index*4,struct.pack("<I",row["address"]|1))
                equal(task_command(memory,task,0xF4,report),1,f"native F4 render preset {row['name']}")
                equal(report[3],call("TaskCommandF4",task),"render preset task continuation")
                compare(f"F4 render preset {row['name']}",((0x02000000,0x40000),(0x03000000,0x7000)))
        for row in callbacks:
            for case in range(32):
                fixture(case);both(0x02020000,struct.pack("<I",row["address"]|1));both(0x03000010,b"\x1C"+bytes(15))
                equal(command(memory,0x1C,0x02020000),1,f"native Main render preset {row['name']}")
                call("MainScriptInvokeCallback",0x02020000)
                compare(f"Main render preset {row['name']}",((0x02000000,0x40000),(0x03000000,0x7000)))
        # A configured slot can overwrite its class selector. Its next helper
        # call must reread that byte, not use the original host-local value.
        for name in dependencies:
            if not name.startswith("ConfigureRenderSetupPreset"):continue
            selector=0x03001F50 if name.endswith(("192A4","19584")) else 0x03002C04 if name.endswith(("195D8","197D0","19878","1AA10","1AAB8","1B7A8")) else 0x03002C00
            delta=1 if name.endswith(("1A500","1AAB8","1B7A8")) else -1
            for value in (2,10,13,16,19,22,25,30,38):
                base=selector-(value+delta)*104-0x60;fixture(value,base);both(selector,bytes([value]))
                equal(preset(memory,symbols[name]|1),1,"native live preset class selector")
                call(name);compare("live preset class selector",((0x02000000,0x40000),(0x03000000,0x7000)))
        print(f"{checks-start_checks} private render-preset comparisons passed (77 source functions and 28 actor dependencies).")

    @timed
    def scene_motion_checks():
        start_checks=checks
        local=random.Random(0x53434E45)
        families={"src/actors/class_anchor_presets.c","src/scripts/scene_control_callbacks.c"}
        mains=[e["name"] for e in entries if e["source"] in families and not e["name"].startswith("AnimationCallback")]
        animations=[e["name"] for e in entries if e["source"] in families and e["name"].startswith("AnimationCallback")]
        equal((len(mains),len(animations)),(38,2),"complete scene-motion family inventory")
        random_word=native("hn_random_memory_next",result=C.c_uint32)
        seed_edges=(0,1,30269,30323,0x7FFFFFFF,0x80000000,0xFFFFFFFE,0xFFFFFFFF)
        for a in seed_edges:
            for b in seed_edges:
                for c in seed_edges:
                    fresh();both(0x03000000,struct.pack("<III",a,b,c))
                    equal(random_word(memory),call("NextRandom"),"signed wrapped PRNG result")
                    compare("signed wrapped PRNG seeds",((0x03000000,12),))
        def fixture(case,base=0x02010000):
            fresh();both(0x03002D98,struct.pack("<I",base));both(0x03002DC0,b"\x28")
            both(base,local.randbytes(40*104));both(0x02004520,local.randbytes(512))
            both(0x03002C90,local.randbytes(256));both(0x03001158,struct.pack("<H",local.randrange(65536)))
            both(0x03002C0C,bytes([case%40]));both(0x03001F50,bytes([(case//2)%40]))
            both(0x03002180,local.randbytes(96));both(0x03000000,local.randbytes(12))
            both(0x02004584,bytes([case%10,(0,1,2,255)[case%4]]))
        def invoke(name,payload=b"",address=0x02020000):
            both(address,struct.pack("<I",symbols[name]|1)+payload);both(0x03000010,b"\x1C"+bytes(15))
            equal(command(memory,0x1C,address),1,f"native scene-motion {name}")
            call("MainScriptInvokeCallback",address)
            compare(f"scene-motion {name}",((0x02000000,0x40000),(0x03000000,0x7000)))
        for name in mains:
            for case in range(64):
                fixture(case)
                payload=bytes([case%40,case%8])+local.randbytes(4)
                if name=="ScriptCallbackCheckSceneSlotRectangle":payload=bytes([case%40])+local.randbytes(4)
                invoke(name,payload)
        scene_animation=bind("hn_scene_animation_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32),C.POINTER(C.c_uint32))
        for name in animations:
            for case in range(512):
                fixture(case);yielded=C.c_uint32(local.getrandbits(32));consumed=C.c_uint32(99)
                both(0x0203FFF0,struct.pack("<I",yielded.value))
                equal(scene_animation(memory,symbols[name]|1,0x02010000,0x02020000,C.byref(consumed),C.byref(yielded)),1,"native scene alignment")
                equal(consumed.value,call(name,0x02010000,0x02020000,0x0203FFF0),"scene alignment consumption")
                equal(yielded.value,struct.unpack("<I",cpu.mem_read(0x0203FFF0,4))[0],"scene alignment yield")
                both(0x0203FFF0,struct.pack("<I",yielded.value))
                compare("scene alignment",((0x02000000,0x40000),(0x03000000,0x7000)))
        # Dense contacts include strict edges, wrapped halfwords, midpoint rounding,
        # absent/aliased actor classes and independent scene-outside flags.
        for case in range(1536):
            fixture(case);leader=(0,1,26)[case%3];follower=(0,2,26)[(case//3)%3]
            both(0x03002C0C,bytes([leader]));both(0x03001F50,bytes([follower]))
            x=(-32768,-48,-1,0,16,100,216,320,32767)[case%9];z=(-32768,-8,0,100,216,217,32767)[(case//9)%7]
            for index,ox,oz in ((leader,0,0),(follower,(case%17)-8,(case%9)-4),(26,(-29,-28,-27,0,15,16,17)[case%7],(-9,-8,-7,0,7,8,9)[(case//7)%7])):
                both(0x02010000+index*104+0x18,struct.pack("<III",((x+ox)<<16)&0xFFFFFFFF,local.getrandbits(32),((z+oz)<<16)&0xFFFFFFFF))
            invoke("ScriptCallbackCheckSceneActorRectangles")
            invoke("ScriptCallbackCheckSceneCorridor")
        # Shared state can overlay render data, and callback payloads can point at
        # fields they mutate. Preserve source read/write order in these cases.
        for case in range(128):
            for name in ("ScriptCallbackCheckSceneActorRectangles","ScriptCallbackCheckSceneSlotRectangle","ScriptCallbackConfigureSceneCloneSources","ScriptCallbackCopySceneClonePair","ScriptCallbackCheckSceneCorridor"):
                fixture(case,0x02004580-(case%4)*104)
                payload=bytes([case%40,128,128,127,127]);invoke(name,payload)
        direct=bind("hn_scene_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
        for case in range(256):
            fixture(case);both(0x030021E0,bytes([case]));consumed=C.c_uint32(99)
            equal(direct(memory,symbols["ScriptCallbackSetSceneSignedSaveValue"]|1,0x030021E0,C.byref(consumed)),1,"native aliased signed scene result")
            equal(consumed.value,call("ScriptCallbackSetSceneSignedSaveValue",0x030021E0),"signed scene result consumption")
            compare("aliased signed scene result",((0x02000000,0x40000),(0x03000000,0x7000)))
        for low in range(256):
            for high_byte in (0,1,127,128,255):
                fixture(low);invoke("ScriptCallbackSetSceneBackgroundOffsetBytes",bytes([low,high_byte]))
        for case in range(256):
            fixture(case)
            invoke("ScriptCallbackConfigureSceneCloneSources")
            for _ in range(8):invoke("ScriptCallbackCopySceneClonePair")
        edges=(0,1,2,31,32,63,64,65,66,0x7FFF,0x8000,0x8001,0xFF88,0xFF89,0xFF8A,0xFFE0,0xFFE1,0xFFE2,0xFFFE,0xFFFF)
        for value in edges:
            for flag_value in (0,1,255):
                fixture(value);both(0x03001158,struct.pack("<H",value));both(0x02004582,bytes([flag_value]))
                invoke("ScriptCallbackOscillateSceneBackgroundOffset");invoke("ScriptCallbackApproachSceneBackgroundOffset")
        print(f"{checks-start_checks} private scene-motion comparisons passed across two complete source families (40 functions).")

    @timed
    def script_memory_checks():
        local=random.Random(0x414248)
        def invoke_transition(name,payload=b""):
            both(0x02020000,struct.pack("<I",symbols[name]|1)+payload);both(0x03000010,b"\x1C"+bytes(15))
            equal(command(memory,0x1C,0x02020000),1,f"native transition {name}")
            call("MainScriptInvokeCallback",0x02020000)
            compare(f"transition {name}",((0x02000000,0x40000),(0x03000000,0x7000)))
        modes=native("hn_render_modes",4,C.c_bool)
        scene_animation=bind("hn_scene_animation_callback",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32),C.POINTER(C.c_uint32))
        for case in range(1024):
            fresh();both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x28")
            both(0x02010000,local.randbytes(40*104));index=case%40;mode=case%256;value=case//256;extra=(case*37)&255
            equal(modes(memory,index,mode,value,extra),True,"native render mode bytes")
            call("SetRenderModeBytes",index,mode,value,extra)
            compare("render mode bytes",((0x02000000,0x40000),(0x03000000,0x7000)))
        for case in range(1024):
            fresh();both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x28")
            both(0x02010000,local.randbytes(40*104));both(0x02004520,local.randbytes(512));both(0x03002180,local.randbytes(96))
            index=case%4;source=(case//4)%4;leader=(case//16)%4
            both(0x03002C0C,bytes([case%4]));both(0x03001F50,bytes([9 if case&1 else case%4]))
            both(0x02004604,bytes([leader]));both(0x0201DCAE,struct.pack("<H",(1,2,16,32,255,32768,65535)[case%7]))
            bounds=((0,384,0,384),(200,0,200,0),(-32768,32767,-32768,32767),(128,512,-128,512))[case%4]
            both(0x030034B0,struct.pack("<4h",*bounds))
            invoke_transition("ScriptCallbackApproachClampedLeader",bytes([index,source]))
            invoke_transition("ScriptCallbackTransferLeaderRenderModes")
            both(0x03002BB0,struct.pack("<II",0x08000100,0x08000110))
            invoke_transition("ScriptCallbackInitializeLinkedScreenSlot9")
            invoke_transition("ScriptCallbackSetLinkedScreenSlot9Animation")
            offset=case%256;both(0x02004520+offset,bytes([(case//16)%4]))
            invoke_transition("ScriptCallbackCompareRenderZ",bytes([case%4,offset]))
            both(0x03002BF0,struct.pack("<I",local.getrandbits(32)))
            for name in ("ScriptCallbackSaveField2BF0","ScriptCallbackReturn0_11EA4","ScriptCallbackReturn0_11EEC","ScriptCallbackReturn0_11EF0","ScriptCallbackReturn0_11F08"):
                invoke_transition(name)
            if case<64:
                for name in ("ScriptCallbackTransferLeaderRenderModes","ScriptCallbackInitializeLinkedScreenSlot9","ScriptCallbackSetLinkedScreenSlot9Animation",
                             "ScriptCallbackSaveField2BF0","ScriptCallbackReturn0_11EA4","ScriptCallbackReturn0_11EEC","ScriptCallbackReturn0_11EF0","ScriptCallbackReturn0_11F08"):
                    consumed=C.c_uint32(0xA5);yielded=C.c_uint32(local.getrandbits(32));both(0x0203FFF0,struct.pack("<I",yielded.value))
                    equal(scene_animation(memory,symbols[name]|1,0x02010000,0x02020000,C.byref(consumed),C.byref(yielded)),1,"native shared callback signature")
                    equal(consumed.value,call(name,0x02010000,0x02020000,0x0203FFF0),"shared callback consumption")
                    equal(yielded.value,struct.unpack("<I",cpu.mem_read(0x0203FFF0,4))[0],"shared callback retains yield")
                    compare("shared callback signature",((0x02000000,0x40000),(0x03000000,0x7000)))
        height=native("hn_motion_map_height",1,C.c_bool);fresh()
        edges_x=(0x00407FFF,0x00408000,0x005C8000,0x005C8001,0x007C7FFF,0x007C8000,0x00948000,0x00948001)
        edges_z=(0x00407FFF,0x00408000,0x00768000,0x00768001,0x00877FFF,0x00878000,0x00BF8000,0x00BF8001)
        for x in edges_x:
            for z in edges_z:
                for flags in range(4):
                    for control in (0,1,255):
                        for y in (-0x80000000,-0x88001,-0x88000,-0x87FFF,-0x80000,0,0x7FFF,0x8000,0x7FFFFFFF):
                            both(0x02010018,struct.pack("<III",x,y&0xFFFFFFFF,z));both(0x03002194,bytes([2 if flags&1 else 0]))
                            both(0x030021A2,bytes([2 if flags&2 else 0]));both(0x02004544,bytes([control]))
                            equal(height(memory,0x02010000),True,"native map surface height")
                            call("UpdateMapChoiceRenderHeight",0x02010000)
                            compare("map surface height",((0x02000000,0x40000),(0x03000000,0x7000)))
        animation=bind("hn_probe_animation",C.c_int,C.c_void_p,C.c_uint32,C.c_uint32,C.c_uint32,C.POINTER(C.c_uint32))
        report=(C.c_uint32*4)()
        for case in range(256):
            fresh();both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x04")
            both(0x02010000,local.randbytes(4*104));both(0x03002C0C,bytes([case%4]));both(0x03001F50,bytes([(case//4)%4]))
            both(0x03002182,bytes([(case//16)%2]));both(0x03002194,bytes([(case//32)%2*2]))
            both(0x030021A2,bytes([(case//64)%2*2]));both(0x02004544,bytes([(case//128)%2]))
            for name in ("AnimationCallbackSetMapChoiceTargetVelocity","AnimationCallbackUpdateMapChoiceClassHeights"):
                both(0x02020000,b"\x8D"+struct.pack("<I",symbols[name]|1));both(0x03007010,struct.pack("<I",0x02020000))
                equal(animation(memory,0x02010000,0x8D,0x03007010,report),1,f"native map animation {name}")
                equal(report[3],call("AnimationInvokeCallback",0x02010000,0x03007010),"map animation yield")
                compare("map animation",((0x02000000,0x40000),(0x03000000,0x7000),(0x03007010,4)))
        for case in range(3072):
            fresh();both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x04")
            both(0x02010000,local.randbytes(4*104));both(0x0201DC40,local.randbytes(96))
            leader=case%4;follower=(case//4)%4
            both(0x03002C0C,bytes([leader]));both(0x03001F50,bytes([follower]))
            pivot=(0,1,64,128,255)[case%5];begin=(0,pivot,(pivot+1)&255,255,local.randrange(256))[(case//5)%5]
            end=(0,pivot,(pivot+1)&255,255,local.randrange(256))[(case//25)%5]
            enabled=(0,1,2,255)[(case//125)%4]
            both(0x02004580,bytes([pivot,begin,end,local.randrange(256),local.randrange(256),enabled]))
            x=(0,1,63,64,127,128,255,256,32767,32768,65535,local.randrange(65536))[case%12]
            slot=0x02010000+(follower if enabled else leader)*104
            both(slot+0x18,struct.pack("<I",(x<<16)|local.randrange(65536)))
            invoke_transition("ScriptCallbackConfigureSceneActorMotion",bytes([(0,1,255)[case%3]]))
            both(0x03002182,bytes([case%2]));invoke_transition("ScriptCallbackRestoreSceneActorPositions")
        scene_actor=native("hn_motion_scene_actor",1,C.c_bool)
        for offset in (0,2,4):
            for case in range(24):
                fresh();position=0x02004580+offset
                both(0x03002D98,struct.pack("<I",position-0x18));both(0x03002DC0,b"\x04")
                control=bytes([128,3,4 if offset==0 else 200,0 if offset==0 else 32,10,0 if offset==2 else 1,10,0])
                both(0x02004580,control);both(0x02020000,bytes([case%2]))
                arguments=(0x02020000,0x0201DC84,0x0201DC86)[case%3]
                both(arguments,bytes([case%2]))
                equal(scene_actor(memory,arguments),True,"native live scene-control aliases")
                equal(call("ScriptCallbackConfigureSceneActorMotion",arguments),1,"scene-control consumed byte")
                compare("live scene-control aliases",((0x02000000,0x40000),(0x03000000,0x7000)))
        for case in range(1024):
            fresh();both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x28")
            index=(0,1,2,39)[case%4];both(0x03002C0C,bytes([index]));both(0x02010000+index*104,local.randbytes(104))
            both(0x02010000+index*104+0x60,bytes([case%256]));invoke_transition("ScriptCallbackReadActorDirectionAndXZ")
        for case in range(768):
            fresh();both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x28")
            both(0x02010000,local.randbytes(40*104));both(0x0201DC40,local.randbytes(96))
            both(0x02004520,local.randbytes(256));both(0x03002180,local.randbytes(96))
            leader=case%3;follower=(case//3)%3
            both(0x03002C0C,bytes([leader]));both(0x03001F50,bytes([follower]))
            both(0x03002274,bytes([(0,leader,follower,39)[case%4]]));both(0x03002270,bytes([(0,follower,leader,39)[(case//4)%4]]))
            both(0x03001F60,bytes([(case//8)%2]));both(0x03001F90,bytes([(case//16)%2]));both(0x03002170,b"\xA5")
            both(0x020000F8,rom[0x58BF68:0x58BF68+16]);both(0x03000820,struct.pack("<H",2 if case&1 else 0))
            both(0x02004531,bytes([(0,1,2,4,7,8,255)[case%7]]));both(0x02004580,bytes([(case//32)%2]))
            for name,payload in (("ScriptCallbackRepositionLeaderAndSelectAnimations",bytes([case%2])),
                                 ("ScriptCallbackRepositionFollowerAndSelectAnimations",b"")):
                invoke_transition(name,payload)
        for case in range(1024):
            fresh();both(0x0201DC40,local.randbytes(96))
            numerator=(0,1,32767,32768,65535,local.randrange(65536))[case%6]
            denominator=(0,1,2,32767,32768,65535,local.randrange(65536))[case%7]
            both(0x0201DC8E,struct.pack("<HHH",denominator,local.randrange(65536),numerator))
            invoke_transition("ScriptCallbackInterpolateClassFacing",bytes([(0,1,255)[case%3]]))
        for bottom in (-32768,-1,0,1,142,143,32767):
            for z in (-32768,-1,0,1,70,71,32767):
                fresh();both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x28")
                both(0x02004520,local.randbytes(512));both(0x02010000+10*104+0x22,struct.pack("<h",z))
                both(0x030034B6,struct.pack("<h",bottom))
                both(0x02020000,struct.pack("<IBB",symbols["ScriptCallbackStoreSlot10ZHalfSide"]|1,local.randrange(256),local.randrange(256)))
                both(0x03000010,b"\x1C"+bytes(15));equal(command(memory,0x1C,0x02020000),1,"native slot10 half side")
                call("MainScriptInvokeCallback",0x02020000);compare("slot10 half side",((0x02000000,0x40000),(0x03000000,0x7000)))
        for direction in range(256):
            for actor in (1,0x80,0x81):
                fresh();both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x28")
                both(0x02004520,local.randbytes(512));both(0x03002C0C,b"\x01");secondary=2 if direction&1 else 0
                both(0x03001F50,bytes([secondary]));index=secondary if actor==0x81 else 1
                both(0x02010000+index*104+0x60,bytes([direction&7]));offset=(direction+actor)&255
                both(0x02004520+offset,bytes([direction,local.randrange(256),0xA5,0xE7]))
                both(0x02020000,struct.pack("<IBB",symbols["ScriptCallbackUpdateActorTurnControl"]|1,offset,actor))
                both(0x03000010,b"\x1C"+bytes(15));equal(command(memory,0x1C,0x02020000),1,"native actor turn")
                call("MainScriptInvokeCallback",0x02020000);compare("actor turn",((0x02000000,0x40000),(0x03000000,0x7000)))
        for primary_y in (0,63,64,65,70,71,72,79,32768,65535):
            for index in (0,7,8,39):
                for secondary in (0,2):
                    for selected_y in (0,71,72,65535):
                        fresh();both(0x03002D98,struct.pack("<I",0x02010000));both(0x03002DC0,b"\x28")
                        both(0x02010000,local.randbytes(40*104));both(0x03002C0C,b"\x01");both(0x03001F50,bytes([secondary]))
                        both(0x02010000+104+0x50,struct.pack("<H",primary_y))
                        both(0x02010000+2*104+0x50,struct.pack("<H",local.choice((0,71,100,65535))))
                        both(0x02010000+index*104+0x50,struct.pack("<H",selected_y));both(0x02004532,bytes([index]))
                        for name in ("ScriptCallbackReadClassScreenYSide","ScriptCallbackStoreClassScreenYSide"):
                            both(0x02004580,b"\xA5");both(0x03001FBC,b"\xAB\xCD")
                            both(0x02020000,struct.pack("<I",symbols[name]|1));both(0x03000010,b"\x1C"+bytes(15))
                            equal(command(memory,0x1C,0x02020000),1,f"native screen side {name}")
                            call("MainScriptInvokeCallback",0x02020000)
                            compare(f"screen side {name}",((0x02000000,0x40000),(0x03000000,0x7000)))
        for opcode,name,width,base in ((0x41,"MainScriptCopyPointerBytes",1,0x02004520),
                                       (0x42,"MainScriptCopyPointerHalfwords",2,0x0201DC40)):
            equal(struct.unpack_from("<I",rom,0x58E5D4+opcode*4)[0]&~1,symbols[name],f"Main dispatch {opcode:02X}")
            for case in range(120):
                fresh();count=(0,1,2,7,31,255)[case%6];index=(3,65535,32)[case%3]
                destination=base+index*width
                source=(0x02024000,destination-width,destination+width,0x030005E8)[case%4]
                pointer=15 if case&1 else source
                both(0x02024000,local.randbytes(512));both(destination-width,local.randbytes(514))
                both(0x030005E8,local.randbytes(12));both(0x03002BEC,struct.pack("<I",source))
                both(0x02020000,struct.pack("<BHI",count,index,pointer))
                both(0x03000010,bytes([opcode])+local.randbytes(15))
                equal(command(memory,opcode,0x02020000),1,f"native pointer copy {opcode:02X} {case}")
                call(name,0x02020000)
                compare(f"pointer copy {opcode:02X} {case}",((0x02000000,0x40000),(0x03000000,0x7000)))

        for enabled in range(32):
            for layer in (0,1,2,3,4,5,31,32,255):
                fresh();both(0x03002CC5,bytes([enabled|(local.randrange(8)<<5)]));both(0x03002180,local.randbytes(8))
                both(0x02020000,struct.pack("<IB",symbols["ScriptCallbackSetGameFlag13FromDisplayLayer"]|1,layer))
                both(0x03000010,bytes([0x1C])+local.randbytes(15))
                equal(command(memory,0x1C,0x02020000),1,f"native display-layer flag {enabled} {layer}")
                call("MainScriptInvokeCallback",0x02020000)
                compare(f"display-layer flag {enabled} {layer}",((0x03000010,16),(0x030005E8,12),(0x03002180,8),(0x03002CC4,2)))
        for count in (0,1,8,255):
            for kept in (0,max(0,count-1),255):
                fresh();both(0x03002E10,bytes([count]));both(0x03003070,struct.pack("<I",0x02010000))
                both(0x02010000,local.randbytes(count*44+1))
                both(0x02020000,struct.pack("<IB",symbols["ScriptCallbackDisableGraphicsScriptsExcept"]|1,kept))
                both(0x03000010,bytes([0x1C])+local.randbytes(15))
                equal(command(memory,0x1C,0x02020000),1,f"native disable graphics except {count} {kept}")
                call("MainScriptInvokeCallback",0x02020000)
                compare(f"disable graphics except {count} {kept}",((0x02000000,0x40000),(0x03000000,0x7000)))

        transitions=(("ScriptCallbackSaveActorsAndChangeMap10_4","ScriptCallbackRestoreActorsAndPreviousMapC2A9C"),
                     ("ScriptCallbackSaveActorsAndChangeMap10_5","ScriptCallbackRestoreActorsAndPreviousMapC3B20"),
                     ("ScriptCallbackSaveActorsAndChangeMap10_10","ScriptCallbackRestoreActorsAndPreviousMap"),
                     ("ScriptCallbackSaveActorsAndChangeMap10_11","ScriptCallbackRestoreActorsAndPreviousMapCBA34"),
                     ("ScriptCallbackSaveActorsAndChangeMap10_12","ScriptCallbackRestoreActorsAndPreviousMapCC6DC"),
                     ("ScriptCallbackSaveActorsAndChangeMap10_17","ScriptCallbackRestoreActorsAndPreviousMapCEEA8"))
        for save_name,restore_name in transitions:
            for case in range(16):
                fresh();heap_init(memory);call("InitializeHeap",0)
                equal(native("hn_map_initialize",2,C.c_bool)(memory,10,10),True,"actor snapshot map setup")
                call("InitializeMapDescriptor",10,10)
                slots=struct.unpack("<I",native_bytes(0x03002D98,4))[0]
                both(slots,local.randbytes(16*104));both(0x02002EB0,local.randbytes(624))
                both(0x03001F60,bytes([case&1]));both(0x03001F90,bytes([(case>>1)&1]))
                both(0x03002182,bytes([128|(case&127)]))
                indices=(1,0 if case&8 else 2,3,4)
                for i,index in enumerate(indices):
                    both((0x03002C0C,0x03002274,0x03001F50,0x03002270)[i],bytes([index]))
                    active=i==0 or case&(1<<(i-1))
                    both(slots+index*104+96,bytes([((case+i)%4)+1 if active else 0]))
                for name in (save_name,restore_name):
                    both(0x02020000,struct.pack("<I",symbols[name]|1));both(0x03000010,bytes([0x1C])+bytes(15))
                    both(0x03002C60,struct.pack("<I",0x02020111))
                    equal(command(memory,0x1C,0x02020000),1,f"native actor map callback {name} {case}")
                    call("MainScriptInvokeCallback",0x02020000,limit=1000000)
                    compare(f"actor map callback {name} {case}",((0x02000000,0x40000),(0x03000000,0x7000)))

    @timed
    def bank2_checks():
        from reference_bank2 import check_bank2
        check_bank2(entries=entries,reference=args.reference,rom=rom,memory=memory,cpu=cpu,
                    bind=bind,call=call,both=both,fresh=fresh,equal=equal,compare=compare,
                    native_bytes=native_bytes,command=command)

    @timed
    def wander_checks():
        from reference_wander import check_wander
        check_wander(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,
                     both=both,fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)

    @timed
    def scene_actor_checks():
        from reference_scene_actor import check_scene_actors
        check_scene_actors(entries=entries,reference=args.reference,rom=rom,memory=memory,cpu=cpu,
                           bind=bind,call=call,both=both,fresh=fresh,equal=equal,compare=compare,
                           native_bytes=native_bytes,command=command)

    @timed
    def actor_preset_checks():
        from reference_actor_presets import check_actor_presets
        check_actor_presets(entries=entries,reference=args.reference,rom=rom,memory=memory,cpu=cpu,
                            bind=bind,call=call,both=both,fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes)

    @timed
    def record_checks():
        from reference_records import check_records
        check_records(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,
                      both=both,fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)

    @timed
    def wave_checks():
        from reference_wave import check_wave
        check_wave(entries=entries,rom=rom,memory=memory,bind=bind,call=call,both=both,
                   fresh=fresh,equal=equal,compare=compare,command=command)

    @timed
    def counter_scene_checks():
        from reference_counter_scene import check_counter_scene
        check_counter_scene(entries=entries,rom=rom,memory=memory,bind=bind,call=call,both=both,
                            fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)

    @timed
    def shared_motion_checks():
        from reference_shared_motion import check_shared_motion
        check_shared_motion(entries=entries,rom=rom,memory=memory,bind=bind,call=call,both=both,
                            fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)

    @timed
    def actor_animation_checks():
        from reference_actor_presets import check_actor_presets
        check_actor_presets(entries=entries,reference=args.reference,rom=rom,memory=memory,cpu=cpu,
                            bind=bind,call=call,both=both,fresh=fresh,equal=equal,compare=compare,
                            native_bytes=native_bytes,animation_families=True)

    @timed
    def actor_sequence_checks():
        from reference_actor_presets import check_actor_presets
        check_actor_presets(entries=entries,reference=args.reference,rom=rom,memory=memory,cpu=cpu,
                            bind=bind,call=call,both=both,fresh=fresh,equal=equal,compare=compare,
                            native_bytes=native_bytes,sequence_family=True)

    @timed
    def bank3_late_checks():
        from reference_bank3 import check_bank3_late
        check_bank3_late(entries=entries,reference=args.reference,rom=rom,memory=memory,cpu=cpu,
                        bind=bind,call=call,both=both,fresh=fresh,equal=equal,compare=compare,
                        native_bytes=native_bytes,command=command)

    @timed
    def bank3_preset_checks():
        from reference_bank3 import check_bank3_presets
        check_bank3_presets(entries=entries,reference=args.reference,rom=rom,memory=memory,cpu=cpu,
                            bind=bind,call=call,both=both,fresh=fresh,equal=equal,compare=compare,
                            native_bytes=native_bytes)

    @timed
    def bank3_game_checks():
        from reference_bank3 import check_bank3_games
        check_bank3_games(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                          fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)

    @timed
    def bank3_chain_checks():
        from reference_bank3 import check_bank3_chain
        check_bank3_chain(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                          fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)

    @timed
    def bank4_checks(presets_only=False,dynamics_only=False):
        from reference_bank4 import check_bank4
        check_bank4(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                    fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command,presets_only=presets_only,dynamics_only=dynamics_only)

    @timed
    def bank5_checks(presets_only=False,dynamics_only=False):
        from reference_bank5 import check_bank5
        check_bank5(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                    fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command,presets_only=presets_only,dynamics_only=dynamics_only)

    @timed
    def bank6_checks(presets_only=False,dynamics_only=False):
        from reference_bank6 import check_bank6
        check_bank6(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                    fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command,presets_only=presets_only,dynamics_only=dynamics_only)

    @timed
    def bank6_chain_checks():
        from reference_bank6_chain import check_bank6_chain
        check_bank6_chain(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                         fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)

    @timed
    def presentation_checks():
        from reference_presentation import check_presentation
        check_presentation(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                           fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)

    @timed
    def bank7_checks():
        from reference_bank7 import check_bank7
        check_bank7(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                     fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)

    @timed
    def record_selection_checks():
        from reference_record_selection import check_record_selection
        check_record_selection(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                               fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)

    @timed
    def scene_effects_checks():
        from reference_shared_motion import check_scene_effects
        check_scene_effects(entries=entries,rom=rom,memory=memory,bind=bind,call=call,both=both,
                           fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)

    @timed
    def group3_checks():
        from reference_group3 import check_group3,check_group3_scenes
        for check in (check_group3,check_group3_scenes):
            check(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                  fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)
        check_group3_scenes(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                            fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command,record_menus=True)
        check_group3_scenes(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                            fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command,selection_scene=True)

    if args.bios_oracle:
        from compare_bios import BiosOracle
        bios_oracle=BiosOracle(args.bios_oracle,args.oracle_runtime_dir)
    try:
        if args.record_selection_only:
            record_selection_checks()
            print(f"{checks} private record selection/counter/transaction comparisons passed; full activities remain unverified.")
            return
        if args.bank7_only:
            bank7_checks()
            print(f"{checks} private bank-7 comparisons passed; complete puzzle/story remains unverified.")
            return
        if args.presentation_only:
            presentation_checks()
            print(f"{checks} private presentation comparisons passed; complete story remains unverified.")
            return
        if args.bank6_chain_only:
            bank6_chain_checks()
            print(f"{checks} private bank-6 chain comparisons passed; complete story remains unverified.")
            return
        if args.bank6_only or args.bank6_presets_only or args.bank6_dynamics_only:
            bank6_checks(args.bank6_presets_only,args.bank6_dynamics_only)
            print(f"{checks} private bank-6 comparisons passed; complete story remains unverified.")
            return
        if args.bank5_only or args.bank5_presets_only or args.bank5_dynamics_only:
            bank5_checks(args.bank5_presets_only,args.bank5_dynamics_only)
            print(f"{checks} private bank-5 comparisons passed; complete story remains unverified.")
            return
        if args.bank4_only or args.bank4_presets_only or args.bank4_dynamics_only:
            bank4_checks(args.bank4_presets_only,args.bank4_dynamics_only)
            print(f"{checks} private bank-4 comparisons passed; complete story remains unverified.")
            return
        if args.selection_scenes_only:
            from reference_group3 import check_group3_scenes
            check_group3_scenes(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                                fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command,selection_scene=True)
            print(f"{checks} private selection-scene comparisons passed; complete story remains unverified.")
            return
        if args.record_menus_only:
            from reference_group3 import check_group3_scenes
            check_group3_scenes(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                                fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command,record_menus=True)
            print(f"{checks} private record-menu comparisons passed; complete story remains unverified.")
            return
        if args.actor_sequences_only:
            actor_sequence_checks()
            print(f"{checks} private actor-sequence comparisons passed; complete story remains unverified.")
            return
        if args.group3_scenes_only:
            from reference_group3 import check_group3_scenes
            check_group3_scenes(entries=entries,rom=rom,memory=memory,cpu=cpu,bind=bind,call=call,both=both,
                                fresh=fresh,equal=equal,compare=compare,native_bytes=native_bytes,command=command)
            print(f"{checks} private SaveGroup3 scene comparisons passed; complete story remains unverified.")
            return
        if args.group3_only:
            group3_checks()
            print(f"{checks} private SaveGroup3 comparisons passed; complete story remains unverified.")
            return
        if args.bank3_chain_only:
            bank3_chain_checks()
            print(f"{checks} private bank-3 chain comparisons passed; complete story remains unverified.")
            return
        if args.bank3_games_only:
            bank3_game_checks()
            print(f"{checks} private bank-3 game comparisons passed; complete story remains unverified.")
            return
        if args.bank3_presets_only:
            bank3_preset_checks()
            print(f"{checks} private bank-3 preset comparisons passed; complete story remains unverified.")
            return
        if args.bank3_late_only:
            bank3_late_checks()
            print(f"{checks} private bank-3 late comparisons passed; complete story remains unverified.")
            return
        if args.actor_animations_only:
            actor_animation_checks()
            print(f"{checks} private actor-animation comparisons passed; complete story remains unverified.")
            return
        if args.scene_effects_only:
            scene_effects_checks()
            print(f"{checks} private scene-effect comparisons passed; complete story remains unverified.")
            return
        if args.shared_motion_only:
            shared_motion_checks()
            print(f"{checks} private shared geometry comparisons passed; complete story remains unverified.")
            return
        if args.counter_scenes_only:
            counter_scene_checks()
            print(f"{checks} private counter-scene comparisons passed; complete story remains unverified.")
            return
        if args.records_only:
            record_checks()
            print(f"{checks} private record comparisons passed; complete story remains unverified.")
            return
        if args.wave_only:
            wave_checks()
            print(f"{checks} private scanline-wave comparisons passed; full raster timing remains unverified.")
            return
        if args.actor_presets_only:
            actor_preset_checks()
            print(f"{checks} private actor-preset comparisons passed; complete story remains unverified.")
            return
        if args.scene_actors_only:
            scene_actor_checks()
            print(f"{checks} private scene actor comparisons passed; complete story remains unverified.")
            return
        if args.wander_only:
            wander_checks()
            print(f"{checks} private wandering comparisons passed; complete story remains unverified.")
            return
        if args.bank2_only:
            bank2_checks()
            print(f"{checks} private bank-2 comparisons passed; complete story remains unverified.")
            return
        if args.map_choice_only:
            map_choice_checks()
            print(f"{checks} private map-choice checks passed; complete story remains unverified.")
            return
        if args.presets_only:
            preset_checks()
            print(f"{checks} private render-preset checks passed; complete story remains unverified.")
            return
        if args.indicators_only:
            indicator_checks()
            print(f"{checks} private destination-indicator comparisons passed; full gameplay remains unverified.")
            return
        if args.book_only:
            book_checks()
            print(f"{checks} private Ham-Chat acquisition/page comparisons passed; full gameplay remains unverified.")
            return
        if args.scripts_only:
            scene_motion_checks();script_memory_checks()
            print(f"{checks} private Main memory/scene-transition comparisons passed; full gameplay remains unverified.")
            return
        if args.collisions_only:
            collision_checks()
            print(f"{checks} private leader/collision comparisons passed; full gameplay remains unverified.")
            print(f"{bios_oracle.calls} independent BIOS angular responses used.")
            return
        if args.motion_only:
            motion_checks();motion_live_checks()
            print(f"{checks} private motion comparisons passed; leader movement and full gameplay remain unverified.")
            return
        if args.choices_only:
            choice_checks()
            print(f"{checks} private word-choice comparisons passed; full gameplay remains unverified.")
            return
        if args.tasks_only:
            task_checks()
            print(f"{checks} private text-task comparisons passed; full gameplay remains unverified.")
            return
        if args.actors_only:
            actor_checks()
            motion_checks()
            print(f"{checks} private actor/motion comparisons passed; live movement and gameplay remain unverified.")
            return
        if args.audio_only:
            audio_checks();region_audio_checks()
            print(f"{checks} private audio comparisons passed; hardware output and gameplay remain unverified.")
            return
        if args.scenes_only:
            scene_checks()
            text_checks()
            counter_checks()
            raster_checks()
            print(f"{checks} private scene-menu/input comparisons passed; full gameplay remains unverified.")
            return
        indicator_checks();book_checks();bank2_checks();bank3_late_checks();bank3_preset_checks();bank3_game_checks();bank3_chain_checks();bank4_checks();bank5_checks();bank6_checks();bank6_chain_checks();presentation_checks();bank7_checks();record_selection_checks();group3_checks();wander_checks();scene_actor_checks();shared_motion_checks();scene_effects_checks();actor_preset_checks();actor_animation_checks();actor_sequence_checks();record_checks();wave_checks();counter_scene_checks();map_choice_checks();preset_checks();scene_motion_checks();script_memory_checks()
        for line in (0,159,227):
            fresh();both(0x04000006,struct.pack("<H",line))
            equal(native("hn_game_hardware_initialize",result=C.c_bool)(memory),True,f"hardware initialization {line}")
            call("InitializeHardware")
            compare(f"hardware initialization {line}",((0x02000000,0x40000),(0x03000000,0x7B00),
                                                       (0x03007FFC,4),(0x04000000,0x400)))
        fresh();heap_init(memory)
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
        both(0x03000820, bytes(26))
        native("hn_input_initialize")(memory)
        call("InitializeKeys")
        sequence = [1] * 180 + [0] * 20 + [0x50] * 100 + [rng.randrange(1024) for _ in range(200)]
        for i, buttons in enumerate(sequence):
            keys_step(C.byref(keys), buttons)
            cpu.mem_write(0x04000130, struct.pack("<H", (~buttons) & 1023))
            call("ReadKeys")
            equal(native("hn_input_update", 1, C.c_bool)(memory, buttons), True, f"guest input success {i}")
            compare(f"guest input layout {i}", ((0x03000820, 26), (0x04000130, 2)))
            original = struct.unpack("<12H", bytes(cpu.mem_read(0x03000820, 24)))
            equal((keys.held, keys.pressed, keys.repeat, keys.released,
                   keys.first_delay, keys.next_delay, keys.counter, keys.held_frames),
                  (original[0], original[1], original[2], original[3],
                   original[6], original[7], original[8], original[9]), f"input {i}")
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
            (0x1C, "MainScriptInvokeCallback", struct.pack("<IB", 0x080C0129, 3)),
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
        def prepare_core(with_audio=False):
            prepare_save()
            native("hn_video_state_init")(memory)
            call("InitializeVideoState")
            if with_audio:
                equal(native("hn_audio_driver_init", result=C.c_bool)(memory), True, "coupled audio initialization")
                call("InitializeAudioDriver", limit=2000000)
            native("hn_input_initialize")(memory);call("InitializeKeys")
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

        for saved in (False, True):
            for opcode, group in ((0x43, 1), (0x43, 2), (0x43, 3), (0x43, 4), (0x60, 1)):
                prepare_core()
                if group == 4:
                    both(0x03002BB0, struct.pack("<I", 0x02010000))
                if saved:
                    equal(native("hn_save_write_groups", result=C.c_bool)(memory), True, "native saved map fixture")
                    call("WriteSaveDataGroups1To3")
                both(0x02020000, bytes([group]))
                equal(command(memory, opcode, 0x02020000), 1, f"native Main load {opcode:02X} {group} {saved}")
                call("MainScriptLoadSaveDataGroup" if opcode==0x43 else "MainScriptLoadSaveDataGroups",
                     0x02020000, limit=6000000)
                compare(f"Main load {opcode:02X} {group} {saved}", save_ranges + ((0x05000000, 0x400),))

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

        graphics_command = bind("hn_probe_graphics_command", C.c_int, C.c_void_p, C.c_uint32,
                                C.c_uint32, C.c_uint32, C.POINTER(C.c_uint32))
        graphics = bind("hn_probe_graphics", C.c_int, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        graphics_handlers = (
            "Return", "Wait", "Jump", "WriteBytes", "PaletteAll", "PaletteStep", "PaletteFrom",
            "CopyHalfwords", "DecompressWram", "PaletteRange", "BeginRepeat", "EndRepeat",
            "WriteHalfwords", "WriteMirrorRows", "SetBlendParameters", "BeginBlendFade",
            "BeginBlendFade", "BlendFadeStep", "BranchByte", "BranchByte", "BranchByte",
            "DisableBlend", "BeginBlendAlpha", "BlendAlphaStep", "ResetMosaic", "SetMosaicRegisters",
            "SetMosaicSizes", "EnableMosaic", "SetMosaicFlags", "EnableObjectMosaicRange",
            "BeginMosaicTransition", "MosaicStep", "MosaicStep", None, None, None, None, None, None,
            None, None, None, "SetWindowsEnabled", "SetWindowMasks", "SetWindowBounds", None, None, "Call")
        def prepare_graphics(case):
            prepare_background(case % 6)
            both(0x03002F20, struct.pack("<I", 8))
            both(0x02000394, struct.pack("<I", 0x05000000))
            both(0x0201C720, rng.randbytes(512))
            both(0x0201CC40, rng.randbytes(4096))
            both(0x02003920, rng.randbytes(3072))
            both(0x05000000, rng.randbytes(1024))
            both(0x03003070, struct.pack("<I", state))
            both(0x03002E10, b"\x02")
            both(state, rng.randbytes(44))
            both(state, bytes([0x9F if case & 1 else 0xA0, 4, 2]))
            both(state + 0x1E, struct.pack("<HH", 4, 8))
            counts = ((0, 0, 0, 0), (3, 0, 0, 0), (2, 1, 0, 0), (2, 2, 0, 0), (1, 1, 1, 1), (1, 3, 1, 0))
            both(state + 3, bytes(counts[case % 6]))
            for i in range(4):
                both(state + 8 + i * 4, struct.pack("<I", 0x02021800 + i * 16))
            both(state + 0x18, struct.pack("<I", 0x02021000))
            both(0x03002D98, struct.pack("<I", 0x02017000))
            both(0x03002DC0, b"\x08")
            both(0x02017000, rng.randbytes(8 * 0x68))
        for case in range(6):
            for opcode, suffix in enumerate(graphics_handlers, 0x80):
                if suffix is None:
                    continue
                prepare_graphics(case)
                payload = bytes([2, 1, 0, 4, 8, 16, 0x80, 0xFF])
                if opcode in (0x82, 0xAF):
                    payload = struct.pack("<I", 0x02021800)
                elif opcode in (0x83, 0x8C):
                    payload = struct.pack("<BBI", 2, 7, 0x02025000) + rng.randbytes(7)
                elif opcode in (0x84, 0x86, 0x89):
                    payload = struct.pack("<BBH", 3, 2, 4)
                    if opcode == 0x86:
                        payload = bytes([1]) + payload
                    elif opcode == 0x89:
                        payload = bytes([8, 1]) + payload
                elif opcode == 0x87:
                    payload = struct.pack("<BBII", 2, 7, 0x06004000, 0x02022000)
                elif opcode == 0x88:
                    payload = struct.pack("<BII", 2, 0x0201C720, 0x02024000)
                elif opcode == 0x8D:
                    payload = struct.pack("<BBBI", 2, 5, 3, 0x02025000) + rng.randbytes(5)
                elif opcode == 0x8E:
                    payload = bytes([2, 255 if case & 1 else 1, 255, 15, 255, 8, 16])
                elif opcode in (0x8F, 0x90):
                    payload = bytes([2, 15, 3 | ((case & 1) << 4), 1, case * 2])
                elif opcode in (0x92, 0x93, 0x94):
                    both(0x02025000, bytes([127 + case % 3]))
                    payload = struct.pack("<IBI", 0x02025000, 128, 0x02021800)
                elif opcode == 0x96:
                    payload = bytes([2, 15, 31, case, 16, 0, 0, 16])
                elif opcode in (0x99, 0x9A):
                    payload = bytes([case % 3, 255 - case, case * 13])
                elif opcode == 0x9B:
                    payload = bytes([1 << case])
                elif opcode == 0x9D:
                    payload = bytes([3, 255 if case & 1 else 7])
                elif opcode == 0x9E:
                    payload = bytes([0x9F if case & 1 else 0xA0, 2, case, case % 3, 12, 15, 0, 1])
                elif opcode == 0xAA:
                    payload = bytes([case, case + 1])
                elif opcode == 0xAC:
                    payload = struct.pack("<BHH", case & 1, 0x1830, 0x2040)
                both(0x02021000, bytes([opcode]) + payload)
                equal(graphics_command(memory, state, opcode, 0x02021001, report4), 1,
                      f"native graphics command {opcode:02X} {case}: {list(report4)}")
                equal(report4[3], call("Graphics" + suffix, state, 0x02021001, limit=1000000),
                      f"graphics command continuation {opcode:02X} {case}")
                compare(f"graphics command {opcode:02X} {case}", background_ranges)
        for case in range(60):
            prepare_graphics(case)
            index = case & 1
            equal(native("hn_graphics_configure", 2, C.c_bool)(memory, index, 0x02021000), True,
                  f"native graphics initialization {case}")
            call("InitializeGraphicsScript", index, 0x02021000)
            compare(f"graphics initialization {case}", background_ranges[:2])
            first, count = case * 7 % 480, 16
            amount = case % 8
            equal(native("hn_palette_prepare", 3, C.c_bool)(memory, amount, first, count), True,
                  f"native prepare palette {case}")
            call("PreparePaletteGroup", amount, first, count, limit=1000000)
            compare(f"prepare palette {case}", background_ranges[:3])
            for step in range(1 << amount):
                equal(native("hn_palette_update", 2, C.c_bool)(memory, first, count), True,
                      f"native update palette {case} {step}")
                call("UpdatePaletteGroup", first, count)
                compare(f"update palette {case} {step}", background_ranges[:3])

        prepare_core(True)
        memory_copy = bind("hn_memory_copy", C.c_bool, C.c_void_p, C.c_uint32, C.c_uint32, C.c_size_t)
        mix_frame = bind("hn_probe_audio_mix", C.c_int, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        actor_frames = bind("hn_probe_actor_frames", C.c_int, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        task_frames = bind("hn_probe_tasks", C.c_int, C.c_void_p, C.c_uint32, C.POINTER(C.c_uint32))
        video_render = bind("hn_video_render", None, C.c_void_p, C.c_void_p)
        if args.video_oracle:
            from compare_video import VideoOracle
            oracle_context = VideoOracle(args.video_oracle, args.oracle_runtime_dir)
        else:
            oracle_context = nullcontext(None)
        video_frames, nonuniform, active_graphics_iterations, hblank_calls = 0, 0, 0, 0
        input_iterations=9000
        movement_positions=set()
        with oracle_context as video_oracle:
            for index in range(180+input_iterations):
                if index == 180:
                    print(f"180 idle script/render iterations matched; graphics scripts active in {active_graphics_iterations}.")
                    prepare_core(True)
                    active_graphics_iterations = 0
                # Original input state/repeat adapter, still a component schedule
                # without live motion, CPU queue flushing or real IRQ cadence.
                relative=index-180
                keys=0
                if index>=180:
                    if relative in (1460,2000):keys=0x10
                    elif relative in (1120,1360,1800):keys=8
                    elif (relative in (1140,1380,1600,1820,2100) or relative<1000 and relative%120==100
                          or relative>2200 and relative%90==0):keys=1
                    if relative>=7310:
                        phase=(relative-7310)%1200
                        keys=16 if phase<160 else 128 if phase<320 else 32 if phase<480 else 64 if phase<640 else 1 if relative%30==0 else 0
                both(0x04000130, struct.pack("<H", (~keys)&1023))
                equal(native("hn_input_update", 1, C.c_bool)(memory, keys), True, f"coupled input {index}")
                call("ReadKeys")
                native("hn_audio_dma_vblank")(memory)
                call("SyncAudioDmaAtVblankAsm")
                equal(frame(memory, 4096, report), 1, f"coupled Main {index}")
                call("RunMainScriptFrame")
                equal(native("hn_motion_update_following",result=C.c_bool)(memory),True,f"coupled gated following {index}")
                call("UpdateRenderXZMotion")
                equal(actor_frames(memory,4096,report3),1,f"coupled native actor scripts {index}: {list(report3)}")
                call("RunActorScriptFrames",limit=2000000)
                equal(task_frames(memory,4096,report3),1,f"coupled native text tasks {index}: {list(report3)}")
                call("UpdateTaskContexts",limit=4000000)
                equal(background(memory, 4096, report3), 1, f"coupled native background {index}: {list(report3)}")
                call("UpdateBackgroundStates", limit=1000000)
                equal(render(memory, 4096, 1, report3), 1, f"coupled native render {index}")
                call("UpdateRenderSlots", limit=1000000)
                graphics_base = struct.unpack("<I", native_bytes(0x03003070, 4))[0]
                graphics_count = native_bytes(0x03002E10, 1)[0]
                active_graphics_iterations += any(native_bytes(graphics_base + slot * 44, 1)[0] not in (0, 255)
                                                 for slot in range(graphics_count))
                equal(graphics(memory, 4096, report3), 1, f"coupled native graphics {index}: {list(report3)}")
                call("UpdateGraphicsScripts", limit=1000000)
                native("hn_mosaic_update")(memory)
                call("UpdateMosaic")
                both(0x04000006, b"\xA0\0")
                equal(memory_copy(memory, 0x07000000, 0x03003080, 1024), True, "OAM shadow transfer")
                cpu.mem_write(0x07000000, bytes(cpu.mem_read(0x03003080, 1024)))
                native("hn_video_commit")(memory)
                call("CommitVideoRegisters")
                native("hn_transfer_flush")(memory)
                call("FlushDmaTransfers")
                # Selected IRQ callbacks over explicit scanline samples. Deferred
                # DMA0/PPU timing and actual CPU/IRQ interleaving remain unmodeled.
                for line in range(228):
                    if native_bytes(0x03001F6C,1)==b"\0":break
                    both(0x04000006,struct.pack("<H",line))
                    equal(native("hn_raster_hblank",result=C.c_bool)(memory),True,"coupled HBlank callback")
                    call("HandleHblankIrq");hblank_calls+=1
                both(0x04000006,b"\xA0\0")
                equal(mix_frame(memory, 4096, report3), 1, f"coupled audio mix {index}: {list(report3)}")
                call("RunAudioMixerAsm", limit=2000000)
                # DMA3 can retain the reference's transient stack-local source
                # addresses; compare transferred data and sound DMA1/2 instead.
                compare(f"coupled Main/background/render/graphics {index}", save_ranges + ((0x05000000, 0x400),
                    (0x06000000, 0x18000), (0x07000000, 0x400), (0x04000000, 0x58),
                    (0x03007FF0, 4), (0x04000060, 0x74), (0x04000100, 4)))
                if relative>=7310:
                    base=struct.unpack("<I",native_bytes(0x03002D98,4))[0]+native_bytes(0x03002C0C,1)[0]*104
                    movement_positions.add((native_bytes(base+0x18,4),native_bytes(base+0x20,4)))
                if index>=180 and relative in (1240,1900,2020):
                    expected_mode,expected_choice={1240:(1,0),1900:(0,1),2020:(0,2)}[relative]
                    equal(native_bytes(0x020046C2,1)[0],expected_mode,f"name scenario mode {relative}")
                    equal(native_bytes(0x020046C1,1)[0],expected_choice,f"name scenario selection {relative}")
                if video_oracle and index in (0, 24, 32, 64, 87, 120, 179, 580, 780, 980, 1420, 1880, 2200, 2600, 3000, 3166, 3500, 4300, 5096, 5800, 6180, 6780, 7475, 7490, 7520, 7650, 7810, 7970, 8180, 9179):
                    pixels = (C.c_uint32 * (240 * 160))()
                    video_render(memory, pixels)
                    parts = [native_bytes(address, size) for address, size in
                        ((0x04000000, 1024), (0x05000000, 1024), (0x06000000, 98304), (0x07000000, 1024))]
                    equal(bytes(pixels), video_oracle.render(parts), f"private ROM PPU snapshot {index}")
                    video_frames += 1
                    nonuniform += len(set(pixels)) > 1
        if args.video_oracle:
            print(f"{video_frames} private ROM PPU snapshots matched; {nonuniform} contained multiple colors.")
        print(f"{input_iterations} input-driven script/render/audio iterations matched; graphics scripts active in {active_graphics_iterations}.")
        equal(len(movement_positions)>=32,True,"coupled leader visibly changes world position")
        print(f"{len(movement_positions)} distinct leader positions matched during directional input.")
        print(f"{hblank_calls} sampled HBlank callbacks matched; scanline DMA/PPU timing remains unverified.")
        audio_checks();region_audio_checks()
        scene_checks()
        text_checks()
        counter_checks()
        actor_checks()
        motion_checks()
        motion_live_checks()
        collision_checks()
        task_checks()
        choice_checks()
        raster_checks()
    finally:
        status="failed" if sys.exc_info()[0] else "passed"
        try:
            destroy(memory)
            if bios_oracle:bios_oracle.close()
        except BaseException:
            status="failed";raise
        finally:
            elapsed=round(time.perf_counter()-started,3)
            print(f"Timing total: {elapsed:.3f}s, {checks} checks, {status}.",flush=True)
            if args.timings_json:
                args.timings_json.write_text(json.dumps(dict(seconds=elapsed,checks=checks,status=status,phases=phases),indent=2)+"\n",encoding="utf-8")
    print(f"{checks} private native/reference checks passed (heap, input, scripts, expressions, rendering, maps, saves, audio).")
    print("Function-level evidence only; boot and gameplay remain unverified.")

if __name__ == "__main__":
    main()
