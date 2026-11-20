/* SPDX-License-Identifier: GPL-3.0-only */
/* KO BASS extras for tests/plate_gen_drift.c: phrase rules, landmark phrases + fader lines, phrase golden,
 * phrase determinism / drift / cross-era / fader axis. */

static int bass_ph_bad_note(const bass_phrase_t *ph)
{
    int bad = 0;
    uint8_t st;

    for (st = 0u; st < 128u; st++) {
        int8_t p = bass_phrase_pitch(ph, st);

        bad += p < -5 || p > 12;
    }
    for (st = 0u; st < 128u; st++) {
        uint8_t pos = st & 15u, on = bass_phrase_gate(ph, st), len = bass_phrase_len(ph, st);
        uint8_t bend = bass_phrase_bend(ph, st);

        bad += (len != 0u) != (on != 0u);
        bad += len != 0u && len != 4u && len != 9u && len != 10u && len != 11u;
        bad += len > 4u && pos != 7u && pos != 14u;
        bad += bend && pos != 7u && pos != 15u;
        bad += bend && pos == 15u && st >= 112u;
        bad += bend && pos == 7u && bend != 4u && bend != 5u;
        bad += bend && pos == 15u && bend != 5u && bend != 6u;
    }
    return bad;
}

static int pgen_rules_check(uint8_t era, int32_t n)
{
    const bass_phrase_t *ph = &bass_gen_ph;
    int bad = 0;
    uint8_t b;

    (void)era;
    bad += ph->rom != 0u || ph->n != n;
    for (b = 0u; b < 8u; b++) {
        uint16_t m = ph->bar[b];
        uint8_t h = bass_gen_popc16(m);

        bad += (m & BGEN_BAR_MUST) != BGEN_BAR_MUST || (m & BGEN_BAR_BAN) != 0u;
        bad += h < 1u || h > BGEN_BAR_HITS_MAX;
    }
    bad += G.pl.sub != BGEN_M(ph->bar[0], ph->bar[1]);
    bad += bass_ph_bad_note(ph);
    return bad;
}

static int bass_ph_rom_diff(const bass_phrase_t *ph, uint8_t slot)
{
    int bad = 0;
    uint8_t st;

    for (st = 0u; st < 8u; st++)
        bad += ph->bar[st] != BASS_LINE_BAR_GATE[slot][st];
    for (st = 0u; st < 128u; st++)
        bad += bass_phrase_pitch(ph, st) != BASS_LINE_LEAD_PITCH[slot][st];
    for (st = 0u; st < 128u; st++) {
        bad += bass_phrase_len(ph, st) != BASS_LINE_LEAD_LEN[slot][st];
        bad += bass_phrase_bend(ph, st) != BASS_LINE_LEAD_BEND[slot][st];
    }
    return bad;
}

/* Landmark plate: its phrase and every genre-fader line of the era are the curated lines. */
static int bass_landmark_extra(uint8_t era, uint8_t slot)
{
    int bad = bass_ph_rom_diff(&bass_gen_ph, slot);
    uint8_t l, base = (uint8_t)(era * GENRE_PRESETS_PER_ERA);

    for (l = 0u; l < GENRE_PRESETS_PER_ERA; l++) {
        bass_phrase_t ph;

        bass_gen_line_phrase(&ph, era, (uint8_t)(base + l));
        bad += bass_ph_rom_diff(&ph, (uint8_t)(base + l));
        bad += strcmp(bass_gen_line_name(era, (uint8_t)(base + l)), BASS_LINE[base + l].name) != 0;
    }
    if (bad)
        printf("FAIL bass phrase landmark slot %u (%d diffs)\n", slot, bad);
    return bad;
}
#define PGEN_LANDMARK_EXTRA(era, slot) bass_landmark_extra(era, slot)

static uint32_t pgen_golden_extra(uint32_t h)
{
    uint8_t st;

#define BMIX(v)                    \
    do {                           \
        h ^= (uint8_t)(v);         \
        h *= 16777619u;            \
    } while (0)
    for (st = 0u; st < 8u; st++) {
        BMIX(bass_gen_ph.bar[st]);
        BMIX(bass_gen_ph.bar[st] >> 8);
    }
    for (st = 0u; st < 16u; st++)
        BMIX(bass_phrase_pitch(&bass_gen_ph, st));
    for (st = 0u; st < 128u; st++) {
        BMIX(bass_phrase_len(&bass_gen_ph, st));
        BMIX(bass_phrase_bend(&bass_gen_ph, st));
    }
#undef BMIX
    return h;
}
#define PGEN_GOLDEN_EXTRA 1

static int bass_ph_dist(const bass_phrase_t *a, const bass_phrase_t *b)
{
    int d = 0;
    uint8_t i;

    for (i = 0u; i < 8u; i++)
        d += popc((uint32_t)(a->bar[i] ^ b->bar[i]) & (uint32_t)(uint16_t)~(BGEN_BAR_MUST | BGEN_BAR_BAN));
    return d;
}

static int pgen_extra_checks(void)
{
    int fails = 0, det = 0, axis = 0, flips = 0, steps = 0, maxf = 0;
    long same = 0, unrel = 0, pairs = 0;
    uint8_t era, eb, l;
    int32_t n;

    for (era = 0u; era < BASS_ERA_N; era++)
        for (n = 1000; n < 1200; n++) {
            bass_phrase_t a, b;
            int f;

            bass_phrase_build(&a, era, n, FM_PGEN_COMPLY_DEF);
            bass_phrase_build(&b, era, n, FM_PGEN_COMPLY_DEF);
            det += memcmp(&a, &b, sizeof a) != 0;
            bass_phrase_build(&b, era, n + 1, FM_PGEN_COMPLY_DEF);
            f = bass_ph_dist(&a, &b);
            flips += f;
            maxf = f > maxf ? f : maxf;
            steps++;
            for (eb = 0u; eb < BASS_ERA_N; eb++) {
                bass_phrase_t c;

                if (eb == era)
                    continue;
                bass_phrase_build(&b, eb, n, FM_PGEN_COMPLY_DEF);
                bass_phrase_build(&c, era, n + FM_PGEN_JUMP, FM_PGEN_COMPLY_DEF);
                same += bass_ph_dist(&a, &b);
                unrel += bass_ph_dist(&a, &c);
                pairs++;
            }
        }
    printf("bass phrase: flips/click avg %.1f max %d (of 88 free bits), determinism fails %d\n",
           (double)flips / steps, maxf, det);
    printf("bass phrase cross-era same n: %.1f free bits, unrelated n: %.1f (%.0f%%)\n", (double)same / pairs,
           (double)unrel / pairs, 100.0 * same / unrel);
    fails += det != 0;
    fails += same * 10 >= unrel * 6;

    /* Fader axis: the fader lines are the ten plates of the plate's 80-click block (same offset to their
     * landmark slot); line k is the plate itself, and moving the plate by GAP inside the block keeps all lines. */
    for (era = 0u; era < BASS_ERA_N; era++)
        for (n = 1001; n < 1100; n += 7) {
            bass_phrase_t lines[GENRE_PRESETS_PER_ERA], ph;
            uint8_t base = (uint8_t)(era * GENRE_PRESETS_PER_ERA), k;

            FM_PGEN_BUILD(era, n, FM_PGEN_COMPLY_DEF);
            k = (uint8_t)(bass_gen_ph.slot % GENRE_PRESETS_PER_ERA);
            for (l = 0u; l < GENRE_PRESETS_PER_ERA; l++)
                bass_gen_line_phrase(&lines[l], era, (uint8_t)(base + l));
            axis += memcmp(&lines[k], &bass_gen_ph, sizeof ph) != 0;
            if (k + 1u == GENRE_PRESETS_PER_ERA)
                continue;
            FM_PGEN_BUILD(era, n + FM_PGEN_GAP, FM_PGEN_COMPLY_DEF);
            for (l = 0u; l < GENRE_PRESETS_PER_ERA; l++) {
                bass_gen_line_phrase(&ph, era, (uint8_t)(base + l));
                axis += memcmp(&ph, &lines[l], sizeof ph) != 0;
            }
        }
    printf("bass fader axis: %d mismatches\n", axis);
    fails += axis != 0;
    fm_pgen_off(&bass_gen);
    return fails;
}
#define PGEN_EXTRA_CHECKS 1
