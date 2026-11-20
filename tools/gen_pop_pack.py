#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate KO POP pack tables in firmware/src/pop_data.h (@pop-pack markers)."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
POP_DATA = ROOT / "firmware" / "src" / "pop_data.h"

ERA_N = 6
TRACKS_PER_ERA = 10
TRACK_N = ERA_N * TRACKS_PER_ERA

MINOR_PROGS = [
    [0, 5, 2, 6], [0, 6, 5, 6], [0, 3, 6, 2], [0, 5, 6, 0], [0, 4, 5, 3], [0, 0, 5, 6], [0, 3, 0, 6],
]
MAJOR_PROGS = [
    [0, 4, 5, 3], [5, 3, 0, 4], [0, 5, 3, 4], [0, 3, 5, 4], [3, 4, 2, 5], [0, 6, 3, 0], [0, 2, 3, 4],
]
DORIAN_PROGS = [[0, 3, 0, 3], [0, 3, 6, 3], [0, 1, 2, 1]]
ALL_PROGS = MINOR_PROGS + MAJOR_PROGS + DORIAN_PROGS
PROG_SCALE_POOL = [1] * len(MINOR_PROGS) + [4] * len(MAJOR_PROGS) + [2] * len(DORIAN_PROGS)

SCALE_SEMIS = {
    1: [0, 2, 3, 5, 7, 8, 10],
    2: [0, 2, 3, 5, 7, 9, 10],
    4: [0, 2, 4, 5, 7, 9, 11],
}
NOTE_NAMES = ["C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"]

ERA_SCALE_BIAS = {
    0: [1, 2, 1, 2, 1, 2, 1, 2, 1, 2],
    1: [4, 4, 4, 1, 4, 4, 2, 4, 4, 1],
    2: [1, 2, 1, 4, 2, 1, 2, 1, 4, 2],
    3: [1, 1, 2, 1, 2, 1, 1, 2, 1, 2],
    4: [4, 1, 4, 1, 4, 1, 4, 1, 4, 1],
    5: [1, 1, 2, 1, 1, 2, 1, 1, 2, 1],
}

ERA_BPM = {0: (108, 126), 1: (100, 124), 2: (112, 128), 3: (116, 126), 4: (76, 98), 5: (114, 128)}

# Lead variant per track within era: 0..3, no adjacent duplicates, each used ≥2× per era.
ERA_LEAD_VAR_SEQ = {
    0: [0, 1, 2, 3, 1, 2, 3, 0, 2, 3],
    1: [1, 2, 3, 0, 2, 3, 0, 1, 3, 0],
    2: [2, 3, 0, 1, 3, 0, 1, 2, 0, 1],
    3: [3, 0, 1, 2, 0, 1, 2, 3, 1, 2],
    4: [0, 2, 1, 3, 2, 1, 3, 0, 1, 3],
    5: [1, 3, 0, 2, 3, 0, 2, 1, 0, 2],
}

KEY_PCS = [5, 7, 9, 10, 0, 2, 3, 4, 11, 1, 6, 8, 5, 7, 9, 10, 0, 2, 3, 4, 11, 1, 6, 8, 5, 7, 9, 10, 0, 2, 3, 4, 11, 1, 6, 8, 5, 7, 9, 10, 0, 2, 3, 4, 11, 1, 6, 8, 5, 7, 9, 10, 0, 2, 3, 4, 11, 1, 6, 8]

BASS_ARCH = ["OCT8", "PULSE8", "SEQ16", "FUNK", "ROOT4", "GALLOP", "MELODIC", "LONG"]
BASS_TEMPLATE = {
    "OCT8": "RORORORORORORORO",
    "PULSE8": "R.R.R.R.R.R.5.O.",
    "SEQ16": "RORORORORORORORO",
    "FUNK": "R..ROR.R5OR.A...",
    "ROOT4": "R...R...R...R...",
    "GALLOP": "R.RR.RR.R.RR.RR.",
    "MELODIC": "R.3.5.O.5.3.R.A.",
    "LONG": "R...............",
}
SYM_MAP = {".": 0, "R": 1, "O": 2, "5": 3, "3": 4, "A": 5}
SYM_REV = {0: ".", 1: "R", 2: "O", 3: "5", 4: "3", 5: "A"}

ERA_BASS_ARCH = {
    0: ["SEQ16", "PULSE8", "ROOT4", "PULSE8", "SEQ16", "ROOT4", "PULSE8", "SEQ16", "ROOT4", "PULSE8"],
    1: ["ROOT4", "MELODIC", "PULSE8", "ROOT4", "MELODIC", "PULSE8", "ROOT4", "MELODIC", "PULSE8", "ROOT4"],
    2: ["FUNK", "OCT8", "GALLOP", "FUNK", "OCT8", "GALLOP", "FUNK", "OCT8", "GALLOP", "FUNK"],
    3: ["OCT8", "GALLOP", "SEQ16", "OCT8", "GALLOP", "SEQ16", "OCT8", "GALLOP", "SEQ16", "OCT8"],
    4: ["LONG", "ROOT4", "MELODIC", "LONG", "ROOT4", "MELODIC", "LONG", "ROOT4", "MELODIC", "LONG"],
    5: ["PULSE8", "OCT8", "SEQ16", "PULSE8", "OCT8", "SEQ16", "PULSE8", "OCT8", "SEQ16", "PULSE8"],
}

# 10 unique kick pickups per era (always includes step 0)
FOUR_FLOOR_ERAS = {3, 5}
FOUR_FLOOR_STEPS = (0, 4, 8, 12)

# Pickups only (always merged with four-floor base on eras 3/5).
ERA_KICK_PICKUPS = {
    0: [(8,), (10,), (6,), (8, 10), (6, 11), (8, 14), (3, 8), (5, 8), (8, 12), (6, 10)],
    1: [(8,), (10,), (6, 8), (8, 11), (9,), (6,), (8, 10), (5, 8), (8, 14), (7, 10)],
    2: [(8,), (8, 11), (8, 10), (5, 8), (11,), (6, 10), (5, 11), (9,), (7, 10), (14,)],
    3: [(6,), (10,), (14,), (3,), (6, 10), (11,), (7,), (9,), (5,), (13,)],
    4: [(10,), (14,), (8,), (10, 14), (12,), (6, 10), (14, 8), (11,), (9, 10), (13,)],
    5: [(6,), (10,), (14,), (3,), (6, 10), (11,), (7,), (9,), (5,), (13,)],
}

# CHROME: ≥5 funk kicks (syncopation on 3/6/10/11), rest machine + pickups.
CHROME_FUNK_IX = {0, 1, 3, 4, 6, 8}
CHROME_FUNK_KICKS = [
    (3, 6, 10, 11),
    (3, 6, 11),
    (3, 10, 11),
    (6, 10, 11),
    (3, 6, 10),
    (3, 11, 14),
]
CHROME_SWING_BY_IX = {2: 53, 4: 54, 5: 55, 8: 56}

# Hat bases: bit n = step n. Even 8ths = 0x5555; Hi-NRG open offbeats = 2,6,10,14 = 0x4444.
HAT_8TH_EVEN = 0x5555
HAT_OFFBEAT_OPEN = 0x4444

# ghost snare steps (core 4+12 or BLUE 8)
ERA_SNARE_GHOST = {
    0: [(), (2,), (6,), (10,), (2, 6), (14,), (7,), (3,), (2, 10), (6, 14)],
    1: [(), (2,), (6,), (10,), (2, 7), (14,), (3,), (6, 10), (2,), (15,)],
    2: [(), (2,), (6,), (10,), (2, 6), (14,), (3, 7), (6,), (10, 14), (2, 10)],
    3: [(), (2,), (6,), (10,), (2, 6), (14,), (7,), (3,), (6, 10), (2, 14)],
    4: [(2,), (6,), (10,), (14,), (3,), (7,), (11,), (15,), (1,), (5,)],  # ghosts around halftime snare
    5: [(), (2,), (6,), (10,), (2, 6), (14,), (7,), (3,), (6, 10), (2, 14)],
}

# Per-era hat seeds (built into unique masks in hat_mask_for_track).
ERA_HAT_KIND = {
    0: "8th",  # COLD
    1: "16th",  # RADIO
    2: "mix",  # CHROME
    3: "offbeat",  # ITALO
    4: "8th",  # BLUE
    5: "offbeat",  # AFTER
}

MELO_RHYTHM = {
    0: [0x1111, 0x1011, 0x1101, 0x1110, 0x0111, 0x1010, 0x0101, 0x1001, 0x0110, 0x1100],
    1: [0x89AB, 0x8A9B, 0x9A8B, 0xA89B, 0x98AB, 0xAB89, 0xB89A, 0x9AB8, 0xA98B, 0xB9A8],
    2: [0xF0F0, 0xF3F0, 0xF0F3, 0xC3C3, 0xFCCF, 0x3C3C, 0xF0CF, 0xCFF0, 0xF3CF, 0xCFC3],
    3: [0xFFFF, 0xFFEF, 0xFEFF, 0xEFFF, 0xFFF7, 0xF7FF, 0xFFFB, 0xFBFF, 0xFFFD, 0xFDFF],
    4: [0x1000, 0x1001, 0x1010, 0x1000, 0x0010, 0x0100, 0x1000, 0x0001, 0x0010, 0x0100],
    5: [0x3333, 0x3033, 0x3303, 0x0333, 0x3131, 0x1313, 0x3330, 0x3030, 0x0303, 0x0033],
}

LEAP_TABLE = [0, 2, 4, 5, 7, 9, 11, 12]
MOTIF_SHAPES = [
    [0, 2, 0, 4],
    [0, 3, 2, 0],
    [0, 4, 2, 0],
    [0, 2, 4, 2],
    [0, 5, 4, 0],
    [0, 2, 3, 2],
    [0, 1, 2, 0],
    [0, 4, 5, 4],
    [0, 3, 5, 3],
    [0, 6, 4, 0],
]


def parse_track_names(text: str) -> list[str]:
    lines = text.splitlines()
    block: list[str] = []
    in_plate = False
    for line in lines:
        if "static const pop_plate_t POP_PLATE" in line:
            in_plate = True
        if in_plate:
            block.append(line)
            if line.strip() == "};":
                break
    if not block:
        raise SystemExit("POP_PLATE table not found")
    names = re.findall(r'\{\s*"([A-Z0-9]+)"\s*,', "\n".join(block))
    if len(names) != TRACK_N:
        raise SystemExit(f"expected {TRACK_N} track names, got {len(names)}")
    return names


def mask16(steps: tuple[int, ...]) -> int:
    m = 0
    for s in steps:
        if 0 <= s < 16:
            m |= 1 << s
    return m & 0xFFFF


def deg_note(root: int, scale: int, deg: int) -> int:
    sem = SCALE_SEMIS[scale]
    d = deg % 7
    oct = deg // 7
    return root // 12 * 12 + root % 12 + oct * 12 + sem[d]


def note_name(midi: int) -> str:
    return NOTE_NAMES[midi % 12]


def assign_prog_triples() -> list[int]:
    used: set[tuple[int, int, int]] = set()
    progs: list[int] = []
    for t in range(TRACK_N):
        era = t // TRACKS_PER_ERA
        ix = t % TRACKS_PER_ERA
        scale = ERA_SCALE_BIAS[era][ix]
        pc = KEY_PCS[t]
        root = 60 + pc - (60 % 12)
        while root < 54:
            root += 12
        while root > 65:
            root -= 12
        pool = [i for i, sc in enumerate(PROG_SCALE_POOL) if sc == scale]
        for off in range(len(pool) * 2):
            pi = pool[(ix + off) % len(pool)]
            key = (pi, pc, scale)
            if key not in used:
                used.add(key)
                progs.append(pi)
                break
        else:
            progs.append(pool[ix % len(pool)])
    return progs


def _odd_steps(m: int) -> int:
    return sum(1 for s in range(16) if m & (1 << s) and s & 1)


def _even_steps(m: int) -> int:
    return sum(1 for s in range(16) if m & (1 << s) and (s & 1) == 0)


def hat_mask_for_track(era: int, ix: int) -> int:
    kind = ERA_HAT_KIND[era]
    chrome_16 = kind == "mix" and ix in (1, 2, 4, 7, 9)
    if kind == "16th" or chrome_16:
        a = 1 << ((ix + era + 2) % 16)
        b = 1 << ((ix * 3 + 7) % 16)
        return (0xFFFF ^ a ^ b ^ (1 << (ix % 16))) & 0xFFFF
    if kind == "offbeat":
        m = HAT_OFFBEAT_OPEN
        if ix & 1:
            m |= 0x00FF
        m ^= (1 << ((ix * 2 + 4) % 16 | 0)) & 0xFF00
        m |= HAT_OFFBEAT_OPEN
        return m & 0xFFFF
    m = HAT_8TH_EVEN
    m ^= 1 << ((ix * 5 + 2) % 16 & ~1)
    for g in range(ix % 3):
        odd = 1 + ((ix * 5 + g * 3 + era) % 7) * 2
        if odd < 16:
            m |= 1 << odd
    for s in range(16):
        if s & 1 and _odd_steps(m) > 2:
            m &= ~(1 << s)
    return m & 0xFFFF


def kick_mask_for_track(era: int, ix: int) -> int:
    if era in FOUR_FLOOR_ERAS:
        extra = ERA_KICK_PICKUPS[era][ix]
        steps = tuple(sorted(set(FOUR_FLOOR_STEPS) | set(extra)))
        return mask16(steps)
    if era == 2 and ix in CHROME_FUNK_IX:
        fk = CHROME_FUNK_KICKS[ix % len(CHROME_FUNK_KICKS)]
        return mask16((0,) + tuple(fk))
    extra = ERA_KICK_PICKUPS[era][ix]
    return mask16((0,) + tuple(extra))


def _bass_hit_symbols(row: list[int]) -> list[int]:
    return [row[s] for s in range(16) if row[s]]


def _max_symbol_run(syms: list[int]) -> int:
    if not syms:
        return 0
    best = 1
    run = 1
    for i in range(1, len(syms)):
        if syms[i] == syms[i - 1]:
            run += 1
            best = max(best, run)
        else:
            run = 1
    return best


def normalize_oct8_row(row: list[int], ix: int = 0) -> list[int]:
    """Hi-NRG bounce: alternate R/O on 8ths; ≤2 same symbol in a row; one odd R pickup max."""
    evens = list(range(0, 16, 2))
    pickup = next((s for s in range(1, 16, 2) if row[s] == 1), None)
    for s in range(16):
        row[s] = 0
    for i, s in enumerate(evens):
        row[s] = 1 if i % 2 == 0 else 2
    if pickup is not None:
        row[pickup] = 1
    o_slots = [s for s in evens if row[s] == 2]
    if len(o_slots) >= 4:
        rep = o_slots[(1 + ix) % len(o_slots)]
        row[rep] = 3 if rep % 4 == 2 else 4
    row[15] = 5
    for s in range(15):
        if row[s] == 5 and s != 15:
            row[s] = 1 if (s // 2) % 2 == 0 else 2
    while _max_symbol_run(_bass_hit_symbols(row)) > 2:
        for s in evens:
            if row[s] == 2:
                row[s] = 1
                break
    return row


def build_oct8_row(era: int, ix: int, t: int) -> list[int]:
    row = [0] * 16
    salt = t * 19 + ix * 7 + era
    if (ix + salt) % 4 == 0:
        for s in range(16):
            row[s] = 1 if s % 2 == 0 else 2
    else:
        for i, s in enumerate(range(0, 16, 2)):
            row[s] = 1 if i % 2 == 0 else 2
    if salt % 5 == 0:
        for s in range(0, 16, 2):
            if row[s] == 2 and s < 12:
                row[s] = 3 if (salt & 1) else 4
                break
    if salt % 7 == 0:
        p = 1 + ((ix + salt) % 7) * 2
        if p < 16:
            row[p] = 1
    row = normalize_oct8_row(row, ix)
    if (salt + ix) % 2 == 0:
        row[15] = 5
    return row


def build_pulse8_row(era: int, ix: int, t: int) -> list[int]:
    row = [0] * 16
    salt = t * 23 + ix * 11 + era * 5
    for s in range(0, 10, 2):
        row[s] = 1
    if salt % 5 == 0:
        row[6] = 3
    elif salt % 5 == 2:
        row[4] = 2
    tv = salt % 8
    turns_lut = [
        (1, 3, 2),
        (1, 2, 5),
        (3, 1, 2),
        (1, 3, 5),
        (1, 2, 2),
        (3, 3, 2),
        (1, 5, 2),
        (2, 3, 5),
    ]
    row[10], row[12], row[14] = turns_lut[tv]
    for s in range(1, 16, 2):
        row[s] = 0
    if row[14] == 5:
        pass
    elif (ix + t) % 4 == 0 and row[14] != 5:
        row[14] = 5
    return row


def build_bass_row(arch: str, era: int, ix: int, t: int) -> list[int]:
    if arch == "OCT8":
        return build_oct8_row(era, ix, t)
    if arch == "PULSE8":
        return build_pulse8_row(era, ix, t)

    tmpl = list(BASS_TEMPLATE[arch][:16].ljust(16, "."))
    row = [SYM_MAP.get(ch, 0) if ch != "." else 0 for ch in tmpl]
    row[0] = 1
    sym_cycle = [1, 2, 1, 3, 1, 4, 1, 5, 1, 3, 2, 1, 4, 3, 5, 2]

    if arch == "LONG":
        row[12] = row[12] or 1
    elif arch == "ROOT4":
        for s in (0, 4, 8, 12):
            row[s] = row[s] or 1

    for s in range(13):
        if row[s] == 5:
            row[s] = 1

    if arch not in ("LONG", "ROOT4"):
        if row[14] == 0 and (t + ix) % 3 != 1:
            row[14] = 5
        if row[15] == 0 and (t + ix) % 5 in (0, 2):
            row[15] = 5

    salt = t * 19 + ix * 7 + era
    for bit in range(4):
        s = 1 + (salt + bit * 11) % 15
        if tmpl[s] == "." and arch in ("SEQ16", "FUNK", "GALLOP", "MELODIC"):
            if row[s] == 0:
                row[s] = sym_cycle[(salt + s) % len(sym_cycle)]
                if row[s] == 5:
                    row[s] = 1

    for s in range(13):
        if row[s] == 5:
            row[s] = 1
    return row


def bass_row_to_sub_mask(row: list[int]) -> int:
    m = 0
    for s, v in enumerate(row):
        if v:
            m |= 1 << s
    return m | 1


def uniquify_bass_rows(rows: list[list[int]], archs: list[str], era: int, ix0: int) -> list[list[int]]:
    seen: set[tuple[int, ...]] = set()
    out: list[list[int]] = []
    for i, row in enumerate(rows):
        arch = archs[i]
        r = row[:]
        n = 0
        while tuple(r) in seen and n < 24:
            s = 2 + ((ix0 + i + n) % 14)
            if arch in ("SEQ16", "FUNK", "GALLOP", "MELODIC") and r[s] == 0:
                r[s] = 1 + (n % 4)
            elif arch == "OCT8":
                es = list(range(0, 16, 2))
                flip = es[(n + i) % len(es)]
                r[flip] = 2 if r[flip] == 1 else 1
                r = normalize_oct8_row(r, i)
            elif arch == "PULSE8":
                for ss in (10, 12, 14):
                    r[ss] = [1, 2, 3, 5][(n + ss) % 4]
                for ss in range(1, 16, 2):
                    r[ss] = 0
            n += 1
        seen.add(tuple(r))
        out.append(r)
    return out


def snap_deg(deg: int, chord_deg: int) -> int:
    tones = sorted(chord_tones_deg(chord_deg))
    d = deg % 7
    best = tones[0]
    best_dist = 8
    for t in tones:
        dist = min((d - t) % 7, (t - d) % 7)
        if dist < best_dist:
            best_dist = dist
            best = t
    return best + (deg // 7) * 7


def build_melody(era: int, ix: int, t: int, scale: int, chord_deg: int) -> tuple[list[int], list[int]]:
    rhythm = MELO_RHYTHM[era][ix]
    motif = MOTIF_SHAPES[ix % len(MOTIF_SHAPES)]
    leap = LEAP_TABLE[(era + ix + t) % len(LEAP_TABLE)]
    a: list[int] = []
    mi = 0
    for s in range(16):
        if not (rhythm & (1 << s)):
            a.append(-1)
        else:
            deg = motif[mi % len(motif)] + (t % 3) - 1
            if mi == 2:
                deg = (deg + leap) % 7
            if deg < 0:
                deg += 7
            if s in (0, 4, 8, 12):
                deg = snap_deg(deg, chord_deg)
            a.append(deg)
            mi += 1
    b = [-1] * 16
    resolve = 0 if scale == 4 else (2 if scale == 1 else 0)
    b_rhythm = MELO_RHYTHM[era][(ix + 3) % 10]
    for s in range(16):
        if b_rhythm & (1 << s):
            b[s] = resolve if s >= 8 else motif[(s // 4) % len(motif)]
    if b[15] < 0:
        b[15] = resolve
    return a, b


def dedupe_era_masks(era: int, masks: list[int], anchor: int) -> list[int]:
    seen: set[int] = set()
    out: list[int] = []
    for ix, m in enumerate(masks):
        m |= anchor
        n = 0
        while m in seen and n < 20:
            m ^= 1 << ((ix + n + 1) % 15 + 1)
            m |= anchor
            n += 1
        seen.add(m)
        out.append(m)
    return out


def build_tracks(names: list[str]) -> list[dict]:
    prog_ix_list = assign_prog_triples()
    era_kicks: dict[int, list[int]] = {}
    for e in range(ERA_N):
        anchor = mask16(FOUR_FLOOR_STEPS) if e in FOUR_FLOOR_ERAS else 1
        era_kicks[e] = dedupe_era_masks(e, [kick_mask_for_track(e, ix) for ix in range(10)], anchor)
    era_hats: dict[int, list[int]] = {}
    for e in range(ERA_N):
        kind = ERA_HAT_KIND[e]
        hat_anchor = HAT_OFFBEAT_OPEN if kind == "offbeat" else (HAT_8TH_EVEN if kind != "16th" else 0)
        era_hats[e] = dedupe_era_masks(e, [hat_mask_for_track(e, ix) for ix in range(10)], hat_anchor)
    era_rims: dict[int, list[int]] = {}
    for e in range(ERA_N):
        raw = [mask16((8,) + tuple(ERA_SNARE_GHOST[e][ix])) if e == 4 else mask16((4, 12) + tuple(ERA_SNARE_GHOST[e][ix])) for ix in range(10)]
        anchor = 1 << 8 if e == 4 else (1 << 4) | (1 << 12)
        era_rims[e] = dedupe_era_masks(e, raw, anchor)
    bass_by_era: dict[int, list[list[int]]] = {}
    for e in range(ERA_N):
        era_rows = [build_bass_row(ERA_BASS_ARCH[e][ix], e, ix, e * 10 + ix) for ix in range(10)]
        bass_by_era[e] = uniquify_bass_rows(era_rows, ERA_BASS_ARCH[e], e, 0)
    tracks: list[dict] = []
    for t in range(TRACK_N):
        era = t // TRACKS_PER_ERA
        ix = t % TRACKS_PER_ERA
        scale = ERA_SCALE_BIAS[era][ix]
        pc = KEY_PCS[t]
        root = 60 + pc - (60 % 12)
        while root < 54:
            root += 12
        while root > 65:
            root -= 12
        lo, hi = ERA_BPM[era]
        bpm = lo + (ix * (hi - lo)) // max(1, TRACKS_PER_ERA - 1)
        swing = CHROME_SWING_BY_IX.get(ix, 50) if era == 2 else 50
        bars = 2 if era == 4 else 1
        arch = ERA_BASS_ARCH[era][ix]
        bass = bass_by_era[era][ix]
        sub_m = bass_row_to_sub_mask(bass)
        kick = era_kicks[era][ix]
        rim = era_rims[era][ix]
        hat = era_hats[era][ix]
        chord_m = (0x1111 | (1 << ((ix + era + 1) % 15 + 1)) | (0x8888 if ix & 1 else 0x4444)) & 0xFFFF
        siren_m = (0x1248 ^ (1 << ((t * 3) % 16)) ^ (1 << ((t * 5 + 7) % 16))) & 0xFFFF
        if era == 4:
            siren_m = (0x1010 | (1 << (ix + 2))) & 0xFFFF
        if era == 0:
            siren_m = (0x2222 ^ (ix << ix % 4)) & 0xFFFF
        chord_deg0 = ALL_PROGS[prog_ix_list[t]][0]
        ma, mb = build_melody(era, ix, t, scale, chord_deg0)
        tracks.append({
            "name": names[t], "era": era, "root": root, "scale": scale, "prog_ix": prog_ix_list[t],
            "bars": bars, "bpm": bpm, "swing": swing, "bass_arch": arch, "bass": bass, "melo_a": ma, "melo_b": mb,
            "kick": kick, "rim": rim, "hat": hat, "sub": sub_m, "chord": chord_m, "siren": siren_m,
        })
    seen_bass: set[tuple[int, ...]] = set()
    for t, tr in enumerate(tracks):
        row = tr["bass"][:]
        arch = tr["bass_arch"]
        n = 0
        while tuple(row) in seen_bass and n < 48:
            if arch == "OCT8":
                row = build_oct8_row(tr["era"], t % 10, t + n * 17 + 1)
            elif arch == "PULSE8":
                row = build_pulse8_row(tr["era"], t % 10, t + n * 13 + 1)
            else:
                s = 8 + (t + n) % 8
                if row[s] == 0:
                    row[s] = 1 + (n % 4)
                else:
                    row[s] = 3 if row[s] == 1 else 1
                for ss in range(13):
                    if row[ss] == 5:
                        row[ss] = 1
            n += 1
        seen_bass.add(tuple(row))
        tr["bass"] = row
        tr["sub"] = bass_row_to_sub_mask(row)
    return tracks


def fmt_mask16(m: int) -> str:
    return f"0x{m:04X}u | ((uint32_t)0x{m:04X}u << 16)"


def emit_pack(tracks: list[dict]) -> str:
    lines = ["/* @pop-pack begin — generated by tools/gen_pop_pack.py */", ""]
    lines.append('static const char *const VOICE_NAMES[VOICE_N] = { "TRI", "7TH", "SUS" };')
    lines.append("")
    lines.append("static const pop_plate_t POP_PLATE[POP_PLATE_N] = {")
    for t, tr in enumerate(tracks):
        era = tr["era"]
        lines.append(
            f'    {{ "{tr["name"]}", {era}, {t % 5}, {t % 3}, {t & 3}, '
            f'{10 + era}, {tr["bpm"]}, 48, 8, 18, 8, 32, {t % 9}, 68, '
            f"{fmt_mask16(tr['kick'])}, {fmt_mask16(tr['rim'])}, {fmt_mask16(tr['hat'])}, "
            f"{fmt_mask16(tr['sub'])}, {fmt_mask16(tr['chord'])}, {fmt_mask16(tr['siren'])} }}, /* {era}-{t%10} */"
        )
    lines.append("};")
    lines.append("")
    lines.append(f"static const int8_t POP_PLATE_ROOT[POP_PLATE_N] = {{ {', '.join(str(tr['root']) for tr in tracks)} }};")
    lines.append(f"static const int8_t POP_PLATE_SCALE[POP_PLATE_N] = {{ {', '.join(str(tr['scale']) for tr in tracks)} }};")
    lines.append(f"static const int8_t POP_PLATE_SWING[POP_PLATE_N] = {{ {', '.join(str(tr['swing']) for tr in tracks)} }};")
    lines.append(f"static const int8_t POP_PLATE_LEAD_RATE[POP_PLATE_N] = {{ {', '.join(str((t % 11) - 5) for t in range(TRACK_N))} }};")
    lines.append(
        f"static const int8_t POP_PLATE_LEAD_TIMB[POP_PLATE_N] = {{ {', '.join(str((t * 3 % 13) - 6) for t in range(TRACK_N))} }};"
    )
    lead_var = []
    for t, tr in enumerate(tracks):
        ix = t % TRACKS_PER_ERA
        lead_var.append(str(ERA_LEAD_VAR_SEQ[tr["era"]][ix]))
    lines.append(f"static const uint8_t POP_LEAD_VARIANT[POP_PLATE_N] = {{ {', '.join(lead_var)} }};")
    lines.append("")
    lines.append(f"enum {{ POP_PROG_N = {len(ALL_PROGS)}u }};")
    lines.append("static const uint8_t POP_PROG_LEN[POP_PROG_N] = { " + ", ".join("4" for _ in ALL_PROGS) + " };")
    lines.append("static const uint8_t POP_PROG_DEG[POP_PROG_N][4] = {")
    for p in ALL_PROGS:
        lines.append("    { " + ", ".join(str(x) for x in p) + " },")
    lines.append("};")
    lines.append(f"static const uint8_t POP_TRACK_PROG_IX[POP_PLATE_N] = {{ {', '.join(str(tr['prog_ix']) for tr in tracks)} }};")
    lines.append(f"static const uint8_t POP_TRACK_CHORD_BARS[POP_PLATE_N] = {{ {', '.join(str(tr['bars']) for tr in tracks)} }};")
    lines.append("")
    lines.append("static const uint8_t POP_BASS_STEP[POP_PLATE_N][16] = {")
    for tr in tracks:
        lines.append("    { " + ", ".join(str(x) for x in tr["bass"]) + " },")
    lines.append("};")
    lines.append("static const int8_t POP_MELO_A[POP_PLATE_N][16] = {")
    for tr in tracks:
        lines.append("    { " + ", ".join(str(x) for x in tr["melo_a"]) + " },")
    lines.append("};")
    lines.append("static const int8_t POP_MELO_B[POP_PLATE_N][16] = {")
    for tr in tracks:
        lines.append("    { " + ", ".join(str(x) for x in tr["melo_b"]) + " },")
    lines.append("};")
    lines.append("/* @pop-pack end */")
    return "\n".join(lines) + "\n"


def lead_variant_sanity(tracks: list[dict]) -> list[str]:
    errs: list[str] = []
    for era in range(ERA_N):
        seq = [ERA_LEAD_VAR_SEQ[era][t % TRACKS_PER_ERA] for t in range(TRACKS_PER_ERA)]
        for ix in range(1, TRACKS_PER_ERA):
            if seq[ix] == seq[ix - 1]:
                errs.append(f"era {era}: adjacent lead variant {seq[ix]} at {ix - 1},{ix}")
        for v in range(4):
            if seq.count(v) < 2:
                errs.append(f"era {era}: lead variant {v} used {seq.count(v)}× (need ≥2)")
    return errs


def uniqueness(tracks: list[dict]) -> dict:
    era_kick: list[set[int]] = [set() for _ in range(ERA_N)]
    era_rim: list[set[int]] = [set() for _ in range(ERA_N)]
    era_hat: list[set[int]] = [set() for _ in range(ERA_N)]
    melo, bass, masks = set(), set(), set()
    for tr in tracks:
        e = tr["era"]
        era_kick[e].add(tr["kick"])
        era_rim[e].add(tr["rim"])
        era_hat[e].add(tr["hat"])
        melo.add(tuple(tr["melo_a"] + tr["melo_b"]))
        bass.add(tuple(tr["bass"]))
        masks.add((tr["kick"], tr["rim"], tr["hat"], tr["sub"], tr["chord"], tr["siren"]))
    return {
        "melo_unique": len(melo),
        "bass_unique": len(bass),
        "mask_unique": len(masks),
        "era_kick_ok": all(len(s) == 10 for s in era_kick),
        "era_rim_ok": all(len(s) == 10 for s in era_rim),
        "era_hat_ok": all(len(s) == 10 for s in era_hat),
    }


def chord_tones_deg(chord_deg: int) -> set[int]:
    return {chord_deg % 7, (chord_deg + 2) % 7, (chord_deg + 4) % 7}


def musical_sanity(tracks: list[dict]) -> list[str]:
    errs: list[str] = []
    for t, tr in enumerate(tracks):
        name = tr["name"]
        era = tr["era"]
        if not (tr["kick"] & 1):
            errs.append(f"{name}: kick missing step 0")
        if era == 4:
            if not (tr["rim"] & (1 << 8)):
                errs.append(f"{name}: BLUE snare not on 8")
        else:
            if not (tr["rim"] & (1 << 4)) or not (tr["rim"] & (1 << 12)):
                errs.append(f"{name}: snare missing 4/12")
        if not (tr["sub"] & 1):
            errs.append(f"{name}: bass mask missing step 0")
        if not tr["bass"][0]:
            errs.append(f"{name}: bass row missing R@0")
        prog = ALL_PROGS[tr["prog_ix"]]
        distinct = len(set(prog))
        is_dorian_vamp = tr["scale"] == 2 and prog in DORIAN_PROGS
        if not is_dorian_vamp and distinct < 3:
            errs.append(f"{name}: progression <3 chords")
        chord_deg = prog[0]
        tones = chord_tones_deg(chord_deg)
        degs_hit = [d for d in tr["melo_a"] if d >= 0] + [d for d in tr["melo_b"] if d >= 0]
        if degs_hit:
            span = max(degs_hit) - min(degs_hit)
            if span > 9:
                errs.append(f"{name}: melody span {span}>9")
        for s in (0, 4, 8, 12):
            d = tr["melo_a"][s]
            if d >= 0 and (d % 7) not in tones:
                errs.append(f"{name}: melo A@{s} deg{d} not chord tone of {chord_deg}")
        if era in FOUR_FLOOR_ERAS:
            for s in FOUR_FLOOR_STEPS:
                if not (tr["kick"] & (1 << s)):
                    errs.append(f"{name}: four-floor kick missing step {s}")
        arch = tr.get("bass_arch", "")
        hits_hi = sum(1 for s in range(8, 16) if tr["bass"][s])
        if arch not in ("LONG", "ROOT4") and hits_hi < 3:
            errs.append(f"{name}: bass <3 hits in steps 8–15 ({arch})")
        if arch == "OCT8":
            syms = _bass_hit_symbols(tr["bass"])
            if sum(1 for x in syms if x == 1) < 3 or sum(1 for x in syms if x == 2) < 3:
                errs.append(f"{name}: OCT8 R/O balance")
            if _max_symbol_run(syms) > 2:
                errs.append(f"{name}: OCT8 symbol run >2")
            if sum(1 for s in range(16) if tr["bass"][s]) < 8:
                errs.append(f"{name}: OCT8 bass <8 hits")
        if arch == "PULSE8":
            for s in range(1, 16, 2):
                if tr["bass"][s]:
                    errs.append(f"{name}: PULSE8 hit on odd step {s}")
            ph = sum(1 for s in range(16) if tr["bass"][s])
            pr = sum(1 for s in range(16) if tr["bass"][s] == 1)
            if ph < 6 or pr < 4:
                errs.append(f"{name}: PULSE8 density (hits={ph} R={pr})")
        if arch == "SEQ16" and sum(1 for s in range(16) if tr["bass"][s]) < 12:
            errs.append(f"{name}: SEQ16 bass <12 hits")
        for s in range(16):
            if tr["bass"][s] != 5:
                continue
            if arch == "PULSE8" and s in (10, 12, 14):
                continue
            if arch == "OCT8" and s in (14, 15):
                continue
            if s >= 13:
                continue
            errs.append(f"{name}: approach A on step {s}")
        hat = tr["hat"]
        kind = ERA_HAT_KIND[era]
        ix = t % TRACKS_PER_ERA
        chrome_16 = kind == "mix" and ix in (1, 2, 4, 7, 9)
        if kind == "offbeat":
            for s in (2, 6, 10, 14):
                if not (hat & (1 << s)):
                    errs.append(f"{name}: offbeat hat missing {s}")
        elif not chrome_16 and kind in ("8th", "mix"):
            if _odd_steps(hat) > 2:
                errs.append(f"{name}: hat odd ghosts >2")
            for s in range(16):
                if s & 1 and (hat & (1 << s)) and _odd_steps(hat) <= 2:
                    continue
                if s & 1 and (hat & (1 << s)):
                    errs.append(f"{name}: hat on odd step {s}")
    chrome = [tr for tr in tracks if tr["era"] == 2]
    funk_n = sum(1 for tr in chrome if any(tr["kick"] & (1 << s) for s in (3, 6, 10, 11)))
    if funk_n < 5:
        errs.append(f"CHROME: only {funk_n}/10 funk kicks (need ≥5 on 3/6/10/11)")
    swing_n = sum(1 for tr in chrome if 53 <= tr["swing"] <= 56)
    if swing_n < 2:
        errs.append(f"CHROME: only {swing_n} tracks swing 53–56")
    return errs


def splice_pop_data(pack: str) -> None:
    text = POP_DATA.read_text()
    text = re.sub(
        r"/\* @pop-pack begin.*?\*/.*?/\* @pop-pack end \*/\n",
        pack,
        text,
        count=1,
        flags=re.DOTALL,
    )
    POP_DATA.write_text(text)


SCALE_NAMES = {1: "minor", 2: "dorian", 4: "major"}


def mask_str(m: int) -> str:
    return "".join("x" if m & (1 << i) else "." for i in range(16))


def mel_str(row: list[int], root: int, scale: int) -> str:
    parts = []
    for d in row:
        if d < 0:
            parts.append("-")
        else:
            parts.append(note_name(deg_note(root, scale, d)))
    return " ".join(parts)


def bass_str(row: list[int]) -> str:
    return "".join({0: "-", 1: "R", 2: "O", 3: "5", 4: "3", 5: "A"}.get(x, "?") for x in row)


def prog_names(root: int, scale: int, prog: list[int]) -> str:
    return "–".join(note_name(deg_note(root, scale, d)) for d in prog)


def print_examples(tracks: list[dict], names: list[str]) -> None:
    want = ["FROSTY", "RADIUM", "GLOSSY", "MIDNIT", "LAZULI", "NEARBY"]
    for label in want:
        tr = next(t for t in tracks if t["name"] == label)
        prog = ALL_PROGS[tr["prog_ix"]]
        print(f"\n=== {label} bpm={tr['bpm']} swing={tr['swing']} bass={tr.get('bass_arch', '?')} ===")
        print(f"key {note_name(tr['root'])} {SCALE_NAMES[tr['scale']]} | {prog_names(tr['root'], tr['scale'], prog)}")
        print(f"bass: {bass_str(tr['bass'])}")
        print(f"melA: {mel_str(tr['melo_a'], tr['root'], tr['scale'])}")
        print(f"melB: {mel_str(tr['melo_b'], tr['root'], tr['scale'])}")
        print(f"kick: {mask_str(tr['kick'])} sn: {mask_str(tr['rim'])} hat: {mask_str(tr['hat'])}")


def main() -> int:
    names = parse_track_names(POP_DATA.read_text())
    tracks = build_tracks(names)
    stats = uniqueness(tracks)
    print("uniqueness:", stats)
    errs = musical_sanity(tracks) + lead_variant_sanity(tracks)
    if errs:
        print("sanity errors:", len(errs))
        for e in errs[:20]:
            print(" ", e)
        return 1
    if stats["melo_unique"] < TRACK_N or stats["bass_unique"] < TRACK_N or stats["mask_unique"] < TRACK_N:
        print("FAIL: uniqueness", stats, file=sys.stderr)
        return 1
    if not (stats["era_kick_ok"] and stats["era_rim_ok"] and stats["era_hat_ok"]):
        print("FAIL: era per-lane drum dup", stats, file=sys.stderr)
        return 1
    splice_pop_data(emit_pack(tracks))
    print(f"wrote pack into {POP_DATA}")
    if "--examples" in sys.argv:
        print_examples(tracks, names)
    return 0


if __name__ == "__main__":
    sys.exit(main())
