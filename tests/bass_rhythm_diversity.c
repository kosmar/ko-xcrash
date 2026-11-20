/* SPDX-License-Identifier: GPL-3.0-only */
/* KO BASS rhythm diversity dump: landmark bar gates, endless phrase gates per era, and the gates the seq bake plays
 * when the PRESET knob walks plates (fader fixed vs. fader on the plate's line). Not a pass/fail test. */
#include "bass_data.h"
#include <stdio.h>
#include <string.h>

#define MAXP 4096

typedef struct {
    uint16_t v[MAXP];
    int n;
} set16_t;

typedef struct {
    uint16_t v[MAXP][8];
    int n;
} set128_t;

static void s16_add(set16_t *s, uint16_t m)
{
    int i;

    for (i = 0; i < s->n; i++)
        if (s->v[i] == m)
            return;
    if (s->n < MAXP)
        s->v[s->n++] = m;
}

static void s128_add(set128_t *s, const uint16_t *bar)
{
    int i;

    for (i = 0; i < s->n; i++)
        if (!memcmp(s->v[i], bar, 16))
            return;
    if (s->n < MAXP)
        memcpy(s->v[s->n++], bar, 16);
}

static void hist_print(const char *tag, const long *h, long bars)
{
    int s;

    printf("%-14s", tag);
    for (s = 0; s < 16; s++)
        printf(" %3ld", bars ? (h[s] * 100 + bars / 2) / bars : 0);
    printf("  (%% of bars, steps 0..15)\n");
}

static void hist_add(long *h, const uint16_t *bar)
{
    int b, s;

    for (b = 0; b < 8; b++)
        for (s = 0; s < 16; s++)
            h[s] += (bar[b] >> s) & 1u;
}

static set16_t bars;
static set128_t phr;

/* Bar gates the seq bake plays for live plate (era, n) at genre fader f (bass_genre_fader_bake line pick). */
static void bake_bars(uint8_t era, uint8_t f, uint16_t *out)
{
    uint32_t pos = ((uint32_t)f * (GENRE_PRESETS_PER_ERA - 1u) * 256u) / 127u;
    uint8_t base = bass_era_first_line(era), lo = (uint8_t)(base + (pos >> 8)), hi = (uint8_t)(lo + 1u);
    bass_phrase_t ph;

    if (hi >= base + GENRE_PRESETS_PER_ERA)
        hi = (uint8_t)(base + GENRE_PRESETS_PER_ERA - 1u);
    bass_gen_line_phrase(&ph, era, bass_bake_pick_line(lo, hi, (uint8_t)(pos & 255u)));
    memcpy(out, ph.bar, 16);
}

int main(void)
{
    uint8_t era, b;
    int32_t n;
    long h[16];
    int mode;

    /* Landmarks */
    memset(&bars, 0, sizeof bars);
    memset(&phr, 0, sizeof phr);
    memset(h, 0, sizeof h);
    for (b = 0; b < BASS_ERA_N * GENRE_PRESETS_PER_ERA; b++) {
        uint8_t i;

        for (i = 0; i < 8u; i++)
            s16_add(&bars, BASS_LINE_BAR_GATE[b][i]);
        s128_add(&phr, BASS_LINE_BAR_GATE[b]);
        hist_add(h, BASS_LINE_BAR_GATE[b]);
    }
    printf("landmarks: %d lines, distinct bars %d, distinct 8-bar phrases %d\n", BASS_ERA_N * GENRE_PRESETS_PER_ERA,
           bars.n, phr.n);
    hist_print("landmark", h, (long)BASS_ERA_N * GENRE_PRESETS_PER_ERA * 8);

    /* Endless phrases (generator only), 50 n per era away from landmarks */
    for (era = 0; era < BASS_ERA_N; era++) {
        char tag[16];

        memset(&bars, 0, sizeof bars);
        memset(&phr, 0, sizeof phr);
        memset(h, 0, sizeof h);
        for (n = 0; n < 50; n++) {
            bass_phrase_t ph;
            uint8_t i;

            bass_phrase_build(&ph, era, 1000 + n * 37, FM_PGEN_COMPLY_DEF);
            for (i = 0; i < 8u; i++)
                s16_add(&bars, ph.bar[i]);
            s128_add(&phr, ph.bar);
            hist_add(h, ph.bar);
        }
        printf("gen era %u: 50 plates, distinct bars %d, distinct phrases %d\n", era, bars.n, phr.n);
        snprintf(tag, sizeof tag, "gen era %u", era);
        hist_print(tag, h, 400);
    }

    /* What the seq plays when PRESET walks 80 consecutive plates: fader fixed at 64 vs. fader on the plate line */
    for (mode = 0; mode < 2; mode++)
        for (era = 0; era < BASS_ERA_N; era++) {
            memset(&bars, 0, sizeof bars);
            memset(&phr, 0, sizeof phr);
            for (n = 0; n < 80; n++) {
                uint16_t bb[8];
                uint8_t i, f;

                FM_PGEN_BUILD(era, 4000 + n, FM_PGEN_COMPLY_DEF);
                if (mode) {
                    uint16_t own[8];

                    bake_bars(era, bass_genre_fader_of_line((uint8_t)(bass_gen_ph.slot % GENRE_PRESETS_PER_ERA)), own);
                    if (memcmp(own, bass_gen_ph.bar, 16))
                        printf("MISMATCH era %u n %d: fader on plate line does not bake the plate\n", era, 4000 + n);
                }
                f = mode ? bass_genre_fader_of_line((uint8_t)(bass_gen_ph.slot % GENRE_PRESETS_PER_ERA)) : 64u;
                bake_bars(era, f, bb);
                for (i = 0; i < 8u; i++)
                    s16_add(&bars, bb[i]);
                s128_add(&phr, bb);
            }
            printf("seq bake %s era %u: 80 plates -> distinct phrases %d, distinct bars %d\n",
                   mode ? "fader=plate" : "fader=64  ", era, phr.n, bars.n);
        }
    fm_pgen_off(&bass_gen);
    return 0;
}
