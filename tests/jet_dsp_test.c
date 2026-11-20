/* SPDX-License-Identifier: GPL-3.0-only */
#include "jet_dsp.h"
#include "jet_plates.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FS 44100u

static double peak_abs(const int32_t *l, const int32_t *r, uint32_t n)
{
    uint32_t i;
    double pk = 0.0;

    for (i = 0u; i < n; i++) {
        double a = (double)l[i];
        if (a < 0.0)
            a = -a;
        if (a > pk)
            pk = a;
        a = (double)r[i];
        if (a < 0.0)
            a = -a;
        if (a > pk)
            pk = a;
    }
    return pk;
}

static double rms_stereo(const int32_t *l, const int32_t *r, uint32_t n)
{
    uint64_t acc = 0;
    uint32_t i;

    for (i = 0u; i < n; i++) {
        int64_t a = l[i];
        int64_t b = r[i];
        acc += (uint64_t)(a * a + b * b);
    }
    return sqrt((double)acc / (double)(2u * n)) / 32767.0;
}

static double dbfs_peak(double pk)
{
    if (pk < 1.0)
        return -120.0;
    return 20.0 * log10(pk / 32767.0);
}

static double dbfs_rms(double rms)
{
    if (rms < 1e-12)
        return -120.0;
    return 20.0 * log10(rms);
}

static void engine_sync(jet_engine_t *e, const jet_patch_t *p)
{
    jet_coef_t c;
    jet_coef_build(&c, p, 120u);
    e->patch = *p;
    e->mode = p->mode;
    e->glide_coef = p->glide;
    e->detune_spread = p->detune;
    jet_engine_publish(e, &c);
}

static int render_block(jet_engine_t *e, int32_t *l, int32_t *r, uint32_t n)
{
    jet_engine_render(e, l, r, n);
    return 0;
}

static int test_level(void)
{
    jet_engine_t eng;
    jet_patch_t patch;
    int32_t *l;
    int32_t *r;
    double pk100;
    double pk30;
    double db;

    l = (int32_t *)malloc(FS * sizeof(int32_t));
    r = (int32_t *)malloc(FS * sizeof(int32_t));
    if (!l || !r)
        return 1;
    jet_engine_init(&eng);
    jet_patch_defaults(&patch);
    engine_sync(&eng, &patch);
    jet_engine_note_on(&eng, 60u, 100u, 0u);
    render_block(&eng, l, r, FS);
    pk100 = peak_abs(l, r, FS);
    db = dbfs_peak(pk100);
    printf("test1 vel100 peak=%.2f dBFS (%.0f)\n", db, pk100);
    if (db < -6.5 || db > -0.5) {
        fprintf(stderr, "test1 fail: peak %.2f dBFS\n", db);
        free(l);
        free(r);
        return 1;
    }
    jet_engine_init(&eng);
    engine_sync(&eng, &patch);
    jet_engine_note_on(&eng, 60u, 30u, 0u);
    render_block(&eng, l, r, FS);
    pk30 = peak_abs(l, r, FS);
    printf("test1 vel30 peak=%.2f dBFS\n", dbfs_peak(pk30));
    if (pk30 > pk100 * 0.5) {
        fprintf(stderr, "test1 fail: vel30 not 6dB quieter\n");
        free(l);
        free(r);
        return 1;
    }
    free(l);
    free(r);
    return 0;
}

static double estimate_period(const int32_t *x, uint32_t n, uint32_t min_lag, uint32_t max_lag)
{
    uint32_t best_lag = min_lag;
    int64_t best = -1;
    int64_t c_m1 = 0;
    int64_t c_0 = 0;
    int64_t c_p1 = 0;
    uint32_t lag;

    for (lag = min_lag; lag <= max_lag && lag < n / 2u; lag++) {
        int64_t c = 0;
        uint32_t i;
        for (i = 0u; i + lag < n; i++)
            c += (int64_t)x[i] * (int64_t)x[i + lag];
        if (c > best) {
            best = c;
            best_lag = lag;
            c_0 = c;
            c_m1 = 0;
            c_p1 = 0;
            if (lag > min_lag && lag > 0u) {
                uint32_t j;
                c_m1 = 0;
                for (j = 0u; j + (lag - 1u) < n; j++)
                    c_m1 += (int64_t)x[j] * (int64_t)x[j + lag - 1u];
            }
            if (lag + 1u <= max_lag && lag + 1u < n / 2u) {
                uint32_t j;
                c_p1 = 0;
                for (j = 0u; j + (lag + 1u) < n; j++)
                    c_p1 += (int64_t)x[j] * (int64_t)x[j + lag + 1u];
            }
        }
    }
    {
        double denom = (double)(c_m1 - 2 * c_0 + c_p1);
        double delta = 0.0;
        if (denom != 0.0)
            delta = (double)(c_m1 - c_p1) / (2.0 * denom);
        if (delta > 0.5)
            delta = 0.5;
        if (delta < -0.5)
            delta = -0.5;
        return (double)best_lag + delta;
    }
}

static int test_pitch(void)
{
    static const uint8_t notes[] = { 24, 36, 48, 60, 72, 84, 96 };
    static const int tol[] = { 25, 10, 10, 10, 10, 10, 25 };
    jet_engine_t eng;
    jet_patch_t patch;
    int32_t *mono;
    uint32_t n = FS;
    uint32_t i;

    mono = (int32_t *)malloc(n * sizeof(int32_t));
    if (!mono)
        return 1;
    jet_engine_init(&eng);
    jet_patch_defaults(&patch);
    patch.harm = 0;
    patch.split = 0;
    patch.tcomp = 127;
    engine_sync(&eng, &patch);
    for (i = 0u; i < 7u; i++) {
        double expect;
        double got;
        double cents_err;
        uint8_t note = notes[i];

        jet_engine_init(&eng);
        engine_sync(&eng, &patch);
        jet_engine_note_on(&eng, note, 100u, 0u);
        {
            int32_t l[JET_SUB];
            int32_t r[JET_SUB];
            uint32_t pos = 0u;
            while (pos < n) {
                jet_engine_render(&eng, l, r, JET_SUB);
                {
                    uint32_t k;
                    for (k = 0u; k < JET_SUB && pos < n; k++, pos++)
                        mono[pos] = (l[k] + r[k]) >> 1;
                }
            }
        }
        expect = (double)FS / (440.0 * pow(2.0, ((int)note - 69) / 12.0));
        got = estimate_period(mono + FS / 4u, FS / 2u, (uint32_t)(expect * 0.7), (uint32_t)(expect * 1.3));
        cents_err = 1200.0 * log2(got / expect);
        printf("test2 note %u expect %.2f got %.2f err %.1f cent\n", (unsigned)note, expect, got, cents_err);
        if (fabs(cents_err) > (double)tol[i]) {
            fprintf(stderr, "test2 fail note %u err %.1f cent\n", (unsigned)note, cents_err);
            free(mono);
            return 1;
        }
    }
    patch.polarity = 1;
    engine_sync(&eng, &patch);
    jet_engine_init(&eng);
    engine_sync(&eng, &patch);
    jet_engine_note_on(&eng, 60u, 100u, 0u);
    {
        int32_t l[JET_SUB];
        int32_t r[JET_SUB];
        uint32_t pos = 0u;
        while (pos < n) {
            jet_engine_render(&eng, l, r, JET_SUB);
            {
                uint32_t k;
                for (k = 0u; k < JET_SUB && pos < n; k++, pos++)
                    mono[pos] = (l[k] + r[k]) >> 1;
            }
        }
    }
    {
        double expect = (double)FS / (440.0 * pow(2.0, (60 - 69) / 12.0));
        double got = estimate_period(mono + FS / 4u, FS / 2u, (uint32_t)(expect * 0.6), (uint32_t)(expect * 1.4));
        double cents_err = 1200.0 * log2(got / expect);
        printf("test2 polarity note 60 err %.1f cent\n", cents_err);
        if (fabs(cents_err) > 25.0) {
            fprintf(stderr, "test2 polarity fail\n");
            free(mono);
            return 1;
        }
    }
    free(mono);
    return 0;
}

static int test_silence(void)
{
    jet_engine_t eng;
    jet_patch_t patch;
    int32_t *l;
    int32_t *r;
    uint32_t tail = (uint32_t)(FS * 1.5);
    double rms;
    uint8_t i;

    l = (int32_t *)malloc(tail * sizeof(int32_t));
    r = (int32_t *)malloc(tail * sizeof(int32_t));
    if (!l || !r)
        return 1;
    jet_engine_init(&eng);
    jet_patch_defaults(&patch);
    engine_sync(&eng, &patch);
    jet_engine_note_on(&eng, 60u, 100u, 0u);
    {
        int32_t dl[JET_SUB];
        int32_t dr[JET_SUB];
        uint32_t warm = FS / 2u;
        uint32_t pos = 0u;
        while (pos < warm) {
            jet_engine_render(&eng, dl, dr, JET_SUB);
            pos += JET_SUB;
        }
    }
    {
        jet_expr_t ex = { 0 };
        ex.sustain = 0u;
        jet_engine_expr(&eng, &ex);
    }
    jet_engine_note_off(&eng, 60u);
    patch.rev_mix = 0u;
    patch.dly_lvl = 0u;
    engine_sync(&eng, &patch);
    jet_fx_init(&eng.fx);
    render_block(&eng, l, r, tail);
    rms = dbfs_rms(rms_stereo(l + tail - 4096u, r + tail - 4096u, 4096u));
    printf("test3 tail RMS %.2f dBFS\n", rms);
    for (i = 0u; i < JET_NVOICE; i++) {
        if (eng.vo[i].active) {
            fprintf(stderr, "test3 fail: voice %u still active\n", (unsigned)i);
            free(l);
            free(r);
            return 1;
        }
    }
    if (rms > -70.0) {
        fprintf(stderr, "test3 fail: RMS %.2f\n", rms);
        free(l);
        free(r);
        return 1;
    }
    free(l);
    free(r);
    return 0;
}

static int test_push(void)
{
    jet_engine_t eng;
    jet_patch_t patch;
    jet_expr_t ex = { 0 };
    int32_t l[JET_SUB];
    int32_t r[JET_SUB];
    double early;
    double late;
    uint32_t i;

    jet_engine_init(&eng);
    jet_patch_defaults(&patch);
    patch.push = 0;
    patch.blow = 90;
    engine_sync(&eng, &patch);
    jet_engine_note_on(&eng, 60u, 100u, 0u);
    ex.breath = 127u;
    jet_engine_expr(&eng, &ex);
    for (i = 0u; i < FS / JET_SUB; i++)
        jet_engine_render(&eng, l, r, JET_SUB);
    patch.blow = 0u;
    engine_sync(&eng, &patch);
    ex.breath = 127u;
    jet_engine_expr(&eng, &ex);
    early = peak_abs(l, r, JET_SUB);
    for (i = 0u; i < FS / JET_SUB; i++)
        jet_engine_render(&eng, l, r, JET_SUB);
    late = peak_abs(l, r, JET_SUB);
    printf("test4 push0 blow off early=%.0f late=%.0f\n", early, late);
    if (late > early * 0.85) {
        fprintf(stderr, "test4 fail: no decay with blow off\n");
        return 1;
    }
    jet_engine_init(&eng);
    patch.push = 50;
    engine_sync(&eng, &patch);
    jet_engine_note_on(&eng, 60u, 100u, 0u);
    ex.breath = 127u;
    jet_engine_expr(&eng, &ex);
    for (i = 0u; i < (2u * FS) / JET_SUB; i++) {
        jet_engine_render(&eng, l, r, JET_SUB);
        if (l[0] > 40000 || l[0] < -40000) {
            fprintf(stderr, "test4 fail: overflow push50\n");
            return 1;
        }
    }
    printf("test4 push50 stable ok\n");
    return 0;
}

static int test_perf(void)
{
    jet_engine_t eng;
    jet_patch_t patch;
    int32_t *l;
    int32_t *r;
    uint32_t total = 10u * FS;
    uint32_t pos = 0u;
    clock_t t0;
    clock_t t1;
    double ns;

    l = (int32_t *)malloc(JET_SUB * sizeof(int32_t));
    r = (int32_t *)malloc(JET_SUB * sizeof(int32_t));
    if (!l || !r)
        return 1;
    jet_engine_init(&eng);
    jet_patch_defaults(&patch);
    engine_sync(&eng, &patch);
    jet_engine_note_on(&eng, 48u, 100u, 0u);
    jet_engine_note_on(&eng, 55u, 90u, 0u);
    jet_engine_note_on(&eng, 60u, 100u, 0u);
    jet_engine_note_on(&eng, 67u, 90u, 0u);
    t0 = clock();
    while (pos < total) {
        jet_engine_render(&eng, l, r, JET_SUB);
        pos += JET_SUB;
    }
    t1 = clock();
    ns = ((double)(t1 - t0) / (double)CLOCKS_PER_SEC) * 1e9 / (double)total;
    printf("test5 ns/sample %.1f\n", ns);
    free(l);
    free(r);
    return 0;
}

static int test_plates(void)
{
    static const uint8_t notes[] = { 48u, 60u, 72u };
    jet_engine_t eng;
    jet_patch_t patch;
    int32_t *l;
    int32_t *r;
    uint8_t pl;
    int fails = 0;
    int ok = 0;

    l = (int32_t *)malloc(FS * sizeof(int32_t));
    r = (int32_t *)malloc(FS * sizeof(int32_t));
    if (!l || !r)
        return 1;

    for (pl = 0u; pl < (uint8_t)JET_PLATE_N; pl++) {
        uint8_t era = (uint8_t)(pl / JET_PLATES_PER_ERA);
        uint8_t idx = (uint8_t)(pl % JET_PLATES_PER_ERA);
        uint32_t n;

        jet_plate_apply(&patch, era, idx);
        for (n = 0u; n < 3u; n++) {
            double pk;
            double rms;
            double pk_db;
            double rms_db;
            uint8_t note = notes[n];

            jet_engine_init(&eng);
            engine_sync(&eng, &patch);
            jet_engine_note_on(&eng, note, 100u, 0u);
            render_block(&eng, l, r, FS);
            pk = peak_abs(l, r, FS);
            rms = rms_stereo(l, r, FS);
            pk_db = dbfs_peak(pk);
            rms_db = dbfs_rms(rms);
            if (pk_db >= 0.0 || rms_db <= -40.0) {
                fprintf(stderr, "test6 fail plate %u (%s) note %u peak %.2f rms %.2f dBFS\n",
                    (unsigned)pl, jet_plate_name(pl), (unsigned)note, pk_db, rms_db);
                fails++;
            } else
                ok++;
        }
    }
    printf("test6 plates: %d ok, %d fail (of %u checks)\n", ok, fails,
        (unsigned)(JET_PLATE_N * 3u));
    free(l);
    free(r);
    return fails ? 1 : 0;
}

/* Normalized autocorrelation at the expected period: a self-oscillating pipe sits near 1.0, noise through a comb ~0.5. */
static int test_tonal(void)
{
    static double x[FS / 2u];
    static const uint8_t eras[] = { JET_ERA_FLUE, JET_ERA_REED, JET_ERA_ORGAN, JET_ERA_BRASS };
    uint32_t ei;

    for (ei = 0u; ei < sizeof(eras); ei++) {
        jet_engine_t eng;
        jet_patch_t patch;
        int32_t l[JET_SUB];
        int32_t r[JET_SUB];
        uint32_t pos = 0u;
        uint32_t i;
        uint32_t a = FS / 4u;
        uint32_t n = FS / 8u;
        double per = (double)FS / (440.0 * pow(2.0, (60 - 69) / 12.0));
        double best = 0.0;
        double e0 = 0.0;
        int lag;

        jet_engine_init(&eng);
        jet_patch_defaults(&patch);
        jet_plate_apply(&patch, eras[ei], 0u);
        engine_sync(&eng, &patch);
        jet_engine_note_on(&eng, 60u, 100u, 0u);
        while (pos < FS / 2u) {
            jet_engine_render(&eng, l, r, JET_SUB);
            for (i = 0u; i < JET_SUB && pos < FS / 2u; i++, pos++)
                x[pos] = (l[i] + r[i]) * 0.5;
        }
        for (i = a; i < a + n; i++)
            e0 += x[i] * x[i];
        for (lag = (int)(per * 0.97); lag <= (int)(per * 1.03) + 1; lag++) {
            double c = 0.0;
            double e1 = 0.0;

            for (i = a; i < a + n; i++) {
                c += x[i] * x[i + (uint32_t)lag];
                e1 += x[i + (uint32_t)lag] * x[i + (uint32_t)lag];
            }
            c /= sqrt(e0 * e1 + 1e-9);
            if (c > best)
                best = c;
        }
        printf("test7 era %u clarity %.2f\n", (unsigned)eras[ei], best);
        if (best < 0.9) {
            fprintf(stderr, "test7 fail: era %u not tonal (%.2f)\n", (unsigned)eras[ei], best);
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    if (test_level())
        return 1;
    if (test_pitch())
        return 1;
    if (test_silence())
        return 1;
    if (test_push())
        return 1;
    if (test_perf())
        return 1;
    if (test_plates())
        return 1;
    if (test_tonal())
        return 1;
    printf("jet_dsp_test ok\n");
    return 0;
}
