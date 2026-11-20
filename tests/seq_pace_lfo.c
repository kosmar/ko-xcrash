/* SPDX-License-Identifier: GPL-3.0-only */
/* SEQ PACE LFO mapping check (pure helpers in dub_types.h).
 * - clamp: effective pace index always 0..DUB_PACE_IDX_MAX
 * - delta mapping: LFO delta -127..127 -> whole steps -4..4, symmetric, dead zone +-15, monotone
 * - every pace index is reachable from every base by sweeping the delta
 * Build: cc -O2 -Wall -Ifirmware/src -o build/host/seq_pace_lfo tests/seq_pace_lfo.c && build/host/seq_pace_lfo */
#include <stdint.h>
#include <stdio.h>

#include "dub_types.h"

static int fails;

#define CHECK(cond, ...)                 \
    do {                                 \
        if (!(cond)) {                   \
            fails++;                     \
            fprintf(stderr, __VA_ARGS__); \
            fprintf(stderr, "\n");       \
        }                                \
    } while (0)

int main(void)
{
    int32_t d, b;
    uint8_t seen[DUB_PACE_IDX_MAX + 1];
    int32_t prev;

    CHECK(DUB_SEQ_LFO_PACE_IX == 9u && DUB_SEQ_LFO_LANE_N == 10u, "seq_lfo_lane layout");
    CHECK(DUB_PACE_IDX_MAX == 4, "pace max must be 4 (5 steps)");

    /* Clamp on the output for every base 0..4 and every delta -127..127 (and beyond). */
    for (b = 0; b <= DUB_PACE_IDX_MAX; b++) {
        for (d = -200; d <= 200; d++) {
            uint8_t e = dub_pace_eff((uint8_t)b, d);

            CHECK(e <= DUB_PACE_IDX_MAX, "clamp high: base %d delta %d -> %d", (int)b, (int)d, (int)e);
        }
    }
    CHECK(dub_pace_eff(0u, -127) == 0u, "base 0 + full negative must stay 0");
    CHECK(dub_pace_eff(4u, 127) == 4u, "base 4 + full positive must stay 4");
    CHECK(dub_pace_eff(1u, 0) == 1u, "zero delta keeps base");
    CHECK(dub_pace_eff(0u, 127) == 4u, "base 0 + full positive -> 4 (1/2)");
    CHECK(dub_pace_eff(4u, -127) == 0u, "base 4 + full negative -> 0 (1/32)");

    /* Mapping: symmetric, dead zone, extremes. */
    CHECK(dub_pace_lfo_offset(0) == 0, "offset(0)");
    CHECK(dub_pace_lfo_offset(15) == 0 && dub_pace_lfo_offset(-15) == 0, "dead zone +-15");
    CHECK(dub_pace_lfo_offset(16) == 1, "offset(16) == 1");
    CHECK(dub_pace_lfo_offset(127) == 4, "offset(127) == 4");
    CHECK(dub_pace_lfo_offset(-127) == -4, "offset(-127) == -4");
    CHECK(dub_pace_lfo_offset(9999) == 4 && dub_pace_lfo_offset(-9999) == -4, "offset clamps input");
    for (d = 0; d <= 127; d++)
        CHECK(dub_pace_lfo_offset(d) == -dub_pace_lfo_offset(-d), "symmetry at %d", (int)d);

    /* Monotone and step-quantized: offset only changes by at most 1 between neighbours. */
    prev = dub_pace_lfo_offset(-127);
    for (d = -126; d <= 127; d++) {
        int32_t o = dub_pace_lfo_offset(d);

        CHECK(o >= prev && o - prev <= 1, "monotone/step at %d", (int)d);
        prev = o;
    }

    /* Reachability: sweeping delta from every base reaches all five indices. */
    for (b = 0; b <= DUB_PACE_IDX_MAX; b++) {
        int32_t n;
        int cnt = 0;

        for (n = 0; n <= DUB_PACE_IDX_MAX; n++)
            seen[n] = 0u;
        for (d = -127; d <= 127; d++)
            seen[dub_pace_eff((uint8_t)b, d)] = 1u;
        for (n = 0; n <= DUB_PACE_IDX_MAX; n++)
            cnt += seen[n];
        CHECK(cnt == DUB_PACE_IDX_MAX + 1, "reachability from base %d: %d of 5", (int)b, cnt);
    }

    if (fails) {
        fprintf(stderr, "seq_pace_lfo: %d failure(s)\n", fails);
        return 1;
    }
    printf("seq_pace_lfo: OK\n");
    return 0;
}
