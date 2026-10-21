"""Check common private/generated content mistakes; not a copyright classifier."""
from __future__ import annotations
import argparse
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
FORBIDDEN = {".gba", ".sav", ".sram", ".state", ".rgba", ".pcm", ".wav", ".zip",
             ".7z", ".exe", ".dll", ".pdb", ".obj", ".o", ".elf", ".bin", ".log", ".env"}
PACKAGE_FILES = {"hamtaro-native.exe", "README.md", "NOTICE.md", "docs/architecture.md", "licenses/SDL.txt", "licenses/Dear-ImGui.txt"}

def git(*args: str) -> bytes:
    return subprocess.check_output(["git", "-C", str(ROOT), *args])

def check_name(name: str) -> list[str]:
    path = Path(name)
    failures = []
    if path.suffix.lower() in FORBIDDEN or path.name.lower() in {"agents.md", "settings.ini", "cmakeuserpresets.json"}:
        failures.append(f"Private/generated file: {name}")
    if name.startswith("src/data/") or any(p.lower() in {"build", "local", "evidence", "screenshots", "credentials"} for p in path.parts):
        failures.append(f"Private/generated directory: {name}")
    return failures

def check_source(name: str, data: bytes) -> list[str]:
    failures = check_name(name)
    if len(data) > 256 * 1024:
        failures.append(f"Unexpectedly large versioned source: {name}")
    if b"\x00" in data or (len(data) >= 0xC0 and data[0xAC:0xB0] == b"AH3P"):
        failures.append(f"Unexpected binary/game content: {name}")
    try:
        text = data.decode("utf-8")
        if "\r" in text:
            failures.append(f"Source must use LF: {name}")
        if re.search(r"[A-Z]:[\\/](?:Users|PorgramarShit)[\\/]", text, re.I):
            failures.append(f"Personal absolute path: {name}")
        if re.search(r"(?:0x[0-9a-f]{2}\s*,\s*){96}", text, re.I):
            failures.append(f"Review embedded byte array: {name}")
    except UnicodeDecodeError:
        failures.append(f"Source must be UTF-8: {name}")
    return failures

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--package", type=Path)
    args = parser.parse_args()
    failures = []
    names = git("ls-files", "-z", "--cached", "--others", "--exclude-standard").decode().split("\0")
    for name in sorted(set(filter(None, names))):
        path = ROOT / name
        if path.is_symlink():
            failures.append(f"Unexpected symlink: {name}")
        elif path.is_file():
            failures += check_source(name, path.read_bytes())
    history = git("rev-list", "--objects", "--all").decode().splitlines()
    for line in history:
        if " " not in line:
            continue
        oid, name = line.split(" ", 1)
        if git("cat-file", "-t", oid).strip() == b"blob":
            failures += check_source(name, git("cat-file", "blob", oid))
    if args.package:
        package = args.package.resolve(strict=True)
        actual = {p.relative_to(package).as_posix() for p in package.rglob("*") if p.is_file()}
        if actual != PACKAGE_FILES:
            failures.append(f"Unexpected package contents: {sorted(actual ^ PACKAGE_FILES)}")
        for p in package.rglob("*"):
            if p.is_symlink():
                failures.append(f"Package symlink: {p.name}")
    if failures:
        print("\n".join(sorted(set(failures))), file=sys.stderr)
        return 1
    print("Repository" + (" and package" if args.package else "") + " content checks passed.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
