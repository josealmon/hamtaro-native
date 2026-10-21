# Validation

Build and run CTest as described in the README. Tests stay active in Release
builds and use explicit checks rather than assertions removed by NDEBUG.

- core: synthetic guest-memory behavior, DMA, BIOS/LZ77, video, timing/IRQ,
  original button transitions/repeat, PRNG arithmetic, heap and Main script control.
- storage: SHA-256 known vectors, strict ROM rejection, atomic SRAM recovery,
  independent profile locking, settings validation and Unicode paths.
- input: injected keyboard events and a local SDL virtual gamepad. Covers hotplug,
  deadzone, diagonals, opposite-direction cancellation, remapping and disconnect.
- app-smoke: creates an isolated temporary profile, initializes the actual
  SDL renderer and menus for six frames, closes cleanly and removes that profile.
  No graphics, settings, ROM or save files are written in the checkout.

The optional private ROM check is:

    build/Release/hn_storage_tests.exe PATH_TO_YOUR_AH3P_ROM

It verifies a positive import and a modified-ROM rejection while preserving a
synthetic save. It uses a temporary profile and does not modify the source ROM.

The executable also accepts:

    hamtaro-native.exe --verify-rom PATH_TO_YOUR_AH3P_ROM

Exit code 0 means identity and full SHA-256 match; 2 means the content does not
match. No profile import is performed by this command.

For private function comparisons, use a Python environment with Unicorn 2.1.4
and the test-only hn_reference_probe.dll built by BUILD_TESTING:

    python -B tools/compare_reference.py --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

The reference must contain build/function-symbols.json. This check writes no
ROM or captured state, and does not alter the reference checkout. It compares
heap RAM, PRNG, input and supported Main command behavior. It does not exercise
the game as a whole and is not part of ROM-free public CI.

Manual UI checks cover import/cancellation, persisted rebinding, keyboard menu
navigation, constrained window sizes, fullscreen, and save import/export.
Physical controller compatibility must be reported by device actually tested;
a virtual-pad pass does not prove every USB/Bluetooth device works.

Repository checks:

    python -B tools/check_repository.py
    git diff --check

These catch common accidental private/generated files and source encodings.
They do not determine the copyright status of reconstructed source code.
