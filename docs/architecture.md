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
The release PE imports only Windows system DLLs and links the MSVC runtime
statically. A separate clean-machine validation remains pending.

## Current coverage

| Area | Status | Evidence or remaining work |
| --- | --- | --- |
| Windows x64 application | Implemented | Native game view, Play/pause/resume/restart/menu, settings and save management; smoke-tested and real-window review |
| ROM import | Implemented | Identity and full hash, invalid imports preserve existing data |
| Settings and bindings | Implemented | Persistence, bounds, defaults, Unicode paths |
| Keyboard/gamepad | Implemented | Synthetic keyboard, virtual-pad input, brief taps retained for one game update, deadzone, remapping, disconnect; physical models not tested |
| SRAM file management | Implemented | 32 KiB import/export, atomic writes, previous-copy recovery, exclusive profile lock; End Game trace persists to disk and a fresh host session retains twenty-one words, ten seeds, Bijou and one rescued love in the Clubhouse; actual-window End Game preserves its previous backup, and closing/relaunching the executable restores the same scene and counters |
| GBA memory and DMA | Foundation | Region/mirroring, read-only IO masks and repeated DMA with working addresses separate from programmed registers; not full hardware conformance |
| BIOS helpers | Foundation | CpuSet, CpuFastSet, bounded LZ77, division, integer square root, ObjAffine and native angular math; 37,009 angular cases compared with mGBA; square root checked throughout the 16-bit domain and at 32-bit boundaries; hardware conformance remains unverified |
| Software video | Foundation | 64 synthetic static frames compared with an independent PPU; 30 private ROM snapshots compared, including directional movement; raster timing still pending |
| Video timing and IRQ | Foundation | Scanline/VBlank/HBlank, timers, IRQ flags, FIFO DMA and native VBlank service connected; 9,000 clocked native updates completed; full-machine cadence and DMA bus contention remain unverified |
| Native game helpers | Adapted | Original guest input layout, directions/repeat/query semantics and integer PRNG/ranges compared; host controls and per-frame PRNG connected |
| Native heap | Adapted | Original 12-byte nodes and 32-bit links; private ROM function comparisons |
| Video state and transfer queues | Adapted | Original video shadows, mosaic/blend/fade updates, DMA queue wrap/fill/flush; private function comparisons |
| Map and core initialization | Adapted | Runtime tables read from ROM, task sentinel, render slots/order, background state and all 11 map banks tested at index 0 |
| Objects and animation scripts | Partial | Physics, positions, priority ordering, affine reuse, OAM and animation/sound commands compared; animation callback bindings remain incomplete |
| Background scripts and transforms | Adapted | All 61 command handlers, repeat/call flow, motion, clamping, affine layers and tilemap copies compared; invalid stacks/resources fail explicitly |
| Graphics scripts and palette effects | Partial | All 48 command handlers compared, including raster allocation, position, windows, radius profiles and sweeps; circular and wave HBlank callbacks compared, remaining scanline callbacks and timed DMA/PPU integration pending |
| Word expressions | Adapted | Arithmetic, comparisons, short-circuit cursor effects and variable/flag updates; bounded recursion and private randomized comparisons |
| Actor script contexts | Adapted | Sixteen 84-byte contexts, Main state preservation, nested calls/waits, activation/stop and same-frame activation compared; per-actor command budgets fail explicitly |
| Scene actor snapshots | Adapted | Four 104-byte render snapshots, active-slot restoration and six modal map roundtrips compared; idle animation variants, optional slots and script cursor retention preserved |
| Shared scene actor control | Adapted | All 40 functions in class_anchor_presets.c and scene_control_callbacks.c compared: anchors/modes, vectors, signed rectangles/corridors, aiming, background alignment/offsets and clone phases; later-activity runtime integration remains pending |
| Scene render presets | Adapted | All 77 functions in script_callbacks_render_presets.c and 28 required actor helpers compared; typed F4/task calls and 35 defined-zero Main callbacks, linked slots, indicators, graphics and live class selectors; full area/activity integration remains pending |
| Bank-2 actor and scene families | Adapted | All 120 functions in map_bank2_render_callbacks.c, map_bank2_scene_control.c, map_bank2_scene_presets.c and map_bank2_height_callbacks.c compared; 116 argument-free task targets, 70 Main contracts, panel stages, randomized motion, height latches and one full-word animation yield; complete Sunny Peak integration remains pending |
| Bank-3 late scene family | Adapted | All 31 functions in map_bank3_late_callbacks.c compared: 25 void presets, four animation callbacks and two Main modal callbacks; 27 F4 targets, song pan/tempo, mirror/affine scales and raw actor snapshot roundtrips; full area integration remains pending |
| Bank-3 main scene/activity family | Adapted | All 282 source functions accounted for across presets, source/spawn entries, pair board, phases/orbits, record text, chains and one shared condition; typed Main/task/animation contracts and helpers compared, including live links, byte wrapping and retained-register paths; complete activity progression and area integration remain pending |
| Shared actor movement and turning | Partial | 23 related functions compared: wandering initialization/update/bounds/velocity, four animation contracts, coordinate/direction readers, facing, stepped turns and class sequences; 17 Main contracts and four reviewed task targets; optional bounds probes accept the reviewed prospective-region predicate and reject unknown targets; later-area integration remains pending |
| Shared actor geometry | Adapted | Eleven functions across eight source families compared: leader offsets/velocity, ten-byte projected-target approach, raw direction pairs, screen cells/sides, class coordinates, projected distance and render-pair setup; ten Main contracts and three task targets; full scene integration remains pending |
| Shared screen and shake effects | Adapted | Eight functions across five complete source families compared: display mode, window/OBJ flags, background anchors, shake setup/reset/update, raw render targets and amplitude decay; eight Main and eight argument-free task contracts; first-reconciliation integration exercised, full-game timing remains pending |
| Scene actor state and panels | Adapted | 44 functions across 15 source families plus one shared cross-bank condition compared: snapshots with and without execution-flag clearing, forward copy/swap, signed accelerations, class-target approach, panel presets and direction constructors; 36 reviewed argument-free task contracts; complete scene integration remains pending |
| Actor render presets | Adapted | All 993 functions across ten base/linked/neighbor/class/secondary/render-setup/graphics/directional/extended/sequence families compared; task ABI, captured auxiliary arguments, live selectors, inactive mapped slots, byte-sized neighbors and direction sectors preserved; only the base family's defined-zero no-op also accepts Main; later-area integration remains pending |
| Map-choice scene control | Adapted | All 273 functions in map_choice_callbacks.c compared: 93 Main contracts, 253 argument-free task targets, five animation contracts and four slot helpers, including cursor skips/jumps, packed choices, motion, regions, phases and presets; complete area/activity integration remains pending |
| Map particles | Adapted | All four functions in map_particles.c and the alternate parameter selection compared; initialization, reset boundaries, live parameter reads, scaling and field60-only hiding; timed scene integration remains pending |
| Motion and collision | Adapted | Leader input/walk/run, action priority, map edge sampling/sliding, collision responses, event staging, follower routes, actor-position snapshots, transition animation/position selection and wrapped facing interpolation compared; full-game coverage remains pending |
| Text counters | Adapted | Three ROM template/palette banks, digit conversion, class aliases, addition/subtraction limits, delayed stepping and sound/yield callbacks compared; resources remain in the private ROM |
| Text tasks and dialog scripts | Adapted | Linked contexts, tiled glyphs, delays/fast advance, line/page scrolling, all 32 text commands, numeric text, icons, language variants and 27 typed animation callbacks compared; other indirect callbacks remain incomplete |
| Native save records | Adapted | Original payloads/checksum, SRAM identity initialization/repair, primary/backup group loading and grouped writes; private function comparisons |
| SaveGroup3 pairs and lists | Adapted | All 67 functions across seven field/catalog/pair/graphics/list/selection families compared, including 51 Main callbacks and reviewed task contracts; photo-room entry/exit and greeting run natively; complete activity and save integration remain pending |
| Record selection menus | Adapted | All 18 record_selection.c functions compared, with two shared strided-transfer helpers; record rows, scrolling, text contexts, counters, markers and window registers preserved; complete Items/Gems and timed raster integration remain pending |
| Gameplay record slots and counters | Adapted | Nine allocation/query/consume/counter functions compared, including packed fields, result layouts, wrap/clamp behavior and three allocation callbacks; batches retain zero/full capacity branches and original animation timing; complete inventory UI and save integration remain pending |
| Status counters and stage sequence | Adapted | All 13 functions in map10_4_counter_sequence.c compared, including the complete 16-bit counter domain, five tile banks, windows, stage/scroll graphics and sequence phases; nine Main and twelve argument-free task contracts; later-stage live integration remains pending |
| Word-choice dialogs | Adapted | Nine visible text contexts after filtering up to 255 candidates, grid navigation/highlights, disabled rows, confirmation/cancellation, requested selection, delayed close, display reset and Main result preservation compared; full-game integration remains pending |
| Ham-Chat book | Adapted | Acquisition, learned-word order, five language permutations, category/page selection, paired text contexts, counters, markers and cleanup compared; initial book display and return run natively; manual book navigation remains unverified |
| Destination map | Adapted | Completion/visibility flags, current/destination markers, backgrounds, direction priority and sequential unlocks compared; native input selects Clubhouse/Sunny Peak in both directions and enters Sunny Peak |
| Main script control/resources | Partial | Fetch/yield/wait, branches, memory/decompression, forward byte/halfword pointer copies, maps, saves, task/actor/indirect setup, word-choice dialogs, collision scenes/routes, random values, core reset, choice flags, fades and typed callbacks; unsupported opcodes fail explicitly |
| Scene menus | Partial | Menu/record selection, name editing, glyph blitting, keyboard banks, panel entry/exit/depth, language snapshots and input dialogs compared; Select status display, left/right navigation and End Game confirmation connected; Items/Gems and later activity panels remain incomplete |
| Native boot/title | Partial | Real SDL window reaches the language selector with music; 9,000 clocked native updates completed; 88 Main-only, 180 idle and 9,000 component-schedule updates compared; complete boot/title/gameplay parity remains unverified |
| New game, movement, in-game save/load | Partial | A 139,833-update native input scenario completes the basic lessons/reward, Sunny Peak through Bijou, Ham-lift, Oopsie, Gossip-P and the first reconciliation, Spat/Harmony conversations, Clubhouse return, photo-room greeting, book/status and seven End Game/Continue roundtrips; host-file/fresh-session validation is recorded below; full story and desktop playthrough remain pending |
| Music and effects | Partial | Native sequencer/mixer/PSG/FIFO/PWM connected to SDL; 600 real-time updates with no device sample shortages and user-confirmed clear menu music; complete game-output parity, filtering and trigger quirks remain pending |
| Complete single-player playthrough | Pending | Story, secondary activities and minigames |
| Cable Link | Out of first release | No local/network link emulation |
| Public release | Pending | Native gameplay, complete coverage and source-provenance review |

Play runs compiled C game logic with native video/audio devices. A missing callback,
unsupported CPU-transfer queue, communication update or IRQ target stops execution
and opens the menu with a private runtime-error.log diagnostic. It never switches
to guest instruction execution. The language selector and menu music establish a
native desktop boot milestone, not a completed title/story/playthrough.

The private opening scenario leaves the bedroom, reaches the Clubhouse, displays
the initial ten-word Ham-Chat book and returns to the conversation. It then performs
Hamha, Tack-Q, Hif-hif and Digdig, including a rejected word and successful retry,
and receives ten sunflower seeds. It exercises native clocked updates from fresh
SRAM with fixed input and script/position/counter checkpoints, without state injection.
It then opens the destination map, checks both directions between Clubhouse and
Sunny Peak, confirms travel and reaches control in map 2:1. Walking north then enters
the river scene, advances its dialogue, performs Hamha and opens the bridge's
filtered word menu. Selecting Tack-Q returns to player control in map 2:11.
It reopens that menu, selects Lookie, advances the response and walks east to
x=203.5, z=200.5. It then enters map 2:17, reaches player control, opens an actor's
word menu, chooses Hamha and completes the conversation. Its last checkpoint
returns control at x=156.5, z=208.5 with the ten learned words and ten seeds retained.
The route then returns west, crosses the northern path into map 2:20, climbs the
stairs to map 2:2 and triggers the stump conversation. It learns Heyhoo, increasing
the word count from ten to eleven, opens the updated book, enters its first page
and returns to movement.
Select then opens the status display. The route selects End Game, confirms the
original SRAM write, returns to the title and chooses Continue. The restored state
has the same map 2:2, position, eleven words and ten seeds. It then talks to Oxnard
to learn Bluhoo, walks east and completes the handkerchief scene to learn Gogo.
A second End Game/Continue restores map 2:3, position, thirteen words and ten seeds.
The route then crosses the upper maps and descends two vines to the boat dock.
Lookie with Arnie teaches Pakapaka. The player boards the boat, cuts its rope and
drifts west to map 2:17, then disembarks and talks to the wandering hamster.
Answering Yep-P teaches Putput. A third End Game/Continue restores map 2:17,
x=131.5, z=208.5, fifteen words and ten seeds.
Heyhoo then summons Seamore, whose dialogue teaches Offdoff. Removing the bridge's
battery and placing it in the boat enables powered travel, including a roundtrip
through map 2:18 and docking on the north bank of map 2:17. Bijou's conversation
teaches Mega-Q and enables her as the player's partner. A fourth End Game/Continue
restores x=168.5, z=80.5, seventeen words, ten seeds, the joined-partner flag and
follower slot 13. Returning by boat and climbing the western path reaches the
Ham-lift lesson. The pair then lowers Pepper from the vine, selects Bluhoo, learns
Oopsie and Gossip-P and completes the first reconciliation. Player control returns
in map 2:2 at x=112.5, z=88.5 with twenty words, ten seeds and one rescued love.
Select displays the retained counters; a fifth End Game/Continue restores that
position, Bijou and the same counters. The subsequent Spat scene leads to Harmony
and a destination-map return to the Clubhouse. A sixth End Game/Continue restores
map 1:1 at x=144.5, z=256.5 with twenty-one words, ten seeds, Bijou and one rescued
love. These checkpoints establish the word count, not which dialogue line teaches
the additional word. The next route enters the photo room, completes the greeting
and returns to its action menu. A seventh End Game/Continue restores map 1:3,
x=130.5, z=80.5, the same twenty-one words, ten seeds, Bijou and one rescued love.
Complete Sunny Peak and photo-activity coverage remain pending.
The scenario counts 139,833 input updates in total; the runtime frame counter resets
on returning to the title. A separate replay through the actual host Session API writes private
32 KiB SRAM files, stops the session and starts a fresh one from disk. Continue
restores the same nine story/position/partner/counter fields; this replay passes
147,133 host-session checks without retaining its temporary profile. A separate
earlier review launches the actual Windows executable with its own private
Clubhouse profile. Continue restores the Clubhouse
scene; Select, assigned to V, displays twenty-one learned words, ten seeds and one rescued
love. End Game returns to the title and writes the 32 KiB file, with the preceding
file's exact bytes retained as its backup. Closing and relaunching the executable,
then choosing Continue, restores the scene and the same three visible counters.
WASD menu navigation and copied control settings were also reviewed. This establishes
bounded desktop save/continuation evidence.
Book pages, word menus, rewards, the destination map and Sunny Peak have been
visually inspected through native PPU output; the full desktop story playthrough
is still pending. No game capture is stored in this repository.

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
2,034,370 checks with the independent BIOS and PPU oracles, including separately
identified corrupt-state guards alongside successful state comparisons.
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
and queues the original palette transfer. Counter changes retain signed limit tests,
halfword conversion, oversized-subtraction rejection, small/large step schedules and
the second bank's shorter addition delay. The stepping callback preserves wrapped
countdown arithmetic, repeated sound triggers and writes only the low yield byte.

The Ham-Chat book reads page boundaries, language order and animation/text resources
from the player's ROM. Learned words retain their acquisition order; the sorted
view uses the original language permutations. Page restoration, paired text contexts,
reverse cleanup, wrapped marker positions and leading-blank number rendering have
private reference checks. Story turns preserve unsigned screen-Y comparisons,
signed half-height tests, leader/follower aliases and retained opposite-turn state.
Destination indicators retain the seven areas' flag conjunctions, mode-specific
visibility and navigation banks, first-direction priority, current/destination
overlays and one-at-a-time unlocking. Positions, labels and animation resources
remain in the ROM; progress and selection remain in their original guest RAM views.
Actor transition callbacks retain full direction bytes when selecting animation
families, original boot-gate ownership, fractional positions, secondary-slot aliases
and run-speed delay adjustments. Facing interpolation uses wrapped 32-bit fixed-point
division/multiplication and original eight-unit rounding, including negative inputs.

River scene adapters preserve the original piecewise actor-position remapping,
live control aliases, saved-position restoration, bounded approach velocities,
render-mode ownership and linked slot setup. Eight argument-free callbacks have
explicit Main and animation bindings; the animation ABI preserves the yield word.
Surface-height updates retain unsigned region boundaries, one-step overshoot and
leader/follower aliases. Region audio reads its records from the player's ROM,
preserves inclusive signed bounds, truncated volume products and song suppression.
The original clamped-approach zero-duration path and inside-region audio modes
above two read uninitialized source values; these cases fail explicitly until a
defined caller contract is established. They are not silently replaced with host data.

Shared scene motion is grouped in scene_motion.c. Its two complete source families
retain strict rectangle edges, signed-half coordinate wrapping, midpoint rounding,
fixed-point aiming, background alignment and phase/countdown overlays in shared
RAM. Clone slot pairs and animation/frame resources are read from the player's ROM;
full 104-byte slot copies preserve aliases and source mutation order. Signed scene
results are loaded before setting the save flag, including aliased arguments.
The PRNG uses signed remainders after wrapped 32-bit products, matching the source
even when the seed words fall outside their normal initialized ranges.

presets.c groups scene-render setup and its actor-helper dependencies. Task F4
dispatch ignores original return values; Main dispatch accepts only the 35 reviewed
functions that explicitly return a zero consumed count. Void helpers never acquire
an invented Main return. Animation selection preserves whether each source helper
clears the follower boot gate. Linked-slot setup retains the original configure/mode
order and reloads class selectors after writes that may alias them. Clear operations
touch only field60, without resetting or releasing the render record. Graphics,
animation/frame resources and labels remain in the player's ROM.

map_choice.c retains the original typed entry points for map selection, actor
presets and motion. Packed choices use live RAM tables initialized from the ROM;
large selectors shift to zero, and a zero flag nibble retains the previous flag.
Main callbacks preserve their variable consumed counts and the unsigned -4 cursor
jump contract. Position save/restore performs sequential guest-memory reads and
writes, including overlap with script variables. Movement retains signed 32-bit
wrap, halfword boundaries, one-step height overshoot, predicted-position clamping,
bounded routes and source INT_MIN quirks. Follower phases and motion graphics keep
their conditional resource ownership. Direction queries use native BIOS angular
math, with the source's four/eight-direction sectors and wrapped coordinate deltas.

particles.c reads slot-index and physical-parameter tables from guest RAM/ROM,
preserves the source's live count and pointer reloads, and resets only the original
position, velocity and scale fields when a particle leaves its bounds. Acceleration
is retained during an update reset. Slot indexes and resources fail explicitly
when they cannot be represented by the native scene. No particle data or animation
contents are embedded in this repository.

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

Scene transitions separately snapshot four render slots into the original save-group
RAM. Six modal map callbacks retain the previous map bytes and return the original
unsigned -4 consumed count. Their restore path retains the caller's script cursor,
restores active actors and selects native idle animations from ROM tables. The fourth
slot keeps the first or third slot's direction variant exactly as the source does.
The plain restore path clears each restored animation execution flag and preserves
inactive destinations. Main pointer-copy commands retain their seven-byte packed
arguments, variable indirection and forward overlap semantics. Graphics-script
disable-except and display-layer flag callbacks are also compared with the source.

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
cursor for up to 255 candidates before enforcing the nine-visible-option limit;
candidate IDs above 16 have no disabled bit in the 16-bit mask. Labels select ROM
language bundles or adjusted-initial buffers. Opening
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
The typed HBlank dispatcher supports the circular effect, scanline wave/blend and known no-op callbacks;
other callback addresses fail explicitly. Static snapshots and sampled callback
comparisons do not establish raster-image fidelity or the real IRQ cadence. Main save-load
commands preserve the failure cursor/depth behavior and reload the saved map with
a white palette on success. The connected input scenario now verifies End Game's
SRAM write and Continue; host-file persistence has separate session checks.

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
the input-only scenario now exercises End Game, reset and Continue with retained
story state. The host-session replay separately verifies persistence to disk and
fresh-session continuation. A separate Windows process also restores the exported
scene and status counters. Invalid
group selectors are rejected; the unused group-0 record write/load path is not exposed.

## Remaining integration order

1. Review remaining area and shared scene families with their dependencies, then adapt
   complete groups of story/scene callbacks, scanline effects and timed CPU transfers.
2. Extend the post-Harmony Clubhouse route through the photo room and SaveGroup3
   UI, then the remaining Sunny Peak and later-area routes; validate story effects and their save state in
   the actual Windows window.
3. Expand coverage through the remaining story, secondary activities and minigames;
   establish input scenarios and compare story state and save effects with the reference.
4. Compare canonical RGBA, state, SRAM and pre-device PCM. Keep original
   frame cadence, original resolution and gameplay; host display refresh is independent.
5. Verify the packaged app on a Windows machine without development tools.
   Public CI remains ROM-free; the private reference suite takes a user path.

## Adaptation workflow

The reference's current progress report verifies 4,214 C functions (332,660 bytes),
4,252 assembly bytes and 45 unidentified bytes in a byte-identical ROM build.
Reconstructed data and padding are separate from recovered executable code.
This is a strong source reference, not a Windows coverage percentage.

Use tools/inventory_reference.py before choosing a batch. It groups the original
functions by source family and identifies callback-named candidates with explicit
native dispatch entries. Names miss some indirect targets; a dispatch entry alone
does not establish correctness, ABI compatibility or execution coverage. Helpers
can be consolidated in native functions, so unmatched helpers are not automatically
missing implementations. The coverage matrix above remains the behavioral record.

| Batch order | Source families and purpose | Completion evidence |
| --- | --- | --- |
| 1 | Shared actor presets, scene control and map-choice callbacks: class_anchor_presets.c, script_callbacks_render_presets.c, scene_control_callbacks.c and map_choice_callbacks.c | Review each family's entry points, helpers, live aliases and calling contracts; adapt related callbacks together, then compare the complete family |
| 2 | Area scripts, beginning with the remaining bank-2 families, then bank-3/4/5/6 and later areas in dependency order | Whole source families and their script dependencies compared before longer area scenarios; story flags and secondary routes checked explicitly |
| 3 | Selection, pair lists, save-group displays and single-player modal activities | Native UI/state roundtrips, original save effects and activity outcomes; Cable Link remains excluded |
| 4 | Remaining raster/IRQ callbacks and full-game audio behavior, alongside the areas that use them | Function comparisons plus timed native video/audio integration; static images alone are insufficient |
| 5 | Complete story, secondary activities, minigames and in-game save/reload | Longer input scenarios and actual-window playthroughs, with completion state and compatible SRAM verified |

For every batch, enumerate all entry points and dependencies before implementation,
record undefined/unresolved contracts, adapt a coherent family, and run its focused
comparisons. Run the full differential suite once at the batch boundary unless a
failure needs isolation. Then extend long native scenarios and inspect affected
desktop behavior. Gameplay discovers integration faults; it is not the primary
inventory of functions still to port. Keep generated inventories private or on stdout,
update these existing documents and retain no per-function reports in the repository.

Within batch 1, class_anchor_presets.c and scene_control_callbacks.c have complete
function-level comparisons for their 40 functions. script_callbacks_render_presets.c
also has complete comparisons for all 77 functions, including its 28 required actor
helpers. map_choice_callbacks.c now has complete comparisons for its 273 functions,
along with 11 required actor helpers, the four-function particle family, direction
queries and relative planar velocity. Other functions in the actor source families
still need adaptation as their area dependencies are reviewed. This batch status
does not establish completion of any later story area or activity; extend native
area scenarios and verify actual-window progression next.

Within batch 2, the four bank-2 actor/control/preset/height families have complete
function comparisons for all 120 functions, including two existing motion adapters.
The three reviewed assembly leaves in map_bank2_script_noops.s are accepted only
through Main opcode 0x51's void argument-word contract. They preserve registers in
the original and do not acquire a fabricated Main1C consumed count. The original
0x51 variable route calls the variable slot address, not its stored pointer; an
unreviewed target fails explicitly. The bank-2 checks include 480 consecutive
motion updates, signed height boundaries, halfword wrapping and live panel-stage
aliases. The longer native scenario now reaches the first reconciliation, but
additional routes, later-area dependencies and complete Sunny Peak coverage remain
pending; a family count is not a completed area.

The connected movement/direction batch covers 23 functions across the original
wandering, render-turn, coordinate/direction, stepped-turn and class-sequence
families. It preserves the two 36-byte motion contexts, live selectors, callback
actor versus captured actor, missing-record yield retention, signed table-column
underflow, byte/halfword wrapping and the source's repeated animation setter.
Bounds stop only the offending axis before optional probes and nearby-actor checks.
The reviewed optional u32(slot, direction) predicate ignores its second register;
unreviewed probe targets fail explicitly. Invalid wandering states that would spin
forever in the source fail on the host; unknown render-turn states retain the
source's no-op behavior. The extended native scenario now exercises these families
through eastern-map entry, wandering actors, facing and a complete conversation.
The longer native scenario now includes the first reconciliation and Clubhouse
return. Complete Sunny Peak coverage and later areas remain pending.

The shared actor preset implementation covers all 476 void functions across
actor_animation_linked_presets.c, actor_animation_neighbor_presets.c,
actor_class_animation_presets.c, actor_secondary_animation_presets.c and
actor_render_setup_presets.c. Earlier partial adapters now delegate to the complete
implementations. Auxiliary setup captures its byte-sized slot and
parent arguments before calling helpers; separate source calls reload live selectors.
No void return is repurposed as Main's consumed-argument count.

Three further complete families add 446 graphics/directional/extended animation
presets in actor_graphics_animation_presets.c,
actor_directional_animation_presets.c and actor_extended_animation_presets.c.
Their native implementations share actor_presets.c and the typed task dispatch;
thirty-nine earlier partial adapters now use the complete implementations. Original
word-choice wrappers accept mapped inactive byte-selected slots, wrap neighbors
at 0/255 and configure graphics records independently of their active count. Raw
native helpers preserve those contracts while the existing guarded APIs keep
their validation. Direction selection retains the source's missing-sector no-op,
and the reviewed indicator that selects a different class from its animation.
Auxiliary arguments remain captured; separate calls reload class selectors after
each potentially overlapping write. These function checks prepare later-area
dependencies; they do not establish that those areas are playable.

Forty-four scene actor/state/panel functions across 15 source families preserve
the nine-byte class-coordinate approach state, fixed-point/signed boundaries,
forward copy/swap order, two-slot backup/restore, screen-pair presets and direction
constructors. One additional shared bank-3 value condition supports the bank-2
scripts without claiming the rest of bank 3. Nine gameplay-record functions retain
packed counters, the twelve-byte query result and original underflow/clamp behavior.
The three-function scanline-wave family has typed Main/task/HBlank adapters with
all-line threshold and frame-end phase comparisons; timed raster images remain
unverified.

tools/inventory_reference.py --area-bank 2 now finds explicit dispatch for all 383
functions declared by the four recovered bank-2 script sources. This is a planning
inventory: declarations can be unused, while literal shared-script targets and
animation resources can introduce other dependencies. It proves neither a complete
call graph nor Sunny Peak completion.

The shared geometry batch covers eleven functions across eight additional source
families. The ten-byte render-slot approach shares the reviewed movement core with
the nine-byte class-coordinate callback while retaining projected target velocity,
signed offsets, selector 255's following byte, one-pixel steps and live idle aliases.
Leader-offset callbacks keep captured pointers and ordered reads even when an actor
is the leader. Raw direction/secondary-screen callbacks preserve mapped inactive
byte selectors; the existing bounded direction API retains its guards. Other
checks cover sequential coordinate aliases, signed-half absolute overflow,
unsigned screen cells, linked render pairs and video-mode dirty state. Empty
screen grids have an analytic host check; the CPU probe deliberately rejects BIOS
division by zero, so that boundary does not establish BIOS behavior.

Shared screen/shake effects also live in scene_motion.c. All eight functions in
display_mode_clear_callback.c, screen_window_callbacks.c,
script_callback_configure_shake.c, script_callback_render_shake.c and
script_callback_reset_background_shake.c have reviewed Main and argument-free F4
contracts. The update preserves byte-sized phase/counter wrapping, signed division
toward zero, captured render destinations, mapped inactive targets, live decay
aliases and background-1 anchors captured before any selected layer is changed.
The original always writes layer 2's anchor, including when other layers move.
Window callbacks retain unrelated display bits and write the original WIN0 bounds.
These contracts are not enabled as animation callbacks without separate ABI evidence.

Use --script-source "shared*main_scripts.c" to inventory common script declarations
in addition to a map bank. The four recovered shared files declare 103 functions;
43 still lack explicit dispatch after this batch. This remains structural planning,
with the same limits on literal targets, ABI correctness and runtime reachability.

The complete bank-3 late family is grouped in bank3.c. Its 31 functions retain
25 void preset contracts, four animation consumed/yield contracts and two Main
modal contracts; the 27 reviewed F4 targets ignore the return where appropriate.
Pan/tempo counters wrap as bytes and resolve the original song-player records.
Mirror uses the entire control byte, while affine scale propagation captures X
and reloads Y in source order. Exhausted affine allocation returns index 32 and
preserves its adjacent usage-byte alias. Modal save/restore accepts mapped inactive
byte-selected actor slots and retains the original script cursor and map controls.
Focused comparisons cover 31,045 checks for that family plus song-43 restart and
its stopped-track helper, including active ten-track audio players,
the eight-bit track mask, all byte counters/mirror values, inactive slots and
SRAM/video memory.

The 282-function main family uses bank3.c for 212 argument-free scene presets,
bank3_games.c for 28 Main callbacks, seven animation callbacks and five typed
helpers, and bank3_chain.c for 28 chain functions. One neighbor-hiding helper and
one previously adapted shared condition complete the source inventory. The 23
base-actor dependencies share actor_presets.c. These are complete-family function
comparisons, not completed activities or a full bank-3 playthrough.

Source/spawn records preserve unknown bytes and timer fields. A full ten-slot pool
retains the original mapped slot-10 alias; a byte selector of 255 wraps the actor
constructor index but not the subsequent full-width velocity address. Phase modes
outside 0..4 retain incoming R4 through an explicit native parameter. Box collision
uses the original strict endpoint tests, including containment and touching edges.
The native square root returns floor(sqrt(value)) for uint32_t; its test oracle
models the BIOS result, without BIOS timing or volatile scratch-register evidence.

Chain effects reload neighbor links after every operation that can change them,
including a mapped index-255 reread. Reversal preserves link fields and inverts
velocities only for entries selected by the original kind predicate. Kind lookup
preserves incoming R4 when absent. Its wrappers retain either their byte value or
loop index. On invalid saved flag values, the original flag initializer instead
retains an old-flags guest stack address. Direct probes pass that frame explicitly;
the normal task path requires the gameplay invariant of flag values 0..9 and
fails explicitly on an unresolved invalid value. Invalid random rejection loops
are bounded at 4,096 attempts, and pathological recursive effect graphs at depth
64, rather than hanging the host. These limits do not establish original behavior
for corrupt game state.

The remaining bank-3 declarations were adapted by complete source family: eight
screen-pair presets, a raw saved actor-pair restore, batch record allocation,
song-43 volume restart and its stopped-track helper. The actor restore preserves
the saved execution byte and reloads indices after each forward copy. Record
batches preserve the original capacity limit, zero-amount flag behavior, byte-wise
argument reads and counter animation division boundaries. Restarting stopped
audio tracks captures the byte count and track pointer, clears only 64 of each
80-byte record and restores its five original defaults; player resume is a
separate operation. Track counts above 16 and overlapping player records are
checked explicitly.

Bank 3's five recovered script sources now declare 512 functions with explicit
dispatch for every declaration. Helpers need separate review; these structural
counts prove neither runtime reachability nor area completion. Longer story,
activity and save-state routes remain required.

A separate private input-only scout completes the initial Ham-Jam conversation,
No-P response and return to Clubhouse. Photo-room entry (map 1:3), free movement
and return to Clubhouse also run natively after the SaveGroup3 batch below.
The greeting now completes and returns to its action menu after the entire
48-function actor_render_sequence_presets.c family was adapted. A subsequent
Items entry stops explicitly at ScriptCallbackChangeMap10_5FlagsFromKeys
(0x080C44C0), in the 14-function script_callback_actor_map10_5.c family. Review
that complete shared family and its dependencies next. The original Ham-Jam scout
is outside the canonical opening scenario; photo entry,
greeting and a save/load roundtrip now have canonical input markers. Neither route
establishes complete activity progression.
A separate Windows process also restores the photo-room save beside the characters
and renders the greeting, portrait, actor animations and continuation indicator.
The complete desktop photo activity and the training-seat quest remain unverified.

The SaveGroup3 batch covers the complete 33-function pair/render family, four
pair-selection functions, six graphics helpers, eight field helpers, the catalog
scanner and seven list functions. It preserves byte-sized selection/count wraps,
live catalog and record aliases, the completed-mask category-25 exception,
original palette prefix sums, five-variant scans, forward tile-map copies and
record backgrounds for all eight styles and empty slots. Void helpers retain
typed task-only contracts; argument-dependent Main callbacks are not enabled as F4.
The eight-function save_group3_list_selection.c family also has complete function
comparisons. It preserves the high selection bit, special lower row, flag-253
cursor variants, before/after entry flags, byte-wrapped coordinates and original
removal animation. The detail builder deliberately retains both original task
allocations for a populated entry and one for an empty entry; it normalizes the
first character and characters after line breaks with the original Japanese
exception. Its static guest text buffer does not need pointer relocation.
A native End Game save made in the photo room is also restored by a separate
Windows application process. Its status screen retains 21 words, ten seeds and
one rescued love; this does not establish complete photo activity progression.

ConfigureSaveGroup3PairRenderSlot leaves its local Z uninitialized for category
255. Direct helper comparisons supply that retained source word explicitly.
Consecutive calls in the native roots carry the last defined Z; the first empty
entry uses canonical zero rather than host undefined behavior. Its default original
OBJ frame has zero parts. Root comparisons seed the corresponding undefined
source input with zero; they do not establish arbitrary stack-history identity.

The list-text callback passes a 20-byte local array to a task that retains its
pointer. The native adapter moves that object to reserved guest storage at
HN_GROUP3_LIST_TEXT, outside host-used game data, so it survives callback return.
Comparisons verify the original escaped-stack pointer, native stable pointer and
identical terminated text, then exclude only that relocated pointer word from
whole-memory equality. A prefix that exceeds the original buffer and allocation
failure stop explicitly. These are explicit lifetime/bounds adaptations, not
evidence that the original stack address or corrupt-state behavior is reproduced.

record_menu.c contains the complete 18-function record_selection.c family, shared
with the save-list detail loader. It preserves five linked text contexts, descending
removal order, live descriptor/count reads, wrapped 30-entry row selection, cursor
and marker construction, decimal counts and window/DMA register writes. The
16 argument-independent entries accept F4; two byte-argument callbacks retain
their Main-only contracts. The forward/backward row helpers have a defined zero
return and ignore their word argument. Their native adapters preserve that contract
without treating a void preset as a Main callback.

The shared strided-transfer family uses the original halfword-normalized width,
row count and stride, forward BIOS halfword copies, cleared heap allocation and
free order. Zero rows preserve memory; failed allocation stops explicitly before
decompression. Function-level comparisons do not establish timed window rendering.
