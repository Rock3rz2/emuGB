#include "cpu.h"

void cpu_init(cpu* c)
{
	c->af.reg_af = 0x01B0;
	c->bc.reg_bc = 0x0013;
	c->de.reg_de = 0x00D8;
	c->hl.reg_hl = 0x014D;
	c->sp = 0xFFFE;
	c->pc = 0x0100;

	c->ime = 0;
	c->div_counter = 0;
	c->tima_counter = 0;
	c->ei_scheduled = 0;
}

