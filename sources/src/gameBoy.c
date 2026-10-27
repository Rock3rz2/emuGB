#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>	
#include <stdlib.h>
#include <string.h>
#include "gameBoy.h"
#include "helper.h"


int cpu_step(gameBoy* gb, int* cycles_out) {

	static uint16_t trace_pc[16];
	static uint8_t  trace_op[16];
	static int      trace_i = 0;

	cpu* c = &gb->cpu;

	if (c->ei_scheduled) {
		c->ei_scheduled = 0;
		c->ime = 1;
	}

	if (c->halted) {
		if ((read8(gb, 0xFF0F) & read8(gb, 0xFFFF)) != 0) {
			c->halted = 0;
		}
		else {
			*cycles_out += 4;
			return 0;
		}
	}

	uint8_t opcode = read8(gb, c->pc);
	c->pc++;

	trace_pc[trace_i] = c->pc - 1;
	trace_op[trace_i] = opcode;
	trace_i = (trace_i + 1) & 15;

	if (opcode >= 0x40 && opcode <= 0x7F && opcode != 0x76) {
		uint8_t s = opcode & 7, d = (opcode >> 3) & 7;
		uint8_t v = (s == 6) ? read8(gb, c->hl.reg_hl) : *get_r8(gb, s);
		if (d == 6) write8(gb, c->hl.reg_hl, v); else *get_r8(gb, d) = v;
		*cycles_out += (s == 6 || d == 6) ? 8 : 4;
		return 0;
	}

	if (opcode >= 0x80 && opcode <= 0xBF) {
		ALU(gb, opcode);

		if ((opcode & 0x07) == 0x06) {
			*cycles_out += 8;
		}
		else {
			*cycles_out += 4;
		}
		return 0;
	}

	if ((opcode & 0xCF) == 0x01) {  // LD rr,d16
		uint16_t* rr = get_r16(gb, (opcode >> 4) & 3);
		*rr = read8(gb, c->pc) | (read8(gb, c->pc + 1) << 8);
		c->pc += 2;
		*cycles_out += 12;
		return 0;
	}

	if ((opcode & 0xCF) == 0x03) {  // INC rr
		uint16_t* rr = get_r16(gb, (opcode >> 4) & 3);
		(*rr)++;
		*cycles_out += 8;
		return 0;
	}

	if ((opcode & 0xCF) == 0x0B) {  // DEC rr
		uint16_t* rr = get_r16(gb, (opcode >> 4) & 3);
		(*rr)--;
		*cycles_out += 8;
		return 0;
	}

	if ((opcode & 0xCF) == 0x09) {  // ADD HL,rr
		uint16_t* rr = get_r16(gb, (opcode >> 4) & 3);
		uint32_t res = (uint32_t)c->hl.reg_hl + (uint32_t)*rr;

		c->af.reg_f &= ~FLAG_N;

		if ((c->hl.reg_hl & 0x0FFF) + (*rr & 0x0FFF) > 0x0FFF) {
			c->af.reg_f |= FLAG_H;
		}
		else {
			c->af.reg_f &= ~FLAG_H;
		}

		if (res > 0xFFFF) {
			c->af.reg_f |= FLAG_C;
		}
		else {
			c->af.reg_f &= ~FLAG_C;
		}

		c->hl.reg_hl = (uint16_t)res;
		*cycles_out += 8;
		return 0;
	}

	switch (opcode) {
	case 0x00:
		*cycles_out += 4;
		break;

	case 0x02:
		write8(gb, c->bc.reg_bc, c->af.reg_a);
		*cycles_out += 8;
		break;

	case 0x03:
		c->bc.reg_bc++;
		*cycles_out += 8;
		break;

	case 0x04:
		inc_r8(c, &c->bc.reg_b);
		*cycles_out += 4;
		break;

	case 0x05:
		dec_r8(c, &c->bc.reg_b);
		*cycles_out += 4;
		break;

	case 0x06:
		c->bc.reg_b = read8(gb, c->pc);
		c->pc++;
		*cycles_out += 8;
		break;

	case 0x07: { // RLCA
		uint8_t v = c->af.reg_a;
		uint8_t out = (v >> 7) & 1;
		c->af.reg_a = (v << 1) | out;
		c->af.reg_f &= ~FLAG_Z;
		c->af.reg_f &= ~FLAG_N;
		c->af.reg_f &= ~FLAG_H;
		if (out) c->af.reg_f |= FLAG_C; else c->af.reg_f &= ~FLAG_C;
		*cycles_out += 4;
		break;
	}

	case 0x08: { // LD (a16),SP
		uint16_t addr = read8(gb, c->pc) | (read8(gb, c->pc + 1) << 8);
		c->pc += 2;
		write8(gb, addr, c->sp & 0xFF);
		write8(gb, addr + 1, (c->sp >> 8) & 0xFF);
		*cycles_out += 20;
		break;
	}

	case 0x0A:
		c->af.reg_a = read8(gb, c->bc.reg_bc);
		*cycles_out += 8;
		break;

	case 0x0C:
		inc_r8(c, &c->bc.reg_c);
		*cycles_out += 4;
		break;

	case 0x0D:
		dec_r8(c, &c->bc.reg_c);
		*cycles_out += 4;
		break;

	case 0x0E:
		c->bc.reg_c = read8(gb, c->pc);
		c->pc++;
		*cycles_out += 8;
		break;

	case 0x0F: { // RRCA
		uint8_t v = c->af.reg_a;
		uint8_t out = v & 1;
		c->af.reg_a = (v >> 1) | (out << 7);
		c->af.reg_f &= ~FLAG_Z;
		c->af.reg_f &= ~FLAG_N;
		c->af.reg_f &= ~FLAG_H;
		if (out) c->af.reg_f |= FLAG_C; else c->af.reg_f &= ~FLAG_C;
		*cycles_out += 4;
		break;
	}

	case 0x10: // STOP
		c->pc++;
		*cycles_out += 4;
		break;

	case 0x12:
		write8(gb, c->de.reg_de, c->af.reg_a);
		*cycles_out += 8;
		break;

	case 0x13:
		c->de.reg_de++;
		*cycles_out += 8;
		break;

	case 0x14:
		inc_r8(c, &c->de.reg_d);
		*cycles_out += 4;
		break;

	case 0x15:
		dec_r8(c, &c->de.reg_d);
		*cycles_out += 4;
		break;

	case 0x16:
		c->de.reg_d = read8(gb, c->pc);
		c->pc++;
		*cycles_out += 8;
		break;

	case 0x17: { // RLA
		uint8_t v = c->af.reg_a;
		uint8_t old_carry = (c->af.reg_f & FLAG_C) ? 1 : 0;
		uint8_t out = (v >> 7) & 1;
		c->af.reg_a = (v << 1) | old_carry;
		c->af.reg_f &= ~FLAG_Z;
		c->af.reg_f &= ~FLAG_N;
		c->af.reg_f &= ~FLAG_H;
		if (out) c->af.reg_f |= FLAG_C; else c->af.reg_f &= ~FLAG_C;
		*cycles_out += 4;
		break;
	}

	case 0x18: {
		int8_t r8 = (int8_t)read8(gb, c->pc);
		c->pc++;
		c->pc += r8;
		*cycles_out += 12;
		break;
	}

	case 0x1A:
		c->af.reg_a = read8(gb, c->de.reg_de);
		*cycles_out += 8;
		break;

	case 0x1C:
		inc_r8(c, &c->de.reg_e);
		*cycles_out += 4;
		break;

	case 0x1D:
		dec_r8(c, &c->de.reg_e);
		*cycles_out += 4;
		break;

	case 0x1E:
		c->de.reg_e = read8(gb, c->pc);
		c->pc++;
		*cycles_out += 8;
		break;

	case 0x1F: { // RRA
		uint8_t v = c->af.reg_a;
		uint8_t old_carry = (c->af.reg_f & FLAG_C) ? 1 : 0;
		uint8_t out = v & 1;
		c->af.reg_a = (v >> 1) | (old_carry << 7);
		c->af.reg_f &= ~FLAG_Z;
		c->af.reg_f &= ~FLAG_N;
		c->af.reg_f &= ~FLAG_H;
		if (out) c->af.reg_f |= FLAG_C; else c->af.reg_f &= ~FLAG_C;
		*cycles_out += 4;
		break;
	}

	case 0x20: {
		int8_t r8 = (int8_t)read8(gb, c->pc);
		c->pc++;
		if ((c->af.reg_f & FLAG_Z) == 0) {
			c->pc += r8;
			*cycles_out += 12;
			break;
		}
		*cycles_out += 8;
		break;
	}

	case 0x22:
		write8(gb, c->hl.reg_hl, c->af.reg_a);
		c->hl.reg_hl++;
		*cycles_out += 8;
		break;

	case 0x23:
		c->hl.reg_hl++;
		*cycles_out += 8;
		break;

	case 0x24:
		inc_r8(c, &c->hl.reg_h);
		*cycles_out += 4;
		break;

	case 0x25:
		dec_r8(c, &c->hl.reg_h);
		*cycles_out += 4;
		break;

	case 0x26:
		c->hl.reg_h = read8(gb, c->pc);
		c->pc++;
		*cycles_out += 8;
		break;

	case 0x27: { // DAA
		uint8_t a = c->af.reg_a;
		uint8_t adjust = 0;
		uint8_t carry = 0;

		if (c->af.reg_f & FLAG_N) {
			if (c->af.reg_f & FLAG_H) adjust |= 0x06;
			if (c->af.reg_f & FLAG_C) adjust |= 0x60;
			a -= adjust;
		}
		else {
			if ((c->af.reg_f & FLAG_H) || (a & 0x0F) > 0x09) adjust |= 0x06;
			if ((c->af.reg_f & FLAG_C) || a > 0x99) { adjust |= 0x60; carry = 1; }
			a += adjust;
		}

		c->af.reg_a = a;
		check_z_flag(c, a);
		c->af.reg_f &= ~FLAG_H;
		if (carry) c->af.reg_f |= FLAG_C; else if (!(c->af.reg_f & FLAG_N)) { /* C può restare 1 se già settato sopra */ }
		*cycles_out += 4;
		break;
	}

	case 0x28: {
		int8_t r8 = (int8_t)read8(gb, c->pc);
		c->pc++;
		if ((c->af.reg_f & FLAG_Z) != 0) {
			c->pc += r8;
			*cycles_out += 12;
			break;
		}
		*cycles_out += 8;
		break;
	}

	case 0x2A:
		c->af.reg_a = read8(gb, c->hl.reg_hl);
		c->hl.reg_hl++;
		*cycles_out += 8;
		break;

	case 0x2C:
		inc_r8(c, &c->hl.reg_l);
		*cycles_out += 4;
		break;

	case 0x2D:
		dec_r8(c, &c->hl.reg_l);
		*cycles_out += 4;
		break;

	case 0x2E:
		c->hl.reg_l = read8(gb, c->pc);
		c->pc++;
		*cycles_out += 8;
		break;

	case 0x2F: // CPL
		c->af.reg_a = ~c->af.reg_a;
		c->af.reg_f |= FLAG_N;
		c->af.reg_f |= FLAG_H;
		*cycles_out += 4;
		break;

	case 0x30: {
		int8_t r8 = (int8_t)read8(gb, c->pc);
		c->pc++;
		if ((c->af.reg_f & FLAG_C) == 0) {
			c->pc += r8;
			*cycles_out += 12;
			break;
		}
		*cycles_out += 8;
		break;
	}

	case 0x32:
		write8(gb, c->hl.reg_hl, c->af.reg_a);
		c->hl.reg_hl--;
		*cycles_out += 8;
		break;

	case 0x33:
		c->sp++;
		*cycles_out += 8;
		break;

	case 0x34: { // INC (HL)
		uint8_t temp = read8(gb, c->hl.reg_hl);
		inc_r8(c, &temp);
		write8(gb, c->hl.reg_hl, temp);
		*cycles_out += 12;
		break;
	}

	case 0x35: { // DEC (HL)
		uint8_t temp = read8(gb, c->hl.reg_hl);
		dec_r8(c, &temp);
		write8(gb, c->hl.reg_hl, temp);
		*cycles_out += 12;
		break;
	}

	case 0x36:
		write8(gb, c->hl.reg_hl, read8(gb, c->pc));
		c->pc++;
		*cycles_out += 12;
		break;

	case 0x37: // SCF
		c->af.reg_f &= ~FLAG_N;
		c->af.reg_f &= ~FLAG_H;
		c->af.reg_f |= FLAG_C;
		*cycles_out += 4;
		break;

	case 0x38: {
		int8_t r8 = (int8_t)read8(gb, c->pc);
		c->pc++;
		if ((c->af.reg_f & FLAG_C) != 0) {
			c->pc += r8;
			*cycles_out += 12;
			break;
		}
		*cycles_out += 8;
		break;
	}

	case 0x3A:
		c->af.reg_a = read8(gb, c->hl.reg_hl);
		c->hl.reg_hl--;
		*cycles_out += 8;
		break;

	case 0x3B:
		c->sp--;
		*cycles_out += 8;
		break;

	case 0x3C:
		inc_r8(c, &c->af.reg_a);
		*cycles_out += 4;
		break;

	case 0x3D:
		dec_r8(c, &c->af.reg_a);
		*cycles_out += 4;
		break;

	case 0x3E:
		c->af.reg_a = read8(gb, c->pc);
		c->pc++;
		*cycles_out += 8;
		break;

	case 0x3F: // CCF
		c->af.reg_f &= ~FLAG_N;
		c->af.reg_f &= ~FLAG_H;
		if (c->af.reg_f & FLAG_C) c->af.reg_f &= ~FLAG_C; else c->af.reg_f |= FLAG_C;
		*cycles_out += 4;
		break;

	case 0x76: { // HALT
		if (c->ime == 1) {
			c->halted = 1;
		}
		else {
			// ime spento: HALT bug se c'è già un interrupt pending,
			// altrimenti halted normale ma senza interrupt che lo sveglino (resta bloccato)
			if ((read8(gb,0xFF0F) & read8(gb, 0xFFFF)) != 0) {
				// bug: non ferma davvero, la prossima istruzione verrà "raddoppiata"
				// per ora, versione semplice: non impostare halted
			}
			else {
				c->halted = 1;
			}
		}
		*cycles_out += 4;
		break;
	}

	case 0xC0: {
		if ((c->af.reg_f & FLAG_Z) == 0) {
			uint16_t addr = read8(gb, c->sp + 1) << 8 | read8(gb, c->sp);
			c->sp += 2;
			c->pc = addr;
		}
		if ((c->af.reg_f & FLAG_Z) == 0) *cycles_out += 20; else *cycles_out += 8;
		break;
	}

	case 0xC1: {
		uint16_t addr = read8(gb, c->sp + 1) << 8 | read8(gb, c->sp);
		c->sp += 2;
		c->bc.reg_bc = addr;
		*cycles_out += 12;
		break;
	}

	case 0xC2: {
		uint16_t addr = read8(gb, c->pc) | (read8(gb, c->pc + 1) << 8);
		c->pc += 2;
		if ((c->af.reg_f & FLAG_Z) == 0) c->pc = addr;
		if ((c->af.reg_f & FLAG_Z) == 0) *cycles_out += 16; else *cycles_out += 12;
		break;
	}

	case 0xC3:
		c->pc = read8(gb, c->pc) | read8(gb, c->pc + 1) << 8;
		*cycles_out += 16;
		break;

	case 0xC4: {
		uint16_t addr = read8(gb, c->pc + 1) << 8 | read8(gb, c->pc);
		c->pc += 2;
		if ((c->af.reg_f & FLAG_Z) == 0) {
			c->sp -= 2;
			write8(gb, c->sp, c->pc & 0xFF);
			write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF) ;
			c->pc = addr;
		}
		if ((c->af.reg_f & FLAG_Z) == 0) *cycles_out += 24; else *cycles_out += 12;
		break;
	}

	case 0xC5:
		c->sp -= 2;
		write8(gb, c->sp, c->bc.reg_bc & 0xFF);
		write8(gb, c->sp + 1, (c->bc.reg_bc >> 8) & 0xFF);
		*cycles_out += 16;
		break;

	case 0xC6: {
		uint8_t val = read8(gb, c->pc);
		c->pc++;
		uint8_t* A = &c->af.reg_a;
		uint16_t res = *A + val;

		check_z_flag(c, (uint8_t)res);
		c->af.reg_f &= ~FLAG_N;

		if ((*A & 0x0F) + (val & 0x0F) > 0x0F) c->af.reg_f |= FLAG_H;
		else c->af.reg_f &= ~FLAG_H;

		if (res > 0xFF) c->af.reg_f |= FLAG_C;
		else c->af.reg_f &= ~FLAG_C;

		*A = (uint8_t)res;
		*cycles_out += 8;
		break;
	}

	case 0xC7:
		c->sp -= 2;
		write8(gb, c->sp, c->pc & 0xFF);
		write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
		c->pc = 0x0000;
		*cycles_out += 16;
		break;

	case 0xC8: {
		if ((c->af.reg_f & FLAG_Z) != 0) {
			uint16_t addr = read8(gb, c->sp + 1) << 8 | read8(gb, c->sp);
			c->sp += 2;
			c->pc = addr;
		}
		if ((c->af.reg_f & FLAG_Z) != 0) *cycles_out += 20; else *cycles_out += 8;
		break;
	}

	case 0xC9: {
		uint16_t addr = read8(gb, c->sp + 1) << 8 | read8(gb, c->sp);
		c->sp += 2;
		c->pc = addr;
		*cycles_out += 16;
		break;
	}

	case 0xCA: {
		uint16_t addr = read8(gb, c->pc) | (read8(gb, c->pc + 1) << 8);
		c->pc += 2;
		if ((c->af.reg_f & FLAG_Z) != 0) c->pc = addr;
		if ((c->af.reg_f & FLAG_Z) != 0) *cycles_out += 16; else *cycles_out += 12;
		break;
	}

	case 0xCB: {
		uint8_t cb = read8(gb, c->pc);
		c->pc++;
		cb_step(gb, cb);

		uint8_t grp = cb >> 6;
		uint8_t is_hl = (cb & 0x07) == 6;

		if (!is_hl) {
			*cycles_out += 8;
		}
		else if (grp == 1) {
			*cycles_out += 12;
		}
		else {
			*cycles_out += 16;
		}
		break;
	}

	case 0xCC: {
		uint16_t addr = read8(gb, c->pc + 1) << 8 | read8(gb, c->pc);
		c->pc += 2;
		if ((c->af.reg_f & FLAG_Z) != 0) {
			c->sp -= 2;
			write8(gb, c->sp, c->pc & 0xFF);
			write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
			c->pc = addr;
		}
		if ((c->af.reg_f & FLAG_Z) != 0) *cycles_out += 24; else *cycles_out += 12;
		break;
	}

	case 0xCD: {
		uint16_t addr = read8(gb, c->pc + 1) << 8 | read8(gb, c->pc);
		c->pc += 2;
		c->sp -= 2;
		write8(gb, c->sp, c->pc & 0xFF);
		write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
		c->pc = addr;
		*cycles_out += 24;
		break;
	}

	case 0xCE: {
		uint8_t carry = (c->af.reg_f & FLAG_C) ? 1 : 0;
		uint8_t val = read8(gb, c->pc);
		c->pc++;
		uint8_t* A = &c->af.reg_a;
		uint16_t res = *A + val + carry;

		check_z_flag(c, (uint8_t)res);
		c->af.reg_f &= ~FLAG_N;

		if ((*A & 0x0F) + (val & 0x0F) + carry > 0x0F) c->af.reg_f |= FLAG_H;
		else c->af.reg_f &= ~FLAG_H;

		if (res > 0xFF) c->af.reg_f |= FLAG_C;
		else c->af.reg_f &= ~FLAG_C;

		*A = (uint8_t)res;
		*cycles_out += 8;
		break;
	}

	case 0xCF:
		c->sp -= 2;
		write8(gb, c->sp, c->pc & 0xFF);
		write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
		c->pc = 0x0008;
		*cycles_out += 16;
		break;

	case 0xD0: {
		if ((c->af.reg_f & FLAG_C) == 0) {
			uint16_t addr = read8(gb, c->sp + 1) << 8 | read8(gb, c->sp);
			c->sp += 2;
			c->pc = addr;
		}
		if ((c->af.reg_f & FLAG_C) == 0) *cycles_out += 20; else *cycles_out += 8;
		break;
	}

	case 0xD1: {
		uint16_t addr = read8(gb, c->sp + 1) << 8 | read8(gb, c->sp);
		c->sp += 2;
		c->de.reg_de = addr;
		*cycles_out += 12;
		break;
	}

	case 0xD2: {
		uint16_t addr = read8(gb, c->pc) | (read8(gb, c->pc + 1) << 8);
		c->pc += 2;
		if ((c->af.reg_f & FLAG_C) == 0) { c->pc = addr; *cycles_out += 16; }
		else { *cycles_out += 12; }
		break;
	}

	case 0xD4: {
		uint16_t addr = read8(gb, c->pc + 1) << 8 | read8(gb, c->pc);
		c->pc += 2;
		if ((c->af.reg_f & FLAG_C) == 0) {
			c->sp -= 2;
			write8(gb, c->sp, c->pc & 0xFF);
			write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
			c->pc = addr;
		}
		if ((c->af.reg_f & FLAG_C) == 0) *cycles_out += 24; else *cycles_out += 12;
		break;
	}

	case 0xD5:
		c->sp -= 2;
		write8(gb, c->sp, c->de.reg_de & 0xFF);
		write8(gb, c->sp + 1, (c->de.reg_de >> 8) & 0xFF);
		*cycles_out += 16;
		break;

	case 0xD6: {
		uint8_t* A = &c->af.reg_a;
		uint8_t val = read8(gb, c->pc);
		c->pc++;
		uint16_t res = *A - val;

		check_z_flag(c, (uint8_t)res);
		c->af.reg_f |= FLAG_N;

		if ((*A & 0x0F) < (val & 0x0F)) c->af.reg_f |= FLAG_H;
		else c->af.reg_f &= ~FLAG_H;

		if (*A < val) c->af.reg_f |= FLAG_C;
		else c->af.reg_f &= ~FLAG_C;

		*A = (uint8_t)res;
		*cycles_out += 8;
		break;
	}

	case 0xD7:
		c->sp -= 2;
		write8(gb, c->sp, c->pc & 0xFF);
		write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
		c->pc = 0x0010;
		*cycles_out += 16;
		break;

	case 0xD8: {
		if ((c->af.reg_f & FLAG_C) != 0) {
			uint16_t addr = read8(gb, c->sp + 1) << 8 | read8(gb, c->sp);
			c->sp += 2;
			c->pc = addr;
		}
		if ((c->af.reg_f & FLAG_C) != 0) *cycles_out += 20; else *cycles_out += 8;
		break;
	}

	case 0xD9: {
		uint16_t addr = read8(gb, c->sp + 1) << 8 | read8(gb, c->sp);
		c->sp += 2;
		c->pc = addr;
		c->ime = 1;
		*cycles_out += 16;
		break;
	}

	case 0xDA: {
		uint16_t addr = read8(gb, c->pc) | (read8(gb, c->pc + 1) << 8);
		c->pc += 2;
		if ((c->af.reg_f & FLAG_C) != 0) { c->pc = addr; *cycles_out += 16; }
		else { *cycles_out += 12; }
		break;
	}

	case 0xDC: {
		uint16_t addr = read8(gb, c->pc + 1) << 8 | read8(gb, c->pc);
		c->pc += 2;
		if ((c->af.reg_f & FLAG_C) != 0) {
			c->sp -= 2;
			write8(gb, c->sp, c->pc & 0xFF);
			write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
			c->pc = addr;
		}
		if ((c->af.reg_f & FLAG_C) != 0) *cycles_out += 24; else *cycles_out += 12;
		break;
	}

	case 0xDE: {
		uint8_t carry = (c->af.reg_f & FLAG_C) ? 1 : 0;
		uint8_t* A = &c->af.reg_a;
		uint8_t val = read8(gb, c->pc);
		c->pc++;
		uint16_t res = *A - val - carry;

		check_z_flag(c, (uint8_t)res);
		c->af.reg_f |= FLAG_N;

		if ((*A & 0x0F) < (val & 0x0F) + carry) c->af.reg_f |= FLAG_H;
		else c->af.reg_f &= ~FLAG_H;

		if (*A < val + carry) c->af.reg_f |= FLAG_C;
		else c->af.reg_f &= ~FLAG_C;

		*A = (uint8_t)res;
		*cycles_out += 8;
		break;
	}

	case 0xDF:
		c->sp -= 2;
		write8(gb, c->sp, c->pc & 0xFF);
		write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
		c->pc = 0x0018;
		*cycles_out += 16;
		break;

	case 0xE0: {
		uint8_t a8 = read8(gb, c->pc);
		c->pc++;
		write8(gb, 0xFF00 + a8, c->af.reg_a);
		*cycles_out += 12;
		break;
	}

	case 0xE1: {
		uint16_t addr = read8(gb, c->sp + 1) << 8 | read8(gb, c->sp);
		c->sp += 2;
		c->hl.reg_hl = addr;
		*cycles_out += 12;
		break;
	}

	case 0xE2:
		write8(gb, 0xFF00 + c->bc.reg_c, c->af.reg_a);
		*cycles_out += 8;
		break;

	case 0xE5:
		c->sp -= 2;
		write8(gb, c->sp, c->hl.reg_hl & 0xFF);
		write8(gb, c->sp + 1, (c->hl.reg_hl >> 8) & 0xFF);
		*cycles_out += 16;
		break;

	case 0xE6: {
		uint8_t val = read8(gb, c->pc);
		c->pc++;
		uint8_t res = c->af.reg_a & val;
		c->af.reg_a = res;
		check_z_flag(c, res);
		c->af.reg_f &= ~FLAG_N;
		c->af.reg_f &= ~FLAG_C;
		c->af.reg_f |= FLAG_H;
		*cycles_out += 8;
		break;
	}

	case 0xE7:
		c->sp -= 2;
		write8(gb, c->sp, c->pc & 0xFF);
		write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
		c->pc = 0x0020;
		*cycles_out += 16;
		break;

	case 0xE8: {
		int8_t r8 = (int8_t)read8(gb, c->pc);
		c->pc++;
		uint8_t val = (uint8_t)r8;

		if (((c->sp & 0x0F) + (val & 0x0F)) > 0x0F) c->af.reg_f |= FLAG_H;
		else c->af.reg_f &= ~FLAG_H;

		if (((c->sp & 0xFF) + val) > 0xFF) c->af.reg_f |= FLAG_C;
		else c->af.reg_f &= ~FLAG_C;

		c->sp = (uint16_t)(c->sp + r8);
		c->af.reg_f &= ~FLAG_Z;
		c->af.reg_f &= ~FLAG_N;
		*cycles_out += 16;
		break;
	}

	case 0xE9:
		c->pc = c->hl.reg_hl;
		*cycles_out += 4;
		break;

	case 0xEA: {
		uint16_t addr = read8(gb, c->pc) | (read8(gb, c->pc + 1) << 8);
		c->pc += 2;
		write8(gb, addr, c->af.reg_a);
		*cycles_out += 16;
		break;
	}

	case 0xEE: {
		uint8_t val = read8(gb, c->pc);
		c->pc++;
		uint8_t res = c->af.reg_a ^ val;
		c->af.reg_a = res;
		check_z_flag(c, res);
		c->af.reg_f &= ~FLAG_N;
		c->af.reg_f &= ~FLAG_C;
		c->af.reg_f &= ~FLAG_H;
		*cycles_out += 8;
		break;
	}

	case 0xEF:
		c->sp -= 2;
		write8(gb, c->sp, c->pc & 0xFF);
		write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
		c->pc = 0x0028;
		*cycles_out += 16;
		break;

	case 0xF0: {
		uint8_t a8 = read8(gb, c->pc);
		c->pc++;
		c->af.reg_a = read8(gb, 0xFF00 + a8);
		*cycles_out += 12;
		break;
	}

	case 0xF1: {
		uint16_t addr = read8(gb, c->sp + 1) << 8 | read8(gb, c->sp);
		c->sp += 2;
		c->af.reg_af = addr & 0xFFF0;
		*cycles_out += 12;
		break;
	}

	case 0xF2:
		c->af.reg_a = read8(gb, 0xFF00 + c->bc.reg_c);
		*cycles_out += 8;
		break;

	case 0xF3:
		c->ime = 0;
		*cycles_out += 4;
		break;

	case 0xF5:
		c->sp -= 2;
		write8(gb, c->sp, c->af.reg_af & 0xFF);
		write8(gb, c->sp + 1, (c->af.reg_af >> 8) & 0xFF);
		*cycles_out += 16;
		break;

	case 0xF6: {
		uint8_t val = read8(gb, c->pc);
		c->pc++;
		uint8_t res = c->af.reg_a | val;
		c->af.reg_a = res;
		check_z_flag(c, res);
		c->af.reg_f &= ~FLAG_N;
		c->af.reg_f &= ~FLAG_C;
		c->af.reg_f &= ~FLAG_H;
		*cycles_out += 8;
		break;
	}

	case 0xF7:
		c->sp -= 2;
		write8(gb, c->sp, c->pc & 0xFF);
		write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
		c->pc = 0x0030;
		*cycles_out += 16;
		break;

	case 0xF8: {
		int8_t r8 = (int8_t)read8(gb, c->pc);
		c->pc++;
		uint8_t val = (uint8_t)r8;

		if (((c->sp & 0x0F) + (val & 0x0F)) > 0x0F) c->af.reg_f |= FLAG_H;
		else c->af.reg_f &= ~FLAG_H;

		if (((c->sp & 0xFF) + val) > 0xFF) c->af.reg_f |= FLAG_C;
		else c->af.reg_f &= ~FLAG_C;

		c->hl.reg_hl = (uint16_t)(c->sp + r8);
		c->af.reg_f &= ~FLAG_Z;
		c->af.reg_f &= ~FLAG_N;
		*cycles_out += 12;
		break;
	}

	case 0xF9:
		c->sp = c->hl.reg_hl;
		*cycles_out += 8;
		break;

	case 0xFA: {
		uint16_t addr = read8(gb, c->pc) | (read8(gb, c->pc + 1) << 8);
		c->pc += 2;
		c->af.reg_a = read8(gb, addr);
		*cycles_out += 16;
		break;
	}

	case 0xFB:
		c->ei_scheduled = 1;
		*cycles_out += 4;
		break;

	case 0xFE: {
		uint8_t val = read8(gb, c->pc);
		c->pc++;

		uint16_t res = c->af.reg_a - val;

		check_z_flag(c, (uint8_t)res);
		c->af.reg_f |= FLAG_N;

		if ((c->af.reg_a & 0x0F) < (val & 0x0F)) c->af.reg_f |= FLAG_H;
		else c->af.reg_f &= ~FLAG_H;

		if (c->af.reg_a < val) c->af.reg_f |= FLAG_C;
		else c->af.reg_f &= ~FLAG_C;

		*cycles_out += 8;
		break;
	}

	case 0xFF:
		c->sp -= 2;
		write8(gb, c->sp, c->pc & 0xFF);
		write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);
		c->pc = 0x0038;
		*cycles_out += 16;
		break;

	default:
		return 1;
	}
	return 0;
}

int load_rom(gameBoy* gb, const char* path) {
	FILE* file = fopen(path, "rb");
	if (!file) {
		printf("ROM READ ERROR\n");
		return 1;
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	printf("ROM SIZE: %ld bytes\n", size);

	gb->rom_data = malloc(size);
	if (!gb->rom_data) {
		printf("ERROR: memory allocation failed\n");
		fclose(file);
		return 1;
	}

	fread(gb->rom_data, 1, size, file);
	fclose(file);

	gb->rom_size = size;
	gb->current_rom_bank = 1;
	gb->cart_type = gb->rom_data[0x0147];
	printf("Cartridge Type: 0x%02X\n", gb->cart_type);

	// copy the first bank (16KB) into the fixed area 0x0000-0x3FFF
	memcpy(gb->memory, gb->rom_data, 0x4000);

	// --- NEW PART: NAME MANAGEMENT AND LOADING .SAV FILE ---
	strncpy(gb->save_path, path, sizeof(gb->save_path) - 1);
	char* dot = strrchr(gb->save_path, '.');
	if (dot) {
		strcpy(dot, ".sav"); // Replaces the extension
	}
	else {
		strcat(gb->save_path, ".sav");
	}

	FILE* sav_file = fopen(gb->save_path, "rb");
	if (sav_file) {
		fread(gb->RAM, 1, sizeof(gb->RAM), sav_file);
		fclose(sav_file);
		printf("Previous save loaded successfully!\n");
	}
	else {
		printf("No save found. Starting from scratch.\n");
	}
	// -----------------------------------------------------------

	return 0;
}

void update_cycles(gameBoy* gb, int cycles)
{
	cpu* c = &gb->cpu;
	uint8_t* m = gb->memory;

	//DIV
	c->div_counter += cycles;
	//write8(gb, 0xFF04, (c->div_counter >> 8) & 0xFF);


	//TIMA
	if ((read8(gb, 0xFF07) >> 2) & 1) {
		c->tima_counter += cycles;

		uint8_t tima_freq = read8(gb, 0xFF07) & 0x03;
		uint16_t threshold;

		switch (tima_freq) {
		case 0x00: threshold = 1024; break;
		case 0x01: threshold = 16;   break;
		case 0x02: threshold = 64;   break;
		case 0x03: threshold = 256;  break;
		}

			while (c->tima_counter >= threshold) {
			c->tima_counter -= threshold;

			write8(gb, 0xFF05, read8(gb, 0xFF05) + 1);   // increment TIMA

			if (read8(gb, 0xFF05) == 0) {          // overflow!
				write8(gb, 0xFF05, read8(gb, 0xFF06));     // reload from TMA
				write8(gb, 0xFF0F, read8(gb, 0xFF0F) | (1 << 2));    // turn on Timer bit in IF
			}
		}
	}
}

int handle_interrupts(gameBoy* gb) {
	cpu* c = &gb->cpu;

	uint8_t pending = read8(gb, 0xFF0F) & read8(gb, 0xFFFF);

	// Any pending interrupt unlocks the CPU from HALT
	if (pending != 0) {
		c->halted = 0;
	}

	// If IME is 0 or there are no interrupts, stop here
	if (c->ime == 0 || pending == 0) return 0;

	for (int i = 0; i < 5; i++) {
		if (pending & (1 << i)) {
			c->ime = 0;
			write8(gb, 0xFF0F, read8(gb, 0xFF0F) & ~(1 << i));

			c->sp -= 2;
			write8(gb, c->sp, c->pc & 0xFF);
			write8(gb, c->sp + 1, (c->pc >> 8) & 0xFF);

			c->pc = 0x0040 + (i * 8);

			// Dispatching an interrupt takes 20 hardware cycles
			return 20;
		}
	}
	return 0;
}

uint8_t read8(gameBoy* gb, uint16_t addr) {
	if (addr >= 0x4000 && addr <= 0x7FFF) {
		uint32_t bank = gb->current_rom_bank % (gb->rom_size / 0x4000);
		return gb->rom_data[bank * 0x4000 + (addr - 0x4000)];
	}

	// EXTERNAL RAM READ MANAGEMENT (SRAM / RTC)
	if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (gb->enabledRam == 0) {
			return 0xFF;
		}

		if (gb->cart_type == 0x01 || gb->cart_type == 0x02 || gb->cart_type == 0x03) {
			// MBC1
			uint8_t ram_bank = (gb->MBC1 == 1) ? gb->currentRam : 0;
			uint16_t ram_offset = (ram_bank * 0x2000) + (addr - 0xA000);
			return gb->RAM[ram_offset];
		}
		else if (gb->cart_type >= 0x0F && gb->cart_type <= 0x13) {
			// MBC3
			if (gb->currentRam <= 0x03) {
				uint16_t ram_offset = (gb->currentRam * 0x2000) + (addr - 0xA000);
				return gb->RAM[ram_offset];
			}
			else if (gb->currentRam >= 0x08 && gb->currentRam <= 0x0C) {
				return gb->rtc_registers[gb->currentRam - 0x08];
			}
		}
		else if (gb->cart_type >= 0x19 && gb->cart_type <= 0x1E) {
			// MBC5
			uint16_t ram_offset = (gb->currentRam * 0x2000) + (addr - 0xA000);
			return gb->RAM[ram_offset];
		}
		return 0xFF;
	}

	// DIV: byte alto del contatore a 16 bit
	if (addr == 0xFF04) {
		return (gb->cpu.div_counter >> 8) & 0xFF;
	}

	// JOYPAD
	if (addr == 0xFF00) {
		uint8_t sel = gb->memory[0xFF00] & 0x30;
		uint8_t low = 0x0F;

		if (!(sel & 0x10)) low &= gb->cross_key;
		if (!(sel & 0x20)) low &= gb->buttons;

		return 0xC0 | sel | low;
	}
	if (addr >= 0xFF10 && addr <= 0xFF3F) {
		return apu_read(gb, addr);
	}
	return gb->memory[addr];
}


void mbc1_write(gameBoy* gb, uint16_t addr, uint8_t value) {
	if (addr >= 0x000 && addr <= 0x1FFF) {
		if ((value & 0x0F) == 0x0A) {
			gb->enabledRam = 1;
		}
		else {
			if (gb->enabledRam == 1) {
				save_ram(gb);
			}
			gb->enabledRam = 0;
		}
		return;
	}
	if (addr >= 0x2000 && addr <= 0x3FFF) {
		uint8_t lower5 = value & 0x1F;
		if (lower5 == 0x00) {
			lower5 = 1;
		}
		gb->current_rom_bank = (gb->current_rom_bank & 0xE0) | lower5;
		return;
	}
	if (addr >= 0x4000 && addr <= 0x5FFF) {
		uint8_t lower2 = value & 0x03;
		if (gb->MBC1 == 0) {
			gb->current_rom_bank = (gb->current_rom_bank & 0x1F) | (lower2 << 5);
		}
		else {
			gb->currentRam = lower2;
		}
		return;
	}
	if (addr >= 0x6000 && addr <= 0x7FFF) {
		gb->MBC1 = value & 0x01;
		return;
	}
}

void mbc3_write(gameBoy* gb, uint16_t addr, uint8_t value) {
	if (addr >= 0x0000 && addr <= 0x1FFF) {
		// Abilita RAM e RTC
		if ((value & 0x0F) == 0x0A) {
			gb->enabledRam = 1;
		}
		else {
			if (gb->enabledRam == 1) {
				save_ram(gb);
			}
			gb->enabledRam = 0;
		}
		return;
	}
	if (addr >= 0x2000 && addr <= 0x3FFF) {
		// Selezione Banco ROM (7 bit interi, ignora il bit 7)
		uint8_t bank = value & 0x7F;
		if (bank == 0) {
			bank = 1; // Il banco 0 è mappato a 1, come su MBC1
		}
		gb->current_rom_bank = bank;
		return;
	}
	if (addr >= 0x4000 && addr <= 0x5FFF) {
		// Selezione Banco RAM (0x00-0x03) o Registro RTC (0x08-0x0C)
		gb->currentRam = value;
		return;
	}
	if (addr >= 0x6000 && addr <= 0x7FFF) {
		// Latch dei dati dell'orologio.
		// Per ora non lo implementiamo, serve solo al vero RTC hardware.
		return;
	}
}

void mbc5_write(gameBoy* gb, uint16_t addr, uint8_t value) {
	if (addr >= 0x0000 && addr <= 0x1FFF) {
		// Abilita la RAM
		if ((value & 0x0F) == 0x0A) {
			gb->enabledRam = 1;
		}
		else {
			if (gb->enabledRam == 1) {
				save_ram(gb);
			}
			gb->enabledRam = 0;
		}
		return;
	}
	if (addr >= 0x2000 && addr <= 0x2FFF) {
		// ROM Bank (8 bit inferiori)
		gb->current_rom_bank = (gb->current_rom_bank & 0x0100) | value;
		return;
	}
	if (addr >= 0x3000 && addr <= 0x3FFF) {
		// ROM Bank (9° bit)
		gb->current_rom_bank = (gb->current_rom_bank & 0x00FF) | ((value & 1) << 8);
		return;
	}
	if (addr >= 0x4000 && addr <= 0x5FFF) {
		// Seleziona il banco RAM (l'MBC5 supporta fino a 16 banchi, quindi la maschera è 0x0F)
		gb->currentRam = value & 0x0F;
		return;
	}
}

void write8(gameBoy* gb, uint16_t addr, uint8_t value) {
	// 1. GESTIONE BANKING ROM
	if (addr < 0x8000) {
		switch (gb->cart_type) {
		case 0x01: case 0x02: case 0x03:
			mbc1_write(gb, addr, value);
			break;
		case 0x0F: case 0x10: case 0x11: case 0x12: case 0x13:
			mbc3_write(gb, addr, value);
			break;
		case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E:
			mbc5_write(gb, addr, value);
			break;
		default:
			break;
		}
		return;
	}

	// 2. GESTIONE SCRITTURA RAM ESTERNA (SRAM / RTC)
	if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (gb->enabledRam == 0) {
			return;
		}

		if (gb->cart_type == 0x01 || gb->cart_type == 0x02 || gb->cart_type == 0x03) {
			// MBC1
			uint8_t ram_bank = (gb->MBC1 == 1) ? gb->currentRam : 0;
			uint16_t ram_offset = (ram_bank * 0x2000) + (addr - 0xA000);
			gb->RAM[ram_offset] = value;
		}
		else if (gb->cart_type >= 0x0F && gb->cart_type <= 0x13) {
			// MBC3
			if (gb->currentRam <= 0x03) {
				uint16_t ram_offset = (gb->currentRam * 0x2000) + (addr - 0xA000);
				gb->RAM[ram_offset] = value;
			}
			else if (gb->currentRam >= 0x08 && gb->currentRam <= 0x0C) {
				gb->rtc_registers[gb->currentRam - 0x08] = value;
			}
		}
		else if (gb->cart_type >= 0x19 && gb->cart_type <= 0x1E) {
			// MBC5
			uint16_t ram_offset = (gb->currentRam * 0x2000) + (addr - 0xA000);
			gb->RAM[ram_offset] = value;
		}
		return;
	}
	// APU WRITE MANAGEMENT
	if (addr >= 0xFF10 && addr <= 0xFF3F) {
		apu_write(gb, addr, value);
		return;
	}
	// 3. I/O AND PPU MANAGEMENT
	if (addr == 0xFF00) { gb->memory[addr] = value & 0x30; return; }
	if (addr == 0xFF04) { gb->memory[addr] = 0; gb->cpu.div_counter = 0; return; }
	if (addr == 0xFF44) { gb->memory[addr] = 0; return; }
	if (addr == 0xFF45) { gb->memory[addr] = value; check_LYC(gb); return; }
	if (addr == 0xFF46) {
		uint16_t source = value << 8;
		for (int i = 0; i < 160; i++) {
			write8(gb, 0xFE00 + i, read8(gb, source + i));
		}
		gb->memory[addr] = value;
		return;
	}

	gb->memory[addr] = value;
}

void save_ram(gameBoy* gb) {
	FILE* f = fopen(gb->save_path, "wb");
	if (f) {
		fwrite(gb->RAM, 1, sizeof(gb->RAM), f);
		fclose(f);
		printf("Game saved to: %s\n", gb->save_path);
	}
	else {
		printf("ERROR: Unable to create save file!\n");
	}
}