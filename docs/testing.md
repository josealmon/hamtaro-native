# Validation

Build and run CTest as described in the README. Tests stay active in Release
builds and use explicit checks rather than assertions removed by NDEBUG.

- core: synthetic guest-memory behavior, DMA, BIOS/LZ77, video, timing/IRQ,
  angular axes/quadrants/monotonicity, original button transitions/repeat, PRNG
  arithmetic, heap and Main script control.
- game: repeated DMA working addresses, byte register writes, transfer queue wrap,
  video shadows, synthetic ROM map changes, preserved render fields, expression
  cursor/recursion boundaries, SRAM checksum/backup recovery, guest input caching
  and repeat, key ordinal priority, menu/dialog boundaries, synthetic font decoding,
  name-buffer/default handling, VRAM copies, text allocation failure, panel transitions
  and explicit missing-resource failures. Actor checks cover Main-state restoration,
  waits, same-frame activation, stop, padding and bounded/unsupported script failures.
  Counter checks cover palette/template banks, signed digits and the low yield byte.
  Motion checks cover route edges/obstacle order, collision setup and resource branches.
  Following checks cover walk/run speeds, boot-gated position updates, near-leader
  stopping, delayed idle animation, asymmetric quantization and circular route bends.
  Collision checks cover signed map wrapping, edge masks, event staging, inclusive
  containment/exclusive interactions, circle/rectangle responses, inactive records,
  action gating and movement-wait completion/pending-result handling.
  Task checks cover list removal/relinking, synthetic glyphs, independent contexts,
  delay/fast advance, name returns, tile-row scrolling, numeric text, unsupported
  callbacks, cyclic lists, memory faults and command budgets.
  Word-choice checks cover initial/requested selection, grid/highlight boundaries,
  configuration selectors, reset ownership, retained key-repeat settings and Main
  result preservation during background-controlled closure.
  Scene actor checks cover four render snapshots, inactive-slot retention and
  animation execution-flag clearing.
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
  deadzone, diagonals, opposite-direction cancellation, remapping, disconnect and
  one-update delivery of taps pressed/released between game updates.
- session: invalid-ROM rejection without file writes. With a private ROM argument,
  verifies initialization persistence, failed-write retry, retained backups and
  rejection of partially modified SRAM after a native runtime fault.
- app-smoke: creates an isolated temporary profile, initializes the actual
  SDL renderer and menus for six frames, closes cleanly and removes that profile.
  No graphics, settings, ROM or save files are written in the checkout.

The optional private ROM check is:

    build/Release/hn_storage_tests.exe PATH_TO_YOUR_AH3P_ROM

It verifies a positive import and a modified-ROM rejection while preserving a
synthetic save. It uses a temporary profile and does not modify the source ROM.

Additional private session and real audio-device checks use:

    build/Release/hn_session_tests.exe PATH_TO_YOUR_AH3P_ROM
    build/Release/hn_session_tests.exe PATH_TO_YOUR_AH3P_ROM --audio

The first passes 24 checks. The second passes 1,233 checks, including 600 updates
paced at the original game rate and submitted to the actual SDL audio device with
zero requests short of samples. It plays the generated game sound and requires an
audio device. Both own a temporary profile, preserve the source ROM, and remove
their temporary data on exit. Neither writes captures in the checkout. User
listening also confirmed the corrected menu playback; complete game-output parity
and physical controller coverage remain separate work.

The executable also accepts:

    hamtaro-native.exe --verify-rom PATH_TO_YOUR_AH3P_ROM

Exit code 0 means identity and full SHA-256 match; 2 means the content does not
match. No profile import is performed by this command.

An input-only native runtime scenario uses the optional test DLL:

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/opening.json

It runs 16,932 clocked native updates from erased SRAM, confirms bedroom control,
walks to the Clubhouse, displays the initial ten-word book and returns to the room.
It performs Hamha, Tack-Q, Hif-hif and Digdig, checks a wrong-word response and retry,
and confirms the ten-seed reward and subsequent conversation. The
JSON contains only button inputs and metadata checkpoints. The runner writes no
save or capture, rejects unknown inputs and fails on any native runtime fault or
checkpoint mismatch. --save PATH supplies optional read-only 32 KiB SRAM input.
This is native runtime scenario evidence, not a desktop playthrough or an independent
full-machine comparison. Manual book navigation, later scenes and the complete
desktop story playthrough remain pending.

For private function comparisons, use a Python environment with Unicorn 2.1.4
and the test-only hn_reference_probe.dll built by BUILD_TESTING:

    python -B tools/compare_reference.py --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

The reference must contain build/function-symbols.json. Hardware initialization
is compared at three VCOUNT values, including IRQ dispatcher data and video/IO
state. This check writes no ROM or captured state, and does not alter the reference
checkout. It compares
heap RAM, PRNG, input, expressions, video state, transfer queues, object animation
and drawing, all 61 background handlers, all 48 graphics handlers, palette transitions,
map/core initialization and SRAM initialization/repair/read/write behavior. It also
compares Main save-load commands with successful and failed records. The ROM
schedules cover 88 Main-only iterations, 180 coupled idle iterations and a separate
9,000 iterations using the guest input adapter and matching ReadKeys calls. The latter schedules call
gated follower motion, actor scripts, text tasks, background/object/graphics/mosaic updates, video transfers and audio service with
driver initialization and VBlank-counter synchronization. Graphics slots are active
in 262 input-schedule iterations; synthetic per-command/update tests provide the
remaining graphics evidence. It compares 3,581 sampled HBlank callbacks during
the circular transition; deferred DMA0/PPU timing is not modeled. The full word-choice game path, CPU-transfer flushing, real IRQ
cadence and hardware audio output are omitted. DMA3 stack-local source registers
are excluded, while transferred data and sound DMA1/2 register images are compared.
The CPU oracle uses explicit BIOS, immediate-DMA and IO-mask models, not a complete
GBA machine. The complete run with BIOS and PPU oracles passes 538,420 comparisons,
including 401 distinct leader positions during directional input.
It does not exercise the game as a whole and is not part of ROM-free public CI.
The schedule ends with MainScriptWaitForByte1FA8 (0x21) active after exercising
directional movement. Word-choice handlers use opcodes 0x24 (await) and 0x45
(configure). The desktop loop is now connected; complete playthrough remains pending.

The Main memory and scene-transition group can also run separately:

    python -B tools/compare_reference.py --scripts-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

This passes 7,241 comparisons: byte/halfword pointer copies, zero/max counts,
variable indirection and overlap, graphics disable-except, display-layer flags,
and 96 actor snapshot/modal-map roundtrips across six map callbacks. Cases include
optional/absent actors, all direction variants, retained cursor and first/third
variant reuse for the fourth restored actor. Input data is synthetic; original
animation families and map descriptors remain in the private ROM. It also covers
unsigned screen-Y sides, signed slot-10 half-height tests and actor turn control,
including leader/follower aliases and preserved opposite-direction state.

The Ham-Chat book group can run separately:

    python -B tools/compare_reference.py --book-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

It passes 25,920 comparisons: all acquisition byte values, five language orders,
learned/sorted pages, absent and duplicate selections, category boundaries, page
counts, markers and decorations. Paired header/column tasks include allocation,
scripts and reverse cleanup; number cases cover leading blanks, coordinate wrap,
high-word truncation and retained unused slots. Resources stay in the private ROM.

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
FIFO/DAC mixing or PSG waveform generation. Native menu playback and SDL delivery
have separate session/listening evidence; full stereo parity and complete-song
duration remain unverified.

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
30 static snapshots from its private ROM schedule; twenty-seven contain multiple colors.
No snapshots or ROM data are saved.

Scene/input and supporting Main commands can be checked separately:

    python -B tools/compare_reference.py --scenes-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

This passes 127,051 private comparisons, including guest key-cache initialization,
repeat/direction/query behavior, masked key ordinals, PRNG ranges, render-slot setup,
menu movement, language snapshots, name editing/confirmation, 168 glyph-blit boundary
cases, keyboard grid navigation, panel entry/exit/depth, raster commands/profiles,
24 circular HBlank scenarios, record selection, random script values, core reset and
choice-option flags, three counter banks and indirect arithmetic/alias behavior.
Counter cases cover zero/capped amounts, oversized subtraction, signed limit tests,
wrapped countdowns, small/large step delays and repeated increment/decrement sounds.
Resources are read from the caller's ROM; no captures or extracted tables are written.

Actor scripts and motion setup can also be checked separately:

    python -B tools/compare_reference.py --actors-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

This passes 3,478 private comparisons. Actor fixtures cover variable-resolved setup,
the NULL sentinel, indirect callback storage, nested script calls, waits, Main-state
restoration and same-frame activation. Motion fixtures cover 400 tile-route queries,
120 route builds, 80 scene/resource configurations and 64 supporting Main command
cases. The compressed branch uses a synthetic stream with a nonzero low header
nibble; no collision map or game script is embedded. This subset does not cover
leader movement; the complete game loop remains unverified.

Motion and live follower comparisons can be run separately:

    python -B tools/compare_reference.py --motion-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

This passes 15,768 private comparisons. In addition to motion setup/routes, it
checks 1,024 direction/classification/quantization cases, 320 circular route
extensions, 1,280 follower updates and 320 gated-following updates. Cases cover
fractional/negative coordinates, route consumption and wrap, proximity resets,
walk/run modes, animation delay and the original INT_MIN direction quirk. Full
EWRAM/IWRAM state is compared. Leader input/collisions remain outside this suite.

Leader/collision comparisons use the independent BIOS arithmetic oracle:

    cmake --build "$env:HAMTARO_NATIVE_WORK/mgba-oracle" --target hn-bios-oracle
    python -B tools/compare_bios.py --oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-bios-oracle.exe"
    python -B tools/compare_reference.py --collisions-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-bios-oracle.exe"

Pass --oracle-runtime-dir PATH_TO_COMPILER_RUNTIME to either tool if needed.
The ROM-free math suite compares all 32,769 ArcTan inputs in -16384..16384 and
4,240 signed-half ArcTan2 vectors: 37,009 comparisons. The separate oracle compiles
mGBA 0.10.5's arithmetic helpers directly; no ROM, BIOS image or CPU execution is used.
The native adaptation is isolated in src/gba/bios_math.c with its MPL 2.0 notice.
Hardware BIOS arithmetic and instruction latency remain unverified.

The collision subset passes 15,362 private comparisons, including 216 independent
angular responses. It covers mask classification, sample pointers, shape containment,
interaction queries, solid-record responses, record lists, map flags/scripts,
action/reset controls, leader movement, combined updates and Main's movement wait.
Positive-response assertions require both circle and rectangle contacts. The full
comparison requires --bios-oracle; other focused suites can run without it.

Text-task checks can be run separately:

    python -B tools/compare_reference.py --tasks-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

This passes 24,197 private comparisons: allocation/removal, 56 graphics constructors,
all 32 text commands, both stages of tilemap scrolling, render indicators, clipped
row transfers, 2,048 numeric formatting cases, bundle retargeting, 27 typed void
callbacks and 56 text-update scenarios. Width/count/centering cases span all five
languages and three variants. Full EWRAM/IWRAM state, VRAM and DMA3 destination/control
are compared where relevant; the transient DMA3 source address is excluded.
No game text, font, animation table or captured state is written to the checkout.

Word-choice comparisons can be run separately:

    python -B tools/compare_reference.py --choices-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

This passes 15,850 private comparisons. It checks leader/follower idle animations,
96 synthetic descriptor layouts, selection/highlighting, 45 complete dialog state
scenarios, expression-filtered options, directional priorities, confirmation,
blocked/allowed cancellation, requested/delayed closure, Main configuration/await
protocols, concurrent updates of all nine text tasks and all seven reset states.
Main handler indexes are checked against the
ROM dispatch table before direct function comparisons. Dialog state, linked tasks,
graphics/background state, audio state, VRAM and DMA effects are compared; no
captured resources are saved. These checks do not establish the full-game path
through a word-choice dialog or a native playthrough.

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
filtering and actual game-output parity remain unverified. Menu listening evidence
is recorded separately above.

Manual UI checks cover import/cancellation, persisted rebinding, keyboard menu
navigation, constrained window sizes, fullscreen, and save import/export.
Physical controller compatibility must be reported by device actually tested;
a virtual-pad pass does not prove every USB/Bluetooth device works.

Repository checks:

    python -B tools/check_repository.py
    git diff --check

These catch common accidental private/generated files and source encodings.
They do not determine the copyright status of reconstructed source code.
