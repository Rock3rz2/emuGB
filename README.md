# emuGB

A **Game Boy (DMG)** emulator written in **C** with **SDL3**, built as a learning project.

## Features

- **CPU**: LR35902 instruction set (including the CB prefix), interrupts, timers (DIV/TIMA), HALT
- **PPU**: scanline-based rendering with background, window and 8x8 sprites (flipping, OBP0/OBP1 palettes, priority), LCDC/STAT/LYC registers
- **APU**: 4 audio channels (two square waves with sweep and envelope, Wave RAM, LFSR noise) driven by a frame sequencer
- **Cartridges**: ROM only, MBC1, MBC3 and MBC5, with cartridge RAM saved to disk
- **Display**: classic green or grayscale palette
- Frame limiter at ~60 FPS

## Known limitations

This is a work in progress. Things that are not implemented or are simplified:

- 8x16 sprite mode is not supported
- STAT interrupts for PPU modes (only the LYC interrupt is raised)
- Some illegal opcodes are not handled
- The APU is a simplified model, so some games may sound slightly off

## Requirements

- A **C compiler** (MSVC, MinGW, GCC or Clang)
- **[CMake](https://cmake.org/download/) 3.24 or newer**
- **[Git](https://git-scm.com/)** (CMake uses it to download SDL3)
- An **internet connection** the first time you build

You do **not** need to install SDL3: CMake downloads and builds it automatically (SDL 3.4.16, statically linked, so there is no `SDL3.dll` to carry around).

## Building

### Command line (any platform)

```bash
git clone https://github.com/Rock3rz2/emuGB.git
cd emuGB
cmake -S . -B build
cmake --build build
```

The first configuration downloads and compiles SDL3, so it can take a few minutes. Later builds are much faster.

The executable is named `emuGB2`:

- Single-config generators (Makefiles, Ninja, MinGW): `build/emuGB2` (or `build/emuGB2.exe` on Windows)
- Visual Studio generators: `build/Debug/emuGB2.exe` (or `build/Release/emuGB2.exe` if you build with `--config Release`)

The `assets/` folder is copied next to the executable automatically at build time.

### CLion

1. Open the project folder (**File -> Open**).
2. Let CLion load the CMake project (the first load downloads SDL3).
3. Select the `emuGB2` target and press **Run**.

### Visual Studio Code

Install the *CMake Tools* extension, open the folder, pick a compiler kit when asked, then use **Build** and **Run**.

## Running

1. Start the emulator. A window with a **Load ROM** button appears.
2. Click **Load ROM** and pick a `.gb` or `.gbc` file from your disk in the file dialog.
3. The game starts right away.

**ROMs are not included in this repository.** You must use ROMs you legally own.

## Controls

| Game Boy | Keyboard |
|----------|----------|
| D-pad Up / Down / Left / Right | W / S / A / D |
| A | F |
| B | G |
| Start | R |
| Select | T |

## Configuration

A few options are compile-time flags at the top of `main.c`:

| Flag | Effect |
|------|--------|
| `use_classic_color` | `0` = grayscale, `1` = classic Game Boy green |
| `renderGB` | `1` = draw the Game Boy shell around the screen (uses `assets/gb.bmp`), `0` = screen only |

## Saves

For cartridges with a battery, cartridge RAM is saved to a `.sav` file and loaded again the next time you open the same ROM.

## Project structure

| Path | Contents |
|------|----------|
| `main.c` | Main loop, SDL3 window, input, audio output, ROM loading |
| `sources/src/gameBoy.c` | Memory bus, MBC handling, interrupts, instruction execution |
| `sources/src/cpu.c`, `helper.c` | CPU state, ALU, CB-prefixed instructions |
| `sources/src/ppu.c` | Scanline rendering and PPU timing |
| `sources/src/apu.c` | Audio synthesis |
| `sources/src/utils.c` | Input handling and small graphics helpers |
| `sources/headers/` | Header files |
| `assets/` | Images used by the optional Game Boy shell |

## Troubleshooting

- **CMake cannot download SDL3**: check your internet connection and that `git` is installed and on your `PATH`.
- **Configuration fails after switching compiler**: delete the `build` folder and configure again.
- **The window opens but stays empty**: you have to click **Load ROM** and select a game.

## Useful resources

- [Pan Docs](https://gbdev.io/pandocs/)
- [Game Boy opcode table](https://gbdev.io/gb-opcodes/optables/)
- [gb-test-roms](https://github.com/retrio/gb-test-roms), used for CPU testing

## License

Released under the MIT License. See the [LICENSE](LICENSE) file.

This project uses [SDL3](https://github.com/libsdl-org/SDL), distributed under the zlib license.