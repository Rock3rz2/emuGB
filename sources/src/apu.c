#include "apu.h"
#include <stdio.h>
#include "gameBoy.h"

void apu_init(gameBoy* gb) {
    gb->apu.audio_cycles = 0;
    gb->apu.frame_sequencer_timer = 8192;
    gb->apu.frame_sequencer_step = 0;
}

void update_apu(gameBoy* gb, int cycles) {
    gb->apu.audio_cycles += cycles;

    // --- Aggiornamento Onda Canale 1 ---
    gb->apu.ch1_timer -= cycles;
    while (gb->apu.ch1_timer <= 0) {
        gb->apu.ch1_timer += (2048 - gb->apu.ch1_frequency) * 4;
        gb->apu.ch1_wave_step = (gb->apu.ch1_wave_step + 1) % 8;
    }

    // --- Aggiornamento Onda Canale 2 ---
    gb->apu.ch2_timer -= cycles;
    while (gb->apu.ch2_timer <= 0) {
        gb->apu.ch2_timer += (2048 - gb->apu.ch2_frequency) * 4;
        gb->apu.ch2_wave_step = (gb->apu.ch2_wave_step + 1) % 8;
    }

    // --- Aggiornamento Onda Canale 3 ---
    gb->apu.ch3_timer -= cycles;
    while (gb->apu.ch3_timer <= 0) {
        gb->apu.ch3_timer += (2048 - gb->apu.ch3_frequency) * 2;
        gb->apu.ch3_wave_step = (gb->apu.ch3_wave_step + 1) % 32;
    }

    // --- Aggiornamento Onda Canale 4 (LFSR Rumore) ---
    gb->apu.ch4_timer -= cycles;
    while (gb->apu.ch4_timer <= 0) {
        const int divisor_table[8] = {8, 16, 32, 48, 64, 80, 96, 112};
        int divisor = divisor_table[gb->apu.ch4_divisor_code];
        gb->apu.ch4_timer += divisor << gb->apu.ch4_clock_shift;

        // XOR tra il bit 0 e il bit 1 dell'LFSR
        uint8_t xor_bit = (gb->apu.ch4_lfsr & 0x01) ^ ((gb->apu.ch4_lfsr & 0x02) >> 1);

        // Scorriamo tutto a destra di 1
        gb->apu.ch4_lfsr >>= 1;

        // Inseriamo il risultato dello XOR al bit 14
        gb->apu.ch4_lfsr |= (xor_bit << 14);

        // Se la modalità è "corta" (7 bit, suono più metallico), inseriamo lo XOR anche al bit 6
        if (gb->apu.ch4_width_mode) {
            gb->apu.ch4_lfsr &= ~(1 << 6);      // Prima azzeriamo il bit 6
            gb->apu.ch4_lfsr |= (xor_bit << 6); // Poi ci scriviamo lo xor_bit
        }
    }

    // --- Campionamento Audio ---
    gb->apu.sampling_timer -= (float)cycles;
    if (gb->apu.sampling_timer <= 0) {
        gb->apu.sampling_timer += 87.3813f;

        float ch1 = get_channel1_sample(gb);
        float ch2 = get_channel2_sample(gb);
        float ch3 = get_channel3_sample(gb);
        float ch4 = get_channel4_sample(gb);
        // Il ch4 lo aggiungeremo al mix nel prossimo passaggio!

        // Mix dei canali con volume master abbassato per evitare distorsioni
        float value = (ch1 + ch2 + ch3 + ch4) * 0.03f;

        gb->apu.buffer[gb->apu.buffer_index] = value;
        gb->apu.buffer_index++;
    }

    // --- Frame Sequencer ---
    gb->apu.frame_sequencer_timer -= cycles;
    if (gb->apu.frame_sequencer_timer <= 0) {
        gb->apu.frame_sequencer_timer += 8192;
        gb->apu.frame_sequencer_step = (gb->apu.frame_sequencer_step + 1) % 8;

        // 1. Length Timer (Scatta agli step 0, 2, 4, 6)
        if (gb->apu.frame_sequencer_step % 2 == 0) {
            // Canale 1
            if (gb->apu.ch1_length_enable == 1 && gb->apu.ch1_length_timer > 0) {
                gb->apu.ch1_length_timer--;
                if (gb->apu.ch1_length_timer == 0) {
                    gb->apu.ch1_channel_enabled = 0;
                }
            }
            // Canale 2
            if (gb->apu.ch2_length_enable == 1 && gb->apu.ch2_length_timer > 0) {
                gb->apu.ch2_length_timer--;
                if (gb->apu.ch2_length_timer == 0) {
                    gb->apu.ch2_channel_enabled = 0;
                }
            }

            // Canale 3
            if (gb->apu.ch3_length_enable == 1 && gb->apu.ch3_length_timer > 0) {
                gb->apu.ch3_length_timer--;
                if (gb->apu.ch3_length_timer == 0) {
                    gb->apu.ch3_channel_enabled = 0;
                }
            }

            // Canale 4
            if (gb->apu.ch4_length_enable == 1 && gb->apu.ch4_length_timer > 0) {
                gb->apu.ch4_length_timer--;
                if (gb->apu.ch4_length_timer == 0) {
                    gb->apu.ch4_channel_enabled = 0;
                }
            }
        }

        // 2. Hardware Sweep Canale 1 (Scatta agli step 2 e 6)
        if (gb->apu.frame_sequencer_step == 2 || gb->apu.frame_sequencer_step == 6) {
            if (gb->apu.ch1_sweep_enabled && gb->apu.ch1_sweep_period > 0) {
                gb->apu.ch1_sweep_timer--;

                if (gb->apu.ch1_sweep_timer == 0) {
                    gb->apu.ch1_sweep_timer = (gb->apu.ch1_sweep_period > 0) ? gb->apu.ch1_sweep_period : 8;

                    uint16_t shifted = gb->apu.ch1_frequency >> gb->apu.ch1_sweep_shift;
                    int new_freq = gb->apu.ch1_frequency;

                    if (gb->apu.ch1_sweep_dir == 1) {
                        new_freq -= shifted;
                    } else {
                        new_freq += shifted;
                    }

                    if (new_freq > 2047) {
                        gb->apu.ch1_channel_enabled = 0;
                    } else if (gb->apu.ch1_sweep_shift > 0) {
                        gb->apu.ch1_frequency = (uint16_t)new_freq;
                    }
                }
            }
        }

        // 3. Inviluppo del volume (Scatta solo allo step 7)
        if (gb->apu.frame_sequencer_step == 7) {
            // Canale 1
            if (gb->apu.ch1_env_period > 0) {
                gb->apu.ch1_env_timer--;
                if (gb->apu.ch1_env_timer == 0) {
                    gb->apu.ch1_env_timer = gb->apu.ch1_env_period;
                    int new_volume = gb->apu.ch1_current_volume + gb->apu.ch1_env_dir;
                    if (new_volume >= 0 && new_volume <= 15) {
                        gb->apu.ch1_current_volume = new_volume;
                    }
                }
            }

            // Canale 2
            if (gb->apu.ch2_env_period > 0) {
                gb->apu.ch2_env_timer--;
                if (gb->apu.ch2_env_timer == 0) {
                    gb->apu.ch2_env_timer = gb->apu.ch2_env_period;
                    int new_volume = gb->apu.ch2_current_volume + gb->apu.ch2_env_dir;
                    if (new_volume >= 0 && new_volume <= 15) {
                        gb->apu.ch2_current_volume = new_volume;
                    }
                }
            }

            // Canale 4
            if (gb->apu.ch4_env_period > 0) {
                gb->apu.ch4_env_timer--;
                if (gb->apu.ch4_env_timer == 0) {
                    gb->apu.ch4_env_timer = gb->apu.ch4_env_period;
                    int new_volume = gb->apu.ch4_current_volume + gb->apu.ch4_env_dir;
                    if (new_volume >= 0 && new_volume <= 15) {
                        gb->apu.ch4_current_volume = new_volume;
                    }
                }
            }
        }
    }
}

uint8_t apu_read(gameBoy* gb, uint16_t addr) {
    return gb->memory[addr];
}

void apu_write(gameBoy* gb, uint16_t addr, uint8_t value) {
    // Registro NR52 (Master Audio Control)
    if (addr == 0xFF26) {
        gb->memory[addr] = (gb->memory[addr] & 0x7F) | (value & 0x80);
        return;
    }

    if ((gb->memory[0xFF26] & 0x80) == 0 && addr != 0xFF26) {
        return;
    }

    gb->memory[addr] = value;

    // --- Gestione Lunghezza Canale 2 (NR21) ---
    if (addr == 0xFF16) {
        gb->apu.ch2_length_timer = 64 - (value & 0x3F);
    }

    // --- Gestione Frequenza, Trigger e Interruttori Canale 2 (NR23 e NR24) ---
    if (addr == 0xFF18 || addr == 0xFF19) {
        gb->apu.ch2_frequency = ((gb->memory[0xFF19] & 0x07) << 8) | gb->memory[0xFF18];

        if (addr == 0xFF19) {
            gb->apu.ch2_length_enable = (value & 0x40) ? 1 : 0;

            if (value & 0x80) {
                gb->apu.ch2_channel_enabled = 1;
                uint8_t env_byte = gb->memory[0xFF17];

                gb->apu.ch2_current_volume = (env_byte >> 4) & 0x0F;
                gb->apu.ch2_env_dir = (env_byte & 0x08) ? 1 : -1;
                gb->apu.ch2_env_period = env_byte & 0x07;
                gb->apu.ch2_env_timer = (gb->apu.ch2_env_period > 0) ? gb->apu.ch2_env_period : 8;

                if (gb->apu.ch2_length_timer == 0) {
                    gb->apu.ch2_length_timer = 64;
                }

                gb->apu.ch2_wave_step = 0;
                gb->apu.ch2_timer = (2048 - gb->apu.ch2_frequency) * 4;
            }
        }
    }

    // --- Gestione Canale 1 (Sweep + Onda Quadra) ---
    if (addr == 0xFF11) {
        gb->apu.ch1_length_timer = 64 - (value & 0x3F);
    }

    if (addr == 0xFF13 || addr == 0xFF14) {
        gb->apu.ch1_frequency = ((gb->memory[0xFF14] & 0x07) << 8) | gb->memory[0xFF13];

        if (addr == 0xFF14) {
            gb->apu.ch1_length_enable = (value & 0x40) ? 1 : 0;

            if (value & 0x80) {
                gb->apu.ch1_channel_enabled = 1;

                uint8_t env_byte = gb->memory[0xFF12];
                gb->apu.ch1_current_volume = (env_byte >> 4) & 0x0F;
                gb->apu.ch1_env_dir = (env_byte & 0x08) ? 1 : -1;
                gb->apu.ch1_env_period = env_byte & 0x07;
                gb->apu.ch1_env_timer = (gb->apu.ch1_env_period > 0) ? gb->apu.ch1_env_period : 8;

                if (gb->apu.ch1_length_timer == 0) {
                    gb->apu.ch1_length_timer = 64;
                }

                gb->apu.ch1_wave_step = 0;
                gb->apu.ch1_timer = (2048 - gb->apu.ch1_frequency) * 4;

                uint8_t sweep_byte = gb->memory[0xFF10];
                gb->apu.ch1_sweep_period = (sweep_byte >> 4) & 0x07;
                gb->apu.ch1_sweep_dir = (sweep_byte & 0x08) ? 1 : 0;
                gb->apu.ch1_sweep_shift = sweep_byte & 0x07;
                gb->apu.ch1_sweep_timer = (gb->apu.ch1_sweep_period > 0) ? gb->apu.ch1_sweep_period : 8;
                gb->apu.ch1_sweep_enabled = (gb->apu.ch1_sweep_period > 0 || gb->apu.ch1_sweep_shift > 0) ? 1 : 0;
            }
        }
    }

    // --- Gestione Canale 3 (Wave RAM) ---
    if (addr == 0xFF1A) {
        if ((value & 0x80) == 0) {
            gb->apu.ch3_channel_enabled = 0;
        }
    }
    else if (addr == 0xFF1B) {
        gb->apu.ch3_length_timer = 256 - value;
    }
    else if (addr == 0xFF1C) {
        gb->apu.ch3_volume = (value >> 5) & 0x03;
    }
    else if (addr == 0xFF1D || addr == 0xFF1E) {
        gb->apu.ch3_frequency = ((gb->memory[0xFF1E] & 0x07) << 8) | gb->memory[0xFF1D];

        if (addr == 0xFF1E) {
            gb->apu.ch3_length_enable = (value & 0x40) ? 1 : 0;

            if (value & 0x80) {
                if (gb->memory[0xFF1A] & 0x80) {
                    gb->apu.ch3_channel_enabled = 1;
                }

                if (gb->apu.ch3_length_timer == 0) {
                    gb->apu.ch3_length_timer = 256;
                }

                gb->apu.ch3_wave_step = 0;
                gb->apu.ch3_timer = (2048 - gb->apu.ch3_frequency) * 2;
            }
        }
    }

    // --- Gestione Canale 4 (Rumore) ---
    if (addr == 0xFF20) {
        // NR41: Lunghezza
        gb->apu.ch4_length_timer = 64 - (value & 0x3F);
    }
    else if (addr == 0xFF22) {
        // NR43: Parametri del rumore
        gb->apu.ch4_clock_shift = value >> 4;
        gb->apu.ch4_width_mode = (value & 0x08) ? 1 : 0;
        gb->apu.ch4_divisor_code = value & 0x07;
    }
    else if (addr == 0xFF23) {
        // NR44: Trigger e Length Enable
        gb->apu.ch4_length_enable = (value & 0x40) ? 1 : 0;

        if (value & 0x80) { // Trigger
            gb->apu.ch4_channel_enabled = 1;

            if (gb->apu.ch4_length_timer == 0) {
                gb->apu.ch4_length_timer = 64;
            }

            // Inizializzazione Inviluppo (da NR42)
            uint8_t env_byte = gb->memory[0xFF21];
            gb->apu.ch4_current_volume = (env_byte >> 4) & 0x0F;
            gb->apu.ch4_env_dir = (env_byte & 0x08) ? 1 : -1;
            gb->apu.ch4_env_period = env_byte & 0x07;
            gb->apu.ch4_env_timer = (gb->apu.ch4_env_period > 0) ? gb->apu.ch4_env_period : 8;

            // Il LFSR si resetta a tutti 1
            gb->apu.ch4_lfsr = 0x7FFF;

            const int divisor_table[8] = {8, 16, 32, 48, 64, 80, 96, 112};
            int divisor = divisor_table[gb->apu.ch4_divisor_code];
            gb->apu.ch4_timer = divisor << gb->apu.ch4_clock_shift;
        }
    }
}

uint8_t get_ch2_wave_form(gameBoy* gb) {
    const uint8_t duty_table[4][8] = {
        {0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 1, 1},
        {0, 1, 1, 1, 1, 1, 1, 0}
    };
    uint8_t pattern = gb->memory[0xFF16] >> 6;
    return duty_table[pattern][gb->apu.ch2_wave_step];
}

float get_channel2_sample(gameBoy* gb) {
    if (gb->apu.ch2_channel_enabled == 0) return 0.0f;
    uint8_t volume = gb->apu.ch2_current_volume;
    if (volume == 0) return 0.0f;

    uint8_t wave = get_ch2_wave_form(gb);
    float wave_centered = (wave == 1) ? 1.0f : -1.0f;
    return wave_centered * ((float)volume / 15.0f);
}

uint8_t get_ch1_wave_form(gameBoy* gb) {
    const uint8_t duty_table[4][8] = {
        {0, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 0, 0, 1},
        {1, 0, 0, 0, 0, 1, 1, 1},
        {0, 1, 1, 1, 1, 1, 1, 0}
    };
    uint8_t pattern = gb->memory[0xFF11] >> 6;
    return duty_table[pattern][gb->apu.ch1_wave_step];
}

float get_channel1_sample(gameBoy* gb) {
    if (gb->apu.ch1_channel_enabled == 0) return 0.0f;
    if (gb->apu.ch1_current_volume == 0) return 0.0f;

    uint8_t wave = get_ch1_wave_form(gb);
    float wave_centered = (wave == 1) ? 1.0f : -1.0f;
    return wave_centered * ((float)gb->apu.ch1_current_volume / 15.0f);
}

float get_channel3_sample(gameBoy* gb) {
    if (gb->apu.ch3_channel_enabled == 0) return 0.0f;
    if ((gb->memory[0xFF1A] & 0x80) == 0) return 0.0f;

    uint16_t wave_ram_addr = 0xFF30 + (gb->apu.ch3_wave_step / 2);
    uint8_t wave_byte = gb->memory[wave_ram_addr];

    uint8_t sample = (gb->apu.ch3_wave_step % 2 == 0) ? (wave_byte >> 4) : (wave_byte & 0x0F);

    if (gb->apu.ch3_volume == 0) {
        return 0.0f;
    } else if (gb->apu.ch3_volume == 2) {
        sample >>= 1;
    } else if (gb->apu.ch3_volume == 3) {
        sample >>= 2;
    }

    return ((float)sample / 7.5f) - 1.0f;
}

float get_channel4_sample(gameBoy* gb) {
    if (gb->apu.ch4_channel_enabled == 0) return 0.0f;
    if (gb->apu.ch4_current_volume == 0) return 0.0f;

    // Il Game Boy usa il bit 0 dell'LFSR invertito per creare il suono
    uint8_t wave = (gb->apu.ch4_lfsr & 0x01) ? 0 : 1;

    float wave_centered = (wave == 1) ? 1.0f : -1.0f;
    return wave_centered * ((float)gb->apu.ch4_current_volume / 15.0f);
}