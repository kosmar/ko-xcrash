/* SPDX-License-Identifier: GPL-3.0-only */
/* KO JET settings store: the length settings_save_jet writes must equal the length settings_load_jet asks for
 * (platform settings_load rejects a length mismatch; tests/host.c does not check it), plus a boot round trip.
 * Built twice by jet_host_smoke.sh: default (SETTINGS_MAX) and -DJET_SETTINGS_N=4080u (single-sector limit). */
#define main jet_host_main
#include "jet.c"
#undef main

#include <stdio.h>
#include <string.h>

static int fails;

#define CHECK(c, msg)                                   \
    do {                                                \
        if (!(c)) {                                     \
            printf("jet_store_test: FAIL %s\n", msg);   \
            fails++;                                    \
        }                                               \
    } while (0)

static uint32_t saved_len(void)
{
    uint32_t n = SETTINGS_MAX;

    while (n > 0u && host_store[4u + n - 1u] == 0xA5u)
        n--;
    return n;
}

static uint32_t rng = 0x1234567u;

static uint8_t rnd(void)
{
    rng = rng * 1103515245u + 12345u;
    return (uint8_t)(rng >> 16);
}

static void store_reset(void)
{
    memset(host_store, 0, sizeof host_store);
    memset(host_store + 4, 0xA5, SETTINGS_MAX);
}

static uint32_t saved_magic(void)
{
    deck_store_hdr_t h;

    memcpy(&h, host_store + 4, sizeof h);
    return h.magic;
}

static jet_cfg_t cfg_ref;
static jet_plate_ov_t ov_ref[JET_PLATE_N];

int main(void)
{
    deck_store_seg_t seg[7];
    uint32_t nseg, raw, n, i;
    deck_store_hdr_t h;

    printf("jet_store_test: JET_SETTINGS_N=%u SETTINGS_MAX=%u\n", (uint32_t)JET_SETTINGS_N, (uint32_t)SETTINGS_MAX);

    /* worst case: every plate override used, noisy overrides + seq bytes */
    cfg_defaults();
    for (i = 0u; i < sizeof jet_plate_ov; i++)
        ((uint8_t *)jet_plate_ov)[i] = rnd();
    for (i = 0u; i < JET_PLATE_N; i++)
        jet_plate_ov[i].used = 1u;
    for (i = 0u; i < sizeof g.cfg.seq; i++)
        ((uint8_t *)&g.cfg.seq)[i] = rnd();
#if JET_SETTINGS_N >= SETTINGS_MAX
    for (i = 0u; i < sizeof g.cfg.patch; i++)
        ((uint8_t *)&g.cfg.patch)[i] = rnd();
#endif
    nseg = jet_store_segs(seg);
    raw = deck_store_raw_len(seg, nseg);
    store_reset();
    CHECK(settings_save_jet() == 0, "worst-case save");
    n = saved_len();
    memcpy(&h, host_store + 4, sizeof h);
    printf("jet_store_test: raw=%u worst comp=%u saved=%u load=%u magic=%s\n", raw, h.comp_len + (uint32_t)sizeof h,
           n, (uint32_t)sizeof jet_store_buf, h.magic == JET_STORE_MAGIC_NOOV ? "NOOV" : "FULL");
    CHECK(n == sizeof jet_store_buf, "saved length == load length");
    CHECK(n == JET_SETTINGS_N, "saved length == JET_SETTINGS_N");
#if JET_SETTINGS_N >= SETTINGS_MAX
    CHECK(h.magic == JET_STORE_MAGIC, "full size: overrides persisted");
#else
    CHECK(h.magic == JET_STORE_MAGIC_NOOV, "worst case falls back to NOOV");
    /* NOOV load: cfg/seq restored exactly, plate overrides cleared */
    memcpy(&cfg_ref, &g.cfg, sizeof cfg_ref);
    jet_seq_sanitize(&cfg_ref.seq); /* the load brings noisy step bytes back into range */
    memset(&g.cfg, 0x5A, sizeof g.cfg);
    CHECK(settings_load_jet() == 0, "NOOV load");
    CHECK(memcmp(&g.cfg.seq, &cfg_ref.seq, sizeof cfg_ref.seq) == 0, "NOOV seq restored");
    CHECK(memcmp(&g.cfg.meta, &cfg_ref.meta, sizeof cfg_ref.meta) == 0, "NOOV meta restored");
    CHECK(memcmp(&g.cfg.groove, &cfg_ref.groove, sizeof cfg_ref.groove) == 0, "NOOV groove restored");
    CHECK(memcmp(&g.cfg.patch, &cfg_ref.patch, sizeof cfg_ref.patch) == 0, "NOOV patch restored");
    CHECK(memcmp(g.cfg.lfo, cfg_ref.lfo, sizeof cfg_ref.lfo) == 0, "NOOV lfo restored");
    CHECK(memcmp(&g.cfg.rec, &cfg_ref.rec, sizeof cfg_ref.rec) == 0, "NOOV rec restored");
    for (i = 0u; i < JET_PLATE_N; i++)
        if (jet_plate_ov[i].used)
            break;
    CHECK(i == JET_PLATE_N, "NOOV overrides cleared");
#endif

#if JET_SETTINGS_N >= SETTINGS_MAX
    /* full size: worst-case overrides round trip */
    memcpy(&cfg_ref, &g.cfg, sizeof cfg_ref);
    jet_seq_sanitize(&cfg_ref.seq); /* the load brings noisy step bytes back into range */
    memcpy(ov_ref, jet_plate_ov, sizeof ov_ref);
    memset(jet_plate_ov, 0, sizeof jet_plate_ov);
    CHECK(settings_load_jet() == 0, "worst-case load");
    CHECK(memcmp(jet_plate_ov, ov_ref, sizeof ov_ref) == 0, "worst-case overrides restored");
    CHECK(memcmp(&g.cfg.seq, &cfg_ref.seq, sizeof cfg_ref.seq) == 0, "worst-case seq restored");
#endif

    /* typical: a few overrides + some seq survive save/load incl. overrides */
    memset(jet_plate_ov, 0, sizeof jet_plate_ov);
    cfg_defaults();
    for (i = 0u; i < 64u; i++)
        ((uint8_t *)&g.cfg.seq)[i * 7u] = rnd();
    for (i = 0u; i < 4u; i++) {
        jet_plate_ov[i * 9u].used = 1u;
        jet_plate_ov[i * 9u].patch = g.cfg.patch;
        jet_plate_ov[i * 9u].bpm = (uint8_t)(90u + i);
    }
    store_reset();
    CHECK(settings_save_jet() == 0, "typical save");
    CHECK(saved_magic() == JET_STORE_MAGIC, "typical keeps overrides");
    CHECK(saved_len() == JET_SETTINGS_N, "typical saved length");
    memcpy(&cfg_ref, &g.cfg, sizeof cfg_ref);
    jet_seq_sanitize(&cfg_ref.seq); /* the load brings noisy step bytes back into range */
    memcpy(ov_ref, jet_plate_ov, sizeof ov_ref);
    memset(jet_plate_ov, 0, sizeof jet_plate_ov);
    memset(&g.cfg, 0x5A, sizeof g.cfg);
    CHECK(settings_load_jet() == 0, "typical load");
    CHECK(memcmp(&g.cfg.seq, &cfg_ref.seq, sizeof cfg_ref.seq) == 0, "typical seq restored");
    CHECK(memcmp(jet_plate_ov, ov_ref, sizeof ov_ref) == 0, "typical overrides restored");

    /* round trip through app_init: loaded patch/groove/plate survive boot (no factory plate on top) */
    memset(jet_plate_ov, 0, sizeof jet_plate_ov);
    cfg_defaults();
    g.era = 1u;
    g.plate = 17u;
    g.cfg.patch.lpf = 33u;
    g.cfg.patch.blow = 99u;
    g.cfg.patch.mode = 1u;
    g.cfg.groove.bpm = 133u;
    g.cfg.groove.swing = 61u;
    jet_plate_ov[17].used = 1u;
    jet_plate_ov[17].patch = g.cfg.patch;
    jet_plate_ov[17].bpm = 77u;
    store_reset();
    CHECK(settings_save_jet() == 0, "save");
    CHECK(saved_len() == sizeof jet_store_buf, "round-trip saved length");
    memset(&g, 0, sizeof g);
    memset(jet_plate_ov, 0, sizeof jet_plate_ov);
    app_init();
    printf("jet_store_test: boot era=%u plate=%u lpf=%u blow=%u mode=%u eng.lpf=%u bpm=%u swing=%u ov17=%u\n", g.era,
           g.plate, g.cfg.patch.lpf, g.cfg.patch.blow, g.cfg.patch.mode, g.eng.patch.lpf, g.cfg.groove.bpm,
           g.cfg.groove.swing, jet_plate_ov[17].used);
    CHECK(g.era == 1u && g.plate == 17u, "era/plate restored");
    CHECK(g.cfg.patch.lpf == 33u && g.cfg.patch.blow == 99u, "patch restored, not factory plate");
    CHECK(g.eng.mode == 1u, "engine mode from loaded patch");
    CHECK(g.cfg.groove.bpm == 133u && g.cfg.groove.swing == 61u, "groove restored");
    CHECK(jet_plate_ov[17].used && jet_plate_ov[17].bpm == 77u, "plate override restored");
    if (fails)
        return 1;
    printf("jet_store_test: OK\n");
    return 0;
}
