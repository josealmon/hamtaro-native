# Architecture

## Reference and runtime

The port adapts the European AH3P revision 0 game from the
[decompilation repository](https://github.com/josealmon/hamtaro-heartbreak), using
reference commit [`14f0aec`](https://github.com/josealmon/hamtaro-heartbreak/tree/14f0aec2014cb0f631a0784f9846ba6419a4ecc1).
Earlier scenario work used `2ff1b43`. The reference checkout is an external,
read-only input to development tools.

Game logic and hardware compatibility code compile as C17; the Windows frontend
uses C++20, SDL3 and Dear ImGui. There is no ARM interpreter or JIT in the
application. The mGBA-derived BIOS arithmetic helpers run as native C, with
attribution in [NOTICE.md](../NOTICE.md). No BIOS image is required.

The player supplies the 16 MiB ROM described in [config/rom.json](../config/rom.json).
Graphics, scripts, text, maps, music and tables are read from that validated ROM.
The launcher and session share one immutable ROM buffer; session ownership keeps
core pointers valid until shutdown. The four files in `assets/` are embedded
launcher artwork and the Windows icon.

## Project layout

| Location | Responsibility |
| --- | --- |
| `src/app/` | Launcher, localization, settings, ROM validation, profile storage and game session |
| `src/platform/` | SDL input and desktop audio output |
| `src/gba/` | Guest memory, DMA, BIOS helpers, software video, timers, IRQs and sound hardware |
| `src/game/` | Adapted game logic, script interpreters, typed callbacks, areas, menus and saves |
| `config/` | Supported ROM identity |
| `cmake/` | Pinned dependencies and generated Windows resources |
| `assets/` | Logo, cover, animated mascot and icon |
| `tests/unit/` | Synthetic subsystem tests |
| `tests/integration/` | Host integration tests, native probe and optional reference oracles |
| `tests/scenarios/` | Button schedules and read-only gameplay checkpoints |
| `tools/` | Reference comparisons, inventories, scenario runner and repository checks |
| `docs/` | Architecture and test instructions |

Build outputs go in `build/`. Dependency sources and private test artifacts live
outside the checkout under `HAMTARO_NATIVE_WORK`, defaulting to
`%USERPROFILE%\.hamtaro-native-dev`. ROMs, saves, extracted resources and captures
are excluded from version control and release packages.

## Guest memory and callback contracts

`GameAddr` is an explicit 32-bit guest address. Little-endian accessors implement
GBA regions, mirrors and register side effects. Host pointers never enter game
structures or serialized fields. Original integer wrapping, aliases and mutation
order are retained where game behavior depends on them.

ROM function addresses resolve through typed native dispatch for Main, text-task,
animation, background and interrupt calls. Each binding preserves its own
arguments, consumed-byte count and yield behavior. Main opcode 25 assigns
text-task callback addresses; the text command F4 selects a callback by index.
An address following an arbitrary F4 byte is not a valid callback inventory.

The interpreters retain guest cursors, variables, return stacks and task state.
Unknown callbacks, invalid resources and exhausted command budgets return a fault
with a guest address. The frontend pauses the session and preserves the last
completed save snapshot.

Some source routines depend on stack contents or escaped local pointers. Their
adapters handle these cases explicitly:

- SaveGroup3's first empty render entry uses Z = 0; subsequent entries retain
  the last defined Z. Helper comparisons also cover supplied nonzero values.
- SaveGroup3's 20-byte list text lives in reserved guest storage
  (`HN_GROUP3_LIST_TEXT`) so a text task can retain it after callback return.
  Comparisons check the relocated pointer separately and compare the text.
- Failed rock-use output is initialized to zero before the original failure path
  copies its two bytes into scene controls.
- Raster setup carries reviewed retained arguments explicitly. Invalid selector,
  cyclic-list and nonterminating-copy cases fail at a bounded guard.

These choices avoid host undefined behavior; they do not reproduce arbitrary ARM
stack history. Function tests distinguish such guards from matching game states.

## Video, timing and audio

The native game clock runs at `16777216 / 280896` Hz, independently of monitor
refresh. Guest video produces a 240 × 160 framebuffer. SDL presents new frames
with nearest-neighbor filtering, preserving the 3:2 aspect ratio. Integer scaling
is optional and disabled by default. The initial window is 960 × 640, adjusted to
fit the monitor; remembered sizes snap to a whole scale at launch.

The software renderer implements backgrounds, objects, palettes, affine
transforms, windows, mosaic and blending. Scanline timing connects HBlank,
VBlank, VCOUNT, timers, DMA and native interrupt callbacks. DMA working addresses
are separate from programmed registers. An already-enabled channel's control
rewrite does not re-latch or immediately restart its transfer.

The game's native mixer and PSG model produce 32768 Hz stereo PCM. Desktop output
passes it through a persistent, approximately 10 Hz DC-blocking filter and submits
floating-point samples to SDL for device conversion. Filter state survives packet
boundaries and pauses, and resets with the session. Raw reference comparisons use
PCM before this desktop filter.

Launcher presentation is capped at 60 Hz, and minimized windows skip rendering.
Input polling and audio backpressure preserve the game clock. Settings sliders
apply immediately and write preferences when editing ends.

## Profiles and persistence

The default profile is `%USERPROFILE%\.hamtaro-native`; `--data-dir` selects another
directory. It contains `game.gba`, `settings.ini`, the original 32 KiB `game.sav`,
at most one `game.sav.bak`, and an error diagnostic when needed. Updating the
executable leaves the profile intact. A profile has an exclusive process lock.

ROM imports check size, header identity, revision and SHA-256 before atomic
replacement. Import sources are never modified. ROM and save replacement is
disabled while a game session is active, including when paused.

The game retains its original SRAM format, checksums and save timing. The host
captures SRAM only after successful initialization or a complete native update.
An update fault cannot overwrite the last committed snapshot. A file-write error
pauses gameplay and retains the completed pending snapshot for retry.

Settings use a versioned INI file. Invalid properties fall back to defaults;
unreadable files and future schema versions are reported and preserved. File
dialogs are asynchronous, and callback result storage remains valid if the app
closes before a dialog completes.

## Tested scope

This matrix summarizes recorded function comparisons, native input scenarios and
desktop checks. Detailed routes and commands are in [testing.md](testing.md).
Recorded runs span multiple revisions; they are not a fresh full playthrough of
every build.

| Area | Recorded coverage |
| --- | --- |
| Main story | New game, Sunny Peak, Sandy Bay and Treasure Island, Fun Land, Haunted House, Wildwoods, temple, Spat's defeat, complete credits and saved postgame |
| Reconciliations and words | All 21 reconciliations and their epilogue; all 86 Ham-Chats, book actions, pages, categories and saved order |
| Optional activities | Key contest, special-stone caves, quarry and Ham-lift routes, polishing, crafting, wardrobe, photos, local Ham-Jam and three contest tiers, pair-board win and loss |
| Menus and languages | Original Save/Continue, title safeguards, Items/Gems/A-Box; title language selection, localized Status and save/reload in all five ROM languages |
| Game subsystems | Main/text/background/graphics/animation contracts; actors, movement, collision, map-bank families, records, saves and audio compared by function |
| Video and sound | Synthetic PPU, BIOS, bus and audio comparisons; sampled clocked raster effects; focused scene regressions and paced SDL audio delivery |
| Windows frontend | Startup, settings persistence, keyboard remapping/play, pause, fullscreen restoration, artwork and menu layout reviewed in the application |
| Controllers | SDL virtual-gamepad hotplug, mappings, deadzone, short taps and disconnect tested |

Coverage is not exhaustive across optional branches, random minigame layouts or
hardware timing. Physical controllers, startup on a clean Windows installation
without development tools, and one uninterrupted full desktop playthrough have
not been validated. The release imports only Windows system DLLs and links the
MSVC runtime statically; this is a dependency check, not a clean-machine test.
Cable Link and internal game editors are outside the supported scope.

## Extending the port

Use `tools/inventory_reference.py` to locate source families and typed dispatch
dependencies before adapting callbacks. Inventory counts describe declarations
and bindings, not execution coverage. Shared scripts contain both ordinary game
subroutines and internal editor entry points; link declarations also need to be
classified separately.

Adapt related callbacks with their helpers, aliases and calling conventions,
then run the affected reference comparisons and input scenarios. Keep one
coverage matrix here, reproducible instructions in `testing.md`, and generated
reports outside the repository.
