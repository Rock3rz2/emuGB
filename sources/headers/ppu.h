#pragma once

#include <stdint.h>

typedef struct gameBoy gameBoy;

typedef struct {
	int dot_counter;
	uint8_t mode;

	
} ppu;

void update_ppu(gameBoy* gb, int cycles);

uint8_t pixel_color(uint8_t lo, uint8_t hi, int c);

uint16_t get_tile_address(gameBoy* gb, uint8_t tile_index);

uint8_t get_tile_pixel_color(gameBoy* gb, uint16_t X, uint16_t Y);

void render_scanline(gameBoy* gb);

uint32_t convert_color(uint8_t color, int use_classic_green);

void convert_framebuffer(gameBoy* gb, uint32_t* out, int use_classic_color);

uint8_t get_window_pixel_color(gameBoy* gb, int X, int Y);

void check_LYC(gameBoy* gb);
