cmake_minimum_required(VERSION 3.24)
project(emuGB2 C)

include(FetchContent)
FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-3.4.16
        FIND_PACKAGE_ARGS 3 CONFIG
)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

set(CMAKE_C_STANDARD 99)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

add_executable(emuGB2
        main.c
        sources/src/cpu.c
        sources/src/gameBoy.c
        sources/src/helper.c
        sources/src/ppu.c
        sources/src/apu.c
        sources/src/utils.c
)

target_include_directories(emuGB2 PRIVATE ${CMAKE_SOURCE_DIR}/sources/headers)
target_link_libraries(emuGB2 PRIVATE SDL3::SDL3)

# copia gli assets accanto all'eseguibile
add_custom_command(TARGET emuGB2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:emuGB2>/assets)