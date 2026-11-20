/* SPDX-License-Identifier: GPL-3.0-only */
#define main triptych_layout_main
#include "dinghy.c"
#undef main
#include <stdio.h>

int main(void)
{
    printf("midi_cfg_v0_t %u\n", (unsigned)sizeof(midi_cfg_v0_t));
    printf("midi_cfg_v1_t %u\n", (unsigned)sizeof(midi_cfg_v1_t));
    printf("midi_cfg_t %u\n", (unsigned)sizeof(midi_cfg_t));
    printf("midi_state_t %u\n", (unsigned)sizeof(midi_state_t));
    printf("perf_state_t %u\n", (unsigned)sizeof(perf_state_t));
    return 0;
}
