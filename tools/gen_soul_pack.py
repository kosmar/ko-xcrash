#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate firmware/src/soul_data.h only (idempotent, --check)."""
from __future__ import annotations

import hashlib
import random
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "firmware" / "src" / "soul_data.h"

ERA_N = 6
PRESETS_PER_ERA = 10
PRESET_N = ERA_N * PRESETS_PER_ERA

ERA_NAMES = ["CHURCH", "DETROIT", "MEMPHIS", "NORTHERN", "PHILLY", "STREET"]

SIDE_NAMES = [
    ["AMEN", "PULPIT", "REVIVAL", "TESTIFY", "JUBILEE", "SUNDAY", "CHOIR", "SHOUTER", "VESPERS", "HYMNAL"],
    ["MOTOR", "CHROME", "SNAKEPIT", "ASSEMBLY", "SHOWROOM", "TAILFIN", "HIGHGEAR", "GLEAM", "DRIVEIN", "TWILIGHT"],
    ["BEALE", "SHOALS", "BACKROAD", "SKILLET", "BLUFF", "DELTA", "SWELTER", "PICKUP", "TENDER", "ACHING"],
    ["ALLNIGHT", "TALC", "BACKDROP", "STOMPER", "RARITY", "ACETATE", "WIGAN", "TORCH", "FLOATER", "PATCHES"],
    ["SIGMA", "BROAD", "SATIN", "LUSH", "BALLROOM", "GOLDLEAF", "SKYLINE", "LIBERTY", "CANDLE", "LOVESEAT"],
    ["PIRATE", "CASSETTE", "NIGHTBUS", "ESTATE", "HIGHRISE", "TERRACE", "POSTCODE", "SHUFFLE", "SLOWJAM", "MIDNIGHT"],
]

BASS_NAMES = ["PEDAL", "UPRIGHT", "FINGER", "FLAT", "THUMB", "PICK", "OCTAVE", "SYNTH", "SUB"]
BASS_ERA = [0, 0, 1, 2, 2, 3, 4, 5, 5]  # primary era affinity
DKIT_NAMES = ["CHAPEL", "SNAP", "DEEP", "STOMP", "SILK", "MACHINE", "TAMB", "CLAP"]
DKIT_ERA = [0, 1, 2, 3, 4, 5, 0, 2]
DKIT_N = len(DKIT_NAMES)
BASS_N = len(BASS_NAMES)

VOICE_NAMES = [
    "MAJ", "MIN7", "DOM7", "DOM9", "MAJ6", "69", "SUS4", "MIN9", "ADD9", "MAJ7", "DOM13", "MIN11",
]
VOICE_ERA_DEF = [2, 4, 3, 0, 9, 11]  # DOM7 MAJ6 DOM9 MAJ MAJ7 MIN11

ERA_BPM = [(84, 100, 60, 68), (116, 128, 92, 100), (96, 112, 64, 74),
           (118, 136, 100, 108), (104, 118, 76, 86), (88, 100, 72, 80)]
ERA_SWING = [(64, 72), (50, 56), (54, 60), (50, 54), (50, 54), (58, 66)]
ERA_SCALE_POOL = [
    [4, 5, 7, 4, 5, 7, 4, 5],  # MAJOR MIXO BLUES
    [4, 5, 0, 4, 5, 0, 4, 5],
    [5, 7, 2, 5, 7, 2, 5, 7],
    [4, 1, 0, 4, 1, 0, 4, 1],
    [4, 2, 4, 2, 4, 2, 4, 2],
    [1, 2, 0, 1, 2, 0, 1, 2],
]
# Roots biased to flats for CHURCH
ERA_ROOT_PC = [
    [3, 8, 10, 5, 3, 8, 10, 5],
    [0, 2, 5, 7, 9, 0, 2, 5],
    [5, 7, 10, 0, 3, 5, 7, 10],
    [0, 2, 5, 7, 9, 0, 2, 5],
    [0, 2, 4, 5, 7, 9, 0, 2],
    [2, 5, 7, 10, 0, 2, 5, 7],
]

ERA_KICK_MUST = [0x0101, 0x0101, 0x0001, 0x1111, 0x1111, 0x0001]
ERA_RIM_MUST = [0x1010, 0x1111, 0x1010, 0x1010, 0x1010, 0x1010]
ERA_HAT_MUST = [0x5555, 0x5555, 0x5555, 0x5555, 0x4444, 0x5555]
ERA_TAMB_MUST = [0x1111, 0x5555, 0x0000, 0x5555, 0x0000, 0x0000]
KICK_BAN_SNARE_ERAS = {0, 2, 5}

ERA_BASS_SLOT = [
    [0, 1, 0, 1, 0, 1, 0, 1],
    [2, 2, 2, 2, 2, 2, 2, 2],
    [3, 4, 3, 4, 3, 4, 3, 4],
    [5, 5, 5, 5, 5, 5, 5, 5],
    [6, 6, 6, 6, 6, 6, 6, 6],
    [7, 8, 7, 8, 7, 8, 7, 8],
]

SOUL_ERA_MACROS = [
    [52, 48, 72, 50, 84, 38, 32, 36, 0, 40],
    [78, 42, 48, 44, 52, 40, 28, 34, 0, 38],
    [58, 62, 46, 48, 44, 42, 68, 52, 0, 44],
    [74, 40, 44, 46, 48, 38, 30, 62, 0, 42],
    [48, 44, 50, 70, 80, 36, 28, 32, 0, 46],
    [54, 48, 42, 44, 40, 76, 34, 38, 0, 48],
]

SOUL_BASS_WALK = [
    [[0, 2, 3, 4, 0, 2, 3, 4], [0, 2, 3, 2, 0, 3, 4, 0], [0, 3, 4, 3, 0, 2, 3, 0],
     [0, 2, 4, 3, 0, 2, 3, 4], [0, 3, 2, 4, 0, 3, 2, 0], [0, 2, 3, 4, 2, 0, 3, 4],
     [0, 2, 3, 4, 0, 3, 2, 4], [0, 3, 4, 2, 0, 2, 3, 0], [0, 0, 0, 0, 0, 0, 0, 0],
     [0, 0, 3, 0, 0, 0, 4, 0]],
    [[0, 1, 2, 3, 2, 1, 0, 2], [0, 2, 1, 3, 2, 0, 1, 2], [0, 1, 3, 2, 1, 2, 3, 1],
     [0, 2, 3, 1, 2, 3, 0, 2], [0, 1, 2, 0, 3, 2, 1, 3], [0, 2, 1, 2, 3, 1, 2, 0],
     [0, 3, 2, 1, 0, 2, 3, 1], [0, 2, 0, 3, 1, 2, 3, 0], [0, 0, 0, 0, 0, 0, 0, 0],
     [0, 0, 3, 0, 0, 0, 4, 0]],
    [[0, 0, 0, 0, 0, 0, 0, 0], [0, 7, 0, 0, 0, 7, 0, 0], [0, 0, 4, 0, 0, 0, 4, 0],
     [0, 7, 0, 4, 0, 0, 0, 0], [0, 0, 0, 4, 0, 0, 0, 0], [0, 7, 0, 0, 4, 0, 0, 0],
     [0, 0, 4, 0, 0, 7, 0, 0], [0, 7, 0, 0, 0, 0, 4, 0], [0, 0, 0, 0, 0, 0, 0, 0],
     [0, 0, 3, 0, 0, 0, 4, 0]],
    [[0, 0, 0, 7, 0, 0, 0, 7], [0, 0, 7, 0, 0, 7, 0, 0], [0, 7, 0, 0, 0, 7, 0, 0],
     [0, 0, 0, 7, 0, 7, 0, 0], [0, 7, 0, 0, 7, 0, 0, 7], [0, 0, 7, 0, 0, 0, 7, 0],
     [0, 7, 0, 7, 0, 0, 0, 7], [0, 0, 0, 7, 0, 0, 7, 0], [0, 0, 0, 0, 0, 0, 0, 0],
     [0, 0, 3, 0, 0, 0, 4, 0]],
    [[0, 7, 0, 7, 0, 7, 0, 7], [0, 7, 0, 7, 0, 7, 0, 7], [0, 7, 0, 7, 0, 7, 0, 7],
     [0, 7, 0, 7, 0, 7, 0, 7], [0, 7, 0, 7, 0, 7, 0, 7], [0, 7, 0, 7, 0, 7, 0, 7],
     [0, 7, 0, 7, 0, 7, 0, 7], [0, 7, 0, 7, 0, 7, 0, 7], [0, 0, 0, 0, 0, 0, 0, 0],
     [0, 0, 3, 0, 0, 0, 4, 0]],
    [[0, 0, 0, 2, 0, 0, 0, 0], [0, 0, 2, 0, 0, 0, 0, 2], [0, 2, 0, 0, 0, 0, 2, 0],
     [0, 0, 0, 0, 2, 0, 0, 0], [0, 2, 0, 0, 0, 2, 0, 0], [0, 0, 2, 0, 0, 0, 0, 2],
     [0, 0, 0, 2, 0, 0, 2, 0], [0, 2, 0, 0, 2, 0, 0, 0], [0, 0, 0, 0, 0, 0, 0, 0],
     [0, 0, 3, 0, 0, 0, 4, 0]],
]

HAT_NOTE = [84, 92, 86, 94, 90, 88, 82, 96]
HAT_FM = [18, 44, 22, 38, 28, 52, 14, 32]
HAT_NOISE = [32, 58, 48, 64, 42, 72, 28, 54]
HAT_METAL = [96, 76, 88, 102, 82, 68, 110, 92]
HAT_DECAY = [9, 6, 8, 5, 7, 4, 10, 7]
HAT_OPEN = [11, 8, 6, 9, 14, 3, 5, 12]
HAT_STYLE = [1, 3, 2, 3, 1, 2, 5, 3]  # TAMB kit uses style 5
HAT_CUT = [58, 66, 56, 62, 54, 60, 52, 64]

ERA_BASE = [
    (34, 90, 92, 20, 44, 40, 96, 0, 14, 28, 76, 56, 58, 64, 70, 14, 100, 42, 36, (64, 68, 52, 76), 10, 54, 28, 68),
    (38, 88, 86, 18, 42, 38, 94, 0, 12, 26, 78, 58, 56, 64, 66, 16, 104, 44, 32, (64, 70, 54, 78), 6, 48, 26, 52),
    (36, 86, 94, 22, 40, 44, 98, 0, 16, 30, 72, 54, 56, 64, 72, 12, 98, 38, 40, (62, 68, 50, 74), 14, 56, 24, 58),
    (40, 84, 88, 24, 46, 42, 92, 0, 18, 24, 80, 60, 54, 64, 64, 18, 102, 48, 34, (64, 72, 56, 80), 8, 50, 30, 52),
    (42, 90, 90, 16, 38, 46, 100, 0, 10, 32, 74, 56, 62, 64, 58, 14, 96, 40, 38, (64, 68, 52, 76), 12, 58, 32, 54),
    (36, 82, 84, 28, 48, 50, 88, 0, 22, 20, 70, 52, 48, 64, 60, 20, 104, 46, 28, (64, 74, 50, 82), 16, 44, 36, 62),
]

ERA_FX = [
    (0, 36, 1, 42, 16, 80, 42, 44, 68, 58, 70, 0, 10),
    (1, 24, 0, 38, 14, 70, 34, 32, 54, 46, 58, 2, 6),
    (2, 40, 1, 44, 18, 76, 46, 40, 62, 56, 64, 3, 14),
    (3, 28, 0, 40, 12, 72, 36, 34, 56, 48, 66, 5, 8),
    (4, 44, 2, 36, 16, 84, 48, 46, 70, 62, 60, 6, 12),
    (5, 32, 1, 46, 20, 68, 38, 36, 52, 44, 72, 7, 18),
]

LEN_16THS = (1, 2, 4, 5, 8, 11, 12, 16, 20, 24, 28, 32, 40, 48, 64)

SCALE_SEMIS = (
    (0, 2, 3, 5, 7, 10),
    (0, 2, 3, 5, 7, 8, 10),
    (0, 2, 3, 5, 7, 9, 10),
    (0, 1, 3, 5, 7, 8, 10),
    (0, 2, 4, 5, 7, 9, 11),
    (0, 2, 4, 5, 7, 9, 10),
    (0, 3, 5, 7, 10),
    (0, 3, 5, 6, 7, 10),
    (0, 2, 3, 5, 7, 8, 11),
)

LEAD_CFG = [
    dict(hi=(4, 8), lo=(4, 8), pr=(-7, 9), jump=7, bend=(2, 5), len_ix=(2, 6), call=1),
    dict(hi=(8, 14), lo=(8, 14), pr=(0, 12), jump=5, bend=(0, 0), len_ix=(0, 3), riff=1),
    dict(hi=(5, 10), lo=(5, 10), pr=(-3, 7), jump=4, bend=(0, 0), len_ix=(4, 8), stab=1),
    dict(hi=(6, 12), lo=(6, 12), pr=(2, 14), jump=7, bend=(0, 2), len_ix=(2, 5), fan=1),
    dict(hi=(3, 6), lo=(3, 6), pr=(0, 9), jump=4, bend=(0, 3), len_ix=(6, 10), legato=1),
    dict(hi=(6, 12), lo=(6, 12), pr=(-2, 7), jump=5, bend=(0, 0), len_ix=(0, 3), swing16=1),
]


def rng(era: int, slot: int, salt: int) -> random.Random:
    h = hashlib.md5(f"soul-{era}-{slot}-{salt}".encode()).hexdigest()
    return random.Random(int(h[:16], 16))


def sgen_m(lo: int, hi: int) -> int:
    return (lo & 0xFFFF) | ((hi & 0xFFFF) << 16)


def fmt_m32(v: int) -> str:
    lo, hi = v & 0xFFFF, (v >> 16) & 0xFFFF
    return f"0x{lo:04X}u | ((uint32_t)0x{hi:04X}u << 16)"


def kick_mask(era: int, slot: int) -> int:
    r = rng(era, slot, 1)
    lo = ERA_KICK_MUST[era]
    hi = lo
    if era in KICK_BAN_SNARE_ERAS:
        lo &= ~ERA_RIM_MUST[era]
        hi &= ~ERA_RIM_MUST[era]
    if era in (2, 5):
        extra = r.choice([0x0800, 0x0400, 0x0200, 0x0010, 0x0008])
        lo |= extra
        if r.random() > 0.5:
            hi |= r.choice([0x0800, 0x0400, 0x0200])
    if era == 1 and slot % 3 == 0:
        lo |= 0x0100  # overlap allowed
    var = r.randint(0, 3)
    if var:
        lo ^= (1 << (var * 4 + r.randint(0, 3))) & 0xEEEE
    return sgen_m(lo, hi)


def hat_mask(era: int, slot: int) -> int:
    r = rng(era, slot, 2)
    lo = ERA_HAT_MUST[era]
    hi = lo
    if era == 4:
        hi = 0x4444 if slot % 2 else 0x5555
    dens = r.randint(0, 2)
    if dens:
        lo |= 0x1111 >> dens
        hi |= 0x1111 >> dens
    return sgen_m(lo, hi)


def sub_chord_masks(era: int, slot: int) -> tuple[int, int]:
    r = rng(era, slot, 3)
    sub_lo = 0x8080 | (r.randint(0, 0x7070) & 0x7777)
    sub_hi = 0x8080 | (r.randint(0, 0x7070) & 0x7777)
    ch_lo = 0x1212 | (r.randint(0, 0x0E0E) & 0x1F1F)
    ch_hi = 0x1212 | (r.randint(0, 0x0E0E) & 0x1F1F)
    return sgen_m(sub_lo, sub_hi), sgen_m(ch_lo, ch_hi)


def scale_pitch(scale_i: int, deg: int) -> int:
    sc = SCALE_SEMIS[scale_i % len(SCALE_SEMIS)]
    deg = max(0, min(len(sc) - 1, deg))
    base = sc[deg]
    if deg >= len(sc):
        base += 12
    return base


def gen_lead(era: int, slot: int, scale_i: int) -> tuple[int, list[int], list[int], list[int], int]:
    r = rng(era, slot, 9)
    cfg = LEAD_CFG[era]
    gate = 0
    pitch = [0] * 32
    length = [0] * 32
    bend = [0] * 32

    def place(bar: int, steps: list[int], pitch_deg: int, li: int, bi: int) -> None:
        nonlocal gate
        for st in steps:
            if bar == 0:
                gate |= 1 << st
            else:
                gate |= 1 << (16 + st)
            pitch[st if bar == 0 else 16 + (st % 16)] = pitch_deg
            length[st if bar == 0 else 16 + (st % 16)] = li
            bend[st if bar == 0 else 16 + (st % 16)] = bi

    n_lo = r.randint(cfg["lo"][0], cfg["lo"][1])
    n_hi = r.randint(cfg["hi"][0], cfg["hi"][1])
    deg = r.randint(2, 5)
    last = scale_pitch(scale_i, deg)
    pr_lo, pr_hi = cfg["pr"]
    b_lo, b_hi = cfg["bend"]

    def mk_steps(n: int, bar: int) -> list[int]:
        if cfg.get("swing16"):
            pool = [1, 3, 5, 7, 9, 11, 13, 15] if bar == 0 else [17, 19, 21, 23, 25, 27, 29, 31]
        elif cfg.get("stab"):
            pool = [2, 6, 10, 14, 18, 22, 26, 30]
        elif cfg.get("legato"):
            pool = [0, 4, 8, 12, 16, 20, 24, 28]
        elif cfg.get("fan"):
            pool = [0, 2, 4, 8, 12, 16, 20, 24]
        elif cfg.get("riff") and bar == 0:
            cell = [0, 2, 4, 6, 8, 10, 12, 14]
            return cell[: min(n, len(cell))]
        else:
            pool = list(range(16 if bar == 0 else 16, 32 if bar == 1 else 16))
            pool = list(range(0, 16)) if bar == 0 else list(range(16, 32))
        r.shuffle(pool)
        return sorted(pool[:n])

    for bar in (0, 1):
        n = n_lo if bar == 0 else n_hi
        steps = mk_steps(n, bar)
        if cfg.get("call") and bar == 1 and steps:
            steps = [(s + 2) % 16 + 16 for s in steps[: min(len(steps), 6)]]
        for st in steps:
            st16 = st & 15 if bar == 0 else (st - 16) if st >= 16 else st
            idx = st if st < 32 else st16
            if idx >= 32:
                idx = st & 31
            jump = r.randint(-cfg["jump"], cfg["jump"])
            deg = max(0, min(6, deg + (1 if jump > 2 else 0) - (1 if jump < -2 else 0)))
            semi = scale_pitch(scale_i, deg)
            semi = max(pr_lo, min(pr_hi, semi + jump))
            li = r.randint(cfg["len_ix"][0], cfg["len_ix"][1])
            bi = r.randint(b_lo, b_hi) if b_hi else 0
            if bar == 0:
                gate |= 1 << (st16 & 15)
            else:
                gate |= 1 << (16 + (st16 & 15))
            pitch[st if st < 32 else (16 + (st16 & 15))] = semi
            length[st if st < 32 else (16 + (st16 & 15))] = li
            bend[st if st < 32 else (16 + (st16 & 15))] = bi

    hits = bin(gate).count("1")
    if hits < 2:
        gate |= 1 | (1 << 16)
    gate_cs = 28 + era * 2 + (8 if slot >= 8 else 0)
    return gate, pitch, length, bend, gate_cs


def bpm_for(era: int, slot: int) -> int:
    full_lo, full_hi, slow_lo, slow_hi = ERA_BPM[era]
    if slot >= 8:
        lo, hi = slow_lo, slow_hi
    else:
        lo, hi = full_lo, full_hi
    span = max(1, PRESETS_PER_ERA - 1)
    return lo + (slot * (hi - lo)) // span


def swing_for(era: int, slot: int) -> int:
    lo, hi = ERA_SWING[era]
    return lo + (slot * (hi - lo)) // max(1, PRESETS_PER_ERA - 1)


def gen_plate_line(era: int, slot: int, name: str) -> str:
    r = rng(era, slot, 4)
    bass = ERA_BASS_SLOT[era][slot % 8]
    voice = (VOICE_ERA_DEF[era] + slot) % 12
    tap = slot & 3
    tank = 20 + (era * 7 + slot * 5) % 56
    bpm = bpm_for(era, slot)
    dens = 32 + (slot % 5) + era
    dly_c = 54 + (slot * 2) % 24
    rev_c = 58 + (slot * 3) % 22
    fb = 12 + (era + slot) % 28
    lf = 36 + (era * 5) % 30
    wob = (era * 9 + slot * 5) % 80
    pump = 58 + (slot * 2) % 18
    kick = kick_mask(era, slot)
    rim = sgen_m(ERA_RIM_MUST[era], ERA_RIM_MUST[era])
    hat = hat_mask(era, slot)
    sub, chord = sub_chord_masks(era, slot)
    sr = sgen_m(r.randint(0, 0xFFFF), r.randint(0, 0xFFFF))
    return (
        f'{{ "{name}", {era}, {bass}, {voice}, {tap}, {tank}, {bpm}, {dens}, '
        f'{dly_c}, {rev_c}, {fb}, {lf}, {wob}, {pump}, '
        f"{fmt_m32(kick)}, {fmt_m32(rim)}, {fmt_m32(hat)}, "
        f"{fmt_m32(sub)}, {fmt_m32(chord)}, {fmt_m32(sr)} }}"
    )


def fmt_era_base(row: tuple) -> str:
    pans = ", ".join(str(p) for p in row[19])
    return (
        f"{{ {row[0]}, {row[1]}, {row[2]}, {row[3]}, {row[4]}, {row[5]}, {row[6]}, {row[7]}, "
        f"{row[8]}, {row[9]}, {row[10]}, {row[11]}, {row[12]}, {row[13]}, {row[14]}, {row[15]}, "
        f"{row[16]}, {row[17]}, {row[18]}, {{{pans}}}, {row[20]}, {row[21]}, {row[22]}, {row[23]} }}"
    )


def fmt_int_row(vals: list, per_line: int = 20) -> list[str]:
    lines = []
    for i in range(0, len(vals), per_line):
        chunk = vals[i : i + per_line]
        lines.append(", ".join(str(v) for v in chunk))
    return lines


def gen_drm_dec_fit() -> list[str]:
    rows = []
    for pi in range(PRESET_N):
        r = rng(pi // PRESETS_PER_ERA, pi % PRESETS_PER_ERA, 77)
        rows.append(
            "{:3d}, {:3d}, {:3d}, {:3d}".format(
                30 + r.randint(0, 8),
                38 + r.randint(0, 10),
                58 + r.randint(0, 40),
                62 + r.randint(0, 6),
            )
        )
    out = ["static const uint8_t soul_drm_dec_fit[GENRE_PRESET_N][4] = {"]
    for i in range(0, PRESET_N, 2):
        pair = "}, {".join(rows[i : i + 2])
        out.append(f"    {{ {pair} }},")
    out.append("};")
    return out


APPLY_TAIL = r'''
#include "fm_pgen_core.h"

static fm_pgen_t soul_gen = { .slot = 0xFFu };
static fm_pgen_lead_t soul_gen_lead;

#define FM_PGEN_STATE soul_gen
#define FM_PGEN_BUILD(era, n, c) soul_gen_build(era, n, c)

static inline uint8_t soul_gen_hit(uint8_t ix)
{
    return fm_pgen_hit(&soul_gen, ix);
}

static inline uint8_t soul_clamp_side(uint8_t ix)
{
    return ix >= SOUL_SIDE_N ? (uint8_t)(SOUL_SIDE_N - 1u) : ix;
}

static inline uint8_t soul_clamp_era(uint8_t era)
{
    return era >= SOUL_ERA_N ? (uint8_t)(SOUL_ERA_N - 1u) : era;
}

static inline const char *soul_era_name(uint8_t era)
{
    return SOUL_ERA_NAMES[soul_clamp_era(era)];
}

static inline const char *soul_side_name(uint8_t side)
{
    if (soul_gen_hit(side))
        return soul_gen.name;
    return SOUL_SIDE[soul_clamp_side(side)].name;
}

static inline const soul_side_t *soul_side(uint8_t ix)
{
    if (soul_gen_hit(ix))
        return &soul_gen.pl;
    return &SOUL_SIDE[soul_clamp_side(ix)];
}

static inline uint8_t soul_era_of_side(uint8_t side)
{
    side = soul_clamp_side(side);
    return (uint8_t)(side / GENRE_PRESETS_PER_ERA);
}

static inline uint8_t soul_era_first_side(uint8_t era)
{
    return (uint8_t)(soul_clamp_era(era) * GENRE_PRESETS_PER_ERA);
}

static inline uint8_t soul_bass_walk_deg(uint8_t era, uint8_t piece, uint8_t step)
{
    era = soul_clamp_era(era);
    piece = (uint8_t)(soul_clamp_side(piece) % GENRE_PRESETS_PER_ERA);
    return SOUL_BASS_WALK[era][piece][step & 7u];
}

static inline uint8_t soul_side_root(uint8_t ix)
{
    return soul_gen_hit(ix) ? (uint8_t)soul_gen.root : (uint8_t)SOUL_SIDE_ROOT[soul_clamp_side(ix)];
}

static inline uint8_t soul_side_scale(uint8_t ix)
{
    return soul_gen_hit(ix) ? (uint8_t)soul_gen.scale : (uint8_t)SOUL_SIDE_SCALE[soul_clamp_side(ix)];
}

static inline uint8_t soul_side_swing(uint8_t ix)
{
    return soul_gen_hit(ix) ? (uint8_t)soul_gen.swing : (uint8_t)SOUL_SIDE_SWING[soul_clamp_side(ix)];
}

static inline uint8_t soul_side_kit(uint8_t ix)
{
    return soul_gen_hit(ix) ? (uint8_t)soul_gen.kit : (uint8_t)SOUL_SIDE_KIT[soul_clamp_side(ix)];
}

static inline int8_t soul_side_lead_rate(uint8_t ix)
{
    return soul_gen_hit(ix) ? soul_gen.rate : SOUL_SIDE_LEAD_RATE[soul_clamp_side(ix)];
}

static inline uint32_t soul_side_lead_gate32(uint8_t side)
{
    if (soul_gen_hit(side))
        return soul_gen.lead_gate;
    return SOUL_SIDE_LEAD_GATE[soul_clamp_side(side)];
}

static inline uint8_t soul_side_lead_gate_cs(uint8_t side)
{
    if (soul_gen_hit(side))
        return soul_gen.gate_cs;
    return SOUL_SIDE_LEAD_GATE_CS[soul_clamp_side(side)];
}

static inline uint8_t soul_side_lead_step_bit(uint8_t side, uint8_t step)
{
    uint32_t g32 = soul_side_lead_gate32(side);
    uint8_t bar4 = (uint8_t)((step >> 4) & 3u);
    uint8_t s16 = (uint8_t)(step & 15u);

    if (bar4 == 0u || bar4 == 2u)
        return (uint8_t)((g32 >> s16) & 1u);
    if (bar4 == 1u)
        return (uint8_t)((g32 >> (16u + s16)) & 1u);
    if (s16 < 8u)
        return 0u;
    return (uint8_t)((g32 >> (16u + s16)) & 1u);
}

static inline uint8_t soul_lead_phrase_span(uint8_t side, uint8_t step)
{
    uint8_t era = soul_era_of_side(side);
    uint8_t s0 = (uint8_t)(step & 31u);
    uint8_t half_end = (s0 < 16u) ? 16u : 32u;
    uint8_t breath, i, empties = 0u, span = 1u;

    if (era == (uint8_t)ERA_CHURCH || era == (uint8_t)ERA_PHILLY)
        breath = 2u;
    else if (era == (uint8_t)ERA_DETROIT)
        breath = 3u;
    else if (era == (uint8_t)ERA_STREET)
        breath = 1u;
    else
        return 1u;

    for (i = 1u; (uint8_t)(s0 + i) < half_end && i < 16u; i++) {
        uint8_t ns = (uint8_t)(s0 + i);

        if (soul_side_lead_step_bit(side, ns))
            break;
        empties++;
        if (empties >= breath)
            break;
        span = (uint8_t)(i + 1u);
    }
    return span;
}

static inline int8_t soul_side_lead_pitch(uint8_t side, uint8_t step)
{
    if (soul_gen_hit(side))
        return soul_gen_lead.pitch[step & 31u];
    return SOUL_SIDE_LEAD_PITCH[soul_clamp_side(side)][step & 31u];
}

static inline uint8_t soul_side_lead_len_ix(uint8_t side, uint8_t step)
{
    if (soul_gen_hit(side))
        return soul_gen_lead.len[step & 31u];
    return SOUL_SIDE_LEAD_LEN[soul_clamp_side(side)][step & 31u];
}

static inline uint8_t soul_side_lead_bend_u7(uint8_t side, uint8_t step)
{
    if (soul_gen_hit(side))
        return soul_gen_lead.bend[step & 31u];
    return SOUL_SIDE_LEAD_BEND[soul_clamp_side(side)][step & 31u];
}

static inline void soul_apply_defaults(dub_params_t *p)
{
    uint8_t di;

    if (!p)
        return;
    p->timbre = 34;
    p->cutoff_c = 88;
    p->decay_c = 90;
    p->cutoff_b = 48;
    p->decay_b = 94;
    p->sub_oct = 0;
    p->drive = 14;
    p->pump = 72;
    p->feedback = 16;
    p->density = 64u;
    p->wobble = 6;
    p->kick_boom = 76;
    p->rim_tone = 56;
    p->hat_lvl = 54;
    p->siren_pitch = 100;
    p->siren_rate = 42;
    p->drum_kit = DKIT_CHAPEL;
    for (di = 0; di < 4u; di++) {
        p->drm_tune[di] = 64u;
        p->drm_dec[di] = 64u;
        p->drm_drv[di] = 18u;
        p->drm_dly[di] = 28u;
        p->drm_rev[di] = 24u;
        p->drm_cut[di] = 64u;
        p->drm_atk[di] = 0u;
        p->drm_dens[di] = 64u;
    }
}

static inline uint8_t dub_step_bit(uint32_t mask, uint8_t step)
{
    return (uint8_t)((mask >> (step & (DUB_STEPS - 1u))) & 1u);
}

static inline void dub_drm_set_bus_dly(dub_params_t *p, uint8_t bus)
{
    uint8_t i;

    if (!p)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_dly[i] = bus;
    p->drm_dly[3] = bus;
}

static inline void dub_drm_set_bus_rev(dub_params_t *p, uint8_t bus)
{
    uint8_t i;
    uint8_t rv = (uint8_t)(48u + (bus >> 1));

    if (!p)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_rev[i] = rv;
    p->drm_rev[3] = 96u;
}

static inline void dub_drm_voice_kit_fill(dub_params_t *p, uint8_t tune, uint8_t decay, uint8_t drive)
{
    uint8_t i;
    uint8_t t = tune ? tune : 64u;
    uint8_t d = decay ? decay : 64u;

    if (!p)
        return;
    for (i = 0; i < 4u; i++) {
        p->drm_tune[i] = t;
        p->drm_dec[i] = d;
        p->drm_drv[i] = drive;
    }
}

static inline void soul_era_apply_base(dub_params_t *p, uint8_t era)
{
    const soul_era_base_t *eb;
    const soul_era_fx_t *fx;
    uint8_t i;

    if (!p)
        return;
    era = soul_clamp_era(era);
    eb = &SOUL_ERA_BASE[era];
    fx = &SOUL_ERA_FX[era];
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
    dub_drm_voice_kit_fill(p, eb->kit_tune, eb->kit_decay, eb->kit_drive);
    p->siren_pitch = eb->siren_pitch;
    p->siren_rate = eb->siren_rate;
    dub_drm_set_bus_dly(p, eb->rim_dly);
    dub_drm_set_bus_rev(p, eb->rim_dly);
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
    p->lvl_c = 92;
    p->rev_b = (uint8_t)(20u + (fx->rev_mix >> 2));
    p->rev_c = (uint8_t)(22u + (fx->rev_mix >> 2));
}

static inline void soul_drm_dec_fit_apply(dub_params_t *p, uint8_t preset)
{
    uint8_t i;

    if (!p || preset >= GENRE_PRESET_N)
        return;
    for (i = 0; i < 3u; i++)
        p->drm_dec[i] = soul_drm_dec_fit[preset][i];
}

static inline void soul_trax_apply(dub_params_t *p, uint8_t side, uint8_t what)
{
    const soul_side_t *tx;
    uint8_t di;

    if (!p)
        return;
    side = soul_clamp_side(side);
    tx = soul_side(side);
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        soul_apply_defaults(p);
        soul_era_apply_base(p, tx->era);
        soul_drm_dec_fit_apply(p, side);
    }
    p->era = tx->era;
    p->plate = side;
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        p->bpm = tx->bpm;
        p->density = 64u;
        for (di = 0; di < 4u; di++)
            p->drm_dens[di] = 64u;
        p->drum_kit = soul_side_kit(side);
        p->bass_model = tx->bass_model;
        p->voicing = tx->voicing;
        p->pump = tx->pump;
        p->wobble = tx->wobble;
        p->feedback = tx->feedback;
        p->tank = tx->tank;
        p->tap = tx->tap;
        p->dly_c = tx->dly_c;
        p->rev_c = tx->rev_c;
        p->loop_filt = tx->loop_filt;
        p->root = soul_side_root(side);
        p->scale = soul_side_scale(side);
        p->swing = soul_side_swing(side);
        {
            int16_t lead = (int16_t)p->siren_rate + (int16_t)soul_side_lead_rate(side);

            if (lead < 0)
                lead = 0;
            else if (lead > 127)
                lead = 127;
            p->siren_rate = (uint8_t)lead;
        }
    }
}

static inline void soul_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t side)
{
    if (!p)
        return;
    era = soul_clamp_era(era);
    side = soul_clamp_side(side);
    soul_trax_apply(p, side, DUB_APPLY_ALL);
    p->era = era;
    p->plate = side;
    p->drum_kit = soul_side_kit(side);
    p->bass_model = soul_side(side)->bass_model;
    p->voicing = soul_side(side)->voicing;
    p->bpm = soul_side(side)->bpm;
    p->pump = soul_side(side)->pump;
    p->feedback = soul_side(side)->feedback;
    p->wobble = soul_side(side)->wobble;
    p->drum_pat = side;
    p->root = soul_side_root(side);
    p->scale = soul_side_scale(side);
    p->swing = soul_side_swing(side);
}

static inline void dub_drm_send_fixup(dub_params_t *p)
{
    uint8_t i;
    uint8_t dnz = 0u;
    uint8_t rnz = 0u;

    if (!p)
        return;
    for (i = 0; i < 4u; i++) {
        if (p->drm_dly[i])
            dnz = 1u;
        if (p->drm_rev[i])
            rnz = 1u;
    }
    if (!dnz)
        dub_drm_set_bus_dly(p, 28u);
    if (!rnz)
        dub_drm_set_bus_rev(p, 28u);
}

static inline void dub_plate_groove_read(uint8_t plate, uint8_t *root, uint8_t *scale, uint8_t *swing, uint8_t *bpm)
{
    const soul_side_t *tx;

    plate = soul_clamp_side(plate);
    tx = soul_side(plate);
    if (root)
        *root = soul_side_root(plate);
    if (scale)
        *scale = soul_side_scale(plate);
    if (swing)
        *swing = soul_side_swing(plate);
    if (bpm)
        *bpm = tx->bpm;
}

static inline void dub_resolve_step(const dub_params_t *p, const dub_step_t *ovr, uint8_t step, dub_events_t *out, uint8_t apply_dens)
{
    const soul_side_t *drum;
    const soul_side_t *main;
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t forced;

    if (!p || !out)
        return;
    if (step >= DUB_STEPS)
        step = (uint8_t)(DUB_STEPS - 1u);
    drum = soul_side(p->drum_pat);
    main = soul_side(p->plate);
    forced = (ovr != 0) && ((ovr->force & 1u) != 0);
    if (forced) {
        kick = (ovr->gate >> 0) & 1u;
        rim = (ovr->gate >> 1) & 1u;
        hat = (ovr->gate >> 2) & 1u;
        siren = (ovr->gate >> 3) & 1u;
        sub = (ovr->gate >> 4) & 1u;
        chord = (ovr->gate >> 5) & 1u;
    } else {
        kick = dub_step_bit(drum->kick, step);
        rim = dub_step_bit(drum->rim, step);
        hat = dub_step_bit(drum->hat, step);
        siren = soul_side_lead_step_bit(p->plate, step);
        sub = dub_step_bit(main->sub, step);
        chord = dub_step_bit(main->chord, step);
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

#include "soul_gen.h"

#endif
'''


def gen_data_h() -> str:
    lines: list[str] = [
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "#ifndef SOUL_DATA_H",
        "#define SOUL_DATA_H",
        "",
        '#include "dub_types.h"',
        '#include "soul_conf.h"',
        '#include "fm_seq.h"',
        "",
        "#define SGEN_M(lo, hi) ((uint32_t)(lo) | ((uint32_t)(hi) << 16))",
        "",
        "enum { SOUL_ERA_N = GENRE_ERA_N, SOUL_SIDE_N = GENRE_PRESET_N };",
        f"enum {{ {', '.join(f'ERA_{n}' for n in ERA_NAMES)} }};",
        "enum {",
        "    BASS_PEDAL = 0, BASS_UPRIGHT, BASS_FINGER, BASS_FLAT, BASS_THUMB,",
        "    BASS_PICK, BASS_OCTAVE, BASS_SYNTH, BASS_SUB, BASS_N",
        "};",
        "enum {",
        "    DKIT_CHAPEL = 0, DKIT_SNAP, DKIT_DEEP, DKIT_STOMP, DKIT_SILK,",
        "    DKIT_MACHINE, DKIT_TAMB, DKIT_CLAP, DKIT_N",
        "};",
        "enum {",
        "    VOICE_MAJ = 0, VOICE_MIN7, VOICE_DOM7, VOICE_DOM9, VOICE_MAJ6, VOICE_69,",
        "    VOICE_SUS4, VOICE_MIN9, VOICE_ADD9, VOICE_MAJ7, VOICE_DOM13, VOICE_MIN11, VOICE_N",
        "};",
        "",
        "typedef dub_plate_t soul_side_t;",
        "",
        f'static const char *const SOUL_ERA_NAMES[SOUL_ERA_N] = {{"' + '", "'.join(ERA_NAMES) + '"};',
        "",
        f'static const char *const DKIT_NAMES[DKIT_N] = {{"' + '", "'.join(DKIT_NAMES) + '"};',
        f"static const uint8_t DKIT_ERA[DKIT_N] = {{ {', '.join(str(e) for e in DKIT_ERA)} }};",
        "",
        "enum {",
        "    HAT_STYLE_TICK = 0, HAT_STYLE_SIZZLE, HAT_STYLE_TR6, HAT_STYLE_TONE, HAT_STYLE_HISS,",
        "    HAT_STYLE_TAMB",
        "};",
        "",
    ]
    hat_tables = [
        ("soul_hat_note", "int8_t", HAT_NOTE),
        ("soul_hat_fm", "uint8_t", HAT_FM),
        ("soul_hat_noise", "uint8_t", HAT_NOISE),
        ("soul_hat_metal", "uint8_t", HAT_METAL),
        ("soul_hat_decay", "uint8_t", HAT_DECAY),
        ("soul_hat_open", "uint8_t", HAT_OPEN),
        ("soul_hat_style", "uint8_t", HAT_STYLE),
        ("soul_hat_cut", "uint8_t", HAT_CUT),
    ]
    for name, ctype, vals in hat_tables:
        lines.append(f"static const {ctype} {name}[DKIT_N] = {{")
        lines.append("    " + ", ".join(str(v) for v in vals))
        lines.append("};")
    lines.append("")
    lines.append(f'static const char *const BASS_NAMES[BASS_N] = {{"' + '", "'.join(BASS_NAMES) + '"};')
    lines.append(f"static const uint8_t BASS_ERA[BASS_N] = {{ {', '.join(str(e) for e in BASS_ERA)} }};")
    lines.append("")
    lines.append(f'static const char *const VOICE_NAMES[VOICE_N] = {{"' + '", "'.join(VOICE_NAMES) + '"};')
    lines.append("")
    lines.append("static const soul_side_t SOUL_SIDE[SOUL_SIDE_N] = {")
    for era in range(ERA_N):
        for slot in range(PRESETS_PER_ERA):
            nm = SIDE_NAMES[era][slot]
            lines.append(f"    /* {ERA_NAMES[era]} {slot} */ {gen_plate_line(era, slot, nm)},")
    lines.append("};")
    lines.append("")

    roots, scales, swings, kits, lead_rates = [], [], [], [], []
    gates: list[int] = []
    pitches: list[list[int]] = []
    lens: list[list[int]] = []
    bends: list[list[int]] = []
    gate_cs_list: list[int] = []

    for era in range(ERA_N):
        for slot in range(PRESETS_PER_ERA):
            pi = era * PRESETS_PER_ERA + slot
            roots.append(str(48 + ERA_ROOT_PC[era][slot % 8]))
            scales.append(str(ERA_SCALE_POOL[era][slot % len(ERA_SCALE_POOL[era])]))
            swings.append(str(swing_for(era, slot)))
            kits.append(str(DKIT_ERA[era] if slot < 8 else DKIT_ERA[era]))
            lead_rates.append(str((slot % PRESETS_PER_ERA) - 4))
            g, p, l, b, gcs = gen_lead(era, slot, ERA_SCALE_POOL[era][slot % len(ERA_SCALE_POOL[era])])
            gates.append(f"0x{g:08X}u")
            pitches.append(p)
            lens.append(l)
            bends.append(b)
            gate_cs_list.append(gcs)

    lines.append(f"static const int8_t SOUL_SIDE_ROOT[SOUL_SIDE_N] = {{ {', '.join(roots)} }};")
    lines.append(f"static const int8_t SOUL_SIDE_SCALE[SOUL_SIDE_N] = {{ {', '.join(scales)} }};")
    lines.append(f"static const int8_t SOUL_SIDE_SWING[SOUL_SIDE_N] = {{ {', '.join(swings)} }};")
    lines.append(f"static const int8_t SOUL_SIDE_KIT[SOUL_SIDE_N] = {{ {', '.join(kits)} }};")
    lines.append(f"static const int8_t SOUL_SIDE_LEAD_RATE[SOUL_SIDE_N] = {{ {', '.join(lead_rates)} }};")
    lines.append("")
    lines.append("static const uint8_t SOUL_ERA_MACROS[SOUL_ERA_N][10] = {")
    for i, m in enumerate(SOUL_ERA_MACROS):
        lines.append(f"    {{ {', '.join(str(x) for x in m)} }}, /* {ERA_NAMES[i]} */")
    lines.append("};")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("    uint8_t timbre, cutoff_c, decay_c, reso_mode, env_filt;")
    lines.append("    uint8_t cutoff_b, decay_b, sub_oct, drive, dly_b;")
    lines.append("    uint8_t kick_boom, rim_tone, hat_lvl, kit_tune, kit_decay, kit_drive;")
    lines.append("    uint8_t siren_pitch, siren_rate, rim_dly;")
    lines.append("    uint8_t drm_pan[4];")
    lines.append("    uint8_t wow, dly_time, isolator, swing;")
    lines.append("} soul_era_base_t;")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("    uint8_t drum_kit, tank, tap, loop_filt, feedback;")
    lines.append("    uint8_t dly_lvl, rev_mix, rev_damp, rev_tail, dly_rev;")
    lines.append("    uint8_t pump, bass_model, wobble;")
    lines.append("} soul_era_fx_t;")
    lines.append("")
    lines.append("static const soul_era_base_t SOUL_ERA_BASE[SOUL_ERA_N] = {")
    for row in ERA_BASE:
        lines.append("    " + fmt_era_base(row) + ",")
    lines.append("};")
    lines.append("")
    lines.append("static const soul_era_fx_t SOUL_ERA_FX[SOUL_ERA_N] = {")
    dkit_enum = ["DKIT_CHAPEL", "DKIT_SNAP", "DKIT_DEEP", "DKIT_STOMP", "DKIT_SILK", "DKIT_MACHINE"]
    bass_enum = ["BASS_PEDAL", "BASS_FINGER", "BASS_FLAT", "BASS_PICK", "BASS_OCTAVE", "BASS_SYNTH", "BASS_SUB"]
    for i, fx in enumerate(ERA_FX):
        lines.append(
            f"    {{ {dkit_enum[i]}, {fx[1]}, {fx[2]}, {fx[3]}, {fx[4]}, {fx[5]}, {fx[6]}, "
            f"{fx[7]}, {fx[8]}, {fx[9]}, {fx[10]}, {bass_enum[i] if i < len(bass_enum) else 'BASS_PEDAL'}, {fx[12]} }},"
        )
    lines.append("};")
    lines.append("")
    lines.append("static const uint8_t SOUL_BASS_WALK[SOUL_ERA_N][GENRE_PRESETS_PER_ERA][8] = {")
    for era in range(ERA_N):
        lines.append("    {")
        for slot in range(PRESETS_PER_ERA):
            row = SOUL_BASS_WALK[era][slot]
            lines.append("        { " + ", ".join(str(x) for x in row) + " },")
        lines.append("    },")
    lines.append("};")
    lines.append("")
    lines.append("static const uint32_t SOUL_SIDE_LEAD_GATE[SOUL_SIDE_N] = {")
    for i in range(0, PRESET_N, PRESETS_PER_ERA):
        lines.append("    " + ", ".join(gates[i : i + PRESETS_PER_ERA]) + ",")
    lines.append("};")
    lines.append("")
    lines.append("static const int8_t SOUL_SIDE_LEAD_PITCH[SOUL_SIDE_N][32] = {")
    for i, row in enumerate(pitches):
        lines.append("    { " + ", ".join(f"{v:4d}" for v in row) + f" }}, /* {ERA_NAMES[i // PRESETS_PER_ERA]} {i % PRESETS_PER_ERA} */")
    lines.append("};")
    lines.append("")
    lines.append("static const uint8_t SOUL_SIDE_LEAD_LEN[SOUL_SIDE_N][32] = {")
    for row in lens:
        lines.append("    { " + ", ".join(f"{v:3d}" for v in row) + " },")
    lines.append("};")
    lines.append("")
    lines.append("static const uint8_t SOUL_SIDE_LEAD_BEND[SOUL_SIDE_N][32] = {")
    for row in bends:
        lines.append("    { " + ", ".join(f"{v:3d}" for v in row) + " },")
    lines.append("};")
    lines.append("")
    lines.append(f"static const uint8_t SOUL_SIDE_LEAD_GATE_CS[SOUL_SIDE_N] = {{ {', '.join(str(x) for x in gate_cs_list)} }};")
    lines.append("")
    lines.extend(gen_drm_dec_fit())
    lines.append("#define GENRE_DRM_DEC_FIT soul_drm_dec_fit")
    lines.append("")
    lines.append(APPLY_TAIL)
    return "\n".join(lines) + "\n"


def main() -> int:
    text = gen_data_h()
    if "--check" in sys.argv:
        if not OUT.is_file():
            print(f"missing {OUT.name}", file=sys.stderr)
            return 1
        cur = OUT.read_text(encoding="utf-8")
        if cur != text:
            print(f"{OUT.name} stale (run without --check)", file=sys.stderr)
            return 1
        return 0
    OUT.write_text(text, encoding="utf-8")
    print(f"wrote {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
