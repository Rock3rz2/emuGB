# emuGB

Emulatore **Game Boy (DMG)** scritto in **C** con **SDL3**, sviluppato come progetto di apprendimento.

## Funzionalità

- **CPU**: set di istruzioni LR35902 (incluso il prefisso CB), interrupt, timer (DIV/TIMA), HALT
- **PPU**: rendering per scanline con background, window, sprite 8x8 (flip, palette OBP0/OBP1, priorità), registri LCDC/STAT/LYC, DMA OAM
- **APU**: 4 canali audio (due onde quadre con sweep ed envelope, Wave RAM, rumore LFSR) e frame sequencer
- **Cartucce**: ROM only, MBC1, MBC3, MBC5, con salvataggio della RAM della cartuccia su file
- **Display**: palette verde classica o scala di grigi
- Limitatore di frame a ~60 FPS

## Stato del progetto

Funzionante, ma ancora in sviluppo. Note sui problemi noti:

- Modalità sprite 8x16 non ancora implementata
- Interrupt STAT e joypad da completare
- Alcuni glitch grafici in giochi specifici (es. HUD di Super Mario Land, testo di Pokémon Rosso)
- Alcuni opcode illegali non gestiti

Le ROM di test `cpu_instrs` di Blargg sono usate per verificare la CPU.

## Requisiti

- Compilatore C (GCC, Clang o MSVC)
- [CMake](https://cmake.org/) 3.x o superiore
- [SDL3](https://github.com/libsdl-org/SDL) (vedi sezione sotto)

## Compilazione

```bash
git clone <URL-DEL-REPO>
cd <NOME-REPO>
cmake -S . -B build
cmake --build build
```

> TODO: adegua questa sezione a come il tuo `CMakeLists.txt` trova SDL3
> (FetchContent, `find_package`, submodule...).

## Utilizzo

```bash
./build/emuGB percorso/alla/rom.gb
```

> TODO: verifica come `main.c` riceve il percorso della ROM (argomento da riga di comando o altro).

**Le ROM non sono incluse nel repository.** Devi usare ROM che possiedi legalmente.

## Controlli

| Game Boy | Tastiera |
|----------|----------|
| Croce direzionale | W / A / S / D |
| Pulsanti | F / G / R / T |

> TODO: indica quale tasto corrisponde ad A, B, Start e Select.

## Salvataggi

Per le cartucce con batteria, la RAM viene salvata su un file `.sav` accanto alla ROM.

## Struttura del codice

| File | Contenuto |
|------|-----------|
| `main.c` | Ciclo principale, finestra SDL3, input, audio |
| `sources/gameBoy.c` | Memoria, bus, MBC, interrupt, esecuzione delle istruzioni |
| `sources/cpu.c`, `helper.c` | Stato della CPU, ALU, istruzioni CB |
| `sources/ppu.c` | Rendering delle scanline e timing della PPU |
| `sources/apu.c` | Sintesi audio |
| `sources/utils.c` | Funzioni di utilità grafiche |

## Risorse utili

- [Pan Docs](https://gbdev.io/pandocs/)
- [Game Boy CPU manual / opcode table](https://gbdev.io/gb-opcodes/optables/)
- [gb-test-roms](https://github.com/retrio/gb-test-roms)

## Licenza

> TODO: scegli una licenza (es. MIT) e aggiungi il file `LICENSE`.

SDL3 è distribuita sotto licenza zlib dai suoi autori.
