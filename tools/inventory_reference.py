"""Read-only planning inventory; dispatch presence is not verified game coverage."""
from __future__ import annotations
import argparse
from collections import defaultdict
import fnmatch
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--source", default="*", help="Reference-relative source glob")
    parser.add_argument("--area-bank", type=int, choices=range(11), help="Inspect declared dependencies of a recovered map-bank script family")
    parser.add_argument("--script-source", help="Inspect declarations in a source glob under src/data/graphics, including shared scripts")
    parser.add_argument("--json", action="store_true", help="Include function metadata on stdout")
    args = parser.parse_args()
    if ((args.area_bank is not None or args.script_source) and args.source != "*") or (args.area_bank is not None and args.script_source):
        parser.error("Choose one of --source, --area-bank or --script-source.")
    reference = args.reference.resolve(strict=True)
    report = (area_inventory(reference, args.area_bank) if args.area_bank is not None else
              script_inventory(reference, args.script_source) if args.script_source else inventory(reference, args.source))
    if args.json:
        print(json.dumps(report, indent=2))
        return 0
    print(f"Reference: {report['reference_revision']} (dirty={report['reference_dirty']})")
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
