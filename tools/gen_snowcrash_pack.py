#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate snowcrash_data.h, fm_views snowcrash section, snowcrash_dsp.h (from idm template)."""
from __future__ import annotations

import hashlib
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "firmware" / "src"

ERA_N = 6
ERA_NAMES = ["CRASH", "GLOSS", "SUMER", "STATIC", "TIDE", "CHAPEL"]
ERA_FULL = [
    "SNOWCRASH", "GLOSSCRASH", "SUMCRASH", "ICECRASH", "TIDECRASH", "WARPCRASH",
]

SLAB_NAMES = [
    ["HOPSCOT", "BOUNCE", "SKIPPY", "TICKLE", "JOGGER", "PULSAR", "STEPUP", "LIMBER", "RUNWAY", "BEATBOX"],
    ["BEATNIK", "RIPPLE", "DRIFTLN", "WOBBLE", "FRINGE", "ECHOED", "BANDING", "SHEARS", "ZIPPER", "MOIRED"],
    ["KNOTWORK", "SNARL", "CROSSED", "WEAVER", "BRAIDS", "TANGLO", "LOOPER", "VINES", "SPOOLED", "KNIT"],
    ["CHECKER", "TILEWAY", "GRIDLOK", "CHESS", "PLAID", "ALTERNA", "BOARD", "CHEQ", "SQUARES", "DITHER"],
    ["GLIMMER", "SPARKLE", "SHIMMY", "TWINKLE", "GLINT", "IRIDESC", "FLICKA", "PRISMA", "SHEEN", "SHINER"],
    ["MAZEWAY", "LABYRNT", "DOORS", "PHASED", "RECALL", "NEWSTEP", "HALLWAY", "SPIRAL", "CORNERS", "DETOUR"],
]

BASS_NAMES = [
    "SUB", "808", "REESE", "303", "FM", "MOOG", "PWM", "PLUCK",
    "RUST", "UNDRT", "HUM", "CLICK", "GLOSS", "TIDE",
]
# era remap: old BSUN/STATIC→ICE(3), HOLLOW/CHAPEL→WARP(5)
BASS_ERA = [3, 1, 2, 2, 3, 5, 3, 0, 1, 1, 5, 5, 0, 4]
BASS_N = len(BASS_NAMES)

DKIT_NAMES = ["CLICK", "HUM", "FOAM", "WIRE", "SLAG", "GLASS", "RIME", "CRASH"]
DKIT_ERA = [5, 5, 0, 2, 1, 5, 4, 5]

VOICE_NAMES = [
    "MAJ7", "MIN7", "DOM9", "MIN9", "SUS4", "69",
    "QUART", "ADD9", "MAJ6", "DOM13", "MIN11", "ADD11",
]

HAT_NOTE = [86, 78, 84, 90, 88, 92, 80, 98]
HAT_FM = [12, 6, 22, 48, 38, 18, 14, 72]
HAT_NOISE = [24, 14, 20, 88, 64, 18, 28, 102]
HAT_METAL = [88, 120, 76, 68, 82, 94, 58, 72]
HAT_DECAY = [8, 18, 6, 3, 5, 4, 10, 2]
HAT_OPEN = [6, 22, 4, 2, 3, 2, 8, 1]
HAT_STYLE = [0, 4, 1, 2, 2, 1, 0, 2]  # TICK/SIZZLE/TR6/TONE/HISS
HAT_CUT = [56, 38, 60, 78, 52, 62, 48, 80]

PRESETS_PER_ERA = 10
PRESET_N = ERA_N * PRESETS_PER_ERA

# Per-era groove targets (SNOW dense … WARP sparse)
ERA_BPM_LO = [88, 92, 100, 104, 110, 116]
ERA_BPM_HI = [96, 100, 108, 112, 118, 124]
ERA_DENS_BASE = [40, 46, 52, 44, 36, 56]
ERA_SWING_BASE = [60, 62, 58, 64, 60, 62]
ERA_PUMP_BASE = [58, 54, 62, 52, 60, 56]
ERA_DLY_C_BASE = [52, 72, 58, 48, 62, 78]
ERA_REV_C_BASE = [62, 70, 64, 58, 66, 82]

# Interference groove: lane period per era. Lane order = dub_plate_t masks, then the lead gate:
# kick rim hat sub chord siren lead. Each lane tiles its p-step pattern over the 32-step loop
# (mask bit s = pattern bit s mod p). 32 is no multiple of 5/7/9/11, so the lanes drift against each other.
LANE_N = 7
LANE_PERIOD = [
    [5, 7, 9, 11, 5, 7, 9],     # CRASH
    [5, 9, 7, 5, 11, 9, 7],     # GLOSS
    [5, 11, 5, 9, 7, 5, 11],    # SUMER
    [5, 5, 11, 7, 9, 11, 5],    # STATIC
    [5, 7, 5, 9, 11, 7, 5],     # TIDE
    [5, 11, 9, 7, 5, 11, 7],    # CHAPEL
]
# Pattern density per lane (fraction of the p bits that may be on), shifted per era by ERA_DENS_OFF.
LANE_DENS = [(0.2, 0.4), (0.4, 0.6), (0.5, 0.8), (0.3, 0.5), (0.3, 0.5), (0.2, 0.4), (0.3, 0.5)]
ERA_DENS_OFF = [0.0, 0.1, -0.1, 0.05, 0.0, -0.05]
# Tuned percussion (Karplus-Strong loop length = 44100 / pitch): rim and hat notes per era.
KS_F_RIM = [587, 523, 440, 659, 392, 494]
KS_F_HAT = [1760, 1397, 1319, 1568, 1245, 1175]
KS_FS = 44100


def mask16(seed: int, era: int, ix: int) -> int:
    h = int(hashlib.md5(f"{seed}-{era}-{ix}".encode()).hexdigest()[:4], 16)
    return h & 0xFFFF


def mask32(seed: int, era: int, ix: int) -> int:
    lo = mask16(seed, era, ix)
    hi = mask16(seed + 1, era, ix)
    return lo | (hi << 16)


def popc(x: int) -> int:
    return bin(x).count("1")


def tile(pat: int, p: int) -> int:
    """32-step lane: mask bit s = pattern bit (s mod p)."""
    m = 0
    for s in range(32):
        if (pat >> (s % p)) & 1:
            m |= 1 << s
    return m


def pcount(pat: int, p: int) -> int:
    """Hits of the tiled lane. Matches snowcrash_gen_pcount() in snowcrash_gen.h."""
    q, r = divmod(32, p)
    return q * popc(pat) + popc(pat & ((1 << r) - 1))


def lane_band(era: int, lane: int) -> tuple[int, int]:
    """Allowed pattern popcount (kmin..kmax) of a lane in an era."""
    p = LANE_PERIOD[era][lane]
    lo, hi = LANE_DENS[lane]
    lo = max(0.0, lo + ERA_DENS_OFF[era])
    hi = min(1.0, hi + ERA_DENS_OFF[era])
    kmin = max(1, round(p * lo))
    kmax = min(p - 1, max(kmin, round(p * hi)))
    return kmin, kmax


def lane_window(era: int, lane: int) -> tuple[int, int]:
    """Hit window [lo, hi] over the 32 steps that every pattern in the band lands in (SNOWCRASH_GEN_HITS)."""
    p = LANE_PERIOD[era][lane]
    kmin, kmax = lane_band(era, lane)
    counts = [pcount(x, p) for x in range(1 << p) if kmin <= popc(x) <= kmax]
    lo, hi = min(counts), max(counts)
    return lo, hi


def lane_pattern(seed: int, era: int, ix: int, lane: int) -> int:
    p = LANE_PERIOD[era][lane]
    kmin, kmax = lane_band(era, lane)
    pat = mask16(seed, era, ix) & ((1 << p) - 1)
    if lane == 0:
        pat |= 1  # kick: downbeat anchor on residue 0 (every p steps)
    start = mask16(seed + 50, era, ix) % p
    while popc(pat) < kmin:
        for j in range(p):
            r = (start + j) % p
            if not (pat >> r) & 1:
                pat |= 1 << r
                break
    while popc(pat) > kmax:
        for j in range(p):
            r = (start + j) % p
            if (pat >> r) & 1 and not (lane == 0 and r == 0):
                pat &= ~(1 << r)
                break
    return pat


def lane_mask(seed: int, era: int, ix: int, lane: int) -> int:
    return tile(lane_pattern(seed, era, ix, lane), LANE_PERIOD[era][lane])


def gen_plate_line(era: int, ix: int, name: str) -> str:
    bass = BASS_ERA[(era * 3 + ix) % BASS_N]
    voice = (era + ix) % 12
    tap = ix & 3
    tank = (era * 11 + ix * 7) % 96
    if era == 5:
        tank = 24 + (ix * 5) % 56
    span = max(1, PRESETS_PER_ERA - 1)
    bpm = ERA_BPM_LO[era] + (ix * (ERA_BPM_HI[era] - ERA_BPM_LO[era])) // span
    dens = ERA_DENS_BASE[era] + (ix % 5) + (era == 0)
    dly_c = ERA_DLY_C_BASE[era] + (ix * 2) % 24
    rev_c = ERA_REV_C_BASE[era] + (ix * 3) % 20
    fb = 12 + (era + ix) % 28
    lf = 38 + (era * 5) % 30
    wob = (era * 9 + ix * 5) % 80
    if era == 4:
        wob = 40 + (ix * 4) % 40
    pump = ERA_PUMP_BASE[era] + (ix * 2) % 18
    k = lane_mask(1, era, ix, 0)
    r = lane_mask(2, era, ix, 1)
    h = lane_mask(3, era, ix, 2)
    sub = lane_mask(4, era, ix, 3)
    ch = lane_mask(5, era, ix, 4)
    sr = lane_mask(6, era, ix, 5)
    return (
        f'{{ "{name}", {era}, {bass}, {voice}, {tap}, {tank}, {bpm}, {dens}, '
        f'{dly_c}, {rev_c}, {fb}, {lf}, {wob}, {pump}, '
        f'0x{k:08X}u, 0x{r:08X}u, 0x{h:08X}u, 0x{sub:08X}u, 0x{ch:08X}u, 0x{sr:08X}u }}'
    )


def gen_data_h() -> str:
    lines = [
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "#ifndef SNOWCRASH_DATA_H",
        "#define SNOWCRASH_DATA_H",
        "",
        '#include "dub_types.h"',
        '#include "snowcrash_conf.h"',
        '#include "fm_seq.h"',
        "",
        "enum { SNOWCRASH_ERA_N = GENRE_ERA_N, SNOWCRASH_SLAB_N = GENRE_PRESET_N };",
    ]
    era_enum = ", ".join(f"ERA_{n}" for n in ERA_NAMES)
    lines.append(f"enum {{ {era_enum} }};")
    lines.append(
        "enum {\n"
        "    BASS_SUB = 0, BASS_808, BASS_REESE, BASS_303, BASS_FM, BASS_MOOG, BASS_PWM, BASS_PLUCK,\n"
        "    BASS_RUST, BASS_UNDRT, BASS_HUM, BASS_CLICK, BASS_GLOSS, BASS_TIDE,\n"
        "    BASS_N\n"
        "};"
    )
    lines.append(
        "enum {\n"
        "    DKIT_CLICK = 0, DKIT_HUM, DKIT_FOAM, DKIT_WIRE, DKIT_SLAG, DKIT_GLASS, DKIT_RIME, DKIT_CRASH, DKIT_N\n"
        "};"
    )
    lines.append(
        "enum {\n"
        "    VOICE_MAJ7 = 0, VOICE_MIN7, VOICE_DOM9, VOICE_MIN9, VOICE_SUS4, VOICE_69,\n"
        "    VOICE_QUART, VOICE_ADD9, VOICE_MAJ6, VOICE_DOM13, VOICE_MIN11, VOICE_ADD11,\n"
        "    VOICE_N\n"
        "};"
    )
    lines.append("")
    lines.append("typedef dub_plate_t snowcrash_slab_t;")
    lines.append("")
    lines.append(f'static const char *const SNOWCRASH_ERA_NAMES[SNOWCRASH_ERA_N] = {{')
    lines.append('    "' + '", "'.join(ERA_FULL) + '"')
    lines.append("};")
    lines.append("")
    lines.append(f'static const char *const DKIT_NAMES[DKIT_N] = {{')
    lines.append('    "' + '", "'.join(DKIT_NAMES) + '"')
    lines.append("};")
    lines.append("")
    lines.append(f"static const uint8_t DKIT_ERA[DKIT_N] = {{ {', '.join(str(e) for e in DKIT_ERA)} }};")
    lines.append("")
    lines.append("enum {")
    lines.append("    HAT_STYLE_TICK = 0, HAT_STYLE_SIZZLE, HAT_STYLE_TR6, HAT_STYLE_TONE, HAT_STYLE_HISS")
    lines.append("};")
    lines.append("")
    for arr, name, vals in [
        (HAT_NOTE, "snowcrash_hat_note", HAT_NOTE),
        (HAT_FM, "snowcrash_hat_fm", HAT_FM),
        (HAT_NOISE, "snowcrash_hat_noise", HAT_NOISE),
        (HAT_METAL, "snowcrash_hat_metal", HAT_METAL),
        (HAT_DECAY, "snowcrash_hat_decay", HAT_DECAY),
        (HAT_OPEN, "snowcrash_hat_open", HAT_OPEN),
        (HAT_STYLE, "snowcrash_hat_style", HAT_STYLE),
        (HAT_CUT, "snowcrash_hat_cut", HAT_CUT),
    ]:
        lines.append(f"static const {'int8_t' if 'note' in name else 'uint8_t'} {name}[DKIT_N] = {{")
        lines.append("    " + ", ".join(str(v) for v in vals))
        lines.append("};")
    lines.append("")
    lines.append(f'static const char *const BASS_NAMES[BASS_N] = {{')
    lines.append('    "' + '", "'.join(BASS_NAMES) + '"')
    lines.append("};")
    lines.append(f"static const uint8_t BASS_ERA[BASS_N] = {{ {', '.join(str(e) for e in BASS_ERA)} }};")
    lines.append("")
    lines.append(f'static const char *const VOICE_NAMES[VOICE_N] = {{')
    lines.append('    "' + '", "'.join(VOICE_NAMES) + '"')
    lines.append("};")
    lines.append("")
    lines.append("static const snowcrash_slab_t SNOWCRASH_SLAB[SNOWCRASH_SLAB_N] = {")
    for era in range(ERA_N):
        for ix in range(PRESETS_PER_ERA):
            nm = SLAB_NAMES[era][ix]
            lines.append(f"    /* {ERA_NAMES[era]} {ix} */ {gen_plate_line(era, ix, nm)},")
    lines.append("};")
    lines.append("")

    roots = []
    scales = []
    swings = []
    for pi in range(PRESET_N):
        roots.append(str(48 + (pi * 5 + pi // PRESETS_PER_ERA) % 14))
        scales.append(str((pi * 3 + pi // 4) % 6))
        er = pi // PRESETS_PER_ERA
        swings.append(str(ERA_SWING_BASE[er] + (pi % 3) - 1))
    lines.append(f"static const int8_t SNOWCRASH_SLAB_ROOT[SNOWCRASH_SLAB_N] = {{")
    lines.append(", ".join(roots))
    lines.append("};")
    lines.append(f"static const int8_t SNOWCRASH_SLAB_SCALE[SNOWCRASH_SLAB_N] = {{")
    lines.append(", ".join(scales))
    lines.append("};")
    lines.append(f"static const int8_t SNOWCRASH_SLAB_SWING[SNOWCRASH_SLAB_N] = {{")
    lines.append(", ".join(swings))
    lines.append("};")

    lead_rate = [str((pi % PRESETS_PER_ERA) - 3) for pi in range(PRESET_N)]
    lines.append(f"static const int8_t SNOWCRASH_SLAB_LEAD_RATE[SNOWCRASH_SLAB_N] = {{")
    lines.append(", ".join(lead_rate))
    lines.append("};")

    macros = [
        [68, 10, 12, 38, 48, 18, 22, 8, 8, 36],
        [62, 8, 10, 42, 44, 20, 28, 10, 12, 40],
        [54, 12, 14, 46, 40, 22, 34, 14, 32, 52],
        [56, 10, 8, 40, 46, 16, 30, 6, 20, 44],
        [58, 14, 18, 44, 42, 24, 18, 16, 10, 42],
        [52, 12, 16, 48, 38, 20, 36, 18, 56, 58],
    ]
    lines.append("static const uint8_t SNOWCRASH_ERA_MACROS[SNOWCRASH_ERA_N][10] = {")
    for i, m in enumerate(macros):
        lines.append(f"    {{ {', '.join(str(x) for x in m)} }}, /* {ERA_NAMES[i]} */")
    lines.append("};")
    lines.append("")

    # era base/fx - compact templates
    lines.append("""typedef struct {
    uint8_t timbre, cutoff_c, decay_c, reso_mode, env_filt;
    uint8_t cutoff_b, decay_b, sub_oct, drive, dly_b;
    uint8_t kick_boom, rim_tone, hat_lvl, kit_tune, kit_decay, kit_drive;
    uint8_t siren_pitch, siren_rate, rim_dly;
    uint8_t drm_pan[4];
    uint8_t wow, dly_time, isolator, swing;
} snowcrash_era_base_t;

typedef struct {
    uint8_t drum_kit, tank, tap, loop_filt, feedback;
    uint8_t dly_lvl, rev_mix, rev_damp, rev_tail, dly_rev;
    uint8_t pump, bass_model, wobble;
} snowcrash_era_fx_t;

static const snowcrash_era_base_t SNOWCRASH_ERA_BASE[SNOWCRASH_ERA_N] = {
    { 34, 90, 92, 20, 44, 84, 94, 28, 14, 26, 74, 54, 52, 64, 66, 16, 100, 42, 36, {64,68,52,76}, 14, 52, 28, 68 },
    { 40, 78, 86, 32, 56, 72, 88, 22, 24, 18, 82, 58, 56, 64, 54, 22, 96, 48, 30, {64,74,48,82}, 10, 44, 36, 66 },
    { 44, 74, 70, 44, 68, 70, 82, 36, 28, 14, 84, 62, 68, 64, 50, 28, 104, 62, 24, {64,78,46,88}, 18, 40, 42, 64 },
    { 36, 92, 88, 28, 46, 78, 86, 26, 22, 26, 70, 56, 64, 64, 58, 18, 102, 36, 32, {62,72,50,80}, 12, 48, 32, 60 },
    { 32, 86, 94, 18, 40, 84, 96, 32, 12, 34, 68, 50, 48, 64, 72, 12, 92, 38, 40, {58,68,46,76}, 20, 58, 24, 70 },
    { 46, 76, 72, 36, 60, 68, 80, 18, 26, 16, 86, 64, 72, 64, 48, 24, 106, 72, 22, {64,82,44,90}, 22, 38, 48, 72 },
};

static const snowcrash_era_fx_t SNOWCRASH_ERA_FX[SNOWCRASH_ERA_N] = {
    { DKIT_FOAM,  28, 1, 42, 18, 62, 36, 34, 58, 48,  68, BASS_HUM,    12 },
    { DKIT_SLAG,  22, 0, 48, 16, 72, 32, 28, 50, 44,  58, BASS_GLOSS,  24 },
    { DKIT_WIRE,  18, 0, 50, 20, 58, 34, 26, 48, 46,  54, BASS_UNDRT,  32 },
    { DKIT_CLICK, 14, 0, 46, 14, 56, 38, 30, 52, 42,  52, BASS_FM,     18 },
    { DKIT_RIME,  26, 2, 40, 14, 78, 40, 38, 60, 52,  72, BASS_TIDE,    8 },
    { DKIT_CRASH, 12, 0, 52, 22, 52, 36, 22, 40, 48,  48, BASS_PWM,    36 },
};
""")

    # lead gates - simplified 32-bit patterns
    gates = []
    for pi in range(PRESET_N):
        g = lane_mask(100, pi // PRESETS_PER_ERA, pi % PRESETS_PER_ERA, 6)
        gates.append(f"0x{g:08X}u")
    lines.append("static const uint32_t SNOWCRASH_SLAB_LEAD_GATE[SNOWCRASH_SLAB_N] = {")
    for i in range(0, PRESET_N, PRESETS_PER_ERA):
        lines.append("    " + ", ".join(gates[i : i + PRESETS_PER_ERA]) + ",")
    lines.append("};")

    kit_map = []
    for pi in range(PRESET_N):
        kit_map.append(
            str(DKIT_ERA[pi % len(DKIT_ERA)] if pi % 3 else (pi // PRESETS_PER_ERA) % ERA_N)
        )
    lines.append("static const int8_t SNOWCRASH_SLAB_KIT[SNOWCRASH_SLAB_N] = {")
    lines.append(", ".join(kit_map))
    lines.append("};")

    # Minimal lead sidecars (pitch/len/bend) - zeros + sparse
    lines.append("static const int8_t SNOWCRASH_SLAB_LEAD_PITCH[SNOWCRASH_SLAB_N][16] = {")
    for pi in range(PRESET_N):
        row = [0] * 16
        row[pi % 16] = (pi % 7) - 3
        if pi % 5 == 0:
            row[(pi + 3) % 16] = 2
        lines.append("    { " + ", ".join(f"{v:4d}" for v in row) + " },")
    lines.append("};")

    lines.append("static const uint8_t SNOWCRASH_SLAB_LEAD_LEN[SNOWCRASH_SLAB_N][32] = {")
    for pi in range(PRESET_N):
        row = [0] * 32
        row[pi % 32] = 9 + (pi % 4)
        lines.append("    { " + ", ".join(f"{v:3d}" for v in row) + " },")
    lines.append("};")

    lines.append("static const uint8_t SNOWCRASH_SLAB_LEAD_BEND[SNOWCRASH_SLAB_N][32] = {")
    for pi in range(PRESET_N):
        row = [0] * 32
        row[pi % 32] = 3 + (pi % 5)
        lines.append("    { " + ", ".join(f"{v:3d}" for v in row) + " },")
    lines.append("};")

    gate_cs = [str(28 + (pi // PRESETS_PER_ERA) * 2) for pi in range(PRESET_N)]
    lines.append("static const uint8_t SNOWCRASH_SLAB_LEAD_GATE_CS[SNOWCRASH_SLAB_N] = {")
    lines.append(", ".join(gate_cs))
    lines.append("};")
    lines.append("")
    lines.append("static const uint8_t SNOWCRASH_LEAD_LEN_16THS[15] = {")
    lines.append("    1, 2, 4, 5, 8, 11, 12, 16, 20, 24, 28, 32, 40, 48, 64")
    lines.append("};")

    lines.append("/* Interference groove: lane period per era (kick rim hat sub chord siren lead). */")
    lines.append(f"static const uint8_t SNOWCRASH_LANE_PERIOD[SNOWCRASH_ERA_N][{LANE_N}] = {{")
    for row in LANE_PERIOD:
        lines.append("    { " + ", ".join(str(v) for v in row) + " },")
    lines.append("};")
    lines.append("/* Karplus-Strong loop length per era: [rim, hat] = KS_FS / note Hz. */")
    lines.append("static const uint8_t SNOWCRASH_KS_LEN[SNOWCRASH_ERA_N][2] = {")
    for er in range(ERA_N):
        lines.append(f"    {{ {round(KS_FS / KS_F_RIM[er])}, {round(KS_FS / KS_F_HAT[er])} }},")
    lines.append("};")
    lines.append("static uint8_t snowcrash_ks_era;  /* set by snowcrash_era_apply_base(), read by the pluck trigger */")
    lines.append("")

    # Apply helpers - copy idm pattern with renames
    lines.append(open(SRC / "_snowcrash_apply_snippet.h").read() if (SRC / "_snowcrash_apply_snippet.h").exists() else "")
    lines.append("#endif")
    return "\n".join(lines)


def write_apply_snippet():
    snippet = r'''
#include "fm_pgen_core.h"

/* Live generated plate (endless continuum); shadows the ROM slab at snowcrash_gen.slot. */
static fm_pgen_t snowcrash_gen = { .slot = 0xFFu };
static fm_pgen_lead_t snowcrash_gen_lead;

#define FM_PGEN_STATE snowcrash_gen
#define FM_PGEN_BUILD(era, n, c) snowcrash_gen_build(era, n, c)

static inline uint8_t snowcrash_gen_hit(uint8_t ix)
{
    return fm_pgen_hit(&snowcrash_gen, ix);
}

static inline uint8_t snowcrash_clamp_slab(uint8_t ix)
{
    return ix >= SNOWCRASH_SLAB_N ? (uint8_t)(SNOWCRASH_SLAB_N - 1u) : ix;
}

static inline uint8_t snowcrash_clamp_era(uint8_t era)
{
    return era >= SNOWCRASH_ERA_N ? (uint8_t)(SNOWCRASH_ERA_N - 1u) : era;
}

static inline const char *snowcrash_era_name(uint8_t era)
{
    return SNOWCRASH_ERA_NAMES[snowcrash_clamp_era(era)];
}

static inline const char *snowcrash_slab_name(uint8_t slab)
{
    if (snowcrash_gen_hit(slab))
        return snowcrash_gen.name;
    return SNOWCRASH_SLAB[snowcrash_clamp_slab(slab)].name;
}

static inline const snowcrash_slab_t *snowcrash_slab(uint8_t ix)
{
    if (snowcrash_gen_hit(ix))
        return &snowcrash_gen.pl;
    return &SNOWCRASH_SLAB[snowcrash_clamp_slab(ix)];
}

static inline uint8_t snowcrash_slab_root(uint8_t ix)
{
    return snowcrash_gen_hit(ix) ? (uint8_t)snowcrash_gen.root : (uint8_t)SNOWCRASH_SLAB_ROOT[snowcrash_clamp_slab(ix)];
}

static inline uint8_t snowcrash_slab_scale(uint8_t ix)
{
    return snowcrash_gen_hit(ix) ? (uint8_t)snowcrash_gen.scale : (uint8_t)SNOWCRASH_SLAB_SCALE[snowcrash_clamp_slab(ix)];
}

static inline uint8_t snowcrash_slab_swing(uint8_t ix)
{
    return snowcrash_gen_hit(ix) ? (uint8_t)snowcrash_gen.swing : (uint8_t)SNOWCRASH_SLAB_SWING[snowcrash_clamp_slab(ix)];
}

static inline uint8_t snowcrash_slab_kit(uint8_t ix)
{
    return snowcrash_gen_hit(ix) ? (uint8_t)snowcrash_gen.kit : (uint8_t)SNOWCRASH_SLAB_KIT[snowcrash_clamp_slab(ix)];
}

static inline int8_t snowcrash_slab_lead_rate(uint8_t ix)
{
    return snowcrash_gen_hit(ix) ? snowcrash_gen.rate : SNOWCRASH_SLAB_LEAD_RATE[snowcrash_clamp_slab(ix)];
}

static inline uint8_t snowcrash_era_of_slab(uint8_t slab)
{
    slab = snowcrash_clamp_slab(slab);
    return (uint8_t)(slab / GENRE_PRESETS_PER_ERA);
}

static inline uint8_t snowcrash_era_first_slab(uint8_t era)
{
    return (uint8_t)(snowcrash_clamp_era(era) * GENRE_PRESETS_PER_ERA);
}

static inline uint32_t snowcrash_slab_lead_gate32(uint8_t slab)
{
    if (snowcrash_gen_hit(slab))
        return snowcrash_gen.lead_gate;
    return SNOWCRASH_SLAB_LEAD_GATE[snowcrash_clamp_slab(slab)];
}

static inline uint8_t snowcrash_slab_lead_gate_cs(uint8_t slab)
{
    if (snowcrash_gen_hit(slab))
        return snowcrash_gen.gate_cs;
    return SNOWCRASH_SLAB_LEAD_GATE_CS[snowcrash_clamp_slab(slab)];
}

static inline uint8_t snowcrash_slab_lead_step_bit(uint8_t slab, uint8_t step)
{
    uint32_t g32 = snowcrash_slab_lead_gate32(slab);
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

static inline int8_t snowcrash_slab_lead_pitch(uint8_t slab, uint8_t step)
{
    if (snowcrash_gen_hit(slab))
        return snowcrash_gen_lead.pitch[step & 15u];
    return SNOWCRASH_SLAB_LEAD_PITCH[snowcrash_clamp_slab(slab)][step & 15u];
}

static inline uint8_t snowcrash_slab_lead_len_ix(uint8_t slab, uint8_t step)
{
    if (snowcrash_gen_hit(slab))
        return snowcrash_gen_lead.len[step & 31u];
    return SNOWCRASH_SLAB_LEAD_LEN[snowcrash_clamp_slab(slab)][step & 31u];
}

static inline uint8_t snowcrash_slab_lead_bend_u7(uint8_t slab, uint8_t step)
{
    if (snowcrash_gen_hit(slab))
        return snowcrash_gen_lead.bend[step & 31u];
    return SNOWCRASH_SLAB_LEAD_BEND[snowcrash_clamp_slab(slab)][step & 31u];
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
        dub_drm_set_bus_rev(p, 24u);
}

static inline void snowcrash_apply_defaults(dub_params_t *p)
{
    uint8_t di;

    if (!p)
        return;
    p->timbre = 36;
    p->cutoff_c = 86;
    p->decay_c = 88;
    p->cutoff_b = 82;
    p->decay_b = 92;
    p->sub_oct = 26;
    p->drive = 14;
    p->pump = 64;
    p->feedback = 16;
    p->density = 100;
    p->wobble = 12;
    p->kick_boom = 76;
    p->rim_tone = 56;
    p->hat_lvl = 58;
    p->siren_pitch = 100;
    p->siren_rate = 44;
    p->drum_kit = DKIT_FOAM;
    for (di = 0; di < 4u; di++) {
        p->drm_tune[di] = 64u;
        p->drm_dec[di] = 64u;
        p->drm_drv[di] = 20u;
        p->drm_dly[di] = 28u;
        p->drm_rev[di] = 24u;
        p->drm_cut[di] = 64u;
        p->drm_atk[di] = 0u;
        p->drm_dens[di] = 100u;
    }
}

static inline void snowcrash_era_apply_base(dub_params_t *p, uint8_t era)
{
    const snowcrash_era_base_t *eb;
    const snowcrash_era_fx_t *fx;
    uint8_t i;

    if (!p)
        return;
    era = snowcrash_clamp_era(era);
    eb = &SNOWCRASH_ERA_BASE[era];
    fx = &SNOWCRASH_ERA_FX[era];
    p->era = era;
    snowcrash_ks_era = era;
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
    p->lvl_c = 92;
    p->rev_b = (uint8_t)(20u + (fx->rev_mix >> 2));
    p->rev_c = (uint8_t)(22u + (fx->rev_mix >> 2));
}

static inline void snowcrash_slab_apply(dub_params_t *p, uint8_t slab, uint8_t what)
{
    const snowcrash_slab_t *sc;
    uint8_t di;

    if (!p)
        return;
    slab = snowcrash_clamp_slab(slab);
    sc = snowcrash_slab(slab);
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        snowcrash_apply_defaults(p);
        snowcrash_era_apply_base(p, sc->era);
    }
    p->era = sc->era;
    p->plate = slab;
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        p->bpm = sc->bpm;
        p->density = sc->density;
        for (di = 0; di < 4u; di++)
            p->drm_dens[di] = sc->density;
        p->drum_kit = snowcrash_slab_kit(slab);
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
        p->root = snowcrash_slab_root(slab);
        p->scale = snowcrash_slab_scale(slab);
        p->swing = snowcrash_slab_swing(slab);
        {
            int16_t lead = (int16_t)p->siren_rate + (int16_t)snowcrash_slab_lead_rate(slab);

            if (lead < 0)
                lead = 0;
            else if (lead > 127)
                lead = 127;
            p->siren_rate = (uint8_t)lead;
        }
    }
}

static inline void snowcrash_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t slab)
{
    if (!p)
        return;
    era = snowcrash_clamp_era(era);
    slab = snowcrash_clamp_slab(slab);
    snowcrash_slab_apply(p, slab, DUB_APPLY_ALL);
    p->era = era;
    p->plate = slab;
    p->drum_kit = snowcrash_slab_kit(slab);
    p->bass_model = snowcrash_slab(slab)->bass_model;
    p->voicing = snowcrash_slab(slab)->voicing;
    p->bpm = snowcrash_slab(slab)->bpm;
    p->pump = snowcrash_slab(slab)->pump;
    p->feedback = snowcrash_slab(slab)->feedback;
    p->wobble = snowcrash_slab(slab)->wobble;
    p->drum_pat = slab;
    p->root = snowcrash_slab_root(slab);
    p->scale = snowcrash_slab_scale(slab);
    p->swing = snowcrash_slab_swing(slab);
}

static inline uint8_t snowcrash_drm_density_allows(uint8_t density, uint8_t step, uint8_t voice_ix)
{
    return ((step * 19u + (voice_ix * 37u)) & 127u) < density;
}

static inline uint8_t snowcrash_gate_after_drm_density(uint8_t on, uint8_t density, uint8_t step, uint8_t voice_ix)
{
    if (!on)
        return 0u;
    return snowcrash_drm_density_allows(density, step, voice_ix);
}

static inline uint8_t dub_step_bit(uint32_t mask, uint8_t step)
{
    return (uint8_t)((mask >> (step & (DUB_STEPS - 1u))) & 1u);
}

static inline void dub_resolve_step(const dub_params_t *p, const dub_step_t *ovr, uint8_t step, dub_events_t *out, uint8_t apply_dens)
{
    const snowcrash_slab_t *drum;
    const snowcrash_slab_t *main;
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t forced;

    if (!p || !out)
        return;
    if (step >= DUB_STEPS)
        step = (uint8_t)(DUB_STEPS - 1u);
    drum = snowcrash_slab(p->drum_pat);
    main = snowcrash_slab(p->plate);
    forced = (ovr != 0) && ((ovr->force & 1u) != 0);
    if (forced) {
        kick = (ovr->gate >> 0) & 1u;
        rim = (ovr->gate >> 1) & 1u;
        hat = (ovr->gate >> 2) & 1u;
        siren = (ovr->gate >> 3) & 1u;
        sub = (ovr->gate >> 4) & 1u;
        chord = (ovr->gate >> 5) & 1u;
    } else {
        kick = dub_step_bit((uint16_t)drum->kick, step);
        rim = dub_step_bit((uint16_t)drum->rim, step);
        hat = dub_step_bit((uint16_t)drum->hat, step);
        siren = dub_step_bit((uint16_t)drum->siren, step);
        sub = dub_step_bit((uint16_t)main->sub, step);
        chord = dub_step_bit((uint16_t)main->chord, step);
    }
    if (apply_dens) {
        kick = snowcrash_gate_after_drm_density(kick, p->drm_dens[0], step, 0u);
        rim = snowcrash_gate_after_drm_density(rim, p->drm_dens[1], step, 1u);
        hat = snowcrash_gate_after_drm_density(hat, p->drm_dens[2], step, 2u);
        siren = snowcrash_gate_after_drm_density(siren, p->drm_dens[3], step, 3u);
    }
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
    const snowcrash_slab_t *sc;

    plate = snowcrash_clamp_slab(plate);
    sc = snowcrash_slab(plate);
    if (root)
        *root = snowcrash_slab_root(plate);
    if (scale)
        *scale = snowcrash_slab_scale(plate);
    if (swing)
        *swing = snowcrash_slab_swing(plate);
    if (bpm)
        *bpm = sc->bpm;
}

#include "snowcrash_gen.h"
'''
    (SRC / "_snowcrash_apply_snippet.h").write_text(snippet)


def patch_snowcrash_dsp_layout(out: str) -> str:
    era_aliases = """
/* IDM-shared DSP paths use semantic era / bass aliases */
#define ERA_OBLIQ ERA_CRASH
#define ERA_WARM ERA_GLOSS
#define ERA_GLITCH ERA_STATIC
#define ERA_MELO ERA_TIDE
#define ERA_DRILL ERA_STATIC
#define ERA_BREAK ERA_CHAPEL
#define BASS_ROUND BASS_SUB
#define BASS_THROB BASS_HUM
"""
    out = out.replace(
        '#include "snowcrash_data.h"\n#include "jazz_pluck.h"',
        '#include "snowcrash_data.h"\n#include "jazz_pluck.h"\n#include "snowcrash_pluck.h"\n' + era_aliases.strip() + "\n",
        1,
    )
    out = re.sub(
        r"/\* --- Bass: soft \(WARM–GLITCH\).*?\n\}\n\n\nstatic inline int32_t snowcrash_bass_house_sub",
        "static inline int32_t snowcrash_bass_house_sub",
        out,
        count=1,
        flags=re.S,
    )
    out = out.replace("snowcrash_piece_lead_gate_cs", "snowcrash_slab_lead_gate_cs")
    extra_compat = """
#define idm_env_body_gate snowcrash_env_body_gate
#define idm_env_body snowcrash_env_body
#define idm_lead_timbre snowcrash_lead_timbre
#define idm_kit_k_note snowcrash_kit_k_note
#define idm_kit_k_sweep snowcrash_kit_k_sweep
#define idm_kit_k_fm snowcrash_kit_k_fm
#define idm_kit_k_drv snowcrash_kit_k_drv
#define idm_kit_r_note snowcrash_kit_r_note
#define idm_kit_r_wire snowcrash_kit_r_wire
#define idm_hat_glitch_note snowcrash_hat_glitch_note
#define idm_hat_glitch_fm snowcrash_hat_glitch_fm
#define idm_hat_glitch_noise snowcrash_hat_glitch_noise
#define idm_hat_glitch_metal snowcrash_hat_glitch_metal
#define idm_hat_glitch_style snowcrash_hat_glitch_style
#define idm_hat_glitch_cut snowcrash_hat_glitch_cut
#define idm_drm_kick_shape snowcrash_drm_kick_shape
#define idm_drm_rim_mix snowcrash_drm_rim_mix
#define idm_drm_bus snowcrash_drm_bus
#define idm_drm_room_send snowcrash_drm_room_send
#define IDM_ERA_MACROS SNOWCRASH_ERA_MACROS
#define IDM_LEAD_LEN_N SNOWCRASH_LEAD_LEN_N
#define IDM_LEAD_LEN_16THS SNOWCRASH_LEAD_LEN_16THS
#define IDM_HAT_GLITCH_WASH SNOWCRASH_HAT_GLITCH_WASH
#define IDM_HAT_GLITCH_BELL SNOWCRASH_HAT_GLITCH_BELL
#define IDM_HAT_GLITCH_SWING SNOWCRASH_HAT_GLITCH_SWING
#define IDM_HAT_GLITCH_ACID_HAT SNOWCRASH_HAT_GLITCH_ACID_HAT
#define idm_lead_dly_boost snowcrash_lead_dly_boost
"""
    if "idm_env_body_gate snowcrash_env_body_gate" not in out:
        out = out.replace(
            "#if defined(GENRE_SNOWCRASH)\n#define idm_stutter_t",
            "#if defined(GENRE_SNOWCRASH)\n" + extra_compat.strip() + "\n#define idm_stutter_t",
            1,
        )
    return out


def patch_snowcrash_dsp_eras(out: str) -> str:
    """6-era lead/stutter/degrade + IDM compat macros (no BSUN/HOOL)."""
    sustain = """
static const snowcrash_lead_sustain_t SNOWCRASH_LEAD_SUSTAIN[SNOWCRASH_ERA_N] = {
    { 58, 4, 10, 28 }, /* SNOWCRASH  — short nervous */
    { 66, 5, 11, 32 }, /* GLOSSCRASH — medium vocal */
    { 56, 4, 11, 24 }, /* SUMCRASH   — long mute-trp */
    { 36, 2,  8, 12 }, /* ICECRASH   — brittle + glare */
    { 54, 4, 10, 26 }, /* TIDECRASH  — liquid clav */
    { 70, 6, 12, 36 }, /* WARPCRASH  — chapel tail */
};
"""
    out = re.sub(
        r"static const snowcrash_lead_sustain_t SNOWCRASH_LEAD_SUSTAIN\[SNOWCRASH_ERA_N\] = \{.*?\};",
        sustain.strip(),
        out,
        count=1,
        flags=re.S,
    )
    timbre = """    static const uint8_t timbre[SNOWCRASH_ERA_N] = {
        SNOWCRASH_LEAD_CLAV, SNOWCRASH_LEAD_EP, SNOWCRASH_LEAD_MUTE_TRP,
        SNOWCRASH_LEAD_HORN, SNOWCRASH_LEAD_FLUTE, SNOWCRASH_LEAD_SAX
    };"""
    out = re.sub(
        r"static const uint8_t timbre\[SNOWCRASH_ERA_N\] = \{.*?\};",
        timbre.strip(),
        out,
        count=1,
        flags=re.S,
    )
    degrade_base = """    static const uint8_t base[SNOWCRASH_ERA_N] = { 8, 10, 18, 22, 16, 12 };"""
    out = re.sub(
        r"static const uint8_t base\[SNOWCRASH_ERA_N\] = \{.*?\};",
        degrade_base.strip(),
        out,
        count=1,
        flags=re.S,
    )
    stutter_fn = """
static inline uint16_t snowcrash_stutter_slice_from_vel(uint8_t vel, uint8_t era)
{
    uint16_t base = (uint16_t)(28u + ((uint32_t)vel * 88u) / 127u);
    uint8_t e = snowcrash_clamp_era(era);

    if (e == (uint8_t)ERA_CRASH)
        base = (uint16_t)((base * 5u) / 4u);
    else if (e == (uint8_t)ERA_STATIC)
        base = (uint16_t)((base * 5u) / 8u);
    else if (e == (uint8_t)ERA_CHAPEL)
        base = (uint16_t)((base * 5u) / 6u);
    else if (e == (uint8_t)ERA_TIDE)
        base = (uint16_t)((base * 3u) / 4u);
    if (base < 6u)
        base = 6u;
    if (base > 512u)
        base = 512u;
    return base;
}
"""
    out = re.sub(
        r"static inline uint16_t snowcrash_stutter_slice_from_vel\(uint8_t vel, uint8_t era\)\n\{.*?\n\}\n",
        stutter_fn.strip() + "\n",
        out,
        count=1,
        flags=re.S,
    )
    lead_dly = """
static inline int32_t snowcrash_lead_dly_boost(const dub_params_t *p)
{
    (void)p;
    return 0;
}
"""
    if "snowcrash_lead_dly_boost" not in out:
        out = out.replace(
            "#if defined(GENRE_SNOWCRASH)\n#define idm_env_body_gate",
            lead_dly.strip() + "\n\n#if defined(GENRE_SNOWCRASH)\n#define idm_env_body_gate",
            1,
        )
    return out


def gen_dsp_h() -> None:
    idm = (SRC / "idm_dsp.h").read_text()
    out = idm
    repl = [
        ("IDM_DSP_H", "SNOWCRASH_DSP_H"),
        ("idm_data.h", "snowcrash_data.h"),
        ("idm_", "snowcrash_"),
        ("IDM_", "SNOWCRASH_"),
        ("idm hat_glitch", "snowcrash hat"),
    ]
    for a, b in repl:
        out = out.replace(a, b)
    # fix double snowcrash_snowcrash
    out = out.replace("snowcrash_snowcrash_", "snowcrash_")
    out = out.replace("SNOWCRASHSNOWCRASH_", "SNOWCRASH_")

    bass_block = r'''
static inline int32_t snowcrash_bass_house_sub(dub_synth_t *s, uint32_t inc, int32_t fc, uint8_t reso)
{
    int32_t raw;

    s->sub_ph += inc;
    raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph >> 1) >> 2);
    dub_svf_mode(raw, 600 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, reso, &raw);
    return raw;
}

static inline int32_t snowcrash_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                       int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t raw = 0;
    int32_t walk = lfo_mod_b;

    switch (model) {
    case BASS_HUM: {
        uint32_t hum = inc / 50u;
        if (hum < 80u)
            hum = 80u;
        s->sub_ph += hum;
        s->sub_ph2 += inc >> 2;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph2) >> 1);
        dub_svf_mode(raw, fc >> 1, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        raw += dub_sine_q64(s->sub_ph + (wob_all << 10)) >> 3;
        break;
    }
    case BASS_CLICK: {
        int32_t click;
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph);
        click = s->sub_env > 28000 ? (dub_sine_q64(s->sub_ph << 3) >> 4) : 0;
        raw = raw + click + (dub_sine_q64(s->sub_ph >> 1) >> 3);
        dub_svf_mode(raw, 400 + (fc >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_GLOSS: {
        int32_t fund, form;
        s->sub_ph += inc;
        s->sub_ph2 += inc + (inc >> 3);
        fund = dub_sine_q64(s->sub_ph);
        form = dub_sine_q64(s->sub_ph2 + (uint32_t)(p->wobble << 14));
        raw = fund + ((form * (24 + (int32_t)p->cutoff_b)) >> 7);
        dub_svf_mode(raw, fc + (walk >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_TIDE: {
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) >> 1;
        raw += dub_sine_q64(s->sub_ph + (uint32_t)(p->wobble << 13)) >> 2;
        dub_svf_mode(raw, fc + ((int32_t)p->wobble * 8), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_808: {
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph << 1) >> 2);
        dub_svf_mode(raw, 1200 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_303: {
        int32_t saw_w, acid_fc;
        s->sub_ph += inc;
        saw_w = dub_saw(s->sub_ph);
        raw = saw_w + (dub_square(s->sub_ph) >> 1);
        acid_fc = fc + (((s->sub_env >> 3) * (int32_t)p->wobble) >> 5);
        dub_svf_mode(raw, acid_fc, &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode + 24u > 127u ? 127u : p->reso_mode + 24u), &raw);
        break;
    }
    case BASS_MOOG: {
        int32_t saw_w;
        s->sub_ph += inc;
        saw_w = dub_saw(s->sub_ph);
        dub_svf_mode(saw_w, fc, &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 20u), &raw);
        dub_svf_mode(raw, fc, &s->sub_svf_b1, &s->sub_svf_b2, (uint8_t)(p->reso_mode >> 1), &raw);
        break;
    }
    case BASS_REESE: {
        uint32_t det = (inc >> 5) + (uint32_t)(wob_all >> 11);
        int32_t s1, s2, s3;
        s->sub_ph += inc;
        s->sub_ph2 += inc + det;
        s->sub_ph3 += inc - det;
        s1 = dub_saw(s->sub_ph);
        s2 = dub_saw(s->sub_ph2);
        s3 = dub_saw(s->sub_ph3);
        raw = ((s1 + s2 + s3) / 3);
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_FM: {
        int32_t mod2;
        s->sub_ph += inc;
        s->sub_ph2 += inc << 1;
        mod2 = dub_sine_q64(s->sub_ph2);
        raw = dub_sine_q64(s->sub_ph + (uint32_t)(mod2 << 13));
        break;
    }
    case BASS_PWM: {
        int32_t pulse;
        s->sub_ph += inc;
        pulse = (s->sub_ph < 0x28000000u) ? 16000 : -16000;
        raw = pulse + (dub_sine_q64(s->sub_ph >> 1) >> 2);
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_PLUCK: {
        s->sub_ph += inc;
        raw = dub_saw(s->sub_ph);
        dub_svf_mode(raw, fc + (walk >> 1), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        raw = (raw * (int32_t)s->sub_env) >> 15;
        break;
    }
    default:
        return snowcrash_bass_house_sub(s, inc, fc, p->reso_mode);
    }
    (void)p;
    return raw;
}

'''
    # Replace old bass render in generated file
    out = re.sub(
        r"/\* --- Bass:.*?\*/\nstatic inline int32_t snowcrash_bass_render\(.*?\n\}\n",
        "",
        out,
        count=1,
        flags=re.S,
    )
    insert_at = out.find("/* --- Lane-3 lead")
    if insert_at < 0:
        insert_at = out.find("/* --- Drum kit")
    out = out[:insert_at] + bass_block + "\n" + out[insert_at:]

    # idm compat macros for fm_app when building snowcrash
    compat = '''
#if defined(GENRE_SNOWCRASH)
#define idm_stutter_t snowcrash_stutter_t
#define idm_piece_lead_step_bit snowcrash_slab_lead_step_bit
#define idm_piece_lead_pitch snowcrash_slab_lead_pitch
#define idm_piece_lead_len_ix snowcrash_slab_lead_len_ix
#define idm_piece_lead_bend_u7 snowcrash_slab_lead_bend_u7
#define idm_lead_bend_plock snowcrash_lead_bend_plock
#define idm_lead_bend_semi snowcrash_lead_bend_semi
#define idm_lead_len_ix snowcrash_lead_len_ix
#define idm_fill_morph_steps snowcrash_fill_morph_steps
#define idm_stutter_edit_live snowcrash_stutter_edit_live
#define idm_u8_blend snowcrash_u8_blend
#define idm_fill_build_amt snowcrash_fill_build_amt
#define idm_stutter_edit_targets snowcrash_stutter_edit_targets
#define idm_fill_drop_targets snowcrash_fill_drop_targets
#define idm_apply_fill_break_voice snowcrash_apply_fill_break_voice
#define idm_stutter_edit_restore snowcrash_stutter_edit_restore
#define idm_stutter_start snowcrash_stutter_start
#define idm_stutter_release snowcrash_stutter_release
#define idm_stutter_bar snowcrash_stutter_bar
#define idm_edit_latch_begin snowcrash_edit_latch_begin
#define idm_stutter_latch_block snowcrash_stutter_latch_block
#define idm_rev_stutter_flip snowcrash_rev_stutter_flip
#define idm_glitch_burst_hit snowcrash_glitch_burst_hit
#define idm_stutter_sync_runtime snowcrash_stutter_sync_runtime
#define idm_edit_apply_drums snowcrash_edit_apply_drums
#define idm_stutter_process_buffer snowcrash_stutter_process_buffer
#define idm_loop_macro_sync snowcrash_loop_macro_sync
#define idm_lead_note_gate_piece snowcrash_lead_note_gate_piece
#define idm_siren_atk_samples snowcrash_siren_atk_samples
#define idm_siren_env_step snowcrash_siren_env_step
#define idm_lead_render snowcrash_lead_render
#define idm_lead_bent_inc snowcrash_lead_bent_inc
#define idm_lead_len_samples snowcrash_lead_len_samples
#define idm_stutter_push snowcrash_stutter_push
#define idm_stutter_mix snowcrash_stutter_mix
#define idm_stutter_slice_from_vel snowcrash_stutter_slice_from_vel
#define idm_degrade_amount snowcrash_degrade_amount
#define idm_degrade_sample snowcrash_degrade_sample
#define idm_hat_glitch_is_acid_kit snowcrash_hat_glitch_is_acid_kit
#define idm_hat_glitch_decay snowcrash_hat_glitch_decay
#define idm_hat_glitch_open snowcrash_hat_glitch_open
#define IDM_PIECE SNOWCRASH_SLAB
#define IDM_PIECE_KIT SNOWCRASH_SLAB_KIT
#define idm_era_first_piece snowcrash_era_first_slab
#define IDM_LEAD_LEN_NAMES SNOWCRASH_LEAD_LEN_NAMES
#define IDM_STUT_HACK SNOWCRASH_STUT_HACK
#define IDM_STUT_SCREW SNOWCRASH_STUT_SCREW
/* IDM_FILL_MORPH_STEPS / IDM_STUTTER_MORPH_STEPS: defined in fm_app.c (no alias here) */
#endif
'''
    out = out.replace("#endif\n", compat + "\n#endif\n", 1)
    out = patch_snowcrash_dsp_layout(out)
    out = patch_snowcrash_dsp_eras(out)
    (SRC / "snowcrash_dsp.h").write_text(out)


def patch_file_genre(path: Path) -> None:
    text = path.read_text()
    if "GENRE_SNOWCRASH" in text:
        return
    text = text.replace(
        "!defined(GENRE_IDM)",
        "!defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH)",
    )
    text = text.replace(
        "defined(GENRE_IDM)",
        "defined(GENRE_IDM) || defined(GENRE_SNOWCRASH)",
    )
    # fix double additions
    text = text.replace(
        "|| defined(GENRE_SNOWCRASH) || defined(GENRE_SNOWCRASH)",
        "|| defined(GENRE_SNOWCRASH)",
    )
    text = text.replace(
        "&& !defined(GENRE_SNOWCRASH) && !defined(GENRE_SNOWCRASH)",
        "&& !defined(GENRE_SNOWCRASH)",
    )
    path.write_text(text)


def patch_genre_conf_pack() -> None:
    conf = SRC / "genre_conf.h"
    t = conf.read_text()
    if "GENRE_SNOWCRASH" not in t:
        t = t.replace(
            "#elif defined(GENRE_IDM)\n#include \"idm_conf.h\"",
            "#elif defined(GENRE_IDM)\n#include \"idm_conf.h\"\n#elif defined(GENRE_SNOWCRASH)\n#include \"snowcrash_conf.h\"",
        )
        conf.write_text(t)
    pack = SRC / "genre_pack.h"
    t = pack.read_text()
    if "snowcrash" not in t:
        block = r'''#elif defined(GENRE_SNOWCRASH)
#include "snowcrash_data.h"
'''
        t = t.replace('#elif defined(GENRE_IDM)\n#include "idm_data.h"', '#elif defined(GENRE_IDM)\n#include "idm_data.h"\n' + block)
        macros = r'''#elif defined(GENRE_SNOWCRASH)
#define genre_clamp_era(era)       snowcrash_clamp_era(era)
#define genre_clamp_preset(ix)     snowcrash_clamp_slab(ix)
#define genre_era_name(era)        snowcrash_era_name(era)
#define genre_preset_name(ix)      snowcrash_slab_name(ix)
#define genre_preset(ix)           snowcrash_slab(ix)
#define genre_era_of_preset(pl)    snowcrash_era_of_slab(pl)
#define genre_era_first_preset(er) snowcrash_era_first_slab(er)
#define genre_preset_rebuild(p, era, pl) snowcrash_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   snowcrash_slab_apply(p, pl, what)
#define genre_era_apply_base(p, era)     snowcrash_era_apply_base(p, era)
#define genre_apply_defaults(p)          snowcrash_apply_defaults(p)
'''
        t = t.replace("#elif defined(GENRE_IDM)\n#define genre_clamp_era", "#elif defined(GENRE_IDM)\n#define genre_clamp_era")
        t = t.replace(
            "#define genre_apply_defaults(p)          idm_apply_defaults(p)\n#else",
            "#define genre_apply_defaults(p)          idm_apply_defaults(p)\n" + macros + "#else",
        )
        pack.write_text(t)


def patch_dub_dsp_include() -> None:
    path = SRC / "dub_dsp.h"
    t = path.read_text()
    if "snowcrash_dsp" not in t:
        t = t.replace(
            "#elif defined(GENRE_IDM)\n#include \"idm_dsp.h\"",
            "#elif defined(GENRE_SNOWCRASH)\n#include \"snowcrash_dsp.h\"\n#elif defined(GENRE_IDM)\n#include \"idm_dsp.h\"",
        )
        t = t.replace(
            "raw = idm_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);",
            "raw = idm_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);\n#elif defined(GENRE_SNOWCRASH)\n                raw = snowcrash_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);",
        )
        # chord table branch
        t = t.replace(
            "#elif defined(GENRE_IDM)\n    static const int8_t tab[VOICE_N][4]",
            "#elif defined(GENRE_SNOWCRASH) || defined(GENRE_IDM)\n    static const int8_t tab[VOICE_N][4]",
        )
        path.write_text(t)


def patch_dub_dsp_pluck() -> None:
    """Rim and hat of KO XCRASH: Karplus-Strong loops instead of the IDM shell/wire and hat-glitch sums."""
    path = SRC / "dub_dsp.h"
    t = path.read_text()
    if "snowcrash_ks_step" in t:
        path.write_text(patch_dub_dsp_ks_casts(t))
        return
    pairs = [
        ("void dub_trig_rim(dub_synth_t *s, uint8_t vel, int8_t tune, uint8_t atk)\n{",
         "#if defined(GENRE_SNOWCRASH)\nstatic inline void snowcrash_ks_excite(uint8_t voice);\nstatic inline int32_t snowcrash_ks_step(uint8_t voice);\n#endif\nvoid dub_trig_rim(dub_synth_t *s, uint8_t vel, int8_t tune, uint8_t atk)\n{"),
        ("    s->rim_st_tune = tune;\n    s->rim_ph = 0u;\n",
         "    s->rim_st_tune = tune;\n    s->rim_ph = 0u;\n#if defined(GENRE_SNOWCRASH)\n    snowcrash_ks_excite(0u);\n#endif\n"),
        ("        s->hat_ph[i] = (uint32_t)(i * 0x11111111u);\n",
         "        s->hat_ph[i] = (uint32_t)(i * 0x11111111u);\n#if defined(GENRE_SNOWCRASH)\n    snowcrash_ks_excite(1u);\n#endif\n"),
        ("#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)\n                mix_r = idm_drm_rim_mix(shell, wire, kit_r_wire[kit], rim_drv);",
         "#elif defined(GENRE_SNOWCRASH)\n                mix_r = snowcrash_ks_step(0u) >> 1;\n#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)\n                mix_r = idm_drm_rim_mix(shell, wire, kit_r_wire[kit], rim_drv);"),
        ("#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)\n                {\n                    uint32_t r_inc = dub_note_inc(dub_clamp_midi(hat_base));",
         "#elif defined(GENRE_SNOWCRASH)\n                {\n                    hm = snowcrash_ks_step(1u) >> 1;\n                    (void)h;\n                }\n#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP) || defined(GENRE_TECHNO) || defined(GENRE_BASS)\n                {\n                    uint32_t r_inc = dub_note_inc(dub_clamp_midi(hat_base));"),
    ]
    for old, new in pairs:
        assert t.count(old) == 1, ("dub_dsp anchor count", t.count(old), old[:60])
        t = t.replace(old, new)
    path.write_text(patch_dub_dsp_ks_casts(t))


# Snowcrash branches of rim/hat no longer read the IDM/Jazz locals; keep their side effects (noise draw) and
# silence the unused-variable warnings with (void) casts. Idempotent: safe on fresh and already patched text.
KS_UNUSED_CASTS = [
    ("                mix_r = snowcrash_ks_step(0u) >> 1;\n",
     "                mix_r = snowcrash_ks_step(0u) >> 1;\n"
     "                (void)shell;\n                (void)wire;\n                (void)rim_drv;\n"),
    ("                    hm = snowcrash_ks_step(1u) >> 1;\n                    (void)h;\n",
     "                    hm = snowcrash_ks_step(1u) >> 1;\n                    (void)h;\n                    (void)hat_base;\n"),
]


def patch_dub_dsp_ks_casts(t: str) -> str:
    for old, new in KS_UNUSED_CASTS:
        if new in t:
            continue
        assert t.count(old) == 1, ("dub_dsp ks cast anchor count", t.count(old), old[:60])
        t = t.replace(old, new)
    return t


def patch_fm_app_snowcrash_ui() -> None:
    path = SRC / "fm_app.c"
    t = path.read_text()
    if '#include "snowcrash_draw.h"' in t:
        return  # already patched (idempotent)
    # DUB_CFG magic prev
    t = t.replace(
        "#elif defined(GENRE_IDM)\n#define DUB_CFG_MAGIC_PREV GENRE_CFG_MAGIC",
        "#elif defined(GENRE_IDM)\n#define DUB_CFG_MAGIC_PREV GENRE_CFG_MAGIC\n#elif defined(GENRE_SNOWCRASH)\n#define DUB_CFG_MAGIC_PREV GENRE_CFG_MAGIC",
    )
    # exclude snowcrash from dub default at top
    t = t.replace(
        "#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM)\n#define C_ACC",
        "#if !defined(GENRE_HOUSE) && !defined(GENRE_HEAVY_PSYCH) && !defined(GENRE_JAZZ) && !defined(GENRE_IDM) && !defined(GENRE_SNOWCRASH)\n#define C_ACC",
    )
    # plate seq - duplicate IDM block for 64 entries
    m = re.search(r"#elif defined\(GENRE_IDM\)\nstatic const dub_plate_seq_t IDM_PLATE_SEQ\[GENRE_PRESET_N\] = \{.*?\n\};\n#endif", t, re.S)
    if m:
        idm_block = m.group(0)
        sc_block = idm_block.replace("GENRE_IDM", "GENRE_SNOWCRASH").replace("IDM_PLATE_SEQ", "SNOWCRASH_PLATE_SEQ")
        # extend to 64 lines - duplicate last 16 entries
        inner = re.search(r"\{.*", sc_block, re.S).group(0)
        entries = re.findall(r"\{ \{.*?\}, \d+, \d+, \d+, \d+, \d+ \},", inner, re.S)
        while len(entries) < 64:
            entries.append(entries[len(entries) % 48])
        new_inner = "static const dub_plate_seq_t SNOWCRASH_PLATE_SEQ[GENRE_PRESET_N] = {\n    " + "\n    ".join(entries) + "\n};\n#endif"
        t = t.replace(idm_block, idm_block + "\n#elif defined(GENRE_SNOWCRASH)\n" + new_inner.replace("#endif", ""))
    t = t.replace(
        "#elif defined(GENRE_IDM)\n    return &IDM_PLATE_SEQ[plate];",
        "#elif defined(GENRE_IDM)\n    return &IDM_PLATE_SEQ[plate];\n#elif defined(GENRE_SNOWCRASH)\n    return &SNOWCRASH_PLATE_SEQ[plate];",
    )
    # draw include
    t = t.replace(
        '#include "idm_draw.h"',
        '#elif defined(GENRE_SNOWCRASH)\n#include "snowcrash_draw.h"\n#elif defined(GENRE_IDM)\n#include "idm_draw.h"',
    )
    # fix double elif if already patched
    t = t.replace("#elif defined(GENRE_IDM)\n#elif defined(GENRE_SNOWCRASH)", "#elif defined(GENRE_SNOWCRASH)")
    # LFO macro pick 8 eras for snowcrash
    t = t.replace(
        "static const uint8_t IDM_MACRO_LFO_PICK[GENRE_ERA_N][7]",
        "static const uint8_t IDM_MACRO_LFO_PICK[GENRE_ERA_N][7]",
    )
    path.write_text(t)


def main() -> None:
    write_apply_snippet()
    (SRC / "snowcrash_data.h").write_text(gen_data_h())
    gen_dsp_h()
    # fm_views_data.h is owned by tools/gen_views.py (canonical views, checked by plate_gen_drift)
    subprocess.run([sys.executable, str(Path(__file__).resolve().parent / "gen_views.py")], check=True, stdout=subprocess.DEVNULL)
    for f in ["dub_dsp.h", "dub_types.h", "fm_app.c", "fm_seq.h"]:
        patch_file_genre(SRC / f)
    patch_genre_conf_pack()
    patch_dub_dsp_include()
    patch_fm_app_snowcrash_ui()
    patch_dub_dsp_pluck()
    # draw header from idm
    draw = (SRC / "idm_draw.h").read_text()
    draw = draw.replace("IDM_DRAW_H", "SNOWCRASH_DRAW_H").replace("idm_", "snowcrash_").replace("IDM", "SNOWCRASH")
    draw = draw.replace("GENRE_IDM", "GENRE_SNOWCRASH")
    draw = draw.replace("snowcrash_piece_t", "snowcrash_slab_t")
    draw = draw.replace("snowcrash_piece(", "snowcrash_slab(")
    draw = draw.replace('"LEAD"', '"VOICE"', 1)
    (SRC / "snowcrash_draw.h").write_text(draw)
    (SRC / "_snowcrash_apply_snippet.h").unlink(missing_ok=True)
    print("snowcrash pack generated")


if __name__ == "__main__":
    main()
