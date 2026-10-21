cmake_minimum_required(VERSION 3.25)
project(HamtaroVideoOracle C CXX)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
set(MGBA_SOURCE "" CACHE PATH "Existing mGBA 0.10.5 source checkout")
if(NOT EXISTS "${MGBA_SOURCE}/include/mgba/internal/gba/renderers/video-software.h")
    message(FATAL_ERROR "Set MGBA_SOURCE to a caller-provided mGBA 0.10.5 checkout.")
endif()
# This optional test project is separate from the application's build graph.
# Use RGB555 arithmetic for the original GBA's blend/fade precision.
add_compile_definitions(COLOR_16_BIT)
set(M_CORE_GBA ON CACHE BOOL "" FORCE)
set(BUILD_STATIC ON CACHE BOOL "" FORCE)
set(USE_EPOXY ON CACHE BOOL "" FORCE)
foreach(option M_CORE_GB BUILD_SHARED BUILD_QT BUILD_SDL BUILD_GL BUILD_GLES2 BUILD_GLES3
        BUILD_LTO BUILD_TEST BUILD_SUITE BUILD_ROM_TEST BUILD_PERF BUILD_PYTHON
        USE_DEBUGGERS USE_FFMPEG USE_LIBZIP USE_MINIZIP USE_SQLITE3 USE_ELF USE_LUA
        USE_LZMA USE_DISCORD_RPC USE_PNG USE_ZLIB ENABLE_SCRIPTING)
    set(${option} OFF CACHE BOOL "" FORCE)
endforeach()
add_subdirectory("${MGBA_SOURCE}" mgba)
add_executable(hn-video-oracle ../video_oracle.c)
target_include_directories(hn-video-oracle PRIVATE "${MGBA_SOURCE}/include" "${CMAKE_CURRENT_BINARY_DIR}/mgba/include")
target_link_libraries(hn-video-oracle PRIVATE mgba)
