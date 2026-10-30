"""Read-only planning inventory; dispatch presence is not verified game coverage."""
from __future__ import annotations
import argparse
from collections import defaultdict
import fnmatch
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[1]
CALLBACK_PREFIXES = ("ScriptCallback", "AnimationCallback", "SceneRenderCallback")


def body(text: str, name: str) -> str:
    # Strip comments/strings before matching braces; this is a bounded source
    # scanner for our explicit dispatchers, not a C parser or reachability proof.
    text = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"', "", text, flags=re.S)
    # A call nested in an if-condition may also precede a brace. Require a
    # declaration at the start of a line, including its return type/qualifiers.
    match = re.search(r"^[ \t]*(?:[A-Za-z_]\w*[ \t*]+)+" + re.escape(name) +
                      r"\([^;{}]*\)\s*\{", text, re.M)
    if not match:
        raise ValueError(f"Dispatch function not found: {name}")
    start = match.end()
    depth = 1
    for end in range(start, len(text)):
        depth += (text[end] == "{") - (text[end] == "}")
        if depth == 0:
            return text[start:end]
    raise ValueError(f"Unbalanced dispatch function: {name}")


def dispatches() -> dict[int, list[str]]:
    result = defaultdict(list)
    definitions = (
        ("main", "scene.c", "hn_scene_callback"),
        ("main", "save_group4.c", "hn_group4_callback"),
        ("main", "link.c", "hn_link_callback"),
        ("main", "save_group4_ui.c", "hn_group4_ui_callback"),
        ("text-task", "save_group4.c", "hn_group4_task"),
        ("main", "text.c", "hn_scene_text_callback"),
        ("main", "book.c", "hn_book_callback"),
        ("animation", "scene.c", "hn_scene_animation_callback"),
        ("text-task", "task_text.c", "task_callback"),
        ("hblank", "raster.c", "hn_raster_hblank"),
        ("vcount", "raster.c", "hn_raster_vcount"),
        ("main", "scene_motion.c", "hn_scene_motion_callback"),
        ("animation", "scene_motion.c", "hn_scene_motion_animation"),
        ("text-task", "presets.c", "hn_preset_apply"),
        ("main", "presets.c", "hn_preset_callback"),
        ("main", "map_choice.c", "hn_map_choice_callback"),
        ("main", "map_choice.c", "simple_main"),
        ("animation", "map_choice.c", "hn_map_choice_animation"),
        ("text-task", "map_choice.c", "hn_map_choice_apply"),
        ("text-task", "map_choice.c", "simple_apply"),
        ("render-slot", "map_choice.c", "hn_map_choice_slot"),
        ("main", "particles.c", "hn_particles_callback"),
        ("main", "bank2.c", "main_contract"),
        ("text-task", "bank2.c", "simple_apply"),
        ("text-task", "bank2.c", "hn_bank2_apply"),
        ("animation", "bank2.c", "hn_bank2_animation"),
        ("argument-word", "script.c", "hn_main_command"),
        ("text-task", "records.c", "hn_records_task"),
        ("animation", "wander.c", "hn_wander_animation"),
        ("main", "wander.c", "hn_wander_callback"),
        ("main", "scene_actor.c", "simple_presets"),
        ("main", "scene_actor.c", "hn_scene_actor_callback"),
        ("animation", "scene_actor.c", "hn_scene_actor_animation"),
        ("main", "bank3.c", "hn_bank3_callback"),
        ("text-task", "bank3.c", "hn_bank3_callback"),
        ("text-task", "bank3.c", "late_presets"),
        ("animation", "bank3.c", "hn_bank3_animation"),
        ("text-task", "scene_actor.c", "hn_scene_actor_task"),
        ("text-task", "actor_presets.c", "hn_actor_presets_apply"),
        ("text-task", "actor_presets.c", "base_actor_presets"),
        ("text-task", "actor_presets.c", "sequence_actor_presets"),
        ("text-task", "bank3.c", "main_presets"),
        ("main", "bank3_games.c", "hn_bank3_games_callback"),
        ("text-task", "bank3_games.c", "hn_bank3_games_task"),
        ("animation", "bank3_games.c", "hn_bank3_games_animation"),
        ("main", "bank3_chain.c", "hn_bank3_chain_callback"),
        ("text-task", "bank3_chain.c", "hn_bank3_chain_task"),
        ("animation", "bank3_chain.c", "hn_bank3_chain_animation"),
        ("main", "bank4_presets.c", "hn_bank4_preset_callback"),
        ("text-task", "bank4_presets.c", "hn_bank4_presets"),
        ("main", "bank4.c", "hn_bank4_callback"),
        ("text-task", "bank4.c", "hn_bank4_task"),
        ("animation", "bank4.c", "hn_bank4_animation"),
        ("animation", "bank4_path.c", "hn_bank4_path_animation"),
        ("main", "bank5_presets.c", "hn_bank5_preset_callback"),
        ("text-task", "bank5_presets.c", "hn_bank5_presets"),
        ("main", "bank5.c", "hn_bank5_callback"),
        ("text-task", "bank5.c", "hn_bank5_task"),
        ("animation", "bank5.c", "hn_bank5_animation"),
        ("vblank", "bank5.c", "hn_bank5_irq"),
        ("main", "bank6_presets.c", "hn_bank6_preset_callback"),
        ("text-task", "bank6_presets.c", "hn_bank6_presets"),
        ("main", "bank6.c", "hn_bank6_callback"),
        ("text-task", "bank6.c", "hn_bank6_task"),
        ("animation", "bank6.c", "hn_bank6_animation"),
        ("main", "bank6_targets.c", "hn_bank6_targets_callback"),
        ("animation", "bank6_targets.c", "hn_bank6_targets_animation"),
        ("main", "bank6_chain.c", "hn_bank6_chain_callback"),
        ("text-task", "bank6_chain.c", "hn_bank6_chain_task"),
        ("animation", "bank6_chain.c", "hn_bank6_chain_animation"),
        ("main", "bank7.c", "hn_bank7_callback"),
        ("text-task", "bank7.c", "hn_bank7_task"),
        ("main", "record_selection.c", "hn_record_selection_callback"),
        ("text-task", "record_selection.c", "hn_record_selection_task"),
        ("main", "scene_selection.c", "hn_scene_selection_callback"),
        ("text-task", "scene_selection.c", "hn_scene_selection_task"),
        ("main", "presentation.c", "hn_presentation_callback"),
        ("text-task", "presentation.c", "hn_presentation_task"),
        ("animation", "presentation.c", "hn_presentation_animation"),
        ("main", "presentation_background.c", "hn_presentation_background_callback"),
        ("text-task", "presentation_background.c", "hn_presentation_background_task"),
        ("animation", "presentation_background.c", "hn_presentation_background_animation"),
        ("main", "presentation_window.c", "hn_presentation_window_callback"),
        ("text-task", "presentation_window.c", "hn_presentation_window_task"),
        ("main", "records.c", "hn_records_callback"),
        ("main", "save_group3.c", "hn_group3_callback"),
        ("text-task", "save_group3.c", "hn_group3_task"),
        ("main", "pair_list.c", "hn_pair_list_callback"),
        ("text-task", "pair_list.c", "hn_pair_list_task"),
        ("main", "record_menu.c", "hn_record_menu_callback"),
        ("text-task", "record_menu.c", "hn_record_menu_task"),
        ("main", "raster.c", "hn_raster_callback"),
        ("main", "counter_scene.c", "hn_counter_scene_callback"),
        ("text-task", "counter_scene.c", "hn_counter_scene_task"),
    )
    for abi, filename, function in definitions:
        text = body((ROOT / "src/game" / filename).read_text(encoding="utf-8"), function)
        addresses = set(re.findall(r"case\s+(0x[0-9A-Fa-f]{8})\s*:", text))
        # Includes the final != guard whose fallthrough implements a callback.
        addresses.update(re.findall(r"(?:callback\s*&\s*~1u\s*\)|callback)\s*[!=]=\s*(0x[0-9A-Fa-f]{8})", text))
        for address in addresses:
            result[int(address, 16) & ~1].append(abi)
    script = (ROOT / "src/game/script.c").read_text(encoding="utf-8")
    builtins = re.search(r"HNMainCallback\s+builtins\[\]\s*=\s*\{(.*?)\n\};", script, re.S)
    if not builtins:
        raise ValueError("Main builtin dispatch table not found")
    for address in re.findall(r"\{\s*(0x[0-9A-Fa-f]{8})\s*,", builtins[1]):
        result[int(address, 16) & ~1].append("main")
    return {address: sorted(set(abis)) for address, abis in result.items()}


def inventory(reference: Path, pattern: str) -> dict:
    symbols = json.loads((reference / "build/function-symbols.json").read_text(encoding="utf-8"))
    bindings = dispatches()
    groups = defaultdict(list)
    for symbol in symbols:
        if not fnmatch.fnmatchcase(symbol["source"], pattern):
            continue
        address = symbol["address"] & ~1
        groups[symbol["source"]].append({
            "name": symbol["name"], "address": f"0x{address:08X}",
            "bytes": symbol["end"] - symbol["address"], "kind": symbol["kind"],
            "callback_named": symbol["name"].startswith(CALLBACK_PREFIXES),
            "native_dispatch_abis": bindings.get(address, []),
        })
    sources = []
    for source, functions in sorted(groups.items()):
        candidates = [f for f in functions if f["callback_named"]]
        sources.append({
            "source": source, "functions": sorted(functions, key=lambda f: f["address"]),
            "c_functions": sum(f["kind"] == "c" for f in functions),
            "asm_functions": sum(f["kind"] == "asm" for f in functions),
            "callback_named_candidates": len(candidates),
            "candidates_with_dispatch": sum(bool(f["native_dispatch_abis"]) for f in candidates),
        })
    revision = subprocess.check_output(["git", "-C", str(reference), "rev-parse", "HEAD"], text=True).strip()
    dirty = bool(subprocess.check_output(["git", "-C", str(reference), "status", "--porcelain"]))
    return {"reference_revision": revision, "reference_dirty": dirty, "sources": sources,
            "note": "Callback names are planning candidates. Dispatch presence does not prove semantics, ABI compatibility, reachability or playthrough coverage. Helpers and unnamed indirect targets require review; this is not a port completion percentage."}


def script_inventory(reference: Path, pattern: str) -> dict:
    report = inventory(reference, "*")
    symbols = {row["name"]: {**row, "source": group["source"]}
               for group in report["sources"] for row in group["functions"]}
    declarations = defaultdict(list)
    files = sorted((reference / "src/data/graphics").glob(pattern))
    if not files:
        raise ValueError(f"No recovered script source matches {pattern}")
    for path in files:
        for name in re.findall(r"^extern[^;\n]*?\b(\w+)\s*\(", path.read_text(encoding="utf-8"), re.M):
            declarations[name].append(path.relative_to(reference).as_posix())
    rows = []
    for name, sources in sorted(declarations.items()):
        row = dict(symbols[name]) if name in symbols else {"name": name, "native_dispatch_abis": []}
        row["declared_in"] = sorted(set(sources))
        rows.append(row)
    return {"reference_revision": report["reference_revision"], "reference_dirty": report["reference_dirty"],
            "script_pattern": pattern, "script_sources": [p.relative_to(reference).as_posix() for p in files],
            "functions": rows, "note": "Declared externs are dependency-planning candidates, not a complete call graph. Literal script targets, animation resources and unused declarations need separate review. Dispatch presence does not prove ABI correctness, execution coverage or area completion."}


def area_inventory(reference: Path, bank: int) -> dict:
    return {**script_inventory(reference, f"map_bank{bank}_*.c"), "bank": bank}


BACKGROUND_BYTES = {
    0x80: 1, 0x81: 3, 0x82: 5, 0x83: 2,
    **dict.fromkeys(range(0x84, 0x88), 5), **dict.fromkeys(range(0x88, 0x8C), 3),
    **dict.fromkeys(range(0x8C, 0x90), 1), 0x90: 2, 0x91: 1, 0x92: 3,
    0x93: 6, 0x94: 7, 0x95: 13, 0x96: 1, 0x97: 11, 0x98: 1,
    0x99: 13, 0x9A: 17, 0x9B: 1, 0x9C: 6, 0x9D: 7, 0x9E: 13, 0x9F: 17,
    0xA0: 5, 0xA1: 9, **dict.fromkeys(range(0xA2, 0xA6), 2),
    **dict.fromkeys(range(0xA6, 0xB0), 3), **dict.fromkeys(range(0xB0, 0xB3), 10),
    0xB3: 5, 0xB4: 6, 0xB5: 2, 0xB6: 2, 0xB7: 2, 0xB8: 3,
    **dict.fromkeys(range(0xB9, 0xBD), 3),
}


def background_fields(text: str) -> list[dict]:
    """Read fixed-address B-macro fields, rejecting unclassified macro uses."""
    text = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"', "", text, flags=re.S)
    joined = text.replace("\\\n", " ")
    definitions = {name: int(value, 0) for name, value in re.findall(
        r"^\s*#define\s+(B_\w+|END)(?:\([^\n]*?\))?\s+(0x[\da-fA-F]+|\d+)\b", joined, re.M)}
    # Older builtins use positional u8-array structs. Compute only this bounded
    # layout; pointer/aligned/union layouts must not inherit host sizeof rules.
    layouts = {}
    for match in re.finditer(r"typedef\s+struct\s*\{([^{}]*)\}\s*(\w+)\s*;", text, re.S):
        members = []
        offset = 0
        valid = True
        for declaration in filter(str.strip, match[1].split(";")):
            declaration = declaration.strip()
            if not declaration.startswith("u8 "):
                valid = False
                break
            for member in declaration[3:].split(","):
                field = re.fullmatch(r"\s*(\w+)\[((?:0x[\da-fA-F]+)|\d+)\]\s*", member)
                if not field:
                    valid = False
                    break
                members.append((field[1], offset))
                offset += int(field[2], 0)
        if valid and members:
            layouts[match[2]] = members
    positional = []
    for type_name, members in layouts.items():
        base = re.search(r"^\s*#define\s+\w+_ADDRESS\(member\)\s+\((0x[\da-fA-F]+)[uUlL]*[^\n]*\(\(" +
                         re.escape(type_name) + r"\s*\*\)", joined, re.M)
        if not base:
            continue
        pattern = r"\bconst\s+" + re.escape(type_name) + r"\s+\w+\s*=\s*\{(.*?)\}\s*;"
        for match in re.finditer(pattern, text, re.S):
            if re.search(r"\.\w+\s*=", match[1]):
                continue
            values = re.findall(r"\{([^{}]*)\}", match[1])
            if len(values) != len(members):
                raise ValueError(f"Unclassified positional script layout: {type_name}")
            positional.extend((name, int(base[1], 16) + offset, value)
                              for (name, offset), value in zip(members, values))
            text = text.replace(match[0], "")
    lines = []
    continued = False
    for line in text.splitlines(keepends=True):
        if continued or line.lstrip().startswith("#"):
            continued = line.rstrip().endswith("\\")
        else:
            lines.append(line)
    text = "".join(lines)
    rows = []
    fields = [(match[1], None, match[2]) for match in re.finditer(r"\.(\w+)\s*=\s*\{([^{}]*)\}", text, re.S)]
    for name, address, value in [*fields, *positional]:
        names = re.findall(r"\b(?:B_\w+|END)\b", value)
        if not any(token.startswith("B_") for token in names) and not (value.strip() == "END" and address is not None):
            continue
        if address is None:
            suffix = re.search(r"_([\da-fA-F]{6,8})$", name)
            if not suffix:
                raise ValueError(f"Unclassified background field: {name}")
            address = int(suffix[1], 16)
            if address < 0x01000000:
                address += 0x08000000
        if any(token not in definitions for token in names):
            raise ValueError(f"Unresolved background macro in field: {name}")
        opcodes = [definitions[token] for token in names]
        if any(opcode >= 0x80 and opcode not in BACKGROUND_BYTES for opcode in opcodes):
            raise ValueError(f"Unknown background macro opcode: {name}")
        rows.append(dict(field=name, address=address, opcodes=opcodes))
    remaining = re.sub(r"\.(\w+)\s*=\s*\{([^{}]*)\}", "", text, flags=re.S)
    if re.search(r"\bB_\w+", remaining):
        raise ValueError("Background macros outside fixed-address fields need review.")
    return rows


def background_commands(rom: bytes, roots: list[int], base: int = 0x08000000) -> dict[int, int]:
    """Conservative static decoding, not background execution or timing emulation."""
    pending = list(roots)
    commands = {}
    while pending:
        pc = pending.pop()
        if pc in commands:
            continue
        offset = pc - base
        if not 0 <= offset < len(rom):
            raise ValueError(f"Background target outside ROM: 0x{pc:08X}")
        opcode = rom[offset]
        commands[pc] = opcode
        if len(commands) > 1_000_000:
            raise ValueError("Background command inventory exceeded its bound.")
        if opcode < 0x80:
            continue  # Original interpreter yields without consuming these bytes.
        size = BACKGROUND_BYTES.get(opcode)
        if size is None or offset + size > len(rom):
            raise ValueError(f"Unknown or truncated background command at 0x{pc:08X}")
        if opcode in (0x80, 0x8C, 0x8D, 0x8E, 0x8F, 0x96, 0x98, 0x9B):
            continue  # Return or an original stop without cursor advancement.
        if opcode in (0x82, 0xA0):
            pending.append(int.from_bytes(rom[offset + 1:offset + 5], "little"))
            if opcode == 0x82:
                continue
        if opcode in (0xB0, 0xB1, 0xB2):
            pending.append(int.from_bytes(rom[offset + 6:offset + 10], "little"))
        # A call also includes its continuation. Repeats include both body and
        # exit; all loop-body commands are visited without simulating counters.
        pending.append(pc + size)
    return commands


def private_rom(rom_path: Path) -> tuple[bytes, str]:
    rom = rom_path.read_bytes()
    digest = hashlib.sha256(rom).hexdigest()
    expected = json.loads((ROOT / "config/rom.json").read_text(encoding="utf-8"))["sha256"]
    if digest != expected:
        raise ValueError("The script inventory requires the supported AH3P ROM.")
    return rom, digest


def task_callback_stores(text: str, rom: bytes, symbols: dict[str, int]) -> list[dict]:
    """Typed opcode-25 initializers, not F4 followed by a guessed pointer."""
    text = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"', "", text, flags=re.S)
    rows = []
    for field, value in re.findall(r"\.(\w+)\s*=\s*\{([^{}]*)\}", text, re.S):
        match = re.match(r"\s*0x25\s*,\s*(0x[\da-fA-F]+|\d+)\s*,(.*)", value, re.S)
        if not match:
            continue
        targets = [name for name in re.findall(r"\b[A-Za-z_]\w*", match[2]) if name in symbols]
        suffix = re.search(r"_([\da-fA-F]{5,8})$", field)
        if len(targets) != 1 or not suffix:
            raise ValueError(f"Unclassified task-callback store: {field}")
        address = int(suffix[1], 16)
        if address < 0x01000000:
            address += 0x08000000
        offset = address - 0x08000000
        target = symbols[targets[0]] & ~1
        slot = int(match[1], 0)
        actual = int.from_bytes(rom[offset + 2:offset + 6], "little") & ~1
        if (not 0 <= offset <= len(rom) - 6 or not 0 <= slot <= 255 or
                rom[offset:offset + 2] != bytes((0x25, slot)) or actual != target):
            raise ValueError(f"Source/ROM task-callback mismatch at 0x{address:08X}")
        rows.append(dict(field=field, site=f"0x{address:08X}", slot=slot,
                         name=targets[0], address=f"0x{target:08X}"))
    return rows


def task_callback_inventory(reference: Path, rom_path: Path) -> dict:
    rom, digest = private_rom(rom_path)
    report = inventory(reference, "*")
    functions = [row for group in report["sources"] for row in group["functions"]]
    symbols = {row["name"]: int(row["address"], 16) for row in functions}
    bindings = {int(row["address"], 16): row["native_dispatch_abis"] for row in functions}
    rows = []
    for path in sorted((reference / "src/data/graphics").rglob("*.c")):
        for row in task_callback_stores(path.read_text(encoding="utf-8"), rom, symbols):
            row["source"] = path.relative_to(reference).as_posix()
            row["native_dispatch_abis"] = bindings.get(int(row["address"], 16), [])
            rows.append(row)
    if not rows:
        raise ValueError("No typed task-callback stores found.")
    return dict(reference_revision=report["reference_revision"], reference_dirty=report["reference_dirty"],
                rom_sha256=digest, task_callback_stores=rows,
                unique_targets=len({row["address"] for row in rows}),
                missing_task_bindings=[row for row in rows if "text-task" not in row["native_dispatch_abis"]],
                note="Exact typed Main opcode25 stores and their text-task ABI membership. F4 consumes a byte index "
                     "into these slots, not a function pointer. Does not prove store-before-use, all indirect writes, "
                     "callback semantics or complete gameplay.")


def background_inventory(reference: Path, rom_path: Path) -> dict:
    rom, digest = private_rom(rom_path)
    rows = []
    declared = {}
    for path in sorted((reference / "src/data/graphics").rglob("*.c")):
        try:
            fields = background_fields(path.read_text(encoding="utf-8"))
        except ValueError as error:
            raise ValueError(f"{path.relative_to(reference)}: {error}") from error
        for row in fields:
            pc = row["address"]
            for opcode in row["opcodes"]:
                offset = pc - 0x08000000
                if not 0 <= offset < len(rom) or rom[offset] != opcode:
                    raise ValueError(f"Source/ROM background command mismatch at 0x{pc:08X}")
                if pc in declared:
                    raise ValueError(f"Overlapping background declarations at 0x{pc:08X}")
                declared[pc] = opcode
                pc += 1 if opcode < 0x80 else BACKGROUND_BYTES[opcode]
            rows.append(dict(source=path.relative_to(reference).as_posix(),
                             field=row["field"], address=f"0x{row['address']:08X}",
                             commands=len(row["opcodes"])))
    if not rows:
        raise ValueError("No source background fields found.")
    decoded = background_commands(rom, [int(row["address"], 16) for row in rows])
    if decoded != declared:
        raise ValueError("Decoded background edges do not match the source command boundaries.")
    counts = defaultdict(int)
    for opcode in decoded.values():
        counts[f"0x{opcode:02X}"] += 1
    return dict(reference_revision=subprocess.check_output(
        ["git", "-C", str(reference), "rev-parse", "HEAD"], text=True).strip(),
        reference_dirty=bool(subprocess.check_output(["git", "-C", str(reference), "status", "--porcelain"])),
        rom_sha256=digest, background_fields=rows, command_count=len(decoded),
        opcode_counts=dict(sorted(counts.items())),
        cpu_queue_sites=[f"0x{pc:08X}" for pc, opcode in sorted(decoded.items()) if 0x9C <= opcode <= 0x9F],
        note="Exact B-macro/ROM command boundaries, static calls/jumps and both conditional successors. "
             "Includes all recovered B-macro fields, not every possible runtime root or script mutation. "
             "Does not execute transfers, emulate their timing or prove complete gameplay.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--source", default="*", help="Reference-relative source glob")
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument("--area-bank", type=int, choices=range(11), help="Inspect declared dependencies of a recovered map-bank script family")
    modes.add_argument("--script-source", help="Inspect declarations in a source glob under src/data/graphics, including shared scripts")
    modes.add_argument("--background-scripts", action="store_true", help="Compare B-macro command boundaries and static edges with the private ROM")
    modes.add_argument("--task-callbacks", action="store_true", help="Check typed Main callback stores against the text-task ABI")
    parser.add_argument("--rom", type=Path, help="Player-supplied ROM, required by --background-scripts or --task-callbacks")
    parser.add_argument("--json", action="store_true", help="Include inventory metadata on stdout")
    args = parser.parse_args()
    if (args.area_bank is not None or args.script_source or args.background_scripts or args.task_callbacks) and args.source != "*":
        parser.error("Choose one inventory mode.")
    if (args.background_scripts or args.task_callbacks) != bool(args.rom):
        parser.error("--rom is required only with --background-scripts or --task-callbacks.")
    reference = args.reference.resolve(strict=True)
    report = (background_inventory(reference, args.rom) if args.background_scripts else
              task_callback_inventory(reference, args.rom) if args.task_callbacks else
              area_inventory(reference, args.area_bank) if args.area_bank is not None else
              script_inventory(reference, args.script_source) if args.script_source else inventory(reference, args.source))
    if args.json:
        print(json.dumps(report, indent=2))
        return 0
    print(f"Reference: {report['reference_revision']} (dirty={report['reference_dirty']})")
    if "task_callback_stores" in report:
        print(f"Task callbacks: {len(report['task_callback_stores'])} typed stores, {report['unique_targets']} targets, "
              f"{len(report['missing_task_bindings'])} without task dispatch")
        for row in report["missing_task_bindings"]:
            print(f"{row['address']} {row['name']} ({row['source']}:{row['field']})")
        print(report["note"])
        return 0
    if "background_fields" in report:
        print(f"Backgrounds: {len(report['background_fields'])} source fields, {report['command_count']} command/stop positions, "
              f"{len(report['cpu_queue_sites'])} CPU-queue sites")
        print(report["note"])
        return 0
    if "script_sources" in report:
        missing = [row for row in report["functions"] if not row["native_dispatch_abis"]]
        label = f"Bank {args.area_bank}" if args.area_bank is not None else f"Scripts {args.script_source}"
        print(f"{label}: {len(report['script_sources'])} script sources, {len(report['functions'])} declared functions, {len(missing)} without explicit native dispatch")
        for row in sorted(missing, key=lambda r: (r.get("source", ""), r["name"])):
            print(f"{row.get('address', 'unresolved'):>10} {row['name']} ({row.get('source', 'no symbol metadata')})")
        print(report["note"])
        return 0
    print(" C    ASM   Named   Dispatch  Reference source")
    for row in report["sources"]:
        print(f"{row['c_functions']:4} {row['asm_functions']:4} {row['callback_named_candidates']:7} {row['candidates_with_dispatch']:9}  {row['source']}")
    print(report["note"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
