include(FetchContent)
if(DEFINED ENV{HAMTARO_NATIVE_WORK})
    set(HN_WORK "$ENV{HAMTARO_NATIVE_WORK}")
else()
    set(HN_WORK "$ENV{USERPROFILE}/.hamtaro-native-dev")
endif()
set(FETCHCONTENT_BASE_DIR "${HN_WORK}/dependencies" CACHE PATH "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
set(SDL_STATIC_VCRT ON CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_GPU OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_VULKAN OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_D3D12 OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_D3D OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
set(SDL_TRAY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(sdl3
    URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.4.18.tar.gz
    URL_HASH SHA256=c4b08b950bd29d83caae0ab8d884298d27046a9d30700fb42841c50992665c90
    BINARY_DIR "${CMAKE_BINARY_DIR}/dependencies/sdl3")
FetchContent_MakeAvailable(sdl3)
FetchContent_Declare(imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.93.0.tar.gz
    URL_HASH SHA256=f7bf6bc76e5d79d40d751096663e6c5d18daf08129dd52e7f7b09e4251c8eeee)
FetchContent_MakeAvailable(imgui)
add_library(hn_imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp")
target_include_directories(hn_imgui PUBLIC "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")
target_link_libraries(hn_imgui PUBLIC SDL3::SDL3-static)
