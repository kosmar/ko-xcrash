/* SPDX-License-Identifier: GPL-3.0-only */
/* Factory groove per era × plate slot (16-step lane masks). */
#ifndef DECK_SEQ_GROOVES_H
#define DECK_SEQ_GROOVES_H

#include <stdint.h>
#include "deck_plates.h"

typedef struct {
    uint16_t bd;
    uint16_t bd_acc;
    uint16_t sn;
    uint16_t cp;
    uint16_t ch;
    uint16_t oh;
    uint16_t rm;
    uint16_t rd;
    uint16_t sh;  /* cabasa/shaker family seed (CB/TB/CW in bake) */
    uint16_t tom; /* tom grid (TM/TH/TL in bake) */
    uint16_t cn;  /* conga hi (CNL offbeat subset) */
    uint16_t cr;  /* crash (sparse; ride stays rd) */
} deck_seq_groove_t;

#define DG_ROW(bd, bda, sn, cp, ch, oh, rm, rd, sh, tom, cn, cr)                                   \
    { (uint16_t)(bd), (uint16_t)(bda), (uint16_t)(sn), (uint16_t)(cp), (uint16_t)(ch),             \
      (uint16_t)(oh), (uint16_t)(rm), (uint16_t)(rd), (uint16_t)(sh), (uint16_t)(tom),             \
      (uint16_t)(cn), (uint16_t)(cr) }

#define DG(bd, bda, sn, cp, ch, oh, rm, rd) DG_ROW(bd, bda, sn, cp, ch, oh, rm, rd, 0u, 0u, 0u, 0u)

/* Era factory shaker/tom/conga/crash when plate row leaves sh/tom/cn/cr at 0. Slot rotates masks. */
static inline uint16_t deck_seq_groove_rot(uint16_t m, uint8_t slot)
{
    unsigned s = (unsigned)(slot % 4u) * 4u;

    return (uint16_t)(((m << s) | (m >> (16u - s))) & 0xFFFFu);
}

static inline void deck_seq_groove_perc_defaults(uint8_t era, uint8_t slot, uint16_t *sh, uint16_t *tom,
                                                 uint16_t *cn, uint16_t *cr)
{
    static const uint16_t SH[DECK_ERA_N] = {0x0A90u, 0x8080u, 0x4040u, 0x2424u, 0x1010u, 0x0040u};
    static const uint16_t TOM[DECK_ERA_N] = {0x0808u, 0x1010u, 0x8208u, 0x8208u, 0x4924u, 0x8080u};
    static const uint16_t CN[DECK_ERA_N] = {0x1414u, 0x2020u, 0x0808u, 0x8080u, 0x0404u, 0x0000u};
    static const uint16_t CR[DECK_ERA_N] = {0x0000u, 0x8000u, 0x8000u, 0x0000u, 0x4000u, 0x0000u};

    era = deck_clamp_era(era);
    slot %= DECK_PLATES_PER_ERA;
    *sh = deck_seq_groove_rot(SH[era], slot);
    *tom = deck_seq_groove_rot(TOM[era], (uint8_t)(slot ^ 1u));
    *cn = deck_seq_groove_rot(CN[era], (uint8_t)(slot + 2u));
    *cr = deck_seq_groove_rot(CR[era], slot);
}

static inline void deck_seq_groove_perc_resolve(const deck_seq_groove_t *g, uint8_t era, uint8_t slot,
                                                  uint16_t *cb, uint16_t *tb, uint16_t *cw, uint16_t *tm,
                                                  uint16_t *th, uint16_t *tl, uint16_t *cnh, uint16_t *cnl,
                                                  uint16_t *crash)
{
    uint16_t sh, tom, cn, cr;

    if (!g || !cb)
        return;
    deck_seq_groove_perc_defaults(era, slot, &sh, &tom, &cn, &cr);
    if (g->sh)
        sh = g->sh;
    if (g->tom)
        tom = g->tom;
    if (g->cn)
        cn = g->cn;
    if (g->cr)
        cr = g->cr;
    *cb = sh;
    *tb = (uint16_t)(sh & 0xAAAAu);
    *cw = (uint16_t)(sh & 0x5555u);
    *tm = tom;
    *th = (uint16_t)(tom & 0xCCCCu);
    *tl = (uint16_t)(tom & 0x3333u);
    *cnh = cn;
    *cnl = (uint16_t)(cn & 0xAAAAu);
    *crash = cr;
}

static const deck_seq_groove_t DECK_SEQ_GROOVE[DECK_ERA_N][DECK_PLATES_PER_ERA] = {
    /* STAX / 60s — motown swing, rim-heavy */
    {
        DG(0x1111u, 0x0101u, 0x1010u, 0x1010u, 0xAAAAu, 0x4000u, 0u, 0u),
        DG(0x1249u, 0x1001u, 0x1010u, 0x1000u, 0x8888u, 0x2000u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0x0010u, 0x0010u, 0x2222u, 0u, 0u, 0u),
        DG(0x1111u, 0x0101u, 0u, 0x1010u, 0xFFFFu, 0u, 0u, 0u),
        DG(0x1111u, 0x0101u, 0u, 0x8080u, 0x5555u, 0u, 0x2020u, 0u),
        DG(0x1421u, 0x1001u, 0x1040u, 0x1000u, 0xCCCCu, 0u, 0x2020u, 0u),
        DG(0x1010u, 0x0010u, 0u, 0u, 0xAAAAu, 0x8000u, 0x4444u, 0u),
        DG(0x1041u, 0x1001u, 0u, 0u, 0x0808u, 0x8010u, 0x1010u, 0u),
        DG(0x1511u, 0x0101u, 0u, 0x1000u, 0xCC33u, 0x4000u, 0u, 0u),
        DG(0x159Du, 0x1101u, 0x1010u, 0x5050u, 0xEEEEu, 0x4000u, 0x8080u, 0u),
    },
    /* DEAD / 70s — disco four-on-floor, tight hats */
    {
        DG(0x0101u, 0x0101u, 0x1010u, 0u, 0xFFFFu, 0u, 0u, 0u),
        DG(0x0101u, 0x0001u, 0x5555u, 0u, 0xAAAAu, 0u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0x0010u, 0u, 0xFFFFu, 0x0800u, 0u, 0u),
        DG(0x0101u, 0x0101u, 0x9090u, 0u, 0xCCCCu, 0u, 0x4040u, 0u),
        DG(0x0101u, 0x0101u, 0x1010u, 0u, 0xFF00u, 0u, 0x2020u, 0u),
        DG(0x0101u, 0x0101u, 0x1010u, 0x8080u, 0xF0F0u, 0x0808u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0x1000u, 0u, 0x8888u, 0u, 0u, 0u),
        DG(0x0101u, 0x0101u, 0x1212u, 0x8080u, 0xCCCCu, 0u, 0u, 0u),
        DG(0x0001u, 0x0001u, 0x1010u, 0u, 0x5555u, 0u, 0x1010u, 0u),
        DG(0x0D05u, 0x0101u, 0xB4B4u, 0x4040u, 0xFFFFu, 0x2000u, 0x4040u, 0u),
    },
    /* GATE / 80s — gated snare, bright hats */
    {
        DG(0x1010u, 0x1010u, 0x1010u, 0u, 0x2020u, 0u, 0u, 0u),
        DG(0x1111u, 0x0101u, 0u, 0x1010u, 0x2222u, 0u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0x1010u, 0u, 0x1111u, 0u, 0u, 0u),
        DG(0x1511u, 0x0101u, 0x1010u, 0u, 0x4444u, 0x4000u, 0x1C07u, 0u),
        DG(0x1001u, 0x1001u, 0x1010u, 0x0010u, 0x1212u, 0u, 0u, 0u),
        DG(0x1241u, 0x1001u, 0x1010u, 0u, 0x4444u, 0u, 0u, 0u),
        DG(0x1111u, 0x0101u, 0u, 0x5050u, 0x2222u, 0x8000u, 0u, 0u),
        DG(0x1411u, 0x0101u, 0x1010u, 0u, 0xCCCCu, 0u, 0u, 0u),
        DG(0x1041u, 0x1001u, 0x1010u, 0u, 0x1212u, 0x2000u, 0u, 0u),
        DG(0x1151u, 0x0101u, 0x1010u, 0x1010u, 0xAAAAu, 0x4000u, 0u, 0u),
    },
    /* CRATE / 90s — boom-bap, swung, sparse */
    {
        DG(0x1111u, 0x0101u, 0x1010u, 0u, 0x5555u, 0u, 0x2020u, 0u),
        DG(0x0889u, 0x0801u, 0x1010u, 0u, 0xAAAAu, 0u, 0u, 0u),
        DG(0x1010u, 0x0010u, 0x1010u, 0u, 0x3333u, 0u, 0x5555u, 0u),
        DG(0x1001u, 0x1001u, 0x1010u, 0u, 0x4444u, 0u, 0u, 0u),
        DG(0x1248u, 0x1000u, 0x1010u, 0x8080u, 0x9999u, 0u, 0x4040u, 0u),
        DG(0x1111u, 0x0101u, 0u, 0u, 0x2222u, 0u, 0u, 0x5555u),
        DG(0x1111u, 0x0101u, 0x1010u, 0u, 0x4924u, 0u, 0x1010u, 0u),
        DG(0x0101u, 0x0101u, 0x5555u, 0u, 0x8888u, 0u, 0u, 0u),
        DG(0x1511u, 0x0101u, 0x1010u, 0u, 0xCCCCu, 0u, 0x5050u, 0x2020u),
        DG(0x1551u, 0x0101u, 0x1010u, 0x5050u, 0xEEEEu, 0u, 0x8080u, 0x4040u),
    },
    /* GRID / 00s — straight techno / electro grid */
    {
        DG(0x1041u, 0x1001u, 0x1010u, 0u, 0x8888u, 0u, 0x1010u, 0u),
        DG(0x1241u, 0x1001u, 0x1010u, 0u, 0xCCCCu, 0u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0u, 0x0010u, 0x2020u, 0u, 0u, 0u),
        DG(0x8041u, 0x8001u, 0x9090u, 0u, 0x5555u, 0u, 0x8080u, 0u),
        DG(0x1091u, 0x1001u, 0u, 0u, 0x9249u, 0u, 0u, 0u),
        DG(0x1010u, 0x0010u, 0x5555u, 0u, 0x2222u, 0u, 0u, 0u),
        DG(0x1041u, 0x1001u, 0u, 0u, 0x0808u, 0u, 0x8080u, 0u),
        DG(0x1415u, 0x0101u, 0x1010u, 0u, 0x4444u, 0u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0u, 0x1000u, 0x1111u, 0u, 0u, 0u),
        DG(0x159Du, 0x1101u, 0x1212u, 0u, 0xAAAAu, 0x4000u, 0x4040u, 0u),
    },
    /* TRAP / 10s — sparse hats, 808 kick patterns */
    {
        DG(0x1001u, 0x1001u, 0u, 0x8180u, 0x6666u, 0x2020u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0x0100u, 0x0100u, 0x8888u, 0x5555u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0u, 0x0100u, 0x4444u, 0u, 0u, 0u),
        DG_ROW(0x1041u, 0x1001u, 0x0100u, 0u, 0xCCCCu, 0x8208u, 0x1C07u, 0u, 0u, 0x8080u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0u, 0x0100u, 0xAAAAu, 0x8208u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0x0100u, 0u, 0xCCCCu, 0x1010u, 0u, 0u),
        DG(0x1000u, 0x1000u, 0x0100u, 0x0100u, 0x6666u, 0x2020u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0u, 0x4140u, 0x5555u, 0x8080u, 0u, 0u),
        DG(0x1001u, 0x1001u, 0u, 0x0100u, 0x8888u, 0x4000u, 0u, 0u),
        DG(0x1101u, 0x1001u, 0u, 0x8180u, 0xAAAAu, 0x5555u, 0u, 0u),
    },
};

#endif /* DECK_SEQ_GROOVES_H */
