# Hamtaro Native

Windows x64 native-port work for Hamtaro: Ham-Ham Heartbreak (European AH3P revision 0).
You must provide your own compatible .gba file. No game assets or ROM are included.

The application imports and validates a ROM, runs adapted native game logic,
configures keyboard/gamepad input and manages compatible saves.

Recorded native input covers the main story through ten reconciliations, Spat's
defeat, the complete credits and saved postgame control. An additional Sunny Peak
route learns Blushie, Bestest, Bloat-T and Hamtast, completes Panda's bridge and
collects the Rainbow Key. With Tran-Q from Clubhouse, the dictionary reaches
64 words. The extension also collects Condor's Wings and Great Springs, identifies
Banana Stone, Crystal and Ammonite, and opens their Gems entries. A separate
continuation collects and polishes two Glass Beads, commissions Pooie Earrings,
trades them for the Star Key and completes its rescue. Swellie increases the
dictionary to 65 words and the rescued-love count to eleven.
The Sunny Boutique continuation buys both Antlers colors, checks insufficient
funds and sold-out selections, and retains both purchases after saving.
The scenarios check original Save/Continue and compare complete
32 KiB saves between independent native runtimes.

This development build still has unfinished functions. Remaining words, optional
activities, minigames and a full desktop playthrough remain unverified.
See [docs/architecture.md](docs/architecture.md) for the tested routes and their limits.

## Build

Use Visual Studio 2026 C++ Build Tools, CMake 4.2+ and Windows 10/11 x64
for the supplied preset. The project also builds with Visual Studio 2022
and CMake 3.25+ using a matching generator (as in CI).

    cmake --preset windows
    cmake --build --preset release
    ctest --preset release
    cmake --install build --config Release --prefix build/package
    cpack --config build/CPackConfig.cmake -C Release -B build

Dependencies are pinned with SHA-256 hashes and stored outside this checkout,
under HAMTARO_NATIVE_WORK (default: the user profile's .hamtaro-native-dev folder).
An Internet connection is needed for the first dependency download, not for normal use.

Run build/Release/hamtaro-native.exe. Import a compatible ROM through the application.
Choose Play to start; Escape or right-stick click opens the pause/settings menu.
ROM, settings and saves live in the profile's .hamtaro-native directory, independently
of the program folder. Use --data-dir PATH for a separate test profile.

The SHA-256 of the supported ROM is
401968322b3245381da64dab35bdaacb3f82a404425c584867f00c45e469ffd8.

## Controls

Directions: arrows or WASD / D-pad or left stick. GBA A: Z / south button.
GBA B: X / east button. L/R: Q/E / shoulders. Start: Enter / Start.
Select: right Shift / Back. Port menu: Escape / right-stick click. Fullscreen: F11.
Bindings, deadzone, video and sound settings are configurable.

Original 32 KiB .sav files can be imported and exported. Emulator save states
are not compatible. Importing a ROM never replaces an existing save.
