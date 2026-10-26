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
  PPU regressions for tile mapping, transparency, mosaic/window phase and effects,
  including analytically known 4bpp colors for every sub-tile window width and
  scroll phase with both horizontal flip states;
  word-choice slot setup, coordinate truncation, class gates and priority flags.
- graphics: palette interpolation/source wrapping, scripted repeat/call/delay
  flow, alpha/mosaic transitions, compressed palette frames, paused states and
  explicit faults for cyclic or unsupported commands.
  Raster tests cover allocation clearing/replacement, window sweeps and layer/DMA
  cleanup, radius arithmetic, circular HBlank phase/termination and unknown IRQ targets.
  VCOUNT tests cover exact line-104/frame-wrap transitions, pending IRQ delivery
  after re-enabling, counter wrap, zero/unknown handlers and original pending bits.
  Window tests cover alternating/stepped/circular setup, radius scratch bounds,
  live phase state and explicit rejection of an unreviewed configure selector.
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
  Generated keyboard checks explicitly deselect connected devices; the gamepad
  portion selects its virtual device. Held hardware input cannot contaminate
  these checks. The production input code is unchanged; physical models remain
  a separate validation requirement.
- session: invalid-ROM rejection without file writes. With a private ROM argument,
  verifies initialization persistence, failed-write retry, retained backups and
  rejection of partially modified SRAM after a native runtime fault.
- app-smoke: creates an isolated temporary profile, initializes the actual
  SDL renderer and menus for six frames, closes cleanly and removes that profile.
  No graphics, settings, ROM or save files are written in the checkout.
- app-preferences: starts the actual hidden application twice against a disposable
  Unicode profile. Both processes retain nondefault settings, window dimensions and
  all ten keyboard/gamepad mappings; SRAM and its previous backup stay byte-identical,
  and the profile lock is released after each shutdown. A separate input process
  exercises every saved mapping, short taps and the saved deadzone with synthetic
  keyboard events and its own SDL virtual gamepad. A third application process
  preserves an unsupported future settings file verbatim. The parent verifies
  removal of the complete temporary profile after closing all child processes.

Run this integration test alone with
`ctest --preset release --parallel 1 -R app-preferences`. It needs no ROM and runs
its child processes sequentially. `--smoke-test --data-dir PATH` preserves the
explicit profile; use only disposable test profiles. Without `--data-dir`, smoke
testing creates and removes its own profile as before. These checks establish
hidden startup/shutdown persistence and saved input behavior; they do not establish
interactive settings-menu use, actual fullscreen transitions or physical-pad support.
The twelve serial CTest suites pass in 3.29 seconds on the development machine,
including 65,844 core and 66,503 game checks. Timing is not a portable guarantee.

Use CTest for each functional batch and the existing focused `--...-only` reference
checks during implementation. For broad native changes, run the complete
reference comparison with all available independent oracles. Keep functional sources
unchanged until that run ends. No exhaustive domains, memory regions or comparisons
are removed to shorten the run. A narrow callback correction uses the affected
family comparison and CTest. Reuse existing story proofs rather than repeating the
entire story through desktop automation; inspect the window when a change affects
desktop integration or presentation.
An uninterrupted manual desktop playthrough is supplementary release evidence,
not a gate for each batch. Physical-controller validation is deferred to the user.

The last complete suite passed 5,861,660 checks in 668.028 seconds. It retains
all 45 preceding named phases and their previous assertion counts, adding 2,502
title record-reset and soft-reset callback checks. The subsequent application
preferences batch changes only smoke-test profile handling and test harnesses;
the native game/GBA sources and reference-probe binary are unchanged. Its separate
CTest and packaged-application checks retain this core proof without claiming
that the frontend executable is byte-identical. The subsequent bank-4 animation
binding correction changes the native core and probe: the full count is historical,
with current focused bank-4 and CTest evidence recorded below. The preceding boundary passed
5,859,158 checks in 655.399 seconds, adding 65,902 local Ham-Jam UI/text checks. The preceding HBlank/window boundary passed
5,793,256 checks in 662.005 seconds. It retains
all 5,561,200 preceding cases and adds 232,056 HBlank/window comparisons,
including 46 independently rendered clocked frames. All 204,233 clothing-catalog
comparisons remain enabled.
The 740,210 VCOUNT checks, including independent timed pixels, remain enabled.
Every one of the 44 preceding named phases retains its previous assertion count.
The preceding 66,963 local music/record comparisons, 61,195 six/eight-choice room checks and 71,722 unassigned-I/O
collision checks, including even/odd register boundaries and every render count,
remain enabled.
Full 16-bit domains, 30 private PPU snapshots and all independent oracles remain
enabled. The focused motion group passes 16,095 checks in 2.561 seconds; all
11 CTest suites pass in 0.82 seconds, including 17,383 game, 823 renderer,
522 graphics and 34 keyboard/gamepad checks. Replay and full-suite timing are
separate local observations, not portable performance guarantees.
Earlier optimization reduced the same 4,200,342-check suite from approximately
1,238 seconds (log timestamps) to 678.102 seconds without reducing coverage.
Use focused groups during iteration and the complete suite at a functional batch
boundary. Hidden-primary modal roundtrips and all four active-follower directions
remain covered.

`compare_reference.py` prints elapsed time and assertion counts per family and for
its complete invocation. Optional `--timings-json PATH_OUTSIDE_CHECKOUT` retains a
private machine-readable report, including failed phases. The total also includes
setup and coupled checks outside named families. Reports and logs are never versioned.
Plain RAM snapshots use bounded bulk copies; device IO, mirrors, wrapping and partial
faults keep the original bus path. Python reuses snapshot buffers by size. The content
audit reads historical objects with one Git batch process and checks the same blobs.

The bus oracle's `--scene-copy` mode supplies independent mGBA LDMIA responses:
44 complete scene copies and 40 Main20 variable-resolved copies, retaining all
previous sources and adding protected BIOS addresses, the observed 0x612 tile-game
return and the BIOS/reserved boundary, each with all four alignments. The native
side models the reviewed post-service BIOS prefetch and reads the player's ROM at
the reserved-bus prefetch site. Reference-only fixtures supply independent bus
responses. Unicorn's generic Thumb LDM lacks ARM7 alignment, so mapped pointer arguments
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
errors, team phase 3 and the twenty-seed reward. The continuation learns Blanko
and Ta-dah, uses Lookie to enable Barrette's request and runs Spat's shore scene.
Gasp-P is then used with Seamore in Clubhouse: rescue flag 0x1AB must change from
false to true before the return to Sandy Bay. The rescue teaches Trust-T and
raises the love count from three to four. Hif-hif retrieves the bottle message.
The preceding 138,742-update prefix passed in 205.90 seconds with 145,216
host-session checks in 209.97 seconds. Its inputs and checkpoints are preserved.
The extended route adds the wooden board pickup, Items display and original
save/Continue. A 6,103-update focused replay from the verified bottle-message SRAM
passes in 8.08 seconds; 8,209 host checks pass in 10.52 seconds, including exact
SRAM-file persistence and fresh continuation. The endpoint is map 4:5 at x=160.5,
z=48, 31 words, 22 seeds, four loves and Bijou, with board flags 0xA6/0x194 and
bottle flag 0x195 retained. The combined 143,084-update SDK replay also passes in
216.04 seconds. A fresh SDK process reopens the host-written file and checks the
saved flags and the board selection (control 0x180 = 1) in 2,255 updates.
The source SRAM stays unchanged.

The extension preserves all 143,084 prior inputs/checkpoints and adds the ten-note
melody (asserting each note/counter), its original save, hidden cache and gem,
Captain/Fussfuss gate, additional shop word, ferry Yep-P choice, island arrival,
Pashmina/Penelope scarf scenes, Hif-hif pickup and music hall entry/movement/save.
The combined 191,915-update SDK replay passes in 301.12 seconds. From the saved
wooden-board checkpoint, the focused 50,592-update SDK route passes in 79.50 seconds
and 54,387 host-session checks pass in 84.18 seconds. The endpoint is map 4:13 at
x=128.5, z=196.5 with 32 words, 22 seeds, four loves and Bijou. Flags 0x11F/0x121/
0x143/0x97/0x14F/0xA6/0x194 and controls 0x84=3, 0x87=2, 0x89=3, 0x8B=1, 0x91=2
are retained by original save/Continue and fresh disk continuation. Host SRAM
matches the SDK export byte for byte; source saves remain unchanged. A fresh SDK
reload independently checks the host-written file. The current core also passes
the complete 281,135-update erased-SRAM Sunny Peak route again in 464.68 seconds.
The separate `treasure_island.json` continuation leaves those 191,915 inputs and
checkpoints unchanged and starts from the saved music hall in a fresh process.
Replaying that prerequisite on the current core passes in 295.419 seconds and
exports exactly the same 32 KiB SRAM as the earlier focused music-hall checkpoint.
It follows the
demonstration's Yep-P choice, guard access, dressing-room mirror, Stickie/Heyhoo
impostor scene, first lighthouse encounter and elder's two Heyhoo conversations.
It asserts Tuggie's book acquisition, its use at the lighthouse, the fifth love,
follow-up word, story phase 0x34=6 and flags 0x72/0x5B/0x138/0x182. The focused
44,850-update route passes in 74.35 seconds from the current prerequisite;
48,522 host-session checks pass in 79.54 seconds. Original save/Continue and a fresh SDK reload retain map 4:12 at
x=128.5, z=128.5 with 34 words, 22 seeds, five loves and Bijou. Host SRAM matches
the SDK export exactly; source saves are unchanged. The fresh SDK process also
opens Status and renders its counters in 2,103 updates.
These are SDK/host-session results. Other Sandy Bay activities and a complete
desktop playthrough remain unverified.
Concatenating the focused recording directly after the earlier in-process
Continue diverges at room navigation. The supported scenario chain uses an
explicit SRAM reload between stages, not a claim of one uninterrupted recording.
The prerequisite is reproducible from erased SRAM without duplicating the opening:

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/sunny_peak.json --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_SUNNY_PROFILE
    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/sandy_bay.json --save PATH_TO_NEW_PRIVATE_SUNNY_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_ISLAND_PROFILE
    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/treasure_island.json --save PATH_TO_NEW_PRIVATE_ISLAND_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_FIFTH_PROFILE
    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/fun_land.json --save PATH_TO_NEW_PRIVATE_FIFTH_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_SIXTH_PROFILE
    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/haunted_house.json --save PATH_TO_NEW_PRIVATE_SIXTH_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_BRONZE_PROFILE

Each output profile must be new and outside the checkout. Every continuation
reads its prerequisite SRAM without changing it and tests a fresh host session.

The Fun Land continuation has 85,539 updates and 36 semantic checkpoints. It
checks the return ferry's confirmation, preserved fifth love, Digdig destination
exit, Clubhouse unlock and Boss lesson, Scrub and book return, trash acquisition,
musical challenge entry fee, 630-point score, green-marble award and the original
No-P replay choice. The original 35,899-update green-marble prefix and its 17
checkpoints are retained unchanged, including Save/Continue at map 3:4. The same
process then learns Blash-T, follows the power failure, frees the operator with
Pakapaka, requests help and sends Ham-Bot using Heyhoo. It learns Chukchuk,
consumes the trash against Spat, repairs the robot with Scrub-E and learns Tiptop.
The sixth love and Clubhouse destination unlock survive original Save/Continue:
map 1:1 at x=144.5, z=192.5, 39 words, 19 seeds, six loves and Bijou. Flags
0x44/0x69/0x62/0xAB/0x127 are set, 0x184/0xBB cleared and controls 0x8F=2/0x90=6
retained. The full SDK stage passes in 138.71 seconds and the host replay passes
90,912 checks in 140.82 seconds, including original disk persistence and a fresh
session. SDK and host SRAM match across all 32 KiB. Another independent SDK
reload/Status inspection passes 2,103 updates in 2.589 seconds and rechecks the
saved flags, controls, position and counters. Later Sunny Peak, remaining activities and full desktop progression remain
unverified.
No earlier scenario, exhaustive reference domain or independent oracle was removed.

The Haunted House route continues from the saved sixth reconciliation in a fresh
process. Its 204,853 updates and 68 scalar checkpoints retain the entrance scene,
Hif-hif seed pickup, Digdig passage, two Blash-T price reductions, five-seed ID card
purchase and Ta-dah entry. Heyhoo at reception teaches Thump-P and opens its book;
another Ta-dah earns the bronze card. Every earlier bronze-card and Save/Continue
checkpoint remains unchanged. Go-P opens the mirror passage; three ghost captures
earn the silver card, followed by the plank passage, Hamlift, Hamspar and Nopookie.
An original Save/Continue preserves Hamtaro's capture and Bijou-only control.
Nok-nok, Pushie, Heyhoo and Libert-T then complete the prison rescue, seventh
reconciliation and Flipflop lesson. All 142,196 preceding inputs and 43 checkpoints
remain unchanged. The extension loses the tile challenge, returns normally,
retries successfully and earns the gold card. Hamlift opens the upper floor;
Thump-P teaches Perksie, two Tack-Q actions open the wall, and three quiz answers
lead to Spat. Flipflop and Nopookie drive him away. Clubhouse unlocks the jungle.
The final original Save/Continue retains map 1:1 at x=144.5, z=192.5, 47 words,
17 seeds, seven loves and both characters, flags 0x45/0xB8/0x7B/0x7D/0x1B2/
0x1B3/0x10 set, 0x185/0x15B cleared and controls 0x91=2, 0x90=6, 0xD0=7.
The complete composed SDK replay passes in 370.114 seconds. Host coverage uses
two chained legs on the same code: the 163,285-update gold-card prefix passes
171,343 checks in 342.02 seconds; a fresh session from that exact saved SRAM
passes the 43,329-update upper-floor/quiz/jungle leg, 46,968 checks in 106.50 seconds.
The composed SDK, final host file and input-only scout match across all 32 KiB.
These are chained host legs, not one uninterrupted full host invocation.
A separate 2,103-update SDK reload/Status inspection passes in 2.598 seconds.
Optional Haunted House activities and complete desktop progression remain unverified.
Literal input recordings from the original guide agree at
entry but diverge later in this native route; their observation labels are not
semantic assertions and the divergent recording is not used as native proof.

`jungle_hat.json` starts from the final Haunted House SRAM in a fresh process.
The 102,159-update, 19-checkpoint route learns Wait-Q and Hampact, follows the
Sunny Peak robbery/hat request and returns to Fun Land for Stan's Hamteam lesson.
It asserts the missing word and inactive trap before that lesson, then the book
entry and active trap afterward. Bog's cage, Cappy's chase, Bluhoo and Yep-P
lead to the returned hat, Hamboree and eighth reconciliation. Original
Save/Continue retains map 2:5 at x=80.5, z=176.5, 51 words, 17 seeds and Bijou;
flags 0x55/0x8C/0x71/0x8F/0x1A0/0x1A7/0x1AC remain set and controls 0x6D=4,
0x6F=3, 0xC9=8. The complete SDK replay passes in 166.08 seconds and the complete
host replay passes 107,364 checks in 173.37 seconds, including fresh-session
continuation and exact 32 KiB SDK/host/scout SRAM equality. Another independent
2,103-update SDK reload/Status inspection passes in 2.766 seconds. Input SRAM
and every preceding scenario remain unchanged. The next chain step is:

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/jungle_hat.json --save PATH_TO_PRIVATE_HAUNTED_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_HAT_PROFILE

`jungle_wedding.json` then starts from the saved hat stage. Its 91,911 input
updates and 21 checkpoints cover the thermal egg, Clingie, Thank-Q, first
ceremony, rubber-band purchase, Hulahula and slingshot crafting. Hamscope's
tutorial, aimed beehive shot, red-marble pickup and second ceremony restore the
ninth love and teach Vast-T. Original Save/Continue retains map 6:9 at x=120.5,
z=192.5, 56 words, 12 seeds and Bijou; flags 0xC4/0x186/0x6C remain set and
controls 0x8C=2, 0xB0=5, 0xB2=2, 0xC5=2, 0xC6=1, 0xC7=2, 0x1A0=2.
The complete SDK replay passes in 161.24 seconds and the complete host replay
passes 97,365 checks in 170.45 seconds, with exact 32 KiB SDK/host/scout SRAM
equality. An independent 2,103-update SDK reload/Status inspection passes in
2.766 seconds. All preceding inputs and assertions remain intact; two old Sandy
Bay labels now accurately describe its hidden cache, whose 0x143 flag does not
establish possession of the blue marble (0xAA).

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/jungle_wedding.json --save PATH_TO_PRIVATE_HAT_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_WEDDING_PROFILE

`sunny_temple.json` starts from that saved wedding stage. Its 89,944 updates and
21 checkpoints retrieve the couple's returned red marble and the actual blue
marble with Digdig in map 4:3. It asserts all three inventory flags before the
altar. Existing oil/open-lever progress is retained; the cart is oiled and ridden
with Gogo, and Rubrub is learned. Putput consumes 0xAA/0xAB/0xAC and sets
0x11B/0x11A/0x119; the spoon sets 0xAD/0x11C. The cave exit covers the repaired
unassigned-I/O record scan. Tower-gate checks precede the inventor's introduction,
the wife's Yep-P request and Ta-dah's spoon delivery. The tenth reconciliation
and lesson survive original Save/Continue in map 6:6 at x=112.5, z=88.5,
58 words, 12 seeds and Bijou; 0xAD clears and control 0x8C becomes 4.
The complete SDK replay passes in 215.79 seconds; the full host replay passes
95,059 checks in 176.98 seconds. SDK, host and scout match all 32 KiB, and a
separate 2,103-update SDK reload/Status inspection passes in 2.701 seconds.
The prior save is unchanged. Inputs and scalar assertions contain no captured
state or proprietary resources.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/sunny_temple.json --save PATH_TO_PRIVATE_WEDDING_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_TEMPLE_PROFILE

`spat_tower.json` starts from the saved temple/workshop stage. Its 58,579 updates
and 12 checkpoints cover the inventor's catapult, Gogo/Stickie launch, complete
sliding puzzle, guard/doorway conversations and Spat's introduction. A normal
Hamscope shot triggers the original retreat; Chukchuk and Hamteam with Harmony
upgrade Hamscope. Original Save/Continue retains the solved puzzle and upgrade
in map 1:2 at x=184.5, z=64.5, 58 words, 12 seeds, ten loves and Bijou.
The SDK replay passes in 105.19 seconds and the full host stream passes 62,902
checks in 97.81 seconds. All 32 KiB match SDK, host and scout; a separate
2,103-update SDK reload/Status inspection passes in 2.684 seconds. The source
save is unchanged. No captured CPU/RAM state is injected into this playthrough.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/spat_tower.json --save PATH_TO_PRIVATE_TEMPLE_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_TOWER_PROFILE

`spat_battle.json` continues from the saved Hamscope upgrade. Normal input
advances both battle phases and lands the shot into Spat's open mouth. Eleven
checkpoints assert the battle outcomes, 59th learned word, credits entry and
progress, complete credits return, restored postgame control and original
Save/Continue. The saved result is map 7:3 at x=116.5, z=160.5, 59 words,
12 seeds, ten loves and Bijou, controls 0xCF=5/0xD1=4 and ending flag 0x190 set.
The 38,099-update SDK replay passes in 58.15 seconds; the same input stream
passes 40,937 host-session checks in 60.53 seconds. SDK, host and scout match
all 32 KiB. A separate 2,103-update SDK reload/Status inspection passes in
2.758 seconds. The prior save is unchanged. The scenario stores button inputs
and scalar expectations, with no captured CPU/RAM state or resource bytes.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/spat_battle.json --save PATH_TO_PRIVATE_TOWER_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_POSTGAME_PROFILE

These results establish the saved main-story ending in the SDK and host session.
They leave 27 words and optional content outside the ending scenario's coverage.

`sunny_optional.json` starts from that saved ending. Blushie, Bestest, Bloat-T,
Tran-Q and Hamtast increase the dictionary to 64 words. The route obtains all
Panda's materials, completes his bridge and collects the Rainbow Key. It then
obtains Condor's Wings and Great Springs, digs two additional rocks in Seedric's
sand bath and completes the Clubhouse record desk's empty-inventory greeting.
The extension identifies Banana Stone, Crystal and Ammonite through Rubrub,
retains three partial polishes and checks that the strength limit does not consume
the selected rock. It completes the result summary and inspects all three owned
Gems entries, including wrap navigation and return to the same scene.
All 153,436 prior updates and 32 checkpoints remain unchanged; the expanded
177,048-update route has 48 checkpoints and passes the SDK in 296.31 seconds.
Original Save/Continue retains map 1:4 at x=80.5, z=80.5, 64 words, 12 seeds,
five rocks, ten loves and Bijou. The final checkpoint includes both song flags,
the Rubrub visit, bridge completion, Rainbow Key and original ending flag.
The full host stream passes 184,866 checks in 300.02 seconds, and all 32 KiB
match SDK, host and scout. A separate 2,103-update SDK reload/Status inspection
passes in 2.826 seconds. It also reads the original collection counters
23/27/30 as one each, counter 10 as still undiscovered, and all eight packed
record counts: the consumed records stay empty, the partial rock retains one
polish and the four untouched rocks retain four each. Earlier source profiles
remain unchanged.
Only normal inputs and scalar expectations are stored in the scenario.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/sunny_optional.json --save PATH_TO_PRIVATE_POSTGAME_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_SUNNY_OPTIONAL_PROFILE

The independent `level_three` route in the same area file starts from the
verified 75-word performer save. Its 37,650 updates and fifteen checkpoints cover
Boss's GoFor lesson, the board and room return, ordinary Rubrub at love level
three, a three-polish partial state, its successful fourth polish, all five
Diamonds, the empty-selector exit and Gems selection/description. Sixteen normal
rub actions consume exactly the remaining 1/4/4/3/4 polishes; the special strength
flag stays clear. Initial and final checkpoints include all 64 packed rock slots.
The final save retains 76 words, sixteen loves, five Diamonds and zero rocks,
as well as the other collection counters, clothing flags/count and all six photos.
The default 177,048-update, 48-checkpoint route retains every input and assertion,
including the earlier level-two rejection. Selecting the independent continuation
avoids replaying that earlier route when checking this new saved stage.

The SDK replay passes in 60.16 seconds and 40,464 host checks pass in 62.96 seconds.
Their 32 KiB saves match the canonical scout exactly. A fresh 2,103-update reload
takes 2.938 seconds and confirms all 76 unique word IDs against their flags, five
Diamonds, all 64 empty rock slots, inventory, clothing, photos and Status.
The eleven CTest suites (0.98 seconds) and nineteen scenario tests (0.042 seconds)
pass. Unchanged native sources and binaries retain the complete 5,793,256-check
reference proof. Ten words and the other documented optional/desktop activities
remain pending.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/sunny_optional.json --route level_three --save PATH_TO_PRIVATE_PERFORMER_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_LEVEL_THREE_PROFILE

The independent `sand_bath` route starts from the verified 76-word
`level_three` save. Six normal searches cover all three sand-bath rewards and
an empty search. It then polishes five rocks with six normal Rubrub actions,
including the Peridot's intermediate one-polish state and identification,
opens its Gems description and uses original Save/Continue. Every checkpoint
checks all 32 collection counters and all 64 packed rock counts. Initial/final
checks retain the clothing catalog and all six photo records. The final save
has five Diamonds, one Peridot, three Ammonites, two Crystals and zero rocks;
words, loves and seeds remain 76, sixteen and four. The Peridot descriptor
requires two polishes, so level three is not a prerequisite for this reward.
The default and `level_three` routes retain every previous input and assertion.
The canonical route has 36,678 updates and 23 checkpoints, removing 2,792
exploratory updates (7.44 percent of the postboot inputs). The SDK passes in
58.07 seconds; 39,666 host-session checks pass in 61.03 seconds. Their 32 KiB
SRAM files match exactly. A fresh 2,103-update reload passes in 2.914 seconds,
including all 76 unique word IDs, all collection/rock counts, retained clothing,
photos and Status. Eleven CTest suites pass serially in 1.93 seconds and the
nineteen scenario tests pass in 0.118 seconds. The unchanged native sources and
binaries retain the full 5,793,256-check reference comparison.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/sunny_optional.json --route sand_bath --save PATH_TO_PRIVATE_LEVEL_THREE_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_SAND_BATH_PROFILE

`sandy_bay_optional.json` continues from the sand-bath save. Its eight checkpoints
cover Tinglie, an unsuccessful Hamspar attempt with no reward, Thump-P's partner
assignment, the matching dancer's successful Hamspar, the reunion and Jamout.
It ends with 77 words, seventeen loves, level three/progress 45 and four seeds
after original Save/Continue. Every checkpoint asserts all 32 collection counters
and all 64 packed rock counts. Initial/final checks also retain the clothing
catalog and all six photo records. The route has 24,913 updates, removing 4,003
exploratory postboot updates (14.83 percent) while retaining both outcomes.
The SDK passes in 40.37 seconds and 27,901 host checks pass in 41.74 seconds.
Both and the canonical scout produce identical 32 KiB SRAM. A fresh 2,103-update
reload checks all 77 unique word IDs against their flags, inventory, photos and
Status in 2.935 seconds. Existing routes and native sources/binaries are unchanged.
This does not establish every partner assignment or the complete desktop activity.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/sandy_bay_optional.json --save PATH_TO_PRIVATE_SAND_BATH_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_JAMOUT_PROFILE

The independent `haunted_house.json` route `music` starts from that Jamout save.
It checks the piano's missing-record rejection, the frog-costume conversation,
both sides of the repaired upper lift, Pushie's attic access and three distinct
composer questions. Moonlight is awarded only after three correct answers.
The return retains the record through the corridor encounter and descending
stairs. The successful piano performance learns Stead-E and reconciles the couple.
The 61,645-update route has 24 checkpoints and ends with 78 words, eighteen loves,
level three/progress 49 and four seeds after original Save/Continue. Every
checkpoint checks all 32 collection counters and 64 packed rock counts;
initial/final checks also retain clothing and all six photo records. Collision
at the attic piano and traversal paused by dialogue are retained boundary cases.
The previous Haunted House route is unchanged. Other question orders and full
desktop activity coverage remain separate checks.
The SDK passes in 88.10 seconds and 65,611 host checks pass in 93.40 seconds.
Both and the canonical scout produce identical 32 KiB SRAM. A fresh 2,103-update
reload passes in 2.934 seconds, checking all 78 unique words against their flags,
the retained inventory, clothing, photos and Status. Eleven CTest suites pass
serially in 0.80 seconds and nineteen scenario tests pass in 0.051 seconds.
The native source and binary hashes remain unchanged, retaining the complete
5,793,256-check reference proof.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/haunted_house.json --route music --save PATH_TO_PRIVATE_JAMOUT_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_HAUNTED_MUSIC_PROFILE

`postgame.json --route key_contest` instead starts from the complete dictionary
and polished contest-prize save. Its 23,578 updates and eighteen checkpoints
cover all four contest answers, letter acquisition/submission, Rubrub's pending
delivery response, the world-map postal-phase transition, all four key awards
and original Save/Continue. Inventory checkpoints retain all 32 gem counters,
64 empty rock slots, 30 accessory slots, clothing and six photos. Separate SRAM
checks retain all 51 choreography records and verify both save mirrors. These
keys do not establish traversal of their caves or special-material refunds.

The SDK replay passes in 34.68 seconds; 26,422 host-session checks pass in 37.83
seconds. Both complete 32 KiB saves agree with the scout. A separate 2,103-update
reload passes in 2.921 seconds, retaining all 86 unique word IDs and their flags.
Twenty scenario tests and eleven serial CTest suites pass. Matching native-source,
executable and probe hashes retain the full 5,859,158-comparison reference proof.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/postgame.json --route key_contest --save PATH_TO_PRIVATE_BOOK_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_KEY_PROFILE

`special_stones.json --route rainbow_cave` starts from the verified contest-key save.
Its 61,002 updates and 21 checkpoints enter Sunny Peak's key cave and collect
its fixed rock with Ham-lift. Slot zero has five polishes remaining; cave reentry
keeps that one rock and removes its Ham-lift offer. The route returns through
the normal cave chambers and closes the merchant dialogue without spending
seeds. Rubrub's right hamster opens the service: one ordinary confirmation
clears the rock and changes Rainbow Stone counter six from unseen to one.
The occupied-rock count changes later in the dialogue. Original Save/Continue
retains all six keys, 86 words, 21 loves, 14 seeds, all other gem counters,
64 empty rock slots, 30 accessory slots, clothing, six photo records and all
51 choreography records.

The SDK passes in 95.03 seconds; 64,470 host checks pass in 97.74 seconds. Scout,
SDK and host match across all 32 KiB. A fresh 2,103-update reload passes in
3.033 seconds, checking the unique word list against its flags. Both save
mirrors agree and all previous routes are unchanged. Matching native-source
and executable/probe hashes retain the full 5,859,158-comparison proof. The
remaining special-stone routes are described below. Crafting refunds remain a
separate gameplay check. Polished-gem cave reentry is covered by the Wave route.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/special_stones.json --route rainbow_cave --save PATH_TO_PRIVATE_KEY_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_RAINBOW_PROFILE

The default `special_stones.json` route starts from the verified Rainbow Stone
save. Its 76,923 updates and 21 checkpoints cover Wildwoods' Sun Key cave,
Ham-lift's fixed key-two rock and its five-polish count, Rubrub's single-action
Sun Stone conversion, eighty seeds earned through ordinary Sunny Digdig visits,
and finding the manual artisan through normal room entries. Recipe nine rejects
missing ingredients without charging. Recipe 34 succeeds with the game's normal
random outcome, consuming one Sun Stone and 45 seeds and storing accessory 34
in slot one. Heart Earrings remain in slot zero; the other 28 slots stay empty.
The scenario retains Rainbow Stone and all unrelated gems, 64 empty rock slots,
clothing, six photo records, all 51 choreography records, 86 unique words and
21 reconciliations. Original Save/Continue retains 49 seeds and the same collection.
This verifies the successful special-material transaction; its refund-on-failure
branch remains pending. The Rainbow route was moved unchanged into the same file.
The SDK passes in 124.07 seconds and 81,342 host-session checks pass in 126.16
seconds. Scout, SDK and host saves match byte for byte. A fresh 2,103-update
reload passes in 3.019 seconds, including the unique word-list/flag audit. Both
SRAM mirrors agree, and every choreography record remains unchanged. Native-source
and executable/probe hashes retain the full 5,859,158-comparison reference proof.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/special_stones.json --save PATH_TO_PRIVATE_RAINBOW_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_SUN_PROFILE

`special_stones.json --route wave_crafting` starts from the verified Sun crafting
save. Its 35,139 updates and 23 checkpoints reach the Wave Key cave through Sandy
Bay's captain cove. Ham-lift obtains fixed rock key three with five polishes;
one Rubrub action clears it and awards Wave Stone counter four. Returning to the
unique-rock room while that polished gem is owned leaves all 64 slots empty,
sets the pickup's unavailable state and opens only the four ordinary Ham-Chats.
The route cancels this menu and returns normally to the manual workshop.
Recipe 55 succeeds with the original RNG outcome. It consumes one Wave Stone,
two Ammonites and 45 seeds, stores accessory 55 in slot two and retains the two
earlier accessories. Original Save/Continue retains four seeds, one Ammonite,
all unrelated gems, 64 empty rock slots, the remaining accessory slots, clothing,
six photo records, all 51 choreography records, 86 unique words and 21 loves.
This verifies polished-gem duplicate prevention and a successful transaction
using both special and ordinary materials; the failure/refund branch remains pending.
The previous Sun and Rainbow routes are unchanged.
The SDK passes in 59.66 seconds; 38,301 host-session checks pass in 62.70 seconds.
All 32 KiB match between scout, SDK and host. A fresh 2,103-update reload passes
in 3.024 seconds and verifies the unique word list against its flags. Both
save mirrors and all choreography records agree. Identical native-source and
executable/probe hashes retain the full 5,859,158-comparison reference proof.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/special_stones.json --route wave_crafting --save PATH_TO_PRIVATE_SUN_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_WAVE_PROFILE

`special_stones.json --route remaining_stones` starts from the verified Wave
crafting save. Its 50,772 updates and 41 checkpoints collect Ice Stone's raw rock
through Fun Land's Pushie passage, reveal Peace Moon's Haunted House entrance
with Hif-hif and follow the tower's Star Key route through its extra antechamber.
Each Ham-lift adds its fixed rock with five polishes while retaining earlier
rocks. Intermediate map, position, key, stair and entrance checks distinguish
the intended activities from merely arriving at the final destination.
Rubrub's first full-strength action converts Ice Stone; the service automatically
selects Peace Moon and then Uni-Star. Three actions clear all occupied slots
and award one of each gem without charging seeds. Original Save/Continue retains
all gem counters, 64 empty rock slots, three accessories, clothing, six photo
records, 51 choreography records, 86 unique words, 21 reconciliations and four
seeds. All earlier routes in the scenario file remain unchanged.
The SDK passes in 80.52 seconds; 54,000 host-session checks pass in 83.07
seconds. Scout, SDK and host match across all 32 KiB. A fresh 2,103-update
reload passes in 2.980 seconds, including the unique word-list/flag audit.
Both save mirrors agree and all choreography records are unchanged. Identical
native-source and executable/probe hashes retain the full 5,859,158-comparison
reference proof. Special-material failure/refunds remain unverified in gameplay.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/special_stones.json --route remaining_stones --save PATH_TO_PRIVATE_WAVE_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_STONES_PROFILE

`special_stones.json --route moon_crafting` starts from the saved three-stone
collection. Its 57,954 updates and 27 checkpoints collect Sandy Bay's fixed
Tack-Q rock, checking pickup flag 0x2BD, slot zero's four-polish count and the
other inventory. Rubrub converts it to one Sapphire. Nine ordinary Digdig
collections earn 45 seeds; every repeated visit checks its five-seed increase.
The manual artisan's original offer selects recipe 67 with all ingredients
available. The successful random outcome consumes one Peace Moon, one Sapphire
and 45 seeds, places accessory 67 in slot three and retains the three earlier
accessories. The route declines another order before original Save/Continue.
Final inventory checks retain all other gems, 64 empty rock slots, clothing,
six photo records, 51 choreography records, four seeds, 86 unique words and
21 reconciliations. Existing routes remain unchanged. This success leaves the
special-material refund branch pending.
The SDK passes in 92.87 seconds; 61,596 host-session checks pass in 97.03
seconds. Scout, SDK and host match across all 32 KiB. A fresh 2,103-update
reload passes in 2.898 seconds, including the word-list/flag audit. Both
SRAM mirrors agree and all choreography records are unchanged. Identical native
source and executable/probe hashes retain the full 5,859,158-comparison proof.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/special_stones.json --route moon_crafting --save PATH_TO_PRIVATE_STONES_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_MOON_PROFILE

`postgame.json` continues from the final `sunny_optional.json` save. It collects
the cave's hidden rock with Hif-hif, drops another with Tack-Q and collects it,
then obtains five seeds on each of two room visits. Rubrub identifies both new
rocks as Glass Beads. The summary and original Save/Continue retain map 1:4 at
x=80.5, z=80.5, 64 words, 22 seeds, five rocks, ten loves and Bijou. The route
also retains one additional partial polish on a common rock. Explicit collection
counter and packed rock-count expectations check the ingredients independently
of the total number of rocks. That checkpoint precedes crafting and delivery.

This chapter passes 49,102 SDK input updates and 11 checkpoints in 91.19 seconds,
then 52,513 host-session checks in 92.93 seconds. SDK, host and scout match all
32 KiB. A separate 2,103-update SDK reload/Status check passes in 2.734 seconds
and verifies the individual collection counters and all eight original rock
slots. The source Sunny save remains unchanged. The ten scenario-parser unit
tests retain all eight prior cases and add bounded inventory IDs/values, packed
field reads and rejection of incorrect gem/count expectations.

Keep the preceding
177,048-update scenario unchanged: when native functional sources have not changed,
its verified save can start this chapter without replaying the completed prefix
for every extension. This reduces repeated integration work without dropping any
prior scenario, assertion or reference-comparison domain. Functional changes require
the affected regressions; broad native batches also require the full comparison.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/postgame.json --save PATH_TO_PRIVATE_SUNNY_OPTIONAL_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_MATERIALS_PROFILE

The continuation receives the Pooie Earrings recipe, selects that accessory with
all requirements available and commissions it for twenty seeds and the four
required gems. A further workshop interaction retains the paid accessory and
seed balance.
Ta-dah at the Wildwoods palace delivers it and obtains the Star Key. The tower's
key door opens normally and its complete rescue teaches Swellie. Original
Save/Continue retains map 9:1 at x=104.5, z=96.5 with 65 words, two seeds,
five rocks, eleven loves and Bijou. Checkpoints retain the key, recipe, word and
ending flags, the three zero spent-gem counters, the remaining Crystal, the
undiscovered 255 counter and all eight original packed rock counts. The artisan
branch does not invoke the separate manual-crafting failure/refund handler.

The expanded chapter retains every earlier update and checkpoint: 102,283 SDK
updates and twenty checkpoints pass in 265.29 seconds, followed by 107,506
host-session checks in 271.68 seconds after connecting VCOUNT. SDK, host and scout match all 32 KiB of
SRAM byte for byte. A fresh SDK reload and Status inspection pass 2,103 updates
in 3.163 seconds, checking the saved flags, collection counters and packed rock
counts. Both source profiles remain unchanged. The ten scenario-parser tests,
eleven CTest suites and app smoke test also pass. The rerun retains every input
and expectation. A read-only dictionary audit confirms 65 unique identifiers,
exact agreement with learned flags and no missing word required by the Sunny
Peak destination indicator. This does not establish complete optional activities.

The other 21 words, the higher love meter needed for the remaining rocks,
manual crafting/resale branches, other activities and the complete desktop
playthrough remain unverified.

`wardrobe.json` is a separate chapter starting from the verified final
`postgame.json` save. It checks Sunny Boutique's insufficient-funds rejection,
cancel/return, four ordinary five-seed pickups, both Antlers colors at ten seeds
each, owned-color rejection, Corsage/Antlers row navigation and original
Save/Continue. The original thirteen checkpoints include prices, seeds, ownership
bytes and the Outfit counter: one count per acquired color. Existing
Sunny optional and postgame scenarios retain every earlier input/checkpoint.

The first photo extension retains all 37,274 preceding updates and thirteen
checkpoints, then adds dressing and two saved photos. Hamtaro's brown and
Bijou's white Antlers are stored in slot one with setting 1. Removing Hamtaro's
worn hat leaves the photo and purchases intact. Title Dress Up reloads both
stored colors; a second photo swaps them and uses setting 2 in slot six.
Declining to erase slot one preserves both photos. Continue restores the earlier
story, and another original Save/Continue retains both records and all four
unused slots. All 73,776 updates and 29 semantic checkpoints remain unchanged
in the current chapter.

The current extension retains every preceding update/checkpoint, then adds 45,123
updates and 22 checkpoints for Corsage/Kerchief purchases and all three worn parts.
It checks declined purchase confirmation, the exact three/five-seed costs, attempted
Corsage sharing, cancellation after accepting photo replacement, and replacement
of slot one while the other five slots remain intact. Toss rejection retains
ownership; confirmed Toss removes one variant and decrements the counter once;
an empty-category retry cannot underflow it. Current selections clear the missing
item while the stored photo keeps it. Remove clears the worn head/body without
deleting purchases or photos. Title Dress Up and original Save/Continue check
the interaction between stored photographs and a discarded garment.

All 118,899 SDK updates and 51 checkpoints pass in 211.57 seconds, followed by
123,990 host-session checks in 214.76 seconds. SDK, host and input-only scout
match all 32 KiB of SRAM. A fresh SDK process passes 2,103 updates in 3.019
seconds, including Status, all six photo slots, ownership/count fields and the
earlier gem/rock assertions. The endpoint is map 1:3 at x=144.5, z=48.5 with
65 words, four seeds, five rocks, eleven loves, Bijou and three owned variants.
Corsage is discarded, Kerchief retained, and the replacement photo still includes
the Corsage. All source profiles remain unchanged. Fifteen scenario-parser tests
and all eleven CTest suites pass, including the actual application smoke test.

The extension changes only scalar checkpoints and recorded input. Native source,
application and probe hashes are identical to the complete 5,793,256-check
HBlank/window boundary, whose function and timed-pixel checks remain applicable.
Only ordinary buttons and read-only scalar assertions are versioned. Additional
clothing purchases and full desktop activity coverage remain unverified.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/wardrobe.json --save PATH_TO_PRIVATE_MATERIALS_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_WARDROBE_PROFILE

The independent `wardrobe.json --route gallery` continuation uses the completed
Moon-crafting save. Its 18,461 updates and 19 checkpoints check default-No
replacement rejection, frame preview with Start, all four forward/backward
transitions between the two unlocked settings, and cancellation after replacement
consent. The original "Erase this photo?" prompt guards replacement; accepting it
keeps the old record until the new photo is committed. A new photo in empty slot
two keeps slots one and six and the three remaining empty slots unchanged.
Reopening the new slot also checks its default-No replacement guard.

The final original Save/Continue and fresh host reload retain all six records,
the three owned outfit variants, four accessories, all gem/rock counters, 86 words,
21 loves and four seeds. Scout, SDK and host SRAM match all 32 KiB; both group-three
mirrors and all 51 retained choreography records are checked independently.
The SDK replay passes in 35.07 seconds, followed by 20,791 host-session checks in
37.35 seconds and a separate 2,103-update fresh-process/Status check in 3.345 seconds.
All earlier wardrobe inputs/checkpoints and the native sources, executable and
probe remain unchanged. The existing full core comparison proof remains applicable;
this route does not establish interactive Windows or physical-pad behavior.

The same packaged executable also passes an actual-window review with a separate
private profile. Its controls panel assigns A to C and Select to Left Shift;
C confirms in the game while the old Z binding does nothing. Status renders
86 words, 21 loves and four seeds. Original End Game writes a complete 32 KiB file
and keeps the exact preceding save as its backup. Title Dress Up renders photos
one, two and six and the three empty slots. F11 switches the game and frontend
between a 960x720 client window and 1920x1080 fullscreen, with centered integer
scaling and the original game aspect ratio.

The application closes normally while fullscreen. A new process visibly restores
fullscreen, both changed keys, deadzone 40, volume 37, mute and disabled
pause-on-focus-loss. F11 restores the previous window size. After the second normal
closure, an independent native process reloads the desktop-written SRAM, checks
the complete gallery/inventory/story checkpoint and reopens Status. Source profiles
remain unchanged. The test is muted and makes no new audio-listening claim.
A complete interactive desktop playthrough remains unverified; physical-controller
testing is left to the user at their request.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/wardrobe.json --route gallery --save PATH_TO_PRIVATE_MOON_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_GALLERY_PROFILE

`optional_words.json` starts from the final wardrobe save. Its 22,472 updates and
13 checkpoints cover Zuzuzu, Delin-Q, Lost-T and Koochi-Q, increasing the dictionary
from 65 to 69 words. This previously verified chapter remains byte-for-byte unchanged.
Its SDK replay takes 35.87 seconds and 25,097 host checks take 39.16 seconds;
scout, SDK and host SRAM agree across all 32 KiB.

`family_reunions.json` starts in a fresh process from that 69-word SRAM. Its 59,504
updates and 16 checkpoints cover the Haunted House ghost reunion and Fun Land's
missing-child reunion. The 57,582 post-boot updates remove 21.47% of exploratory
inputs while retaining both reunions and saved progress. A matching scene and
scalar frame counter after an in-process Continue does not establish identical
gameplay/RNG history. The original door assertions therefore use this explicit
fresh-process boundary; no RNG or progression state is injected.

The route checks the silver-card lift and hidden passage, the original door-game
confirmation and ghost result, Dingding's lesson, the child's ride confirmation,
completed ride, Lost-T response, scare sequence, Trust-T lesson and thirteenth
reconciliation. The dictionary advances from 69 to 71 words. The love meter moves
from progress 30 through 33 to 35, retaining level two; these fields represent
love-meter strength rather than the number of Ham-Jams.

The SDK replay passes in 98.12 seconds and 63,449 host-session checks pass in
102.01 seconds. Scout, SDK and host SRAM match byte-for-byte. An independent
2,103-update fresh-process check takes 2.983 seconds, verifies all 71 unique
word IDs against learned flags and reopens Status. All eleven CTest suites
(0.86 seconds) and fifteen scenario-parser tests (0.040 seconds) also pass.

Initial and final checkpoints retain all six photo slots, clothing ownership/count,
packed rock quantities and collection counters.
Source saves remain read-only. Native source, application and probe hashes match
the complete 5,793,256-check HBlank/window boundary, so its function-level proof
still applies. The extended route, independent save/reload checks and quick
regression suites cover this input-only change.

`fun_land_optional.json` starts from that 71-word save. Its 48,913 updates and
15 checkpoints cover Offdoff, Howdy and Blash-T coaching, the two original
confirmations, three No-P responses, Minglie's lesson, the Tea Cups reunion and
Twirlie's lesson. The 46,991 post-boot updates remove 19.92% of exploratory input.
Original Save/Continue retains 73 words and fourteen loves; meter progress rises
from 35 to 39, retaining level two. Initial and final assertions retain all six
photos, clothing flags/count, collection counters and packed rock quantities.
The earlier optional-word and family-reunion chapters remain byte-for-byte
unchanged. Native source and both binaries still match the complete
5,793,256-check reference boundary.

The Tea Cups SDK replay passes in 86.51 seconds; 52,459 host-session checks pass in
88.80 seconds. Their 32 KiB SRAM matches the canonical scout byte-for-byte. An
independent 2,103-update reload takes 2.908 seconds, confirms all 73 unique words
against learned flags and retains the inventory and photos when reopening Status.
The eleven CTest suites (1.54 seconds) and fifteen scenario-parser tests
(0.110 seconds) also pass.

The same file contains `routes.ferris`, an independent saved continuation selected
with `--route ferris`. Its 27,236 updates and ten checkpoints cover Pushie, the
stuck date's Hamha conversation, Tuggie, Gossip-P, Koochi-Q, Smoochie, Luck-E,
the fifteenth reconciliation, the park announcement and original Save/Continue.
It starts from the verified 73-word Tea Cups save and ends with 74 words, fifteen
loves and meter progress 41 at level two. It retains all six photos, clothing
flags/count, collection counters and packed rock quantities. The 25,314 post-boot
updates remove 23.31% of its exploratory inputs.

Each named route has its own complete input/checkpoint list and requires its
documented starting SRAM. Selecting one preserves its semantic assertions and
host save checks. All route definitions are validated, including unselected
routes; an unknown selector fails rather than falling back to the default route.
`--route` requires JSON `--scenario`; `--until` applies within the selected route.
The default Tea Cups route retains all 48,913 updates and fifteen semantic
checkpoints, with corrected attraction labels. Its prior gameplay proof remains
applicable because those inputs, expectations and native binaries are unchanged.
Twelve words and the other documented optional/desktop activities remain pending.

The independent Ferris SDK replay passes in 50.40 seconds and 30,152 host checks
pass in 53.30 seconds. Both match the canonical scout's full 32 KiB SRAM. A fresh
2,103-update reload takes 2.959 seconds and checks all 74 unique word IDs against
learned flags, the retained inventory/photos and Status. The eleven CTest suites
(1.11 seconds) and all nineteen scenario tests (0.049 seconds) pass, including
selection isolation, unknown/malformed routes, retained semantic failures and
rejection of `--route` with external input schedules.

`routes.performers` starts from the 74-word Ferris save and covers the complete
connected performer search in 81,465 updates and ten checkpoints. Its 79,543
post-boot updates remove 12.97% of exploratory inputs. Assertions retain the
host's search state, both park artists, the Haunted House discovery and
conversation, the partner's invitation, the restroom message and the automatic
theater performance. The reconciliation
teaches Hardihar, raises the love-meter level from two to three (progress 41 to
43), and retains 75 words and sixteen loves through original Save/Continue.
Initial and final assertions retain all six photos, clothing flags/count,
collection counters and packed rock quantities. The earlier default and Ferris
routes remain unchanged. Eleven words and the other documented optional/desktop
activities remain pending.

The performer SDK replay passes in 125.98 seconds; 85,746 host-session checks
pass in 128.79 seconds. Both produce the canonical scout's exact 32 KiB SRAM.
An independent 2,103-update reload takes 2.915 seconds, checks all 75 unique words
against learned flags and retains inventory/photos when opening Status. The
eleven CTest suites (1.18 seconds) and nineteen scenario tests (0.041 seconds)
pass. Native sources/binaries are unchanged, preserving the complete
5,793,256-check reference boundary without rerunning identical comparisons.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/optional_words.json --save PATH_TO_PRIVATE_WARDROBE_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_OPTIONAL_WORDS_PROFILE
    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/family_reunions.json --save PATH_TO_PRIVATE_OPTIONAL_WORDS_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_REUNION_PROFILE
    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/fun_land_optional.json --save PATH_TO_PRIVATE_REUNION_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_PARK_PROFILE
    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/fun_land_optional.json --route ferris --save PATH_TO_PRIVATE_PARK_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_FERRIS_PROFILE
    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/fun_land_optional.json --route performers --save PATH_TO_PRIVATE_FERRIS_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_PERFORMER_PROFILE

Checkpoints can include `"flags": {"0x77": true, "0x198": false}` and
`"controls": {"0x83": 3, "0x02": 0, "0x1A6": 12}`. A checkpoint may also assert
`"classes": {"0xE1": 630}` for the musical challenge's score. IDs are decimal or hexadecimal
strings, unique after parsing. Flags address the 96-byte game flag block (0 through
767) and require boolean values; controls address the 512-byte control block (0
through 511) and require integer bytes (0 through 255). Classes address the first
256 scene halfwords and require integers from 0 through 65,535.
`"record_counters": {"29": 2}` reads the 32 collection-counter bytes; 255 means
undiscovered and stays distinct from zero. `"rock_counts": {"0": 0, "63": 31}`
reads the remaining-polish field of the 64 packed rock slots, excluding the key
and other packed bits. These mappings require unsigned integer bytes and counts
from 0 through 31 respectively. `"rock_keys": {"12": 0}` independently checks
the seven-bit rock type (0 through 127). Type zero is valid and must be paired
with a nonzero count when checking the fixed special rock; unrelated high flags
do not enter either field. `"catalog_flags": {"0": 255}` reads the first
ownership byte of each SaveGroup3 category (IDs 0 through 109), and
`"catalog_count": 2` reads the Outfit acquisition counter. Both require unsigned
bytes; the counter counts acquired variants, not completed categories. All mappings are nonempty
read-only assertions. Malformed/out-of-range IDs, numeric aliases and incorrect
value types fail validation; booleans are rejected as numeric values. Deliberately
incorrect flag/control expectations are also rejected by ROM-backed tests.

`"outfit_indices": [0, 0, 0, 1, 0, 0]` asserts the six active catalog indices,
ordered as Hamtaro's head/item/outfit followed by Bijou's. Each is an unsigned
byte. `"photo_records"` maps unique string slot IDs (0 through 5) to objects with
`"background"` and `"outfits"`. The background is one byte; outfits are six
category/variation byte pairs in the same character/part order. Background 0
denotes an unused slot; category 255 denotes no selected item. The checker reads
only the thirteen defined record bytes and excludes its three padding bytes.
These are read-only scalar metadata assertions, with no resource data or state
injection. Invalid layouts/types, aliases, truncated lists and color/background
mismatches are rejected by the asset-free scenario tests.

Original Continue checks word flags, ball acquisition/delivery, team progress and
the final rescue/bottle state. A separate SDK process reloads the host-written
SRAM and checks the challenge, rescue, word and bottle flags, scene controls, map,
position and counters in 1,922 updates.

`--until "CHECKPOINT LABEL"` stops after exactly one named top-level checkpoint,
while omitting it retains the entire scenario. An unknown or ambiguous label fails.
The 7,310-update Bedroom control prefix is independently checked. Use a checkpoint
after an original in-game save when combining this option with `--session-test`,
because that test compares the persisted state after restarting. This can produce
a private reusable profile at the cart or Clubhouse milestone without repeating
all later steps during focused development.

External reference schedules can be read directly with `--input-schedule PATH`
instead of `--scenario`. Each non-comment line is a decimal frame count, a
hexadecimal GBA button mask and a label (`-` omits the label). Blank lines and
`#` comments are accepted. Zero-frame labels observe state without advancing the
game; unlabeled zero-frame lines are no-ops. Invalid masks, duplicate labels,
negative durations and malformed lines fail before execution. The same two-million-update total
budget applies to text and JSON, including repeated steps. `--until LABEL` can
stop a text replay at a unique observation after the entire input is validated.

Text labels print native observations, not semantic assertions or original-ROM
parity. JSON `expect` checkpoints retain strict scalar/flag/control assertions.
`accessory_records` additionally checks the thirty original accessory byte slots;
string slot IDs must denote unique integers from zero through twenty-nine, with byte values.
This can distinguish material rejection, successful storage, failure cleanup and
saved continuation instead of inferring an accessory from the seed balance.
Both formats execute every requested update with the same native graphics/audio
path and stop on faults; no renderer, audio, comparison domain or game update is
skipped. `--save` and `--session-test` work with either format. Keep external
reference files and all saves outside this checkout. Run the asset-free parser
and malformed-input checks with `python -B tests/unit/scenario.py`.

The Spanish reference prerequisite has now been reconstructed with normal native
inputs: erased-SRAM tutorial/save (31,136 updates), fresh Continue and five-seed
pickup (5,103), pickup persistence and word acquisition (14,448), then a fresh
word-persistence/MonTak route (17,940). Only original 32 KiB SRAM passes between
the four fresh runtimes. The last route also passes 20,466 host-session checks,
including exact SDK/host SRAM equality and fresh continuation at map 2:2,
x=288.5, z=111.5, 13 words and 15 seeds. The English progress profile is separate.

The reference's verbatim longer Spanish story sequence diverges before the
Pepper/Oxnard reunion. A replay of the original ROM from the same native-produced
SRAM shows position/timing differences first at the vine descent and later at
the return transition. Map-only landmarks did not detect the earlier drift.
Aligning the approach to Pepper with normal movement inputs then passes the
first reconciliation, original save/Continue and Clubhouse return in Spanish.
The next divergence is a later map transition on the training-seat delivery
route. Further input adjustments and semantic checks are required before reusing
the remainder. These observations do not establish a missing native handler or
completed native credits. The full reference schedule is planning material,
not proof that all 443 original checkpoints pass natively.

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
It adds two profile-export checks to the default replay's count. The Python runner
also compares the exported 32 KiB file with the SDK replay's SRAM byte for byte,
printing its SHA-256 only after equality succeeds. Matching counters alone are
insufficient. This reuses the two required replays and needs no third full replay.
The asset-free parser suite checks missing files, wrong lengths, equal-length
single-byte corruption, exact equality and read-only verification.

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
game_record_transactions.c have explicit entry/helper coverage. The 49,944 focused
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
The added 7,443 checks execute all 80 retail recipes with all four original random
outcomes, comparing payment and resolution against ARM and checking material and
accessory conservation explicitly. The 320 chains produce 172 successes,
136 ordinary failures and 12 refunds spanning all six special materials. Fixture
PRNG states are selected with the original isolated ARM routine; no game session
or player save is loaded or modified. This focused run passed in 22.178 seconds.
Complete workshop/activity progression remains a separate integration requirement.

The complete clothing-catalog family can run separately:

    python -B tools/compare_reference.py --pair-lists-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

This passes 204,233 checks in 12.079 seconds. All 40 functions in
script_callbacks_pair_list.c have reviewed contracts: 18 Main entries, 26 F4
targets and 13 typed helpers, with overlaps. Checks use all six lists and five
languages from the private ROM, every flag/mask combination, raw byte selectors,
counter and sentinel boundaries, palette queues, live aliases and actual Main/F4
commands. Void animation presets and argument-dependent helpers are rejected by
the wrong calling interface. The checked render-number guard remains covered
alongside the explicit raw-slot adapter. Unterminated lists fail explicitly.
Complete clothing, dressing and photo activity coverage remains separate from
these function-level comparisons.

The Main memory and scene-transition group can also run separately:

    python -B tools/compare_reference.py --scripts-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

This passes 133,746 comparisons: byte/halfword pointer copies, zero/max counts,
variable indirection and overlap, graphics disable-except, display-layer flags,
and 192 actor snapshot/modal-map roundtrips across six map callbacks. Cases include
optional/absent actors, all direction variants, retained cursor and first/third
variant reuse for the fourth restored actor. A hidden primary actor retains the
original unsigned-byte subtraction to selector 255; the adapter reads the guest
animation table instead of rejecting that valid Bijou-only state. Input data is synthetic; original
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

    python -B tools/compare_reference.py --motion-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --bus-oracle PATH_TO_HN_BUS_ORACLE

This passes 16,095 private comparisons with the independent bus oracle. In addition to motion setup/routes, it
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

The collision subset with both oracles passes 107,577 private comparisons in
2.292 seconds, including 216 independent
angular responses. It covers mask classification, sample pointers, shape containment,
interaction queries, solid-record responses, record lists, map flags/scripts,
action/reset controls, leader movement, combined updates and Main's movement wait.
Positive-response assertions require both circle and rectangle contacts. The full
comparison requires --bios-oracle; other focused suites can run without it.

The isolated bus oracle executes mGBA's halfword-load implementation without a
ROM, BIOS or CPU instruction loop. It checks the three reviewed ROM prefetch sites
used by collision-record scans, with even/odd addresses and every byte-sized
render count. All 27,648 list scans retain RAM and terminate at the same bus-derived
index, including a preceding mapped inactive record. A separate private input-only
replay reaches map 2:12 after the oilcan sequence; the original mGBA game also
continues from the same SRAM and retains the same animation-derived list. The
adapter covers those reads above 0x0FFFFFFF and unassigned I/O above 0x040003FF.
The temple-cart return independently retains 0x0480011C as its first list entry
in the original map 2:7 snapshot; its halfword index is a prefetched instruction,
so the original scan stops before reading record fields. New cases retain all
6,144 previous scans and cover both alignments, the extended memory-control
boundaries and the debug-enable boundary. The assigned extended memory-control
and debug-enable registers remain unsupported by this local adapter; unit checks
require their explicit faults. Ordinary memory reads are unchanged. Other floating-bus sites and detailed
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

These pass 51,514 scene actor comparisons, 95,870 actor preset comparisons,
228,477 record/local-SaveGroup4 comparisons and 756,707 scanline-wave/VCOUNT comparisons
when the independent video oracle is supplied. The scene group
covers 45 functions across 16 families and one shared bank-3 condition, including
approach completion, signed limits, all ARM shift counts, direction constructors,
snapshots, copy aliases and the three-byte render-selection Main callback. Its
primary/secondary paths cover every byte selection, inactive mapped records and
arguments overwritten by slot reset. Its 41,470 previous comparisons remain and
the complete one-function actor_animation_mirror.c family adds 10,044. The focused
scene group takes 3.171 seconds. Reflection checks cover both 23/18-entry mutable
lookup tables, matched/changed/retained animation offsets, both fallbacks, wrapped
Z and baseline boundaries, the companion branch, every byte slot index, inactive
mapped slots and table/control/class aliases. The animation ABI returns zero
consumed bytes and a full-word yield of one; Main and task calls reject it.
The fixture compares all EWRAM and game IWRAM without the original ARM call stack.
Public synthetic checks also cover the last primary row, retained flags/offsets,
fallback clearing and companion overflow. No original animation tables are embedded.
The preset group covers all 498 functions in six
families, Main rejection of void callbacks, F4 dispatch, direction sectors,
captured auxiliary arguments versus live selector aliases and inactive mapped
byte selectors at 0, 1, 254 and 255. Record checks cover
allocation/empty/full slots, twelve-byte result views, consuming zero counts,
sentinel counters, arithmetic wrap and allocation callbacks. Batch allocation checks
cover all 65 occupancy values, zero/full capacity, amount and animation timing
boundaries, inactive mapped slots and byte-wise argument aliases. The record group
retains its 30,930 prior comparisons and adds 27,779 for the five shared offline
SaveGroup4 Main callbacks, taking 4.313 seconds in the focused run. These check
both context/control copy directions, every byte value and record column, all
14 ROM descriptors, clear-before-copy and forward aliases, retained headers,
available-word flags, every classification mode and category-rank thresholds.
Payloads and category tables come from the player's ROM. The compared RAM covers
all EWRAM and 0x7000 bytes of game IWRAM; the original ARM call stack is outside
this serialized callback contract. This supports music hall entry without
implementing link transport.

The same record group adds 61,682 comparisons for all twelve
game_record_messages.c functions and the Main51 counter-prefix assembly leaf.
Message checks cover all 256 record selectors in five languages, all 193 original
bundles, special initials, default scripts, wrapped output indices, source/output
and live-argument aliases, eleven void presets, follower gates, timers and actual
Main/F4 dispatch. The assembly leaf covers every byte value at all 32 positions,
including the read-but-excluded final byte; ordinary Main dispatch rejects it.
Undefined message selectors fail explicitly. That 120,391-check portion remains
enabled; its earlier focused run took 8.333 seconds.

The record group adds 108,086 comparisons for the complete 18-function consume,
navigation, counter and caption families. Cases cover all sixty original record
descriptors and five languages, exhausted/partial/forced consumption, accumulator
insertion/full tables/count wraps, the original catalog's graphics, signed navigation
and sentinels, every occupancy count, all 81 caption bundles, wrapped titles,
two-context removal, packed tile pairs and live output aliases. Typed Main/F4
commands run the callbacks and reject inappropriate argument-dependent or void
contracts. The full focused group passes 228,477 checks in 13.872 seconds.
All EWRAM, game IWRAM, palette and VRAM are compared. I/O comparison excludes
only the synchronous fill-DMA source address, which points at a transient ARM
stack word; destination/control remain checked, as in the existing task tests.
Rejected consumption uses the explicit zero-initialized local described in
architecture.md. The original ROM file is unchanged; synthetic kind/index cases
modify and restore only the two private in-memory engine fixtures.

Wave checks cover
all 228 scanlines, blend thresholds, signed wrapping, frame-end phase updates and
typed Main/F4/HBlank calls. Public synthetic game tests cover these boundaries
without original assets. The additional clocked pixels are covered by the complete
HBlank/window group below; these function checks alone do not establish full-area
progression or complete inventory UI/SRAM integration.

The same `--wave-only` group includes ten connected VCOUNT functions: the shared
enable setter and IRQ/noop, both complete blend/increment families, and the two
pair-list IRQ controls. It retains every byte selector/scanline pair with both
pending-bit paths, the complete 16-bit status and increment domains, arbitrary
transition phases, original dirty flags and five typed Main/F4 contracts. The
VCOUNT transition phase is separate from the wave phase. An already enabled
compare change alone retains the original pending-bit behavior. Native runtime
tests check VBlank/HBlank/VCOUNT priority and invalid selectors. Fault-save tests
still require a failed frame to leave committed SRAM intact; their unsupported
IRQ is now bit 3 because VCOUNT bit 2 has an implementation.

With `--video-oracle PATH_TO_HN_VIDEO_ORACLE`, this group also advances the actual
native game clock through nine synthetic frames without Main. All pixels match
independently rendered mGBA bands before and after line 104, including initial
phases zero, one and 255 and subsequent frame wrap. The focused group passes in
5.470 seconds. These checks establish the selected register/line/pixel contracts;
they do not model original CPU instruction latency or complete list-activity
progression. All previous wave comparisons and full-suite oracles remain enabled.

    python -B tools/compare_reference.py --wave-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --video-oracle PATH_TO_HN_VIDEO_ORACLE --oracle-runtime-dir PATH_TO_ORACLE_RUNTIME_DLLS

The `--scanlines-only` group compares all nineteen HBlank selectors, eighteen
distinct table targets and 26 connected functions. It includes complete global
window/step/circle/radius and wave-setup families, the remaining graphics-script
raster kernels, four Main contracts and two valid F4 contracts. Every real line,
full byte phase/boundary domains, zero periods, radius increments/fractions,
pre-allocation heap order, pointer aliases and wrong calling contracts are checked.
Inactive ARM scratch is explicitly defined for the radius helper's unwritten
first byte, including nonzero retained-value cases. This is a defined adaptation,
not evidence for arbitrary stack-history equivalence.

With the independent video oracle, the group passes 232,056 checks in 16.805
seconds. Forty-six native frames compare six scroll/window/blend registers at
every visible line before comparing every pixel. Original callback execution
supplies the register timeline; the separate mGBA PPU consumes it through the
oracle's optional `--scanlines` protocol (160 rows of six little-endian halfwords
after the usual static snapshot). The ordinary static protocol is unchanged.
Cases include repeating HBlank DMA before IRQ delivery, frame wrap, visible
per-line alpha, multiple stepped widths, effect cleanup and combined VCOUNT/window
phases. This models boundary ordering, not original ARM instruction latency.

mGBA 0.10.5's optimized 4bpp flipped-tile suffix reads the wrong nibble when the
window ends before the source tile does. A minimal one-pixel window at x=239,
scroll 2 and flipped tile with nibbles 1..8 selects index 7; native returns red 57,
whereas that oracle returns red 8 (index 1). The earlier forty clocked frames
retain flipped maps. Six additional completion frames use unflipped maps to keep
their pixel comparison independent of that defect. Native analytical regressions
retain both flip states, all eight scroll phases and window widths one through
eight, with no skipped output pixels. The external mGBA source is unchanged.

    python -B tools/compare_reference.py --scanlines-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP --video-oracle PATH_TO_HN_VIDEO_ORACLE --oracle-runtime-dir PATH_TO_ORACLE_RUNTIME_DLLS

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
the early-wait SL distinction. The recovered 0x8D call at 0x088DC6D4 also requires
the argument-free MapBank4ReturnZero6BAD4 animation binding. The family verifier
compares its consumed count, all 32 yield bits and complete memory across 128
fixtures against ARM; public regressions reproduce the former callback fault,
check both address forms and continue at exactly the next command after five
bytes. The typed census now includes 17 animation contracts. Full bank-4
story/activity and timed desktop integration remain pending.

After this binding correction, the focused dynamic family passes 263,508 checks
in 48.853 seconds, retaining its previous domains and adding 1,280 animation
comparisons. All twelve CTest suites pass, including 847 renderer checks. This is
current focused evidence; the historical complete-suite count above was not rerun
or relabeled as a new full run.

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
behavior and background setup. These families run in the full comparison.
The separate tower scenario above now completes the sliding puzzle; full final
area/story integration remains pending.

The complete six/eight-choice room family has a focused command:

    python -B tools/compare_reference.py --scene-selection-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

Its 61,195 comparisons cover all 13 source entries, ten Main and three void task
contracts, actual Main/F4 command calls and rejected interfaces. Cases include all
byte bank/selection inputs, both probability groups, random substitutions, the
fixed result's unchanged PRNG, all special values and slot states, live constructor
aliases, four-byte record strides, a matching empty record before an occupied
duplicate in slot 63, and all counter values. The focused run takes 1.939 seconds.
These checks also run in the full comparison. Complete room/activity progression
remains a separate integration requirement.

The independent `quarry.json` continuation supplies normal-input
integration for both shared quarry rooms in the Star Key region. It starts from
the original postgame save with five rocks, collects all six outer positions and
all eight inner positions, exercises both sides of the ledges, and checks that
leaving/reentering an exhausted room does not reshuffle or duplicate its records.
Each pickup checks the result type, remaining polishes, exact destination slot
and retained old inventory. Original Save/Continue retains all nineteen rocks,
65 unique words, eleven reconciliations, two seeds and Bijou.

The 26,406-update route has 30 checkpoints. SDK replay passes in 42.03 seconds;
29,498 host-session checks pass in 44.25 seconds. Scout, SDK and host saves agree
across all 32 KiB. A separate 2,103-update reload passes in 2.649 seconds and
reopens Status. Twenty-one parser tests include key-zero handling, packed-field
separation, rejected truncation/aliases and detection of an incorrect rock type
with the correct polish count. Previous postgame routes and all native source,
executable and probe hashes remain unchanged; existing native comparisons are
reused. Alternate pillar Ham-lift actions and other regional presentations remain
outside this route.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/quarry.json --save PATH_TO_PRIVATE_STAR_KEY_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_QUARRY_PROFILE

The independent `quarry_lift` route starts from the same five-rock prerequisite
and covers all four pillar Ham-lift choices before Tack-Q. It checks zero-to-two
selection transitions, one allocation per action, retained old records, result
types and polish counts, no seed charge, and the original Save/Continue result.
The fixed special rock and ordinary ground pickup remain available. Its 10,220
updates and seven checkpoints pass in 16.46 seconds; 12,550 host-session checks
pass in 19.03 seconds. Scout, SDK and host agree on all 32 KiB. A separate fresh
process reloads and opens Status in 2,103 updates and 2.735 seconds. All native
source, executable and probe hashes are unchanged, so existing native comparisons
are reused. No complete story or desktop replay is required for this input-only
addition; other regional presentations are outside this focused route.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/quarry.json --route quarry_lift --save PATH_TO_PRIVATE_STAR_KEY_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_QUARRY_LIFT_PROFILE

The local music/record presentation and selection families have a focused command:

    python -B tools/compare_reference.py --group4-only --rom PATH_TO_YOUR_AH3P_ROM --reference PATH_TO_READ_ONLY_DECOMP

Its 135,367 comparisons account for all 31 record/presentation/selection/title entries,
24 composer Main contracts and three text/boundary helpers. Checks cover byte selectors, all
14 original song descriptors and restart, raw resource readers, affine allocation
and exhaustion, render/base/control aliases, suffix collisions, depth/number
boundaries and both SRAM copies. Audio fixtures synchronize VCOUNT after the
oracle's initialization wait and compare the baseline before calling the selected
song callback. Composer checks include all five languages, balanced task creation
and removal, all tile-ring byte values, fractional scrolling in both directions,
six scroll modes and unsupported-mode behavior, empty and terminal resource
boundaries, glyph replacements and multiline heap cleanup. The focused run takes
31.125 seconds. Title checks also reset all 51 records in five languages, retain
untouched SRAM regions and record padding, repeat the reset, test every reset-disable
byte and aliased argument, and invoke both callbacks through Main. All previous
five local group4 dependency comparisons remain in
the full suite; link transport and complete music-activity progression are
separate requirements.

`menus.json --route title_reset` starts from the verified Peace Moon crafting
save. Its 10,113 updates and 12 checkpoints retain the original menu route and
check New Game's default No, both overwrite safeguards, reset-time locking,
all 51 initialized local records and the original A+B+Start+Select soft reset.
The final saved state legitimately resumes at naming with zero learned words;
host-session verification compares the complete restored snapshot rather than
requiring an arbitrary minimum word count. Original progress assertions remain
in the versioned scenarios. Scout, SDK and host match across all 32 KiB. The SDK
passes in 18.18 seconds; 13,841 host checks pass in 21.06 seconds. A separate
1,922-update fresh process passes in 2.581 seconds without opening field-only
Status from the naming screen. The starting story profile stays unchanged.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/menus.json --route title_reset --save PATH_TO_PRIVATE_MOON_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_TITLE_PROFILE

The independent `menus.json --route inventory` continuation uses the three-photo
gallery save. Its 29,171 updates and 97 checkpoints cover all 30 Gems positions,
15 encountered descriptions including zero quantities, 15 rejected unseen Read
actions and both list wraps. It visits every A-Box cell, including the four crafted
accessories and all empty slots, both column wraps, the lower Toss control and
its inactive horizontal navigation. Cancelling Toss for each occupied slot and
retrying an empty slot retain the entire collection. Original Save/Continue
restores the workshop actors and all inventory, clothing and photos.

The SDK passes in 56.15 seconds; 32,035 host-session checks pass in 59.13 seconds.
Scout, SDK and host SRAM match all 32 KiB. Both group-three payloads and all 51
choreography records remain identical to the starting save. A separate 2,103-update
reload passes in 2.915 seconds, checks all 86 unique words against flags and
reopens Status. Twenty scenario-parser tests pass. Previous menu/title-reset
inputs and checkpoints, native sources, executable and probe remain unchanged;
the complete core comparison proof is retained.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/menus.json --route inventory --save PATH_TO_PRIVATE_GALLERY_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_INVENTORY_PROFILE

The independent `menus.json --route languages` continuation starts from that same
gallery prerequisite. It checks all five language rows, both clamped boundaries
and cancellation before confirming each language. Every language then runs
Continue, localized Status entry/exit and original End Game. It returns to
language zero and reloads the workshop with all 86 words, 21 reconciliations,
four seeds, all 32 counters, 64 rock slots, 30 accessory slots and six photo
records checked; the three populated photos and clothing remain intact.

The 24,237-update route has 32 checkpoints. SDK replay passes in 36.53 seconds;
26,867 host-session checks pass in 39.59 seconds. Exploratory, SDK and host saves
agree across all 32 KiB. Both group-three payloads and all 51 choreography records
remain unchanged from the source. A separate 2,103-update fresh process passes
in 2.985 seconds and reopens Status. Twenty parser tests pass. Previous routes
and all native source/executable/probe hashes are unchanged from the bank-4
animation correction; its focused proof is reused. The earlier complete-suite
count remains historical. No complete story replay or gameplay state/RNG writes
are used. Translated scene coverage and desktop visual review remain separate.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/menus.json --route languages --save PATH_TO_PRIVATE_GALLERY_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_LANGUAGES_PROFILE

The input-only continuation `sandy_bay_optional.json --route ham_jam` covers the
unmodified beginner rejection, title-menu composition, a changed Ham-Chat in
“Dizzy for You”, an empty-slot save, return to the story, custom-slot selection,
the paid entry and successful beginner result. Ta-dah delivers the prize to the
captain and triggers the Sandy Bay shop reunion and Hambond. The route contains
81,168 updates and 34 checkpoints, ending after original Save/Continue with
79 words, 19 rescued loves, two seeds and progress 51. All 32 collection counters
and 64 rock counts are asserted at story checkpoints; title/composer screens have
unloaded story state. Initial/final checkpoints also assert the complete catalog
and all six photos. Its prerequisite is the saved `haunted_house.json --route music`
continuation. The existing default Sandy Bay route remains unchanged.

The SDK replay passes in 134.68 seconds; 86,004 host-session checks pass in
133.82 seconds. Their complete 32 KiB SRAM matches byte for byte, and an
independent 2,103-update fresh reload passes in 2.967 seconds with all 79 unique
word IDs matching their flags. The split exploratory scout differs in two retained
auxiliary render-attribute bytes per SRAM copy and the associated group-one
checksum; all choreography records match. The uninterrupted canonical SDK/host
proof compares every byte without normalization. A separate actual Windows-window
check opens Ham-Jam, selects “Dizzy for You”, changes its second beat to Bestest
and saves it as Hamtaro in an empty second slot, reaching “Save complete”. This
uses an isolated private profile, mouse input and the default WASD/Z/X/Enter/Escape
bindings. The controls panel identifies Computer Use's injected `Right` as
`Keypad 6`; the original Right binding is restored after this diagnostic. Physical
arrow keys, physical controllers and the complete desktop activity remain outside
this check. The single test window is closed afterwards.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/sandy_bay_optional.json --route ham_jam --save PATH_TO_PRIVATE_HAUNTED_MUSIC_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_HAMBOND_PROFILE

The independent `family_reunions.json --route wildwoods_training` continuation
starts from the saved Hambond reunion. Its 76,746 updates and 17 checkpoints cover
the already completed palace reunion, training-partner recruitment, both failed
and successful medicine-fruit slingshot outcomes, Might-T, the twentieth
reconciliation, the full reconciliation epilogue, the twenty-first love and the
level-four Lalalala lesson. The final original Save/Continue retains 81 words,
21 loves, level four / progress 56, two seeds and Bijou at Clubhouse map 1:1,
x=144.5, z=136.5. Every checkpoint asserts all 32 collection counters and 64 packed
rock counts; initial/final checkpoints also retain clothing and all six photos.
All preceding family-reunion route inputs and expectations remain identical.

The SDK replay passes in 118.57 seconds; 80,787 host-session checks pass in
121.16 seconds. Scout, SDK and host produce identical complete 32 KiB SRAM. A
separate 2,103-update fresh reload passes in 2.901 seconds, verifies all 81 unique
word identifiers against their flags and reopens Status. The five remaining words
are Dazzlie, Bye-Q, Goodgo, Hamcheer and Greatchu; no Sunny Peak destination-indicator
word is missing. All eleven CTest suites (1.85 seconds) and nineteen scenario
tests (0.106 seconds) pass. Native source, executable and probe hashes still match
the complete 5,859,158-check reference boundary, retaining its proof without an
identical full rerun. Other minigame variants, optional operations and full
desktop/physical-controller progression remain separate requirements.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/family_reunions.json --route wildwoods_training --save PATH_TO_PRIVATE_HAMBOND_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_WILDWOODS_PROFILE

`sunny_optional.json --route full_strength` continues from that saved epilogue.
Its 72,885 updates and 22 checkpoints cover three temple Hif-hif pickups,
searching a collected spot, in-place Save/Continue with hidden pickup sprites,
Rubrub's polisher prompt and three enhanced polishing actions. The first action
changes the remaining counts from [2, 1, 1] to [0, 1, 1]; the next actions empty
only their selected rock. Each reward increments only its collection counter.
The empty-inventory response, Gems panel and second original Save/Continue
retain all 32 collection counters, 64 rock counts, clothing and six photos.
The final state has 81 words, 21 loves, two seeds, no unpolished rocks, one new
gem and two additional Crystals, at Rubrub map 1:4, x=103.5, z=80.0.

The SDK passes in 114.38 seconds; 76,902 host-session checks pass in 117.46
seconds. Both saves match the scout across all 32 KiB. An independent
2,103-update reload passes in 2.846 seconds and confirms the 81 unique word
identifiers against their flags. Eleven serial CTest suites pass in 0.79
seconds, nineteen scenario tests in 0.055 seconds, and the installed package
passes its smoke and content checks. All previous Sunny routes are unchanged.
Identical native-source, executable and probe hashes retain the complete
5,859,158-check reference proof. Other random outcomes and a complete desktop
visit remain unverified.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/sunny_optional.json --route full_strength --save PATH_TO_PRIVATE_WILDWOODS_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_STRENGTH_PROFILE

A separate actual Windows test restores the original temple save with three
rocks and checks the visible 81 words, 21 loves, two seeds and three rocks.
It opens Crystal's two-unit entry, description and map, returns through Gems and
Status, and checks WASD movement with Bijou following. For the cart interaction,
a continuous input-only replay prepares an original save beside the cart:
4,697 updates, SDK 6.83 seconds and 6,740 host checks in 9.58 seconds, with exact
32 KiB SRAM agreement and a fresh reload before opening the window. The actual
window selects Gogo, returns to the cave and uses original End Game / Yep-P.
The resulting Windows-written save independently reloads at map 2:8, x=48.5,
z=136.5 in 2,103 updates / 2.926 seconds. All 32 collection counters, 64 rock
counts, owned clothing, six photos and word identifiers survive. The temple
pickup flags still persist in that cave save and clear later on the Clubhouse
return. The game resets after saving; this observation did not capture the
brief Save complete text. Audio was muted, and physical gamepad, window resizing
and a complete uninterrupted desktop playthrough remain unverified.

`postgame.json --route workshop_funding` starts from the full-strength Rubrub
save. Its 53,599 updates and nine checkpoints collect eighty seeds through normal
Sunny Digdig visits, find the second workshop artisan through ordinary entries,
reject missing materials without charging and perform original Save/Continue.
The SDK passes in 78.06 seconds; 57,154 host checks pass in 82.63 seconds. Scout,
SDK and host saves match across all 32 KiB. An independent 2,103-update reload
passes in 2.890 seconds with 82 seeds, 81 words, 21 loves and the manual artisan.

`postgame.json --route manual_crafting` starts from that verified workshop save.
Its 69,999 updates and ten checkpoints check two original failures and a success:
the balance changes from 82 to 62, 42 and 22 seeds; the failures consume two
Diamonds and one Peridot, and the success consumes two more Diamonds. Failed
attempts leave all thirty accessory slots empty; success stores record 44 only
in slot zero. Navigation continues after the exhausted Peridot disappears from
the material list. Original Save/Continue retains the result. Each checkpoint
checks all 32 gem counters, 64 rock counts, 30 accessory slots, clothing and six
photos. The normal input trace retains the original random outcomes; no game
state or RNG is injected. Special-material refunds, resale and other recipes
remain outside this route.

The SDK passes in 117.42 seconds; 74,568 host-session checks pass in 120.89
seconds. Scout, SDK and host saves agree across all 32 KiB. An independent
2,103-update reload passes in 3.026 seconds and retains record 44 in slot zero,
all other accessory slots empty, 81 words, 21 loves and the remaining inventory.
Twenty scenario tests and eleven serial CTest suites pass. Identical native
source, executable and probe hashes retain the full 5,859,158-check comparison.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/postgame.json --route workshop_funding --save PATH_TO_PRIVATE_STRENGTH_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_FUNDED_PROFILE
    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/postgame.json --route manual_crafting --save PATH_TO_PRIVATE_FUNDED_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_CRAFT_PROFILE

`postgame.json --route accessory_toss` starts from that successful craft in a
separate profile. Its 8,682 updates and nine checkpoints cover the populated
Heart Earrings entry, default-No confirmation, B cancellation, explicit No,
confirmed removal, empty-slot rejection and original Save/Continue. All thirty
accessory slots and all unrelated gems, rocks, clothing and photos are checked;
the balance remains 22 seeds, and Toss grants no gem refund. The SDK passes in
15.22 seconds; 10,833 host checks pass in 17.55 seconds. All 32 KiB agree with
the scout; a fresh 2,103-update reload passes in 2.872 seconds. This disposal
branch does not replace the story save that retains the accessory or establish
an original selling flow.

`fun_land_optional.json --route goodgo` instead retains the crafted accessory
and travels to Fun Land's gift counter. Hamteam triggers the original Goodgo
lesson, sets flag 0x6B and increases the unique learned count from 81 to 82.
Its 12,749 updates and eight checkpoints retain 21 loves, 22 seeds, all 32 gem
counters, 64 empty rock slots, 30 accessory records, clothing and six photos.
The SDK passes in 19.26 seconds; 15,071 host checks pass in 21.84 seconds. Scout,
SDK and host saves match across all 32 KiB. A fresh 2,103-update reload passes
in 3.080 seconds at central plaza map 3:3, x=248.5, z=112.5. The 82 learned IDs
are unique and match their flags; the missing IDs are 17, 29, 74 and 82. Complete
pair-board gameplay and full desktop progression remain separate checks.

    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/postgame.json --route accessory_toss --save PATH_TO_PRIVATE_CRAFT_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_TOSS_PROFILE
    python -B tools/run_scenario.py --rom PATH_TO_YOUR_AH3P_ROM --scenario tests/scenarios/fun_land_optional.json --route goodgo --save PATH_TO_PRIVATE_CRAFT_PROFILE/game.sav --session-test build/Release/hn_session_tests.exe --desktop-profile PATH_TO_NEW_PRIVATE_GOODGO_PROFILE

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
