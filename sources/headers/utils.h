//
// Created by lerco on 09/10/2026.
//

#ifndef EMUGB2_UTILS_H
#define EMUGB2_UTILS_H

#endif //EMUGB2_UTILS_H

#include "SDL3/SDL_render.h"
#include "gameBoy.h"
void DrawGlowingCircle(SDL_Renderer* renderer, float center_x, float center_y, float radius);
void set_key(gameBoy* gb, SDL_Keycode key, int pressed);
void poll_events(gameBoy* gb, int* running, SDL_Window* window);

