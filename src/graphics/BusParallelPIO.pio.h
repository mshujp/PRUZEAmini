#pragma once

#if defined(ARDUINO_ARCH_RP2040)

#include <hardware/pio.h>

#define pruzea_parallel_wrap_target 0
#define pruzea_parallel_wrap 1

static const uint16_t pruzea_parallel_program_instructions[] = {
    0x6008,
    0xb042,
};

static const struct pio_program pruzea_parallel_program = {
    .instructions = pruzea_parallel_program_instructions,
    .length = 2,
    .origin = -1,
};

static inline pio_sm_config pruzea_parallel_program_get_default_config(uint offset)
{
    pio_sm_config config = pio_get_default_sm_config();
    sm_config_set_wrap(&config, offset + pruzea_parallel_wrap_target, offset + pruzea_parallel_wrap);
    sm_config_set_sideset(&config, 1, false, false);
    return config;
}

#endif
