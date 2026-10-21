# Architecture and verified scope

## Target and source boundary

The current read-only reference revision is 14f0aec, "Isolate build tests from
installed toolchains". The original scenario baseline is 2ff1b43, "Validate Spat,
Clubhouse conversations and sunflower reward". Its SHA-256-matching GBA build is
a source of behavioral evidence; it does not make the Windows port complete.

This repository has independent history and contains no resource catalogs from
the reference. The player imports the 16 MiB European AH3P revision 0 ROM.
Read-only resource data stays in that private file. Metadata may contain names,
addresses, lengths and hashes, never an embedded copy of a game resource.

C17 compatibility/game code uses GameAddr (uint32_t) for GBA addresses and
explicit little-endian memory access. Host pointers never enter serialized
32-bit fields. ROM callback addresses must eventually resolve through typed
native dispatch, with explicit handling of unusual ARM calling contracts.
There is no ARM interpreter, JIT or mGBA runtime dependency in this application.
The isolated angular BIOS helpers are native C adapted from mGBA under MPL 2.0;
their source and license are identified in NOTICE.md. No BIOS image is needed.

The frontend uses C++20, SDL3 and Dear ImGui. Its menus support keyboard and
gamepad navigation. The native game loop drives a nearest-filtered 240x160 texture
and 32768 Hz stereo PCM. SDL converts PCM to the output device's format. The game
clock runs at 16777216/280896 Hz independently of the display refresh.

## Current coverage

| Area | Status | Evidence or remaining work |
| --- | --- | --- |
| Windows x64 application | Implemented | Native game view, Play/pause/resume/restart/menu, settings and save management; smoke-tested and real-window review |
| ROM import | Implemented | Identity and full hash, invalid imports preserve existing data |
| Settings and bindings | Implemented | Persistence, bounds, defaults, Unicode paths |
| Keyboard/gamepad | Implemented | Synthetic keyboard, virtual-pad input, brief taps retained for one game update, deadzone, remapping, disconnect; physical models not tested |
| SRAM file management | Implemented | 32 KiB import/export, atomic writes, previous-copy recovery, exclusive profile lock |
| GBA memory and DMA | Foundation | Region/mirroring, read-only IO masks and repeated DMA with working addresses separate from programmed registers; not full hardware conformance |
| BIOS helpers | Foundation | CpuSet, CpuFastSet, bounded LZ77, division, ObjAffine and native angular math; 37,009 angular cases compared with mGBA, hardware conformance remains unverified |
| Software video | Foundation | 64 synthetic static frames compared with an independent PPU; 30 private ROM snapshots compared, including directional movement; raster timing still pending |
| Video timing and IRQ | Foundation | Scanline/VBlank/HBlank, timers, IRQ flags, FIFO DMA and native VBlank service connected; 9,000 clocked native updates completed; full-machine cadence and DMA bus contention remain unverified |
| Native game helpers | Adapted | Original guest input layout, directions/repeat/query semantics and integer PRNG/ranges compared; host controls and per-frame PRNG connected |
| Native heap | Adapted | Original 12-byte nodes and 32-bit links; private ROM function comparisons |
| Video state and transfer queues | Adapted | Original video shadows, mosaic/blend/fade updates, DMA queue wrap/fill/flush; private function comparisons |
| Map and core initialization | Adapted | Runtime tables read from ROM, task sentinel, render slots/order, background state and all 11 map banks tested at index 0 |
| Objects and animation scripts | Partial | Physics, positions, priority ordering, affine reuse, OAM and animation/sound commands compared; animation callback bindings remain incomplete |
| Background scripts and transforms | Adapted | All 61 command handlers, repeat/call flow, motion, clamping, affine layers and tilemap copies compared; invalid stacks/resources fail explicitly |
| Graphics scripts and palette effects | Partial | All 48 command handlers compared, including raster allocation, position, windows, radius profiles and sweeps; circular HBlank callback compared, remaining scanline callbacks and timed DMA/PPU integration pending |
| Word expressions | Adapted | Arithmetic, comparisons, short-circuit cursor effects and variable/flag updates; bounded recursion and private randomized comparisons |
| Actor script contexts | Adapted | Sixteen 84-byte contexts, Main state preservation, nested calls/waits, activation/stop and same-frame activation compared; per-actor command budgets fail explicitly |
| Motion and collision | Adapted | Leader input/walk/run, action priority, map edge sampling/sliding, circle/rectangle queries/responses, event staging, circular follower routes, proximity/idle animation and movement wait protocol compared; full-game coverage remains pending |
| Text counters | Adapted | Three ROM template/palette banks, digit conversion, class aliases and typed animation callback compared; resources remain in the private ROM |
| Text tasks and dialog scripts | Adapted | Linked contexts, tiled glyphs, delays/fast advance, line/page scrolling, all 32 text commands, numeric text, icons, language variants and 27 typed animation callbacks compared; other indirect callbacks remain incomplete |
| Native save records | Adapted | Original payloads/checksum, SRAM identity initialization/repair, primary/backup group loading and grouped writes; private function comparisons |
| Word-choice dialogs | Adapted | Nine text contexts, expression-filtered options, grid navigation/highlights, disabled rows, confirmation/cancellation, requested selection, delayed close, display reset and Main result preservation compared; full-game integration remains pending |
| Main script control/resources | Partial | Fetch/yield/wait, branches, memory/decompression, maps, saves, task/actor/indirect setup, word-choice dialogs, collision scenes/routes, random values, core reset, choice flags, fades and typed callbacks; unsupported opcodes fail explicitly |
| Scene menus | Partial | Menu/record selection, name editing, glyph blitting, keyboard banks, panel entry/exit/depth, language snapshots and input dialogs compared; later scene callbacks remain incomplete |
| Native boot/title | Partial | Real SDL window reaches the language selector with music; 9,000 clocked native updates completed; 88 Main-only, 180 idle and 9,000 component-schedule updates compared; complete boot/title/gameplay parity remains unverified |
| New game, movement, in-game save/load | Partial | Native loop connected; function-level movement/save comparisons and atomic host persistence verified; actual story and in-game save/reload playthrough remain pending |
| Music and effects | Partial | Native sequencer/mixer/PSG/FIFO/PWM connected to SDL; 600 real-time updates with no device sample shortages and user-confirmed clear menu music; complete game-output parity, filtering and trigger quirks remain pending |
| Complete single-player playthrough | Pending | Story, secondary activities and minigames |
| Cable Link | Out of first release | No local/network link emulation |
| Public release | Pending | Native gameplay, complete coverage and source-provenance review |

Play runs compiled C game logic with native video/audio devices. A missing callback,
unsupported CPU-transfer queue, communication update or IRQ target stops execution
and opens the menu with a private runtime-error.log diagnostic. It never switches
to guest instruction execution. The language selector and menu music establish a
native desktop boot milestone, not a completed title/story/playthrough.

The runtime follows the source's VBlank service, input, Main, follower motion,
actors, tasks, background/object/graphics scripts, mosaic and PRNG update order.
CPU instruction costs and timed CPU transfers are not modeled. Source-level and
clocked native schedules are separate evidence; neither proves full-machine parity.

Scanline object culling preserves OAM order, transparent priority effects, mosaic
bounds and raster register changes between lines. On the local test machine it
reduced mean native update time from 18.174 ms to 0.953 ms over 1,200 idle updates.
The complete pixel and PCM sequences retained identical SHA-256 digests. The PPU's
64 synthetic frames also continue to match the independent oracle. Audio starts
with 2,048 silent stereo frames (62.5 ms) to absorb device scheduling jitter; pause
preserves queued samples. There is no sample dropping or game-clock speed change.

## Storage and lifetime

The user profile's .hamtaro-native directory contains game.gba, settings.ini,
game.sav, at most one game.sav.bak, and an error diagnostic if needed. Updates replace the executable, not this
directory. --data-dir selects an independent profile for testing. The import
source is never modified. A running profile is exclusively locked.

ROM identity comes from config/rom.json, compiled into the application at build
time. Settings use a small versioned INI file; malformed individual properties
fall back to defaults, while a future schema version is reported to the user.
The app does not overwrite an unreadable/newer settings file automatically.

All file dialogs are asynchronous and hand selected paths back to the main
thread. The dialog callback owns its result state even if the application exits.
Save tests use private temporary directories, never the player's profile.
The host captures SRAM only after successful initialization or a complete native
update. A failed update cannot overwrite the last committed snapshot. File errors
pause the game and retain the completed pending snapshot for retry before another
update. ROM/save replacement is disabled during a running or paused game session.

## Function-level reference checks

The optional tests/integration/reference_probe.c bridge exports the native core
only for private differential tests. It is not installed or shipped. The private
comparison tool reads the player's ROM and the reference function-symbol map,
runs selected ARM/Thumb functions in Unicorn, and compares them with compiled
native operations. It models the BIOS services, immediate DMA and IO masks needed
by these functions; it does not provide a complete machine/timing oracle.
Heap layouts, input, PRNG, expressions, video, transfers, object/background
rendering, maps, core initialization and save operations are compared. Synthetic
fixtures exercise resource commands without embedding a game resource. Tests
also run 88 Main-only iterations, 180 idle iterations and a separate 9,000-iteration
schedule with scripted input through the original guest input adapter. The latter
enters and confirms both name fields, completes a circular menu transition and
advances into the opening scene, including collision setup, counter drawing, text
and directional movement through 401 distinct leader positions.
Those schedules call Main, gated follower motion, actor scripts, text tasks, background,
object, graphics and mosaic updates, OAM transfer, video commit, DMA flush and
audio service. They initialize the audio driver and synchronize its VBlank counter.
Graphics slots are active in 262 iterations of the input schedule; command
comparisons and synthetic updates cover the remaining implemented behavior. Thirty
static PPU snapshots across the schedules match a separate mGBA renderer; twenty-seven
contain multiple colors. The schedule also compares 3,581 HBlank callbacks using
explicit scanline samples, without deferred DMA0/PPU timing or instruction interleaving.
The complete function run, including three hardware-initialization cases, passes
494,314 comparisons with the independent BIOS and PPU oracles.
These schedules omit the full-game word-choice path, CPU-transfer
flushing, the complete IRQ cadence and hardware audio output. Transient DMA3 register
values that refer to the reference's stack are excluded; destination memory and
sound DMA1/2 register images are compared. These are not a verified native boot,
title screen or playthrough.
MainScriptWaitForByte1FA8 (0x21) now executes leader/follower control and collision
updates. Word-choice awaiting/configuration use opcodes 0x24/0x45. The current
schedule ends while the movement wait is active; it is not a complete game path.
See docs/testing.md in this repository for the commands and current check count.

Scene callbacks use typed native dispatch with explicit resource failures. They
read menu banks, option codes, background scripts and animations from the private
ROM. The reusable render-slot adapter preserves halfword coordinate truncation,
priority flags and the class-slot boot gate. Menu movement retains original
boundaries and sound triggers; guest input preserves direction caching, repeat
delays, held-frame wrapping, key-mask side effects and masked ordinal priority.
Random save-record selection
uses the original guest PRNG/range calculation with a bounded retry guard for
degenerate state. No menu strings, glyphs or background data are embedded.
Name entry reads the three keyboard grids, defaults, glyphs and advance widths
from the private ROM. The glyph adapter converts packed 2bpp data to tiled 4bpp,
preserving odd-pixel merging and clipping. The original name renderer fills 0x680
guest bytes despite requesting a 0x280-byte shadow allocation; this behavior is
preserved through guest bus writes and never overruns a host allocation. Panel
entry/exit/depth callbacks preserve their per-language layouts and animation yields.
Indirect arithmetic callbacks preserve the original scratch aliases and flag-update
order, including byte subtraction's comparison after the write. Counter drawing
reads three templates and palettes from the private ROM, retains class-value aliases
and queues the original palette transfer. Its animation callback updates the low
yield byte while preserving the rest of the word.

The native Main interpreter keeps cursor, scratch, variables and return stack
in their original 32-bit memory views. Callback bindings accept a specific typed
Main signature, never a cast of a ROM address to a host function pointer. Unknown
commands/callbacks and invalid stacks return a fault with the guest address;
a per-frame command budget also reports an explicit fault instead of hanging.

Actor scripts reuse the typed Main interpreter with sixteen separate 84-byte guest
contexts. Context zero saves Main while each active actor runs, then restores its
cursor, return stack, wait/result fields and execution flags. Actors activate in
index order, including a new later actor started during the same frame. Script and
budget faults retain the actor's diagnostic state and restore Main before returning.
The three padding bytes are preserved. Native context setup rejects indexes above 16.

Text tasks preserve their 52-byte guest layout and doubly linked context list.
The updater uses the original key cache, packed glyphs and ROM advance widths,
including odd-pixel merging, fixed-cell centering, tile-row clipping and the
two-stage scroll for four/six-tile-high windows. The E0..FF command family retains
wait/acknowledgement sounds, one-level text returns, numeric padding/truncation,
inline icons, per-language bundle selection and in-place variant retargeting.
Immediate shadow fills preserve DMA3 destination/control effects. Typed void
callback dispatch supports the 27 slot-16 animation presets; other callback words
fail explicitly. Glyphs, strings, icon animations and bundle tables remain in the
user's ROM. Malformed resources, cyclic lists and exhausted command budgets fail
without interpreting guest instructions.

Word-choice dialogs retain nine twelve-byte records and allocate nine independent
text tasks, including unused cells. Conditions advance the original expression
cursor; labels select ROM language bundles or adjusted-initial buffers. Opening
waits for background ownership, then initializes both cursor slots and tile palette
highlights. Input preserves directional priority, vertical disabled-row skipping,
the backward correction from empty cells, blocked cancellation and original songs.
Requested selection can enter an eight-tick delayed close. Main preserves the
selected result through the closing states and resumes only after the background
releases the dialog. Display reset retains its distinct key-cache behavior. Native
guards reject invalid grids and missing task/render resources. Idle-animation
selection reads the original primary/secondary family tables from the user's ROM.

Motion setup preserves the original class selectors, control bytes and optional
128-byte parameter block. Collision data is read from the caller's resource,
copied/decompressed into guest RAM, or retained when already in the default map.
Selected background bounds are updated; unselected bounds are preserved. Route
queries keep unsigned coordinate expansion and exclusive edges, testing tile bits
12..15. The follower route chooses between X-first and Z-first paths using those
queries and the original signed distance/tie rules. The native query rejects a
map extent larger than guest RAM. The follower update preserves full-word direction
selection, signed high-half speed classification, asymmetric negative-step
quantization, circular route bends/consumption, proximity thresholds, fractional
target bias and the three-tick idle animation delay. Gated following runs between
Main and actor scripts and commits X/Z while clearing the consumed velocities.
Leader movement retains key priority, walk/run animation selection, action gating,
per-axis snapping and slide responses. Map sampling distinguishes truncated fixed
division from arithmetic shifts and uses the ROM's directional offsets and masks.
Record collision keeps inclusive containment versus exclusive interaction bounds,
signed-half coordinates, radius tests, angular projection and rectangular response
quadrants. Solid-record and interaction lists retain their separate flags and the
original count-inclusive record selector. Movement wait preserves initialization,
pending-result consumption, script-yield semantics and idle-counter saturation.

Object animations retain original 32-bit resource references and explicit typed
callback dispatch. Missing callback bindings fail visibly. Sound commands require
an initialized native audio driver and retain the original suppression rules. OAM
sorting preserves the original algorithm's distinct priority/key/index exits,
including its order for equal keys. Background scripts retain separate local
and committed cursors, original wait/repeat behavior and shared queued-write
storage. Their native adapters reject overflowing script stacks and invalid
tilemap rectangles rather than letting malformed inputs overwrite adjacent state.

Graphics scripts keep their original 44-byte state and shared four-level
repeat/call resources. Palette transitions use separate eight-byte step entries
and six-byte RGB accumulators; source colors wrap at 256 while destination indexes
remain full width. Interpolation and scratch fields are compared with the original
functions. Invalid palette extents fail instead of taking the original zero-count
wraparound path. Fade, alpha and mosaic modes preserve their distinct delay and
completion rules. Raster state retains guest heap ownership, DMA0 register setup,
window/layer restoration, byte/phase wrapping and the signed radius-update condition.
The typed HBlank dispatcher supports the circular effect and known no-op callbacks;
other callback addresses fail explicitly. Static snapshots and sampled callback
comparisons do not establish raster-image fidelity or the real IRQ cadence. Main save-load
commands preserve the failure cursor/depth behavior and reload the saved map with
a white palette on success; file persistence still needs the real game loop.

Audio uses explicit guest layouts for the driver, six player records, 80-byte
tracks and 64-byte channels. Initialization reads dispatch and song metadata
from the private ROM. Callback words remain guest addresses and are resolved
through typed native calls; relocated ARM mixer bytes are retained as guest
data only. The Windows application never executes that image.

Native audio comparisons cover driver setup, sample-frequency parameters,
player/song controls, fades, volume/pitch/pan, 30 sequence opcodes, memory and
extended commands, PCM/PSG note allocation and the player tick loop. Tests
compare 300 song-control selections and 120 sequencer updates for each of 12
private ROM sequences. Separate tests run the complete original mixer and PSG
register service for 120 calls on each of those 12 sequences. All 1,440 buffer/state
snapshots match; 349 contain nonzero PCM. The PCM comparison is of the original
eight-bit DMA planes, before hardware FIFO/DAC mixing. Synthetic cases cover
interpolation, loop/end boundaries, packed byte addition, reverb and PSG envelopes.
Main and animation sound controls call this native driver through typed adapters.
The frequency setter's VCOUNT wait is represented by an explicit boundary model
in the CPU oracle. The clocked native runtime initializes the driver at its initial
clock boundary; original busy-wait instruction costs remain unmodeled.

The native clock also drives four 16-bit timers, prescalers, overflow reloads,
cascades and IRQ flags. A separate Direct Sound model receives timer 0/1 edges,
maintains each FIFO's seven-word queue plus four-byte shifter, requests four-word
DMA1/2 refills, and applies stereo routing, volume, bias clipping and PWM resolution.
Host output averages the held PWM levels into 32768 Hz signed stereo samples;
neither a ROM nor a CPU interpreter is needed for this hardware model. Native
device links are host-only fields, never guest addresses or serialized state.

An optional, hardware-only mGBA 0.10.5 oracle compares 5,696 synthetic snapshots
(256,640 field checks): timer reload/count/control, cascade/IRQ state, FIFO words,
shifter and sample state, settled DMA sources and quantized DAC levels. It loads
no ROM and executes no CPU instructions. Timer reads use the hardware API without
mGBA's two-cycle CPU bus-read adjustment. The oracle's desktop gain/resampler is
excluded; PWM quantization is applied explicitly to its raw ten-bit levels.
DMA comparisons occur after bursts settle: the native bus still performs each
burst atomically and does not model instruction stalls or arbitration. FIFO
byte/halfword lane writes, host averaging and unusual bias clipping have separate
analytic unit checks. mGBA 0.10.5 retains held PCM while master power is off;
DAC parity excludes that transition, while native tests require the documented
mute. This does not establish full hardware conformance.

Four native PSG generators now share the hardware clock: two eight-step pulse
channels, frequency sweep, length/envelope sequencing, a rotating one/two-bank
wave buffer and the seven/fifteen-bit noise polynomials. Frequency writes reload
after an in-flight period ends. Pulse retriggers retain duty phase and divider
low bits. Register reads expose their hardware masks and active-channel status;
wave RAM accesses select the opposite bank. The mixed PSG levels are centered
before adding FIFO PCM, with the wave channel's opposite polarity. This follows
the documented digital DAC contract rather than adding unipolar signals to the
FIFO range. Final clipping/quantization stays in the shared PWM path.

The hardware harness now covers 10,432 synthetic snapshots, totaling 698,616
field checks including the Direct Sound cases above. PSG checks cover steady
pulse/noise periods and envelopes, wave order/attenuation/bank rotation,
sweep/length/control state and routing/master volume. Native analytic tests
also cover first-step suppression, retriggers, deferred period reload, both
two-bank starting selections, complete polynomial cycles, physical mix polarity
and partition-invariant stereo output with all four generators plus PCM.

These are component comparisons with explicit oracle limits. Noise tests inject
GBATEK's initial polynomial seed into the test oracle; stock mGBA 0.10.5 uses
all ones instead. Silent pulse phases and stopped sweep countdowns are excluded
because that oracle stops advancing them. Sweep tests compare control state,
not the phase of an in-flight divider after a frequency change. Wave comparisons
inspect matching steady edges after the oracle's unexplained 24-cycle trigger
delay, and use bank 0 when comparing two-bank mode. The native bank-1 start is
covered analytically. The oracle routing API receives centered digital samples,
inactive digital zero and inverted wave polarity for the mix component test;
stock mGBA's final PSG DAC waveform is not asserted equal. No source/library
from that oracle is linked into the native hardware model.

Game-loop/host delivery now runs in the native window, with clear menu playback
confirmed by the user. Analog filtering, real trigger/retrigger timing and complete
pre-device game-output comparisons remain pending. The hardware checks and driver
comparisons do not establish correct sound throughout the game. Hardware captures
are still needed to resolve the startup and model-specific uncertainties.

The optional mGBA 0.10.5 software PPU oracle is a separate test build; its code and
runtime never enter the application or package. Static synthetic comparisons
cover text, affine, bitmap and object layers, windows, mosaic and color effects.
The oracle uses RGB555 output and renders each color channel through red before
reassembling RGBA, avoiding mGBA's channel-dependent darkening precision. Fixtures
isolate mosaic and windows because that reference version has carry-pixel artifacts
when clipping mosaic blocks. Analytic regressions cover the combined global
mosaic/window phase and mode-5 out-of-bounds backdrop selection. Raster changes,
timing and general gameplay image parity remain unverified.

Build the optional hardware oracles outside this checkout with the CMake project
in tests/integration/mgba and a caller-provided MGBA_SOURCE checkout. Run
tools/compare_audio.py with --oracle and, if needed, --oracle-runtime-dir. This
ROM-free check uses build/Release/hn_reference_probe.dll by default; --native
selects another test build. No oracle executable or library enters the package.
The sound contracts follow [GBATEK](https://mgba-emu.github.io/gbatek/#gbasoundcontroller)
and the [FIFO hardware measurements](https://github.com/mgba-emu/mgba/issues/1847).
The PSG divider/envelope and GBA digital DAC contracts are described in
[Pan Docs audio details](https://gbdev.io/pandocs/Audio_details.html) and
[audio registers](https://gbdev.io/pandocs/Audio_Registers.html); native GBA wave
RAM banking follows GBATEK, not Game Boy compatibility-mode access rules.

The ObjAffine helper follows a single-precision HLE model, independently implemented
from the documented scale/angle contract. Comparison with a CPU oracle using the
same BIOS model does not establish the real BIOS's exact rounding. See the
[mGBA BIOS implementation](https://github.com/mgba-emu/mgba/blob/0.10.5/src/gba/bios.c)
and [GBA video documentation](https://mgba-emu.github.io/gbatek/) for those external
contracts; hardware-level conformance needs separate evidence.

The bounded LZ77 model ignores the low header nibble used by the collision-stream
selector. Synthetic tests cover these tags, overlapping copies, invalid backreferences
and destination limits. Exact BIOS signature handling remains unverified on hardware.

SRAM adapters retain the original guest callback images by reading them from the
ROM into guest RAM as data. Save operations call typed native functions; the copied
ARM bytes are never executed by the Windows application. Save compatibility checks
cover blank storage, identity repair, checksum failures, primary/backup selection
and grouped writes. File persistence remains separate from these in-memory calls;
in-game save timing still needs the connected game loop. Invalid group selectors
are rejected; the unused group-0 record write/load path is not exposed.

## Remaining integration order

1. Add remaining scanline callbacks and timed effects, CPU-transfer flushing and
   audio delivery, then connect the verified
   initialization, typed callbacks and script/render systems to the real game loop.
2. Establish a deterministic title/new-game/movement/save-load scenario.
3. Compare the native core against the reference's existing opening and first
   reconciliation schedules, then expand to the remaining story and minigames.
4. Compare canonical RGBA, state, SRAM and pre-device PCM. Keep original
   frame cadence, original resolution and gameplay; host display refresh is independent.
5. Verify the packaged app on a Windows machine without development tools.
   Public CI remains ROM-free; the private reference suite takes a user path.
