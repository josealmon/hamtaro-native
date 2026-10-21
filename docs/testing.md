# Validation

Build and run CTest as described in the README. Tests stay active in Release
builds and use explicit checks rather than assertions removed by NDEBUG.

- core: synthetic guest-memory behavior, DMA, BIOS/LZ77, video, timing/IRQ,
  original button transitions/repeat, PRNG arithmetic, heap and Main script control.
- game: repeated DMA working addresses, byte register writes, transfer queue wrap,
  video shadows, synthetic ROM map changes, preserved render fields, expression
  cursor/recursion boundaries, SRAM checksum/backup recovery, guest input caching
  and repeat, key ordinal priority, menu/dialog boundaries, synthetic font decoding,
  name-buffer/default handling, VRAM copies, text allocation failure, panel transitions
  and explicit missing-resource failures.
- render: animation repeat/call/delay flow, typed callback guards, physics,
  affine matrices, OAM drawing/order, background motion/repeats/calls/stacks and
  PPU regressions for tile mapping, transparency, mosaic/window phase and effects;
  word-choice slot setup, coordinate truncation, class gates and priority flags.
- graphics: palette interpolation/source wrapping, scripted repeat/call/delay
  flow, alpha/mosaic transitions, compressed palette frames, paused states and
  explicit faults for cyclic or unsupported commands.
  Raster tests cover allocation clearing/replacement, window sweeps and layer/DMA
  cleanup, radius arithmetic, circular HBlank phase/termination and unknown IRQ targets.
- audio: synthetic driver/player setup, parameters and fades, linked-channel
  release, typed sequence dispatch, note allocation, complete synthetic track
  progression, PCM interpolation/end/reverb behavior, PSG wave-register writes,
  script sound/wait controls and independent command/channel-cycle budgets.
- apu: four timers and cascade/IRQ timing, FIFO word lanes/shifters, sound DMA,
  stereo routing/PWM/averaging, PSG duty/length/envelope/sweep behavior, wave RAM
  banks/attenuation, maximal noise cycles and deferred frequency reloads.
  Whole-frame versus irregular advances produce identical mixed PCM/PSG output.
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
heap RAM, PRNG, input, expressions, video state, transfer queues, object animation
and drawing, all 61 background handlers, all 48 graphics handlers, palette transitions,
map/core initialization and SRAM initialization/repair/read/write behavior. It also
compares Main save-load commands with successful and failed records. The ROM
schedules cover 88 Main-only iterations, 180 coupled idle iterations and a separate
2,329 iterations using the guest input adapter and matching ReadKeys calls. The latter schedules call
background/object/graphics/mosaic updates, video transfers and audio service with
driver initialization and VBlank-counter synchronization. Graphics slots are active
in 161 input-schedule iterations; synthetic per-command/update tests provide the
remaining graphics evidence. It compares 3,581 sampled HBlank callbacks during
the circular transition; deferred DMA0/PPU timing is not modeled. Task/actor updates, CPU-transfer flushing, real IRQ
cadence and hardware audio output are omitted. DMA3 stack-local source registers
are excluded, while transferred data and sound DMA1/2 register images are compared.
The CPU oracle uses explicit BIOS, immediate-DMA and IO-mask models, not a complete
GBA machine. The complete run with the optional PPU oracle passes 269,738 comparisons.
It does not exercise the game as a whole and is not part of ROM-free public CI.

The audio part can be run independently with the same private ROM and reference:

    python -B tools/compare_reference.py --audio-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

It passes 57,988 function/state comparisons, including 300 song-control selections,
240 synthetic note-allocation cases and 120 player updates for each of 12 actual
ROM sequences. It also compares 1,440 complete mixer/PSG-service calls over those
12 sequences, including the eight-bit PCM DMA planes and relevant sound/timer
register images. Of those snapshots, 349 contain nonzero PCM. Synthetic cases cover
PSG envelopes, PCM loop/end/interpolation boundaries and reverberation; Main and
animation audio commands are compared separately. The CPU oracle supplies only
the frequency initializer's VCOUNT boundary. It does not model elapsed audio time,
FIFO/DAC mixing or PSG waveform generation. Final stereo output, audible playback,
real game integration and complete-song duration remain unverified.

An independent static PPU comparison uses an existing mGBA 0.10.5 source checkout
and a compatible C compiler. It is a separate test project, not an application
dependency. For example, with Ninja and the compiler on PATH:

    cmake -S tests/integration/mgba -B "$env:HAMTARO_NATIVE_WORK/mgba-oracle" -G Ninja -DCMAKE_BUILD_TYPE=Release -DMGBA_SOURCE="$env:MGBA_SOURCE"
    cmake --build "$env:HAMTARO_NATIVE_WORK/mgba-oracle" --target hn-video-oracle
    python -B tools/compare_video.py --oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-video-oracle.exe"

Set HAMTARO_NATIVE_WORK explicitly for these commands. MGBA_SOURCE points to the
caller-provided source checkout. If the compiler needs runtime DLLs outside PATH,
pass --runtime-dir PATH_TO_COMPILER_RUNTIME to compare_video.py. The suite compares
64 synthetic static frames (2,457,600 pixels) without a ROM or output captures.
The RGB555/channel normalization and mosaic/window isolation are described in
architecture.md; this is not a claim of complete hardware conformance.

Add --video-oracle PATH_TO_ORACLE and, if needed,
--oracle-runtime-dir PATH_TO_COMPILER_RUNTIME to compare_reference.py to compare
13 static snapshots from its private ROM schedule; eleven contain multiple colors.
No snapshots or ROM data are saved.

Scene/input and supporting Main commands can be checked separately:

    python -B tools/compare_reference.py --scenes-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

This passes 111,252 private comparisons, including guest key-cache initialization,
repeat/direction/query behavior, masked key ordinals, PRNG ranges, render-slot setup,
menu movement, language snapshots, name editing/confirmation, 168 glyph-blit boundary
cases, keyboard grid navigation, panel entry/exit/depth, raster commands/profiles,
24 circular HBlank scenarios, record selection, random script values, core reset and
choice-option flags. Resources are read from the caller's ROM; no captures or extracted
tables are written. The coupled schedule exits both name fields and completes the
circular transition. Its next unbound callback is ScriptCallbackSetBackground2RightBottom1024;
this is still component evidence, without the full game loop or task/actor execution.

The same separate project can build the ROM-free hardware audio oracle:

    cmake --build "$env:HAMTARO_NATIVE_WORK/mgba-oracle" --target hn-audio-oracle
    python -B tools/compare_audio.py --oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-audio-oracle.exe"

Use --oracle-runtime-dir PATH_TO_COMPILER_RUNTIME if required. It uses the native
test DLL from build/Release by default, or --native PATH_TO_TEST_DLL. The oracle
loads no ROM and executes no CPU instructions. It compares 10,432 synthetic
snapshots (698,616 fields) across timers, Direct Sound, pulse/noise envelopes,
wave banks/attenuation, sweep/control state and PSG routing/volume components.
The native apu suite has 786 checks, including documented startup, physical DAC
polarity and complete mixed-output partition invariance. architecture.md records
the controlled noise seed, steady wave alignment, inactive-counter exclusions
and digital DAC input normalization in this component oracle. Its final stock
PSG waveform is not the reference for these checks; trigger quirks, analog
filtering, listening evidence and actual game-output parity remain unverified.

Manual UI checks cover import/cancellation, persisted rebinding, keyboard menu
navigation, constrained window sizes, fullscreen, and save import/export.
Physical controller compatibility must be reported by device actually tested;
a virtual-pad pass does not prove every USB/Bluetooth device works.

Repository checks:

    python -B tools/check_repository.py
    git diff --check

These catch common accidental private/generated files and source encodings.
They do not determine the copyright status of reconstructed source code.
