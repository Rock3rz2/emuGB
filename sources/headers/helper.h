#pragma once

#include <stdint.h>
#include "cpu.h"
#include "gameBoy.h"

#define FLAG_Z (1 << 7)
#define FLAG_N (1 << 6)
#define FLAG_H (1 << 5)
#define FLAG_C (1 << 4)

void inc_r8(cpu* c, uint8_t* reg);
void dec_r8(cpu* c, uint8_t* reg);
void rst_addr(cpu* c, uint8_t* m, uint16_t addr);

void set_flag_z(cpu* c, uint8_t* reg);
void set_flag_h_inc(cpu* c, uint8_t* reg);
void set_flag_h_dec(cpu* c, uint8_t* reg);
void set_flag_to(cpu* c, uint8_t FLAG, uint8_t value);
void check_z_flag(cpu* c, uint8_t reg);

uint8_t* get_r8(gameBoy* gb, uint8_t idx);
void ALU(gameBoy* gb, uint8_t opcode);

void cb_step(gameBoy* gb, uint8_t cb);
uint16_t* get_r16(gameBoy* gb, uint8_t idx);
