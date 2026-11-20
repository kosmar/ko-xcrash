/* SPDX-License-Identifier: GPL-3.0-only */
/* Era / plate rules shared by every genre app (fm_app.c) and KO KIT (deck.c). One implementation,
 * no per-app forks:
 *  - Era knob and plate index are independent. An era turn re-voices the current plate; a plate turn
 *    keeps the era. Left and right plate turns behave the same (no direction coupling, no cue).
 *  - Groove (bpm/swing/root/scale) never jumps while playing: the held groove is put back and the
 *    plate's own groove lands on stop. Stopped, a plate turn takes the plate's groove, an era turn
 *    keeps the groove.
 *  - Colour: plate surfaces (title, plate knob, plate tick, plate toast) use the plate's home era;
 *    era surfaces use the era knob.
 * Include after ko_chrome.h. */
#ifndef KO_ERA_PLATE_H
#define KO_ERA_PLATE_H

#include <stdint.h>

typedef uint16_t (*ko_era_col_fn)(uint8_t era);

/* Live groove fields of the app; NULL = the app has no such field. */
typedef struct {
    uint8_t *bpm, *swing, *root, *scale;
} ko_groove_ref_t;

typedef struct {
    uint8_t bpm, swing, root, scale;
} ko_groove_t;

enum { KO_TURN_PLATE = 0u, KO_TURN_ERA = 1u };

static inline void ko_groove_take(ko_groove_t *out, const ko_groove_ref_t *live)
{
    out->bpm = live->bpm ? *live->bpm : 0u;
    out->swing = live->swing ? *live->swing : 0u;
    out->root = live->root ? *live->root : 0u;
    out->scale = live->scale ? *live->scale : 0u;
}

static inline void ko_groove_put(const ko_groove_ref_t *live, const ko_groove_t *g)
{
    if (live->bpm)
        *live->bpm = g->bpm;
    if (live->swing)
        *live->swing = g->swing;
    if (live->root)
        *live->root = g->root;
    if (live->scale)
        *live->scale = g->scale;
}

/* Call after a plate or era turn has rewritten the live groove; `held` is the groove before the turn. */
static inline void ko_turn_groove(const ko_groove_ref_t *live, const ko_groove_t *held, uint8_t turn,
                                  uint8_t playing, uint8_t *pending)
{
    if (playing) {
        ko_groove_put(live, held);
        *pending = 1u;
    } else if (turn == KO_TURN_ERA) {
        ko_groove_put(live, held);
    }
}

static inline uint16_t ko_plate_col(ko_era_col_fn col, uint8_t plate_home_era)
{
    return col(plate_home_era);
}

static inline uint16_t ko_era_knob_col(ko_era_col_fn col, uint8_t knob_era)
{
    return col(knob_era);
}

static inline void ko_top_bar_era_plate_cols(ko_top_bar_t *t, ko_era_col_fn col, uint8_t plate_home_era,
                                             uint8_t knob_era)
{
    t->title_col = ko_plate_col(col, plate_home_era);
    t->sub_col = ko_era_knob_col(col, knob_era);
}

#endif /* KO_ERA_PLATE_H */
