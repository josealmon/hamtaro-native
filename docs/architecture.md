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

The frontend uses C++20, SDL3 and Dear ImGui. Its menus support keyboard and
gamepad navigation. Audio is delivered through an SDL stereo PCM stream.
SDL's device conversion happens after game PCM generation.

## Current coverage

| Area | Status | Evidence or remaining work |
| --- | --- | --- |
| Windows x64 shell | Implemented | Compiled and smoke-tested; real-window review |
| ROM import | Implemented | Identity and full hash, invalid imports preserve existing data |
| Settings and bindings | Implemented | Persistence, bounds, defaults, Unicode paths |
| Keyboard/gamepad | Implemented | Synthetic keyboard, virtual-pad input, deadzone, remapping, disconnect; physical models not tested |
| SRAM file management | Implemented | 32 KiB import/export, atomic writes, previous-copy recovery, exclusive profile lock |
| GBA memory and DMA | Foundation | Region/mirroring, read-only IO masks and repeated DMA with working addresses separate from programmed registers; not full hardware conformance |
| BIOS helpers | Foundation | CpuSet, CpuFastSet, bounded LZ77, division semantics and modeled ObjAffine; hardware matrix rounding remains unverified |
| Software video | Foundation | 64 synthetic static frames compared with an independent PPU; seven private ROM snapshots compared; raster timing still pending |
| Video timing and IRQ | Foundation | Scanline/VBlank/HBlank events and DMA, tested independently; CPU/timer/audio timing still pending |
| Native game helpers | Foundation | Button transitions/repeat and integer PRNG |
| Native heap | Adapted | Original 12-byte nodes and 32-bit links; private ROM function comparisons |
| Video state and transfer queues | Adapted | Original video shadows, mosaic/blend/fade updates, DMA queue wrap/fill/flush; private function comparisons |
| Map and core initialization | Adapted | Runtime tables read from ROM, task sentinel, render slots/order, background state and all 11 map banks tested at index 0 |
| Objects and animation scripts | Partial | Physics, positions, priority ordering, affine reuse, OAM and animation/sound commands compared; animation callback bindings remain incomplete |
| Background scripts and transforms | Adapted | All 61 command handlers, repeat/call flow, motion, clamping, affine layers and tilemap copies compared; invalid stacks/resources fail explicitly |
| Graphics scripts and palette effects | Partial | 37 of 48 handlers compared: palette interpolation, delay/call/repeat flow, blend/fade, mosaic and windows; 11 raster-effect handlers remain unsupported |
| Word expressions | Adapted | Arithmetic, comparisons, short-circuit cursor effects and variable/flag updates; bounded recursion and private randomized comparisons |
| Native save records | Adapted | Original payloads/checksum, SRAM identity initialization/repair, primary/backup group loading and grouped writes; private function comparisons |
| Main script control/resources | Partial | Fetch/yield/wait, branches, memory/decompression, maps, saves, fades and typed callbacks; unsupported opcodes fail explicitly |
| Native boot/title | Pending | 88 Main-only, 180 idle and 369 input-injected script/render/audio iterations compared; scene-menu callbacks, task/actor updates, real IRQ cadence and hardware audio output remain incomplete |
| New game, movement, in-game save/load | Pending | Requires the connected native game loop |
| Music and effects | Partial | Sequencer, original PCM mixer, PSG register service and script sound controls compared; timer/FIFO cadence, PSG synthesis, final output mixing and host delivery remain pending |
| Complete single-player playthrough | Pending | Story, secondary activities and minigames |
| Cable Link | Out of first release | No local/network link emulation |
| Public release | Pending | Native gameplay, complete coverage and source-provenance review |

The current application intentionally offers preparation/settings/save management
rather than a Play button backed by an incomplete or emulated game. Loading a ROM
or rendering synthetic pixels is not evidence of a native title screen.

## Storage and lifetime

The user profile's .hamtaro-native directory contains game.gba, settings.ini,
game.sav and at most one game.sav.bak. Updates replace the executable, not this
directory. --data-dir selects an independent profile for testing. The import
source is never modified. A running profile is exclusively locked.

ROM identity comes from config/rom.json, compiled into the application at build
time. Settings use a small versioned INI file; malformed individual properties
fall back to defaults, while a future schema version is reported to the user.
The app does not overwrite an unreadable/newer settings file automatically.

All file dialogs are asynchronous and hand selected paths back to the main
thread. The dialog callback owns its result state even if the application exits.
Save tests use private temporary directories, never the player's profile.

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
also run 88 Main-only iterations, 180 idle iterations and a separate 369-iteration
schedule with injected A-button registers. Those schedules call Main, background,
object, graphics and mosaic updates, OAM transfer, video commit, DMA flush and
audio service. They initialize the audio driver and synchronize its VBlank counter.
Graphics slots are inactive in both ROM schedules; their implementation evidence
comes from separate command comparisons and synthetic update-loop tests. Seven
idle-schedule static PPU snapshots match a separate mGBA renderer; five contain
multiple colors. These schedules omit task/actor systems, CPU-transfer flushing,
the complete IRQ/input cadence and hardware audio output. Transient DMA3 register
values that refer to the reference's stack are excluded; destination memory and
sound DMA1/2 register images are compared. These are not a verified native boot,
title screen or playthrough.
The source checkout's docs/testing.md records the command and current check count.

The native Main interpreter keeps cursor, scratch, variables and return stack
in their original 32-bit memory views. Callback bindings accept a specific typed
Main signature, never a cast of a ROM address to a host function pointer. Unknown
commands/callbacks and invalid stacks return a fault with the guest address;
a per-frame command budget also reports an explicit fault instead of hanging.

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
completion rules. Raster-effect commands remain explicit faults. Main save-load
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
in the CPU oracle. Real timer/FIFO cadence, PSG waveform synthesis, final stereo
mixing and delivery to the host stream remain pending. These checks do not replace
pre-device output validation or listening evidence.

The optional mGBA 0.10.5 software PPU oracle is a separate test build; its code and
runtime never enter the application or package. Static synthetic comparisons
cover text, affine, bitmap and object layers, windows, mosaic and color effects.
The oracle uses RGB555 output and renders each color channel through red before
reassembling RGBA, avoiding mGBA's channel-dependent darkening precision. Fixtures
isolate mosaic and windows because that reference version has carry-pixel artifacts
when clipping mosaic blocks. Analytic regressions cover the combined global
mosaic/window phase and mode-5 out-of-bounds backdrop selection. Raster changes,
timing and general gameplay image parity remain unverified.

The ObjAffine helper follows a single-precision HLE model, independently implemented
from the documented scale/angle contract. Comparison with a CPU oracle using the
same BIOS model does not establish the real BIOS's exact rounding. See the
[mGBA BIOS implementation](https://github.com/mgba-emu/mgba/blob/0.10.5/src/gba/bios.c)
and [GBA video documentation](https://mgba-emu.github.io/gbatek/) for those external
contracts; hardware-level conformance needs separate evidence.

SRAM adapters retain the original guest callback images by reading them from the
ROM into guest RAM as data. Save operations call typed native functions; the copied
ARM bytes are never executed by the Windows application. Save compatibility checks
cover blank storage, identity repair, checksum failures, primary/backup selection
and grouped writes. File persistence remains separate from these in-memory calls;
in-game save timing still needs the connected game loop. Invalid group selectors
are rejected; the unused group-0 record write/load path is not exposed.

## Remaining integration order

1. Add raster effects, task/actor systems and audio, then connect the verified
   initialization, typed callbacks and script/render systems to the real game loop.
2. Establish a deterministic title/new-game/movement/save-load scenario.
3. Compare the native core against the reference's existing opening and first
   reconciliation schedules, then expand to the remaining story and minigames.
4. Compare canonical RGBA, state, SRAM and pre-device PCM. Keep original
   frame cadence, original resolution and gameplay; host display refresh is independent.
5. Verify the packaged app on a Windows machine without development tools.
   Public CI remains ROM-free; the private reference suite takes a user path.
