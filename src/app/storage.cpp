#include "app/storage.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <algorithm>
namespace hn {
static void winerror(const char *operation)
{ throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), operation); }
Paths default_paths()
{
    PWSTR profile=nullptr;
    if(FAILED(SHGetKnownFolderPath(FOLDERID_Profile,0,nullptr,&profile)))
        throw std::runtime_error("Cannot locate the Windows user profile.");
    std::filesystem::path root(profile); CoTaskMemFree(profile);
    return {root / ".hamtaro-native"};
}
std::string utf8(const std::filesystem::path &p)
{
    auto s=p.u8string(); return {reinterpret_cast<const char *>(s.data()),s.size()};
}
std::filesystem::path from_utf8(std::string_view s)
{
    auto *first=reinterpret_cast<const char8_t *>(s.data());
    return std::filesystem::path(std::u8string_view(first,s.size()));
}
std::vector<std::uint8_t> read_file(const std::filesystem::path &p,std::size_t maximum)
{
    std::ifstream file(p,std::ios::binary|std::ios::ate);
    if(!file) throw std::runtime_error("Cannot open the selected file.");
    auto length=file.tellg();
    if(length<0 || static_cast<std::uintmax_t>(length)>maximum)
        throw std::runtime_error("File size is not supported.");
    std::vector<std::uint8_t> data(static_cast<std::size_t>(length));
    file.seekg(0);
    if(!data.empty() && !file.read(reinterpret_cast<char *>(data.data()),static_cast<std::streamsize>(data.size())))
        throw std::runtime_error("Cannot read the entire file.");
    return data;
}
void atomic_write(const std::filesystem::path &p,std::span<const std::uint8_t> bytes,bool backup)
{
    if(!p.parent_path().empty()) std::filesystem::create_directories(p.parent_path());
    auto temporary=p; temporary += L".tmp-"+std::to_wstring(GetCurrentProcessId());
    HANDLE file=CreateFileW(temporary.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH,nullptr);
    if(file==INVALID_HANDLE_VALUE) winerror("Create temporary file");
    try {
        std::size_t offset=0;
        while(offset<bytes.size()) {
            DWORD written=0,chunk=static_cast<DWORD>(std::min<std::size_t>(bytes.size()-offset,1u<<20));
            if(!WriteFile(file,bytes.data()+offset,chunk,&written,nullptr)||!written) winerror("Write file");
            offset+=written;
        }
        if(!FlushFileBuffers(file)) winerror("Flush file");
        CloseHandle(file); file=INVALID_HANDLE_VALUE;
        if(backup && std::filesystem::exists(p)) {
            auto previous=p; previous += L".bak";
            if(!CopyFileW(p.c_str(),previous.c_str(),FALSE)) winerror("Back up previous save");
        }
        if(!MoveFileExW(temporary.c_str(),p.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            winerror("Replace file");
    } catch(...) {
        if(file!=INVALID_HANDLE_VALUE) CloseHandle(file);
        DeleteFileW(temporary.c_str());
        throw;
    }
}
void atomic_text(const std::filesystem::path &p,const std::string &text)
{ atomic_write(p,{reinterpret_cast<const std::uint8_t *>(text.data()),text.size()}); }
Save load_save(const Paths &paths)
{
    Save result; result.fill(0xFF);
    if(!std::filesystem::exists(paths.save())) return result;
    auto bytes=read_file(paths.save(),save_size);
    if(bytes.size()!=save_size) throw std::runtime_error("Invalid save size. A compatible SRAM save has 32768 bytes.");
    std::copy(bytes.begin(),bytes.end(),result.begin());
    return result;
}
void write_save(const Paths &paths,const Save &data)
{
    if(std::filesystem::exists(paths.save()) && std::filesystem::file_size(paths.save())!=save_size)
        throw std::runtime_error("Existing save is damaged. Restore or replace it explicitly before saving.");
    atomic_write(paths.save(),data,true);
}
void import_save(const Paths &paths,const std::filesystem::path &source)
{
    auto bytes=read_file(source,save_size);
    if(bytes.size()!=save_size) throw std::runtime_error("Expected a 32768-byte .sav file, not an emulator state.");
    if(std::filesystem::exists(paths.save()) && std::filesystem::equivalent(source,paths.save())) return;
    if(std::filesystem::exists(paths.backup()) && std::filesystem::equivalent(source,paths.backup())) {
        restore_save(paths); return;
    }
    bool valid_previous=std::filesystem::exists(paths.save()) &&
                        std::filesystem::file_size(paths.save())==save_size;
    atomic_write(paths.save(),bytes,valid_previous);
}
void export_save(const Paths &paths,const std::filesystem::path &destination)
{
    if(!std::filesystem::exists(paths.save())) throw std::runtime_error("There is no saved game to export yet.");
    if(std::filesystem::exists(destination) && std::filesystem::equivalent(paths.save(),destination)) return;
    auto data=load_save(paths); atomic_write(destination,data);
}
void restore_save(const Paths &paths)
{
    auto previous=read_file(paths.backup(),save_size);
    if(previous.size()!=save_size) throw std::runtime_error("The save backup is not valid.");
    atomic_write(paths.save(),previous);
}
ProfileLock::ProfileLock(const Paths &paths)
{
    std::filesystem::create_directories(paths.root);
    handle_=CreateFileW((paths.root/"running.lock").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,
                       OPEN_ALWAYS,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
    if(handle_==INVALID_HANDLE_VALUE) { handle_=nullptr; throw std::runtime_error("This profile is already open, or is not writable."); }
}
ProfileLock::~ProfileLock() { if(handle_) CloseHandle(handle_); }
}
