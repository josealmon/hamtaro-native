# Testing and packaging

Build instructions are in the [README](../README.md#building-from-source).
Commands below run from the repository root in PowerShell. Replace `PATH_TO_...`
placeholders with your local paths; keep ROMs, saves and generated reports outside
the checkout. Run gameplay tests sequentially, using disposable profiles.

## Synthetic tests and CI

```powershell
ctest --preset release --parallel 1
python -B -m unittest discover -s tests/unit -p "*.py"
python -B tools/check_repository.py
```

These checks require no ROM. CTest checks remain active in Release builds.
[Windows CI](../.github/workflows/build.yml) runs the synthetic suites and audits
the installed package.

| CTest suite | Coverage |
| --- | --- |
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

The Python tests cover scenario parsing and source inventories, including malformed
layouts, corrupt ROM/source matches and incorrect callback contracts. Public
fixtures contain synthetic data, button schedules and scalar observations.

## Private ROM and host checks

The application can verify a ROM without importing it:

```powershell
& ".\build\Release\Hamtaro - Ham-Ham Heartbreak.exe" --verify-rom "PATH_TO_ROM"
```

Exit code 0 means the identity and full hash match; 2 means they do not.
Additional storage/session checks use temporary profiles and preserve the input
ROM:

```powershell
& .\build\Release\hn_storage_tests.exe "PATH_TO_ROM"
& .\build\Release\hn_session_tests.exe "PATH_TO_ROM"
& .\build\Release\hn_session_tests.exe "PATH_TO_ROM" --audio
```

The audio option requires an output device and plays 600 paced game frames. It
checks SDL sample availability as well as session behavior. Listening and waveform
comparison are separate checks when investigating an audible defect.

## Gameplay scenarios

`tools/run_scenario.py` feeds ordinary buttons into the native runtime and checks
read-only state observations. It does not inject RAM, force RNG or modify an input
save. The test-only native probe defaults to `build/Release/hn_reference_probe.dll`.

Start the main route from erased SRAM:

```powershell
python -B tools/run_scenario.py --rom "PATH_TO_ROM" --scenario tests/scenarios/sunny_peak.json --session-test build/Release/hn_session_tests.exe --desktop-profile "PATH_TO_NEW_SUNNY_PROFILE"
```

Then continue from its saved result in a new process:

```powershell
python -B tools/run_scenario.py --rom "PATH_TO_ROM" --scenario tests/scenarios/sandy_bay.json --save "PATH_TO_NEW_SUNNY_PROFILE/game.sav" --session-test build/Release/hn_session_tests.exe --desktop-profile "PATH_TO_NEW_SANDY_PROFILE"
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

Each continuation needs the preceding route's original saved state. Inputs are
not designed for verbatim concatenation inside one live session. The first
checkpoint in each scenario describes its prerequisite.

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
python -B tools/compare_reference.py --rom "PATH_TO_ROM" --reference "PATH_TO_DECOMP" --audio-only
python -B tools/compare_reference.py --rom "PATH_TO_ROM" --reference "PATH_TO_DECOMP" --bank2-only --bios-oracle "PATH_TO_BIOS_ORACLE"
```

The tool's `--help` lists all family selectors, including scenes, actors,
collisions, records, area banks, scanlines and local music. A full run with the
independent video, BIOS and bus checks is:

```powershell
python -B tools/compare_reference.py --rom "PATH_TO_ROM" --reference "PATH_TO_DECOMP" --bios-oracle "PATH_TO_BIOS_ORACLE" --video-oracle "PATH_TO_VIDEO_ORACLE" --bus-oracle "PATH_TO_BUS_ORACLE" --timings-json "PATH_OUTSIDE_CHECKOUT/timings.json"
```

Use `--oracle-runtime-dir` if oracle executables require compiler runtime DLLs.
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
cmake -S tests/integration/mgba -B "$env:HAMTARO_NATIVE_WORK/mgba-oracle" -G Ninja -DCMAKE_BUILD_TYPE=Release -DMGBA_SOURCE="$env:MGBA_SOURCE"
cmake --build "$env:HAMTARO_NATIVE_WORK/mgba-oracle" --target hn-video-oracle hn-audio-oracle hn-bios-oracle hn-bus-oracle
python -B tools/compare_video.py --oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-video-oracle.exe"
python -B tools/compare_audio.py --oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-audio-oracle.exe"
python -B tools/compare_bios.py --oracle "$env:HAMTARO_NATIVE_WORK/mgba-oracle/hn-bios-oracle.exe"
```

Video comparisons use RGB555 precision. Analytical tests in `render` also cover
narrow text windows, where mGBA 0.10.5 has partial-tile discrepancies. Audio
comparisons cover digital timers, FIFOs, PSG and DAC behavior, before the desktop
DC filter and device resampling. These component checks do not establish analog
hardware output or complete scene timing.

### Source inventories

```powershell
python -B tools/inventory_reference.py --reference "PATH_TO_DECOMP"
python -B tools/inventory_reference.py --reference "PATH_TO_DECOMP" --background-scripts --rom "PATH_TO_ROM"
python -B tools/inventory_reference.py --reference "PATH_TO_DECOMP" --task-callbacks --rom "PATH_TO_ROM"
```

The default inventory groups source families and explicit dispatch bindings.
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

Use existing story results as regression context, with their source revision and
binary hashes. A narrow change does not require replaying unrelated routes.
Keep logs, timing reports, captures and saves outside Git. Hardware/controller
testing and clean-Windows startup remain distinct from hidden smoke tests.

## Packaging

After a successful Release build:

```powershell
cmake --install build --config Release --prefix build/package
cpack --config build/CPackConfig.cmake -C Release -B build
python -B tools/check_repository.py --package build/package
git diff --check
```

The archive is named `Hamtaro - Ham-Ham Heartbreak <version>.zip`, using the
version in `CMakeLists.txt`. Its contents are:

```text
Hamtaro - Ham-Ham Heartbreak.exe
NOTICE.md
licenses/
  SDL.txt
  Dear-ImGui.txt
  mGBA-MPL-2.0.txt
```

README and developer documentation stay in the source repository. The package
excludes ROMs, saves, settings, tests and reference tools. Check the ZIP entries
and extracted bytes as well as the install directory; installing into an old
directory does not remove obsolete files.

For a hidden packaged startup check, run the installed executable with
`--smoke-test`. Without `--data-dir`, it creates and removes its own temporary
profile. An explicit `--data-dir` is preserved, so use a disposable directory.
The repository checker audits public files and historical Git objects for
private/generated content; it does not classify copyright or license rights.
