#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate techno_data.h, fm_views techno section, techno_dsp.h (from idm template)."""
from __future__ import annotations

import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "firmware" / "src"

ERA_N = 6
ERA_NAMES = ["DETROIT", "BELGIUM", "SCHRANZ", "HYPNO", "BERLIN", "EBM"]
ERA_FULL = ERA_NAMES

LOOP_NAMES = [
    ["MODEL", "ALPHA", "PULSE", "SYNC", "MINOR", "SUS4", "HAT16", "DEEP", "FM7", "ROLL"],
    ["MENTAS", "HOOVR", "RAVE1", "OFFBT", "STAB", "SIREN", "BEAT", "HARD", "FIFTH", "GATE"],
    ["CLIPS", "METAL", "DRIVE", "KICK2", "TOP16", "MINML", "CRUSH", "IRON", "STOMP", "FLAT"],
    ["PHASE", "POLY3", "POLY5", "POLY7", "BLEEP", "ROT8", "SUBPL", "BAND", "ECHO", "DRIFT"],
    ["RUMBL", "TANK", "DUB5", "BIGRM", "HATDR", "SUBDK", "WIDE", "STEEL", "PUMP", "CLUB"],
    ["OCT16", "BODY", "COLD", "FM5TH", "SNARE", "CLAP", "SEQ", "INDUS", "TRIG", "DARK"],
]

BASS_NAMES = [
    "RUMBLE", "SUB909", "EBM16", "HOOVER", "ACID303", "DETROIT", "SCHRANZ", "REESE", "MODFM", "DRONE",
]
BASS_ERA = [4, 0, 5, 1, 0, 0, 2, 1, 3, 2]
BASS_N = len(BASS_NAMES)

DKIT_NAMES = ["909", "DETROIT", "RAVE", "SCHRANZ", "HYPNO", "BERLIN", "EBM", "INDUS"]
DKIT_ERA = [0, 0, 1, 2, 3, 4, 5, 5]

VOICE_NAMES = [
    "MIN7", "FIFTH", "OCT", "RAVE5", "SUS4", "DIM",
    "MINOR", "CLUSTER", "DUB5", "DET9", "COLD4", "DRONE1",
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

ERA_BPM_LO = [124, 130, 148, 128, 130, 122]
ERA_BPM_HI = [132, 138, 158, 136, 138, 132]
ERA_DENS_BASE = [40, 42, 28, 36, 38, 34]
ERA_SWING_BASE = [66, 62, 58, 60, 58, 60]
ERA_PUMP_BASE = [62, 58, 78, 54, 82, 64]
ERA_DLY_C_BASE = [48, 52, 38, 56, 62, 44]
ERA_REV_C_BASE = [58, 54, 42, 50, 72, 46]

# Global techno mix headroom (~7–8 dB on 0–127 faders); applied in era bases + bus lvls.
MIX_HEADROOM = 16
MIX_HEADROOM_LEAD = 18
MIX_HEADROOM_DRV = 8

ERA_BASE_RAW = [
    (34, 90, 92, 20, 44, 84, 94, 28, 14, 26, 74, 54, 52, 64, 66, 16, 100, 42, 36, (64, 68, 52, 76), 14, 52, 28, 68),
    (40, 78, 86, 32, 56, 72, 88, 22, 24, 18, 82, 58, 56, 64, 54, 22, 96, 48, 30, (64, 74, 48, 82), 10, 44, 36, 66),
    (44, 74, 70, 44, 68, 70, 82, 36, 28, 14, 84, 62, 68, 64, 50, 28, 104, 62, 24, (64, 78, 46, 88), 18, 40, 42, 64),
    (36, 92, 88, 28, 46, 78, 86, 26, 22, 26, 70, 56, 64, 64, 58, 18, 102, 36, 32, (62, 72, 50, 80), 12, 48, 32, 60),
    (32, 86, 94, 18, 40, 84, 96, 32, 12, 34, 68, 50, 48, 64, 72, 12, 92, 38, 40, (58, 68, 46, 76), 20, 58, 24, 70),
    (46, 76, 72, 36, 60, 68, 80, 18, 26, 16, 86, 64, 72, 64, 48, 24, 106, 72, 22, (64, 82, 44, 90), 22, 38, 48, 72),
]


def headroom_u8(v: int, trim: int, floor: int = 10) -> int:
    return max(floor, v - trim)


def era_base_trimmed(row: tuple) -> tuple:
    pans = row[19]
    return (
        row[0],
        row[1],
        row[2],
        row[3],
        row[4],
        row[5],
        row[6],
        row[7],
        headroom_u8(row[8], MIX_HEADROOM_DRV, 6),
        row[9],
        headroom_u8(row[10], MIX_HEADROOM),
        headroom_u8(row[11], MIX_HEADROOM),
        headroom_u8(row[12], MIX_HEADROOM),
        row[13],
        row[14],
        headroom_u8(row[15], MIX_HEADROOM_DRV, 8),
        headroom_u8(row[16], MIX_HEADROOM_LEAD),
        row[17],
        row[18],
        pans,
        row[20],
        row[21],
        row[22],
        row[23],
    )


def fmt_era_base_row(row: tuple) -> str:
    pans = ", ".join(str(p) for p in row[19])
    return (
        f"{{ {row[0]}, {row[1]}, {row[2]}, {row[3]}, {row[4]}, {row[5]}, {row[6]}, {row[7]}, "
        f"{row[8]}, {row[9]}, {row[10]}, {row[11]}, {row[12]}, {row[13]}, {row[14]}, {row[15]}, "
        f"{row[16]}, {row[17]}, {row[18]}, {{{pans}}}, {row[20]}, {row[21]}, {row[22]}, {row[23]} }}"
    )


def mask16(seed: int, era: int, ix: int) -> int:
    h = int(hashlib.md5(f"{seed}-{era}-{ix}".encode()).hexdigest()[:4], 16)
    return h & 0xFFFF


def mask32(seed: int, era: int, ix: int) -> int:
    lo = mask16(seed, era, ix)
    hi = mask16(seed + 1, era, ix)
    return lo | (hi << 16)


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
    k = mask16(1, era, ix)
    r = mask16(2, era, ix)
    h = mask16(3, era, ix)
    sub = mask16(4, era, ix)
    ch = mask16(5, era, ix)
    sr = mask32(6, era, ix)
    return (
        f'{{ "{name}", {era}, {bass}, {voice}, {tap}, {tank}, {bpm}, {dens}, '
        f'{dly_c}, {rev_c}, {fb}, {lf}, {wob}, {pump}, '
        f'0x{k:04X}u | ((uint32_t)0x{mask16(7, era, ix):04X}u << 16), '
        f'0x{r:04X}u | ((uint32_t)0x{mask16(8, era, ix):04X}u << 16), '
        f'0x{h:04X}u | ((uint32_t)0x{mask16(9, era, ix):04X}u << 16), '
        f'0x{sub:04X}u | ((uint32_t)0x{mask16(10, era, ix):04X}u << 16), '
        f'0x{ch:04X}u | ((uint32_t)0x{mask16(11, era, ix):04X}u << 16), '
        f'0x{sr & 0xFFFF:04X}u | ((uint32_t)0x{(sr >> 16) & 0xFFFF:04X}u << 16) }}'
    )


def gen_data_h() -> str:
    lines = [
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "#ifndef TECHNO_DATA_H",
        "#define TECHNO_DATA_H",
        "",
        '#include "dub_types.h"',
        '#include "techno_conf.h"',
        '#include "fm_seq.h"',
        "",
        "enum { TECHNO_ERA_N = GENRE_ERA_N, TECHNO_LOOP_N = GENRE_PRESET_N };",
    ]
    era_enum = ", ".join(f"ERA_{n}" for n in ERA_NAMES)
    lines.append(f"enum {{ {era_enum} }};")
    lines.append(
        "enum {\n"
        "    BASS_RUMBLE = 0, BASS_SUB909, BASS_EBM16, BASS_HOOVER, BASS_ACID303, BASS_DETROIT,\n"
        "    BASS_SCHRANZ, BASS_REESE, BASS_MODFM, BASS_DRONE, BASS_N\n"
        "};"
    )
    lines.append(
        "enum {\n"
        "    DKIT_909 = 0, DKIT_DETROIT, DKIT_RAVE, DKIT_SCHRANZ, DKIT_HYPNO, DKIT_BERLIN, DKIT_EBM, DKIT_INDUS, DKIT_N\n"
        "};"
    )
    lines.append(
        "enum {\n"
        "    VOICE_MIN7 = 0, VOICE_FIFTH, VOICE_OCT, VOICE_RAVE5, VOICE_SUS4, VOICE_DIM,\n"
        "    VOICE_MINOR, VOICE_CLUSTER, VOICE_DUB5, VOICE_DETROIT9, VOICE_COLD4, VOICE_DRONE1,\n"
        "    VOICE_N\n"
        "};"
    )
    lines.append("")
    lines.append("typedef dub_plate_t techno_loop_t;")
    lines.append("")
    lines.append(f'static const char *const TECHNO_ERA_NAMES[TECHNO_ERA_N] = {{')
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
        (HAT_NOTE, "techno_hat_note", HAT_NOTE),
        (HAT_FM, "techno_hat_fm", HAT_FM),
        (HAT_NOISE, "techno_hat_noise", HAT_NOISE),
        (HAT_METAL, "techno_hat_metal", HAT_METAL),
        (HAT_DECAY, "techno_hat_decay", HAT_DECAY),
        (HAT_OPEN, "techno_hat_open", HAT_OPEN),
        (HAT_STYLE, "techno_hat_style", HAT_STYLE),
        (HAT_CUT, "techno_hat_cut", HAT_CUT),
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
    lines.append("static const techno_loop_t TECHNO_LOOP[TECHNO_LOOP_N] = {")
    for era in range(ERA_N):
        for ix in range(PRESETS_PER_ERA):
            nm = LOOP_NAMES[era][ix]
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
    lines.append(f"static const int8_t TECHNO_LOOP_ROOT[TECHNO_LOOP_N] = {{")
    lines.append(", ".join(roots))
    lines.append("};")
    lines.append(f"static const int8_t TECHNO_LOOP_SCALE[TECHNO_LOOP_N] = {{")
    lines.append(", ".join(scales))
    lines.append("};")
    lines.append(f"static const int8_t TECHNO_LOOP_SWING[TECHNO_LOOP_N] = {{")
    lines.append(", ".join(swings))
    lines.append("};")

    lead_rate = [str((pi % PRESETS_PER_ERA) - 3) for pi in range(PRESET_N)]
    lines.append(f"static const int8_t TECHNO_LOOP_LEAD_RATE[TECHNO_LOOP_N] = {{")
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
    lines.append("static const uint8_t TECHNO_ERA_MACROS[TECHNO_ERA_N][10] = {")
    for i, m in enumerate(macros):
        lines.append(f"    {{ {', '.join(str(x) for x in m)} }}, /* {ERA_NAMES[i]} */")
    lines.append("};")
    lines.append("")

    # era base/fx - compact templates
    era_base_lines = "\n".join(
        "    " + fmt_era_base_row(era_base_trimmed(r)) + "," for r in ERA_BASE_RAW
    )
    lines.append(f"""typedef struct {{
    uint8_t timbre, cutoff_c, decay_c, reso_mode, env_filt;
    uint8_t cutoff_b, decay_b, sub_oct, drive, dly_b;
    uint8_t kick_boom, rim_tone, hat_lvl, kit_tune, kit_decay, kit_drive;
    uint8_t siren_pitch, siren_rate, rim_dly;
    uint8_t drm_pan[4];
    uint8_t wow, dly_time, isolator, swing;
}} techno_era_base_t;

typedef struct {{
    uint8_t drum_kit, tank, tap, loop_filt, feedback;
    uint8_t dly_lvl, rev_mix, rev_damp, rev_tail, dly_rev;
    uint8_t pump, bass_model, wobble;
}} techno_era_fx_t;

static const techno_era_base_t TECHNO_ERA_BASE[TECHNO_ERA_N] = {{
{era_base_lines}
}};

static const techno_era_fx_t TECHNO_ERA_FX[TECHNO_ERA_N] = {{
    {{ DKIT_DETROIT, 28, 1, 42, 18, 58, 34, 32, 54, 52,  62, BASS_DETROIT,  14 }},
    {{ DKIT_RAVE,    24, 0, 46, 16, 64, 30, 28, 50, 48,  58, BASS_HOOVER,   22 }},
    {{ DKIT_SCHRANZ, 16, 0, 50, 20, 52, 28, 22, 44, 72,  82, BASS_SCHRANZ,   8 }},
    {{ DKIT_HYPNO,   20, 0, 44, 14, 56, 36, 30, 52, 46,  54, BASS_MODFM,    18 }},
    {{ DKIT_BERLIN,  32, 2, 40, 14, 72, 42, 40, 62, 84,  88, BASS_RUMBLE,   10 }},
    {{ DKIT_EBM,     18, 0, 48, 18, 54, 32, 24, 46, 58,  66, BASS_EBM16,    26 }},
}};
""")

    # lead gates - simplified 32-bit patterns
    gates = []
    for pi in range(PRESET_N):
        g = mask32(100, pi // PRESETS_PER_ERA, pi % PRESETS_PER_ERA)
        gates.append(f"0x{g:08X}u")
    lines.append("static const uint32_t TECHNO_LOOP_LEAD_GATE[TECHNO_LOOP_N] = {")
    for i in range(0, PRESET_N, PRESETS_PER_ERA):
        lines.append("    " + ", ".join(gates[i : i + PRESETS_PER_ERA]) + ",")
    lines.append("};")

    kit_map = []
    for pi in range(PRESET_N):
        kit_map.append(
            str(DKIT_ERA[pi % len(DKIT_ERA)] if pi % 3 else (pi // PRESETS_PER_ERA) % ERA_N)
        )
    lines.append("static const int8_t TECHNO_LOOP_KIT[TECHNO_LOOP_N] = {")
    lines.append(", ".join(kit_map))
    lines.append("};")

    # Minimal lead sidecars (pitch/len/bend) - zeros + sparse
    lines.append("static const int8_t TECHNO_LOOP_LEAD_PITCH[TECHNO_LOOP_N][16] = {")
    for pi in range(PRESET_N):
        row = [0] * 16
        row[pi % 16] = (pi % 7) - 3
        if pi % 5 == 0:
            row[(pi + 3) % 16] = 2
        lines.append("    { " + ", ".join(f"{v:4d}" for v in row) + " },")
    lines.append("};")

    lines.append("static const uint8_t TECHNO_LOOP_LEAD_LEN[TECHNO_LOOP_N][32] = {")
    for pi in range(PRESET_N):
        row = [0] * 32
        row[pi % 32] = 9 + (pi % 4)
        lines.append("    { " + ", ".join(f"{v:3d}" for v in row) + " },")
    lines.append("};")

    lines.append("static const uint8_t TECHNO_LOOP_LEAD_BEND[TECHNO_LOOP_N][32] = {")
    for pi in range(PRESET_N):
        row = [0] * 32
        row[pi % 32] = 3 + (pi % 5)
        lines.append("    { " + ", ".join(f"{v:3d}" for v in row) + " },")
    lines.append("};")

    gate_cs = [str(28 + (pi // PRESETS_PER_ERA) * 2) for pi in range(PRESET_N)]
    lines.append("static const uint8_t TECHNO_LOOP_LEAD_GATE_CS[TECHNO_LOOP_N] = {")
    lines.append(", ".join(gate_cs))
    lines.append("};")
    lines.append("")
    lines.append("static const uint8_t TECHNO_LEAD_LEN_16THS[15] = {")
    lines.append("    1, 2, 4, 5, 8, 11, 12, 16, 20, 24, 28, 32, 40, 48, 64")
    lines.append("};")

    # Apply helpers - copy idm pattern with renames
    lines.append(open(SRC / "_techno_apply_snippet.h").read() if (SRC / "_techno_apply_snippet.h").exists() else "")
    lines.append("#endif")
    return "\n".join(lines)


def write_apply_snippet():
    snippet = r'''
#include "fm_pgen_core.h"

/* Live generated plate (endless continuum); shadows the ROM loop at techno_gen.slot. */
static fm_pgen_t techno_gen = { .slot = 0xFFu };
static fm_pgen_lead_t techno_gen_lead;

#define FM_PGEN_STATE techno_gen
#define FM_PGEN_BUILD(era, n, c) techno_gen_build(era, n, c)

static inline uint8_t techno_gen_hit(uint8_t ix)
{
    return fm_pgen_hit(&techno_gen, ix);
}

static inline uint8_t techno_clamp_loop(uint8_t ix)
{
    return ix >= TECHNO_LOOP_N ? (uint8_t)(TECHNO_LOOP_N - 1u) : ix;
}

static inline uint8_t techno_clamp_era(uint8_t era)
{
    return era >= TECHNO_ERA_N ? (uint8_t)(TECHNO_ERA_N - 1u) : era;
}

static inline const char *techno_era_name(uint8_t era)
{
    return TECHNO_ERA_NAMES[techno_clamp_era(era)];
}

static inline const char *techno_loop_name(uint8_t slab)
{
    if (techno_gen_hit(slab))
        return techno_gen.name;
    return TECHNO_LOOP[techno_clamp_loop(slab)].name;
}

static inline const techno_loop_t *techno_loop(uint8_t ix)
{
    if (techno_gen_hit(ix))
        return &techno_gen.pl;
    return &TECHNO_LOOP[techno_clamp_loop(ix)];
}

static inline uint8_t techno_loop_root(uint8_t ix)
{
    return techno_gen_hit(ix) ? (uint8_t)techno_gen.root : (uint8_t)TECHNO_LOOP_ROOT[techno_clamp_loop(ix)];
}

static inline uint8_t techno_loop_scale(uint8_t ix)
{
    return techno_gen_hit(ix) ? (uint8_t)techno_gen.scale : (uint8_t)TECHNO_LOOP_SCALE[techno_clamp_loop(ix)];
}

static inline uint8_t techno_loop_swing(uint8_t ix)
{
    return techno_gen_hit(ix) ? (uint8_t)techno_gen.swing : (uint8_t)TECHNO_LOOP_SWING[techno_clamp_loop(ix)];
}

static inline uint8_t techno_loop_kit(uint8_t ix)
{
    return techno_gen_hit(ix) ? (uint8_t)techno_gen.kit : (uint8_t)TECHNO_LOOP_KIT[techno_clamp_loop(ix)];
}

static inline int8_t techno_loop_lead_rate(uint8_t ix)
{
    return techno_gen_hit(ix) ? techno_gen.rate : TECHNO_LOOP_LEAD_RATE[techno_clamp_loop(ix)];
}

static inline uint8_t techno_era_of_loop(uint8_t slab)
{
    slab = techno_clamp_loop(slab);
    return (uint8_t)(slab / GENRE_PRESETS_PER_ERA);
}

static inline uint8_t techno_era_first_loop(uint8_t era)
{
    return (uint8_t)(techno_clamp_era(era) * GENRE_PRESETS_PER_ERA);
}

static inline uint32_t techno_loop_lead_gate32(uint8_t slab)
{
    if (techno_gen_hit(slab))
        return techno_gen.lead_gate;
    return TECHNO_LOOP_LEAD_GATE[techno_clamp_loop(slab)];
}

static inline uint8_t techno_loop_lead_gate_cs(uint8_t slab)
{
    if (techno_gen_hit(slab))
        return techno_gen.gate_cs;
    return TECHNO_LOOP_LEAD_GATE_CS[techno_clamp_loop(slab)];
}

static inline uint8_t techno_loop_lead_step_bit(uint8_t slab, uint8_t step)
{
    uint32_t g32 = techno_loop_lead_gate32(slab);
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

static inline int8_t techno_loop_lead_pitch(uint8_t slab, uint8_t step)
{
    if (techno_gen_hit(slab))
        return techno_gen_lead.pitch[step & 15u];
    return TECHNO_LOOP_LEAD_PITCH[techno_clamp_loop(slab)][step & 15u];
}

static inline uint8_t techno_loop_lead_len_ix(uint8_t slab, uint8_t step)
{
    if (techno_gen_hit(slab))
        return techno_gen_lead.len[step & 31u];
    return TECHNO_LOOP_LEAD_LEN[techno_clamp_loop(slab)][step & 31u];
}

static inline uint8_t techno_loop_lead_bend_u7(uint8_t slab, uint8_t step)
{
    if (techno_gen_hit(slab))
        return techno_gen_lead.bend[step & 31u];
    return TECHNO_LOOP_LEAD_BEND[techno_clamp_loop(slab)][step & 31u];
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

static inline void techno_apply_defaults(dub_params_t *p)
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
    p->kick_boom = 60;
    p->rim_tone = 40;
    p->hat_lvl = 42;
    p->siren_pitch = 82;
    p->siren_rate = 44;
    p->drum_kit = DKIT_909;
    for (di = 0; di < 4u; di++) {
        p->drm_tune[di] = 64u;
        p->drm_dec[di] = 64u;
        p->drm_drv[di] = 14u;
        p->drm_dly[di] = 28u;
        p->drm_rev[di] = 24u;
        p->drm_cut[di] = 64u;
        p->drm_atk[di] = 0u;
        p->drm_dens[di] = 100u;
    }
}

static inline void techno_era_apply_base(dub_params_t *p, uint8_t era)
{
    const techno_era_base_t *eb;
    const techno_era_fx_t *fx;
    uint8_t i;

    if (!p)
        return;
    era = techno_clamp_era(era);
    eb = &TECHNO_ERA_BASE[era];
    fx = &TECHNO_ERA_FX[era];
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
    p->lvl_b = 80;
    p->lvl_c = 76;
    p->rev_b = (uint8_t)(20u + (fx->rev_mix >> 2));
    p->rev_c = (uint8_t)(22u + (fx->rev_mix >> 2));
}

static inline void techno_loop_apply(dub_params_t *p, uint8_t slab, uint8_t what)
{
    const techno_loop_t *sc;
    uint8_t di;

    if (!p)
        return;
    slab = techno_clamp_loop(slab);
    sc = techno_loop(slab);
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        techno_apply_defaults(p);
        techno_era_apply_base(p, sc->era);
    }
    p->era = sc->era;
    p->plate = slab;
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        p->bpm = sc->bpm;
        p->density = sc->density;
        for (di = 0; di < 4u; di++)
            p->drm_dens[di] = sc->density;
        p->drum_kit = techno_loop_kit(slab);
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
        p->root = techno_loop_root(slab);
        p->scale = techno_loop_scale(slab);
        p->swing = techno_loop_swing(slab);
        {
            int16_t lead = (int16_t)p->siren_rate + (int16_t)techno_loop_lead_rate(slab);

            if (lead < 0)
                lead = 0;
            else if (lead > 127)
                lead = 127;
            p->siren_rate = (uint8_t)lead;
        }
    }
}

static inline void techno_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t slab)
{
    if (!p)
        return;
    era = techno_clamp_era(era);
    slab = techno_clamp_loop(slab);
    techno_loop_apply(p, slab, DUB_APPLY_ALL);
    p->era = era;
    p->plate = slab;
    p->drum_kit = techno_loop_kit(slab);
    p->bass_model = techno_loop(slab)->bass_model;
    p->voicing = techno_loop(slab)->voicing;
    p->bpm = techno_loop(slab)->bpm;
    p->pump = techno_loop(slab)->pump;
    p->feedback = techno_loop(slab)->feedback;
    p->wobble = techno_loop(slab)->wobble;
    p->drum_pat = slab;
    p->root = techno_loop_root(slab);
    p->scale = techno_loop_scale(slab);
    p->swing = techno_loop_swing(slab);
}

static inline uint8_t techno_drm_density_allows(uint8_t density, uint8_t step, uint8_t voice_ix)
{
    return ((step * 19u + (voice_ix * 37u)) & 127u) < density;
}

static inline uint8_t techno_gate_after_drm_density(uint8_t on, uint8_t density, uint8_t step, uint8_t voice_ix)
{
    if (!on)
        return 0u;
    return techno_drm_density_allows(density, step, voice_ix);
}

static inline uint8_t dub_step_bit(uint32_t mask, uint8_t step)
{
    return (uint8_t)((mask >> (step & (DUB_STEPS - 1u))) & 1u);
}

static inline void dub_resolve_step(const dub_params_t *p, const dub_step_t *ovr, uint8_t step, dub_events_t *out, uint8_t apply_dens)
{
    const techno_loop_t *drum;
    const techno_loop_t *main;
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t forced;

    if (!p || !out)
        return;
    if (step >= DUB_STEPS)
        step = (uint8_t)(DUB_STEPS - 1u);
    drum = techno_loop(p->drum_pat);
    main = techno_loop(p->plate);
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
        kick = techno_gate_after_drm_density(kick, p->drm_dens[0], step, 0u);
        rim = techno_gate_after_drm_density(rim, p->drm_dens[1], step, 1u);
        hat = techno_gate_after_drm_density(hat, p->drm_dens[2], step, 2u);
        siren = techno_gate_after_drm_density(siren, p->drm_dens[3], step, 3u);
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
    const techno_loop_t *sc;

    plate = techno_clamp_loop(plate);
    sc = techno_loop(plate);
    if (root)
        *root = techno_loop_root(plate);
    if (scale)
        *scale = techno_loop_scale(plate);
    if (swing)
        *swing = techno_loop_swing(plate);
    if (bpm)
        *bpm = sc->bpm;
}

#include "techno_gen.h"
'''
    (SRC / "_techno_apply_snippet.h").write_text(snippet)


def patch_techno_dsp_layout(out: str) -> str:
    era_aliases = """
/* IDM-shared DSP paths use semantic era / bass aliases */
#define ERA_OBLIQ ERA_SCHRANZ
#define ERA_WARM ERA_DETROIT
#define ERA_GLITCH ERA_HYPNO
#define ERA_MELO ERA_BELGIUM
#define ERA_DRILL ERA_SCHRANZ
#define ERA_BREAK ERA_EBM
#define BASS_ROUND BASS_RUMBLE
#define BASS_THROB BASS_SUB909
"""
    out = out.replace(
        '#include "techno_data.h"\n#include "jazz_pluck.h"',
        '#include "techno_data.h"\n#include "jazz_pluck.h"\n' + era_aliases.strip() + "\n",
        1,
    )
    out = re.sub(
        r"/\* --- Bass: soft \(WARM–GLITCH\).*?\n\}\n\n\nstatic inline int32_t techno_bass_house_sub",
        "static inline int32_t techno_bass_house_sub",
        out,
        count=1,
        flags=re.S,
    )
    out = out.replace("techno_piece_lead_gate_cs", "techno_loop_lead_gate_cs")
    extra_compat = """
#define idm_env_body_gate techno_env_body_gate
#define idm_env_body techno_env_body
#define idm_lead_timbre techno_lead_timbre
#define idm_kit_k_note techno_kit_k_note
#define idm_kit_k_sweep techno_kit_k_sweep
#define idm_kit_k_fm techno_kit_k_fm
#define idm_kit_k_drv techno_kit_k_drv
#define idm_kit_r_note techno_kit_r_note
#define idm_kit_r_wire techno_kit_r_wire
#define idm_hat_glitch_note techno_hat_glitch_note
#define idm_hat_glitch_fm techno_hat_glitch_fm
#define idm_hat_glitch_noise techno_hat_glitch_noise
#define idm_hat_glitch_metal techno_hat_glitch_metal
#define idm_hat_glitch_style techno_hat_glitch_style
#define idm_hat_glitch_cut techno_hat_glitch_cut
#define idm_drm_kick_shape techno_drm_kick_shape
#define idm_drm_rim_mix techno_drm_rim_mix
#define idm_drm_bus techno_drm_bus
#define idm_drm_room_send techno_drm_room_send
#define IDM_ERA_MACROS TECHNO_ERA_MACROS
#define IDM_LEAD_LEN_N TECHNO_LEAD_LEN_N
#define IDM_LEAD_LEN_16THS TECHNO_LEAD_LEN_16THS
#define IDM_HAT_GLITCH_WASH TECHNO_HAT_GLITCH_WASH
#define IDM_HAT_GLITCH_BELL TECHNO_HAT_GLITCH_BELL
#define IDM_HAT_GLITCH_SWING TECHNO_HAT_GLITCH_SWING
#define IDM_HAT_GLITCH_ACID_HAT TECHNO_HAT_GLITCH_ACID_HAT
#define idm_lead_dly_boost techno_lead_dly_boost
"""
    if "idm_env_body_gate techno_env_body_gate" not in out:
        out = out.replace(
            "#if defined(GENRE_TECHNO)\n#define idm_stutter_t",
            "#if defined(GENRE_TECHNO)\n" + extra_compat.strip() + "\n#define idm_stutter_t",
            1,
        )
    return out


def patch_techno_dsp_eras(out: str) -> str:
    """6-era lead/stutter/degrade + IDM compat macros (no BSUN/HOOL)."""
    sustain = """
static const techno_lead_sustain_t TECHNO_LEAD_SUSTAIN[TECHNO_ERA_N] = {
    { 62, 4, 10, 30 }, /* DETROIT */
    { 68, 5, 11, 34 }, /* BELGIUM */
    { 42, 2,  8, 14 }, /* SCHRANZ */
    { 52, 4, 10, 22 }, /* HYPNO */
    { 58, 5, 12, 32 }, /* BERLIN */
    { 48, 3,  9, 18 }, /* EBM */
};
"""
    out = re.sub(
        r"static const techno_lead_sustain_t TECHNO_LEAD_SUSTAIN\[TECHNO_ERA_N\] = \{.*?\};",
        sustain.strip(),
        out,
        count=1,
        flags=re.S,
    )
    timbre = """    static const uint8_t timbre[TECHNO_ERA_N] = {
        TECHNO_LEAD_EP, TECHNO_LEAD_SAX, TECHNO_LEAD_MUTE_TRP,
        TECHNO_LEAD_CLAV, TECHNO_LEAD_HORN, TECHNO_LEAD_FLUTE
    };"""
    out = re.sub(
        r"static const uint8_t timbre\[TECHNO_ERA_N\] = \{.*?\};",
        timbre.strip(),
        out,
        count=1,
        flags=re.S,
    )
    degrade_base = """    static const uint8_t base[TECHNO_ERA_N] = { 8, 10, 18, 22, 16, 12 };"""
    out = re.sub(
        r"static const uint8_t base\[TECHNO_ERA_N\] = \{.*?\};",
        degrade_base.strip(),
        out,
        count=1,
        flags=re.S,
    )
    stutter_fn = """
static inline uint16_t techno_stutter_slice_from_vel(uint8_t vel, uint8_t era)
{
    uint16_t base = (uint16_t)(28u + ((uint32_t)vel * 88u) / 127u);
    uint8_t e = techno_clamp_era(era);

    if (e == (uint8_t)ERA_SCHRANZ)
        base = (uint16_t)((base * 5u) / 8u);
    else if (e == (uint8_t)ERA_BELGIUM)
        base = (uint16_t)((base * 5u) / 4u);
    else if (e == (uint8_t)ERA_BERLIN)
        base = (uint16_t)((base * 3u) / 4u);
    else if (e == (uint8_t)ERA_EBM)
        base = (uint16_t)((base * 5u) / 6u);
    if (base < 6u)
        base = 6u;
    if (base > 512u)
        base = 512u;
    return base;
}
"""
    out = re.sub(
        r"static inline uint16_t techno_stutter_slice_from_vel\(uint8_t vel, uint8_t era\)\n\{.*?\n\}\n",
        stutter_fn.strip() + "\n",
        out,
        count=1,
        flags=re.S,
    )
    lead_dly = """
static inline int32_t techno_lead_dly_boost(const dub_params_t *p)
{
    (void)p;
    return 0;
}
"""
    if "techno_lead_dly_boost" not in out:
        out = out.replace(
            "#if defined(GENRE_TECHNO)\n#define idm_env_body_gate",
            lead_dly.strip() + "\n\n#if defined(GENRE_TECHNO)\n#define idm_env_body_gate",
            1,
        )
    return out


def gen_dsp_h() -> None:
    idm = (SRC / "idm_dsp.h").read_text()
    out = idm
    repl = [
        ("IDM_DSP_H", "TECHNO_DSP_H"),
        ("techno_data.h", "techno_data.h"),
        ("idm_", "techno_"),
        ("IDM_", "TECHNO_"),
        ("idm hat_glitch", "techno hat"),
    ]
    for a, b in repl:
        out = out.replace(a, b)
    # fix double techno_snowcrash
    out = out.replace("techno_", "techno_")
    out = out.replace("SNOWCRASHTECHNO_", "TECHNO_")

    bass_block = r'''
static inline int32_t techno_bass_house_sub(dub_synth_t *s, uint32_t inc, int32_t fc, uint8_t reso)
{
    int32_t raw;

    s->sub_ph += inc;
    raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph >> 1) >> 2);
    dub_svf_mode(raw, 600 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, reso, &raw);
    return raw;
}

static inline int32_t techno_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                       int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t raw = 0;
    int32_t walk = lfo_mod_b;

    switch (model) {
    case BASS_RUMBLE: {
        s->sub_ph += inc >> 1;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph >> 1) >> 2);
        dub_svf_mode(raw, 280 + (fc >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        raw = (raw * (int32_t)(128u - (p->pump >> 1))) >> 7;
        break;
    }
    case BASS_SUB909: {
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph << 1) >> 2);
        dub_svf_mode(raw, 1100 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_EBM16: {
        s->sub_ph += inc;
        raw = dub_saw(s->sub_ph) + (dub_square(s->sub_ph) >> 2);
        dub_svf_mode(raw, fc + (walk >> 1), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 12u), &raw);
        raw = (raw * (int32_t)s->sub_env) >> 14;
        break;
    }
    case BASS_HOOVER: {
        uint32_t det = inc >> 4;
        s->sub_ph += inc;
        s->sub_ph2 += inc + det;
        s->sub_ph3 += inc - det;
        raw = dub_saw(s->sub_ph) + dub_saw(s->sub_ph2) + dub_saw(s->sub_ph3);
        dub_svf_mode(raw, fc + (wob_all >> 9), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 18u), &raw);
        break;
    }
    case BASS_ACID303: {
        int32_t saw_w, acid_fc;
        s->sub_ph += inc;
        saw_w = dub_saw(s->sub_ph);
        raw = saw_w + (dub_square(s->sub_ph) >> 1);
        acid_fc = fc + (((s->sub_env >> 3) * (int32_t)p->wobble) >> 5);
        dub_svf_mode(raw, acid_fc, &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode + 28u > 127u ? 127u : p->reso_mode + 28u), &raw);
        break;
    }
    case BASS_DETROIT: {
        int32_t mod2;
        s->sub_ph += inc;
        s->sub_ph2 += inc + (inc >> 2);
        mod2 = dub_sine_q64(s->sub_ph2);
        raw = dub_sine_q64(s->sub_ph + (uint32_t)(mod2 << 12));
        dub_svf_mode(raw, fc + (walk >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        raw = (raw * (int32_t)s->sub_env) >> 15;
        break;
    }
    case BASS_SCHRANZ: {
        s->sub_ph += inc;
        raw = dub_saw(s->sub_ph) + (dub_sine_q64(s->sub_ph) >> 2);
        dub_svf_mode(raw, 500 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 8u), &raw);
        raw = (raw * (int32_t)(96u + (p->drive >> 1))) >> 7;
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
        raw = (s1 + s2 + s3) / 3;
        dub_svf_mode(raw, fc, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_MODFM: {
        int32_t mod2;
        s->sub_ph += inc;
        s->sub_ph2 += inc << 1;
        mod2 = dub_sine_q64(s->sub_ph2);
        raw = dub_sine_q64(s->sub_ph + (uint32_t)(mod2 << 13));
        dub_svf_mode(raw, fc + ((int32_t)p->wobble * 6), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_DRONE: {
        s->sub_ph += inc >> 2;
        raw = dub_sine_q64(s->sub_ph) >> 1;
        raw += dub_sine_q64(s->sub_ph + (uint32_t)(p->wobble << 13)) >> 2;
        dub_svf_mode(raw, fc >> 1, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode >> 1, &raw);
        break;
    }
    default:
        return techno_bass_house_sub(s, inc, fc, p->reso_mode);
    }
    (void)p;
    return raw;
}

'''
    # Replace old bass render in generated file
    out = re.sub(
        r"/\* --- Bass:.*?\*/\nstatic inline int32_t techno_bass_render\(.*?\n\}\n",
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
#if defined(GENRE_TECHNO)
#define idm_stutter_t techno_stutter_t
#define idm_piece_lead_step_bit techno_loop_lead_step_bit
#define idm_piece_lead_pitch techno_loop_lead_pitch
#define idm_piece_lead_len_ix techno_loop_lead_len_ix
#define idm_piece_lead_bend_u7 techno_loop_lead_bend_u7
#define idm_lead_bend_plock techno_lead_bend_plock
#define idm_lead_bend_semi techno_lead_bend_semi
#define idm_lead_len_ix techno_lead_len_ix
#define idm_fill_morph_steps techno_fill_morph_steps
#define idm_stutter_edit_live techno_stutter_edit_live
#define idm_u8_blend techno_u8_blend
#define idm_fill_build_amt techno_fill_build_amt
#define idm_stutter_edit_targets techno_stutter_edit_targets
#define idm_fill_drop_targets techno_fill_drop_targets
#define idm_apply_fill_break_voice techno_apply_fill_break_voice
#define idm_stutter_edit_restore techno_stutter_edit_restore
#define idm_stutter_start techno_stutter_start
#define idm_stutter_release techno_stutter_release
#define idm_stutter_bar techno_stutter_bar
#define idm_edit_latch_begin techno_edit_latch_begin
#define idm_stutter_latch_block techno_stutter_latch_block
#define idm_rev_stutter_flip techno_rev_stutter_flip
#define idm_glitch_burst_hit techno_glitch_burst_hit
#define idm_stutter_sync_runtime techno_stutter_sync_runtime
#define idm_edit_apply_drums techno_edit_apply_drums
#define idm_stutter_process_buffer techno_stutter_process_buffer
#define idm_loop_macro_sync techno_loop_macro_sync
#define idm_lead_note_gate_piece techno_lead_note_gate_piece
#define idm_siren_atk_samples techno_siren_atk_samples
#define idm_siren_env_step techno_siren_env_step
#define idm_lead_render techno_lead_render
#define idm_lead_bent_inc techno_lead_bent_inc
#define idm_lead_len_samples techno_lead_len_samples
#define idm_stutter_push techno_stutter_push
#define idm_stutter_mix techno_stutter_mix
#define idm_stutter_slice_from_vel techno_stutter_slice_from_vel
#define idm_degrade_amount techno_degrade_amount
#define idm_degrade_sample techno_degrade_sample
#define idm_hat_glitch_is_acid_kit techno_hat_glitch_is_acid_kit
#define idm_hat_glitch_decay techno_hat_glitch_decay
#define idm_hat_glitch_open techno_hat_glitch_open
#define IDM_PIECE TECHNO_LOOP
#define IDM_PIECE_KIT TECHNO_LOOP_KIT
#define idm_era_first_piece techno_era_first_loop
#define IDM_LEAD_LEN_NAMES TECHNO_LEAD_LEN_NAMES
#define IDM_STUT_HACK TECHNO_STUT_HACK
#define IDM_STUT_SCREW TECHNO_STUT_SCREW
/* IDM_FILL_MORPH_STEPS / IDM_STUTTER_MORPH_STEPS: defined in fm_app.c */
#endif
'''
    out = out.replace("#endif\n", compat + "\n#endif\n", 1)
    out = patch_techno_dsp_layout(out)
    out = patch_techno_dsp_eras(out)
    (SRC / "techno_dsp.h").write_text(out)


def gen_views_section() -> str:
    """Generate FM_VIEWS rows from plate 16-bit masks (S2..S8 variants)."""
    # read plates from generated data - parse kick from file later; use algorithm
    lines = ["#elif defined(GENRE_TECHNO)", "#define FM_VIEW_PLATE_N TECHNO_LOOP_N"]
    lines.append("static const uint16_t FM_VIEWS[][FM_VIEW_LANE_N][FM_VIEW_N - 1u] = {")
    lane_ix = ["kick", "rim", "hat", "siren", "sub", "chord"]
    for pi in range(PRESET_N):
        era, ix = pi // PRESETS_PER_ERA, pi % PRESETS_PER_ERA
        name = LOOP_NAMES[era][ix]
        lines.append(f"    /* {pi} {name} */")
        lines.append("    {")
        for li, _ in enumerate(lane_ix):
            base = mask16(20 + li, era, ix)
            row = []
            for v in range(7):
                m = base
                m ^= mask16(30 + v, era, ix + li)
                if v & 1:
                    m = ((m & 0xFF00) >> 8) | ((m & 0xFF) << 8)
                row.append(f"0x{m:04X}u")
            lines.append(f"        {{ {', '.join(row)} }}, /* {lane_ix[li][0].upper()} */")
        lines.append("    },")
    lines.append("};")
    return "\n".join(lines)


def patch_fm_views(section: str) -> None:
    path = SRC / "fm_views_data.h"
    text = path.read_text()
    if "GENRE_TECHNO" in text:
        text = re.sub(r"#elif defined\(GENRE_TECHNO\).*?(?=#elif defined|#else\n#define FM_VIEW)", section + "\n", text, flags=re.S)
    else:
        text = text.replace("#else\n#define FM_VIEW_PLATE_N", section + "\n#else\n#define FM_VIEW_PLATE_N", 1)
    path.write_text(text)


def patch_file_genre(path: Path) -> None:
    text = path.read_text()
    if "GENRE_TECHNO" in text:
        return
    text = text.replace(
        "!defined(GENRE_FUNK)",
        "!defined(GENRE_FUNK) && !defined(GENRE_TECHNO)",
    )
    text = text.replace(
        "|| defined(GENRE_POP) || defined(GENRE_FUNK)",
        "|| defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_TECHNO)",
    )
    text = text.replace(
        "defined(GENRE_IDM) || defined(GENRE_SNOWCRASH)",
        "defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO)",
    )
    text = text.replace(
        "|| defined(GENRE_TECHNO) || defined(GENRE_TECHNO)",
        "|| defined(GENRE_TECHNO)",
    )
    text = text.replace(
        "&& !defined(GENRE_TECHNO) && !defined(GENRE_TECHNO)",
        "&& !defined(GENRE_TECHNO)",
    )
    path.write_text(text)


def write_techno_conf() -> None:
    conf = """/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TECHNO_CONF_H
#define TECHNO_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_CFG_USER_PLATE_N 60u
#define GENRE_PRESET_N_LEGACY 48u
#define GENRE_CFG_MAGIC  0x54454331u /* TEC1 */
#define GENRE_APP_NAME   "KO TECHNO"
#define GENRE_FELUCCA_ID "FM-1_56409"

#define GENRE_PRESET_LABEL "LOOP"
#define GENRE_PRESET_HDR   "LOP"

#define GENRE_C_ACC RGB(80, 220, 255)

#define GENRE_BANK_COL \\
    { RGB(60, 200, 230), RGB(255, 220, 60), RGB(255, 60, 60), RGB(180, 80, 255), \\
      RGB(120, 160, 200), RGB(80, 255, 100) }

#define GENRE_ERA_COL \\
    { RGB(0, 220, 255), RGB(255, 230, 40), RGB(255, 40, 50), RGB(160, 60, 255), \\
      RGB(100, 150, 210), RGB(60, 255, 80) }

#endif
"""
    (SRC / "techno_conf.h").write_text(conf)


def patch_genre_conf_pack() -> None:
    conf = SRC / "genre_conf.h"
    t = conf.read_text()
    if "GENRE_TECHNO" not in t:
        t = t.replace(
            "#elif defined(GENRE_SNOWCRASH)\n#include \"snowcrash_conf.h\"",
            "#elif defined(GENRE_SNOWCRASH)\n#include \"snowcrash_conf.h\"\n#elif defined(GENRE_TECHNO)\n#include \"techno_conf.h\"",
        )
        conf.write_text(t)
    pack = SRC / "genre_pack.h"
    t = pack.read_text()
    if "techno_data.h" not in t:
        block = r'''#elif defined(GENRE_TECHNO)
#include "techno_data.h"
'''
        t = t.replace(
            '#elif defined(GENRE_SNOWCRASH)\n#include "snowcrash_data.h"',
            '#elif defined(GENRE_SNOWCRASH)\n#include "snowcrash_data.h"\n' + block,
        )
        macros = r'''#elif defined(GENRE_TECHNO)
#define genre_clamp_era(era)       techno_clamp_era(era)
#define genre_clamp_preset(ix)     techno_clamp_loop(ix)
#define genre_era_name(era)        techno_era_name(era)
#define genre_preset_name(ix)      techno_loop_name(ix)
#define genre_preset(ix)           techno_loop(ix)
#define genre_era_of_preset(pl)    techno_era_of_loop(pl)
#define genre_era_first_preset(er) techno_era_first_loop(er)
#define genre_preset_rebuild(p, era, pl) techno_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   techno_loop_apply(p, pl, what)
#define genre_era_apply_base(p, era)     techno_era_apply_base(p, era)
#define genre_apply_defaults(p)          techno_apply_defaults(p)
'''
        t = t.replace(
            "#define genre_apply_defaults(p)          snowcrash_apply_defaults(p)\n#elif defined(GENRE_FUNK)",
            "#define genre_apply_defaults(p)          snowcrash_apply_defaults(p)\n" + macros + "#elif defined(GENRE_FUNK)",
        )
        pack.write_text(t)
    t = pack.read_text()
    if "GENRE_TECHNO)" not in t.split("genre_lead_step_bit")[0]:
        t = t.replace(
            "#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH)",
            "#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO)",
            2,
        )
        t = t.replace(
            "#elif defined(GENRE_SNOWCRASH)\n#define genre_lead_gate32(b)          snowcrash_slab_lead_gate32(b)",
            "#elif defined(GENRE_SNOWCRASH)\n#define genre_lead_gate32(b)          snowcrash_slab_lead_gate32(b)\n#elif defined(GENRE_TECHNO)\n#define genre_lead_gate32(b)          techno_loop_lead_gate32(b)",
        )
        pack.write_text(t)


def patch_dub_dsp_include() -> None:
    path = SRC / "dub_dsp.h"
    t = path.read_text()
    if "techno_dsp" not in t:
        t = t.replace(
            "#elif defined(GENRE_SNOWCRASH)\n#include \"snowcrash_dsp.h\"",
            "#elif defined(GENRE_SNOWCRASH)\n#include \"snowcrash_dsp.h\"\n#elif defined(GENRE_TECHNO)\n#include \"techno_dsp.h\"",
        )
        t = t.replace(
            "raw = snowcrash_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);",
            "raw = snowcrash_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);\n#elif defined(GENRE_TECHNO)\n                raw = techno_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);",
        )
        t = t.replace(
            "#elif defined(GENRE_SNOWCRASH) || defined(GENRE_IDM)\n    static const int8_t tab[VOICE_N][4]",
            "#elif defined(GENRE_TECHNO) || defined(GENRE_SNOWCRASH) || defined(GENRE_IDM)\n    static const int8_t tab[VOICE_N][4]",
        )
        path.write_text(t)


def patch_fm_app_techno_ui() -> None:
    path = SRC / "fm_app.c"
    t = path.read_text()
    if "TECHNO_PLATE_SEQ" in t:
        return
    t = t.replace(
        "#elif defined(GENRE_SNOWCRASH)\n#define DUB_CFG_MAGIC_S16_PREV48 0x534E4331u",
        "#elif defined(GENRE_SNOWCRASH)\n#define DUB_CFG_MAGIC_S16_PREV48 0x534E4331u\n#elif defined(GENRE_TECHNO)\n#define DUB_CFG_MAGIC_S16_PREV48 0x54454330u\n#define DUB_CFG_MAGIC_S16_PREV   0x54454331u\n#define DUB_CFG_MAGIC_PREV       0xD4544331u",
    )
    m = re.search(
        r"#elif defined\(GENRE_SNOWCRASH\)\nstatic const dub_plate_seq_t SNOWCRASH_PLATE_SEQ\[GENRE_PRESET_N\] = \{.*?\n\};\n#endif",
        t,
        re.S,
    )
    if m:
        sc_block = m.group(0)
        tc_block = sc_block.replace("GENRE_SNOWCRASH", "GENRE_TECHNO").replace(
            "SNOWCRASH_PLATE_SEQ", "TECHNO_PLATE_SEQ"
        )
        t = t.replace(sc_block, sc_block + "\n" + tc_block.replace("#endif", ""))
    t = t.replace(
        "#elif defined(GENRE_SNOWCRASH)\n    return &SNOWCRASH_PLATE_SEQ[plate];",
        "#elif defined(GENRE_SNOWCRASH)\n    return &SNOWCRASH_PLATE_SEQ[plate];\n#elif defined(GENRE_TECHNO)\n    return &TECHNO_PLATE_SEQ[plate];",
    )
    t = t.replace(
        '#elif defined(GENRE_SNOWCRASH)\n#include "snowcrash_draw.h"',
        '#elif defined(GENRE_SNOWCRASH)\n#include "snowcrash_draw.h"\n#elif defined(GENRE_TECHNO)\n#include "techno_draw.h"',
    )
    t = t.replace(
        "#elif defined(GENRE_SNOWCRASH)\nstatic const char *const SEQ_LANE[6] = {\"KICK\", \"SNAR\", \"HATS\", \"BLIP\", \"BASS\", \"CHORD\"};",
        "#elif defined(GENRE_SNOWCRASH)\nstatic const char *const SEQ_LANE[6] = {\"KICK\", \"SNAR\", \"HATS\", \"BLIP\", \"BASS\", \"CHORD\"};\n#elif defined(GENRE_TECHNO)\nstatic const char *const SEQ_LANE[6] = {\"KICK\", \"SNAR\", \"HAT\", \"RAVE\", \"BASS\", \"CHORD\"};",
    )
    t = t.replace(
        "#elif defined(GENRE_SNOWCRASH)\n    \"GLOSS\",  \"SLICE\",  \"HOP\",    \"TIDE\",   \"GHOST\",  \"CLICK\",  \"SLAG\",   \"HUM\",",
        "#elif defined(GENRE_SNOWCRASH)\n    \"GLOSS\",  \"SLICE\",  \"HOP\",    \"TIDE\",   \"GHOST\",  \"CLICK\",  \"SLAG\",   \"HUM\",\n#elif defined(GENRE_TECHNO)\n    \"PUMP\",   \"RUMBLE\", \"HOVER\",  \"TANK\",   \"STEEL\",  \"DIST\",   \"HARD\",   \"GATE\",",
    )
    t = t.replace(
        "#ifdef GENRE_SNOWCRASH\n    \"DRIFT\",  \"FLUX\",",
        "#ifdef GENRE_SNOWCRASH\n    \"DRIFT\",  \"FLUX\",\n#elif defined(GENRE_TECHNO)\n    \"PHASE\",  \"DRIVE\",",
    )
    t = t.replace(
        "#elif defined(GENRE_SNOWCRASH)\n    \"DECK\",",
        "#elif defined(GENRE_SNOWCRASH)\n    \"DECK\",\n#elif defined(GENRE_TECHNO)\n    \"RIG\",",
    )
    t = t.replace(
        "|| defined(GENRE_SNOWCRASH) || defined(GENRE_JAZZ)",
        "|| defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_JAZZ)",
    )
    t = t.replace(
        "#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH)\n    return idm_stutter_edit_live();",
        "#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO)\n    return idm_stutter_edit_live();",
    )
    if 'L0[4] = {"PUMP", "RUMBLE", "HOVER", "TANK"}' not in t:
        t = t.replace(
            '#elif defined(GENRE_FUNK)\n        static const char *const L0[4] = {"CHOP", "CROP", "WAH", "SWEEP"};\n'
            '        static const char *const L1[4] = {"SPACE", "DUCK", "GRIT", "THUMP"};\n'
            '#else\n        static const char *const L0[4] = {"SKANK", "LAYER", "WARP", "SWEEP"};',
            '#elif defined(GENRE_FUNK)\n        static const char *const L0[4] = {"CHOP", "CROP", "WAH", "SWEEP"};\n'
            '        static const char *const L1[4] = {"SPACE", "DUCK", "GRIT", "THUMP"};\n'
            '#elif defined(GENRE_TECHNO)\n        static const char *const L0[4] = {"PUMP", "RUMBLE", "HOVER", "TANK"};\n'
            '        static const char *const L1[4] = {"STEEL", "DIST", "HARD", "GATE"};\n'
            '#else\n        static const char *const L0[4] = {"SKANK", "LAYER", "WARP", "SWEEP"};',
        )
        t = t.replace(
            '#elif defined(GENRE_FUNK)\n        static const char *const L2[4] = {"MOTION", "FLUX", "UNIT", "INVERT"};\n'
            '#else\n        static const char *const L2[4] = {"MOTION", "FLUX", "SYSTEM", "INVERT"};',
            '#elif defined(GENRE_FUNK)\n        static const char *const L2[4] = {"MOTION", "FLUX", "UNIT", "INVERT"};\n'
            '#elif defined(GENRE_TECHNO)\n        static const char *const L2[4] = {"PHASE", "DRIVE", "RIG", "INVERT"};\n'
            '#else\n        static const char *const L2[4] = {"MOTION", "FLUX", "SYSTEM", "INVERT"};',
        )
    path.write_text(t)


def patch_fm_seq_techno() -> None:
    path = SRC / "fm_seq.h"
    t = path.read_text()
    if "GENRE_TECHNO" in t:
        return
    t = t.replace(
        "#elif defined(GENRE_SNOWCRASH)\nstatic const char *const FM_SEQ_KNOB_P4[4] = {\"CYCLE\", \"REPEAT\", \"DECK\", \"INVERT\"};",
        "#elif defined(GENRE_SNOWCRASH)\nstatic const char *const FM_SEQ_KNOB_P4[4] = {\"CYCLE\", \"REPEAT\", \"DECK\", \"INVERT\"};\n#elif defined(GENRE_TECHNO)\nstatic const char *const FM_SEQ_KNOB_P4[4] = {\"CYCLE\", \"REPEAT\", \"RIG\", \"INVERT\"};",
    )
    path.write_text(t)


def patch_gen_views_py() -> None:
    path = ROOT / "tools" / "gen_views.py"
    t = path.read_text()
    if '"TECHNO"' in t:
        return
    entry = (
        '    ("TECHNO", "techno_data.h", "TECHNO_LOOP", "TECHNO_LOOP_N", "techno_conf.h", "GENRE_TECHNO"),\n'
    )
    t = t.replace(
        '    ("SNOWCRASH", "snowcrash_data.h", "SNOWCRASH_SLAB", "SNOWCRASH_SLAB_N", "snowcrash_conf.h", "GENRE_SNOWCRASH"),\n',
        '    ("SNOWCRASH", "snowcrash_data.h", "SNOWCRASH_SLAB", "SNOWCRASH_SLAB_N", "snowcrash_conf.h", "GENRE_SNOWCRASH"),\n'
        + entry,
    )
    t = t.replace(
        '    "SNOWCRASH": "SNOWCRASH_SLAB_LEAD_GATE",\n',
        '    "SNOWCRASH": "SNOWCRASH_SLAB_LEAD_GATE",\n    "TECHNO": "TECHNO_LOOP_LEAD_GATE",\n',
    )
    path.write_text(t)


def main() -> None:
    write_techno_conf()
    write_apply_snippet()
    (SRC / "techno_data.h").write_text(gen_data_h())
    gen_dsp_h()
    patch_gen_views_py()
    patch_fm_views(gen_views_section())
    for f in ["dub_dsp.h", "dub_types.h", "fm_app.c", "fm_seq.h"]:
        patch_file_genre(SRC / f)
    patch_genre_conf_pack()
    patch_dub_dsp_include()
    patch_fm_app_techno_ui()
    patch_fm_seq_techno()
    draw = (SRC / "idm_draw.h").read_text()
    draw = draw.replace("IDM_DRAW_H", "TECHNO_DRAW_H").replace("idm_", "techno_").replace("IDM", "TECHNO")
    draw = draw.replace("GENRE_IDM", "GENRE_TECHNO")
    draw = draw.replace("techno_piece_t", "techno_loop_t")
    draw = draw.replace("techno_piece(", "techno_loop(")
    draw = draw.replace('"LEAD"', '"RAVE"', 1)
    (SRC / "techno_draw.h").write_text(draw)
    (SRC / "_techno_apply_snippet.h").unlink(missing_ok=True)
    import subprocess

    subprocess.run([sys.executable, str(ROOT / "tools" / "gen_views.py")], check=True, cwd=ROOT)
    print("techno pack generated")


if __name__ == "__main__":
    main()
