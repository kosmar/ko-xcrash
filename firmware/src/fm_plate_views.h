/* SPDX-License-Identifier: GPL-3.0-only */
/* Runtime S2..S8 song views for generated plates; C port of tools/gen_views.py (same roles and rules). */
#ifndef FM_PLATE_VIEWS_H
#define FM_PLATE_VIEWS_H

#include <stdint.h>

enum { PV_K = 0, PV_R, PV_H, PV_S, PV_B, PV_C, PV_LANES };
enum { PV_STEPS = 16, PV_VIEWS = 8, PV_JITTER = 90 };

typedef struct {
    uint32_t s;
} pv_rng_t;

static inline uint32_t pv_next(pv_rng_t *r)
{
    uint32_t x = r->s;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->s = x;
    return x;
}

static inline uint32_t pv_below(pv_rng_t *r, uint32_t n)
{
    return n ? pv_next(r) % n : 0u;
}

static inline int pv_on(uint16_t m, int i)
{
    return i >= 0 && i < PV_STEPS && ((m >> i) & 1u);
}

static inline int pv_hits(uint16_t m)
{
    int c = 0;

    while (m) {
        m &= (uint16_t)(m - 1u);
        c++;
    }
    return c;
}

static inline int pv_weight(int i)
{
    if (i == 0)
        return 4;
    if (i == 8)
        return 3;
    if (i % 4 == 0)
        return 2;
    return (i % 2 == 0) ? 1 : 0;
}

static inline int pv_anchored(int lane, int i)
{
    if (lane == PV_K || lane == PV_R)
        return pv_weight(i) >= 2;
    return lane == PV_B && (i == 0 || i == 8);
}

static inline int pv_no_new(int lane, int i)
{
    if (lane == PV_K || lane == PV_R || lane == PV_C)
        return pv_weight(i) >= 2;
    return (lane == PV_B || lane == PV_S) && i == 0;
}

static inline int pv_slot_used(uint16_t m, int i)
{
    int j;

    for (j = i & 3; j < PV_STEPS; j += 4)
        if (j != i && pv_on(m, j))
            return 1;
    return 0;
}

static inline int pv_run_with(uint16_t m, int i)
{
    int lo = i, hi = i;

    m |= (uint16_t)(1u << i);
    while (pv_on(m, lo - 1))
        lo--;
    while (pv_on(m, hi + 1))
        hi++;
    return hi - lo + 1;
}

static inline int pv_add_score(uint16_t m, uint16_t m0, int lane, int i, int late, int fill)
{
    int grid = ((lane == PV_H || lane == PV_B) && !fill && !(m0 & 0xAAAAu)) ? 2 : 1;
    int s = 0;

    if (pv_on(m, i ^ 8))
        s += 200;
    if (pv_slot_used(m, i))
        s += 100;
    if (pv_on(m, i - grid) || pv_on(m, i + grid))
        s += 100;
    if (grid == 2 && (i & 1))
        s -= 120;
    if (late && i >= 8)
        s += 60;
    s -= (lane == PV_H ? 20 : 60) * pv_weight(i);
    if (lane != PV_H && pv_run_with(m, i) >= 3)
        s -= 150;
    return s;
}

static inline uint16_t pv_add(uint16_t m, uint16_t m0, int lane, int n, pv_rng_t *r, int late, int lo, int hi,
                              int fill)
{
    for (; n > 0; n--) {
        int best_i = -1, best_s = 0, i;

        for (i = lo; i <= hi; i++) {
            int s;

            if (pv_on(m, i) || pv_no_new(lane, i))
                continue;
            s = pv_add_score(m, m0, lane, i, late, fill) + (int)pv_below(r, PV_JITTER);
            if (best_i < 0 || s > best_s) {
                best_s = s;
                best_i = i;
            }
        }
        if (best_i < 0)
            break;
        m |= (uint16_t)(1u << best_i);
    }
    return m;
}

static inline int pv_drop_score(uint16_t m, int i)
{
    int w = pv_weight(i);
    int s = 100 * (3 - (w < 3 ? w : 3));

    if (!pv_on(m, i ^ 8))
        s += 150;
    if (!pv_slot_used(m, i))
        s += 80;
    return s;
}

static inline uint16_t pv_drop(uint16_t m, int lane, int n, pv_rng_t *r)
{
    for (; n > 0; n--) {
        int best_i = -1, best_s = 0, i;

        if (pv_hits(m) <= 1)
            break;
        for (i = 0; i < PV_STEPS; i++) {
            int s;

            if (!pv_on(m, i) || pv_anchored(lane, i))
                continue;
            s = pv_drop_score(m, i) + (int)pv_below(r, PV_JITTER);
            if (best_i < 0 || s > best_s) {
                best_s = s;
                best_i = i;
            }
        }
        if (best_i < 0)
            break;
        m &= (uint16_t)~(1u << best_i);
    }
    return m;
}

/* half: -1 = both halves; dir_mask bit0 = -1, bit1 = +1 */
static inline uint16_t pv_nudge(uint16_t m, int lane, int n, pv_rng_t *r, int dir_mask, int half)
{
    uint16_t moved = 0u, left = 0u;

    for (; n > 0; n--) {
        int best_i = -1, best_t = 0, best_s = 0, i, d;

        for (i = 0; i < PV_STEPS; i++) {
            if (!pv_on(m, i) || pv_anchored(lane, i) || ((moved >> i) & 1u) || (half >= 0 && i / 8 != half))
                continue;
            for (d = -1; d <= 1; d += 2) {
                int t = i + d, w = pv_weight(i), s;

                if (!(dir_mask & (d < 0 ? 1 : 2)))
                    continue;
                if (!(t > 0 && t < PV_STEPS) || pv_on(m, t) || ((left >> t) & 1u) || pv_no_new(lane, t))
                    continue;
                s = 100 * (3 - (w < 3 ? w : 3)) + (pv_on(m, i ^ 8) ? 0 : 60) + (int)pv_below(r, PV_JITTER);
                if (best_i < 0 || s > best_s) {
                    best_s = s;
                    best_i = i;
                    best_t = t;
                }
            }
        }
        if (best_i < 0)
            break;
        m = (uint16_t)((m & ~(1u << best_i)) | (1u << best_t));
        moved |= (uint16_t)(1u << best_t);
        left |= (uint16_t)(1u << best_i);
    }
    return m;
}

/* Two beats trade their off-quarter figures when their quarter hit matches. */
static inline uint16_t pv_swap(uint16_t m, const uint8_t (*pairs)[2], int np, pv_rng_t *r)
{
    int k, rev = (np > 1) && pv_below(r, 2u);

    for (k = 0; k < np; k++) {
        const uint8_t *pr = pairs[rev ? np - 1 - k : k];
        uint16_t ga = (uint16_t)((m >> (4 * pr[0])) & 15u), gb = (uint16_t)((m >> (4 * pr[1])) & 15u);

        if (ga == gb || (ga & 1u) != (gb & 1u))
            continue;
        m &= (uint16_t)~((15u << (4 * pr[0])) | (15u << (4 * pr[1])));
        return (uint16_t)(m | (gb << (4 * pr[0])) | (ga << (4 * pr[1])));
    }
    return m;
}

static inline uint16_t pv_move_beat(uint16_t m, int lane, pv_rng_t *r)
{
    int cands[PV_STEPS], nc = 0, i, k, t;

    for (i = 0; i < PV_STEPS; i++)
        if (pv_on(m, i) && !pv_anchored(lane, i))
            cands[nc++] = i;
    if (!nc || pv_hits(m) >= 4)
        return m;
    i = cands[pv_below(r, (uint32_t)nc)];
    k = pv_below(r, 2u) ? -4 : 4;
    for (t = i + k; ; t = i - k) {
        if (t > 0 && t < PV_STEPS && !pv_on(m, t) && !pv_no_new(lane, t))
            return (uint16_t)((m & ~(1u << i)) | (1u << t));
        if (t == i - k)
            break;
    }
    return m;
}

static inline uint16_t pv_echo(uint16_t m, int lane)
{
    static const int8_t off[5] = { 3, 2, -3, -2, -4 };
    int h = -1, i, k;

    if (!m || pv_hits(m) >= 4)
        return m;
    for (i = 0; i < PV_STEPS; i++)
        if (pv_on(m, i))
            h = i;
    for (k = 0; k < 5; k++) {
        int t = h + off[k];

        if (t > 0 && t < PV_STEPS && !pv_on(m, t) && !pv_no_new(lane, t))
            return (uint16_t)(m | (1u << t));
    }
    return m;
}

typedef struct {
    uint8_t lane, w;
} pv_wt_t;

static inline int pv_s1_hits(const uint16_t *s1)
{
    int i, n = 0;

    for (i = 0; i < PV_LANES; i++)
        n += pv_hits(s1[i]);
    return n;
}

static inline void pv_density(uint16_t *ls, const uint16_t *s1, int pct, const pv_wt_t *wts, int nw, pv_rng_t *r,
                              int late)
{
    int apct = pct < 0 ? -pct : pct;
    int n = (pv_s1_hits(s1) * apct + 50) / 100, done[PV_LANES] = { 0 };

    if (n < 1)
        n = 1;
    for (; n > 0; n--) {
        pv_wt_t pool[PV_LANES];
        int np = 0, k, tot = 0, pick;

        for (k = 0; k < nw; k++) {
            int lane = wts[k].lane, i, ok = 0;
            uint16_t m = ls[lane], m0 = s1[lane];

            if (!m0)
                continue;
            if (pct > 0) {
                int cap = (pv_hits(m0) + 1) / 2;

                if (done[lane] >= (cap < 1 ? 1 : cap) || (lane == PV_S && pv_hits(m0) < 4))
                    continue;
                for (i = 0; i < PV_STEPS && !ok; i++)
                    ok = !pv_on(m, i) && !pv_no_new(lane, i);
            } else {
                if (done[lane] >= (lane == PV_K ? 1 : pv_hits(m0) / 2) || pv_hits(m) <= 1)
                    continue;
                for (i = 0; i < PV_STEPS && !ok; i++)
                    ok = pv_on(m, i) && !pv_anchored(lane, i);
            }
            if (ok)
                pool[np++] = wts[k];
        }
        if (!np)
            break;
        for (k = 0; k < np; k++)
            tot += pool[k].w;
        pick = (int)pv_below(r, (uint32_t)tot);
        for (k = 0; k < np - 1 && pick >= pool[k].w; k++)
            pick -= pool[k].w;
        {
            int lane = pool[k].lane;

            ls[lane] = pct > 0 ? pv_add(ls[lane], s1[lane], lane, 1, r, late, 0, PV_STEPS - 1, 0)
                               : pv_drop(ls[lane], lane, 1, r);
            done[lane]++;
        }
    }
}

enum { PV_F_ADD = 0, PV_F_PICKUP, PV_F_ECHO };

typedef struct {
    uint8_t kind, lane, lo, hi, n;
} pv_fill_t;

static inline void pv_fill(uint16_t *ls, const uint16_t *s1, const pv_fill_t *plan, int np, int pct, pv_rng_t *r)
{
    int budget = (pv_s1_hits(s1) * pct + 50) / 100, k;

    if (budget < 2)
        budget = 2;
    for (k = 0; k < np && budget > 0; k++) {
        int lane = plan[k].lane;
        uint16_t m = ls[lane];

        if (!s1[lane] || (lane == PV_B && pv_hits(s1[PV_B]) < 2))
            continue;
        if (plan[k].kind == PV_F_ADD) {
            int n = plan[k].n < budget ? plan[k].n : budget;

            m = pv_add(m, s1[lane], lane, n, r, 0, plan[k].lo, plan[k].hi, 1);
        } else if (plan[k].kind == PV_F_PICKUP) {
            if (pv_on(m, 0) && pv_hits(m) >= 2)
                m = pv_add(m, s1[lane], PV_K, 1, r, 0, 14, 15, 1);
        } else {
            m = pv_echo(m, lane);
        }
        budget -= pv_hits(m) - pv_hits(ls[lane]);
        ls[lane] = m;
    }
}

static inline void pv_make_view(uint16_t *ls, const uint16_t *s1, int v, pv_rng_t *r)
{
    static const uint8_t P23[1][2] = { { 2, 3 } };
    static const uint8_t P13[2][2] = { { 1, 3 }, { 1, 2 } };
    int i;

    for (i = 0; i < PV_LANES; i++)
        ls[i] = s1[i];
    if (v == 1) { /* ANSWER */
        uint16_t m = pv_swap(ls[PV_H], P23, 1, r);

        ls[PV_H] = m != ls[PV_H] ? m : pv_nudge(ls[PV_H], PV_H, 1, r, 3, 1);
        if (pv_hits(ls[PV_B]) >= 2)
            ls[PV_B] = pv_nudge(ls[PV_B], PV_B, 1, r, 1, 1);
        if (s1[PV_S]) {
            m = pv_swap(ls[PV_S], P23, 1, r);
            ls[PV_S] = m != ls[PV_S] ? m : pv_nudge(ls[PV_S], PV_S, 1, r, 3, 1);
        }
    } else if (v == 2) { /* LIFT */
        static const pv_wt_t w[] = { { PV_H, 5 }, { PV_R, 3 }, { PV_S, 3 } };

        pv_density(ls, s1, 15, w, 3, r, 1);
    } else if (v == 3) { /* TURN */
        static const pv_fill_t f[] = { { PV_F_ADD, PV_H, 12, 15, 2 }, { PV_F_ADD, PV_R, 13, 15, 1 },
                                       { PV_F_ADD, PV_B, 13, 15, 1 }, { PV_F_ADD, PV_S, 13, 15, 1 } };

        pv_fill(ls, s1, f, 4, 20, r);
    } else if (v == 4) { /* THIN */
        static const pv_wt_t w[] = { { PV_H, 5 }, { PV_R, 2 }, { PV_C, 2 }, { PV_B, 1 }, { PV_K, 1 }, { PV_S, 1 } };

        pv_density(ls, s1, -25, w, 6, r, 0);
    } else if (v == 5) { /* SHIFT */
        static const pv_wt_t w[] = { { PV_H, 3 }, { PV_R, 1 }, { PV_C, 1 } };

        ls[PV_H] = pv_swap(ls[PV_H], P13, 2, r);
        ls[PV_R] = pv_swap(ls[PV_R], P13, 2, r);
        ls[PV_C] = pv_swap(ls[PV_C], P13, 2, r);
        ls[PV_S] = pv_move_beat(ls[PV_S], PV_S, r);
        if (pv_hits(ls[PV_B]) >= 2)
            ls[PV_B] = pv_nudge(ls[PV_B], PV_B, 1, r, 1, 0);
        pv_density(ls, s1, -10, w, 3, r, 0);
    } else if (v == 6) { /* BUILD */
        static const pv_wt_t w[] = { { PV_H, 5 }, { PV_R, 2 }, { PV_C, 2 }, { PV_B, 1 }, { PV_S, 3 } };

        pv_density(ls, s1, 25, w, 5, r, 1);
    } else { /* FILL */
        static const pv_fill_t f[] = {
            { PV_F_ADD, PV_H, 12, 15, 3 }, { PV_F_ADD, PV_R, 13, 15, 2 }, { PV_F_PICKUP, PV_K, 0, 0, 1 },
            { PV_F_ADD, PV_B, 11, 15, 1 }, { PV_F_ADD, PV_C, 12, 15, 1 }, { PV_F_ECHO, PV_S, 0, 0, 1 },
            { PV_F_ADD, PV_H, 8, 11, 1 },
        };

        pv_fill(ls, s1, f, 7, 30, r);
    }
}

static inline void pv_fallback(uint16_t *ls, const uint16_t *s1, int k, pv_rng_t *r)
{
    static const uint8_t lane_of[9] = { PV_H, PV_C, PV_B, PV_R, PV_S, PV_H, PV_H, PV_C, PV_H };
    int op = k % 9, lane = lane_of[op];

    if (!s1[lane])
        return;
    switch (op) {
    case 0: ls[lane] = pv_nudge(ls[lane], lane, 1, r, 3, 1); break;
    case 1: ls[lane] = pv_add(ls[lane], s1[lane], lane, 1, r, 1, 0, PV_STEPS - 1, 0); break;
    case 2: ls[lane] = pv_nudge(ls[lane], lane, 1, r, 1, -1); break;
    case 3: ls[lane] = pv_add(ls[lane], s1[lane], lane, 1, r, 1, 0, PV_STEPS - 1, 0); break;
    case 4: ls[lane] = pv_move_beat(ls[lane], lane, r); break;
    case 5: ls[lane] = pv_nudge(ls[lane], lane, 1, r, 3, 0); break;
    case 6: ls[lane] = pv_add(ls[lane], s1[lane], lane, 1, r, 1, 0, PV_STEPS - 1, 0); break;
    case 7: ls[lane] = pv_nudge(ls[lane], lane, 1, r, 3, -1); break;
    default: ls[lane] = pv_drop(ls[lane], lane, 1, r); break;
    }
}

static inline int pv_seen(const uint16_t (*views)[PV_LANES], int nv, const uint16_t *ls)
{
    int v, l;

    for (v = 0; v < nv; v++) {
        for (l = 0; l < PV_LANES && views[v][l] == ls[l]; l++)
            ;
        if (l == PV_LANES)
            return 1;
    }
    return 0;
}

/* s1/s1_hi: 16-step halves per SEQ lane (K R H S B C). out[lane][view-1] = S2..S8. */
static inline void fm_pgen_views(uint16_t out[PV_LANES][PV_VIEWS - 1], const uint16_t *s1, const uint16_t *s1_hi,
                                 uint32_t seed)
{
    uint16_t views[PV_VIEWS][PV_LANES];
    int v, l;

    for (l = 0; l < PV_LANES; l++)
        views[0][l] = s1[l];
    for (v = 1; v < PV_VIEWS; v++) {
        pv_rng_t r;
        int k = 0;

        r.s = (seed ^ ((uint32_t)v * 0x9E3779B9u)) | 1u;
        pv_make_view(views[v], s1, v, &r);
        if (v == 1)
            for (l = 0; l < PV_LANES; l++)
                if (s1_hi[l])
                    views[v][l] = s1_hi[l];
        while (pv_seen((const uint16_t (*)[PV_LANES])views, v, views[v]) && k < 27) {
            pv_fallback(views[v], s1, k, &r);
            k++;
        }
        for (l = 0; l < PV_LANES; l++)
            out[l][v - 1] = views[v][l];
    }
}

#endif
