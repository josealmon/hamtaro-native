#include "app/rom.h"
#include "rom_spec.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <stdexcept>
#include <limits>
#include <cstring>
namespace hn {
std::string sha256(std::span<const std::uint8_t> bytes)
{
    if(bytes.size()>std::numeric_limits<ULONG>::max()) throw std::runtime_error("Hash input is too large.");
    BCRYPT_ALG_HANDLE algorithm=nullptr;
    BCRYPT_HASH_HANDLE hash=nullptr;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0)
        throw std::runtime_error("Cannot initialize SHA-256.");
    std::array<std::uint8_t,32> digest{};
    bool ok=BCryptCreateHash(algorithm,&hash,nullptr,0,nullptr,0,0)>=0;
    if(ok) ok=BCryptHashData(hash,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),0)>=0;
    if(ok) ok=BCryptFinishHash(hash,digest.data(),static_cast<ULONG>(digest.size()),0)>=0;
    if(hash) BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm,0);
    if(!ok) throw std::runtime_error("SHA-256 verification failed.");
    const char hex[]="0123456789abcdef";
    std::string text; text.reserve(64);
    for(auto b:digest) { text+=hex[b>>4]; text+=hex[b&15]; }
    return text;
}
RomStatus validate_rom(std::span<const std::uint8_t> bytes)
{
    if(bytes.size()!=rom_size) return RomStatus::wrong_size;
    if(std::memcmp(bytes.data()+0xAC,"AH3P",4)!=0 || bytes[0xBC]!=0) return RomStatus::wrong_identity;
    if(sha256(bytes)!=HN_ROM_SHA256) return RomStatus::wrong_hash;
    return RomStatus::valid;
}
static void require_valid(RomStatus status)
{
    switch(status) {
    case RomStatus::valid:return;
    case RomStatus::wrong_size:throw std::runtime_error("The ROM must be exactly 16 MiB.");
    case RomStatus::wrong_identity:throw std::runtime_error("This build needs the European AH3P revision 0 ROM.");
    default:throw std::runtime_error("The ROM hash does not match. Modified or damaged ROMs are not supported.");
    }
}
void import_rom(const Paths &paths,const std::filesystem::path &source)
{
    auto bytes=read_file(source,rom_size);
    require_valid(validate_rom(bytes));
    if(std::filesystem::exists(paths.rom())&&std::filesystem::equivalent(source,paths.rom())) return;
    atomic_write(paths.rom(),bytes);
}
std::vector<std::uint8_t> load_rom(const Paths &paths)
{
    auto bytes=read_file(paths.rom(),rom_size);
    require_valid(validate_rom(bytes));
    return bytes;
}
}
