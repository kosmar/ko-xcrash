/* SPDX-License-Identifier: GPL-3.0-only */
/* KO BASS EXT clock: 24 ppqn → one step per 6 F8, samples-per-step from F8 timing. */
#include "dub_types.h"
#include "bass_app.h"
#include <stdio.h>

#define FS    44100u
#define BLOCK 256u

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "bass_clock_ext: %s\n", m);
        fail = 1;
    }
}

static uint32_t run(uint32_t bpm, uint32_t pulses, bass_app_rt_t *rt)
{
    /* F8 every FS*60/(bpm*24) samples, seen at audio-block granularity */
    uint64_t f8_q = (uint64_t)FS * 60u * 1000u / ((uint64_t)bpm * 24u);
    uint32_t i, steps = 0u;

    for (i = 0; i < pulses; i++) {
        uint32_t t = (uint32_t)(((uint64_t)i * f8_q) / 1000u);
        uint32_t seen = (t / BLOCK) * BLOCK;

        steps += bass_ext_clock_pulse(rt, seen, FS);
    }
    return steps;
}

int main(void)
{
    static const uint32_t bpms[] = {80u, 120u, 174u};
    uint32_t k;

    for (k = 0; k < sizeof bpms / sizeof bpms[0]; k++) {
        bass_app_rt_t rt;
        uint32_t want = FS * 60u / (bpms[k] * 4u);
        uint32_t steps;
        int32_t err;

        bass_app_rt_init(&rt);
        steps = run(bpms[k], 24u * 16u, &rt);
        err = (int32_t)rt.ext_spp - (int32_t)want;
        if (err < 0)
            err = -err;
        printf("bass_clock_ext: %3u BPM  steps=%u (want 64)  spp=%u (want %u)\n", bpms[k], steps, rt.ext_spp,
               want);
        check(steps == 64u, "24 F8 per quarter must give 4 steps per quarter");
        check((uint32_t)err * 50u <= want, "ext spp off by more than 2 %");
    }
    if (fail)
        return 1;
    printf("bass_clock_ext: OK\n");
    return 0;
}
