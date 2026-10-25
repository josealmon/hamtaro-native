# Hamtaro Native

Windows x64 native-port work for Hamtaro: Ham-Ham Heartbreak (European AH3P revision 0).
You must provide your own compatible .gba file. No game assets or ROM are included.

The application imports and validates a ROM, runs adapted native game logic,
configures keyboard/gamepad input and manages compatible saves.

Recorded native input covers Spat's defeat, complete credits, all 21
reconciliations and their epilogue, and saved postgame control. Optional routes
reach 81 of 86 Ham-Chats, including the level-four Lalalala lesson. They also
cover Sunny Peak's bridge, keys, songs and sand bath; accessory commissioning
and trading; outfit purchases and saved photos; Haunted House's piano; and
local Ham-Jam composition followed by a beginner contest win.
The scenarios check original Save/Continue and compare complete
32 KiB saves between independent native runtimes.

This development build still has unfinished functions. Remaining words, optional
activities, minigame branches, physical controllers and a full desktop playthrough
remain unverified.
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
