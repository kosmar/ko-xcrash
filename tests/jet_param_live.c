/* SPDX-License-Identifier: GPL-3.0-only */
/* KO JET: no dead knobs. Every patch param must change the rendered audio between its lowest and
 * highest value. */
#include "jet_dsp.h"
#include <stdio.h>

#define N_RENDER (44100u * 8u)

static jet_engine_t eng;

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

static void base_patch(jet_patch_t *p)
{
    jet_patch_defaults(p);
    p->mode = JET_MODE_UNISON; /* DETUNE and GLIDE only act here / on legato notes */
    p->glide = 40u;
    p->sat = 90u;
    p->sat_shape = 60u;
    p->sat_sym = 40u;
    p->split = 40u;
    p->harm = 30u;
    p->dly_lvl = 80u;
    p->dly_fb = 70u;
    p->dly_rev = 50u;
    p->rev_mix = 70u;
    p->wow = 50u;
    p->s_dly = 10u;
    p->s_hold = 10u;
    p->x_breath_blow = 100u;
    p->x_mw_lpf = 30u;
    p->x_mw_vib = 100u;
    p->x_at_push = 100u;
}

static uint32_t render_hash(const jet_patch_t *p, int32_t *peak)
{
    static int32_t l[JET_SUB], r[JET_SUB];
    jet_expr_t x = {0};
    uint32_t h = 2166136261u;
    uint32_t n;
    uint32_t i;

    jet_engine_init(&eng);
    engine_sync(&eng, p);
    x.breath = 60u;
    x.mw = 80u;
    x.at = 80u;
    jet_engine_expr(&eng, &x);
    *peak = 0;
    for (n = 0u; n < N_RENDER; n += JET_SUB) {
        if (n == 0u)
            jet_engine_note_on(&eng, 55u, 80u, 0u);
        if (n == (N_RENDER / 4u) / JET_SUB * JET_SUB)
            jet_engine_note_on(&eng, 62u, 80u, 1u);
        if (n == (N_RENDER / 2u) / JET_SUB * JET_SUB) {
            jet_engine_note_off(&eng, 55u);
            jet_engine_note_off(&eng, 62u);
        }
        jet_engine_render(&eng, l, r, JET_SUB);
        for (i = 0u; i < JET_SUB; i++) {
            int32_t a = l[i] < 0 ? -l[i] : l[i];

            if (a > *peak)
                *peak = a;
            h = (h ^ (uint32_t)l[i]) * 16777619u;
            h = (h ^ (uint32_t)r[i]) * 16777619u;
        }
    }
    return h;
}

int main(void)
{
    jet_patch_t p;
    int32_t pk;
    uint32_t h0;
    uint8_t id;
    int dead = 0;

    base_patch(&p);
    h0 = render_hash(&p, &pk);
    if (pk < 500) {
        printf("jet_param_live: base patch silent (peak %d)\n", (int)pk);
        return 1;
    }
    if (render_hash(&p, &pk) != h0) {
        printf("jet_param_live: render not deterministic\n");
        return 1;
    }
    for (id = 0u; id < JET_P_N; id++) {
        const jet_param_t *pr = &JET_PARAM[id];
        uint32_t hl, hh;

        /* routed in jet.c (jet_patch_eff_rebuild), not in the DSP: macros and the two mod wheel depths */
        if (id == JET_P_MACBRTH || id == JET_P_MACGRIT || id == JET_P_MACSPC || id == JET_P_XMWLPF ||
            id == JET_P_XMWVIB)
            continue;
        base_patch(&p);
        if (id == JET_P_TCOMP)
            p.lpf = 30u; /* tuning compensation only has something to do once the loop filter delays */
        if (id == JET_P_REVSIZE)
            p.tank = 1u; /* TAIL is the TANK feedback; the SPRING has a fixed length */
        jet_param_set(&p, id, pr->lo);
        hl = render_hash(&p, &pk);
        jet_param_set(&p, id, pr->hi);
        hh = render_hash(&p, &pk);
        if (hl == hh) {
            printf("jet_param_live: DEAD %s (id %u)\n", pr->lbl, id);
            dead++;
        }
    }
    printf("jet_param_live: %u params, %d dead\n", (unsigned)JET_P_N - 5u, dead);
    return dead ? 1 : 0;
}
