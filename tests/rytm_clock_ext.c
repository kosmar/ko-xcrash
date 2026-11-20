/* SPDX-License-Identifier: GPL-3.0-only */
/* KO RYTM EXT clock: 96 F8 (24 ppqn, 1 bar) advances 16 steps via app_audio. */
#include <stdio.h>
#define main rytm_host_main
#include "rytm.c"
#undef main

#define BLOCK 256u

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "rytm_clock_ext: %s\n", m);
        fail = 1;
    }
}

int main(void)
{
    static int32_t buf[BLOCK * 2];
    uint32_t i;
    uint8_t start_step;
    uint8_t start_bar;
    uint8_t end_step;
    uint8_t len;

    app_init();
    g.playing = 1u;
    g.ext_clock = 1u;
    g.xport_reset_req = 1u;
    app_audio(buf, BLOCK);
    start_step = g.step;
    start_bar = g.bar;
    len = seq_len();
    check(len >= 16u, "seq_len must be at least 16");
    check(start_step == 0u, "transport reset must leave step at 0");

    for (i = 0; i < 96u; i++) {
        g.ext_f8_in++;
        app_audio(buf, BLOCK);
    }
    end_step = g.step;
    printf("rytm_clock_ext: start_step=%u end_step=%u start_bar=%u end_bar=%u\n",
           start_step, end_step, start_bar, g.bar);
    /* 16 step fires wrap a 16-step page: step returns to 0 and bar advances. */
    check(end_step == start_step, "96 F8 must land on the same step after one page");
    check(g.bar == (uint8_t)(start_bar + 1u), "96 F8 must advance exactly one bar");
    if (fail)
        return 1;
    printf("rytm_clock_ext: OK\n");
    return 0;
}
