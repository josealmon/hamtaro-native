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
    match = re.search(r"\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", text)
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
        ("main", "text.c", "hn_scene_text_callback"),
        ("main", "book.c", "hn_book_callback"),
        ("animation", "scene.c", "hn_scene_animation_callback"),
        ("text-task", "task_text.c", "task_callback"),
        ("hblank", "raster.c", "hn_raster_hblank"),
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path, required=True)
    parser.add_argument("--source", default="*", help="Reference-relative source glob")
    parser.add_argument("--json", action="store_true", help="Include function metadata on stdout")
    args = parser.parse_args()
    report = inventory(args.reference.resolve(strict=True), args.source)
    if args.json:
        print(json.dumps(report, indent=2))
        return 0
    print(f"Reference: {report['reference_revision']} (dirty={report['reference_dirty']})")
    print(" C    ASM   Named   Dispatch  Reference source")
    for row in report["sources"]:
        print(f"{row['c_functions']:4} {row['asm_functions']:4} {row['callback_named_candidates']:7} {row['candidates_with_dispatch']:9}  {row['source']}")
    print(report["note"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
