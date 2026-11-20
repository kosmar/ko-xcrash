/* SPDX-License-Identifier: GPL-3.0-only */
/* CPL share LSB check.
 * 1) Macro/MOTION couplings: old hardcoded factor vs dub_cpl_scale() at the genre default depths.
 * 2) FLUX/GLIDE couplings: old multi-term sum (copied from the pre-CPL code) vs dub_cpl_scale(S, default share)
 *    over a realistic sweep of the LFO sources (mean limit 2 LSB, max documented).
 *    Default shares must match dub_cpl_map[] in fm_app.c.
 * Build: cc -O2 -Wall -Ifirmware/src -o build/host/cpl_lsb tests/cpl_lsb.c && build/host/cpl_lsb */
#include <stdint.h>
#include <stdio.h>

#include "dub_types.h"

typedef struct {
    const char *name;
    int32_t (*old)(int32_t x);
    uint8_t depth;
} cpl_case_t;

static int32_t old_full(int32_t x) { return x; }
static int32_t old_half(int32_t x) { return x >> 1; }
static int32_t old_quarter(int32_t x) { return x >> 2; }
static int32_t old_three_quarter(int32_t x) { return (x * 3) / 4; }
static int32_t old_third(int32_t x) { return x / 3; }
static int32_t old_div4(int32_t x) { return x / 4; }

static const cpl_case_t cases[] = {
    { "x (depth 127)",       old_full,           127u },
    { "x>>1 (depth 64)",     old_half,            64u },
    { "x>>2 (depth 32)",     old_quarter,         32u },
    { "x*3/4 (depth 96)",    old_three_quarter,   96u },
    { "x/3 (depth 43)",      old_third,           43u },
    { "x/4 (depth 32)",      old_div4,            32u },
};

/* FLUX / GLIDE couplings. a/b: FLUX -> fd/ph, GLIDE -> eff_motion/md. */
enum { F_FB, F_WOW, F_REV, F_LOOP, F_CUTC, F_CUTB, F_WOB, F_DRMPAN, F_GLIDE, F_N };
static const char *const f_name[F_N] = {
    "FB_FLUX", "WOW_FLUX", "REV_FLUX", "LOOP_FLUX", "FLX_CUTC", "FLX_CUTB", "FLX_WOB", "FLX_DRMPAN", "MOT_GLIDE"
};
static const uint8_t f_dflt[F_N] = { 127u, 127u, 127u, 127u, 127u, 127u, 127u, 127u, 127u };

/* Old two-term sums as written before the CPL change. */
static int32_t old_term(int t, int32_t a, int32_t b)
{
    switch (t) {
    case F_FB:     return a / 2 + b / 6;
    case F_WOW:    return b / 2 + a / 4;
    case F_REV:    return a / 8 + b / 10;
    case F_LOOP:   return b / 5 + a / 10;
    case F_CUTC:   return (a * 3) / 4 + b / 3;
    case F_CUTB:   return a / 2 + b / 4;
    case F_WOB:    return (a * 2) / 3 + b / 4;
    case F_DRMPAN: return a / 3 + b / 6;
    default:       return (a >> 3) + b / 12;
    }
}

/* New single-term source S (dub_types.h) scaled by the default share. */
static int32_t new_term(int t, int32_t a, int32_t b)
{
    switch (t) {
    case F_FB:     return dub_cpl_scale(dub_s_fb_flux(a, b), f_dflt[t]);
    case F_WOW:    return dub_cpl_scale(dub_s_wow_flux(a, b), f_dflt[t]);
    case F_REV:    return dub_cpl_scale(dub_s_rev_flux(a, b), f_dflt[t]);
    case F_LOOP:   return dub_cpl_scale(dub_s_loop_flux(a, b), f_dflt[t]);
    case F_CUTC:   return dub_cpl_scale(dub_s_flx_cutc(a, b), f_dflt[t]);
    case F_CUTB:   return dub_cpl_scale(dub_s_flx_cutb(a, b), f_dflt[t]);
    case F_WOB:    return dub_cpl_scale(dub_s_flx_wob(a, b), f_dflt[t]);
    case F_DRMPAN: return dub_cpl_scale(dub_s_flx_drmpan(a, b), f_dflt[t]);
    default:       return dub_cpl_scale(dub_s_mot_glide(a, b), f_dflt[t]);
    }
}

typedef struct {
    double sum_def, sum_alt, num, den;
    int32_t max_def, max_alt;
    uint8_t alt_share;
} stat_t;

#define MEAN_LIMIT 2.0
#define SWEEP_E_STEP 3
#define SWEEP_W_STEP 4
#define SWEEP_S_STEP 4

static int32_t iabs(int32_t v) { return v < 0 ? -v : v; }

/* Sweep: eff 1..127; w_slow/w_fast -128..127 (dub_ms_tri); slot delta -127..127 (fm_lfo_slot_delta).
 * pass 0: default-share error + LSQ fit of the alternative S = fd + ph; pass 1: alternative error at its fitted share. */
static double sweep(stat_t st[F_N], int pass)
{
    double count = 0.0;
    int32_t e, ws, wf, sd;

    for (e = 1; e <= 127; e += SWEEP_E_STEP)
    for (ws = -128; ws <= 127; ws += SWEEP_W_STEP)
    for (wf = -128; wf <= 127; wf += SWEEP_W_STEP)
    for (sd = -127; sd <= 127; sd += SWEEP_S_STEP) {
        int32_t fd = (ws * e) / 64 + (sd * e) / 128;
        int32_t ph = (wf * e) / 64 - (sd * e) / 192;
        int32_t md = (ws * e) / 64 + (wf * e) / 96 + (sd * e) / 128;
        int t;

        count += 1.0;
        for (t = 0; t < F_N; t++) {
            int32_t a = (t == F_GLIDE) ? e : fd;
            int32_t b = (t == F_GLIDE) ? md : ph;
            int32_t o = old_term(t, a, b);
            int32_t s_alt = a + b;

            if (pass == 0) {
                int32_t d = iabs(new_term(t, a, b) - o);

                st[t].sum_def += d;
                if (d > st[t].max_def)
                    st[t].max_def = d;
                if (t != F_GLIDE) {
                    st[t].num += (double)o * (double)s_alt;
                    st[t].den += (double)s_alt * (double)s_alt;
                }
            } else if (t != F_GLIDE) {
                int32_t d = iabs(dub_cpl_scale(s_alt, st[t].alt_share) - o);

                st[t].sum_alt += d;
                if (d > st[t].max_alt)
                    st[t].max_alt = d;
            }
        }
    }
    return count;
}

static int flux_sweep(void)
{
    stat_t st[F_N];
    double count;
    int t, fail = 0;

    for (t = 0; t < F_N; t++) {
        st[t].sum_def = st[t].sum_alt = st[t].num = st[t].den = 0.0;
        st[t].max_def = st[t].max_alt = 0;
        st[t].alt_share = 0u;
    }
    count = sweep(st, 0);
    for (t = 0; t < F_N; t++) {
        if (t == F_GLIDE)
            continue;
        {
            double sh = st[t].den > 0.0 ? 128.0 * st[t].num / st[t].den : 0.0;
            int32_t r = (int32_t)(sh + (sh >= 0.0 ? 0.5 : -0.5));

            st[t].alt_share = (uint8_t)(r < 0 ? 0 : r > 127 ? 127 : r);
        }
    }
    sweep(st, 1);

    printf("flux sweep: %d samples per target\n", (int)count);
    for (t = 0; t < F_N; t++) {
        double mean = st[t].sum_def / count;

        printf("  %-10s share=%3u  mean=%.2f LSB  max=%d LSB", f_name[t], (unsigned)f_dflt[t], mean, (int)st[t].max_def);
        if (t != F_GLIDE)
            printf("   [alt S=fd+ph: share=%3u mean=%.2f max=%d]", (unsigned)st[t].alt_share,
                   st[t].sum_alt / count, (int)st[t].max_alt);
        printf("\n");
        if (mean > MEAN_LIMIT)
            fail++;
    }
    if (fail) {
        printf("flux sweep: FAIL (%d target(s) over mean limit %.1f LSB)\n", fail, MEAN_LIMIT);
        return 1;
    }
    printf("flux sweep: OK (all targets mean <= %.1f LSB)\n", MEAN_LIMIT);
    return 0;
}

int main(void)
{
    const int32_t lo = -127, hi = 127; /* covers the -64..63 sweep and the wider LFO delta range */
    size_t i;
    int32_t x, worst = 0;
    int fail = 0;

    for (i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        int32_t worst_case = 0;

        for (x = lo; x <= hi; x++) {
            int32_t d = dub_cpl_scale(x, cases[i].depth) - cases[i].old(x);

            if (d < 0)
                d = -d;
            if (d > worst_case)
                worst_case = d;
        }
        printf("%-18s max |delta| = %d LSB\n", cases[i].name, (int)worst_case);
        if (worst_case > 1)
            fail++;
        if (worst_case > worst)
            worst = worst_case;
    }

    /* Depth 0 must switch the coupling off completely. */
    for (x = lo; x <= hi; x++) {
        if (dub_cpl_scale(x, 0u) != 0) {
            printf("depth 0 not zero at x=%d\n", (int)x);
            fail++;
            break;
        }
    }

    if (fail) {
        printf("cpl_lsb: FAIL (%d case(s), worst %d LSB)\n", fail, (int)worst);
        return 1;
    }
    printf("cpl_lsb: OK (worst %d LSB, x in [%d, %d])\n", (int)worst, (int)lo, (int)hi);

    if (flux_sweep())
        return 1;
    return 0;
}
