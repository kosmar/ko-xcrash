/* SPDX-License-Identifier: GPL-3.0-only */
/* Host check: shared 5-anchor CURVE axis in fm_env_plot.h (attack / decay / release / step / names). */
#include "fm_env_plot.h"
#include <stdio.h>

static int fail;

static void check(int c, const char *m)
{
    if (!c) {
        fprintf(stderr, "fm_env_curve_anchors: %s\n", m);
        fail = 1;
    }
}

/* Pre-change 2-anchor blend (reference, copied from the old fm_env_plot.h). */
static int32_t old_mix(int32_t y_lin, int32_t y_low, int32_t y_high, uint8_t curve)
{
    if (curve < 64u)
        return y_lin + ((y_low - y_lin) * (int32_t)(64u - curve)) / 64;
    if (curve > 64u)
        return y_lin + ((y_high - y_lin) * (int32_t)(curve - 64u)) / 63;
    return y_lin;
}

static int32_t old_attack(int32_t y_bot, int32_t y_peak, uint8_t prog, uint8_t curve)
{
    int32_t y_lin = y_bot + ((y_peak - y_bot) * (int32_t)prog) / 128;
    int32_t y_high = y_bot + ((y_peak - y_bot) * (int32_t)prog * (int32_t)prog) / (128 * 128);
    int32_t y_low = y_lin + y_lin - y_high;

    return old_mix(y_lin, y_low, y_high, curve);
}

static int32_t old_fall(int32_t top, int32_t bot, uint8_t prog, uint8_t curve)
{
    int32_t p = (int32_t)prog;
    int32_t span = top - bot;
    int32_t y_lin = top - (span * p) / 128;
    int32_t y_low = top - (span * p * p) / (128 * 128);
    int32_t tr = 128 - p;
    int32_t y_high = bot + (span * tr * tr) / (128 * 128);

    return old_mix(y_lin, y_low, y_high, curve);
}

static int32_t old_step_mul(uint8_t prog, uint8_t curve, uint8_t release_seg)
{
    int32_t lin = 128;
    int32_t low_m;
    int32_t high_m;
    int32_t t = (int32_t)prog;

    if (release_seg) {
        int32_t tr = 128 - t;

        low_m = 24 + (t * t >> 6);
        high_m = 24 + (tr * tr >> 6);
    } else {
        low_m = 24 + (t * t >> 6);
        high_m = 24 + ((128 - t) * t >> 6);
    }
    return old_mix(lin, low_m, high_m, curve);
}

static int32_t isqrt_ref(int32_t v)
{
    int32_t r = 0;

    while ((r + 1) * (r + 1) <= v)
        r++;
    return r;
}

int main(void)
{
    static const int32_t bots[4] = {0, 0, 10, 100};
    static const int32_t peaks[4] = {32767, 128, 5000, 120};
    uint32_t i;
    int32_t b;
    int32_t pk;
    int32_t v;
    int32_t prev;
    int c;
    int p;
    int rel;

    fail = 0;

    /* isqrt matches floor(sqrt) over the full argument range used (0..128*128) */
    for (v = 0; v <= 16384; v++)
        if (fm_env_isqrt(v) != isqrt_ref(v)) {
            check(0, "fm_env_isqrt != floor(sqrt)");
            break;
        }

    /* curve 0 / 64 / 127 identical to the pre-change formulas (attack, decay, release, step) */
    for (i = 0; i < 4u; i++) {
        b = bots[i];
        pk = peaks[i];
        for (p = 0; p <= 128; p++) {
            static const uint8_t anc[3] = {0u, 64u, 127u};
            int k;

            for (k = 0; k < 3; k++) {
                uint8_t cv = anc[k];
                uint8_t pu = (uint8_t)p;

                check(fm_env_attack_level(b, pk, pu, cv) == old_attack(b, pk, pu, cv), "attack anchor != old");
                check(fm_env_decay_level(pk, b, pu, cv) == old_fall(pk, b, pu, cv), "decay anchor != old");
                check(fm_env_release_level(pk, b, pu, cv) == old_fall(pk, b, pu, cv), "release anchor != old");
                for (rel = 0; rel < 2; rel++)
                    check(fm_env_decay_step_mul(pu, cv, (uint8_t)rel) == old_step_mul(pu, cv, (uint8_t)rel),
                          "step_mul anchor != old");
            }
        }
    }

    /* all curves: attack within [bot, peak] and non-decreasing; decay/release within [bot, top], non-increasing.
     * Monotonic check allows 1 LSB: integer floors in the legacy nexp / exp rounding dip by 1 on small spans. */
    for (i = 0; i < 4u; i++) {
        b = bots[i];
        pk = peaks[i];
        for (c = 0; c < 128; c++) {
            prev = b - 1;
            for (p = 0; p <= 128; p++) {
                v = fm_env_attack_level(b, pk, (uint8_t)p, (uint8_t)c);
                check(v >= b && v <= pk, "attack out of bounds");
                check(v >= prev - 1, "attack not monotonic");
                prev = v;
            }
            prev = pk + 1;
            for (p = 0; p <= 128; p++) {
                v = fm_env_decay_level(pk, b, (uint8_t)p, (uint8_t)c);
                check(v >= b && v <= pk, "decay out of bounds");
                check(v <= prev + 1, "decay not monotonic");
                prev = v;
            }
            prev = pk + 1;
            for (p = 0; p <= 128; p++) {
                v = fm_env_release_level(pk, b, (uint8_t)p, (uint8_t)c);
                check(v >= b && v <= pk, "release out of bounds");
                check(v <= prev + 1, "release not monotonic");
                prev = v;
            }
            for (p = 0; p <= 128; p++)
                for (rel = 0; rel < 2; rel++) {
                    int32_t m = fm_env_decay_step_mul((uint8_t)p, (uint8_t)c, (uint8_t)rel);

                    check(m >= 24 && m <= 280, "step_mul out of 24..280");
                }
        }
    }

    /* anchors 32 and 96 are distinct from 0 and 64 (and from each other) at mid progress */
    {
        int32_t a0 = fm_env_attack_level(0, 32767, 64u, 0u);
        int32_t a32 = fm_env_attack_level(0, 32767, 64u, 32u);
        int32_t a64 = fm_env_attack_level(0, 32767, 64u, 64u);
        int32_t a96 = fm_env_attack_level(0, 32767, 64u, 96u);
        int32_t a127 = fm_env_attack_level(0, 32767, 64u, 127u);

        check(a32 != a0 && a32 != a64, "anchor 32 equals 0 or 64");
        check(a96 != a0 && a96 != a64 && a96 != a32, "anchor 96 equals another anchor");
        check(a127 != a96, "anchor 127 equals anchor 96");
        check(fm_env_attack_level(0, 32767, 128u, 32u) == 32767, "attack curve 32 reaches peak at p=128");
        check(fm_env_attack_level(0, 32767, 0u, 96u) == 0, "attack curve 96 starts at floor");
    }

    /* anchor names */
    check(fm_env_shape_name(0u)[0] == '-' && fm_env_shape_name(0u)[1] == 'E', "name 0 is -EXP");
    check(fm_env_shape_name(32u)[0] == 'L' && fm_env_shape_name(32u)[1] == 'O', "name 32 is LOG");
    check(fm_env_shape_name(64u)[0] == 'L' && fm_env_shape_name(64u)[1] == 'I', "name 64 is LIN");
    check(fm_env_shape_name(96u)[0] == '-' && fm_env_shape_name(96u)[1] == 'L', "name 96 is -LOG");
    check(fm_env_shape_name(127u)[0] == 'E' && fm_env_shape_name(127u)[1] == 'X', "name 127 is EXP");
    check(fm_env_shape_name(16u)[0] == 'B', "name 16 is BLND");
    check(fm_env_shape_name(53u)[0] == 'B', "name 53 is BLND");

    if (fail) {
        fprintf(stderr, "fm_env_curve_anchors: FAIL\n");
        return 1;
    }
    printf("fm_env_curve_anchors: OK\n");
    return 0;
}
