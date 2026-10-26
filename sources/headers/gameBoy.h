#pragma once

#include "cpu.h"
#include "ppu.h"
#include "apu.h"

typedef struct gameBoy{
	cpu cpu;
	uint8_t memory[65536];
	ppu ppu;
	uint8_t framebuffer[160 * 144];
	apu apu;


	uint8_t* rom_data;
	uint32_t rom_size;
	uint16_t current_rom_bank;

	uint8_t cross_key;
	uint8_t buttons;

	uint8_t enabledRam;
	uint8_t MBC1;

	uint8_t RAM[8192*4];
	uint8_t currentRam;

	char save_path[512];

	uint8_t cart_type;
	uint8_t rtc_registers[5];

} gameBoy;

int cpu_step(gameBoy* gb, int* cycles_out);

int load_rom(gameBoy* gb, const char* path);

void update_cycles(gameBoy* gb, int cycles);

int handle_interrupts(gameBoy* gb);

uint8_t read8(gameBoy* gb, uint16_t addr);

void write8(gameBoy* gb, uint16_t addr, uint8_t value);

void mbc1_write(gameBoy* gb, uint16_t addr, uint8_t value);

void mbc3_write(gameBoy* gb, uint16_t addr, uint8_t value);

void mbc5_write(gameBoy* gb, uint16_t addr, uint8_t value);

void save_ram(gameBoy* gb);

