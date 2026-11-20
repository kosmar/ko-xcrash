#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate firmware/src/bass_data.h — KO BASS genre pack (60 lines, bass-heavy gates)."""
from __future__ import annotations

import hashlib
import itertools
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "firmware" / "src" / "bass_data.h"

ERA_N = 6
PRESETS_PER_ERA = 10
PRESET_N = ERA_N * PRESETS_PER_ERA

ERA_NAMES = ["ACID", "SUB", "REESE", "FM", "FOLD", "SYNC"]

LINE_NAMES = [
    ["SLIDE", "SQUEL", "ROLL", "GATE", "LOW", "CUT", "DRIVE", "WORM", "STEP", "DROP"],
    ["DEEP", "PILL", "SINE", "808", "TONE", "BODY", "HOLLOW", "DRONE", "WARM", "TIGHT"],
    ["DET1", "DET2", "WIDE", "NARROW", "PHASE", "MOVE", "SWARM", "BITE", "DARK", "BLOOM"],
    ["BELL", "METL", "RING", "DING", "BYTE", "CRISP", "GLASS", "EDGE", "HARD", "SOFT"],
    ["CRUSH", "CLIP", "FOLD1", "FOLD2", "GRIT", "SMEAR", "BEND", "WARP", "TEAR", "BREAK"],
    ["LOCK", "PULL", "PUSH", "DRIFT", "CHASE", "MIRROR", "TWIN", "OFFSET", "LINK", "CHAIN"],
]

BASS_NAMES = [
    "RUMBLE", "SUB909", "EBM16", "HOOVER", "ACID303", "DETROIT",
    "SCHRANZ", "REESE", "MODFM", "DRONE", "FOLD", "SYNC", "PWM", "WAVEFOLD",
]
BASS_ERA = [4, 0, 5, 1, 0, 0, 2, 1, 3, 2, 4, 5, 3, 4]
BASS_N = len(BASS_NAMES)

ERA_BPM_LO = [124, 118, 128, 126, 132, 130]
ERA_BPM_HI = [132, 124, 136, 134, 142, 138]

# Minimal drum-kit ids for fm_app / draw compatibility (lanes muted in patterns).
DKIT_NAMES = ["SUB", "DEEP", "TIGHT", "WIDE", "FM", "SYNC", "ACID", "DRONE"]
DKIT_N = len(DKIT_NAMES)
DKIT_ERA = [1, 1, 0, 2, 3, 5, 0, 4]

VOICE_NAMES = ["ROOT", "FIFTH", "OCT", "MIN7", "MIN9", "SUS4", "DIM", "CLUSTER"]
VOICE_N = len(VOICE_NAMES)


def mask16(seed: int, era: int, ix: int) -> int:
    h = int(hashlib.md5(f"bass-{seed}-{era}-{ix}".encode()).hexdigest()[:4], 16)
    return h & 0xFFFF


def mask32(seed: int, era: int, ix: int) -> int:
    lo = mask16(seed, era, ix)
    hi = mask16(seed + 7, era, ix)
    return lo | (hi << 16)


# Musical archetypes: 3–6 hits/bar, downbeat root, space over grid.
ARCHETYPE_ORDER = [
    "one_drop",
    "root_fifth",
    "octave_jump",
    "syncopated_332",
    "halftime",
    "walking_sparse",
]
ARCHETYPE_CORE: dict[str, list[int]] = {
    "one_drop": [0, 8],
    "root_fifth": [0, 7, 8],
    "octave_jump": [0, 8, 12],
    "syncopated_332": [0, 3, 6, 8, 11],
    "halftime": [0, 4, 12],
    "walking_sparse": [0, 2, 6, 8, 13],
}
# Per-line core variants (same archetype character, different IOI skeleton). Index 0 = ARCHETYPE_CORE.
ARCHETYPE_CORE_ALT: dict[str, list[list[int]]] = {
    "one_drop": [],
    "root_fifth": [[0, 6, 8], [0, 7, 10]],
    "octave_jump": [[0, 8, 10], [0, 6, 12]],
    "syncopated_332": [[0, 3, 5, 8, 11]],
    "halftime": [[0, 4, 11]],
    "walking_sparse": [[0, 2, 6, 9, 13], [0, 3, 6, 8, 13]],
}


def core_variant(archetype: str, era: int, ix: int) -> list[int]:
    alts = ARCHETYPE_CORE_ALT[archetype]
    v = mask16(43, era, ix) % (1 + len(alts))
    return list(ARCHETYPE_CORE[archetype] if v == 0 else alts[v - 1])


ARCHETYPE_SYNC: dict[str, list[int]] = {
    "one_drop": [6, 10, 14],
    "root_fifth": [6, 10],
    "octave_jump": [6, 14],
    "syncopated_332": [10, 14],
    "halftime": [10, 14],
    "walking_sparse": [4, 10, 14],
}


# Optional onsets per archetype (never core steps). Each bar draws 0–2 of them from a hash-rotated pool,
# so lines of the same archetype no longer share one fixed IOI skeleton. Era bias shifts the draw count.
ARCHETYPE_OPT: dict[str, list[int]] = {
    "one_drop": [4, 10, 12, 14],
    "root_fifth": [3, 4, 10, 11, 14],
    "octave_jump": [4, 6, 10, 14],
    "syncopated_332": [1, 13, 14],
    "halftime": [6, 10, 14],
    "walking_sparse": [4, 10, 11, 14, 15],
}
ERA_OPT_BIAS = [1, 0, 1, -1, 0, 1]  # per era: integer shift of optional-hit count (density guard)


def popcount16(m: int) -> int:
    return sum((m >> b) & 1 for b in range(16))


ARCHETYPE_OPT_MAX: dict[str, int] = {
    "one_drop": 1, "root_fifth": 2, "octave_jump": 2, "syncopated_332": 2, "halftime": 2, "walking_sparse": 3,
}


def _opt_hits(archetype: str, era: int, ix: int, bar: int, salt: int = 0) -> int:
    """Hash picks how many optional onsets (0..ARCHETYPE_OPT_MAX, shifted by era bias) and which subset."""
    opt = ARCHETYPE_OPT[archetype]
    key = ix * 8 + bar + SALT_STRIDE * salt
    r = mask16(37, era, key)
    want = max(0, min(ARCHETYPE_OPT_MAX[archetype], (r % 4) + ERA_OPT_BIAS[era]))
    if want == 0:
        return 0
    subsets = list(itertools.combinations(opt, want))
    combo = subsets[mask16(41, era, key) % len(subsets)]
    m = 0
    for s in combo:
        m |= 1 << s
    return m


def pick_archetype(era: int, ix: int) -> str:
    return ARCHETYPE_ORDER[(era * 3 + ix) % len(ARCHETYPE_ORDER)]


def _drop_last_sync(m: int, sync_pool: list[int]) -> int:
    for s in reversed(sync_pool):
        if (m >> s) & 1 and s != 0:
            return m & ~(1 << s)
    for s in range(15, 0, -1):
        if (m >> s) & 1:
            return m & ~(1 << s)
    return m


def bar_gate16(archetype: str, era: int, ix: int, bar: int, salt: int = 0) -> int:
    core = core_variant(archetype, era, ix)
    sync = ARCHETYPE_SYNC[archetype]
    key = ix * 8 + bar + SALT_STRIDE * salt  # unique per (line, bar, salt); old ix + bar key collided
    m = 1 << 0
    for s in core:
        if s != 0:
            m |= 1 << s
    m |= _opt_hits(archetype, era, ix, bar, salt)
    if bar in (3, 7) and (mask16(19, era, key) & 3) == 0:
        m &= (1 << 0) | (1 << 8)
    elif bar in (2, 4, 6) and (mask16(21, era, key) & 1):
        m &= (1 << 0) | (1 << 8) | (1 << (10 if era & 1 else 6))
    while popcount16(m) > 6:
        m = _drop_last_sync(m, sync)
    while popcount16(m) < 3 and (mask16(29, era, key) & 1):
        for s in (6, 10, 14, 4, 12):
            if not ((m >> s) & 1) and popcount16(m) < 6:
                m |= 1 << s
                break
        else:
            break
    return m & 0xFFFF


SALT_STRIDE = 80  # hash-key stride per salt (bars are ix*8 + bar, so ix*8+bar < 80)


def _line_bars(era: int, ix: int, salt: int) -> list[int]:
    arch = pick_archetype(era, ix)
    bars: list[int] = []
    for bar in range(8):
        key = ix * 8 + bar + SALT_STRIDE * salt
        if bar > 0 and bar % 2 == 1 and ((mask16(27, era, key) & 3) == 0):
            bars.append(bars[bar - 1])
        else:
            bars.append(bar_gate16(arch, era, ix, bar, salt))
    return bars


_SALT_CACHE: dict[tuple[int, int], int] | None = None


def _salt_table() -> dict[tuple[int, int], int]:
    """Per line: first salt (0..15) whose 8-bar hit-count sequence is not used by an earlier line.
    Deterministic, integer-only. Falls back to salt 0 if all 16 collide."""
    global _SALT_CACHE
    if _SALT_CACHE is not None:
        return _SALT_CACHE
    seen: set[tuple[int, ...]] = set()
    table: dict[tuple[int, int], int] = {}
    for e in range(ERA_N):
        for i in range(PRESETS_PER_ERA):
            chosen = 0
            sig = tuple(popcount16(m) for m in _line_bars(e, i, 0))
            for salt in range(16):
                cand = tuple(popcount16(m) for m in _line_bars(e, i, salt))
                if cand not in seen:
                    chosen, sig = salt, cand
                    break
            seen.add(sig)
            table[(e, i)] = chosen
    _SALT_CACHE = table
    return table


def line_bar_gates(era: int, ix: int) -> list[int]:
    return _line_bars(era, ix, _salt_table()[(era, ix)])


def line_gate16(era: int, ix: int) -> int:
    return line_bar_gates(era, ix)[0]


def line_gate32(era: int, ix: int) -> int:
    bars = line_bar_gates(era, ix)
    return bars[0] | (bars[1] << 16)


def sparse16(era: int, ix: int, seed: int) -> int:
    return mask16(seed, era, ix) & 0x0010


# Pitch contour (semitones relative to the line root). Range = PITCH_LO..PITCH_HI (old ROM range -5..12).
PITCH_LO, PITCH_HI = -5, 12
# Per era: sorted pool of allowed pitches; walk = index deltas between hits; prog = 8 chord roots (bars).
PITCH_POOL: dict[int, list[int]] = {
    0: [-2, -1, 0, 1, 2, 3, 5, 7, 8, 10, 12],  # ACID: chromatic/pentatonic 16th motion
    1: [-5, 0, 7, 12],                          # SUB: root-fifth-octave
    2: [-5, -2, 0, 3, 5, 7, 10, 12],            # REESE: slow detuned intervals
    3: [-5, -3, 0, 3, 4, 7, 9, 12],             # FM: leaps and thirds
    4: [-2, -1, 0, 1, 2, 3],                    # FOLD: tight chromatic cluster
    5: [-5, 0, 3, 5, 7, 10, 12],                # SYNC: call and response
}
ERA_WALK: dict[int, list[int]] = {
    0: [-1, 1, 1, -1, 0, 2, -2],
    1: [1, -1, 2, -2, 0, 0, 1],
    2: [1, -1, 2, -2, 0],
    3: [1, -2, 2, -1, 3, -3],
    4: [1, -1, 1, -1, 0],
    5: [1, -1, 2, -2, 0],
}
ERA_LEAP_ERAS = {3, 5}  # FM and SYNC take occasional leaps (walk delta doubled)
ERA_PROG: dict[int, list[int]] = {
    0: [0, 0, 3, 0, -2, 0, 3, -2],
    1: [0, 0, 7, 0, 0, -5, 0, 7],
    2: [0, 0, -2, -2, 3, 3, 0, 0],
    3: [0, 3, 7, 3, 0, -3, 0, 7],
    4: [0, 0, 1, 1, 0, 0, -1, 0],
    5: [0, 5, 7, 5, 0, 3, 7, 0],
}
ERA_CALL_RESP = {5: {3: 7, 7: 0}}  # SYNC: phrase bar 3 ends on the fifth (call), bar 7 resolves to root


def h32(*key: object) -> int:
    return int(hashlib.md5("bass-pitch-".encode() + repr(key).encode()).hexdigest()[:8], 16)


def chord_anchors(era: int, chord: int) -> list[int]:
    pool = PITCH_POOL[era]
    tones = {0, 7} if era in (1, 4) else {0, 3, 4, 7}
    hits = [v for v in pool if (v - chord) % 12 in tones]
    return hits or [min(pool, key=lambda v: abs(v - chord))]


def line_pitches128(era: int, ix: int) -> list[int]:
    """128 pitches (bar * 16 + step) for one line. Only gated (bar, step) cells are set; others stay 0.
    Downbeat (first hit of a bar) = chord-tone anchor; other hits walk the era pool; last hit approaches the
    next bar's chord root (passing tone). Bars differ through the hash and the 8-bar chord progression."""
    pool = PITCH_POOL[era]
    walk = ERA_WALK[era]
    prog = ERA_PROG[era]
    off = (ix * 3 + era) % 8
    gates = line_bar_gates(era, ix)
    out = [0] * 128
    idx: int | None = None
    for b in range(8):
        chord = prog[(b + off) % 8]
        nxt = prog[(b + 1 + off) % 8]
        hits = [st for st in range(16) if (gates[b] >> st) & 1]
        for k, st in enumerate(hits):
            r = h32(era, ix, b, st)
            if k == 0 or idx is None:
                cand = chord_anchors(era, chord)
                if idx is None:
                    pick = cand[r % len(cand)]
                else:
                    prev = pool[idx]
                    near = sorted(cand, key=lambda v: (abs(v - prev), v))
                    reach = [v for v in near if abs(v - prev) <= 12] or near
                    pick = reach[-1] if (r >> 3) % 5 == 0 else reach[0]
                idx = pool.index(pick)
            elif k == len(hits) - 1 and (b < 7 or era in ERA_CALL_RESP):
                tgt = ERA_CALL_RESP.get(era, {}).get(b)
                if tgt is None:
                    tgt = min(pool, key=lambda v: abs(v - nxt))
                tidx = pool.index(min(pool, key=lambda v: abs(v - tgt)))
                idx += (tidx > idx) - (tidx < idx)
            else:
                d = walk[r % len(walk)]
                if era in ERA_LEAP_ERAS and (r >> 7) % 9 == 0:
                    d *= 2
                ni = idx + d
                if not 0 <= ni < len(pool):  # bounce off the register edge instead of parking on it
                    ni = idx - d
                idx = min(len(pool) - 1, max(0, ni))
            out[b * 16 + st] = max(PITCH_LO, min(PITCH_HI, pool[idx]))
    return out


def line_lens(era: int, ix: int, gate16: int) -> list[str]:
    bars = line_bar_gates(era, ix)
    lens: list[str] = []
    for bar in range(8):
        g16 = bars[bar]
        for st in range(16):
            if not ((g16 >> st) & 1):
                lens.append("0")
                continue
            if st in (7, 15) and ((g16 >> ((st + 1) & 15)) & 1):
                lens.append("9")
            elif st in (14, 15) and bar < 7:
                lens.append(str(10 + (ix % 2)))
            else:
                lens.append("4")
    return lens


def line_bends(era: int, ix: int, gate16: int) -> list[str]:
    bars = line_bar_gates(era, ix)
    bends: list[str] = []
    for bar in range(8):
        g16 = bars[bar]
        for st in range(16):
            if st == 7 and ((g16 >> 8) & 1):
                bends.append(str(4 + (era % 2)))
            elif st == 15 and bar < 7 and (bars[bar + 1] & 1):
                bends.append(str(5 + (ix % 2)))
            else:
                bends.append("0")
    return bends


def gen_plate(era: int, ix: int) -> str:
    name = LINE_NAMES[era][ix]
    bass_model = (era * 2 + ix) % BASS_N
    voicing = (era + ix) % VOICE_N
    tap = ix & 3
    tank = (era * 9 + ix * 11) % 80
    span = max(1, PRESETS_PER_ERA - 1)
    bpm = ERA_BPM_LO[era] + (ix * (ERA_BPM_HI[era] - ERA_BPM_LO[era])) // span
    dens = 28 + (era * 3) + (ix % 4)
    dly_c = 44 + (era * 4 + ix * 2) % 28
    rev_c = 38 + (era * 3 + ix) % 24
    fb = 10 + (era + ix) % 22
    lf = 36 + (era * 7) % 28
    wob = (era * 11 + ix * 7) % 72
    pump = 20 + (era * 5) % 40
    k = sparse16(era, ix, 1)
    r = sparse16(era, ix, 2)
    h = sparse16(era, ix, 3)
    su = line_gate32(era, ix)
    ch = sparse16(era, ix, 5) & 0x00F0
    si = sparse16(era, ix, 6) & 0x0003
    sub_lo = su & 0xFFFF
    sub_hi = (su >> 16) & 0xFFFF
    return (
        f'    /* {ERA_NAMES[era]} {ix} */ {{ "{name}", {era}, {bass_model}, {voicing}, {tap}, {tank}, '
        f"{bpm}, {dens}, {dly_c}, {rev_c}, {fb}, {lf}, {wob}, {pump}, "
        f"0x{k:04X}u, 0x{r:04X}u, 0x{h:04X}u, "
        f"0x{sub_lo:04X}u | ((uint32_t)0x{sub_hi:04X}u << 16), "
        f"0x{ch:04X}u | ((uint32_t)0x0000u << 16), 0x{si:04X}u | ((uint32_t)0x0000u << 16) }}"
    )


def emit_typedefs() -> str:
    return r'''
typedef struct {
    uint8_t timbre, cutoff_c, decay_c, reso_mode, env_filt;
    uint8_t cutoff_b, decay_b, sub_oct, drive, dly_b;
    uint8_t kick_boom, rim_tone, hat_lvl, kit_tune, kit_decay, kit_drive;
    uint8_t siren_pitch, siren_rate, rim_dly;
    uint8_t drm_pan[4];
    uint8_t wow, dly_time, isolator, swing;
} bass_era_base_t;

typedef struct {
    uint8_t drum_kit, tank, tap, loop_filt, feedback;
    uint8_t dly_lvl, rev_mix, rev_damp, rev_tail, dly_rev;
    uint8_t pump, bass_model, wobble;
} bass_era_fx_t;
'''


def emit_helpers() -> str:
    return r'''
static inline uint8_t bass_clamp_line(uint8_t ix)
{
    return ix >= BASS_LINE_N ? (uint8_t)(BASS_LINE_N - 1u) : ix;
}

static inline uint8_t bass_clamp_era(uint8_t era)
{
    return era >= BASS_ERA_N ? (uint8_t)(BASS_ERA_N - 1u) : era;
}

static inline const char *bass_era_name(uint8_t era)
{
    return BASS_ERA_NAMES[bass_clamp_era(era)];
}

static inline const char *bass_line_name(uint8_t slab)
{
    return BASS_LINE[bass_clamp_line(slab)].name;
}

static inline const bass_line_t *bass_line(uint8_t ix)
{
    return &BASS_LINE[bass_clamp_line(ix)];
}

static inline uint8_t bass_era_of_line(uint8_t slab)
{
    slab = bass_clamp_line(slab);
    return (uint8_t)(slab / GENRE_PRESETS_PER_ERA);
}

static inline uint8_t bass_era_first_line(uint8_t era)
{
    return (uint8_t)(bass_clamp_era(era) * GENRE_PRESETS_PER_ERA);
}

static inline uint32_t bass_line_lead_gate32(uint8_t slab)
{
    return BASS_LINE_LEAD_GATE[bass_clamp_line(slab)];
}

static inline uint8_t bass_line_bar_step_bit(uint8_t slab, uint8_t bar, uint8_t step)
{
    slab = bass_clamp_line(slab);
    bar &= 7u;
    step &= 15u;
    return (uint8_t)((BASS_LINE_BAR_GATE[slab][bar] >> step) & 1u);
}

static inline uint8_t bass_line_lead_step_bit(uint8_t slab, uint8_t step)
{
    uint8_t bar = (uint8_t)((step >> 4) & 7u);
    uint8_t st = (uint8_t)(step & 15u);
    return bass_line_bar_step_bit(slab, bar, st);
}

static inline int8_t bass_line_lead_pitch(uint8_t slab, uint8_t step)
{
    return BASS_LINE_LEAD_PITCH[bass_clamp_line(slab)][step & 15u];
}

static inline uint8_t bass_line_lead_len_ix(uint8_t slab, uint8_t step)
{
    return BASS_LINE_LEAD_LEN[bass_clamp_line(slab)][step & 127u];
}

static inline uint8_t bass_line_lead_bend_u7(uint8_t slab, uint8_t step)
{
    return BASS_LINE_LEAD_BEND[bass_clamp_line(slab)][step & 127u];
}

static inline void bass_apply_defaults(dub_params_t *p)
{
    uint8_t di;

    if (!p)
        return;
    p->timbre = 32;
    p->cutoff_c = 72;
    p->decay_c = 80;
    p->cutoff_b = 88;
    p->decay_b = 94;
    p->sub_oct = 32;
    p->drive = 18;
    p->pump = 24;
    p->feedback = 12;
    p->density = 72;
    p->wobble = 16;
    p->kick_boom = 0;
    p->rim_tone = 0;
    p->hat_lvl = 0;
    p->siren_pitch = 0;
    p->siren_rate = 0;
    p->drum_kit = 0;
    p->bass_model = BASS_SUB909;
    for (di = 0; di < 4u; di++) {
        p->drm_tune[di] = 64u;
        p->drm_dec[di] = 64u;
        p->drm_drv[di] = 8u;
        p->drm_dly[di] = 20u;
        p->drm_rev[di] = 16u;
        p->drm_cut[di] = 64u;
        p->drm_atk[di] = 0u;
        p->drm_dens[di] = 0u;
    }
}

static inline void bass_era_apply_base(dub_params_t *p, uint8_t era)
{
    const bass_era_base_t *eb;
    const bass_era_fx_t *fx;
    uint8_t i;

    if (!p)
        return;
    era = bass_clamp_era(era);
    eb = &BASS_ERA_BASE[era];
    fx = &BASS_ERA_FX[era];
    p->era = era;
    p->timbre = eb->timbre;
    p->cutoff_c = eb->cutoff_c;
    p->decay_c = eb->decay_c;
    p->reso_mode = eb->reso_mode;
    p->env_filt = eb->env_filt;
    p->cutoff_b = eb->cutoff_b;
    p->decay_b = eb->decay_b;
    p->sub_oct = eb->sub_oct;
    p->drive = eb->drive;
    p->dly_b = eb->dly_b;
    p->kick_boom = eb->kick_boom;
    p->rim_tone = eb->rim_tone;
    p->hat_lvl = eb->hat_lvl;
    for (i = 0; i < 4u; i++) {
        p->drm_tune[i] = eb->kit_tune;
        p->drm_dec[i] = eb->kit_decay;
        p->drm_drv[i] = eb->kit_drive;
    }
    p->siren_pitch = eb->siren_pitch;
    p->siren_rate = eb->siren_rate;
    for (i = 0; i < 3u; i++)
        p->drm_dly[i] = eb->rim_dly;
    p->drm_dly[3] = eb->rim_dly;
    for (i = 0; i < 4u; i++)
        p->drm_pan[i] = eb->drm_pan[i];
    p->wow = eb->wow;
    p->dly_time = eb->dly_time;
    p->isolator = eb->isolator;
    p->swing = eb->swing;
    p->tap = fx->tap;
    p->tank = fx->tank;
    p->loop_filt = fx->loop_filt;
    p->feedback = fx->feedback;
    p->dly_lvl = fx->dly_lvl;
    p->rev_mix = fx->rev_mix;
    p->rev_damp = fx->rev_damp;
    p->rev_tail = fx->rev_tail;
    p->dly_rev = fx->dly_rev;
    p->pump = fx->pump;
    p->wobble = fx->wobble;
    p->drum_kit = fx->drum_kit;
    p->bass_model = fx->bass_model;
    p->lvl_b = 96;
    p->lvl_c = 0;
    p->rev_b = (uint8_t)(24u + (fx->rev_mix >> 2));
    p->rev_c = 0;
}

static inline void bass_line_apply(dub_params_t *p, uint8_t slab, uint8_t what)
{
    const bass_line_t *sc;

    if (!p)
        return;
    slab = bass_clamp_line(slab);
    sc = bass_line(slab);
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        bass_apply_defaults(p);
        bass_era_apply_base(p, sc->era);
    }
    p->era = sc->era;
    p->plate = slab;
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        p->bpm = sc->bpm;
        p->density = sc->density;
        p->bass_model = sc->bass_model;
        p->voicing = sc->voicing;
        p->pump = sc->pump;
        p->wobble = sc->wobble;
        p->feedback = sc->feedback;
        p->tank = sc->tank;
        p->tap = sc->tap;
        p->dly_c = sc->dly_c;
        p->rev_c = sc->rev_c;
        p->loop_filt = sc->loop_filt;
        p->root = BASS_LINE_ROOT[slab];
        p->scale = BASS_LINE_SCALE[slab];
        p->swing = BASS_LINE_SWING[slab];
    }
}

static inline void bass_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t slab)
{
    if (!p)
        return;
    era = bass_clamp_era(era);
    slab = bass_clamp_line(slab);
    bass_line_apply(p, slab, DUB_APPLY_ALL);
    p->era = era;
    p->plate = slab;
    p->bass_model = BASS_LINE[slab].bass_model;
    p->voicing = BASS_LINE[slab].voicing;
    p->bpm = BASS_LINE[slab].bpm;
    p->pump = BASS_LINE[slab].pump;
    p->feedback = BASS_LINE[slab].feedback;
    p->wobble = BASS_LINE[slab].wobble;
    p->drum_pat = slab;
    p->root = BASS_LINE_ROOT[slab];
    p->scale = BASS_LINE_SCALE[slab];
    p->swing = BASS_LINE_SWING[slab];
}

static inline uint8_t dub_step_bit(uint32_t mask, uint8_t step)
{
    return (uint8_t)((mask >> (step & (DUB_STEPS - 1u))) & 1u);
}

static inline void dub_resolve_step(const dub_params_t *p, const dub_step_t *ovr, uint8_t step, dub_events_t *out, uint8_t apply_dens)
{
    const bass_line_t *main;
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t forced;

    if (!p || !out)
        return;
    if (step >= DUB_STEPS)
        step = (uint8_t)(DUB_STEPS - 1u);
    main = bass_line(p->plate);
    forced = (ovr != 0) && ((ovr->force & 1u) != 0);
    if (forced) {
        kick = (ovr->gate >> 0) & 1u;
        rim = (ovr->gate >> 1) & 1u;
        hat = (ovr->gate >> 2) & 1u;
        siren = (ovr->gate >> 3) & 1u;
        sub = (ovr->gate >> 4) & 1u;
        chord = (ovr->gate >> 5) & 1u;
    } else {
        kick = dub_step_bit((uint16_t)main->kick, step);
        rim = dub_step_bit((uint16_t)main->rim, step);
        hat = dub_step_bit((uint16_t)main->hat, step);
        siren = dub_step_bit((uint16_t)main->siren, step);
        sub = dub_step_bit(main->sub, step);
        chord = dub_step_bit((uint16_t)main->chord, step);
    }
    (void)apply_dens;
    out->kick = kick;
    out->rim = rim;
    out->hat = hat;
    out->siren = siren;
    out->sub = sub;
    out->chord = chord;
    out->note = forced ? ovr->note : 0u;
    out->dly_throw = forced ? ovr->dly_throw : 0u;
    out->rev_throw = forced ? ovr->rev_throw : 0u;
    out->accent = forced ? ovr->accent : 0u;
}

static inline void dub_plate_groove_read(uint8_t plate, uint8_t *root, uint8_t *scale, uint8_t *swing, uint8_t *bpm)
{
    const bass_line_t *sc;

    plate = bass_clamp_line(plate);
    sc = bass_line(plate);
    if (root)
        *root = BASS_LINE_ROOT[plate];
    if (scale)
        *scale = BASS_LINE_SCALE[plate];
    if (swing)
        *swing = BASS_LINE_SWING[plate];
    if (bpm)
        *bpm = sc->bpm;
}
'''


def emit_bake() -> str:
    return r'''
#include "bass_seq.h"

static inline uint8_t bass_style_gate_u8(uint8_t slab, uint8_t st)
{
    uint8_t len;

    if (!bass_line_lead_step_bit(slab, st))
        return 0u;
    len = bass_line_lead_len_ix(slab, st);
    if (len >= 9u && len <= 16u)
        return (uint8_t)(len - 8u);
    return 4u;
}

static inline uint8_t bass_bake_line_pick(uint8_t lo_line, uint8_t hi_line, uint8_t frac, uint8_t abs_st)
{
    uint8_t lo = bass_line_lead_step_bit(lo_line, abs_st);
    uint8_t hi = bass_line_lead_step_bit(hi_line, abs_st);

    if (frac < 128u)
        return lo;
    return hi;
}

static inline void bass_genre_bake_one(bass_step_t *step, uint8_t lo_line, uint8_t hi_line, uint8_t frac,
                                       uint8_t abs_st, uint8_t era)
{
    uint8_t sub_on;
    uint8_t gate;
    int8_t pitch;
    uint8_t slot;
    uint8_t dens;
    uint8_t acc_on;
    uint8_t slide_on;
    uint8_t pick_line;
    uint8_t st_in_bar;

    if (!step || step->lock)
        return;
    era = bass_clamp_era(era);
    st_in_bar = (uint8_t)(abs_st & 15u);
    sub_on = bass_bake_line_pick(lo_line, hi_line, frac, abs_st);
    pick_line = (frac < 128u) ? lo_line : hi_line;
    gate = sub_on ? bass_style_gate_u8(pick_line, st_in_bar) : 0u;
    step->gate = gate;
    pitch = bass_line_lead_pitch(pick_line, st_in_bar);
    if (sub_on && st_in_bar == 0u)
        pitch = 0;
    step->note = (sub_on && pitch) ? (uint8_t)(int8_t)pitch : 0u;
    slot = (uint8_t)(lo_line % GENRE_PRESETS_PER_ERA);
    if (slot >= GENRE_PRESETS_PER_ERA)
        slot = (uint8_t)(GENRE_PRESETS_PER_ERA - 1u);
    dens = BASS_ERA_ACCENT_DENS[era][slot];
    acc_on = 0u;
    if (sub_on && (st_in_bar == 0u || st_in_bar == 8u)) {
        if (st_in_bar == 0u)
            acc_on = 1u;
        else if (dens > 36u && ((abs_st * 13u + era * 19u) % 100u) < (dens / 4u))
            acc_on = 1u;
    }
    step->accent = acc_on;
    slide_on = 0u;
    if (sub_on)
        slide_on = bass_line_lead_bend_u7(pick_line, abs_st) ? 1u : 0u;
    step->slide = slide_on;
    step->ratchet = 1u;
    if (sub_on && bass_step_ratchet_allowed(slide_on, gate)) {
        if (st_in_bar == 8u && ((era + slot + abs_st) % 11u) == 0u)
            step->ratchet = 2u;
    }
}

static inline void bass_genre_bake(bass_step_t *steps, uint8_t n, uint8_t fader_pos, uint8_t era)
{
    uint8_t base;
    uint8_t per;
    uint32_t pos;
    uint8_t lo;
    uint8_t hi;
    uint8_t frac;
    uint8_t st;

    if (!steps)
        return;
    if (n == 0u)
        n = 16u;
    era = bass_clamp_era(era);
    base = bass_era_first_line(era);
    per = GENRE_PRESETS_PER_ERA;
    if (per < 2u)
        per = 2u;
    pos = ((uint32_t)fader_pos * (uint32_t)(per - 1u) * 256u) / 127u;
    lo = (uint8_t)(base + (pos >> 8));
    hi = (uint8_t)(lo + 1u);
    if (hi >= base + per)
        hi = (uint8_t)(base + per - 1u);
    frac = (uint8_t)(pos & 255u);
    for (st = 0u; st < n; st++) {
        if (steps[st].lock)
            continue;
        bass_genre_bake_one(&steps[st], lo, hi, frac, st, era);
    }
}

static inline const char *bass_genre_fader_name(uint8_t fader, uint8_t era)
{
    uint8_t base;
    uint8_t per;
    uint32_t pos;
    uint8_t lo;

    era = bass_clamp_era(era);
    base = bass_era_first_line(era);
    per = GENRE_PRESETS_PER_ERA;
    if (per < 2u)
        per = 2u;
    pos = ((uint32_t)fader * (uint32_t)(per - 1u) * 256u) / 127u;
    lo = (uint8_t)(base + (pos >> 8));
    if (lo >= base + per)
        lo = (uint8_t)(base + per - 1u);
    return bass_line_name(lo);
}

static inline void dub_drm_voice_kit_fill(dub_params_t *p, uint8_t tune, uint8_t decay, uint8_t drive)
{
    (void)p;
    (void)tune;
    (void)decay;
    (void)drive;
}

static inline void dub_drm_send_fixup(dub_params_t *p)
{
    (void)p;
}

static inline void bass_genre_fader_bake(dub_step_t *seq, bass_step_x_t *ext, uint8_t fader, uint8_t era,
                                         uint8_t nsteps)
{
    uint8_t base;
    uint8_t per;
    uint32_t pos;
    uint8_t lo;
    uint8_t hi;
    uint8_t frac;
    uint8_t st;

    if (!seq || !ext)
        return;
    if (nsteps == 0u)
        nsteps = 16u;
    era = bass_clamp_era(era);
    base = bass_era_first_line(era);
    per = GENRE_PRESETS_PER_ERA;
    if (per < 2u)
        per = 2u;
    pos = ((uint32_t)fader * (uint32_t)(per - 1u) * 256u) / 127u;
    lo = (uint8_t)(base + (pos >> 8));
    hi = (uint8_t)(lo + 1u);
    if (hi >= base + per)
        hi = (uint8_t)(base + per - 1u);
    frac = (uint8_t)(pos & 255u);
    for (st = 0u; st < nsteps; st++) {
        bass_step_t tmp;

        if (ext[st].lock)
            continue;
        tmp.note = seq[st].note;
        tmp.gate = ext[st].gate;
        tmp.accent = ext[st].accent;
        tmp.slide = ext[st].slide;
        tmp.ratchet = ext[st].ratchet;
        tmp.lock = 0u;
        bass_genre_bake_one(&tmp, lo, hi, frac, st, era);
        if (tmp.gate) {
            if (tmp.note)
                seq[st].note = tmp.note;
            seq[st].gate = (uint8_t)(1u << 4);
        } else {
            seq[st].note = 0u;
            seq[st].gate = 0u;
        }
        ext[st].gate = tmp.gate;
        ext[st].accent = tmp.accent;
        ext[st].slide = tmp.slide;
        ext[st].ratchet = tmp.ratchet;
    }
}
'''


def lead_tables() -> tuple[str, str, str, str, str]:
    gates = []
    bar_rows = []
    pitch_rows = []
    len_rows = []
    bend_rows = []
    for i in range(PRESET_N):
        era = i // PRESETS_PER_ERA
        ix = i % PRESETS_PER_ERA
        bars = line_bar_gates(era, ix)
        g16 = bars[0]
        g = bars[0] | (bars[1] << 16)
        gates.append(f"    0x{g:08X}u")
        bar_rows.append("    { " + ", ".join(f"0x{b:04X}u" for b in bars) + " }")
        pitches = line_pitches128(era, ix)
        pitch_rows.append("    { " + ", ".join(f"{p:>3}" for p in pitches) + " }")
        len_rows.append("    { " + ", ".join(line_lens(era, ix, g16)) + " }")
        bend_rows.append("    { " + ", ".join(line_bends(era, ix, g16)) + " }")
    return (
        ",\n".join(gates),
        ",\n".join(bar_rows),
        ",\n".join(pitch_rows),
        ",\n".join(len_rows),
        ",\n".join(bend_rows),
    )


OWNED_HEADS = [
    "static const uint32_t BASS_LINE_LEAD_GATE[BASS_LINE_N] = {\n",
    "static const uint16_t BASS_LINE_BAR_GATE[BASS_LINE_N][8] = {\n",
    "static const int8_t BASS_LINE_LEAD_PITCH[BASS_LINE_N][128] = {\n",
    "static const uint8_t BASS_LINE_LEAD_LEN[BASS_LINE_N][128] = {\n",
    "static const uint8_t BASS_LINE_LEAD_BEND[BASS_LINE_N][128] = {\n",
]


def patch_header() -> int:
    """Patch only the five data blocks this generator owns in firmware/src/bass_data.h:
    BASS_LINE_LEAD_GATE, BASS_LINE_BAR_GATE, BASS_LINE_LEAD_PITCH, BASS_LINE_LEAD_LEN, BASS_LINE_LEAD_BEND.

    Everything else (typedefs, plate table, era tables, helpers, bake, bass_gen.h integration) stays byte-identical.
    Idempotent: re-running with unchanged generator output rewrites the same bytes.
    """
    text = OUT.read_text()
    gates, bar_rows, pitch_rows, len_rows, bend_rows = lead_tables()
    blocks = dict(zip(OWNED_HEADS, [gates, bar_rows, pitch_rows, len_rows, bend_rows]))
    for head in OWNED_HEADS:
        n = text.count(head)
        if n != 1:
            print(f"block head found {n}x (want 1): {head[:70]!r}", file=sys.stderr)
            return 1
    for head in OWNED_HEADS:
        start = text.index(head) + len(head)
        end = text.index("\n};", start)
        text = text[:start] + blocks[head] + text[end:]
    text = _patch_plate_sub_field(text)
    OUT.write_text(text)
    print(f"patched {len(OWNED_HEADS)} owned blocks + BASS_LINE.sub in {OUT}")
    return 0


PLATE_HEAD = "static const bass_line_t BASS_LINE[BASS_LINE_N] = {\n"
_SUB_FIELD = re.compile(r"0x[0-9A-Fa-f]{4}u \| \(\(uint32_t\)0x[0-9A-Fa-f]{4}u << 16\)")


def _patch_plate_sub_field(text: str) -> str:
    """BASS_LINE rows carry a copy of the lead gate (bar0 | bar1 << 16) as their first 3-field group ('sub').
    Only that field is rewritten; curated plate names and the other fields are left as they are."""
    if text.count(PLATE_HEAD) != 1:
        raise SystemExit("BASS_LINE block head not found exactly once")
    start = text.index(PLATE_HEAD) + len(PLATE_HEAD)
    end = text.index("\n};", start)
    rows = text[start:end].split("\n")
    if len(rows) != PRESET_N:
        raise SystemExit(f"BASS_LINE has {len(rows)} rows, want {PRESET_N}")
    for i, row in enumerate(rows):
        era, ix = divmod(i, PRESETS_PER_ERA)
        g32 = line_gate32(era, ix)
        new = f"0x{g32 & 0xFFFF:04X}u | ((uint32_t)0x{(g32 >> 16) & 0xFFFF:04X}u << 16)"
        rows[i] = _SUB_FIELD.sub(lambda _m, n=new: n, row, count=1)
    return text[:start] + "\n".join(rows) + text[end:]


def main() -> int:
    if "--patch-header" in sys.argv:
        return patch_header()
    print("WARNUNG: volle Regeneration ist veraltet (kein bass_gen.h); bevorzugt --patch-header", file=sys.stderr)
    plates = []
    for era in range(ERA_N):
        for ix in range(PRESETS_PER_ERA):
            plates.append(gen_plate(era, ix))

    roots = ", ".join(str(36 + (i % 12)) for i in range(PRESET_N))
    scales = ", ".join(str(i % 6) for i in range(PRESET_N))
    swings = ", ".join(str(58 + (i % 8)) for i in range(PRESET_N))
    kits = ", ".join(str(i % DKIT_N) for i in range(PRESET_N))

    era_base = [
        (30, 82, 90, 24, 40, 92, 96, 30, 22, 32, 0, 0, 0, 64, 64, 8, 0, 0, 36, (64, 64, 64, 64), 12, 56, 64, 62),
        (28, 78, 94, 16, 36, 88, 98, 36, 14, 28, 0, 0, 0, 64, 64, 6, 0, 0, 32, (64, 64, 64, 64), 10, 52, 64, 60),
        (34, 86, 88, 28, 44, 84, 92, 28, 20, 30, 0, 0, 0, 64, 64, 10, 0, 0, 38, (64, 64, 64, 64), 14, 54, 64, 64),
        (32, 90, 86, 32, 48, 80, 88, 24, 18, 34, 0, 0, 0, 64, 64, 12, 0, 0, 40, (64, 64, 64, 64), 16, 50, 64, 58),
        (36, 74, 82, 36, 52, 76, 84, 22, 26, 38, 0, 0, 0, 64, 64, 14, 0, 0, 42, (64, 64, 64, 64), 18, 48, 64, 66),
        (30, 88, 90, 20, 42, 86, 94, 26, 16, 36, 0, 0, 0, 64, 64, 8, 0, 0, 34, (64, 64, 64, 64), 12, 58, 64, 62),
    ]
    era_fx = [
        (0, 24, 1, 40, 14, 62, 38, 34, 56, 48, 18, 4, 14),
        (1, 28, 0, 38, 12, 58, 36, 32, 54, 44, 14, 1, 10),
        (2, 20, 0, 42, 16, 54, 34, 30, 52, 46, 16, 7, 18),
        (3, 22, 0, 44, 10, 56, 40, 28, 50, 42, 12, 8, 22),
        (4, 18, 0, 46, 18, 52, 32, 26, 48, 40, 20, 10, 24),
        (5, 26, 2, 48, 14, 60, 42, 36, 58, 50, 22, 11, 16),
    ]

    def fmt_base(r):
        pans = ", ".join(str(p) for p in r[19])
        return (
            f"    {{ {r[0]}, {r[1]}, {r[2]}, {r[3]}, {r[4]}, {r[5]}, {r[6]}, {r[7]}, "
            f"{r[8]}, {r[9]}, {r[10]}, {r[11]}, {r[12]}, {r[13]}, {r[14]}, {r[15]}, "
            f"{r[16]}, {r[17]}, {r[18]}, {{{pans}}}, {r[20]}, {r[21]}, {r[22]}, {r[23]} }}"
        )

    def fmt_fx(r):
        return f"    {{ {r[0]}, {r[1]}, {r[2]}, {r[3]}, {r[4]}, {r[5]}, {r[6]}, {r[7]}, {r[8]}, {r[9]}, {r[10]}, {r[11]}, {r[12]} }}"

    gates, bar_gates, pitches, lens, bends = lead_tables()

    era_degree = []
    era_accent = []
    for e in range(ERA_N):
        deg = ", ".join(str((e + s) % 7) for s in range(16))
        era_degree.append(f"    {{ {deg} }}")
        era_accent.append(f"    {{ {28 + e * 2}, {32}, {24 + e}, {36}, {26}, {30}, {22}, {34}, {28}, {32} }}")

    body = f"""/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef BASS_DATA_H
#define BASS_DATA_H

#include "dub_types.h"
#include "bass_conf.h"
#include "fm_seq.h"

enum {{ BASS_ERA_N = GENRE_ERA_N, BASS_LINE_N = GENRE_PRESET_N }};
enum {{ ERA_ACID, ERA_SUB, ERA_REESE, ERA_FM, ERA_FOLD, ERA_SYNC }};
enum {{
    BASS_RUMBLE = 0, BASS_SUB909, BASS_EBM16, BASS_HOOVER, BASS_ACID303, BASS_DETROIT,
    BASS_SCHRANZ, BASS_REESE, BASS_MODFM, BASS_DRONE, BASS_FOLD, BASS_SYNC, BASS_PWM, BASS_WAVEFOLD, BASS_N
}};
enum {{ DKIT_SUB = 0, DKIT_DEEP, DKIT_TIGHT, DKIT_WIDE, DKIT_FM, DKIT_SYNC, DKIT_ACID, DKIT_DRONE, DKIT_N }};
enum {{ VOICE_ROOT = 0, VOICE_FIFTH, VOICE_OCT, VOICE_MIN7, VOICE_MIN9, VOICE_SUS4, VOICE_DIM, VOICE_CLUSTER, VOICE_N }};

typedef dub_plate_t bass_line_t;

static const char *const BASS_ERA_NAMES[BASS_ERA_N] = {{
    {", ".join(f'"{n}"' for n in ERA_NAMES)}
}};

static const char *const DKIT_NAMES[DKIT_N] = {{
    {", ".join(f'"{n}"' for n in DKIT_NAMES)}
}};
static const uint8_t DKIT_ERA[DKIT_N] = {{ {", ".join(str(x) for x in DKIT_ERA)} }};

static const char *const BASS_NAMES[BASS_N] = {{
    {", ".join(f'"{n}"' for n in BASS_NAMES)}
}};
static const uint8_t BASS_ERA[BASS_N] = {{ {", ".join(str(x) for x in BASS_ERA)} }};

static const char *const VOICE_NAMES[VOICE_N] = {{
    {", ".join(f'"{n}"' for n in VOICE_NAMES)}
}};

static const bass_line_t BASS_LINE[BASS_LINE_N] = {{
{",\n".join(plates)}
}};

static const int8_t BASS_LINE_ROOT[BASS_LINE_N] = {{ {roots} }};
static const int8_t BASS_LINE_SCALE[BASS_LINE_N] = {{ {scales} }};
static const int8_t BASS_LINE_SWING[BASS_LINE_N] = {{ {swings} }};
static const int8_t BASS_LINE_KIT[BASS_LINE_N] = {{ {kits} }};

static const uint8_t BASS_ERA_MACROS[BASS_ERA_N][10] = {{
{",\n".join(f"    {{ {', '.join(str(40 + e * 2 + i) for i in range(10))} }} /* {ERA_NAMES[e]} */" for e in range(ERA_N))}
}};

static const uint8_t BASS_ERA_DEGREE[BASS_ERA_N][16] = {{
{",\n".join(era_degree)}
}};

static const uint8_t BASS_ERA_ACCENT_DENS[BASS_ERA_N][10] = {{
{",\n".join(era_accent)}
}};

{emit_typedefs()}

static const bass_era_base_t BASS_ERA_BASE[BASS_ERA_N] = {{
{",\n".join(fmt_base(r) for r in era_base)}
}};

static const bass_era_fx_t BASS_ERA_FX[BASS_ERA_N] = {{
{",\n".join(fmt_fx(r) for r in era_fx)}
}};

static const uint32_t BASS_LINE_LEAD_GATE[BASS_LINE_N] = {{
{gates}
}};

static const uint16_t BASS_LINE_BAR_GATE[BASS_LINE_N][8] = {{
{bar_gates}
}};

static const int8_t BASS_LINE_LEAD_PITCH[BASS_LINE_N][16] = {{
{pitches}
}};

static const uint8_t BASS_LINE_LEAD_LEN[BASS_LINE_N][128] = {{
{lens}
}};

static const uint8_t BASS_LINE_LEAD_BEND[BASS_LINE_N][128] = {{
{bends}
}};

{emit_helpers()}
{emit_bake()}
#endif
"""
    OUT.write_text(body)
    print(f"wrote {OUT} ({len(body)} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
