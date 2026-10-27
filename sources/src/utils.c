//
// Created by lerco on 09/10/2026.
//
#include "utils.h"
#include "math.h"

// Function to draw a filled circle in SDL3
// Function to draw a LED with a light gradient
void DrawGlowingCircle(SDL_Renderer* renderer, float center_x, float center_y, float radius) {
    for (float w = 0; w < radius * 2; w++) {
        for (float h = 0; h < radius * 2; h++) {
            float dx = radius - w;
            float dy = radius - h;
            float distance = sqrt(dx * dx + dy * dy); // Pythagorean theorem for the distance

            if (distance <= radius) {
                // Intensity goes from 1.0 (exactly at the center) to 0.0 (on the outer edge)
                float intensity = 1.0f - (distance / radius);

                // Center (intensity = 1.0): R=255, G=150, B=150 (Very light red/Pinkish)
                // Edge (intensity = 0.0): R=100, G=0, B=0 (Dark red)
                uint8_t r = (uint8_t)(150 + (105 * intensity));
                uint8_t g = (uint8_t)(220 * intensity);
                uint8_t b = (uint8_t)(220 * intensity);

                SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                SDL_RenderPoint(renderer, center_x + dx, center_y + dy);
            }
        }
    }
}
// SET KEYS HERE
void set_key(gameBoy* gb, SDL_Keycode key, int pressed) {
    switch (key) {
    case SDLK_W: //up
        if (pressed) gb->cross_key &= ~(1 << 2);
        else         gb->cross_key |= (1 << 2);
        break;
    case SDLK_S: //Down
        if (pressed) gb->cross_key &= ~(1 << 3);
        else         gb->cross_key |= (1 << 3);
        break;
    case SDLK_A: //right
        if (pressed) gb->cross_key &= ~(1 << 1);
        else         gb->cross_key |= (1 << 1);
        break;
    case SDLK_D: //left
        if (pressed) gb->cross_key &= ~1;
        else         gb->cross_key |= 1;
        break;
    case SDLK_F: //button A
        if (pressed)gb->buttons &= ~1;
        else gb->buttons |= 1;
        break;
    case SDLK_G: // button b
        if (pressed)gb->buttons &= ~(1 << 1);
        else gb->buttons |= (1 << 1);
        break;
    case SDLK_R: // start
        if (pressed)gb->buttons &= ~(1 << 3);
        else  gb->buttons |= (1 << 3);
        break;
    case SDLK_T: // select
        if (pressed)gb->buttons &= ~(1 << 2);
        else  gb->buttons |= (1 << 2);
        break;
    default:
        ;
    }
}

void poll_events(gameBoy* gb, int* running, SDL_Window* window) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) *running = 0;
        if (e.type == SDL_EVENT_KEY_DOWN) set_key(gb, e.key.key, 1);
        if (e.type == SDL_EVENT_KEY_UP)   set_key(gb, e.key.key, 0);
        if (e.type == SDL_EVENT_DROP_FILE) { /* e.drop.data = path */ }
    }
}
