# Hamtaro Native

Windows x64 native-port work for Hamtaro: Ham-Ham Heartbreak (European AH3P revision 0).
You must provide your own compatible .gba file. No game assets or ROM are included.

The application imports and validates a ROM, runs adapted native game logic,
configures keyboard/gamepad input and manages compatible saves. Native input
replays cover the first Sunny Peak story, three reconciliations and a saved return
to Clubhouse, followed by Sandy Bay's normal unlock, swimming lesson, Tinglie,
ball delivery, perfect six/twelve-rally games, Spat's trap, Seamore and Barrette's
reconciliation, the bottle message, wooden board and Items panel. The continuation
plays the ten-note melody, opens the Captain's passage, takes the ferry to Treasure
Island and recovers the scarf. The music demonstration, dressing-room impostor,
elder's Tuggie lesson and lighthouse confrontation now reach the fifth
reconciliation and original saved continuation.
The next saved stage returns by ferry, follows Clubhouse's Fun Land unlock,
learns Scrub, collects the trash and earns the green marble with 630 points in
the musical-note challenge. It then rescues the operator, repairs Ham-Bot,
reaches the sixth reconciliation and returns to Clubhouse, retaining the new
destination unlock through original Save and Continue.
Haunted House now covers the ID and bronze cards, Go-P and the mirror passage,
the ghost challenge and silver card, upper-floor lessons and Bijou's prison
rescue. The seventh reconciliation and Flipflop survive original Save and Continue,
including a separate saved continuation while Bijou is alone.
The upper-floor continuation covers a failed tile challenge and successful retry,
the gold card, Perksie, the quiz and Spat's escape. Clubhouse then unlocks the
jungle, and another original Save and Continue retains the completed stage.
This development build still stops at unfinished functions; complete story, optional activities,
remaining minigames and desktop playthrough coverage remain unverified.
See docs/architecture.md for the current evidence.

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
