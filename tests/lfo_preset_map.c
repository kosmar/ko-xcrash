/* SPDX-License-Identifier: GPL-3.0-only */
#include "fm_lfo.h"
#include <stdio.h>
#include <stdlib.h>

static int fail;

static void check(int cond, const char *msg)
{
    if (!cond) {
        fprintf(stderr, "lfo_preset_map: %s\n", msg);
        fail = 1;
    }
}

static int within2(uint8_t a, uint8_t b)
{
    int d = (int)a - (int)b;

    if (d < 0)
        d = -d;
    return d <= 2;
}

int main(void)
{
    uint8_t r;
    uint8_t min120;

    fail = 0;

    check(fm_lfo_legacy_shape(0u) == 0u, "legacy 0");
    check(fm_lfo_legacy_shape(1u) == 30u, "legacy 1");
    check(fm_lfo_legacy_shape(2u) == 75u, "legacy 2");
    check(fm_lfo_legacy_shape(3u) == 90u, "legacy 3");
    check(fm_lfo_legacy_shape(4u) == 45u, "legacy 4");
    check(fm_lfo_legacy_shape(90u) == 90u, "legacy pass 90");
    check(fm_lfo_legacy_shape(114u) == 114u, "legacy pass 114");
    check(fm_lfo_legacy_shape(127u) == 127u, "legacy pass 127");

    check(fm_lfo_rate_from_div(FM_LFO_DIV_1_1, 120u) == 22u, "1/1 sentinel");
    check(fm_lfo_rate_from_div(FM_LFO_DIV_1_1, 90u) == 22u, "1/1 any bpm");

    r = fm_lfo_rate_from_div(FM_LFO_DIV_1_2, 120u);
    check(within2(r, 32u), "1/2 near 32 at 120");
    r = fm_lfo_rate_from_div(FM_LFO_DIV_1_4, 120u);
    check(within2(r, 45u), "1/4 near 45 at 120");
    r = fm_lfo_rate_from_div(FM_LFO_DIV_1_8, 120u);
    check(within2(r, 64u), "1/8 near 64 at 120");

    min120 = fm_lfo_rate_min(120u);
    r = fm_lfo_rate_from_div(FM_LFO_DIV_1_2, 120u);
    check(r > min120, "1/2 above min");
    r = fm_lfo_rate_from_div(FM_LFO_DIV_1_4, 120u);
    check(r > min120, "1/4 above min");
    r = fm_lfo_rate_from_div(FM_LFO_DIV_1_8, 120u);
    check(r > min120, "1/8 above min");

    check(fm_lfo_div_ticks(FM_LFO_DIV_1_8T) == 8u, "1/8T ticks");

    return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
