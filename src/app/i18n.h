#pragma once
#include <array>
#include <cstdint>

namespace hn {
inline constexpr int automatic_language=-1;
// Keep the original Spanish/English preference values compatible with saved settings.
inline constexpr std::array<const char *,5> language_names={"Español","English","Français","Deutsch","Italiano"};
int language_from_windows_id(std::uint16_t id);
int system_language();
const char *translate(int language,const char *english);
}
