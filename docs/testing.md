# Testing and packaging

Start with the [Release build in the README](../README.md#building-from-source).
Run the commands below from the repository root in PowerShell with Python 3
and Git on `PATH`. For tests that use a ROM or save, replace the example paths
with your own and keep test profiles outside the checkout.

## Tests without a ROM

```powershell
ctest --test-dir build -C Release --output-on-failure
python -m unittest discover -s tests/unit -p "*.py"
python tools/check_repository.py
```

CTest runs the native suites below. The Python tests exercise the scenario
parser and source inventory. The repository check scans files and Git history
for generated or private content. [Windows CI](../.github/workflows/build.yml)
runs all three and also checks the contents of the installed package.

| CTest suite | Coverage |
| --- | --- |
| `lan` | Real loopback TCP, fragmented messages, invalid version/data, repeated trades, cancellation, disconnect and native record publication |
| `probe` | Bulk snapshots against guest bus reads, boundaries, mirrors and partial faults |
| `core` | Memory, DMA, BIOS, video, IRQ/timing, input, PRNG, heap and script control |
| `game` | Resources, maps, saves, actors, motion, collision, text tasks, dialogs and records |
| `render` | Animation, OAM, affine transforms, backgrounds and analytical PPU cases |
| `graphics` | Graphics scripts, palette effects, windows and raster callbacks |
| `audio` | Native driver, players, sequences, tracks, channels and mixer |
| `apu` | Timers, FIFO DMA, routing, PWM and PSG behavior |
| `storage` | SHA-256, ROM rejection, atomic saves, profile locks and settings |
| `input` | Synthetic keyboard and SDL virtual-pad input, remapping, deadzone, hotplug and short taps |
| `session` | Invalid-ROM handling and desktop audio filter behavior; optional ROM checks below |
| `app-smoke` | Actual hidden application startup, renderer/menu initialization and clean shutdown |
| `app-preferences` | Separate application restarts, saved bindings, Unicode paths, preserved SRAM/backup and future settings versions |

## Tests with a player-supplied ROM

To verify a ROM without importing it, run the executable and read its exit code.

```powershell
& ".\build\Release\Hamtaro - Ham-Ham Heartbreak.exe" --verify-rom "PATH_TO_ROM" | Out-Host
$LASTEXITCODE
```

Exit code `0` means the ROM passed validation, `2` indicates a rejected ROM,
and `1` indicates an error such as a file that could not be read. The storage
and session checks below use temporary profiles.

```powershell
& .\build\Release\hn_storage_tests.exe "PATH_TO_ROM"
& .\build\Release\hn_session_tests.exe "PATH_TO_ROM"
& .\build\Release\hn_session_tests.exe "PATH_TO_ROM" --audio
```

The `--audio` option requires an output device. It plays 600 game frames at
normal speed and checks that SDL receives enough samples throughout playback.

## Gameplay scenarios

### Two-process Cable Link

Use a save that opens Ham-Jam from the title menu, with different dances available
for the first two songs and the second slot free for each. Supply a new output
directory. The runner starts two game processes, connects them over loopback TCP
and operates the original menus with ordinary button inputs.

```powershell
python tools/check_lan.py --worker build/Release/hn_link_game.exe --rom "PATH_TO_ROM" --save "PATH_TO_SAVE" --work "PATH_TO_NEW_LAN_PROOF"
```

The runner compares the received dances with the ones offered, saves them in
the game and reloads them in fresh processes. It also checks one-way transfers,
cancellation and disconnect. The output directory contains the test profiles
and `proof.json`.

### Story and activities

`tools/run_scenario.py` feeds ordinary buttons into the native runtime and checks
read-only state observations. It does not inject RAM, force RNG or modify an input
save. The test-only native probe defaults to `build/Release/hn_reference_probe.dll`.

Start the main route from erased SRAM:

```powershell
python tools/run_scenario.py --rom "PATH_TO_ROM" --scenario tests/scenarios/sunny_peak.json --session-test build/Release/hn_session_tests.exe --desktop-profile "PATH_TO_NEW_SUNNY_PROFILE"
```

Then continue from its saved result in a new process:

```powershell
python tools/run_scenario.py --rom "PATH_TO_ROM" --scenario tests/scenarios/sandy_bay.json --save "PATH_TO_NEW_SUNNY_PROFILE/game.sav" --session-test build/Release/hn_session_tests.exe --desktop-profile "PATH_TO_NEW_SANDY_PROFILE"
```

`--desktop-profile` requires `--session-test`, a new directory outside the checkout
and an existing parent directory. The runner compares the complete host-written
32 KiB SRAM with the probe result. The host replay also checks disk persistence
and fresh-session continuation. Profiles retain the validated ROM and save for
optional desktop review.

The main story is a chain of saved continuations, in this order:

| Scenario in `tests/scenarios/` | Destination / result |
| --- | --- |
| `sunny_peak.json` | New game through Lovedove and the third reconciliation; saved Clubhouse return |
| `sandy_bay.json` | Beach, swimming, rally challenge, Seamore rescue and Treasure Island music hall |
| `treasure_island.json` | Lighthouse impostor and fifth reconciliation |
| `fun_land.json` | Green marble, Ham-Bot and sixth reconciliation |
| `haunted_house.json` | Seventh reconciliation, gold card, Spat and Wildwoods unlock |
| `jungle_hat.json` | Hat, Hamteam and eighth reconciliation |
| `jungle_wedding.json` | Wedding, red marble and ninth reconciliation |
| `sunny_temple.json` | Temple altar, spoon and tenth reconciliation |
| `spat_tower.json` | Tower puzzle, battle retreat and Hamscope upgrade |
| `spat_battle.json` | Spat defeat, credits and saved postgame |

Start each continuation in a fresh process using the previous route's save.
The first checkpoint in the scenario describes its starting state.

Additional scenarios and named routes cover:

| Files | Activities |
| --- | --- |
| `opening.json`, `sunny_peak_reunion.json`, `sunny_peak_rubrub.json` | Focused opening, Lovedove and cart/polishing regressions |
| `sunny_optional.json`, `optional_words.json` | Bridge, songs, sand bath, temple pickups, polishing and optional Ham-Chats |
| `fun_land_optional.json`, `family_reunions.json` | Tea Cups, Ferris wheel, performers, missing child, ghost, Wildwoods training and reconciliation epilogue |
| `sandy_bay_optional.json`, `haunted_house.json` (`music`) | Dance, local composition, beginner/intermediate/expert Ham-Jam, piano and final Ham-Chats |
| `postgame.json` | Crafting costs and outcomes, accessory disposal, contest prizes and Postie's key contest |
| `special_stones.json`, `quarry.json` | Key caves, duplicate prevention, polishing, special-stone recipes and quarry variants |
| `wardrobe.json` | Purchases, outfits, photos, title gallery, replacement and Toss |
| `pair_board.json` | Instructions, four-match win, four-attempt loss, fees, rewards and saved continuation |
| `menus.json` | Complete book, inventory, title safeguards, record reset and five languages |

Use `--route NAME` to select a named route and supply the save required by its
initial checkpoint. These optional routes have different prerequisites; they do
not all start from the main story endpoint. `--until "CHECKPOINT_LABEL"` stops at
a unique top-level checkpoint for a focused regression. `--input-schedule` accepts
an external input text file instead of JSON; its labels only observe state.

The recorded coverage and remaining validation gaps are summarized in
[architecture.md](architecture.md#tested-scope).

## Differential reference checks

`tools/compare_reference.py` executes selected original ARM/Thumb functions in
Unicorn and compares their state with compiled native operations. It requires:

- Python with the `unicorn` package installed in an external environment.
- The supported ROM and a read-only decompilation checkout containing
  `build/function-symbols.json`.
- The native probe built with `BUILD_TESTING=ON`.
- The BIOS oracle for a full run and for groups marked as requiring it in `--help`.

Use focused groups while changing a subsystem:

```powershell
python tools/compare_reference.py --rom "PATH_TO_ROM" --reference "PATH_TO_DECOMP" --audio-only
python tools/compare_reference.py --rom "PATH_TO_ROM" --reference "PATH_TO_DECOMP" --bank2-only --bios-oracle "PATH_TO_BIOS_ORACLE"
```

The tool's `--help` lists all family selectors, including scenes, actors,
collisions, records, area banks, scanlines and local music. A full run with the
independent video, BIOS and bus checks is:

```powershell
python tools/compare_reference.py --rom "PATH_TO_ROM" --reference "PATH_TO_DECOMP" --bios-oracle "PATH_TO_BIOS_ORACLE" --video-oracle "PATH_TO_VIDEO_ORACLE" --bus-oracle "PATH_TO_BUS_ORACLE" --timings-json "PATH_OUTSIDE_CHECKOUT/timings.json"
```

Use `--oracle-runtime-dir` with `compare_reference.py` if the external reference
executables need compiler runtime DLLs.
Reports include elapsed time and assertion counts by family. Keep compared
sources unchanged during a run.

These comparisons model the BIOS services, immediate DMA and IO masks used by
the tested functions. They compare memory and register effects with explicit
exceptions for reference stack addresses and documented pointer relocations.
They do not model complete ARM instruction timing, bus contention or arbitrary
uninitialized stack contents. Synthetic guard tests cover invalid states
separately from valid-state parity.

### Independent hardware oracles

[tests/integration/mgba](../tests/integration/mgba/CMakeLists.txt) is a separate
project using an external mGBA 0.10.5 source checkout. It builds small BIOS, bus,
video and audio reference executables; none is linked into the game.

Set `HAMTARO_NATIVE_WORK` to an external work directory and `MGBA_SOURCE` to the
mGBA source path. With Ninja and a compatible C/C++ toolchain on `PATH`:

```powershell
cmake -S tests/integration/mgba -B "$env:HAMTARO_NATIVE_WORK/mgba-oracle" -G Ninja -DCMAKE_BUILD_TYPE=Release "-DMGBA_SOURCE=$env:MGBA_SOURCE"
cmake --build "$env:HAMTARO_NATIVE_WORK/mgba-oracle" --target hn-video-oracle hn-audio-oracle hn-bios-oracle hn-bus-oracle
python tools/compare_video.py --oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-video-oracle.exe"
python tools/compare_audio.py --oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-audio-oracle.exe"
python tools/compare_bios.py --oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-bios-oracle.exe"
```

Video comparisons use RGB555 precision. Analytical tests in `render` also cover
narrow text windows, where mGBA 0.10.5 has partial-tile discrepancies. Audio
comparisons cover digital timers, FIFOs, PSG and DAC behavior, before the desktop
DC filter and device resampling. These component checks do not establish analog
hardware output or complete scene timing.

### Source inventories

```powershell
python tools/inventory_reference.py --reference "PATH_TO_DECOMP"
python tools/inventory_reference.py --reference "PATH_TO_DECOMP" --background-scripts --rom "PATH_TO_ROM"
python tools/inventory_reference.py --reference "PATH_TO_DECOMP" --task-callbacks --rom "PATH_TO_ROM"
```

The default inventory reads `build/function-symbols.json` from the decompilation
checkout and groups the functions by source file and native callback binding.
The ROM-backed modes check compiled background-script layouts and typed
text-task callback stores, rejecting unknown layouts and source/ROM mismatches.
Use `--json` for machine-readable output. An inventory identifies dependencies;
it does not establish reachability or gameplay completion.

## Choosing checks for a change

| Change | Checks |
| --- | --- |
| Game or compatibility code | Affected CTest suites, focused reference family and relevant native scenario; full comparison for broad changes |
| Input, storage or audio delivery | Related synthetic/integration suites; private session or device check where applicable |
| Launcher behavior or layout | App smoke/preferences plus inspection of the affected actual window |
| Scenario only | Parser tests, probe/host replay, full SRAM comparison and fresh save continuation |
| Documentation or package contents | Links, factual commands, repository/package audit and unchanged executable/probe hashes |

## Packaging

Build Release first, then install it and generate the ZIP from that build.

```powershell
cmake --install build --config Release --prefix build/package
python tools/check_repository.py --package build/package
cpack --config build/CPackConfig.cmake -C Release -B build/distribution
```

The archive is written to `build/distribution/Hamtaro - Ham-Ham Heartbreak <version>.zip`.
The version comes from `CMakeLists.txt`. The expected files are listed below.

```text
Hamtaro - Ham-Ham Heartbreak.exe
NOTICE.md
licenses/
  SDL.txt
  Dear-ImGui.txt
  mGBA-MPL-2.0.txt
```

Before distributing the ZIP, check its contents against this list and compare
the extracted executable's SHA-256 with `build/Release/Hamtaro - Ham-Ham Heartbreak.exe`.
The repository checker validates the install directory. CPack packages the
configured build separately.

To check startup, run the extracted executable with `--smoke-test`. It opens a
hidden window and exits with code `0` on success. When run with just this flag,
the test creates and removes a temporary profile.
