#pragma once
#include <array>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>
#include <string>
#include <string_view>
namespace hn {
inline constexpr std::size_t save_size = 32768;
using Save = std::array<std::uint8_t, save_size>;
struct Paths {
    std::filesystem::path root;
    std::filesystem::path rom() const { return root / "game.gba"; }
    std::filesystem::path save() const { return root / "game.sav"; }
    std::filesystem::path backup() const { return root / "game.sav.bak"; }
    std::filesystem::path settings() const { return root / "settings.ini"; }
};
Paths default_paths();
std::vector<std::uint8_t> read_file(const std::filesystem::path &, std::size_t maximum);
void atomic_write(const std::filesystem::path &, std::span<const std::uint8_t>, bool backup=false);
void atomic_text(const std::filesystem::path &, const std::string &);
Save load_save(const Paths &);
void write_save(const Paths &, const Save &);
void import_save(const Paths &, const std::filesystem::path &);
void export_save(const Paths &, const std::filesystem::path &);
void restore_save(const Paths &);
std::string utf8(const std::filesystem::path &);
std::filesystem::path from_utf8(std::string_view);
class ProfileLock {
    void *handle_=nullptr;
public:
    explicit ProfileLock(const Paths &);
    ~ProfileLock();
    ProfileLock(const ProfileLock &)=delete;
    ProfileLock &operator=(const ProfileLock &)=delete;
};
}
