#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "ppu.h"
#include "gameBoy.h"

//LY = m[0xFF44]

//tile map 0x9800-0x9BFF

//2 = OAM search
//3 = pixel transfer
//0 = H-Blank
//1 = V-Blank

void check_LYC(gameBoy* gb)
{
	uint8_t LY = gb->memory[0xFF44];
	uint8_t LYC = gb->memory[0xFF45];
	if (LY == LYC) {
		if (((gb->memory[0xFF41] & 0x04) == 0) && ((gb->memory[0xFF41] & 0x40) !=0)) {
			gb->memory[0xFF0F] |= 0x02;

		}

		gb->memory[0xFF41] |= 0x04;
	}
	else {
		gb->memory[0xFF41] &= ~0x04;
	}

}

void update_ppu(gameBoy* gb, int cycles)
{
	gb->ppu.dot_counter += cycles;
	switch (gb->ppu.mode) {
	case 2:
		if (gb->ppu.dot_counter >= 80) {
			gb->ppu.dot_counter -= 80;
			gb->ppu.mode = 3;
		}
		break;

	case 3:
		if (gb->ppu.dot_counter >= 172) {
			gb->ppu.dot_counter -= 172;
			render_scanline(gb);

			gb->ppu.mode = 0;
		}
		break;

	case 0:
		if (gb->ppu.dot_counter >= 204) {
			gb->ppu.dot_counter -= 204;
			gb->memory[0xFF44]++;
			check_LYC(gb);

			if (gb->memory[0xFF44] == 144) {
				gb->ppu.mode = 1;
				gb->memory[0xFF0F] |= 0x01;
			}
			else {
				gb->ppu.mode = 2;
			}
		}
		break;

	case 1:
		if (gb->ppu.dot_counter >= 456) {
			gb->ppu.dot_counter -= 456;
			gb->memory[0xFF44]++;
			check_LYC(gb);

			if (gb->memory[0xFF44] == 154) {
				gb->memory[0xFF44] = 0;
				gb->ppu.mode = 2;
				check_LYC(gb);
			}
			
		}
		break;
		
	}
	gb->memory[0xFF41] = (gb->memory[0xFF41] & 0xFC) | gb->ppu.mode;
}

uint8_t pixel_color(uint8_t lo, uint8_t hi, int c)
{
	uint8_t bit_lo = (lo >> (7 - c)) & 0x01;
	uint8_t bit_hi = (hi >> (7 - c)) & 0x01;
	uint8_t color = (bit_hi << 1) | bit_lo;
	return color;
}

uint16_t get_tile_address(gameBoy* gb, uint8_t tile_index) {
	if (gb->memory[0xFF40] & 0x10)
		return 0x8000 + tile_index * 16;
	else
		return 0x9000 + (int8_t)tile_index * 16;
}


uint8_t get_tile_pixel(gameBoy* gb, uint8_t tile_index, int row, int c) {
	uint16_t addr = get_tile_address(gb, tile_index) + row *2;
	uint8_t lo = gb->memory[addr];
	uint8_t hi = gb->memory[addr + 1];
	return pixel_color(lo, hi, c);
}

uint8_t get_tile_pixel_color(gameBoy* gb, uint16_t X, uint16_t Y) {
	uint8_t SCX = gb->memory[0xFF43];
	uint8_t SCY = gb->memory[0xFF42];

	uint8_t world_x = (X + SCX) & 0xFF;
	uint8_t world_y = (Y + SCY) & 0xFF;

	uint8_t tile_row = world_y / 8;
	uint8_t tile_col = world_x / 8;

	uint16_t map_base = (gb->memory[0xFF40] & 0x08) ? 0x9C00 : 0x9800;
	uint16_t tile_map_addr = map_base + (tile_row * 32) + tile_col;
	uint8_t tile_index = gb->memory[tile_map_addr];

	uint8_t riga_interna = world_y % 8;
	uint8_t colonna_interna = world_x % 8;

	// raw color index 0-3: the BGP palette is applied only once in render_scanline
	return get_tile_pixel(gb, tile_index, riga_interna, colonna_interna);
}

uint8_t get_window_pixel_color(gameBoy* gb, int X, int Y)
{
	uint8_t lcdc = read8(gb, 0xFF40);

	uint8_t wx = read8(gb, 0xFF4B);
	uint8_t wy = read8(gb, 0xFF4A);

	uint8_t winX = (X - (wx - 7));
	uint8_t winY = Y - wy;

	uint8_t tileRow = winY / 8;
	uint8_t tileCol = winX / 8;

	uint16_t addr = (lcdc >> 6) & 0x01 ? 0x9C00 : 0x9800;

	uint16_t tile_map_addr = addr + tileRow * 32 + tileCol;
	uint8_t tile_index = read8(gb, tile_map_addr);

	uint8_t riga_interna = winY % 8;
	uint8_t colonna_interna = winX % 8;

	return get_tile_pixel(gb, tile_index, riga_interna, colonna_interna);

}



void render_scanline(gameBoy* gb)
{
	uint8_t lcdc = read8(gb, 0xFF40);

		uint8_t LY = gb->memory[0xFF44];
		uint8_t bgp = gb->memory[0xFF47];
		uint8_t bg_idx[160];
		int spr_x[160];   // X dello sprite che ha già vinto su ogni pixel
		for (int i = 0; i < 160; i++) spr_x[i] = 1000;

		uint8_t wy = gb->memory[0xFF4A];
		int win_x = gb->memory[0xFF4B] - 7;
		uint8_t raw;
		for (int i = 0; i < 160; i++) {
			if (((lcdc >> 5) & 0x01) && LY >= wy && i >= win_x) {
				raw = get_window_pixel_color(gb, i, LY);
			}
			else {
				raw = get_tile_pixel_color(gb, i, LY);
			}

			  // raw index 0-3
			bg_idx[i] = raw;
			gb->framebuffer[LY * 160 + i] = (bgp >> (raw * 2)) & 3;
		}
		if ((lcdc >> 1) & 0x01) {

		for (int i = 0; i < 40; i++) {
			uint16_t oam = 0xFE00 + i * 4;
			int y = read8(gb, oam) - 16;
			int x = read8(gb, oam + 1) - 8;
			uint8_t tile = read8(gb, oam + 2);
			uint8_t flags = read8(gb, oam + 3);

			if (LY < y || LY >= y + 8) continue;

			uint8_t priority = (flags >> 7) & 1;
			uint8_t flipY = (flags >> 6) & 1;
			uint8_t flipX = (flags >> 5) & 1;
			uint8_t palette = (flags >> 4) & 1;

			uint8_t row = !flipY ? LY - y : 7 - (LY - y);
			uint16_t addr = 0x8000 + tile * 16 + row * 2;
			uint8_t lo = read8(gb, addr);
			uint8_t hi = read8(gb, addr + 1);

			uint8_t obp = read8(gb, palette ? 0xFF49 : 0xFF48);

			for (int j = 0; j < 8; j++) {
				uint8_t col = !flipX ? j : 7 - j;
				uint8_t color = pixel_color(lo, hi, col);
				if (color == 0) continue;                        // transparent
				int sx = x + j;
				if (sx < 0 || sx >= 160) continue;
				if (spr_x[sx] <= x) continue;                   // DMG priority: X lower, with same index lower
				spr_x[sx] = x;
				if (priority && bg_idx[sx] != 0) continue;       // sprite behind BG

				gb->framebuffer[LY * 160 + sx] = (obp >> (color * 2)) & 3;
			}
		}
	}
}

uint32_t convert_color(uint8_t color, int use_classic_green)
{
	if (use_classic_green) {
		// Game Boy Palette (Green)
		switch (color) {
		case 0: return 0xC4CFA1FF; // "White"
		case 1: return 0x8B966CFF; // Light gray
		case 2: return 0x4C533CFF; // Dark gray
		case 3: return 0x1F211AFF; // "Black"
		default: return 0xFFFFFFFF;
		}
	}
	else {
		// Pure grayscale palette
		switch (color) {
		case 0: return 0xE0E0E0FF; // Pure white
		case 1: return 0x909090FF; // Light gray
		case 2: return 0x404040FF; // Dark gray
		case 3: return 0x101010FF; // Pure black
		default: return 0xE0E0E0FF;
		}
	}
}

void convert_framebuffer(gameBoy* gb, uint32_t* out, int use_classic_color)
{
	for (int i = 0; i < 160 * 144; i++) {
		out[i] = convert_color(gb->framebuffer[i], use_classic_color);
	}
}	