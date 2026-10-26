#pragma once
#include "app/storage.h"
#include <memory>
namespace hn {
inline constexpr std::size_t rom_size = 16777216;
using Rom = std::shared_ptr<const std::vector<std::uint8_t>>;
std::string sha256(std::span<const std::uint8_t>);
enum class RomStatus { valid, wrong_size, wrong_identity, wrong_hash };
RomStatus validate_rom(std::span<const std::uint8_t>);
void import_rom(const Paths &,const std::filesystem::path &);
Rom load_rom(const Paths &);
}
