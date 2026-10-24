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
| Software video | Foundation | 64 synthetic static frames compared with an independent PPU; 30 private ROM snapshots compared, including directional movement; nine clocked VCOUNT frames match independent PPU bands; remaining raster timing pending |
| Video timing and IRQ | Foundation | Scanline/VBlank/HBlank/VCOUNT, timers, IRQ flags, FIFO DMA and native IRQ services connected; 9,000 clocked native updates completed; coincident VBlank/HBlank/VCOUNT priority checked; full-machine cadence and DMA bus contention remain unverified |
| Native game helpers | Adapted | Original guest input layout, directions/repeat/query semantics and integer PRNG/ranges compared; host controls and per-frame PRNG connected |
| Native heap | Adapted | Original 12-byte nodes and 32-bit links; private ROM function comparisons |
| Video state and transfer queues | Adapted | Original video shadows, mosaic/blend/fade updates, DMA queue wrap/fill/flush; private function comparisons |
| Map and core initialization | Adapted | Runtime tables read from ROM, task sentinel, render slots/order, background state and all 11 map banks tested at index 0 |
| Objects and animation scripts | Partial | Physics, positions, priority ordering, affine reuse, OAM and animation/sound commands compared; animation callback bindings remain incomplete |
| Background scripts and transforms | Adapted | All 61 command handlers, repeat/call flow, motion, clamping, affine layers and tilemap copies compared; invalid stacks/resources fail explicitly |
| Graphics scripts and palette effects | Adapted contracts | All 48 command handlers compared; all 19 HBlank selectors and 26 connected window/raster functions adapted, including source heap/DMA order and typed callbacks; 46 clocked frames match per-line source registers and independent PPU output; original ARM latency and DMA bus contention remain unverified |
| VCOUNT blend and counter modes | Adapted | Ten connected functions compared, including complete blend/increment families, byte selectors/scanlines, 16-bit status/counter domains and five Main/F4 contracts; original pending bits, transition phase and live callback table retained; nine clocked frames match independent PPU bands at line 104; original CPU timing remains pending |
| Word expressions | Adapted | Arithmetic, comparisons, short-circuit cursor effects and variable/flag updates; bounded recursion and private randomized comparisons |
| Actor script contexts | Adapted | Sixteen 84-byte contexts, Main state preservation, nested calls/waits, activation/stop and same-frame activation compared; per-actor command budgets fail explicitly |
| Scene actor snapshots | Adapted | Four 104-byte render snapshots and six modal map roundtrips compared, including a hidden primary actor during Bijou-only play; byte-wrapped idle animation selectors, optional slots and script cursor retention preserved |
| Actor reflections | Adapted | Complete actor_animation_mirror.c animation contract compared: 23/18-entry mutable tables, wrapped Z, changed/retained animation offsets, fallbacks and companion aliases; dressing-room introduction and movement execute natively |
| Shared scene actor control | Adapted | All 40 functions in class_anchor_presets.c and scene_control_callbacks.c compared: anchors/modes, vectors, signed rectangles/corridors, aiming, background alignment/offsets and clone phases; later-activity runtime integration remains pending |
| Scene render presets | Adapted | All 77 functions in script_callbacks_render_presets.c and 28 required actor helpers compared; typed F4/task calls and 35 defined-zero Main callbacks, linked slots, indicators, graphics and live class selectors; full area/activity integration remains pending |
| Bank-2 actor and scene families | Adapted | All 120 functions in map_bank2_render_callbacks.c, map_bank2_scene_control.c, map_bank2_scene_presets.c and map_bank2_height_callbacks.c compared; 116 argument-free task targets, 70 Main contracts, panel stages, randomized motion, height latches and one full-word animation yield; complete Sunny Peak integration remains pending |
| Bank-3 late scene family | Adapted | All 31 functions in map_bank3_late_callbacks.c compared: 25 void presets, four animation callbacks and two Main modal callbacks; 27 F4 targets, song pan/tempo, mirror/affine scales and raw actor snapshot roundtrips; full area integration remains pending |
| Bank-3 main scene/activity family | Adapted | All 282 source functions accounted for across presets, source/spawn entries, pair board, phases/orbits, record text, chains and one shared condition; typed Main/task/animation contracts and helpers compared, including live links, byte wrapping and retained-register paths; complete activity progression and area integration remain pending |
| Bank-4 scene/path family | Adapted | All 483 map_bank4_callbacks.c functions and the shared screen-indicator family compared; presentation, jump stages/windows, byte counters, audio distance scaling and three path followers retain reviewed Main/task/animation/helper contracts; complete area story/activity and timed scene integration remain pending |
| Bank-5 scene/mechanism family | Adapted | All 207 map_bank5_callbacks.c functions and the four-function stairs/idle dependency family compared; grid choices, actor mirrors, lane states/outcomes, round timing and a typed VBlank callback; complete area progression and timed desktop integration remain pending |
| Bank-6 main/final scene families | Adapted | All 243 functions in map_bank6_callbacks.c and map_bank6_final_callbacks.c plus the complete neighbor-pair dependency compared; oscillators/orbits, projected motion, cursor/rectangles, target selection/restore, task patterns and typed helpers; full area integration remains pending |
| Bank-6 chain family | Adapted | All 12 map_bank6_chain_callbacks.c functions compared: three chain transforms, shared position, paired landing, containment/overlap, sine flight, fixed approach, wrapping and a live indicator; five Main, two argument-free task and seven animation contracts; full area/story and timed scene integration remain pending |
| Shared presentation families | Adapted | All 56 functions across nine bank-7 height/preset, region-motion, background/reset/scroll, window and tilemap-transfer families compared, including two existing modal roots; 28 Main, 40 task, six animation and six helper contracts, with overlaps; complete scene/activity and timed rendering integration remain pending |
| Bank-7 tile board and scene presets | Adapted | All 32 functions in tile_board_callbacks.c and scene_render_presets.c compared: five Main, 30 argument-free task contracts and one typed helper, with overlaps; complete sliding puzzle solved with normal directional input and retained by original Save/Continue; complete final area remains pending |
| Six/eight-choice rooms | Adapted | All 13 choice-family entries in scene_selection_callbacks.c compared: ten Main callbacks and three void task presets; shuffled banks, live actor states, weighted outcomes, four-byte record slots and counter availability; complete room/activity progression remains pending |
| Shared actor movement and turning | Partial | 23 related functions compared: wandering initialization/update/bounds/velocity, four animation contracts, coordinate/direction readers, facing, stepped turns and class sequences; 17 Main contracts and four reviewed task targets; optional bounds probes accept the reviewed prospective-region predicate and reject unknown targets; later-area integration remains pending |
| Shared actor geometry | Adapted | Eleven functions across eight source families compared: leader offsets/velocity, ten-byte projected-target approach, raw direction pairs, screen cells/sides, class coordinates, projected distance and render-pair setup; ten Main contracts and three task targets; full scene integration remains pending |
| Shared screen and shake effects | Adapted | Eight functions across five complete source families compared: display mode, window/OBJ flags, background anchors, shake setup/reset/update, raw render targets and amplitude decay; eight Main and eight argument-free task contracts; first-reconciliation integration exercised, full-game timing remains pending |
| Scene actor state and panels | Adapted | 45 functions across 16 source families plus one shared cross-bank condition compared: snapshots with and without execution-flag clearing, forward copy/swap, signed accelerations, class-target approach, panel presets, direction constructors and render-selection setup; 36 reviewed argument-free task contracts; complete scene integration remains pending |
| Actor render presets | Adapted | All 993 functions across ten base/linked/neighbor/class/secondary/render-setup/graphics/directional/extended/sequence families compared; task ABI, captured auxiliary arguments, live selectors, inactive mapped slots, byte-sized neighbors and direction sectors preserved; only the base family's defined-zero no-op also accepts Main; later-area integration remains pending |
| Map-choice scene control | Adapted | All 273 functions in map_choice_callbacks.c compared: 93 Main contracts, 253 argument-free task targets, five animation contracts and four slot helpers, including cursor skips/jumps, packed choices, motion, regions, phases and presets; complete area/activity integration remains pending |
| Map particles | Adapted | All four functions in map_particles.c and the alternate parameter selection compared; initialization, reset boundaries, live parameter reads, scaling and field60-only hiding; timed scene integration remains pending |
| Motion and collision | Adapted | Leader input/walk/run, action priority, map edge sampling/sliding, collision responses, event staging, follower routes, actor-position snapshots, transition animation/position selection and wrapped facing interpolation compared; full-game coverage remains pending |
| Text counters | Adapted | Three ROM template/palette banks, digit conversion, class aliases, addition/subtraction limits, delayed stepping and sound/yield callbacks compared; resources remain in the private ROM |
| Text tasks and dialog scripts | Adapted | Linked contexts, tiled glyphs, delays/fast advance, line/page scrolling, all 32 text commands, numeric text, icons, language variants and 27 typed animation callbacks compared; other indirect callbacks remain incomplete |
| Native save records | Adapted | Original payloads/checksum, SRAM identity initialization/repair, primary/backup group loading and grouped writes; private function comparisons |
| Local music and SaveGroup4 support | Adapted | All 29 functions in communication_record_callbacks.c and communication_record_selection.c compared: 28 Main callbacks and one void layout task, plus the prior payload classifier; resource songs/readers, render layout/aliases, affine selection, record numbering/depth, both SRAM copies and local/ROM selection; complete music activity and Cable Link transport remain outside this evidence |
| SaveGroup3 pairs and lists | Adapted | All 67 functions across seven field/catalog/pair/graphics/list/selection families compared, including 51 Main callbacks and reviewed task contracts; all three worn parts, Corsage sharing rejection, photo replacement/cancellation, Toss confirmation/rejection/empty-category behavior, title photo/Dress Up and original Save/Continue exercised; further gallery/settings and full desktop activity coverage remain pending |
| Clothing catalog and variations | Adapted | All 40 script_callbacks_pair_list.c functions accounted for and compared: 18 Main, 26 F4 and 13 helper contracts, with overlaps; six ROM-backed lists, five languages, ownership masks, prices, cursors and palettes; Sunny Boutique rejection, cancellation, two Antlers colors, Corsage and Kerchief purchases, sold-out selection and saved continuation run natively; other purchases and complete wardrobe coverage remain pending |
| Record selection menus | Adapted | All 18 record_selection.c functions compared, with two shared strided-transfer helpers; record rows, scrolling, text contexts, counters, markers and window registers preserved; three owned Gems entries and wrap navigation inspected in the SDK; complete Items/Gems and timed raster integration remain pending |
| Record messages and eligible-item count | Adapted | All 12 game_record_messages.c functions and the save_counter_prefix.s Main51 leaf compared; five-language text, byte-index wrap, live aliases, eleven void animation presets and the excluded final counter byte; the Clubhouse desk's empty-inventory greeting completes in the SDK; populated activity branches remain pending |
| Counter displays and crafting selection | Adapted | All 23 functions across game_counter_display.c, game_record_selection.c and game_record_transactions.c compared; 18 Main, 14 argument-free task contracts and five typed helpers; cached/random candidates, five-language graphics, palette channels, raw selectors and counter costs/refunds; paid artisan Pooie Earrings and delivery tested; manual crafting, resale and other recipes remain separate |
| Shared Items/Gems selection | Adapted | All 14 script_callback_actor_map10_5.c functions and three complete key-mark/list/panel dependency families compared; modal actor snapshots, byte cursors, scrolling, five-language text pages, empty task allocations and debug-key priority preserved; empty entry/return and a one-item training-seat SDK view/save/reload run; multi-item inventories and complete activity integration remain pending |
| Gameplay record slots and counters | Adapted | Nine allocation/query/consume/counter functions compared, including packed fields, result layouts, wrap/clamp behavior and three allocation callbacks; batches retain zero/full capacity branches and original animation timing; complete inventory UI and save integration remain pending |
| Rock use, counters and captions | Adapted | All 18 functions in game_record_use.c, script_callbacks_game_record_counter.c and record_display.c compared: 15 Main, 14 F4 contracts and two direct helpers; native SDK Rubrub identifies Banana Stone, Crystal and Ammonite, retains partial rocks and enforces the love-meter strength limit; complete crafting and store branches remain pending |
| Status counters and stage sequence | Adapted | All 13 functions in map10_4_counter_sequence.c compared, including the complete 16-bit counter domain, five tile banks, windows, stage/scroll graphics and sequence phases; nine Main and twelve argument-free task contracts; later-stage live integration remains pending |
| Word-choice dialogs | Adapted | Nine visible text contexts after filtering up to 255 candidates, grid navigation/highlights, disabled rows, confirmation/cancellation, requested selection, delayed close, display reset and Main result preservation compared; full-game integration remains pending |
| Ham-Chat book | Adapted | Acquisition, learned-word order, five language permutations, category/page selection, paired text contexts, counters, markers and cleanup compared; initial book display and return run natively; manual book navigation remains unverified |
| Destination map | Adapted | Completion/visibility flags, current/destination markers, backgrounds, direction priority and sequential unlocks compared; native input selects Clubhouse/Sunny Peak in both directions and enters Sunny Peak |
| Main script control/resources | Partial | Fetch/yield/wait, branches, memory/decompression, forward byte/halfword pointer copies, maps, saves, task/actor/indirect setup, word-choice dialogs, collision scenes/routes, random values, core reset, choice flags, fades and typed callbacks; unsupported opcodes fail explicitly |
| Scene menus | Partial | Menu/record selection, name editing, glyph blitting, keyboard banks, panel entry/exit/depth, language snapshots and input dialogs compared; Select status, Items/Gems entry/return, one-item read/navigation and End Game confirmation connected; multi-item inventories and later activity panels remain incomplete |
| Native boot/title | Partial | Real SDL window reaches the language selector with music; 9,000 clocked native updates completed; 88 Main-only, 180 idle and 9,000 component-schedule updates compared; complete boot/title/gameplay parity remains unverified |
| New game, movement, in-game save/load | Partial | Versioned 281,135-update route from erased SRAM through the first Sunny Peak story, Lovedove and three loves, Boss's announcement, Clubhouse return and original save/Continue; focused reunion and optional Rubrub continuations retained. Full optional activities and complete desktop story coverage remain pending |
| Saved story continuations | Partial | Input-only SDK/host stages continue through Sandy Bay, Treasure Island, Fun Land, Haunted House, jungle wedding, Sunny Peak's tenth reconciliation, tower puzzle, Spat's defeat, complete credits and postgame control, followed by Panda's bridge and Rainbow Key; original Save/Continue, exact SRAM equality and fresh reload checked at stage boundaries; full desktop progression and optional completion remain pending |
| Music and effects | Partial | Native sequencer/mixer/PSG/FIFO/PWM connected to SDL; 600 real-time updates with no device sample shortages and user-confirmed clear menu music; complete game-output parity, filtering and trigger quirks remain pending |
| Complete single-player playthrough | Pending | Story, secondary activities and minigames |
| Cable Link | Out of first release | No local/network link emulation |
| Public release | Pending | Native gameplay, complete coverage and source-provenance review |

Play runs compiled C game logic with native video/audio devices. A missing callback,
unsupported CPU-transfer queue, communication update or IRQ target stops execution
and opens the menu with a private runtime-error.log diagnostic. It never switches
to guest instruction execution. The language selector and menu music establish a
native desktop boot milestone, not a completed title/story/playthrough.

The current `tests/scenarios/sunny_peak.json` route starts from erased SRAM and
covers the basic lessons, Bijou's rescue, Pepper/Oxnard, Harmony, the photo-room
training-seat request/delivery, Sesam-E, the cave maze, Arnie's reunion and Hamigos.
It explicitly completes the missing-partner conversation (flag 0x187) before the
two Hamigos confrontations with Spat and Pakapaka's Libert-T lesson. Lovedove
restores the third love. The pair returns to Clubhouse, completes Boss's room
announcement and uses original End Game/Continue again. The final checkpoint is
map 1:1, x=144.5, z=256.5, 25 words, two seeds, three loves and Bijou in follower
slot 13. The scenario passes 281,135 native updates and 293,049 host-session checks,
including exact SRAM-file persistence and a fresh host continuation. The runtime frame counter
resets at the title, while the runner's total remains cumulative. Inputs and scalar
checkpoints are versioned; no ROM, resource, save or captured state is included.

`opening.json` retains the first-reconciliation regression. The shorter
`sunny_peak_reunion.json` starts from an original post-Libert-T SRAM checkpoint
(map 2:20, 24 words, two loves) and covers Lovedove, Clubhouse and saved continuation
in 21,978 updates. The optional `sunny_peak_rubrub.json` starts from original
cart-area SRAM (23 words, two loves) and passes 11,469 SDK updates plus 13,692 host
checks through the cart ride, Rubrub lesson and save/reload. The story route skips
that optional ride, so its word count describes a different route. Full Sunny Peak
optional activities and later visits remain unverified.

`sandy_bay.json` continues from the exact saved Clubhouse endpoint above. Boss's
Hamha conversation sets pending destination flag 0x183; the original map unlock
consumes it and enables flag 0x43. Selection 4 enters Sandy Bay (map bank 4; bank 3
is a separate area). The 191,915-update route retains the first arrival and map 4:2
beach conversation/save. It continues west to the swimming lesson and word 26,
then the Lookie conversation in map 4:4 and changed entrance scene. Tinglie raises
the word count to 27; using it at the cart obtains the ball. The team receives it,
then the six-rally practice and twelve-rally challenge finish with zero errors.
The practice advances team phase 0x83 to 3; the challenge awards twenty seeds.
The route then learns Blanko at the shop and Ta-dah at the entrance. Lookie reports
enable Barrette's request and Spat's shore scene, which teaches Gasp-P. The pair
returns through the original destination map to alert Seamore in Clubhouse; flag
0x1AB changes from false to true. Back in Sandy Bay, Seamore rescues Barrette,
Trust-T is learned and the fourth love is restored. Hif-hif retrieves the bottle
message and then collects the wooden board. Its Items entry is rendered and
selected with ordinary keyboard inputs. Original End Game/Continue restores
map 4:5 at x=160.5, z=48, 31 words,
22 seeds, four loves and Bijou. Read-only flag and control-byte assertions establish
the unlocks, words, ball delivery, rally results, rescue prerequisite, reconciliation
and bottle/board acquisition. The unchanged 138,742-update prefix previously passed
the SDK replay and 145,216 host-session checks. The extension passes a separate
6,103-update SDK replay and 8,209 host-session checks, including SRAM-file
persistence and fresh continuation; board flags 0xA6/0x194 remain set.
The prior combined 143,084-update SDK replay passed in 216.04 seconds; all of its
inputs and checkpoints remain unchanged in the extension.

The continuation completes and saves the ten-note melody, opens the hidden cache,
collects its gem with Hif-hif and reaches the Captain through Digdig. Fussfuss enables
the ferry and anniversary continuation. A shop interaction learns one additional
word, then an explicit Yep-P choice takes the ferry to Treasure Island. Pashmina's
introduction, Penelope's scarf scene and its Hif-hif pickup precede the music hall.
The 191,915-update SDK prerequisite passes again on the current core in 295.419
seconds. Its exported 32 KiB SRAM matches the earlier focused music-hall file
byte for byte. A focused
50,592-update replay from the wooden-board SRAM passes in 79.50 seconds, followed
by 54,387 host-session checks in 84.18 seconds. Original save/Continue and a fresh
disk session retain map 4:13 at x=128.5, z=196.5 with 32 words, 22 seeds, four loves
and Bijou, including melody/cache/gem flags and Captain/ferry/scarf phases.
The host-written 32 KiB SRAM equals the SDK export, and the input saves are unchanged.
The current core also passes the full erased-SRAM Sunny Peak route: 281,135
updates in 464.68 seconds. A separate SDK process reloads the host-written island
SRAM and rechecks the saved flags, scene controls, map, position and counters.
`treasure_island.json` preserves the earlier scenario as a separate prerequisite
and starts from its saved music hall in a fresh native process. It follows the
music demonstration's Yep-P choice, guard access, dressing-room reflection and
Stickie/Heyhoo impostor scene. The first lighthouse encounter enables the elder's
two Heyhoo conversations, Tuggie and its book entry. Tuggie then exposes the
lighthouse impostor, restores the fifth love and teaches the follow-up word.
A 44,850-update replay from the current prerequisite passes in 74.35 seconds and
48,522 host-session checks pass in 79.54 seconds. Original save/Continue and a fresh SDK process
retain map 4:12 at x=128.5, z=128.5 with 34 words, 22 seeds, five loves and Bijou,
including flags 0x72/0x5B/0x138/0x182 and story phase 0x34=6. The host-written
32 KiB SRAM equals the SDK export. The fresh reload also opens Status through
native input; its five-love counters are inspected through native PPU output.
The two scenarios do not claim one uninterrupted deterministic button recording:
verbatim concatenation after an in-process Continue diverges at room navigation.
Complete Sandy Bay activities, later visits and full desktop progression remain unverified.
The input SRAM is a private output of the erased-SRAM
Sunny Peak route; the versioned continuation contains only buttons and scalar checkpoints.

`fun_land.json` continues from the saved fifth reconciliation in another fresh
native process. It returns by ferry, uses Digdig at Sandy Bay's entrance, returns
to Clubhouse for the original destination unlock and Boss lesson, then enters
Fun Land. Scrub and its book return, trash acquisition and the musical-note
challenge run through ordinary input. A read-only class checkpoint asserts 630
points, within the original green-marble reward tier. Flags 0xAB/0x127 verify the
award, and the first original Save/Continue retains map 3:4 at x=232.5, z=216.5
with 36 words, 19 seeds, five loves and Bijou. Without another process restart,
the route learns Blash-T, follows the power failure, rescues the operator with
Pakapaka, requests help and sends Ham-Bot with Heyhoo. Chukchuk consumes the
trash against Spat; Scrub-E repairs the robot and teaches Tiptop. The sixth
reconciliation and Clubhouse arrival unlock the next destination. A second
original Save/Continue retains map 1:1 at x=144.5, z=192.5 with 39 words, 19 seeds,
six loves and Bijou, flags 0x44/0x69/0x62/0xAB/0x127 set, 0x184/0xBB cleared and
controls 0x8F=2/0x90=6. The 85,539-update scenario has 36 semantic checkpoints,
retains every earlier green-marble check and contains only literal buttons and
scalar assertions. There is no adaptive controller, game data or state injection.
The complete SDK replay passes in 138.71 seconds; 90,912 host-session checks pass
in 140.82 seconds. Their entire 32 KiB SRAM images match byte for byte. A separate
2,103-update SDK reload and Status inspection passes in 2.589 seconds with the
same saved flags, controls and counters. Later Sunny Peak visits, remaining activities and complete desktop progression
remain pending.

`haunted_house.json` starts from the saved sixth reconciliation. Its 204,853
updates and 68 scalar checkpoints retain the entrance scene, Hif-hif seed pickup,
Digdig passage, two Blash-T price reductions and purchase of the ID card for five
seeds. Ta-dah opens the entrance; Heyhoo at the reception desk teaches Thump-P,
and another Ta-dah earns the bronze card. All earlier bronze-card and original
Save/Continue checks are retained. The route adds Go-P, the false mirror,
three ghost captures, the silver card, the plank passage, Hamlift access,
Hamspar and Nopookie. Hamtaro's capture passes an original Save/Continue with
Bijou alone; she learns Nok-nok and Pushie, opens the prison and uses Heyhoo
and Libert-T to complete the rescue. The seventh reconciliation and Flipflop
survive another original Save/Continue. All 142,196 preceding updates and
43 checkpoints remain unchanged. A failed tile challenge now returns normally;
the retry clears all four tile counters, wins the gold card and survives another
Save/Continue. The upper floor adds Perksie, the hidden wall, three quiz answers
and Flipflop/Nopookie against Spat. Clubhouse unlocks the jungle and the final
original Save/Continue retains map 1:1, x=144.5, z=192.5, 47 words, 17 seeds,
seven loves and both characters. Flags 0x45/0xB8/0x7B/0x7D/0x1B2/0x1B3/0x10
remain set, 0x185/0x15B cleared and controls 0x91=2, 0x90=6, 0xD0=7.
The complete composed SDK route passes in 370.114 seconds. Current-code host
coverage chains the 163,285-update gold prefix (171,343 checks, 342.02 seconds)
with a fresh 43,329-update quiz/jungle session (46,968 checks, 106.50 seconds).
The full SDK export, final host file and input-only scout match across all 32 KiB.
A separate 2,103-update SDK reload/Status inspection passes in 2.598 seconds.
This is SDK and chained host-session evidence; optional activities and complete
desktop progression remain pending. Inputs and scalar assertions contain no
game resources or state injection.

`jungle_hat.json` continues from that saved jungle unlock in a fresh process.
Its 102,159 input updates and 19 checkpoints cover Wait-Q, Hampact, Sunny Peak's
robbery and hat request, and the required Hamteam lesson from Stan in Fun Land.
The route checks that the trap remains inactive without Hamteam, then uses it
with Bog and follows Cappy around the jungle until the cage drops. Bluhoo and
the original Yep-P choice return to Sunny Peak; the hat scene teaches Hamboree
and restores the eighth love. Original Save/Continue retains map 2:5 at x=80.5,
z=176.5, 51 words, 17 seeds and both characters, flags 0x55/0x8C/0x71/0x8F/
0x1A0/0x1A7/0x1AC set and controls 0x6D=4, 0x6F=3, 0xC9=8.
The SDK replay passes in 166.08 seconds; the same complete input stream passes
107,364 host-session checks in 173.37 seconds. All 32 KiB match the SDK and
input-only scout. A separate 2,103-update SDK reload/Status inspection passes
in 2.766 seconds. The preceding Haunted House scenario and its SRAM input remain
unchanged.

`jungle_wedding.json` continues from the saved eighth reconciliation in a fresh
process. Its 91,911 input updates and 21 checkpoints cover the thermal egg,
Clingie's vines, Jingle's Thank-Q lesson, the first ceremony, the rubber-band
purchase and Hulahula. Putput crafts the slingshot from the actual materials;
Hamscope's tutorial and the aimed beehive shot precede Hif-hif's red-marble pickup.
The second ceremony restores the ninth love and teaches Vast-T. Original
Save/Continue retains map 6:9 at x=120.5, z=192.5, 56 words, 12 seeds and Bijou;
flags 0xC4/0x186/0x6C remain set and controls 0x8C=2, 0xB0=5, 0xB2=2,
0xC5=2, 0xC6=1, 0xC7=2, 0x1A0=2. The complete SDK replay passes in 161.24
seconds and the complete host replay passes 97,365 checks in 170.45 seconds.
All 32 KiB match the input-only scout; another 2,103-update SDK reload/Status
inspection passes in 2.766 seconds. The prior SRAM input remains unchanged.
The wedding temporarily consumes the red marble; a later couple conversation
returns it. The earlier Sandy Bay hidden-cache check (flag 0x143) was mislabeled
as a blue marble: the actual blue-marble prerequisite is 0xAA, obtained with
Digdig in map 4:3. That pickup, the temple/workshop and complete desktop
playthrough coverage remain outside this ninth-stage proof.

`sunny_temple.json` continues from that saved ninth reconciliation in a fresh
process. It obtains the couple's returned red marble and the actual blue marble
with Digdig on Sandy Bay's west beach, then asserts all three inventory flags.
The earlier oil purchase and open lever remain intact. Oil applied to the mine
cart, Gogo and the Rubrub lesson reach the temple. Putput consumes flags
0xAA/0xAB/0xAC in order and sets 0x11B/0x11A/0x119; the spoon sets 0xAD/0x11C.
The route returns through the previously failing cave exit, examines the tower
gate, and uses Ta-dah in the inventor-couple story. Its tenth reconciliation and
new lesson survive original Save/Continue: map 6:6 at x=112.5, z=88.5, 58 words,
12 seeds and Bijou, with 0xAD cleared, 0x119/0x11A/0x11B/0x11C set and
control 0x8C=4. All 89,944 input updates and 21 checkpoints pass in 215.79 seconds;
the same full stream passes 95,059 host-session checks in 176.98 seconds.
All 32 KiB match SDK, host and scout. A separate 2,103-update SDK reload/Status
inspection passes in 2.701 seconds. The input SRAM remains unchanged; no state
injection is used. The following tower stage is described below.

`spat_tower.json` continues from that tenth save. It launches the inventor's
catapult with Gogo/Stickie, solves the sliding puzzle with normal directional
input, advances the guard/doorway conversations through Perksie, Passchat,
Libert-T and Yep-P, and enters Spat's first battle phase. A normal Hamscope shot
triggers the original retreat. The return to Clubhouse uses Chukchuk and Hamteam
with Harmony to upgrade Hamscope. Original Save/Continue retains map 1:2 at
x=184.5, z=64.5, 58 words, 12 seeds, ten loves and Bijou, with flags
0xCA/0x18C/0x1B9/0x1AD/0x1B7 set and controls 0xCF=4/0xD1=1.
All 58,579 input updates and 12 checkpoints pass in 105.19 seconds; the same
full stream passes 62,902 host-session checks in 97.81 seconds. SDK, host and
scout match all 32 KiB. A separate 2,103-update SDK reload/Status inspection
passes in 2.684 seconds. The prior save is unchanged. The following battle stage
is described below; remaining activities and full desktop playthrough remain pending.

`spat_battle.json` resumes from the saved Hamscope upgrade. Normal movement and
shots advance both battle phases, including a shot into Spat's open mouth. The
ending learns the 59th word, runs the complete credits in map 10:11, returns to
the tower and restores postgame control. Original Save/Continue retains map 7:3
at x=116.5, z=160.5, 59 words, 12 seeds, ten loves and Bijou, flags
0xCA/0x190/0x1AD set, 0x2A8 cleared and controls 0xCF=5/0xD1=4.
All 38,099 input updates and 11 checkpoints pass in 58.15 seconds; the same
full stream passes 40,937 host-session checks in 60.53 seconds. SDK, host and
scout match all 32 KiB. A separate 2,103-update SDK reload/Status inspection
passes in 2.758 seconds. The prior save is unchanged. This establishes a saved
native main-story ending along the tested stage chain. At this boundary, 27 words,
optional activities and the full desktop route remain outside this evidence.

`sunny_optional.json` continues from that saved ending and learns Blushie through
Stickie, Bestest from the reunited eastern pair and Bloat-T after the sunflower
reunion. Its first eight checkpoints and intermediate Save/Continue are retained.
The extension learns Tran-Q from Boss before accepting the Clubhouse board.
It repeats Boo Manor's ghost challenge, collects its board, crosses with Putput
and recovers the board with Offdoff. Panda receives all three boards and the
strap; returning to the cave announces completion, and Panda then teaches Hamtast.
The completed bridge leads to the Rainbow Key, knocked down with Tack-Q and
collected with Hif-hif. The cave junction changes its control 0x1A0 from 1 to 2
on the left route while staying in map 2:9; a separate map transition is not
required for this traversal.

Original Save/Continue retains map 2:15 at x=152.5, z=120.5, 64 words, 12 seeds,
ten loves and Bijou. Flags 0x82/0x88/0x89/0x8A/0x99/0xBD/0x190/0x1D8 are set,
the four delivered-material flags 0xA5..0xA8 are clear, and controls 0x7D=8 and
0x1AF=2 retain the completed bridge and key pickup.
That bridge stage's 125,864 input updates and 24 checkpoints pass in 192.02
seconds; the same full stream passes 132,137 host-session checks in 197.58 seconds. SDK, host and
scout match all 32 KiB. A separate 2,103-update SDK reload/Status inspection
passes in 2.654 seconds, and the source ending save is unchanged. No core changes
were needed. The 22 remaining words, other activities and full desktop progression
remain pending; this does not establish complete optional-game coverage or a full
love meter.

The next extension obtains Condor's Wings by greeting the feather and collecting
the dropped resource with Hif-hif. Seedric's sand bath, map 2:5, supplies Great
Springs and two additional rocks through Digdig. The top hole in map 2:4 leads
to this bath. The route then completes the Clubhouse record desk's greeting with
no eligible records and saves normally. Original Save/Continue retains map 1:6
at x=96.5, z=64.5, 64 words, 12 seeds, eight rocks, ten loves and Bijou, with
song flags 0x141/0x142 and greeting flag 0x101 set. All prior bridge inputs and
checkpoints remain unchanged. The expanded 153,436-update, 32-checkpoint SDK
replay passes in 291.86 seconds. The full host stream passes 160,567 checks in
279.29 seconds. SDK, host and scout match all 32 KiB; a separate 2,103-update
SDK reload/Status inspection passes in 2.679 seconds. Both earlier source saves
remain unchanged. At that stage the newly collected rocks are still unidentified.

The latest extension enters Rubrub with those eight records and identifies a
Banana Stone, then the bath's Crystal and Ammonite. The populated selector checks
its first-record boundary and forward navigation. A further rock retains its
three partial polishes; the next attempt is rejected at the current love-meter
strength without consuming it. The exit summary, owned Gems panel, wrap to Crystal,
Ammonite/Banana Stone entries and return to the same scene complete normally.
Original Save/Continue retains map 1:4 at x=80.5, z=80.5, 64 words, 12 seeds,
five rocks, ten loves and Bijou, with both songs and the prior ending/key retained.
All 153,436 prior updates and 32 checkpoints remain unchanged. The expanded
177,048-update, 48-checkpoint SDK replay passes in 296.31 seconds. The same full
host stream passes 184,866 checks in 300.02 seconds. SDK, host and scout match
all 32 KiB. A separate 2,103-update SDK reload/Status inspection passes in 2.826
seconds and reads the three gem counters and eight packed record counts: each
gem is retained once, the partial rock has one polish remaining and the four
untouched rocks retain four each. Earlier source profiles remain unchanged.
The five remaining rocks need a higher love-meter strength; other optional
activities, 22 words, populated crafting/store branches and full desktop
progression remain pending.

The separate `postgame.json` chapter starts from that verified Sunny save and
collects both cave rocks through Hif-hif and Tack-Q. Rubrub identifies two Glass
Beads. Two normal Digdig visits provide ten more seeds. Original Save/Continue
retains the same Clubhouse room, position, 64 words, five rocks, ten loves and
Bijou, now with 22 seeds, two Glass Beads and one additional partial common-rock
polish. The 49,102-update, 11-checkpoint chapter asserts collection counters and
remaining-polish fields as well as the previous story flags and scalar state.
It passes the SDK in 91.19 seconds and 52,513 host-session checks in 92.93 seconds;
SDK, host and scout match all 32 KiB. A separate 2,103-update SDK reload/Status
inspection passes in 2.734 seconds and retains all checked counters and slots.
The source Sunny save remains unchanged. No native functional sources changed.
The preceding 177,048-update scenario remains unchanged; unchanged native code
can reuse its verified save for focused extensions. This preserves every earlier
scenario and the full comparison suite. At that checkpoint accessory crafting,
the remaining words, higher love-meter strength and complete desktop progression
remain pending.

The continuation obtains the Pooie Earrings recipe from the Wildwoods palace,
commissions the accessory at the Clubhouse and checks the exact costs: twenty
seeds, one Ammonite, one Banana Stone and two Glass Beads. The artisan's branch
stores the completed accessory without calling the manual-crafting random-outcome
handler. A further workshop interaction retains it. Ta-dah at the palace removes
that accessory and grants the Star Key. The key opens Spat Tower's cave door; the
complete scene teaches Swellie and increases rescued loves to eleven. Original
Save/Continue returns to map 9:1 at x=104.5, z=96.5 with 65 words, two seeds,
five rocks, eleven loves and Bijou. The three spent gem counters remain zero;
Crystal remains one and the undiscovered counter remains 255. All earlier
49,102 updates and eleven checkpoints remain in the expanded chapter. The full
102,283-update chapter passes twenty SDK checkpoints in 265.29 seconds and
107,506 host-session checks in 271.68 seconds after the VCOUNT changes. All 32 KiB
of SDK, host and scout SRAM match; a fresh SDK reload and Status inspection pass in 3.163 seconds and
retain the checked flags, counters and rock slots. The source profiles remain
unchanged. All 65 learned identifiers are unique and match their learned flags;
none required by the Sunny Peak destination indicator is missing. Manual crafting,
the other twenty-one words, the full love meter and full desktop progression
remain pending.

`wardrobe.json` starts from that verified Star Key save in a fresh process. It
enters Sunny Boutique, rejects an unaffordable Antlers purchase and cancels back
to the room. Four normal Digdig seed pickups raise the balance from two to
twenty-two. Buying brown Antlers spends ten seeds; selecting the owned color
shows Sold Out without spending again. Buying white spends ten more. Down/Up
navigation displays Corsage and returns to Antlers without changing ownership
or the balance. The Outfit counter advances once per purchased color, from
zero to two; the Antlers ownership byte changes from 63 to 191 to 255, while
the next category remains unchanged.

The original 37,274-update purchase route and thirteen semantic checkpoints
remain unchanged in the expanded chapter. Existing postgame and Sunny optional
chapters also remain unchanged.

The extension enters Clubhouse's photo room, dresses Hamtaro in brown Antlers
and Bijou in white, and saves the first photo with setting 1. Removing Hamtaro's
worn hat keeps ownership and the stored photo intact. End Game and the title's
Dress Up option reload both original colors. The extension then swaps the
colors, saves setting 2 in slot six, declines erasing the first photo, returns
through Continue and repeats original Save/Continue. Both photos, four empty
slots, both purchased colors and the earlier story progress are asserted.
The title's scene counters are temporarily zero while outside the story; the
world save restores 65 words and eleven loves. Native PPU output confirms the
two outfits and that the first photo appears behind the title menu.

A further 45,123 updates retain all 73,776 preceding updates and 29 checkpoints,
then add 22 checkpoints for Sunny's item/body purchases and related wardrobe
operations. Two ordinary seed pickups raise the balance from two to twelve.
Declining Corsage confirmation retains both seeds and ownership; purchasing it
costs three. Kerchief costs five, leaving four seeds and four owned variants.
Hamtaro wears white Antlers, the Corsage and red Kerchief together. Bijou cannot
select the Corsage while Hamtaro wears that same variant.

Cancelling after accepting a photo replacement keeps the original record. Saving
the replacement puts all three Hamtaro pieces and Bijou's brown Antlers into slot
one, retaining setting 1, slot six and four unused slots. Declining Toss keeps
ownership; confirming discards only the Corsage and reduces the Outfit counter
from four to three. Toss on the empty category cannot decrement it again. The
stored photo retains the discarded Corsage, while the worn selection clears it.
Remove clears Hamtaro's worn hat and Kerchief without changing purchases or photos.
Title Dress Up renders the saved photo but skips its unowned item when rebuilding
the selectable wardrobe. Original Save/Continue restores the story and all records.

All 118,899 updates and 51 checkpoints pass the SDK in 211.57 seconds, followed
by 123,990 host-session checks in 214.76 seconds. SDK, host and input-only scout
SRAM agree across all 32 KiB. A fresh SDK process passes 2,103 updates in
3.019 seconds, including Status, both photos, the four empty slots and the earlier
gem/rock assertions. The endpoint is map 1:3 at x=144.5, z=48.5 with 65 words,
four seeds, five rocks, eleven loves, Bijou, both Antlers colors and Kerchief.
The Outfit counter is three; Corsage ownership is cleared while its image remains
stored in photo one. The source profiles stay unchanged. Native source and binary
hashes are identical to the complete 5,793,256-check HBlank/window boundary, whose
proof remains applicable to this input/checkpoint extension.

Further clothing purchases, gallery operations/settings and full desktop activity
coverage remain pending.

Host-session replays write private 32 KiB SRAM files, verify their bytes, stop the
session and start a new one to compare map, position, words, seeds, partner and
rescued loves. Focused input saves are read-only; all writes use disposable profiles.
The actual Windows executable was previously reviewed with the 21-word Clubhouse
save, including visible status counters, keyboard menus, End Game, exact backup
retention and relaunch/Continue. That bounded desktop evidence does not establish
the new full desktop story route. Book pages, dialogs and the new story scenes have
also been inspected through native PPU output. Game captures stay outside Git.

Two original bus contracts are retained at their reviewed calling sites. Collision
record-index LDRH reads above 0x0FFFFFFF and at unassigned I/O above 0x040003FF
use the ROM-prefetched Thumb halfword, including odd-address rotation, and
terminate before invalid record fields. The original temple-cart return to map
2:7 retains 0x0480011C as its first animation-derived list entry. The local
index adapter now covers that unassigned-register read; extended memory-control
and debug-enable registers retain explicit faults. Independent loads cover all
three sites, even/odd boundaries and every byte-sized render count. The
scene-parameter copy in ConfigureRenderXZCollisionScene uses 32 forward LDMIA word
loads, with word alignment, overlap order and SRAM byte replication. Its reserved
bus reads use the halfword prefetched at 0x08006690 in both word lanes; a native
return from the third reconciliation had otherwise stopped on a leftover script
variable used as the optional parameter pointer. The tile-challenge loss also
resolves a leftover scalar to BIOS address 0x612. The BIOS-free original replay
returns normally and copies its protected post-service prefetch word, 0xE3A02004.
That reviewed scene-copy path models the word locally, including alignment and
the BIOS/reserved boundary; it does not read or distribute firmware. Independent
mGBA LDM responses cover both contracts. Ordinary memory access and unreviewed
regions keep their explicit faults. These are native calling-site contracts, not
CPU execution or a runtime emulator fallback.

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

The counter/crafting selection implementation uses record_selection.c. Its three
complete source families retain the 20-byte counter and 24-byte record descriptor
layouts, resource pointers in ROM, five-language label tables, two scrolling rows,
three candidate palettes and status tiles. Candidate generation preserves the
original PRNG draws, duplicate rejection, special record reservation and cache
reuse. Transactions keep the separate requirements check and cost writes: key zero
is skipped by the former but charged by the latter, repeated keys are not summed
before checking, and byte-sized costs/refunds wrap in the original order.

The 42,501 focused comparisons cover every source entry and reviewed calling
contract, all raw byte selectors, five-language record graphics, palette marker
bits, overlapping counter/list/selection writes and forward glyph copies. The
graphics helper's original three-pointer local array and candidate buffer's
80-entry capacity have explicit failure guards; a non-terminating random selection
also fails after its budget. Those invalid-state guards do not establish original
stack-overrun parity. Counter 31 has no display descriptor; the resource-rendering
fixtures use the recovered 0..30 catalog. Function checks prepare the workshop;
they do not establish its full player-facing activity or timed desktop behavior.

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
4,200,342 checks with the independent BIOS, PPU and floating-bus oracles, including separately
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
The typed HBlank dispatcher resolves all 19 original selectors through the live
guest table, including eighteen distinct targets. The complete window, stepped,
circle, alternating-band, scrolled-band and wave families share raster.c. Four
window/wave Main contracts, two argument-free F4 contracts and two direct helpers
complete a 26-function comparison batch. Argument-dependent callbacks have no F4
binding; the phase-reset helper has no invented script contract. Other callback
addresses fail explicitly.

The window transition phase remains separate from the wave phase. Configuration
retains the original profile update before allocation, overlapping guest reads,
DMA setup, completion thresholds and retained pointer after free. The radius
helper uses bounded host scratch for the source's radius-239/240 overrun, preserving
guest writes. Positive radii leave source scratch byte zero unwritten; runtime
supplies zero, while comparisons also supply explicit nonzero retained values.
Only this helper discards the original pre-allocation byte stores into protected
BIOS memory. A nonterminating radius-255 copy and unspecified configure modes
above eight fail explicitly. No arbitrary ARM stack-history identity is claimed.

Forty-six synthetic clocked frames compare every visible line's scroll, window
and blend registers with original callbacks, then all pixels with an independent
mGBA PPU. They exercise HBlank DMA before the IRQ, frame wrap, changing stepped
widths, completion and combined VCOUNT/window effects. Analytical native pixel
tests retain narrow flipped-tile windows where mGBA 0.10.5 has a clipping defect.
These checks establish the selected clock/register/pixel contracts, without
modelling original ARM instruction latency or DMA bus contention. Main save-load
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

1. Preserve the input-only story chain through the ten tested story reconciliations, Spat's
   defeat, complete credits and saved postgame. Keep its actual temple/three-marble
   prerequisites and semantic battle checkpoints rather than relying on capture labels.
2. Keep full routes reproducible from erased SRAM. Use focused private SRAM
   continuations while implementing, then verify original saves and fresh host
   sessions at a coherent integration boundary.
3. Extend actual-window coverage of Sunny Peak and its Items/Gems/photo/Rubrub UI.
   First-visit story evidence does not establish every optional activity or later visit.
4. Complete remaining secondary activities, words and minigames. Compare
   canonical RGBA, state, SRAM and pre-device PCM alongside timed raster/IRQ/audio.
5. Verify physical controllers and the packaged app on Windows without development
   tools. Public CI stays ROM-free; private reference checks take user paths.

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

The read-only reference also provides `tests/cold-load/spanish-montak-reload.txt`
and `tests/fixtures/montak.json`: 1,224,925 original frames and 443 checkpoints per
primary/backup branch through ten story reconciliations, credits and selected postgame
activities. Use this route to plan the remaining areas and compare original outcomes.
It begins from a Spanish MonTak SRAM checkpoint; its prerequisite, language, timing
and VCOUNT-dependent randomness must be checked when adapting native inputs. These
are mGBA reference results. The Spanish prerequisite now has native input-only
coverage through four fresh SRAM-only runtimes, ending with 13 words and 15 seeds;
the last segment also passes host-file persistence and fresh continuation.
Verbatim story timings drift at map/vine transitions. A normal-input approach
adjustment passes the first Spanish reconciliation and saved continuation, then
the training-seat route needs another movement adjustment. External text schedules
are accepted directly by run_scenario.py; their labels observe native state and
do not assert reference parity. Full native execution and the remaining optional
content still require separate evidence.

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

The complete 22-function actor_class_animation_pairs.c family composes existing
presets for paired/triple actors, neighbor slots and indicators. Separate source
calls reload live class selectors. Flag 0x17 is consumed by the next animation
setter, transferring its bit to that render slot while preserving other flags.
The original functions are void/F4 targets; Main must not invent an argument count.
All 22 entries, task calls, inactive byte selectors, overlapping selector writes
and flag consumption are compared. This enables Boss's post-Sunny Peak room
announcement in the native story route.

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

Forty-five scene actor/state/panel functions across 16 source families preserve
the nine-byte class-coordinate approach state, fixed-point/signed boundaries,
forward copy/swap order, two-slot backup/restore, screen-pair presets and direction
constructors. One additional shared bank-3 value condition supports the bank-2
scripts without claiming the rest of bank 3. Nine gameplay-record functions retain
packed counters, the twelve-byte query result and original underflow/clamp behavior.
The three-function scanline-wave family has typed Main/task/HBlank adapters with
all-line threshold and frame-end phase comparisons. Its clocked alpha and scroll
pixels are included in the complete HBlank/window batch above.

The render-selection Main callback captures all three argument bytes and signed
class coordinates before resetting a slot. It selects either the player's ROM
table entry or the secondary animation, then initializes graphics slot 6 and four
linked overlays in source order. Selection 255 preserves the byte-wrapped zero
mode; inactive mapped records remain writable. Exhaustive byte selections,
nonzero secondary values, signed coordinates and arguments overlapping the reset
slot are compared. This argument-dependent callback has no F4 contract.

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

The complete 483-function bank-4 family is grouped by responsibility:
bank4_presets.c contains 428 presentation presets; bank4.c covers jump preparation,
counter display/progression, audio, projected bounds and other scene state;
bank4_path.c contains the three path followers and their eight helpers. A separate
shared indicator dependency changes only the original indicator fields. Together
they expose 220 reviewed Main contracts, 445 argument-free task targets and 16
animation contracts, with 16 typed helpers. These counts overlap by calling
contract and are not a completion percentage.

Jump staging preserves the clamped stage, rejection-based PRNG choice, mask
updates and original double-precision acceleration calculation. Counter drawing
retains live counts and byte-wrapped render indices; counter completion preserves
its sentinel and reset order. Distance audio uses wrapped signed products and
halfword volume inputs. Background return reloads saved coordinates after reset
and again after the first step write, including aliased destinations. Wandering
accepts the reviewed bank-4 projected-position predicate and its pending-script
side effect.

Path coordinates, cursors, waits and accumulators retain their original byte and
halfword behavior. The stepped path accumulates fractional movement before its
blocking test; reversible routes retain direction priority, optional follower
blocking, one-reversal limits and animation suppression across command advances.
Two early-wait paths test a value inherited from incoming SL: direct adapters pass
it explicitly and compare both zero and nonzero values. The normal adapter uses
the reference reset baseline's zero initial SL, preserved by the startup and
top-level render callers. Arbitrary BIOS/register-history equivalence is not
established. The bounds helper leaves outputs unchanged for an invalid direction;
path roots require an active facing of 1..4. Malformed nonterminating paths and
rejection loops fail explicitly at bounded limits rather than hanging the host.

Bank 4's three recovered script sources declare 506 functions, all with explicit
native dispatch. Four declarations previously classified as link-only are also
used by the offline music hall. save_group4.c adapts these local operations and
the inverse context-copy helper: five Main contracts, without link transport.
They retain the 32-bit context pointer, packed halfword arguments, clear-before-copy
record selection, forward byte aliases and retained headers. Record data, lengths
and category flags are read from the player's ROM. Classification preserves
ignored zero/one payload bytes, the special marker and category-rank thresholds;
the availability scan uses control 4 rather than the selection row.
Music hall entry/movement and an original save/Continue now exercise these dependencies.
The complete local record/presentation and record-selection families are now
adapted in the same module: 29 source functions, 28 Main contracts and one void
layout task. Credits share the original resource-song, duration and payload
readers with the music hall. Resource pointers, animations, positions and tables
stay in the player's ROM. Local record updates retain suffix collisions, depth
and byte-number wrapping, header/padding writes, two original save records and
both SRAM copies. A received kind-3 pattern longer than its nine-byte source
buffer stops explicitly. Layout/indicator helpers preserve captured versus live
selectors, raw inactive slots, zero-count comparisons before byte conversion and
affine allocation/exhaustion. These local helpers do not start link transport.
This remains a structural inventory: literal indirect targets,
complete bank-4 progression, activities and actual-window integration need
separate validation. The existing opening scenario is an earlier-area regression.

The bank-5 family is grouped in bank5_presets.c and bank5.c: 172 presentation
presets, grid choices, actor mirrors, oscillation/motion, mechanism lanes and
outcomes, round initialization and time ranking. Its reviewed interfaces expose
34 Main contracts, 191 argument-free task targets, 13 animation contracts, one
VBlank contract and two indexed lane helpers. These overlapping counts are not
an area-completion percentage. Original tables remain in the player's ROM or
its initialized mutable RAM views.

Grid placement makes exactly three random attempts without retrying occupied
cells. Raw cell/kind selectors and wrapped slot arguments retain their distinct
widths. Mirrors preserve unsigned area boundaries, table matching, fallback
images, live animation offsets and overlapping source/destination actors.
Mechanism lanes retain signed depth/count comparisons, byte state transitions,
wrapped spawn counts and eight-bit outcome sums. Time ranking preserves the
halfword limit, equal-score insertion order and original fractional conversion.

Main opcode 0x4E copies its four argument bytes forward, resolves variable
references 0..15 and converts the all-ones sentinel to a null callback. VBlank
dispatch accepts the reviewed void(void) mechanism callback after video commit
and before transfer/audio service; unsupported targets still fail explicitly.
The complete shared script_callbacks_stairs_residual.c family adds direction
configuration, class-selected velocity clearing, idle-animation refresh and
game-flag setup. Idle refresh uses the existing valid-active-slot contract;
raw direction configuration and class-selected clearing retain mapped selectors.
Complete bank-5 story/activity integration remains a separate pending check.
The four recovered bank-5 script sources declare 394 functions, all with explicit
native dispatch. This is a structural planning count; literal indirect targets,
correct calling forms and full area progression still need integration evidence.

Bank 6's main/final source families share bank6_presets.c, bank6.c and
bank6_targets.c. Their 200 presentation presets, 30 Main contracts, 223 task
targets, six animation contracts and four typed helpers have whole-family
comparisons. The one-function actor_neighbor_animation_pair.c dependency adds a
task target through the existing actor preset adapters. Calling-contract counts
overlap; the separate map_bank6_chain_callbacks.c family is described below.

Orbits preserve the low-byte slot and low-halfword class selectors, wrapped phase
addition, upper-band clamp and signed center coordinates. Projected motion keeps
eight lookahead reads while consuming five bytes and signed division of the
wrapped square sum. Payload/return approaches store an even duration but divide
by the original unmasked value. Cursor rectangles retain strict
edges, halfword wrapping and result-store order. Target selection preserves both
failure paths, two random draws and wrapped modular selection; task patterns use
the original prefix matches and final-match priority; malformed nonterminating
walks fail at an 8,192-byte guard. Affine allocation retains the 255 initialization
sentinel and raw exhausted index 32, including its mapped write.

The target-restore no-match path reads incoming R5 as a descriptor. A direct
adapter accepts that guest address explicitly. The normal Main adapter uses
0x030005E8, which the original MainScriptInvokeCallback loads into R5 before its
indirect call. Both the actual Main opcode and arbitrary mapped retained
descriptors are compared; this does not enable an unreviewed task/animation ABI.
Area progression and timed desktop integration remain pending.
The three recovered bank-6 script sources declare 528 distinct functions, all with
explicit native dispatch after the chain family. Indirect and literal targets
require separate review; this declaration count does not establish area coverage.

The complete 12-function chain family uses bank6_chain.c. Its five Main contracts,
two argument-free task targets and seven animation contracts preserve full-word
yield values, distinct consumed counts and pointer identity. First-chain phase
thresholds use the full halfword while interpolation uses its low byte. Phase 65
retains both attached actors' Y positions. The three transforms preserve wrapped
products, shared-position capture and live slot/selector reads. Their zero affine
sentinel can allocate matrix zero again on a later call, and exhausted allocation
retains the raw index 32 and its usage write.

The second chain captures both argument bytes before allocation. Its null internal
call discards the original two BIOS reads: those values never enter calculations
or writes. The private CPU oracle varies synthetic bytes for these reads; the
native adapter needs no BIOS image and does not model their bus/timing effects.
Landing and rectangle callbacks retain signed strict boundaries, paired class
coordinates, original facing animations and pending-script flags. Flight keeps
halfword counters, invalid-slot class 460, wrapped branch thresholds and flag-based
continuation; wrapping tests the signed sum before a single Y decrement. Whole
function-family comparisons do not establish complete timed scene or area play.

The shared presentation batch uses presentation.c, presentation_background.c and
presentation_window.c, with the tilemap-row queue in transfer.c. Its nine complete
source families cover bank-7 actor height/presets, region motion, graphics/background
resets, affine background callbacks, background actors/scroll, window presets, the
scroll-text-row helper and the DMA tilemap-row queue. The 56-function census includes
the two already-adapted window modal roots. Its 28 Main, 40 argument-free task, six
animation and six direct-helper contracts overlap; these counts are not progress
percentages or evidence of full scene completion.

Height changes preserve the original one-step overshoots and unsigned rectangle
tests. Region motion retains its mixed signed/unsigned bounds and animation identity.
Affine backgrounds use the full halfword angle, signed scale reciprocals and wrapped
anchors. Background actors retain five shared timers/offsets, two random draws per
spawn, live selectors, exhausted matrix index 32 and a word-sized velocity index
after the byte-sized constructor index. Scroll rows preserve compact split buffers,
two tilemap pages, late descriptor reads and private linked text tasks. Tilemap DMA
queueing retains zero-width rows, low-byte word arguments and the original queue wrap.

Window sequences keep byte countdown underflow, null-script allocations, stage
thresholds, raw selectors and pointer reloads after overlapping effect writes. Object
window styles are limited to the eight pointers copied by the original stack frame;
the located recovered callers use styles 0..3. An undefined larger style or removal
of a task absent from the list fails explicitly. These malformed-state guards do
not establish original caller-stack or invalid-list parity. Source resources remain
in the player's ROM. The ending route now exercises the complete credits sequence;
other window sequences and timed desktop presentation still require integration checks.

The bank-7 tile-board and scene-preset families use bank7.c. All 32 original
entries have reviewed contracts: five Main callbacks, 30 argument-free task
targets and one two-word helper. The move callback clears its outputs before
reading the direction and consumes one byte. It preserves the special cursor
connections between 0, 10 and 4, conditional movement into an empty cell, byte
wrapping, signed scaled velocities and the RAM direction-offset reload after
the render setter. Arrow coordinates are captured before construction while the
cursor and neighboring cells are read live after each constructor. Unused arrows
retain their previous state. Animation, coordinate and vector tables stay in ROM.

The 26 scene presets retain raw class selectors, neighbor wrapping, constructor
and graphics order, half-cell positions, background resets and the indicator.
Their void returns acquire only a task contract; Main never invents a consumed
count. The recovered bank-7 script source now has explicit dispatch for all 141
declared functions. This structural dependency census does not establish the
complete final area or story. The separate input-only tower scenario above now
solves the puzzle and checks that its flag survives original Save/Continue.

The six/eight-choice room family uses scene_selection.c. Its 13 original entries
retain ten Main consumed-count contracts and three void task-only presets.
The shufflers read bank pointers from guest RAM, preserve rejection sampling and
the two index groups, and obtain their data from the player's ROM. The eight-choice
room preserves its fixed first result without advancing the PRNG. Constructors
read later state/value bytes live, preserve inactive slots and the flag-1EB position
adjustment. Outcome thresholds, special choices, five-bit counts in four-byte
record slots, duplicate empty records and counter values 0/255 are compared.
Malformed PRNG state stops after a bounded rejection loop. These function-level
checks do not establish a complete room, activity or tower playthrough.

A separate private input-only scout completes the initial Ham-Jam conversation,
No-P response and return to Clubhouse. Photo-room entry (map 1:3), free movement
and return to Clubhouse also run natively after the SaveGroup3 batch below.
The greeting now completes and returns to its action menu after the entire
48-function actor_render_sequence_presets.c family was adapted. The shared
14-function script_callback_actor_map10_5.c family and three one-function
key-save-mark, key-save-sequence and raster-E5 panel families are now adapted.
All 112 declarations in map_bank10_selection_scripts.c have explicit native
dispatch. This structural inventory is not full runtime coverage. Native input
opens and leaves both empty Items and Gems panels, retaining the photo-room
actor position and story counters. The original Ham-Jam scout
is outside the canonical opening scenario; photo entry,
greeting and a save/load roundtrip now have canonical input markers. Neither route
establishes complete activity progression.
A separate Windows process also restores the photo-room save beside the characters
and renders the greeting, portrait, actor animations and continuation indicator.
The complete photo activity and training-seat route remain unverified in the
actual desktop window.
The current Windows executable separately opens the empty Items and Gems panels
with keyboard input and returns through Status to the same photo-room scene.

The shared selection adapters retain four text-task allocations even for an empty
selection. Paged text leaves context zero unconfigured, uses halfword item offsets
and reads the language and page from live state. Scroll reconstruction retains the
source's separate physical list position and nonempty ordinal; refresh uses an
eight-bit row start, including its row-zero underflow. Old/new item-validity flags
update even when navigation cannot move. Debug-key entry points remain gated by
the original flag 2 and preserve key priority and list boundaries. The raster-E5
panel retains signed coordinates, its selector-seven linked overlay and the
follower animation gate. Argument-dependent navigation and modal transitions do
not acquire an invented argument-free task contract.

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
first character and characters after spaces with the original French
capitalization exception. Its static guest text buffer does not need pointer relocation.
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

The complete game_record_messages.c family shares record_menu.c. Message types
select their original ROM-backed text bundles and retain the special-initial key
list, language-specific casing, live argument reads and independently wrapped
eight-bit output index. The eleven animation presets retain void/F4 contracts
and clear the follower boot gate at the original point. Undefined message types
and strings that do not terminate within 65,536 reads stop explicitly.
The assembly counter-prefix leaf has a separate Main51 binding: it reads 32
counter bytes, excludes index 31 and values zero/255, and writes control 0x60.
Its original R0 result is unused by Main51; it is not an ordinary Main callback.

records.c also contains the complete ten-function game_record_use.c family and
the three game-record counter callbacks. They retain packed-slot scans, signed
selection/sentinel behavior, the shared halfword result store, original accumulator
categories, eight-bit counts, localized text casing, decompression and queued
tile/palette transfers. The three counter callbacks preserve the independent
second occupied-record count and leave old animation fields intact when empty.
record_menu.c contains all five record_display.c functions: two linked caption
contexts, byte-wrapped title copying, decimal digits, counter defaults, BG2 offset
and the packed tile-pair helper. The latter reloads the text and descriptor after
overlapping writes and masks the original four palette bits.

When ConsumeGameRecord rejects its comparison, the source leaves its local
result unwritten but copies two bytes from it into controls 0x2C/0x2D. The adapter
initializes that local to zero, returns the original failure result and consumes
nothing. Comparisons supply the corresponding zeroed inactive ARM stack; they do
not establish arbitrary stack-history identity. A normal SDK rejection also
preserves the selected rock and reports the need for a higher love meter.
An empty remaining-graphics query, an out-of-range local class-word index and
nonterminating text/tile-pair input stop explicitly rather than using indeterminate
source locals or hanging the host.

The shared strided-transfer family uses the original halfword-normalized width,
row count and stride, forward BIOS halfword copies, cleared heap allocation and
free order. Zero rows preserve memory; failed allocation stops explicitly before
decompression. Function-level comparisons do not establish timed window rendering.

pair_list.c groups the complete 40-function script_callbacks_pair_list.c family.
Its source name refers to clothing categories and color variants. Six list pointers,
localized rows, preview graphics, animation and palette tables are read from the
player's ROM. The adapters preserve the original ownership-mask classification,
first-unset search, price and seed counters, row highlights and raw byte render
selectors. The existing checked number-drawing interface remains checked; the
source-compatible number callback uses an explicit raw-slot interface.

Eighteen Main contracts include the two IRQ controls in raster.c and the
three-byte number callback moved from book.c. Seventeen argument-free callbacks
and nine void animation presets accept F4. Thirteen typed helpers include that
number renderer and the return-zero leaf; the latter has no invented Main/F4
binding because its incoming ABI is unknown. Unterminated lists stop explicitly.
Byte shift counts of 32 or more preserve the original ARM result. Palette slots
are hidden by clearing only their active byte, while reset operations clear the
complete slot. Selection movement captures the original cursor pointer before
highlight writes can change live selectors.

The Boutique list deliberately disables its earlier preview's VCOUNT effect;
zero blend registers in the list are expected source behavior. All 204,233 focused
checks retain full flag/mask domains, raw selectors, list and counter boundaries,
live aliases, palette transfers and actual Main/F4 command calls. Function-level
coverage does not establish every clothing activity or original CPU timing.
