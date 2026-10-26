//
// Created by lerco on 09/10/2026.
//
#include "utils.h"
#include "math.h"

// Funzione per disegnare un cerchio pieno in SDL3
// Funzione per disegnare un LED con gradiente luminoso
void DrawGlowingCircle(SDL_Renderer* renderer, float center_x, float center_y, float radius) {
    for (float w = 0; w < radius * 2; w++) {
        for (float h = 0; h < radius * 2; h++) {
            float dx = radius - w;
            float dy = radius - h;
            float distance = sqrt(dx * dx + dy * dy); // Teorema di Pitagora per la distanza

            if (distance <= radius) {
                // L'intensità va da 1.0 (esattamente al centro) a 0.0 (sul bordo estremo)
                float intensity = 1.0f - (distance / radius);

                // Centro (intensity = 1.0): R=255, G=150, B=150 (Rosso chiarissimo/Rosato)
                // Bordo (intensity = 0.0): R=100, G=0, B=0 (Rosso scuro)
                uint8_t r = (uint8_t)(150 + (105 * intensity));
                uint8_t g = (uint8_t)(220 * intensity);
                uint8_t b = (uint8_t)(220 * intensity);

                SDL_SetRenderDrawColor(renderer, r, g, b, 255);
                SDL_RenderPoint(renderer, center_x + dx, center_y + dy);
            }
        }
    }
}
