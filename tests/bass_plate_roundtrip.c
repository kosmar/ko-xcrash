/* SPDX-License-Identifier: GPL-3.0-only */
/* KO BASS user plates: dub_params_t -> bass_plate_t (dub_persist_save) -> factory base + bass_plate_t
 * (dub_persist_unpack) must give back every stored field; unstored fields come from the factory
 * plate. Every unstored field is also perturbed on the live params while playing: if the audio
 * changes, KO BASS reads it and a user plate would lose it -> FAIL.
 * Build: cc -DDINGHY_HOST -DGENRE_BASS -Ifirmware/src -Ifirmware/gen tests/bass_plate_roundtrip.c */
#include <stddef.h>
#define main bass_plate_host_main
#include "fm_app.c"
#undef main

static int fail;

static void check(int c, const char *m, const char *f)
{
    if (!c) {
        fprintf(stderr, "bass_plate_roundtrip: FAIL %s %s\n", m, f);
        fail = 1;
    }
}

typedef struct {
    const char *name;
    size_t off;
} field_t;

#define FLD(f) {#f, offsetof(dub_params_t, f)},
#define ARR4(f) {#f "[0]", offsetof(dub_params_t, f) + 0u}, {#f "[1]", offsetof(dub_params_t, f) + 1u}, \
                {#f "[2]", offsetof(dub_params_t, f) + 2u}, {#f "[3]", offsetof(dub_params_t, f) + 3u},

static const field_t STORED[] = {
    BASS_PLATE_FIELDS(FLD)
    {"pan[0]", offsetof(dub_params_t, pan)},
};

static const field_t UNSTORED[] = {
    FLD(timbre) FLD(cutoff_c) FLD(decay_c) FLD(dly_c) FLD(voicing) FLD(rev_c) FLD(rim_tone)
    FLD(siren_pitch) FLD(siren_rate) FLD(kick_boom) FLD(hat_lvl) FLD(drum_kit) FLD(lvl_c) FLD(atk_c)
    FLD(glide_c)
    ARR4(drm_dly) ARR4(drm_rev) ARR4(drm_tune) ARR4(drm_dec) ARR4(drm_drv) ARR4(drm_pan) ARR4(drm_cut)
    ARR4(drm_atk) ARR4(drm_dens)
    {"pan[1]", offsetof(dub_params_t, pan) + 1u}, {"pan[2]", offsetof(dub_params_t, pan) + 2u},
    {"pan[3]", offsetof(dub_params_t, pan) + 3u},
};

#define NSTORED   (sizeof STORED / sizeof STORED[0])
#define NUNSTORED (sizeof UNSTORED / sizeof UNSTORED[0])

static uint8_t *fld(dub_params_t *p, size_t off) { return (uint8_t *)p + off; }

static void roundtrip(uint8_t plate)
{
    dub_params_t factory, src;
    const dub_params_t *got;
    uint32_t k;
    char what[64];

    memset(&factory, 0, sizeof factory);
    genre_preset_rebuild(&factory, genre_era_of_preset(plate), plate);
    src = factory;
    for (k = 0; k < NSTORED; k++) {
        uint8_t *v = fld(&src, STORED[k].off);
        uint8_t nv = (uint8_t)((*v + 23u + k * 7u) & 127u);

        *v = nv == *v ? (uint8_t)((nv + 1u) & 127u) : nv;
    }
    for (k = 0; k < NUNSTORED; k++) {
        uint8_t *v = fld(&src, UNSTORED[k].off);

        *v = (uint8_t)((*v + 51u) & 127u);
    }
    memset(g.user_plates, 0, sizeof g.user_plates);
    g.user_plate_mask = 1ull << plate;
    g.user_plates[plate] = src;
    dub_persist_save();
    memset(g.user_plates, 0xA5, sizeof g.user_plates);
    g.user_plate_mask = 0u;
    dub_persist_unpack();
    got = &g.user_plates[plate];
    snprintf(what, sizeof what, "(plate %u)", plate);
    check(g.user_plate_mask == (1ull << plate), "mask lost", what);
    for (k = 0; k < NSTORED; k++) {
        if (*fld((dub_params_t *)got, STORED[k].off) != *fld(&src, STORED[k].off)) {
            snprintf(what, sizeof what, "%s (plate %u)", STORED[k].name, plate);
            check(0, "stored field not restored:", what);
        }
    }
    for (k = 0; k < NUNSTORED; k++) {
        if (*fld((dub_params_t *)got, UNSTORED[k].off) != *fld(&factory, UNSTORED[k].off)) {
            snprintf(what, sizeof what, "%s (plate %u)", UNSTORED[k].name, plate);
            check(0, "unstored field not taken from factory plate:", what);
        }
    }
}

#define RENDER_BLOCKS 400u /* ~2.3 s */
static int16_t ref_pcm[RENDER_BLOCKS * 512u];
static int16_t cur_pcm[RENDER_BLOCKS * 512u];
static dub_app_t snap;
static int16_t snap_dly[DUB_DLY_LEN];
static int16_t snap_rev[DUB_REV_LEN];

static void snap_take(void)
{
    snap = g;
    memcpy(snap_dly, dub_dly_pool, sizeof snap_dly);
    memcpy(snap_rev, dub_rev_pool, sizeof snap_rev);
}

static void snap_restore(void)
{
    g = snap;
    memcpy(dub_dly_pool, snap_dly, sizeof snap_dly);
    memcpy(dub_rev_pool, snap_rev, sizeof snap_rev);
}

static void render(int16_t *dst)
{
    static int32_t blk[512];
    uint32_t b, i;

    for (b = 0; b < RENDER_BLOCKS; b++) {
        memset(blk, 0, sizeof blk);
        app_audio(blk, 256u);
        for (i = 0; i < 512u; i++)
            dst[b * 512u + i] = (int16_t)(blk[i] > 32767 ? 32767 : blk[i] < -32767 ? -32767 : blk[i]);
    }
}

static void read_check(void)
{
    uint32_t k, lost = 0u;

    g.cfg.bass.clock_mode = BASS_CLOCK_INT;
    g.ext_clock = 0u;
    g.playing = 1u;
    transport_reset();
    snap_take();
    render(ref_pcm);
    snap_restore();
    render(cur_pcm);
    if (memcmp(ref_pcm, cur_pcm, sizeof ref_pcm)) {
        fprintf(stderr, "bass_plate_roundtrip: skip read check (render not deterministic)\n");
        return;
    }
    snap_restore();
    g.cfg.p.cutoff_b = (uint8_t)(g.cfg.p.cutoff_b >= 64u ? 0u : 127u);
    render(cur_pcm);
    check(memcmp(ref_pcm, cur_pcm, sizeof ref_pcm) != 0, "read check insensitive:", "cutoff_b change not audible");
    for (k = 0; k < NUNSTORED; k++) {
        uint8_t *v;

        snap_restore();
        v = fld(&g.cfg.p, UNSTORED[k].off);
        *v = (uint8_t)(*v >= 64u ? 0u : 127u);
        render(cur_pcm);
        if (memcmp(ref_pcm, cur_pcm, sizeof ref_pcm)) {
            check(0, "unstored field is read by KO BASS (lost in user plates):", UNSTORED[k].name);
            lost++;
        }
    }
    fprintf(stderr, "bass_plate_roundtrip: %u unstored fields audio-checked, %u read\n", (unsigned)NUNSTORED,
            lost);
}

int main(void)
{
    if (!freopen("/dev/null", "w", stdout))
        return 1;
    check(NSTORED + NUNSTORED == sizeof(dub_params_t), "field lists do not cover dub_params_t",
          "(new field? add it to BASS_PLATE_FIELDS or UNSTORED)");
    check(sizeof(bass_plate_t) == NSTORED, "bass_plate_t has padding/extra bytes", "");
    app_init();
    roundtrip(0u);
    roundtrip(17u);
    roundtrip((uint8_t)(GENRE_CFG_USER_PLATE_N - 1u));
    read_check();
    fprintf(stderr, "bass_plate_roundtrip: %u stored + %u unstored bytes, sizeof(bass_plate_t)=%u\n",
            (unsigned)NSTORED, (unsigned)NUNSTORED, (unsigned)sizeof(bass_plate_t));
    if (fail)
        return 1;
    fprintf(stderr, "bass_plate_roundtrip: OK\n");
    return 0;
}
