cmake_minimum_required(VERSION 3.20)
project(emuGB2 C)

set(CMAKE_C_STANDARD 99)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

add_executable(emuGB2
        main.c
        sources/src/cpu.c
        sources/src/gameBoy.c
        sources/src/helper.c
        sources/src/ppu.c
        sources/src/apu.c
        sources/headers/utils.h
        sources/src/utils.c
)

# Aggancia gli header del tuo emulatore
target_include_directories(emuGB2 PRIVATE ${CMAKE_SOURCE_DIR}/sources/headers)

# Aggancia gli header e le librerie statiche di SDL3 in modo esplicito
target_include_directories(emuGB2 PRIVATE ${CMAKE_SOURCE_DIR}/SDL3/include)
target_link_directories(emuGB2 PRIVATE ${CMAKE_SOURCE_DIR}/SDL3/lib/x64)
target_link_libraries(emuGB2 PRIVATE SDL3)