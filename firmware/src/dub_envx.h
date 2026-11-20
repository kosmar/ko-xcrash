/* SPDX-License-Identifier: GPL-3.0-only */
/* Genre apps (fm_app.c, not KO BASS): KO KIT ENV per lane, layered on the legacy voice envelopes.
 * All-zero settings = the legacy envelope (attack / snap / decay with gate body), bit for bit.
 * Lanes: 0 KICK, 1 SNARE/RIM, 2 HAT, 3 LEAD/SIREN, 4 BASS, 5 CHORD. */
#ifndef DUB_ENVX_H
#define DUB_ENVX_H

#include <stdint.h>

#define DUB_ENVX_LANES 6u

/* Persisted (dub_cfg_t tail). Signed fields are offsets from the KIT knob centre 64. */
typedef struct {
    uint8_t sus;      /* 0 = off (one-slope legacy decay) */
    uint8_t rel;      /* release time below SUST */
    uint8_t dly;      /* 0 = none */
    uint8_t hold;     /* 0 = legacy snap only */
    int8_t curve;     /* 0 = legacy decay law (shown as 64 / LIN) */
    uint8_t noretrig; /* 0 = RETRIG ON */
    uint8_t inv;
    int8_t pdec;      /* 0 = neutral pitch decay */
    int8_t sweep;     /* 0 = no pitch sweep */
} dub_envx_v_t;

typedef struct {
    int32_t set;
    int32_t pend_env;
    int32_t tru;
    int32_t pk;
    int32_t pitch_q8;
    int32_t pf_q16;
    uint32_t dly_left;
    uint32_t hold_left;
    uint32_t ph_prev[DUB_CHORD_N];
    uint16_t pend_snap;
    uint16_t pend_atk;
    uint16_t prev_atk;
    uint16_t prev_snap;
    uint8_t active;
    uint8_t pf_cnt;
} dub_envx_rt_t;

static inline int dub_envx_neutral(const dub_envx_v_t *c)
{
    return !(c->sus | c->dly | c->hold | (uint8_t)c->curve | c->noretrig | c->inv | (uint8_t)c->sweep);
}

static inline uint8_t dub_envx_u8(int8_t off)
{
    int32_t v = 64 + (int32_t)off;

    return (uint8_t)(v < 0 ? 0 : v > 127 ? 127 : v);
}

static inline int8_t dub_envx_off(uint8_t v)
{
    return (int8_t)((v > 127u ? 127 : (int32_t)v) - 64);
}

#endif /* DUB_ENVX_H */
