#include "app/session.h"
#include <algorithm>
#include <stdexcept>
namespace hn {
void Session::capture_save()
{
    if(!game_->memory.sram_dirty)return;
    Save candidate;std::copy_n(game_->memory.sram,candidate.size(),candidate.begin());
    if(candidate!=committed_)pending_=candidate;
    game_->memory.sram_dirty=false;
}
void Session::persist(const Paths &paths)
{
    if(!pending_)return;
    write_save(paths,*pending_);committed_=*pending_;pending_.reset();
}
bool Session::start(const Paths &paths,std::span<const std::uint8_t> rom)
{
    if(present())throw std::runtime_error("Stop the current game before starting another.");
    if(validate_rom(rom)!=RomStatus::valid)throw std::runtime_error("A verified AH3P revision 0 ROM is required.");
    Save saved=load_save(paths);
    rom_.assign(rom.begin(),rom.end());game_=std::make_unique<HNGame>();committed_=saved;
    if(!hn_game_initialize(game_.get(),rom_.data(),rom_.size(),saved.data()))return false;
    capture_save();persist(paths);return true;
}
bool Session::frame(const Paths &paths,std::uint16_t buttons)
{
    persist(paths);
    if(!playable()||!hn_game_frame(game_.get(),buttons))return false;
    /* Failed updates may have partially modified SRAM. Never persist those bytes. */
    capture_save();persist(paths);return true;
}
void Session::stop(const Paths &paths)
{
    persist(paths);game_.reset();rom_.clear();
}
std::span<const std::uint32_t> Session::pixels() const
{ return game_?std::span<const std::uint32_t>(game_->pixels):std::span<const std::uint32_t>{}; }
std::span<const std::int16_t> Session::samples() const
{ return game_?std::span<const std::int16_t>(game_->samples,game_->sample_frames*2):std::span<const std::int16_t>{}; }
}
