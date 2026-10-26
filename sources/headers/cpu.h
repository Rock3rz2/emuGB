#pragma once

#include <stdint.h>

typedef struct {
	union {
		struct {
			uint8_t reg_f;   // byte basso
			uint8_t reg_a;   // byte alto
		};
		uint16_t reg_af;
	} af;

	union bc {
		struct {
			uint8_t reg_c;
			uint8_t reg_b;
		};
		uint16_t reg_bc;
	} bc;

	union de {
		struct {
			uint8_t reg_e;
			uint8_t reg_d;
		};
		uint16_t reg_de;
	} de;

	union hl {
		struct {
			uint8_t reg_l;
			uint8_t reg_h;
		};
		uint16_t reg_hl;
	} hl;

	uint16_t pc;
	uint16_t sp;

	uint8_t ime;

	uint16_t div_counter;
	uint16_t tima_counter;
	uint8_t  ei_scheduled;

	uint8_t halted;

} cpu;

// inizializza la struct cpu con i valori di default
void cpu_init(cpu* c);