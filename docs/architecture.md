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
| BIOS helpers | Foundation | CpuSet, CpuFastSet, bounded LZ77 and compiler division semantics |
| Software video | Foundation | Synthetic text/affine/bitmap/OBJ/window/blending paths; no game-scene comparison yet |
| Video timing and IRQ | Foundation | Scanline/VBlank/HBlank events and DMA, tested independently; CPU/timer/audio timing still pending |
| Native game helpers | Foundation | Button transitions/repeat and integer PRNG |
| Native heap | Adapted | Original 12-byte nodes and 32-bit links; private ROM function comparisons |
| Video state and transfer queues | Adapted | Original video shadows, mosaic/blend/fade updates, DMA queue wrap/fill/flush; private function comparisons |
| Map and core initialization | Adapted | Runtime tables read from ROM, task sentinel, render slots/order, background state and all 11 map banks tested at index 0 |
| Word expressions | Adapted | Arithmetic, comparisons, short-circuit cursor effects and variable/flag updates; bounded recursion and private randomized comparisons |
| Native save records | Adapted | Original payloads/checksum, SRAM identity initialization/repair, primary/backup group loading and grouped writes; private function comparisons |
| Main script control/resources | Partial | Fetch/yield/wait, branches, memory/decompression, maps, saves, fades and typed callbacks; unsupported opcodes fail explicitly |
| Native boot/title | Pending | Core initialization and 24 Main-only script iterations compared; background/render/graphics/task updates and audio are not connected |
| New game, movement, in-game save/load | Pending | Requires the connected native game loop |
| Music and effects | Pending | Native sequencer/mixer and PSG; the host stream is ready |
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
Heap layouts, input, PRNG, expressions, video, transfers, maps, core initialization
and save operations are compared. Synthetic fixtures exercise resource commands
without embedding a game resource. The first 24 Main-only iterations read real
scripts from the private ROM and compare persistent RAM, SRAM and video memory.
This does not run the other per-frame systems, so it is not an executed native
boot, title screen or playthrough. The source checkout's docs/testing.md records
the reproducible command and current check count.

The native Main interpreter keeps cursor, scratch, variables and return stack
in their original 32-bit memory views. Callback bindings accept a specific typed
Main signature, never a cast of a ROM address to a host function pointer. Unknown
commands/callbacks and invalid stacks return a fault with the guest address;
a per-frame command budget also reports an explicit fault instead of hanging.

SRAM adapters retain the original guest callback images by reading them from the
ROM into guest RAM as data. Save operations call typed native functions; the copied
ARM bytes are never executed by the Windows application. Save compatibility checks
cover blank storage, identity repair, checksum failures, primary/backup selection
and grouped writes. File persistence remains separate from these in-memory calls;
in-game save timing still needs the connected game loop. Invalid group selectors
are rejected; the unused group-0 record write/load path is not exposed.

## Remaining integration order

1. Connect verified native initialization, typed callback resolution and the main
   script loop, then add the background/graphics/task systems needed for boot.
2. Establish a deterministic title/new-game/movement/save-load scenario.
3. Compare the native core against the reference's existing opening and first
   reconciliation schedules, then expand to the remaining story and minigames.
4. Compare canonical RGBA, state, SRAM and pre-device PCM. Keep original
   frame cadence, original resolution and gameplay; host display refresh is independent.
5. Verify the packaged app on a Windows machine without development tools.
   Public CI remains ROM-free; the private reference suite takes a user path.
