# Hamtaro Native

Windows x64 native-port candidate for Hamtaro: Ham-Ham Heartbreak (European AH3P revision 0).
You must provide your own compatible .gba file. The launcher includes its artwork;
the ROM and gameplay resources are supplied by the player.

The application imports and validates a ROM, runs adapted native game logic,
configures keyboard/gamepad input and manages compatible saves.

Recorded native input covers Spat's defeat, complete credits, all 21
reconciliations and their epilogue, and saved postgame control. Optional routes
reach all 86 Ham-Chats, including Lalalala, Goodgo and the expert Ham-Jam reward. They also
cover Sunny Peak's bridge, keys, songs, sand bath and temple rock pickups;
epilogue-strength polishing; accessory commissioning, manual crafting failures
and success, trading and accessory disposal; outfit purchases and saved photos; Haunted House's piano; and
local Ham-Jam composition followed by beginner, intermediate and expert contest
wins, the perfect-ending credits and polishing both retained contest prizes.
Postie's four-answer Ham-Chat contest also delivers all four keys through the
original letter, departure and return sequence.
Sunny Peak's locked Rainbow cave, protection against a duplicate special rock
and polishing that rock into a Rainbow Stone are also covered. Wildwoods' Sun Key
cave adds its fixed rock, Sun Stone polishing, and a successful special-stone
recipe that spends one Sun Stone and 45 seeds. Sandy Bay's Wave Key cave also
covers polishing, duplicate prevention while the polished gem is owned, and
crafting with a special stone plus ordinary gems.
Shared-quarry routes also collect all six outer-room and eight inner-room rocks,
including hidden drops, ledge interactions and the fixed Ham-lift rock, then
retain their types and remaining polishes through original save and reload.
A separate route uses Ham-lift directly at all four pillars before Tack-Q,
checks one new rock per action and preserves the results through save and reload.
Fun Land's pair board also has complete winning and losing routes, including
entry fees, the nine-rock reward, replay rejection and saved continuation.
The scenarios check original Save/Continue and compare complete
32 KiB saves between independent native runtimes.

This candidate does not yet have verified coverage of every single-player
branch. Further optional activities, minigame variants, physical controllers and a
full desktop playthrough remain unverified. Title language changes, localized
Status, original saving and Continue are exercised in all five ROM languages.
Startup on a Windows installation without development tools remains unverified.
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

Launcher artwork lives in assets/launcher/ and the Windows icon in assets/windows/.
The supplied logo, cover and animated GIF are embedded at build time. No separate
artwork files are needed beside the executable. The ROM and saves remain user-supplied.

Run build/Release/hamtaro-native.exe. Import a compatible ROM through the application.
Choose Play to start; Escape or right-stick click opens the pause/settings menu.
Press Escape again to resume. Automatic pausing when switching applications is off
by default and can be enabled in Video and audio.
The launcher supports English, Spanish, French, German and Italian. It follows the
Windows display language by default, falling back to English for other languages.
Choose Automatic or a specific language in Video and audio; manual choices persist.
The game's own language setting is independent of the launcher.
The window opens at a whole multiple of the original 240 x 160 image (960 x 640
by default), fitting the monitor's work area. Previously saved window sizes are
adjusted on launch. Integer scaling is off by default: resizing and fullscreen fit
the largest image with its original proportions, leaving margins on just one axis
when needed. Integer scaling can be enabled in Video and audio.
ROM, settings and saves live in the profile's .hamtaro-native directory, independently
of the program folder. Use --data-dir PATH for a separate test profile.

The SHA-256 of the supported ROM is
401968322b3245381da64dab35bdaacb3f82a404425c584867f00c45e469ffd8.

## Controls

Directions: arrows or WASD / D-pad or left stick. GBA A: Z / south button.
GBA B: X / east button. L/R: Q/E / shoulders. Start: Enter / Start.
Select: right Shift / Back. Port menu: Escape / right-stick click. Fullscreen: F11.
Bindings, deadzone, video and sound settings are configurable.

Import an original 32 KiB .sav from Home, or open the save folder to copy it.
Emulator save states are not compatible. Importing a ROM never replaces an existing save.
