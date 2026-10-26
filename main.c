#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>
#include <stdio.h>
#include <stdint.h>
#include "cpu.h"
#include "gameBoy.h"
#include "utils.h"

#define FRAME_NS 16742706ULL

// FLAG: 1 = Mostra la cornice del Game Boy, 0 = Solo schermo
static int renderGB = 0;
static int use_classic_color = 0;

void set_key(gameBoy* gb, SDL_Keycode key, int pressed) {
    switch (key) {
    case SDLK_W:
          if (pressed) gb->cross_key &= ~(1 << 2);
          else         gb->cross_key |= (1 << 2);
       break;
    case SDLK_S:
          if (pressed) gb->cross_key &= ~(1 << 3);
          else         gb->cross_key |= (1 << 3);
       break;
    case SDLK_A:
          if (pressed) gb->cross_key &= ~(1 << 1);
          else         gb->cross_key |= (1 << 1);
       break;
    case SDLK_D:
          if (pressed) gb->cross_key &= ~1;
          else         gb->cross_key |= 1;
       break;
    case SDLK_F:
          if (pressed)gb->buttons &= ~1;
          else gb->buttons |= 1;
       break;
    case SDLK_G:
          if (pressed)gb->buttons &= ~(1 << 1);
          else gb->buttons |= (1 << 1);
       break;
    case SDLK_R:
          if (pressed)gb->buttons &= ~(1 << 3);
          else  gb->buttons |= (1 << 3);
          break;
    case SDLK_T:
       if (pressed)gb->buttons &= ~(1 << 2);
       else  gb->buttons |= (1 << 2);
       break;
    default:
       ;
    }
}

int main(int argc, char* argv[]) {
    gameBoy gb;
    memset(&gb, 0, sizeof(gameBoy));
    cpu_init(&gb.cpu);
    apu_init(&gb);
    gb.apu.sampling_timer = 87;
    gb.apu.buffer_index = 0; // INIZIALIZZA APU
    gb.memory[0xFF26] = 0xF1; // Accende l'audio a livello hardware di default
    gb.memory[0xFF00] = 0xCF;
    gb.memory[0xFF40] = 0x91;
    gb.memory[0xFF47] = 0xFC;
    gb.memory[0xFF48] = 0xFF;
    gb.memory[0xFF49] = 0xFF;
    gb.buttons = 0x0F;
    gb.cross_key = 0x0F;

    load_rom(&gb, "C:\\Users\\lerco\\Desktop\\emuGB2\\roms\\Pokemon red.gb");

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    // --- IMPOSTAZIONE FINESTRA DINAMICA ---
    int win_width = renderGB ? 600 : 160 * 3;
    int win_height = renderGB ? 900 : 144 * 3;

    SDL_Window* window = SDL_CreateWindow("Game Boy Emulator", win_width, win_height, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 160, 144);

    static uint32_t pixels[160 * 144];
    uint64_t frame_start = SDL_GetTicksNS();

    //AUDIO
    SDL_AudioSpec spec;
    spec.freq = 48000;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 1;

    SDL_AudioStream* audio_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    SDL_ResumeAudioStreamDevice(audio_stream);

    float silence[1600] = {0};
    SDL_PutAudioStreamData(audio_stream, silence, sizeof(silence));
    SDL_ResumeAudioStreamDevice(audio_stream);

    // --- CARICAMENTO TEXTURE SCOCCA ---
    SDL_Texture* bg_texture = NULL;

    if (renderGB) {
       SDL_Surface* bg_surface = SDL_LoadBMP("C:\\Users\\lerco\\Desktop\\emuGB2\\assets\\gb.bmp");
       if (bg_surface) {
           bg_texture = SDL_CreateTextureFromSurface(renderer, bg_surface);
           SDL_DestroySurface(bg_surface);
       } else {
           printf("Attenzione: Impossibile caricare gb.bmp. Errore: %s\n", SDL_GetError());
       }
    }

    int running = 1;
    while (running) {
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

          // 1. SPEDIAMO L'AUDIO
          SDL_PutAudioStreamData(audio_stream, gb.apu.buffer, gb.apu.buffer_index * sizeof(float));
          gb.apu.buffer_index = 0; // Azzeriamo per il frame successivo

          // 2. SPEDIAMO IL VIDEO
          convert_framebuffer(&gb, pixels, use_classic_color);
          SDL_UpdateTexture(texture, NULL, pixels, 160 * sizeof(uint32_t));

          SDL_RenderClear(renderer);

          // --- LOGICA DI RENDER CON FLAG ---
          if (renderGB && bg_texture != NULL) {
              // A. Disegna la scocca a tutto schermo
              SDL_RenderTexture(renderer, bg_texture, NULL, NULL);

              // B. Disegna lo schermo nel "buco"
              SDL_FRect game_screen;
              game_screen.x = 141.0f;
              game_screen.y = 116.0f;
              game_screen.w = 160.0f * 2.0f;
              game_screen.h = 133.0f * 2.0f;

              SDL_RenderTexture(renderer, texture, NULL, &game_screen);

             float led_x = 86.0f;     // <-- Regola la coordinata X del CENTRO
             float led_y = 207.0f;    // <-- Regola la coordinata Y del CENTRO
             float led_radius = 4.0f; // <-- Regola la grandezza

             DrawGlowingCircle(renderer, led_x, led_y, led_radius);

             // Ripristina il colore del renderer a nero
             SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
          } else {
              // Render classico: il gioco occupa tutta la finestra
              SDL_RenderTexture(renderer, texture, NULL, NULL);
          }

          SDL_RenderPresent(renderer);

          // 3. EVENTI INPUT
          SDL_Event e;
          while (SDL_PollEvent(&e)) {
             if (e.type == SDL_EVENT_QUIT) running = 0;
             if (e.type == SDL_EVENT_KEY_DOWN) { set_key(&gb, e.key.key, 1); };
             if (e.type == SDL_EVENT_KEY_UP) { set_key(&gb, e.key.key, 0); };
          }

          // 4. TIMING DEL FRAME
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
    }

    // Pulizia finale
    if (bg_texture) SDL_DestroyTexture(bg_texture);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}