/* SPDX-License-Identifier: GPL-3.0-only */
#include "jet_dsp.h"
#include <stdio.h>

int main(void)
{
    jet_synth_t s;
    jet_voice_cfg_t p = {80, 40, 48, 64, 70, 100, 40, 90, 32, 48, 100, 24, 32, 24};
    int16_t L[8];
    int16_t R[8];
    uint32_t i;
    int32_t peak = 0;

    jet_synth_init(&s);
    jet_synth_note_on(&s, &p, 60u, 100u);
    for (i = 0u; i < 32u; i++)
        jet_synth_render_block(&s, &p, L, R, 8u);
    for (i = 0u; i < 8u; i++) {
        int32_t a = L[i] >= 0 ? L[i] : -L[i];
        if (a > peak)
            peak = a;
    }
    if (peak < 50) {
        fprintf(stderr, "jet_dsp_smoke: expected audio after note_on, peak=%d\n", (int)peak);
        return 1;
    }
    printf("jet_dsp_smoke ok peak=%d\n", (int)peak);
    return 0;
}
