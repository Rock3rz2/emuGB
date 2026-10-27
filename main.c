#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "cpu.h"
#include "gameBoy.h"
#include "utils.h"

#define FRAME_NS 16742706ULL

// FLAG: 1 = Rendender a realistic GB frame (eheh); 0 = just the screen, little bigger
static int renderGB = 0;

// FLAG: 0 = Use normal grey scale palette; 1; Use the classic green gb
static int use_classic_color = 0;

//Can't touch this (dan da da dan )
static int hasRom = 0;

static char pending_rom[512] = "";
static const SDL_DialogFileFilter rom_filters[] = { { "ROM Game Boy", "gb;gbc" } };
static const SDL_FRect load_button = { 140.0f, 194.0f, 200.0f, 44.0f };  // finestra 480x432

static void SDLCALL on_rom_selected(void* userdata, const char* const* filelist, int filter) {
    if (filelist && filelist[0]) {      // NULL is error
        SDL_strlcpy(pending_rom, filelist[0], sizeof(pending_rom));
    }
}

int main(int argc, char* argv[]) {
    // ---------- Inizializzazione Game Boy ----------
    gameBoy gb;
    memset(&gb, 0, sizeof(gameBoy));
    cpu_init(&gb.cpu);
    apu_init(&gb);
    gb.apu.sampling_timer = 87;
    gb.apu.buffer_index = 0;
    gb.memory[0xFF26] = 0xF1;
    gb.memory[0xFF00] = 0xCF;
    gb.memory[0xFF40] = 0x91;
    gb.memory[0xFF47] = 0xFC;
    gb.memory[0xFF48] = 0xFF;
    gb.memory[0xFF49] = 0xFF;
    gb.buttons = 0x0F;
    gb.cross_key = 0x0F;

    // ---------- SDL: finestra, renderer, texture ----------
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    int win_width = renderGB ? 600 : 160 * 3;
    int win_height = renderGB ? 900 : 144 * 3;

    SDL_Window* window = SDL_CreateWindow("Game Boy Emulator", win_width, win_height, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_STREAMING, 160, 144);

    static uint32_t pixels[160 * 144];
    uint64_t frame_start = SDL_GetTicksNS();

    // ---------- Audio ----------
    SDL_AudioSpec spec;
    spec.freq = 48000;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 1;

    SDL_AudioStream* audio_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    SDL_ResumeAudioStreamDevice(audio_stream);

    float silence[1600] = {0};
    SDL_PutAudioStreamData(audio_stream, silence, sizeof(silence));

    // ---------- Texture della scocca (opzionale) ----------
    SDL_Texture* bg_texture = NULL;

    if (renderGB) {
        char bmp_path[512];
        SDL_snprintf(bmp_path, sizeof(bmp_path), "%sassets/gb.bmp", SDL_GetBasePath());

        SDL_Surface* bg_surface = SDL_LoadBMP(bmp_path);
        if (bg_surface) {
            bg_texture = SDL_CreateTextureFromSurface(renderer, bg_surface);
            SDL_DestroySurface(bg_surface);
        } else {
            printf("Attenzione: impossibile caricare gb.bmp. Errore: %s\n", SDL_GetError());
        }
    }

    // ---------- PLACEHOLDER CARICAMENTO ROM ----------
    // METTI QUI il caricamento della ROM (file picker, drag & drop, argv[1]...).
    // Quando la ROM e' caricata con successo, imposta: hasRom = 1;
    // Per provare subito, puoi mettere temporaneamente:
    //     load_rom(&gb, "percorso/della/rom.gb");
    //     hasRom = 1;
    // ---------------------------------------------------
    //load_rom(&gb, "C:\\Users\\lerco\\Desktop\\emuGB2\\roms\\Pokemon red.gb");
    //hasRom = 1;

    // ---------- Ciclo principale ----------
    int running = 1;
    while (running) {
        if (pending_rom[0]) {
            load_rom(&gb, pending_rom);
            hasRom = 1;
            pending_rom[0] = '\0';
        }
        if (hasRom) {
            int cycles = 0;
            int result = cpu_step(&gb, &cycles);

            if (result != 0) running = 0;

            update_cycles(&gb, cycles);
            update_ppu(&gb, cycles);
            update_apu(&gb, cycles);

            static int last_ly = -1;
            int current_ly = gb.memory[0xFF44];

            if (current_ly == 144 && last_ly != 144) {
                last_ly = 144;

                // 1. Audio
                SDL_PutAudioStreamData(audio_stream, gb.apu.buffer,
                                       gb.apu.buffer_index * sizeof(float));
                gb.apu.buffer_index = 0;

                // 2. Video
                convert_framebuffer(&gb, pixels, use_classic_color);
                SDL_UpdateTexture(texture, NULL, pixels, 160 * sizeof(uint32_t));

                SDL_RenderClear(renderer);

                if (renderGB && bg_texture != NULL) {
                    SDL_RenderTexture(renderer, bg_texture, NULL, NULL);

                    SDL_FRect game_screen;
                    game_screen.x = 141.0f;
                    game_screen.y = 116.0f;
                    game_screen.w = 160.0f * 2.0f;
                    game_screen.h = 133.0f * 2.0f;

                    SDL_RenderTexture(renderer, texture, NULL, &game_screen);

                    DrawGlowingCircle(renderer, 86.0f, 207.0f, 4.0f);

                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                } else {
                    SDL_RenderTexture(renderer, texture, NULL, NULL);
                }

                SDL_RenderPresent(renderer);

                // 3. Eventi input
                poll_events(&gb, &running, window);

                // 4. Timing del frame
                uint64_t elapsed = SDL_GetTicksNS() - frame_start;
                if (elapsed < FRAME_NS) { SDL_DelayNS(FRAME_NS - elapsed); }
                frame_start += FRAME_NS;
                if (SDL_GetTicksNS() - frame_start > FRAME_NS * 4) frame_start = SDL_GetTicksNS();
            }
            last_ly = current_ly;

            int int_cycles = handle_interrupts(&gb);
            if (int_cycles > 0) {
                update_cycles(&gb, int_cycles);
                update_ppu(&gb, int_cycles);
                update_apu(&gb, int_cycles);
            }
        } else {
            SDL_Event e;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_EVENT_QUIT) running = 0;
                if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) {
                    SDL_FPoint p = { e.button.x, e.button.y };
                    if (SDL_PointInRectFloat(&p, &load_button)) {
                        SDL_ShowOpenFileDialog(on_rom_selected, NULL, window, rom_filters, 1, NULL, false);
                    }
                }
            }

            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderClear(renderer);

            SDL_SetRenderDrawColor(renderer, 70, 120, 200, 255);
            SDL_RenderFillRect(renderer, &load_button);

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, load_button.x + 60, load_button.y + 18, "Carica ROM");

            SDL_RenderPresent(renderer);
            SDL_Delay(10);
        }
    }

    // ---------- Pulizia finale ----------
    if (bg_texture) SDL_DestroyTexture(bg_texture);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}