# Validation

Build and run CTest as described in the README. Tests stay active in Release
builds and use explicit checks rather than assertions removed by NDEBUG.

- probe: RAM snapshots are byte-for-byte equivalent to the original bus reader,
  including full regions, last bytes, mirrors, IO/ROM fallback, address wrapping,
  partial faults and pre-existing faults. The fast path is test-only.
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
  Motion checks cover route edges/obstacle order, collision setup and resource branches,
  aligned forward word copies, overlapping scene parameters, SRAM replication and
  a reserved-bus copy without weakening ordinary invalid-read failures.
  Following checks cover walk/run speeds, boot-gated position updates, near-leader
  stopping, delayed idle animation, asymmetric quantization and circular route bends.
  Collision checks cover signed map wrapping, edge masks, event staging, inclusive
  containment/exclusive interactions, circle/rectangle responses, inactive records,
  action gating and movement-wait completion/pending-result handling. Synthetic
  floating-bus record indices terminate the scan without hiding later memory faults.
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

Use CTest for each functional batch and the existing focused `--...-only` reference
checks during implementation. At a coherent batch boundary, run the complete
reference comparison with all available independent oracles. Keep functional sources
unchanged until that run ends. No exhaustive domains, memory regions or comparisons
are removed to shorten the run.

The optimized complete suite passed the same 4,200,342 assertions, including the
full 16-bit domains, 30 independent PPU snapshots and the existing floating-bus
oracle, in 678.102 seconds on the development machine. The preceding log spanned
approximately 1,238 seconds; that older value comes from file timestamps. Concurrent
native route replays ran during both executions, so the roughly 45% reduction is
an observed local comparison, not a portable performance guarantee. Those measurements preceded the scene-copy and paired-actor additions.
The expanded suite now passes 4,208,502 checks in 613.221 seconds, with the same
exhaustive domains and oracles plus scene-copy, paired-actor and render-selection
contracts; the Sandy Bay native and host replays ran concurrently.

`compare_reference.py` prints elapsed time and assertion counts per family and for
its complete invocation. Optional `--timings-json PATH_OUTSIDE_CHECKOUT` retains a
private machine-readable report, including failed phases. The total also includes
setup and coupled checks outside named families. Reports and logs are never versioned.
Plain RAM snapshots use bounded bulk copies; device IO, mirrors, wrapping and partial
faults keep the original bus path. Python reuses snapshot buffers by size. The content
audit reads historical objects with one Git batch process and checks the same blobs.

The bus oracle's `--scene-copy` mode supplies independent mGBA LDMIA responses:
28 complete scene copies (all four RAM alignments and six reserved-address groups)
and 24 Main20 variable-resolved copies. The native side reads the player's ROM at
the typed prefetch site. Reference-only fixtures supply the original reserved-bus
values. Unicorn's generic Thumb LDM lacks ARM7 alignment, so mapped pointer arguments
are aligned only for that CPU reference call; all four hardware alignments are also
checked against mGBA. No existing cases or comparison regions are removed.

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

The main `tests/scenarios/sunny_peak.json` route starts from erased SRAM and checks
281,135 updates through the first Sunny Peak story, Lovedove, three rescued loves,
Boss's room announcement and original save/Continue back in Clubhouse. It retains
explicit story, position and counter checkpoints. `opening.json` remains the earlier
first-reconciliation regression. Scenarios contain only input durations and scalar
metadata; the player still supplies the ROM. The 281,135-update SDK replay passes
in 582.42 seconds; the same input stream passes 293,049 host-session checks in
568.441 seconds, including exact SRAM-file persistence and fresh continuation.

`sunny_peak_reunion.json` is a 21,978-update focused continuation. Supply original
SRAM after Libert-T, with map 2:20 about to show the reunion (24 words, two loves,
Bijou present). It checks Lovedove, the Clubhouse return and original save/Continue.
`sunny_peak_rubrub.json` instead starts from original cart-area SRAM (map 2:12,
23 words, two loves) and retains the optional ride, Rubrub and save/reload coverage:
11,469 SDK updates and 13,692 host checks. Different prerequisites fail the initial
checkpoint; no input save or resource data is shipped.

`sandy_bay.json` starts from the final private SRAM produced by `sunny_peak.json`,
before talking to Boss about the next mission (Clubhouse map 1:1, 25 words, two
seeds, three loves, Bijou present). It checks the normal destination unlock,
Sandy Bay arrival and beach conversation/save, then the swimming lesson and word 26.
The Lookie conversation changes the entrance scene and makes the Tinglie lesson
available. Tinglie raises the word count to 27 and obtains the ball at the cart.
The ball is delivered to the team, followed by a perfect six-rally practice and
twelve-rally challenge. Checkpoints require six/twelve completed rallies, zero
errors, team phase 3 and the twenty-seed reward. The route passes 92,220 SDK
updates in 136.24 seconds and 96,963 host-session checks in 140.50 seconds,
including SRAM-file persistence and fresh continuation. It ends with original
save/Continue in map 4:2 at x=32.5, z=64.5, 27 words, 22 seeds, three loves and Bijou.
The source SRAM stays unchanged. This does not establish the complete Sandy Bay story.
The prerequisite is reproducible from erased SRAM without duplicating the opening:

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/sunny_peak.json --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_SUNNY_PROFILE
    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/sandy_bay.json --save PATH_TO_NEW_PRIVATE_SUNNY_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe

The first command requires a new profile directory outside the checkout. The
second reads its SRAM without changing it and tests a separate temporary session.

Checkpoints can include `"flags": {"0x77": true, "0x198": false}` and
`"controls": {"0x83": 3, "0x02": 0, "0x1A6": 12}`. IDs are decimal or hexadecimal
strings, unique after parsing. Flags address the 96-byte game flag block (0 through
767) and require boolean values; controls address the 512-byte control block (0
through 511) and require integer bytes (0 through 255). Both mappings are nonempty
read-only assertions. Malformed/out-of-range IDs, numeric aliases and incorrect
value types fail validation; booleans are rejected as control bytes. Deliberately
incorrect flag/control expectations are also rejected by ROM-backed tests.
Original Continue checks both word flags, ball acquisition/delivery and retained
team progress. A separate SDK process reloads the host-written SRAM and verifies
the challenge flag, team phase, map, position and counters in 1,922 updates (2.28 seconds).

`--until "CHECKPOINT LABEL"` stops after exactly one named top-level checkpoint,
while omitting it retains the entire scenario. An unknown or ambiguous label fails.
The 7,310-update Bedroom control prefix is independently checked. Use a checkpoint
after an original in-game save when combining this option with `--session-test`,
because that test compares the persisted state after restarting. This can produce
a private reusable profile at the cart or Clubhouse milestone without repeating
all later steps during focused development.

For a focused continuation, `--save PATH_TO_PRIVATE_SRAM --session-test
build/Release/hn_session_tests.exe` starts both replays from the same private SRAM
copy. The host uses a disposable profile, verifies persistence and starts a fresh
session to compare map, position, words, seeds, partner and love count. The source
save is checked for unchanged bytes. Omitting `--save` retains erased-SRAM coverage.
Neither focused replay substitutes for the combined route from erased SRAM.

An input-only native runtime scenario uses the optional test DLL:

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/opening.json

The opening regression covers the basic lessons and reward, wrong-word retry,
book/status, boat/battery travel, Bijou, Ham-lift, Pepper/Oxnard and Harmony, followed
by Clubhouse/photo-room continuation and empty Items/Gems panels. It runs 144,204
updates and retains repeated original save/Continue checks. The runner rejects
unknown inputs and stops on any runtime fault or checkpoint mismatch. By default
it writes no save or capture. These are native runtime checks, not proof of a full
desktop playthrough or independent full-machine parity.

To replay the same inputs through the application's host Session API and test
actual SRAM-file persistence, add:

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/opening.json --session-test build/Release/hn_session_tests.exe

The host replay performs the input updates in a disposable private profile, verifies
that the saved 32 KiB file equals the game's SRAM, stops and starts a fresh Session,
then selects Continue. Photo-room map/position, twenty-one words, ten seeds, one
rescued love, the joined-partner flag and follower slot 13 are unchanged; 151,630
host-session checks pass. The partner checkpoints describe the
returned overworld state; UI scenes can reuse the same render-class slots.
The original ROM and player's profile are untouched; temporary input traces and
SRAM files are removed. Trace scratch uses HAMTARO_NATIVE_WORK or the private
.hamtaro-native-dev profile outside the checkout. It starts from erased SRAM unless --save supplies a read-only
32 KiB starting image for both replays. It does not create an SDL gameplay window
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

    python -B tools/compare_reference.py --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE --video-oracle PATH_TO_HN_VIDEO_ORACLE --bus-oracle PATH_TO_HN_BUS_ORACLE

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
GBA machine. The complete run with BIOS, PPU and floating-bus oracles passes 4,200,342 checks,
including 401 distinct leader positions during directional input.
It does not exercise the game as a whole and is not part of ROM-free public CI.
The schedule ends with MainScriptWaitForByte1FA8 (0x21) active after exercising
directional movement. Word-choice handlers use opcodes 0x24 (await) and 0x45
(configure). The desktop loop is now connected; complete playthrough remains pending.

The complete counter-display, candidate-selection and crafting-transaction group
can also run separately:

    python -B tools/compare_reference.py --record-selection-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

All 23 functions across game_counter_display.c, game_record_selection.c and
game_record_transactions.c have explicit entry/helper coverage. The 42,501 focused
comparisons include 18 Main contracts, 14 argument-free F4 contracts, five helpers,
actual Main/F4 command calls, all three candidate graphics rows and five languages,
raw selectors, cache/PRNG behavior and counter requirement boundaries. Synthetic
public checks retain zero-key charging, separate repeated-key requirements,
cost/refund wrapping, full-list retention and cached selections without PRNG draws.
Private alias checks cover captured clear pointers, palette writes over selection,
velocity writes that change live selectors, overlapping glyph copies and a counter
list overlapping its own selection/count controls. The original source's local
array capacities and non-returning impossible draws have explicit host guards;
those malformed states are not treated as matching original stack behavior.
Complete workshop/activity progression remains a separate integration requirement.

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

    cmake --build "$env:HAMTARO_NATIVE_WORK/mgba-oracle" --target hn-bios-oracle hn-bus-oracle
    python -B tools/compare_bios.py --oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-bios-oracle.exe"
    python -B tools/compare_reference.py --collisions-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-bios-oracle.exe" --bus-oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-bus-oracle.exe"

Pass --oracle-runtime-dir PATH_TO_COMPILER_RUNTIME to either tool if needed.
The ROM-free math suite compares all 32,769 ArcTan inputs in -16384..16384 and
4,240 signed-half ArcTan2 vectors: 37,009 comparisons. The separate oracle compiles
mGBA 0.10.5's arithmetic helpers directly; no ROM, BIOS image or CPU execution is used.
The native adaptation is isolated in src/gba/bios_math.c with its MPL 2.0 notice.
Hardware BIOS arithmetic and instruction latency remain unverified.

The collision subset with both oracles passes 35,855 private comparisons, including 216 independent
angular responses. It covers mask classification, sample pointers, shape containment,
interaction queries, solid-record responses, record lists, map flags/scripts,
action/reset controls, leader movement, combined updates and Main's movement wait.
Positive-response assertions require both circle and rectangle contacts. The full
comparison requires --bios-oracle; other focused suites can run without it.

The isolated bus oracle executes mGBA's halfword-load implementation without a
ROM, BIOS or CPU instruction loop. It checks the three reviewed ROM prefetch sites
used by collision-record scans, with even/odd addresses and every byte-sized
render count. All 6,144 list scans retain RAM and terminate at the same bus-derived
index, including a preceding mapped inactive record. A separate private input-only
replay reaches map 2:12 after the oilcan sequence; the original mGBA game also
continues from the same SRAM and retains the same animation-derived list. The
adapter covers those reads above 0x0FFFFFFF; other floating-bus sites and detailed
bus/DMA timing remain unverified. Without --bus-oracle these additional checks are
explicitly skipped.

The extended private erased-SRAM scenario passes 237,173 native updates through
Hamigos, the second reconciliation, oilcan purchase/use, lever interaction and
the previously failing return to map 2:12. Original End Game/Continue restores
x=32.5, z=128.5, 23 words, two seeds, Bijou and two rescued loves. Its separate
host-file replay passes 247,641 checks and restores those fields in a fresh session.
The canonical 144,204-update opening, including empty Items/Gems roundtrips and
eight saves, also passes with this adapter. These are bounded compiled-runtime
and save tests; the complete cart activity and desktop story remain unverified.

A separate private input-only scenario starts from the read-only cart-area save,
uses Putput and Gogo, learns Rubrub and completes another original End
Game/Continue. Its 11,469 updates retain map 2:23 at x=72.5, z=96.5 with 24 words,
two seeds, two rescued loves and Bijou. Another SDK process passes 1,922 startup
and Continue updates from the exported 32 KiB SRAM and restores the same fields.
This follow-up does not start from erased SRAM and has no host-session or actual
desktop playthrough claim. Its private traces and saves remain outside the checkout.

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

These pass 41,470 scene actor comparisons, 95,870 actor preset comparisons,
30,930 record comparisons and 16,497 scanline-wave comparisons. The scene group
covers 45 functions across 16 families and one shared bank-3 condition, including
approach completion, signed limits, all ARM shift counts, direction constructors,
snapshots, copy aliases and the three-byte render-selection Main callback. Its
primary/secondary paths cover every byte selection, inactive mapped records and
arguments overwritten by slot reset. The preset group covers all 498 functions in six
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

The complete 483-function bank-4 source family and shared indicator dependency
have a combined private check:

    python -B tools/compare_reference.py --bank4-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

--bank4-presets-only selects the 428 presentation presets; --bank4-dynamics-only
selects jump/counter/audio/path behavior and the 16 typed helpers. The combined
check also runs in the full suite. Its census requires every original source
entry, with Main, F4 and animation contracts checked separately and unsupported
calling forms rejected. Comparisons include EWRAM, IWRAM below the oracle stack,
palette, VRAM, OAM and SRAM.

Cases cover sparse/inactive mapped actors, live class selectors, all byte counter
and timer values, double-precision jump acceleration, counter slot wrapping,
background reset/step destinations aliased to saved and live coordinates,
wrapped distance-volume products and the new wandering predicate. Path helpers
cover all 65,536 speed values, byte cursor wrapping, reversed sentinel scans,
overlapping output bounds, invalid-direction output retention, arbitrary mapped
actor pointers and live animation/overlay tables. Three path followers run every
supported operation, wait/flag variants and continuous sequences through their normal
typed animation adapter. Explicit direct probes also vary incoming SL for the
early-wait decision. The normal adapter's reset-baseline assumption and corrupt
state guards are documented in architecture.md; these checks do not establish
arbitrary original register/stack history.

ROM-free public regressions separately check stage clamping, timer wrapping,
packed writes with aliased destinations, projected-bound side effects, final
counter reset, byte-wrapped actor clearing, accumulator carry, bounds aliases and
the early-wait SL distinction. Full bank-4 story/activity and timed desktop
integration remain pending.

The complete 207-function bank-5 family and its stairs/idle dependencies use:

    python -B tools/compare_reference.py --bank5-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

--bank5-presets-only selects all 172 presentation presets and shared VM contracts;
--bank5-dynamics-only
selects grid/mirror logic, motion, mechanism lanes/outcomes and timing. The
checker inventories every source function and its Main/task/animation/VBlank or
indexed-helper contract. It also covers all four functions in
script_callbacks_stairs_residual.c and rejects unreviewed calling forms.
The combined bank-5 group adds 1,187,266 comparisons to the full suite.

Focused fixtures exercise all phase/count bytes, primary and partner lane states,
signed depth/spawn-count boundaries, byte outcome sums, every mirror source-image
index and coordinate edges, overlapping mirror actors, raw grid cell/kind bytes,
mixed occupied cells and all 65,536 depth/time halfword values. Opcode 0x4E checks
include variable references, the null sentinel and aliased forward copies. Public
synthetic tests additionally deliver the registered mechanism through the actual
native clock's VBlank handler and reject an unreviewed IRQ target. These checks
do not establish a full bank-5 story/activity or desktop playthrough.

The complete bank-6 main/final families and neighbor-pair dependency use:

    python -B tools/compare_reference.py --bank6-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

--bank6-presets-only selects all 200 presentation presets, the shared dependency
and the retained-R5 Main calling contract; --bank6-dynamics-only selects the other
motion/cursor/target functions and helpers. The separate 12-function chain source
family is not included. Census checks account for every function in both covered
source families and reject inappropriate calling forms.

Fixtures cover wrapped orbit phases and center aliases, screen-edge cursor limits,
strict rectangle hits, signed motion thresholds, every language's task patterns,
missing target sets/ranges, aliased coordinate snapshots, affine exhaustion and
wrapped scale curves. Direct helpers cover raw selectors and modular planar
transforms. Target restoration compares matched selectors, explicit retained R5
descriptors and the real MainScriptInvokeCallback scratch-address convention.
Public synthetic tests include duplicate identifier removal, phase wrap/clamps,
raw affine failure writes, signed coordinates, strict rectangle edges and zero
movement duration. Full bank-6 story and desktop coverage remain pending.

The complete 12-function bank-6 chain family has a separate focused command:

    python -B tools/compare_reference.py --bank6-chain-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

Its 34,535 comparisons cover every source entry, five Main contracts, two reviewed
F4 targets and seven animation contracts, including actual script command calls
and rejection through inappropriate interfaces. Fixtures vary the phase's low byte
and high halfword, allocation sentinel/exhaustion, duplicate/raw slot selectors,
captured positions, arguments overwritten by affine allocation, live selectors,
signed rectangle edges, landing overflow, flight flags/counters and Y/Z wrap sums.
The original second-chain null call performs two unused BIOS byte reads. Only for
these comparisons, the CPU oracle maps varying synthetic bytes and removes that
mapping afterward; they are discarded by the native adapter. No BIOS contents are
embedded, and BIOS bus history/timing is not claimed. Public synthetic checks also
cover repeated allocation of matrix zero, phase-65 Y retention, preserved yield,
strict edges, signed division and invalid calling forms. These checks run in the
full suite; complete scene/story and desktop integration remain pending.

Nine complete shared presentation families can be checked together:

    python -B tools/compare_reference.py --presentation-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

The 39,118 comparisons cover all 56 source entries, including two existing window
modal callbacks, 28 Main contracts, 40 reviewed F4 targets, six animation contracts
and six direct helpers. The counts overlap. Checks exercise the actual Main, F4
and animation commands and reject unreviewed interfaces. Fixtures cover height
overshoot, unsigned rectangles, mixed signed region bounds, full halfword angles,
signed affine reciprocals, raw/wrapped render selectors, word-sized velocity indices,
timer/random/offset ordering, byte counters and stage/delay underflow. Whole-memory
comparisons retain effect-pointer reloads, constructor/control aliases, tilemap queue
wrap including zero-width rows, split scrolling pages and task allocations. Only
DMA3's transient source register is excluded. Public synthetic tests independently
cover these boundaries and the undefined-style/missing-task failure guards.
These families run in the full comparison; complete scene/activity and timed PPU
integration remain pending.

The complete bank-7 tile-board and scene-preset families have a focused command:

    python -B tools/compare_reference.py --bank7-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bios-oracle PATH_TO_HN_BIOS_ORACLE

Its 121,552 comparisons account for all 32 original functions, five Main contracts,
30 reviewed F4 targets and one direct helper, including actual Main/F4 command
calls and rejection of inappropriate interfaces. Cases cover every byte cursor
with all four directions and both empty/occupied targets, raw word inputs and
all 256 helper direction selectors with signed mapped offsets. Whole-memory
comparisons include constructor changes to a later board cell, output/argument
overlap, cursor changes during motion, offset-table writes during the setter,
captured arrow coordinates and class changes during graphics or neighbor setup.
Public synthetic tests additionally check solved/unsolved results, the four
special cursor links, blocked movement, preserved unused arrows, boot-gate
behavior and background setup. These families run in the full comparison;
complete puzzle and area/story integration remain pending.

The seven complete SaveGroup3 families, the 18-function record-selection family,
shared item-selection families and two strided-transfer helpers can be compared separately:

    python -B tools/compare_reference.py --group3-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

The helper portion passes 85,078 comparisons, including allocated strided copies.
The 51 Main callbacks and their reviewed F4 contracts pass 80,705 comparisons, including typed returns and Main
argument cursors, all byte mask and grid cursors, all four navigation directions,
flag-253 variants, skipped completion fields, overlapping record/catalog writes,
all eight record styles and empty backgrounds. Record selection passes another
23,974 comparisons, including 30-entry scrolling and wrapped window thresholds.
All portions also run in the full comparison suite. --group3-scenes-only selects
the SaveGroup3 scene checks; --record-menus-only selects the record menu.
--selection-scenes-only selects the complete 14-function shared item-selection
family and three one-function key-save-mark, key-save-sequence and raster-E5
panel dependencies. These 112,737 comparisons include all items and five
languages at defined text pages, full-byte page wrapping, row-start underflow,
signed scroll thresholds, all cursor bytes and all 1,024 held-key combinations
with debug flag 2 both enabled and disabled. The fixture preserves sparse-list
ordinal reads and compares all covered memory plus display/window IO. Main cursor
and reviewed F4 contracts are checked independently. Synthetic public tests also
exercise text allocation/removal, halfword offsets, empty unconfigured tasks,
key priority, retained list boundaries and linked panel behavior.

Direct helper checks cover all byte categories and arbitrary retained Z, aliased
coordinate outputs, byte constructor versus full-word animation indices, palette
prefix wrapping, forward overlapping tile copies, stable insertion and ten-row
catalog scans whose output can overlap their input. The first empty-category root
Z is canonicalized to zero, as documented in architecture.md; corresponding root
fixtures establish that undefined input explicitly. Temporary list text uses a
verified pointer relocation: its text and all other covered state still compare.
Neither adaptation establishes exact original stack-history behavior.

Private native input checks now enter and leave the photo room without a fault.
The greeting completes and returns to its action menu after the actor-sequence
batch. Items and Gems entry, empty-inventory input and return now run natively,
restoring the same photo-room actor position and story counters. Populated
photo-list navigation, the training-seat quest and complete activity integration remain pending.
A separate Windows process restores the native photo-room End Game save and
opens Status with the existing 21 words, ten seeds and one rescued love. The
reference profile remains read-only; the desktop test uses a private copied profile.
Reading all 81 list entries in five languages confirms a maximum text length of
18 bytes, within the original 20-byte buffer.

The 48 actor render-sequence presets have a separate focused check:

    python -B tools/compare_reference.py --actor-sequences-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

Its 9,361 comparisons cover every source entry, typed F4 calls, rejection through
the Main consumed-count ABI, mapped inactive slots, neighbor byte wrapping, follower
boot gates, indicators and constructors that overwrite the selector read by a later
source call. These checks also run in the full suite. No asset data or function
machine-code images are embedded in the native implementation.
A separate Windows process restores the photo-room save near the characters and
displays the greeting's full text, portrait, actor animations and continuation
indicator. The canonical opening scenario now completes the greeting, returns to
its action menu and restores the photo room through a seventh End Game/Continue.
It then opens empty Items and Gems panels, exercises direction/read/cancel input,
restores the photo-room actors and completes an eighth End Game/Continue. The
fresh host-file/session replay retains the same map, position and counters.
The actual Windows executable also opens both empty panels with keyboard input
and returns through Status to the photo room. Populated inventory scrolling and
text paging remain separate gameplay integration checks.
The complete desktop conversation/activity remains a separate pending check.

A separate private route repeats the erased-SRAM opening through the Clubhouse,
travels by boat to Arnie and exercises the training-seat request plus a seventh
End Game/Continue. It passes 147,635 input updates and 155,127 host-session checks,
including a fresh host session from disk in map 2:18 at x=249.5, z=56.5 with the
same 21 words, ten seeds, partner and rescued love. The canonical 144,204-update
scenario and its eight save cycles remain separate.
Further input-only SDK exploration returns to the photo room, uses Lookie with
Seamore and receives the training seat. Its populated Items panel displays one
item's icon and description and accepts navigation/read/cancel. An in-game save,
Continue and a separate SDK process loaded from the exported original 32 KiB SRAM
retain that item. The source profile is read-only; the development checkpoint
has one prior-copy backup. The extended input-only replay then gives the seat to
Arnie, follows him up the vines and learns Sesam-E. All 179,239 updates pass from
erased SRAM, followed by 187,706 host-session checks and a fresh continuation from
private 32 KiB SRAM in map 2:7 at x=72.5, z=80.5 with 22 words, ten seeds, Bijou and
one rescued love. An SDK inspection after Continue also displays empty Items after
the delivery. This remains separate from the canonical opening and its empty
Items/Gems checks. Multiple-item scrolling, populated Gems, the actual desktop
quest route and complete Sunny Peak progression remain pending.
