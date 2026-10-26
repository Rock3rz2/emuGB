#include "helper.h"

void inc_r8(cpu* c, uint8_t* reg)
{
	set_flag_h_inc(c, reg);
	(*reg)++;
	set_flag_z(c, reg);
	set_flag_to(c, FLAG_N, 0);
}

void dec_r8(cpu* c, uint8_t* reg)
{
	set_flag_h_dec(c, reg);
	(*reg)--;
	set_flag_z(c, reg);
	set_flag_to(c, FLAG_N, 1);
}

void rst_addr(cpu* c, uint8_t* m, uint16_t addr)
{
	c->sp -= 2;
	m[c->sp] = c->pc & 0xFF;
	m[c->sp + 1] = (c->pc >> 8) & 0xFF;
	c->pc = addr;
}

void set_flag_z(cpu* c, uint8_t* reg)
{
	if (*reg == 0) {
		c->af.reg_f |= FLAG_Z;
	}
	else {
		c->af.reg_f &= ~FLAG_Z;
	}
}

void set_flag_h_inc(cpu* c, uint8_t* reg)
{
	if ((*reg & 0x0f) == 0x0f) {
		c->af.reg_f |= FLAG_H;
	}
	else {
		c->af.reg_f &= ~FLAG_H;
	}
}

void set_flag_h_dec(cpu* c, uint8_t* reg)
{
	if ((*reg & 0x0f) == 0x00) {
		c->af.reg_f |= FLAG_H;
	}
	else {
		c->af.reg_f &= ~FLAG_H;
	}
}

void set_flag_to(cpu* c, uint8_t FLAG, uint8_t value)
{
	if (value == 1) {
		c->af.reg_f |= FLAG;
	}
	else {
		c->af.reg_f &= ~FLAG;
	}
}

void check_z_flag(cpu* c, uint8_t reg)
{
	if (reg == 0) {
		c->af.reg_f |= FLAG_Z;
	}
	else {
		c->af.reg_f &= ~FLAG_Z;
	}
}

uint8_t* get_r8(gameBoy* gb, uint8_t idx)
{
	switch (idx) {
	case 0: return &gb->cpu.bc.reg_b;
	case 1: return &gb->cpu.bc.reg_c;
	case 2: return &gb->cpu.de.reg_d;
	case 3: return &gb->cpu.de.reg_e;
	case 4: return &gb->cpu.hl.reg_h;
	case 5: return &gb->cpu.hl.reg_l;
	case 6: return &gb->memory[gb->cpu.hl.reg_hl];
	case 7: return &gb->cpu.af.reg_a;
	default: return NULL;
	}
}

void ALU(gameBoy* gb, uint8_t opcode)
{
	uint8_t op = (opcode >> 3) & 0x07;
	uint8_t* A = &gb->cpu.af.reg_a;
	uint8_t idx = opcode & 7;
	uint8_t val = (idx == 6) ? read8(gb, gb->cpu.hl.reg_hl) : *get_r8(gb, idx);

	switch (op) {
	case 0: {
		// ADD
		uint16_t res = *A + val;

		check_z_flag(&gb->cpu, (uint8_t)res);
		gb->cpu.af.reg_f &= ~FLAG_N;

		if ((*A & 0x0F) + (val & 0x0F) > 0x0F) {
			gb->cpu.af.reg_f |= FLAG_H;
		}
		else {
			gb->cpu.af.reg_f &= ~FLAG_H;
		}

		if (res > 0xFF) {
			gb->cpu.af.reg_f |= FLAG_C;
		}
		else {
			gb->cpu.af.reg_f &= ~FLAG_C;
		}

		*A = (uint8_t)res;
		break;
	}

	case 1: {
		// ADC
		uint8_t carry = (gb->cpu.af.reg_f & FLAG_C) ? 1 : 0;
		uint16_t res = *A + val + carry;

		check_z_flag(&gb->cpu, (uint8_t)res);
		gb->cpu.af.reg_f &= ~FLAG_N;

		if ((*A & 0x0F) + (val & 0x0F) + carry > 0x0F) {
			gb->cpu.af.reg_f |= FLAG_H;
		}
		else {
			gb->cpu.af.reg_f &= ~FLAG_H;
		}

		if (res > 0xFF) {
			gb->cpu.af.reg_f |= FLAG_C;
		}
		else {
			gb->cpu.af.reg_f &= ~FLAG_C;
		}

		*A = (uint8_t)res;
		break;
	}

	case 2: {
		// SUB
		uint16_t res = *A - val;

		check_z_flag(&gb->cpu, (uint8_t)res);
		gb->cpu.af.reg_f |= FLAG_N;

		if ((*A & 0x0F) < (val & 0x0F)) {
			gb->cpu.af.reg_f |= FLAG_H;
		}
		else {
			gb->cpu.af.reg_f &= ~FLAG_H;
		}

		if (*A < val) {
			gb->cpu.af.reg_f |= FLAG_C;
		}
		else {
			gb->cpu.af.reg_f &= ~FLAG_C;
		}

		*A = (uint8_t)res;
		break;
	}

	case 3: {
		// SBC
		uint8_t carry = (gb->cpu.af.reg_f & FLAG_C) ? 1 : 0;
		uint16_t res = *A - val - carry;

		check_z_flag(&gb->cpu, (uint8_t)res);
		gb->cpu.af.reg_f |= FLAG_N;

		if ((*A & 0x0F) < (val & 0x0F) + carry) {
			gb->cpu.af.reg_f |= FLAG_H;
		}
		else {
			gb->cpu.af.reg_f &= ~FLAG_H;
		}

		if (*A < val + carry) {
			gb->cpu.af.reg_f |= FLAG_C;
		}
		else {
			gb->cpu.af.reg_f &= ~FLAG_C;
		}

		*A = (uint8_t)res;
		break;
	}

	case 4:
		// AND

	{

		uint8_t res = *A & val;
		*A = res;
		check_z_flag(&gb->cpu, res);
		gb->cpu.af.reg_f &= ~FLAG_N;
		gb->cpu.af.reg_f &= ~FLAG_C;
		gb->cpu.af.reg_f |= FLAG_H;
		break;
	}

	case 5:
		// XOR
	{
		uint8_t res = *A ^ val;
		*A = res;
		check_z_flag(&gb->cpu, res);
		gb->cpu.af.reg_f &= ~FLAG_N;
		gb->cpu.af.reg_f &= ~FLAG_C;
		gb->cpu.af.reg_f &= ~FLAG_H;
		break;
	}

	case 6:
		// OR
	{
		uint8_t res = *A | val;
		*A = res;
		check_z_flag(&gb->cpu, res);
		gb->cpu.af.reg_f &= ~FLAG_N;
		gb->cpu.af.reg_f &= ~FLAG_C;
		gb->cpu.af.reg_f &= ~FLAG_H;
		break;
	}

	case 7:
		// CP
	{
		// SUB
		uint16_t res = *A - val;

		check_z_flag(&gb->cpu, (uint8_t)res);
		gb->cpu.af.reg_f |= FLAG_N;

		if ((*A & 0x0F) < (val & 0x0F)) {
			gb->cpu.af.reg_f |= FLAG_H;
		}
		else {
			gb->cpu.af.reg_f &= ~FLAG_H;
		}

		if (*A < val) {
			gb->cpu.af.reg_f |= FLAG_C;
		}
		else {
			gb->cpu.af.reg_f &= ~FLAG_C;
		}

		break;
	}
	}
}

void cb_step(gameBoy* gb, uint8_t cb)
{
	uint8_t is_hl = (cb & 7) == 6;
	uint8_t temp_val = 0;
	uint8_t* r;

	if (is_hl) {
		temp_val = read8(gb, gb->cpu.hl.reg_hl);
		r = &temp_val;
	}
	else {
		r = get_r8(gb, cb & 7);
	}

	uint8_t  y = (cb >> 3) & 7;
	uint8_t  grp = cb >> 6;              // gruppo

	switch (grp) {
	case 0: // rotazioni/shift, l'operazione è y
		switch (y) {
		case 0: /* RLC  */ 
		{
			uint8_t v = *r;
			uint8_t out = (v >> 7) & 1;
			*r = (v << 1) | out;
			check_z_flag(&gb->cpu, *r);
			set_flag_to(&gb->cpu, FLAG_N, 0);
			set_flag_to(&gb->cpu, FLAG_H, 0);
			set_flag_to(&gb->cpu, FLAG_C, out);
			break;
		}
		case 1: /* RRC  */ 
		{
			uint8_t v = *r;
			uint8_t out = v & 1;
			*r = (v >> 1) | (out << 7);
			check_z_flag(&gb->cpu, *r);
			set_flag_to(&gb->cpu, FLAG_N, 0);
			set_flag_to(&gb->cpu, FLAG_H, 0);
			set_flag_to(&gb->cpu, FLAG_C, out);
			break;
		}
		case 2: /* RL   */ 
		{
			uint8_t v = *r;
			uint8_t old_carry = (gb->cpu.af.reg_f & FLAG_C) ? 1 : 0;
			uint8_t out = (v >> 7) & 1;
			*r = (v << 1) | old_carry;
			check_z_flag(&gb->cpu, *r);
			set_flag_to(&gb->cpu, FLAG_N, 0);
			set_flag_to(&gb->cpu, FLAG_H, 0);
			set_flag_to(&gb->cpu, FLAG_C, out);
			break;
		}
		case 3: /* RR   */ 
		{
			uint8_t v = *r;
			uint8_t old_carry = (gb->cpu.af.reg_f & FLAG_C) ? 1 : 0;
			uint8_t out = v & 1;
			*r = (v >> 1) | (old_carry << 7);
			check_z_flag(&gb->cpu, *r);
			set_flag_to(&gb->cpu, FLAG_N, 0);
			set_flag_to(&gb->cpu, FLAG_H, 0);
			set_flag_to(&gb->cpu, FLAG_C, out);
			break;
		}
		case 4: /* SLA  */ 
		{
			uint8_t v = *r;
			uint8_t out = (v >> 7) & 1;
			set_flag_to(&gb->cpu, FLAG_C, out);
			*r = v << 1;
			check_z_flag(&gb->cpu, *r);
			set_flag_to(&gb->cpu, FLAG_N, 0);
			set_flag_to(&gb->cpu, FLAG_H, 0);
			break;
		}
		case 5: /* SRA  */ 
		{
			uint8_t v = *r;
			uint8_t out = v & 1;
			*r = (v >> 1) | (v & 0x80);
			check_z_flag(&gb->cpu, *r);
			set_flag_to(&gb->cpu, FLAG_N, 0);
			set_flag_to(&gb->cpu, FLAG_H, 0);
			set_flag_to(&gb->cpu, FLAG_C, out);
			break;
		}
		case 6: /* SWAP */ 
		{
			uint8_t v = *r;
			*r = ((v & 0x0F) << 4) | ((v & 0xF0) >> 4);
			check_z_flag(&gb->cpu, *r);
			set_flag_to(&gb->cpu, FLAG_N, 0);
			set_flag_to(&gb->cpu, FLAG_H, 0);
			set_flag_to(&gb->cpu, FLAG_C, 0);
			break;
		}
		case 7: /* SRL  */ 
		{
			uint8_t v = *r;
			uint8_t out = v & 1;
			set_flag_to(&gb->cpu, FLAG_C, out);
			*r = v >> 1;
			check_z_flag(&gb->cpu, *r);
			set_flag_to(&gb->cpu, FLAG_N, 0);
			set_flag_to(&gb->cpu, FLAG_H, 0);
			break;
		}
		}
		break;
	case 1: /* BIT y, r */ 
	{
		uint8_t bit = (*r >> y) & 1;
		check_z_flag(&gb->cpu, bit);   // Z=1 se bit==0
		set_flag_to(&gb->cpu, FLAG_N, 0);
		set_flag_to(&gb->cpu, FLAG_H, 1);
		break;
	}
	case 2: /* RES y, r */ 
		*r &= ~(1 << y);
		break;
	case 3: /* SET y, r */ 
		*r |= (1 << y);
		break;
	}
	if (is_hl && grp != 1) {
		write8(gb, gb->cpu.hl.reg_hl, *r);
	}
}

uint16_t* get_r16(gameBoy* gb, uint8_t idx)
{
	switch (idx) {
	case 0: return &gb->cpu.bc.reg_bc;
	case 1: return &gb->cpu.de.reg_de;
	case 2: return &gb->cpu.hl.reg_hl;
	case 3: return &gb->cpu.sp;
	default: return NULL;
	}
}