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
  SaveGroup3 checks cover list shifting, field masks, halfword underflow, mask
  cursor wrapping, stable temporary-text lifetime, incremental prefixes, end-of-text
  signaling, allocation failure and rejected task contracts.
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

It runs 134,892 clocked native updates from erased SRAM, confirms bedroom control,
walks to the Clubhouse, displays the initial ten-word book and returns to the room.
It performs Hamha, Tack-Q, Hif-hif and Digdig, checks a wrong-word response and retry,
and confirms the ten-seed reward and subsequent conversation. Destination checkpoints
then select Sunny Peak, return the highlight to Clubhouse, select Sunny Peak again
and confirm travel to player control in map 2:1. The route then enters the river
scene, advances dialogue, executes Hamha, opens the bridge's filtered word menu,
selects Tack-Q and returns to player control. It then reopens the menu, selects
Lookie, advances the response and walks to the river's eastern approach. It enters
map 2:17, reaches control, opens an actor word menu, selects Hamha and completes
the conversation, returning to movement with its words and seed count retained.
It then returns to the river, takes the northern path through map 2:20, climbs the
stairs to map 2:2 and triggers the stump conversation. Heyhoo acquisition increases
the word count from ten to eleven; the scenario opens the updated book, enters its
first page and returns to the same movement position. Select opens the status
display; the route moves to End Game, confirms the original SRAM write, returns
to the title and chooses Continue. The same map, position, eleven learned words
and ten seeds are restored. Oxnard's conversation then teaches Bluhoo, and the
eastern handkerchief scene teaches Gogo. A second status/End Game/Continue restores
map 2:3 at the same position with thirteen words and ten seeds.
Crossing the upper maps and descending the vines reaches Arnie and the boat dock.
Lookie teaches Pakapaka; cutting the boat's rope carries the player west to map
2:17. Hamha and Yep-P with the wandering hamster teach Putput. A third
status/End Game/Continue restores map 2:17 at x=131.5, z=208.5 with fifteen words
and ten seeds.
Heyhoo summons Seamore and learns Offdoff. The route removes the bridge's battery,
places it in the boat, exercises powered travel through map 2:18 and docks beside
Bijou. Mega-Q increases the count to seventeen; the joined-partner flag and
follower slot 13 are checked after returning to movement. A fourth End Game/Continue
retains that state at map 2:17, x=168.5, z=80.5. Returning by boat reaches the Ham-lift
lesson. The pair lowers Pepper from the vine, chooses Bluhoo, learns Oopsie and
Gossip-P and completes the first reconciliation. Control returns in map 2:2 at
x=112.5, z=88.5 with twenty words and one rescued love. Select/status and a fifth
End Game/Continue retain the map, position, ten seeds and Bijou as well. The later
Spat conversation leads to Harmony and the destination map. Returning to Clubhouse
and a sixth End Game/Continue restore map 1:1 at x=144.5, z=256.5, twenty-one words,
ten seeds, the joined partner and one rescued love. The checkpoints establish the
word count rather than the exact dialogue line that teaches it. The runtime
frame counter resets at the title, while the runner's update total stays cumulative.
Complete Sunny Peak coverage remains unverified. The JSON contains only button inputs
and metadata checkpoints. The default replay writes no
save or capture, rejects unknown inputs and fails on any native runtime fault or
checkpoint mismatch. --save PATH supplies optional read-only 32 KiB SRAM input.
This is native runtime scenario evidence, not a desktop playthrough or an independent
full-machine comparison. Manual book navigation, later scenes and the complete
desktop story playthrough remain pending.

To replay the same inputs through the application's host Session API and test
actual SRAM-file persistence, add:

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/opening.json --session-test build/Release/hn_session_tests.exe

The host replay performs all 134,892 input updates in a disposable private profile, verifies
that the saved 32 KiB file equals the game's SRAM, stops and starts a fresh Session,
then selects Continue. Clubhouse map/position, twenty-one words, ten seeds, one
rescued love, the joined-partner flag and follower slot 13 are unchanged; 142,033
host-session checks pass. The partner checkpoints describe the
returned overworld state; UI scenes can reuse the same render-class slots.
The original ROM and player's profile are untouched; temporary input traces and
SRAM files are removed. Trace scratch uses HAMTARO_NATIVE_WORK or the private
.hamtaro-native-dev profile outside the checkout. This option starts from erased
SRAM and cannot be combined with --save. It does not create an SDL gameplay window
or prove a complete desktop playthrough.

For desktop review, append --desktop-profile PATH_TO_NEW_PRIVATE_PROFILE. The
host replay including this export retains the validated user ROM and tested SRAM
in that new directory. The directory must
be outside the checkout and must not already exist. Start the actual application
with --data-dir PATH_TO_NEW_PRIVATE_PROFILE to inspect the same saved scene.
This explicit private export differs from the default disposable test profile.
It adds two profile-export checks to the default replay's count.

A real-window review launched the Windows executable with the exported twenty-one-word
profile, selected Continue and restored the Clubhouse scene. Select, assigned to V
in the copied test settings, displayed twenty-one words,
ten seeds and one rescued love. End Game returned to the title and updated the
private 32 KiB SRAM file; its previous bytes exactly matched the backup. The
executable was then closed and relaunched. Continue restored the same scene and
Select displayed the same three counters. The review used Z/X and WASD bindings;
the fresh host-session replay separately checks all nine story/position/partner/
counter fields from disk.
This is bounded desktop evidence, not a full playthrough or a physical-controller
test; the default Right Shift binding and physical arrow keys were not established by
this window review.

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
GBA machine. The complete run with BIOS and PPU oracles passes 1,835,252 checks,
including 401 distinct leader positions during directional input.
It does not exercise the game as a whole and is not part of ROM-free public CI.
The schedule ends with MainScriptWaitForByte1FA8 (0x21) active after exercising
directional movement. Word-choice handlers use opcodes 0x24 (await) and 0x45
(configure). The desktop loop is now connected; complete playthrough remains pending.

The Main memory and scene-transition group can also run separately:

    python -B tools/compare_reference.py --scripts-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

This passes 133,074 comparisons: byte/halfword pointer copies, zero/max counts,
variable indirection and overlap, graphics disable-except, display-layer flags,
and 96 actor snapshot/modal-map roundtrips across six map callbacks. Cases include
optional/absent actors, all direction variants, retained cursor and first/third
variant reuse for the fourth restored actor. Input data is synthetic; original
animation families and map descriptors remain in the private ROM. It also covers
unsigned screen-Y sides, signed slot-10 half-height tests and actor turn control,
including leader/follower aliases and preserved opposite-direction state.
Actor transition cases cover coordinate snapshots, both reposition selectors,
primary/secondary slot aliases, animation retention, boot-gate ownership, running
delays and signed fixed-point facing interpolation with wrapped products/rounding.
Scene cases additionally cover piecewise actor remapping/restoration with live
aliases, surface-height region boundaries and overshoot, clamped approach/mode
transfers, linked slot setup and explicit Main/animation callback ABI sharing.
The shared scene-motion families add 36,713 comparisons covering all 40 functions
in class_anchor_presets.c and scene_control_callbacks.c. Cases include strict and
wrapped rectangle/corridor boundaries, signed aiming and alignment, both animation
callbacks, render/control overlays, complete clone phase sequences, signed payload bytes,
background offset boundaries and 512 PRNG seed triples with signed/overflow values.
The public game suite also covers missing-slot guards, clone countdown wrapping,
retained slot-37 state and argument/save-flag alias order. Full activity playthroughs
remain outside this family-level evidence.

The destination-map group can run separately:

    python -B tools/compare_reference.py --indicators-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

It passes 31,847 comparisons: completion flags with each requirement removed,
all byte indices, visibility masks and shift boundaries, marker states, five
language backgrounds, refresh retention, sequential unlocks and navigation.
Navigation covers all 256 unlock-byte values, both navigation modes, zero/combined
directions, locked destinations, same-area selection and original menu sounds.

The scene-render presets and their actor dependencies can run separately:

    python -B tools/compare_reference.py --presets-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

This passes 33,728 private comparisons: all 77 scene-render preset functions and
28 required actor helpers, direct calls, F4 task dispatch and the 35 Main callbacks
with defined zero returns. Cases cover class aliases, conditional controls,
animation execution flags, boot-gate ownership, graphics reset state, linked slots,
indicator ownership, field60-only clearing and class selectors overwritten during
configuration. The public suite checks those ABI boundaries and rejects unknown
targets and missing slots. Other functions in the actor-helper source families and
complete activity/story integration remain pending. Resource contents are never
embedded in fixtures or written by this comparison.

The complete map-choice and particle families use the independent BIOS oracle:

    python -B tools/compare_reference.py --map-choice-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

Pass --oracle-runtime-dir PATH_TO_COMPILER_RUNTIME if the oracle needs external
runtime DLLs. This group passes 195,507 private comparisons across all 273 map-choice
functions, 11 actor dependencies, four particle functions, direction queries and
relative planar velocity. It checks direct calls, 93 Main contracts and 253 reviewed
task targets, all five animation contracts and all four slot helpers. Cases cover
all packed-choice byte selectors, signed/wrapped region boundaries, retained flags,
positive/negative/zero motion, predicted clamps, complete follower phase branches,
conditional graphics/animation changes and particle reset boundaries. Main cursor
jumps and overlapping position/script variables are compared with the original.
The source's unusual integer wrapping and height overshoot are preserved. Fixtures
use synthetic state and caller-provided resources, with no captures or extracted
tables written to disk. Complete area/activity playthroughs remain separate evidence.

The complete bank-2 actor/control/preset/height group uses the BIOS oracle:

    python -B tools/compare_reference.py --bank2-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

This group passes 35,982 comparisons across all 120 C functions and three reviewed
assembly leaves. It compares direct calls, 116 F4 task targets, 70 Main contracts,
the height-latch animation's full-word yield and Main51's void dispatch. Fixtures
cover all four panel stages, aliased stage resets, wrapped coordinates, conditional
presets, original PRNG consumption, 480 consecutive motion updates, velocity clamps
and exact signed height boundaries. Public checks reject void functions as Main1C
callbacks and preserve the unusual Main51 slot-address failure. Resources stay in
the private ROM; no captured state or extracted table is written. These checks
do not establish the complete Sunny Peak route or a reconciliation.

The connected shared actor movement/direction families use the BIOS oracle:

    python -B tools/compare_reference.py --wander-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

This passes 58,449 comparisons across 23 source functions: four animation contracts,
17 Main contracts and two pointer-based helpers, including four argument-free F4
targets. It covers coordinate readers, mutual facing, stepping/class-turn sequences,
all byte directions, signed/wrapped bounds, optional probes and a 2,400-update
synthetic move/pause schedule. Source/render/control/argument aliases, shared consumed counts,
missing-record yield retention, distinct callback/captured actors, negative signed
slot indexes and duplicate animation setter effects are compared. The public tests
check explicit unsupported probes and invalid wandering states; no CPU interpreter
or test oracle is linked into the game. Fixtures remain synthetic or read resources
from the private ROM, without writing extracted data. The input scenario separately
checks eastern-map movement, actor facing and completion of a conversation. These
checks do not establish a complete area or the full desktop story playthrough.

The Ham-Chat book group can run separately:

    python -B tools/compare_reference.py --book-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

It passes 25,920 comparisons: all acquisition byte values, five language orders,
learned/sorted pages, absent and duplicate selections, category boundaries, page
counts, markers and decorations. Paired header/column tasks include allocation,
scripts and reverse cleanup; number cases cover leading blanks, coordinate wrap,
high-word truncation and retained unused slots. Resources stay in the private ROM.

The audio part can be run independently with the same private ROM and reference:

    python -B tools/compare_reference.py --audio-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

It passes 81,029 function/state comparisons, including 300 song-control selections,
240 synthetic note-allocation cases and 120 player updates for each of 12 actual
ROM sequences. It also compares 1,440 complete mixer/PSG-service calls over those
12 sequences, including the eight-bit PCM DMA planes and relevant sound/timer
register images. Of those snapshots, 349 contain nonzero PCM. Synthetic cases cover
PSG envelopes, PCM loop/end/interpolation boundaries and reverberation; Main and
animation audio commands are compared separately. Region callback cases cover
state transitions, signed boundaries, volume truncation, song suppression and
yield semantics. These do not establish audible area mixing. The CPU oracle supplies
only the frequency initializer's VCOUNT boundary. It does not model elapsed audio time,
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

This passes 16,462 private comparisons. It checks leader/follower idle animations,
96 synthetic descriptor layouts, selection/highlighting, 45 complete dialog state
scenarios, expression-filtered options, directional priorities, confirmation,
blocked/allowed cancellation, requested/delayed closure, Main configuration/await
protocols, concurrent updates of all nine text tasks and all seven reset states.
Long candidate lists cover up to 255 entries filtered to nine visible options,
expression-cursor advancement, high candidate IDs and 16-bit disabled-mask limits.
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

Before adapting a source family, inspect its read-only planning inventory:

    python -B tools/inventory_reference.py --reference PATH_TO_READ_ONLY_DECOMP
    python -B tools/inventory_reference.py --reference PATH_TO_READ_ONLY_DECOMP --source "src/actors/*.c" --json
    python -B tools/inventory_reference.py --reference PATH_TO_READ_ONLY_DECOMP --area-bank 2
    python -B tools/inventory_reference.py --reference PATH_TO_READ_ONLY_DECOMP --script-source "shared*main_scripts.c"

The default table groups C/ASM functions and callback-named candidates by original
source. JSON additionally lists function names, addresses and explicit native
dispatch ABIs. It reads metadata and native source only, writes no report and never
loads a ROM or executes reference code. A missing dispatch candidate needs review;
an existing one still needs contract and behavior checks. This is neither a complete
indirect-call inventory nor a completion percentage. Follow the batch order and
validation boundaries in architecture.md; keep any redirected output outside Git.

Additional focused area-dependency checks use:

    python -B tools/compare_reference.py --scene-actors-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP
    python -B tools/compare_reference.py --actor-presets-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE
    python -B tools/compare_reference.py --records-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP
    python -B tools/compare_reference.py --wave-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP
    python -B tools/compare_reference.py --counter-scenes-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

These pass 37,798 scene actor comparisons, 91,597 actor preset comparisons,
30,930 record comparisons and 16,497 scanline-wave comparisons. The scene group
covers 44 functions across 15 families and one shared bank-3 condition, including
approach completion, signed limits, all ARM shift counts, direction constructors,
snapshots and copy aliases. The preset group covers all 476 functions in five
families, Main rejection of void callbacks, F4 dispatch, direction sectors,
captured auxiliary arguments versus live selector aliases and inactive mapped
byte selectors at 0, 1, 254 and 255. Record checks cover
allocation/empty/full slots, twelve-byte result views, consuming zero counts,
sentinel counters, arithmetic wrap and allocation callbacks. Batch allocation checks
cover all 65 occupancy values, zero/full capacity, amount and animation timing
boundaries, inactive mapped slots and byte-wise argument aliases. Wave checks cover
all 228 scanlines, blend thresholds, signed wrapping, frame-end phase updates and
typed Main/F4/HBlank calls. Public synthetic game tests cover these boundaries
without original assets. These comparisons do not establish full-area progression,
the complete inventory UI, SRAM integration or timed raster-image fidelity.

The counter-scene group passes 179,460 comparisons across all thirteen functions
in map10_4_counter_sequence.c. It covers every 16-bit counter value, five tile banks,
palette/window setup, byte-sized offsets, stage graphics, all active sequence
phases, signed halfword/wrap boundaries, same-call phase fallthrough and live class
aliases. Nine Main contracts and twelve argument-free F4 contracts are compared.
The draw callback's invalid selector retains an incoming R6 pointer in the source;
the host rejects that unreviewed contract before changing any tile bank. These
checks and the opening scenario establish initial Select/status and End Game use;
later stage animations, Items/Gems and the complete activity UI remain unverified.

Shared actor geometry comparisons use:

    python -B tools/compare_reference.py --shared-motion-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

The group passes 12,351 comparisons across eleven functions in eight complete
source families, with ten Main and three F4 contracts. It checks projected target
velocity, target/actor offsets, approach initialization/completion, selector 255,
one-pixel/zero steps, live render aliases, leader/offset storage overlap, signed
clamping, inactive raw direction selectors, screen cells/sides, sequential class
coordinate aliases, signed-half absolute overflow, render pairs and video mode.
The original nine-byte class approach remains in the full scene-actor comparisons.
Synthetic public checks cover the empty screen grid separately; the private BIOS
probe rejects division by zero and does not establish that hardware behavior.

Shared screen and shake comparisons use:

    python -B tools/compare_reference.py --scene-effects-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

The group passes 27,793 comparisons across eight functions in five complete source
families, with eight Main and eight argument-free F4 contracts. It compares RAM and
window registers for display mode/flags, screen-window bounds, anchor resets,
every byte-sized phase/layer selector, amplitude/rate/wrap boundaries and continuous
320-update shake sequences. Raw inactive render slots, captured background anchors,
overlapping position/control/class storage and a position write over the render-base
pointer are included. Synthetic public checks independently establish signed-division
truncation, halfword wrapping, the fixed layer-2 anchor, rate-zero behavior and
rejection of unreviewed animation contracts. The long native scenario exercises
the first reconciliation's screen effects; complete scene timing remains unverified.

The related graphics/directional/extended actor animation families use:

    python -B tools/compare_reference.py --actor-animations-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

This group passes 85,975 comparisons across all 446 functions in three source
families, including direct calls, F4 continuation and rejection of void functions
as Main callbacks. It covers direction sectors, captured auxiliary arguments,
live class-selector aliases, separate graphics initialization and every function
with byte selectors at 0, 1, 254 and 255 while only one actor/graphics record is
active. Synthetic public checks also verify wrapped neighbors, the missing-sector
no-op, asymmetric animation/indicator classes and preservation of the general
APIs' active-count guards. Full later-area integration remains unverified.

The complete bank-3 late scene family uses:

    python -B tools/compare_reference.py --bank3-late-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

This group passes 31,045 checks for all 31 source functions plus song-43 restart
and its stopped-track dependency. The original late family has 25 void presets,
four animation callbacks and two Main modal callbacks, with 27 reviewed F4 targets.
It compares EWRAM, IWRAM excluding the oracle stack, palette, VRAM, OAM and SRAM.
Cases cover mapped inactive byte-selected actors, signed centered positions, all
256 pan/tempo counters and mirror bytes, captured scale X versus live scale Y,
matrix exhaustion returning 32 and its adjacent usage-byte alias, and modal actor
snapshots with optional slots, both animation families and script-cursor restoration.
Audio cases initialize actual song/player records, ten active/inactive tracks,
the eight-bit mask and player identity, then compare continuous callback sequences.
Synthetic public tests independently check pan/tempo values, upper mirror bits,
affine allocation/exhaustion, raw snapshot behavior and rejected calling contracts.
Song-43 cases cover all volume bytes, stopped/empty/playing states, valid and
invalid player identities, Main cursor effects and F4 calls. Stopped-track restart
checks all status bytes, counts 0/1/2/8/16/17/255, retained 16-byte record tails and
overlapping player storage. Synthetic public checks separately verify its five
defaults, unchanged active-only tracks and the eight-bit volume mask.
The complete 282-function bank-3 main family is split into focused groups:

    python -B tools/compare_reference.py --bank3-presets-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE
    python -B tools/compare_reference.py --bank3-games-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE
    python -B tools/compare_reference.py --bank3-chain-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

The preset group passes 78,329 comparisons for 212 scene functions and all 23 base-actor dependencies,
including randomized PRNG seeds, mapped inactive slots, live class aliases,
graphics mode 0, follower boot gates and strict Main versus task return contracts.
The game group passes 147,719 comparisons for source/spawn records, all four
collision effects with score/time wrapping, a full pool's mapped slot-10 alias,
constructor-byte versus velocity-word indexing, pair-board selection/removal,
phase/orbit updates, score/record text and strict endpoint collision. Square-root
checks cover all 65,536 unsigned halfwords plus 32-bit boundaries against a
result-only BIOS model; native public tests independently check floor inequalities.
These do not prove BIOS timing or volatile scratch-register behavior.

The chain group passes 52,339 checks for all 28 source functions, including typed Main/F4/animation
contracts, high argument words, all kind bytes, capture modes, live linked-entry
rereads, index 255, self and out-of-chain neighbors, sound priority, all 512
combinations of grid flags and continuous spawning/background updates. Missing-kind
lookups check retained incoming R4. Direct flag-initializer probes explicitly pass
the original old-flags stack address for invalid flag bytes. Normal gameplay task
dispatch retains the source invariant of flag values 0..9; invalid values whose
legacy frame is unavailable fail explicitly. Corrupt-state rejection loops and
recursive graphs are bounded, as described in architecture.md.
Deliberately inconsistent neighbor graphs that hit the native depth bound are
checked separately: tracing the original demonstrates its stack descending into
the chain records. Their native refusal is a corrupt-state guard, not an original
state-parity claim.

All three groups compare EWRAM, IWRAM below the oracle stack, palette, VRAM, OAM
and SRAM. Synthetic public regressions separately check lookup fallback, byte
selectors, preserved unknown record bytes, speed reversal predicates, all-eight
versus single completed grid lines, unchanged-line signaling, tile byte wrapping
and rejected calling contracts. Function-family checks and the existing long
opening scenario do not establish complete bank-3 activity progression, rendering
timing, save effects or a desktop playthrough of those activities.

The six complete SaveGroup3 families can be compared separately:

    python -B tools/compare_reference.py --group3-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

The helper portion passes 84,181 comparisons. The 43 Main callbacks and their
reviewed F4 contracts pass 55,773 comparisons, including typed returns and Main
argument cursors, all byte mask cursors, skipped completion fields, overlapping
record/catalog writes, all eight record styles and empty backgrounds. Both portions
also run in the full comparison suite. --group3-scenes-only selects the latter.

Direct helper checks cover all byte categories and arbitrary retained Z, aliased
coordinate outputs, byte constructor versus full-word animation indices, palette
prefix wrapping, forward overlapping tile copies, stable insertion and ten-row
catalog scans whose output can overlap their input. The first empty-category root
Z is canonicalized to zero, as documented in architecture.md; corresponding root
fixtures establish that undefined input explicitly. Temporary list text uses a
verified pointer relocation: its text and all other covered state still compare.
Neither adaptation establishes exact original stack-history behavior.

Private native input checks now enter and leave the photo room without a fault.
The greeting reaches a separate actor-sequence dependency. Complete photo-list
navigation, the training-seat quest and activity/save integration remain pending.
A separate Windows process restores the native photo-room End Game save and
opens Status with the existing 21 words, ten seeds and one rescued love. The
reference profile remains read-only; the desktop test uses a private copied profile.
Reading all 81 list entries in five languages confirms a maximum text length of
18 bytes, within the original 20-byte buffer.
