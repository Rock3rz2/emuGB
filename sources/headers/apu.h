//
// Created by Nicola on 08/10/2026.
//
#pragma once

#ifndef EMUGB2_APU_H
#define EMUGB2_APU_H

#endif //EMUGB2_APU_H

//
// Created by Nicola on 08/10/2026.
//
#include "apu.h"

#include <stdint.h>

typedef struct gameBoy gameBoy;

typedef struct {
    int audio_cycles;
    float sampling_timer;
    float buffer[2048];
    uint16_t buffer_index;
    int frame_sequencer_timer;
    uint8_t frame_sequencer_step;

    //canale1
    uint16_t ch1_frequency;
    uint8_t ch1_wave_step;
    int ch1_timer;
    int ch1_length_timer;
    uint8_t ch1_length_enable;
    uint8_t ch1_channel_enabled;
    uint8_t ch1_current_volume;
    uint8_t ch1_env_period;
    uint8_t ch1_env_timer;
    int ch1_env_dir;

    // Variabili esclusive dello Sweep
    uint8_t ch1_sweep_period;
    uint8_t ch1_sweep_timer;
    uint8_t ch1_sweep_dir;
    uint8_t ch1_sweep_shift;
    uint8_t ch1_sweep_enabled;

    //canale 2
    uint16_t ch2_frequency;
    uint8_t ch2_wave_step;
    int ch2_timer;
    int ch2_length_timer;
    uint8_t ch2_length_enable;
    uint8_t ch2_channel_enabled;
    uint8_t ch2_current_volume;
    uint8_t ch2_env_period;
    uint8_t ch2_env_timer;
    int ch2_env_dir;

    // --- CANALE 3 (Wave RAM) ---
    uint16_t ch3_frequency;
    uint8_t ch3_wave_step;        // Conta da 0 a 31 (i campioni della Wave RAM)
    int ch3_timer;
    int ch3_length_timer;         // Attenzione: questo parte da 256, non 64!
    uint8_t ch3_length_enable;
    uint8_t ch3_channel_enabled;
    uint8_t ch3_volume;

    // --- CANALE 4 (Rumore Bianco) ---
    int ch4_timer;
    int ch4_length_timer;
    uint8_t ch4_length_enable;
    uint8_t ch4_channel_enabled;
    uint8_t ch4_current_volume;
    uint8_t ch4_env_period;
    uint8_t ch4_env_timer;
    int ch4_env_dir;

    // Variabili esclusive per il rumore
    uint16_t ch4_lfsr;        // Il registro a scorrimento (15 bit)
    uint8_t ch4_clock_shift;  // Regola la frequenza base
    uint8_t ch4_width_mode;   // 0 = 15 bit, 1 = 7 bit (suono più metallico)
    uint8_t ch4_divisor_code; // Regola il moltiplicatore della frequenza
} apu;

void apu_init(gameBoy* gb);
void update_apu(gameBoy* gb, int cycles);
uint8_t apu_read(gameBoy* gb, uint16_t addr);
void apu_write(gameBoy* gb, uint16_t addr, uint8_t value);
uint8_t get_ch2_wave_form(gameBoy* gb);

float get_channel2_sample(gameBoy* gb);
uint8_t get_ch1_wave_form(gameBoy* gb);
float get_channel1_sample(gameBoy* gb);
float get_channel3_sample(gameBoy* gb);

float get_channel4_sample(gameBoy* gb);