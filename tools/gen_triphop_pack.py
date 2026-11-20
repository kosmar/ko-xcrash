#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate triphop_data.h, fm_views techno section, triphop_dsp.h (from idm template)."""
from __future__ import annotations

import hashlib
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "firmware" / "src"

ERA_N = 6
ERA_NAMES = ["DUST", "VINYL", "SMOKE", "STRINGS", "DARK", "CINEMA"]
ERA_FULL = ERA_NAMES

LOOP_NAMES = [
    ["CRACK", "HUSH", "LOFI", "TAPE", "WARP", "DUSTY", "GRIT", "FADE", "HUM", "DRIFT"],
    ["GROOV", "SKIP", "WOBB", "POP", "HISS", "WORN", "DEEP", "SOFT", "WARM", "RUST"],
    ["HAZE", "SMOKE", "MIST", "VEIL", "SHADE", "DUSK", "FOG", "PALE", "DIM", "ASH"],
    ["PAD", "RHODE", "STRNG", "VIOLA", "CELLO", "BOW", "AIRY", "LUSH", "WIDE", "SLOW"],
    ["SUB", "VOID", "NIGHT", "BASS", "DOOM", "LOW", "HEAVY", "GRIM", "BLACK", "ABYSS"],
    ["SCORE", "FILM", "SCENE", "CUT", "REEL", "FRAME", "SCOPE", "EPIC", "MOOD", "TALE"],
]

BASS_NAMES = [
    "SUBDEEP", "VINYL", "SMOKE", "RHODES", "WARM", "DARK", "STRING", "DOOM", "TAPE", "CINEMA",
]
BASS_ERA = [0, 1, 2, 3, 4, 0, 3, 4, 1, 5]
BASS_N = len(BASS_NAMES)

DKIT_NAMES = ["DUST", "VINYL", "SMOKE", "STRINGS", "DARK", "CINEMA", "TAPE", "FILM"]
DKIT_ERA = [0, 1, 2, 3, 4, 5, 1, 5]

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

ERA_BPM_LO = [72, 78, 82, 76, 70, 68]
ERA_BPM_HI = [88, 92, 98, 90, 86, 84]
ERA_DENS_BASE = [32, 34, 28, 30, 26, 24]
ERA_SWING_BASE = [58, 62, 54, 56, 52, 60]
ERA_PUMP_BASE = [28, 32, 24, 20, 36, 22]
ERA_DLY_C_BASE = [58, 62, 54, 64, 60, 66]
ERA_REV_C_BASE = [68, 72, 62, 78, 74, 80]

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
        "#ifndef TRIPHOP_DATA_H",
        "#define TRIPHOP_DATA_H",
        "",
        '#include "dub_types.h"',
        '#include "triphop_conf.h"',
        '#include "fm_seq.h"',
        "",
        "enum { TRIPHOP_ERA_N = GENRE_ERA_N, TRIPHOP_LOOP_N = GENRE_PRESET_N };",
    ]
    era_enum = ", ".join(f"ERA_{n}" for n in ERA_NAMES)
    lines.append(f"enum {{ {era_enum} }};")
    lines.append(
        "enum {\n"
        "    BASS_SUBDEEP = 0, BASS_VINYL, BASS_SMOKE, BASS_RHODES, BASS_WARM, BASS_DARK,\n"
        "    BASS_STRING, BASS_DOOM, BASS_TAPE, BASS_CINEMA, BASS_N\n"
        "};"
    )
    lines.append(
        "enum {\n"
        "    DKIT_DUST = 0, DKIT_VINYL, DKIT_SMOKE, DKIT_STRINGS, DKIT_DARK, DKIT_CINEMA, DKIT_TAPE, DKIT_FILM, DKIT_N\n"
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
    lines.append("typedef dub_plate_t triphop_loop_t;")
    lines.append("")
    lines.append(f'static const char *const TRIPHOP_ERA_NAMES[TRIPHOP_ERA_N] = {{')
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
        (HAT_NOTE, "triphop_hat_note", HAT_NOTE),
        (HAT_FM, "triphop_hat_fm", HAT_FM),
        (HAT_NOISE, "triphop_hat_noise", HAT_NOISE),
        (HAT_METAL, "triphop_hat_metal", HAT_METAL),
        (HAT_DECAY, "triphop_hat_decay", HAT_DECAY),
        (HAT_OPEN, "triphop_hat_open", HAT_OPEN),
        (HAT_STYLE, "triphop_hat_style", HAT_STYLE),
        (HAT_CUT, "triphop_hat_cut", HAT_CUT),
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
    lines.append("static const triphop_loop_t TRIPHOP_LOOP[TRIPHOP_LOOP_N] = {")
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
    lines.append(f"static const int8_t TRIPHOP_LOOP_ROOT[TRIPHOP_LOOP_N] = {{")
    lines.append(", ".join(roots))
    lines.append("};")
    lines.append(f"static const int8_t TRIPHOP_LOOP_SCALE[TRIPHOP_LOOP_N] = {{")
    lines.append(", ".join(scales))
    lines.append("};")
    lines.append(f"static const int8_t TRIPHOP_LOOP_SWING[TRIPHOP_LOOP_N] = {{")
    lines.append(", ".join(swings))
    lines.append("};")

    lead_rate = [str((pi % PRESETS_PER_ERA) - 3) for pi in range(PRESET_N)]
    lines.append(f"static const int8_t TRIPHOP_LOOP_LEAD_RATE[TRIPHOP_LOOP_N] = {{")
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
    lines.append("static const uint8_t TRIPHOP_ERA_MACROS[TRIPHOP_ERA_N][10] = {")
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
}} triphop_era_base_t;

typedef struct {{
    uint8_t drum_kit, tank, tap, loop_filt, feedback;
    uint8_t dly_lvl, rev_mix, rev_damp, rev_tail, dly_rev;
    uint8_t pump, bass_model, wobble;
}} triphop_era_fx_t;

static const triphop_era_base_t TRIPHOP_ERA_BASE[TRIPHOP_ERA_N] = {{
{era_base_lines}
}};

static const triphop_era_fx_t TRIPHOP_ERA_FX[TRIPHOP_ERA_N] = {{
    {{ DKIT_DUST, 28, 1, 42, 18, 58, 34, 32, 54, 52,  62, BASS_DARK,  14 }},
    {{ DKIT_VINYL,    24, 0, 46, 16, 64, 30, 28, 50, 48,  58, BASS_RHODES,   22 }},
    {{ DKIT_SMOKE, 16, 0, 50, 20, 52, 28, 22, 44, 72,  82, BASS_STRING,   8 }},
    {{ DKIT_STRINGS,   20, 0, 44, 14, 56, 36, 30, 52, 46,  54, BASS_TAPE,    18 }},
    {{ DKIT_DARK,  32, 2, 40, 14, 72, 42, 40, 62, 84,  88, BASS_SUBDEEP,   10 }},
    {{ DKIT_CINEMA,     18, 0, 48, 18, 54, 32, 24, 46, 58,  66, BASS_SMOKE,    26 }},
}};
""")

    # lead gates - simplified 32-bit patterns
    gates = []
    for pi in range(PRESET_N):
        g = mask32(100, pi // PRESETS_PER_ERA, pi % PRESETS_PER_ERA)
        gates.append(f"0x{g:08X}u")
    lines.append("static const uint32_t TRIPHOP_LOOP_LEAD_GATE[TRIPHOP_LOOP_N] = {")
    for i in range(0, PRESET_N, PRESETS_PER_ERA):
        lines.append("    " + ", ".join(gates[i : i + PRESETS_PER_ERA]) + ",")
    lines.append("};")

    kit_map = []
    for pi in range(PRESET_N):
        kit_map.append(
            str(DKIT_ERA[pi % len(DKIT_ERA)] if pi % 3 else (pi // PRESETS_PER_ERA) % ERA_N)
        )
    lines.append("static const int8_t TRIPHOP_LOOP_KIT[TRIPHOP_LOOP_N] = {")
    lines.append(", ".join(kit_map))
    lines.append("};")

    # Minimal lead sidecars (pitch/len/bend) - zeros + sparse
    lines.append("static const int8_t TRIPHOP_LOOP_LEAD_PITCH[TRIPHOP_LOOP_N][16] = {")
    for pi in range(PRESET_N):
        row = [0] * 16
        row[pi % 16] = (pi % 7) - 3
        if pi % 5 == 0:
            row[(pi + 3) % 16] = 2
        lines.append("    { " + ", ".join(f"{v:4d}" for v in row) + " },")
    lines.append("};")

    lines.append("static const uint8_t TRIPHOP_LOOP_LEAD_LEN[TRIPHOP_LOOP_N][32] = {")
    for pi in range(PRESET_N):
        row = [0] * 32
        row[pi % 32] = 9 + (pi % 4)
        lines.append("    { " + ", ".join(f"{v:3d}" for v in row) + " },")
    lines.append("};")

    lines.append("static const uint8_t TRIPHOP_LOOP_LEAD_BEND[TRIPHOP_LOOP_N][32] = {")
    for pi in range(PRESET_N):
        row = [0] * 32
        row[pi % 32] = 3 + (pi % 5)
        lines.append("    { " + ", ".join(f"{v:3d}" for v in row) + " },")
    lines.append("};")

    gate_cs = [str(28 + (pi // PRESETS_PER_ERA) * 2) for pi in range(PRESET_N)]
    lines.append("static const uint8_t TRIPHOP_LOOP_LEAD_GATE_CS[TRIPHOP_LOOP_N] = {")
    lines.append(", ".join(gate_cs))
    lines.append("};")
    lines.append("")
    lines.append("static const uint8_t TRIPHOP_LEAD_LEN_16THS[15] = {")
    lines.append("    1, 2, 4, 5, 8, 11, 12, 16, 20, 24, 28, 32, 40, 48, 64")
    lines.append("};")

    # Apply helpers - copy idm pattern with renames
    lines.append(open(SRC / "_triphop_apply_snippet.h").read() if (SRC / "_triphop_apply_snippet.h").exists() else "")
    lines.append("#endif")
    return "\n".join(lines)


def write_apply_snippet():
    snippet = r'''
#include "fm_pgen_core.h"

/* Live generated plate (endless continuum); shadows the ROM loop at triphop_gen.slot. */
static fm_pgen_t triphop_gen = { .slot = 0xFFu };
static fm_pgen_lead_t triphop_gen_lead;

#define FM_PGEN_STATE triphop_gen
#define FM_PGEN_BUILD(era, n, c) triphop_gen_build(era, n, c)

static inline uint8_t triphop_gen_hit(uint8_t ix)
{
    return fm_pgen_hit(&triphop_gen, ix);
}

static inline uint8_t triphop_clamp_loop(uint8_t ix)
{
    return ix >= TRIPHOP_LOOP_N ? (uint8_t)(TRIPHOP_LOOP_N - 1u) : ix;
}

static inline uint8_t triphop_clamp_era(uint8_t era)
{
    return era >= TRIPHOP_ERA_N ? (uint8_t)(TRIPHOP_ERA_N - 1u) : era;
}

static inline const char *triphop_era_name(uint8_t era)
{
    return TRIPHOP_ERA_NAMES[triphop_clamp_era(era)];
}

static inline const char *triphop_loop_name(uint8_t slab)
{
    if (triphop_gen_hit(slab))
        return triphop_gen.name;
    return TRIPHOP_LOOP[triphop_clamp_loop(slab)].name;
}

static inline const triphop_loop_t *triphop_loop(uint8_t ix)
{
    if (triphop_gen_hit(ix))
        return &triphop_gen.pl;
    return &TRIPHOP_LOOP[triphop_clamp_loop(ix)];
}

static inline uint8_t triphop_loop_root(uint8_t ix)
{
    return triphop_gen_hit(ix) ? (uint8_t)triphop_gen.root : (uint8_t)TRIPHOP_LOOP_ROOT[triphop_clamp_loop(ix)];
}

static inline uint8_t triphop_loop_scale(uint8_t ix)
{
    return triphop_gen_hit(ix) ? (uint8_t)triphop_gen.scale : (uint8_t)TRIPHOP_LOOP_SCALE[triphop_clamp_loop(ix)];
}

static inline uint8_t triphop_loop_swing(uint8_t ix)
{
    return triphop_gen_hit(ix) ? (uint8_t)triphop_gen.swing : (uint8_t)TRIPHOP_LOOP_SWING[triphop_clamp_loop(ix)];
}

static inline uint8_t triphop_loop_kit(uint8_t ix)
{
    return triphop_gen_hit(ix) ? (uint8_t)triphop_gen.kit : (uint8_t)TRIPHOP_LOOP_KIT[triphop_clamp_loop(ix)];
}

static inline int8_t triphop_loop_lead_rate(uint8_t ix)
{
    return triphop_gen_hit(ix) ? triphop_gen.rate : TRIPHOP_LOOP_LEAD_RATE[triphop_clamp_loop(ix)];
}

static inline uint8_t triphop_era_of_loop(uint8_t slab)
{
    slab = triphop_clamp_loop(slab);
    return (uint8_t)(slab / GENRE_PRESETS_PER_ERA);
}

static inline uint8_t triphop_era_first_loop(uint8_t era)
{
    return (uint8_t)(triphop_clamp_era(era) * GENRE_PRESETS_PER_ERA);
}

static inline uint32_t triphop_loop_lead_gate32(uint8_t slab)
{
    if (triphop_gen_hit(slab))
        return triphop_gen.lead_gate;
    return TRIPHOP_LOOP_LEAD_GATE[triphop_clamp_loop(slab)];
}

static inline uint8_t triphop_loop_lead_gate_cs(uint8_t slab)
{
    if (triphop_gen_hit(slab))
        return triphop_gen.gate_cs;
    return TRIPHOP_LOOP_LEAD_GATE_CS[triphop_clamp_loop(slab)];
}

static inline uint8_t triphop_loop_lead_step_bit(uint8_t slab, uint8_t step)
{
    uint32_t g32 = triphop_loop_lead_gate32(slab);
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

static inline int8_t triphop_loop_lead_pitch(uint8_t slab, uint8_t step)
{
    if (triphop_gen_hit(slab))
        return triphop_gen_lead.pitch[step & 15u];
    return TRIPHOP_LOOP_LEAD_PITCH[triphop_clamp_loop(slab)][step & 15u];
}

static inline uint8_t triphop_loop_lead_len_ix(uint8_t slab, uint8_t step)
{
    if (triphop_gen_hit(slab))
        return triphop_gen_lead.len[step & 31u];
    return TRIPHOP_LOOP_LEAD_LEN[triphop_clamp_loop(slab)][step & 31u];
}

static inline uint8_t triphop_loop_lead_bend_u7(uint8_t slab, uint8_t step)
{
    if (triphop_gen_hit(slab))
        return triphop_gen_lead.bend[step & 31u];
    return TRIPHOP_LOOP_LEAD_BEND[triphop_clamp_loop(slab)][step & 31u];
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

static inline void triphop_apply_defaults(dub_params_t *p)
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
    p->drum_kit = DKIT_DUST;
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

static inline void triphop_era_apply_base(dub_params_t *p, uint8_t era)
{
    const triphop_era_base_t *eb;
    const triphop_era_fx_t *fx;
    uint8_t i;

    if (!p)
        return;
    era = triphop_clamp_era(era);
    eb = &TRIPHOP_ERA_BASE[era];
    fx = &TRIPHOP_ERA_FX[era];
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

static inline void triphop_loop_apply(dub_params_t *p, uint8_t slab, uint8_t what)
{
    const triphop_loop_t *sc;
    uint8_t di;

    if (!p)
        return;
    slab = triphop_clamp_loop(slab);
    sc = triphop_loop(slab);
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        triphop_apply_defaults(p);
        triphop_era_apply_base(p, sc->era);
    }
    p->era = sc->era;
    p->plate = slab;
    if ((what & DUB_APPLY_ALL) == DUB_APPLY_ALL) {
        p->bpm = sc->bpm;
        p->density = sc->density;
        for (di = 0; di < 4u; di++)
            p->drm_dens[di] = sc->density;
        p->drum_kit = triphop_loop_kit(slab);
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
        p->root = triphop_loop_root(slab);
        p->scale = triphop_loop_scale(slab);
        p->swing = triphop_loop_swing(slab);
        {
            int16_t lead = (int16_t)p->siren_rate + (int16_t)triphop_loop_lead_rate(slab);

            if (lead < 0)
                lead = 0;
            else if (lead > 127)
                lead = 127;
            p->siren_rate = (uint8_t)lead;
        }
    }
}

static inline void triphop_preset_rebuild(dub_params_t *p, uint8_t era, uint8_t slab)
{
    if (!p)
        return;
    era = triphop_clamp_era(era);
    slab = triphop_clamp_loop(slab);
    triphop_loop_apply(p, slab, DUB_APPLY_ALL);
    p->era = era;
    p->plate = slab;
    p->drum_kit = triphop_loop_kit(slab);
    p->bass_model = triphop_loop(slab)->bass_model;
    p->voicing = triphop_loop(slab)->voicing;
    p->bpm = triphop_loop(slab)->bpm;
    p->pump = triphop_loop(slab)->pump;
    p->feedback = triphop_loop(slab)->feedback;
    p->wobble = triphop_loop(slab)->wobble;
    p->drum_pat = slab;
    p->root = triphop_loop_root(slab);
    p->scale = triphop_loop_scale(slab);
    p->swing = triphop_loop_swing(slab);
}

static inline uint8_t triphop_drm_density_allows(uint8_t density, uint8_t step, uint8_t voice_ix)
{
    return ((step * 19u + (voice_ix * 37u)) & 127u) < density;
}

static inline uint8_t triphop_gate_after_drm_density(uint8_t on, uint8_t density, uint8_t step, uint8_t voice_ix)
{
    if (!on)
        return 0u;
    return triphop_drm_density_allows(density, step, voice_ix);
}

static inline uint8_t dub_step_bit(uint32_t mask, uint8_t step)
{
    return (uint8_t)((mask >> (step & (DUB_STEPS - 1u))) & 1u);
}

static inline void dub_resolve_step(const dub_params_t *p, const dub_step_t *ovr, uint8_t step, dub_events_t *out, uint8_t apply_dens)
{
    const triphop_loop_t *drum;
    const triphop_loop_t *main;
    uint8_t kick, rim, hat, sub, chord, siren;
    uint8_t forced;

    if (!p || !out)
        return;
    if (step >= DUB_STEPS)
        step = (uint8_t)(DUB_STEPS - 1u);
    drum = triphop_loop(p->drum_pat);
    main = triphop_loop(p->plate);
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
        kick = triphop_gate_after_drm_density(kick, p->drm_dens[0], step, 0u);
        rim = triphop_gate_after_drm_density(rim, p->drm_dens[1], step, 1u);
        hat = triphop_gate_after_drm_density(hat, p->drm_dens[2], step, 2u);
        siren = triphop_gate_after_drm_density(siren, p->drm_dens[3], step, 3u);
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
    const triphop_loop_t *sc;

    plate = triphop_clamp_loop(plate);
    sc = triphop_loop(plate);
    if (root)
        *root = triphop_loop_root(plate);
    if (scale)
        *scale = triphop_loop_scale(plate);
    if (swing)
        *swing = triphop_loop_swing(plate);
    if (bpm)
        *bpm = sc->bpm;
}

#include "triphop_gen.h"
'''
    (SRC / "_triphop_apply_snippet.h").write_text(snippet)


def patch_triphop_dsp_layout(out: str) -> str:
    era_aliases = """
/* IDM-shared DSP paths use semantic era / bass aliases */
#define ERA_OBLIQ ERA_SMOKE
#define ERA_WARM ERA_DUST
#define ERA_GLITCH ERA_VINYL
#define ERA_MELO ERA_STRINGS
#define ERA_DRILL ERA_DARK
#define ERA_BREAK ERA_CINEMA
#define BASS_ROUND BASS_SUBDEEP
#define BASS_THROB BASS_VINYL
"""
    out = out.replace(
        '#include "triphop_data.h"\n#include "jazz_pluck.h"',
        '#include "triphop_data.h"\n#include "jazz_pluck.h"\n' + era_aliases.strip() + "\n",
        1,
    )
    out = re.sub(
        r"/\* --- Bass: soft \(WARM–GLITCH\).*?\n\}\n\n\nstatic inline int32_t triphop_bass_house_sub",
        "static inline int32_t triphop_bass_house_sub",
        out,
        count=1,
        flags=re.S,
    )
    out = out.replace("triphop_piece_lead_gate_cs", "triphop_loop_lead_gate_cs")
    extra_compat = """
#define idm_env_body_gate triphop_env_body_gate
#define idm_env_body triphop_env_body
#define idm_lead_timbre triphop_lead_timbre
#define idm_kit_k_note triphop_kit_k_note
#define idm_kit_k_sweep triphop_kit_k_sweep
#define idm_kit_k_fm triphop_kit_k_fm
#define idm_kit_k_drv triphop_kit_k_drv
#define idm_kit_r_note triphop_kit_r_note
#define idm_kit_r_wire triphop_kit_r_wire
#define idm_hat_glitch_note triphop_hat_glitch_note
#define idm_hat_glitch_fm triphop_hat_glitch_fm
#define idm_hat_glitch_noise triphop_hat_glitch_noise
#define idm_hat_glitch_metal triphop_hat_glitch_metal
#define idm_hat_glitch_style triphop_hat_glitch_style
#define idm_hat_glitch_cut triphop_hat_glitch_cut
#define idm_drm_kick_shape triphop_drm_kick_shape
#define idm_drm_rim_mix triphop_drm_rim_mix
#define idm_drm_bus triphop_drm_bus
#define idm_drm_room_send triphop_drm_room_send
#define IDM_ERA_MACROS TRIPHOP_ERA_MACROS
#define IDM_LEAD_LEN_N TRIPHOP_LEAD_LEN_N
#define IDM_LEAD_LEN_16THS TRIPHOP_LEAD_LEN_16THS
#define IDM_HAT_GLITCH_WASH TRIPHOP_HAT_GLITCH_WASH
#define IDM_HAT_GLITCH_BELL TRIPHOP_HAT_GLITCH_BELL
#define IDM_HAT_GLITCH_SWING TRIPHOP_HAT_GLITCH_SWING
#define IDM_HAT_GLITCH_ACID_HAT TRIPHOP_HAT_GLITCH_ACID_HAT
#define idm_lead_dly_boost triphop_lead_dly_boost
"""
    if "idm_env_body_gate triphop_env_body_gate" not in out:
        out = out.replace(
            "#if defined(GENRE_TRIPHOP)\n#define idm_stutter_t",
            "#if defined(GENRE_TRIPHOP)\n" + extra_compat.strip() + "\n#define idm_stutter_t",
            1,
        )
    return out


def patch_triphop_dsp_eras(out: str) -> str:
    """6-era lead/stutter/degrade + IDM compat macros (no BSUN/HOOL)."""
    sustain = """
static const triphop_lead_sustain_t TRIPHOP_LEAD_SUSTAIN[TRIPHOP_ERA_N] = {
    { 62, 4, 10, 30 }, /* DETROIT */
    { 68, 5, 11, 34 }, /* BELGIUM */
    { 42, 2,  8, 14 }, /* SCHRANZ */
    { 52, 4, 10, 22 }, /* HYPNO */
    { 58, 5, 12, 32 }, /* BERLIN */
    { 48, 3,  9, 18 }, /* EBM */
};
"""
    out = re.sub(
        r"static const triphop_lead_sustain_t TRIPHOP_LEAD_SUSTAIN\[TRIPHOP_ERA_N\] = \{.*?\};",
        sustain.strip(),
        out,
        count=1,
        flags=re.S,
    )
    timbre = """    static const uint8_t timbre[TRIPHOP_ERA_N] = {
        TRIPHOP_LEAD_EP, TRIPHOP_LEAD_SAX, TRIPHOP_LEAD_MUTE_TRP,
        TRIPHOP_LEAD_CLAV, TRIPHOP_LEAD_HORN, TRIPHOP_LEAD_FLUTE
    };"""
    out = re.sub(
        r"static const uint8_t timbre\[TRIPHOP_ERA_N\] = \{.*?\};",
        timbre.strip(),
        out,
        count=1,
        flags=re.S,
    )
    degrade_base = """    static const uint8_t base[TRIPHOP_ERA_N] = { 8, 10, 18, 22, 16, 12 };"""
    out = re.sub(
        r"static const uint8_t base\[TRIPHOP_ERA_N\] = \{.*?\};",
        degrade_base.strip(),
        out,
        count=1,
        flags=re.S,
    )
    stutter_fn = """
static inline uint16_t triphop_stutter_slice_from_vel(uint8_t vel, uint8_t era)
{
    uint16_t base = (uint16_t)(28u + ((uint32_t)vel * 88u) / 127u);
    uint8_t e = triphop_clamp_era(era);

    if (e == (uint8_t)ERA_SMOKE)
        base = (uint16_t)((base * 5u) / 8u);
    else if (e == (uint8_t)ERA_VINYL)
        base = (uint16_t)((base * 5u) / 4u);
    else if (e == (uint8_t)ERA_DARK)
        base = (uint16_t)((base * 3u) / 4u);
    else if (e == (uint8_t)ERA_CINEMA)
        base = (uint16_t)((base * 5u) / 6u);
    if (base < 6u)
        base = 6u;
    if (base > 512u)
        base = 512u;
    return base;
}
"""
    out = re.sub(
        r"static inline uint16_t triphop_stutter_slice_from_vel\(uint8_t vel, uint8_t era\)\n\{.*?\n\}\n",
        stutter_fn.strip() + "\n",
        out,
        count=1,
        flags=re.S,
    )
    lead_dly = """
static inline int32_t triphop_lead_dly_boost(const dub_params_t *p)
{
    (void)p;
    return 0;
}
"""
    if "triphop_lead_dly_boost" not in out:
        out = out.replace(
            "#if defined(GENRE_TRIPHOP)\n#define idm_env_body_gate",
            lead_dly.strip() + "\n\n#if defined(GENRE_TRIPHOP)\n#define idm_env_body_gate",
            1,
        )
    return out


def gen_dsp_h() -> None:
    idm = (SRC / "idm_dsp.h").read_text()
    out = idm
    repl = [
        ("IDM_DSP_H", "TRIPHOP_DSP_H"),
        ("triphop_data.h", "triphop_data.h"),
        ("idm_", "triphop_"),
        ("IDM_", "TRIPHOP_"),
        ("idm hat_glitch", "techno hat"),
    ]
    for a, b in repl:
        out = out.replace(a, b)
    # fix double triphop_snowcrash
    out = out.replace("triphop_", "triphop_")
    out = out.replace("SNOWCRASHTRIPHOP_", "TRIPHOP_")

    bass_block = r'''
static inline int32_t triphop_bass_house_sub(dub_synth_t *s, uint32_t inc, int32_t fc, uint8_t reso)
{
    int32_t raw;

    s->sub_ph += inc;
    raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph >> 1) >> 2);
    dub_svf_mode(raw, 600 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, reso, &raw);
    return raw;
}

static inline int32_t triphop_bass_render(dub_synth_t *s, const dub_params_t *p, uint32_t inc, int32_t fc,
                                       int32_t wob_all, int32_t lfo_mod_b, uint8_t model)
{
    int32_t raw = 0;
    int32_t walk = lfo_mod_b;

    switch (model) {
    case BASS_SUBDEEP: {
        s->sub_ph += inc >> 1;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph >> 1) >> 2);
        dub_svf_mode(raw, 280 + (fc >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        raw = (raw * (int32_t)(128u - (p->pump >> 1))) >> 7;
        break;
    }
    case BASS_VINYL: {
        s->sub_ph += inc;
        raw = dub_sine_q64(s->sub_ph) + (dub_sine_q64(s->sub_ph << 1) >> 2);
        dub_svf_mode(raw, 1100 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_SMOKE: {
        s->sub_ph += inc;
        raw = dub_saw(s->sub_ph) + (dub_square(s->sub_ph) >> 2);
        dub_svf_mode(raw, fc + (walk >> 1), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 12u), &raw);
        raw = (raw * (int32_t)s->sub_env) >> 14;
        break;
    }
    case BASS_RHODES: {
        uint32_t det = inc >> 4;
        s->sub_ph += inc;
        s->sub_ph2 += inc + det;
        s->sub_ph3 += inc - det;
        raw = dub_saw(s->sub_ph) + dub_saw(s->sub_ph2) + dub_saw(s->sub_ph3);
        dub_svf_mode(raw, fc + (wob_all >> 9), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 18u), &raw);
        break;
    }
    case BASS_WARM: {
        int32_t saw_w, acid_fc;
        s->sub_ph += inc;
        saw_w = dub_saw(s->sub_ph);
        raw = saw_w + (dub_square(s->sub_ph) >> 1);
        acid_fc = fc + (((s->sub_env >> 3) * (int32_t)p->wobble) >> 5);
        dub_svf_mode(raw, acid_fc, &s->sub_svf_l1, &s->sub_svf_l2,
                     (uint8_t)(p->reso_mode + 28u > 127u ? 127u : p->reso_mode + 28u), &raw);
        break;
    }
    case BASS_DARK: {
        int32_t mod2;
        s->sub_ph += inc;
        s->sub_ph2 += inc + (inc >> 2);
        mod2 = dub_sine_q64(s->sub_ph2);
        raw = dub_sine_q64(s->sub_ph + (uint32_t)(mod2 << 12));
        dub_svf_mode(raw, fc + (walk >> 2), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        raw = (raw * (int32_t)s->sub_env) >> 15;
        break;
    }
    case BASS_STRING: {
        s->sub_ph += inc;
        raw = dub_saw(s->sub_ph) + (dub_sine_q64(s->sub_ph) >> 2);
        dub_svf_mode(raw, 500 + (fc >> 1), &s->sub_svf_l1, &s->sub_svf_l2, (uint8_t)(p->reso_mode + 8u), &raw);
        raw = (raw * (int32_t)(96u + (p->drive >> 1))) >> 7;
        break;
    }
    case BASS_DOOM: {
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
    case BASS_TAPE: {
        int32_t mod2;
        s->sub_ph += inc;
        s->sub_ph2 += inc << 1;
        mod2 = dub_sine_q64(s->sub_ph2);
        raw = dub_sine_q64(s->sub_ph + (uint32_t)(mod2 << 13));
        dub_svf_mode(raw, fc + ((int32_t)p->wobble * 6), &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode, &raw);
        break;
    }
    case BASS_CINEMA: {
        s->sub_ph += inc >> 2;
        raw = dub_sine_q64(s->sub_ph) >> 1;
        raw += dub_sine_q64(s->sub_ph + (uint32_t)(p->wobble << 13)) >> 2;
        dub_svf_mode(raw, fc >> 1, &s->sub_svf_l1, &s->sub_svf_l2, p->reso_mode >> 1, &raw);
        break;
    }
    default:
        return triphop_bass_house_sub(s, inc, fc, p->reso_mode);
    }
    (void)p;
    return raw;
}

'''
    # Replace old bass render in generated file
    out = re.sub(
        r"/\* --- Bass:.*?\*/\nstatic inline int32_t triphop_bass_render\(.*?\n\}\n",
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
#if defined(GENRE_TRIPHOP)
#define idm_stutter_t triphop_stutter_t
#define idm_piece_lead_step_bit triphop_loop_lead_step_bit
#define idm_piece_lead_pitch triphop_loop_lead_pitch
#define idm_piece_lead_len_ix triphop_loop_lead_len_ix
#define idm_piece_lead_bend_u7 triphop_loop_lead_bend_u7
#define idm_lead_bend_plock triphop_lead_bend_plock
#define idm_lead_bend_semi triphop_lead_bend_semi
#define idm_lead_len_ix triphop_lead_len_ix
#define idm_fill_morph_steps triphop_fill_morph_steps
#define idm_stutter_edit_live triphop_stutter_edit_live
#define idm_u8_blend triphop_u8_blend
#define idm_fill_build_amt triphop_fill_build_amt
#define idm_stutter_edit_targets triphop_stutter_edit_targets
#define idm_fill_drop_targets triphop_fill_drop_targets
#define idm_apply_fill_break_voice triphop_apply_fill_break_voice
#define idm_stutter_edit_restore triphop_stutter_edit_restore
#define idm_stutter_start triphop_stutter_start
#define idm_stutter_release triphop_stutter_release
#define idm_stutter_bar triphop_stutter_bar
#define idm_edit_latch_begin triphop_edit_latch_begin
#define idm_stutter_latch_block triphop_stutter_latch_block
#define idm_rev_stutter_flip triphop_rev_stutter_flip
#define idm_glitch_burst_hit triphop_glitch_burst_hit
#define idm_stutter_sync_runtime triphop_stutter_sync_runtime
#define idm_edit_apply_drums triphop_edit_apply_drums
#define idm_stutter_process_buffer triphop_stutter_process_buffer
#define idm_loop_macro_sync triphop_loop_macro_sync
#define idm_lead_note_gate_piece triphop_lead_note_gate_piece
#define idm_siren_atk_samples triphop_siren_atk_samples
#define idm_siren_env_step triphop_siren_env_step
#define idm_lead_render triphop_lead_render
#define idm_lead_bent_inc triphop_lead_bent_inc
#define idm_lead_len_samples triphop_lead_len_samples
#define idm_stutter_push triphop_stutter_push
#define idm_stutter_mix triphop_stutter_mix
#define idm_stutter_slice_from_vel triphop_stutter_slice_from_vel
#define idm_degrade_amount triphop_degrade_amount
#define idm_degrade_sample triphop_degrade_sample
#define idm_hat_glitch_is_acid_kit triphop_hat_glitch_is_acid_kit
#define idm_hat_glitch_decay triphop_hat_glitch_decay
#define idm_hat_glitch_open triphop_hat_glitch_open
#define IDM_PIECE TRIPHOP_LOOP
#define IDM_PIECE_KIT TRIPHOP_LOOP_KIT
#define idm_era_first_piece triphop_era_first_loop
#define IDM_LEAD_LEN_NAMES TRIPHOP_LEAD_LEN_NAMES
#define IDM_STUT_HACK TRIPHOP_STUT_HACK
#define IDM_STUT_SCREW TRIPHOP_STUT_SCREW
/* IDM_FILL_MORPH_STEPS / IDM_STUTTER_MORPH_STEPS: defined in fm_app.c */
#endif
'''
    out = out.replace("#endif\n", compat + "\n#endif\n", 1)
    out = patch_triphop_dsp_layout(out)
    out = patch_triphop_dsp_eras(out)
    (SRC / "triphop_dsp.h").write_text(out)


def gen_views_section() -> str:
    """Generate FM_VIEWS rows from plate 16-bit masks (S2..S8 variants)."""
    # read plates from generated data - parse kick from file later; use algorithm
    lines = ["#elif defined(GENRE_TRIPHOP)", "#define FM_VIEW_PLATE_N TRIPHOP_LOOP_N"]
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
    if "GENRE_TRIPHOP" in text and "FM_VIEW_PLATE_N TRIPHOP_LOOP_N" in text:
        text = re.sub(r"#elif defined\(GENRE_TRIPHOP\).*?(?=#elif defined|#else\n#define FM_VIEW)", section + "\n", text, flags=re.S)
    else:
        text = text.replace("#else\n#define FM_VIEW_PLATE_N", section + "\n#else\n#define FM_VIEW_PLATE_N", 1)
    path.write_text(text)


def _apply_triphop_genre_gates(text: str) -> str:
    """Add GENRE_TRIPHOP to shared preprocessor gates only (never GENRE_TECHNO)."""
    _no_th = r"(?!\s*&&\s*!defined\(GENRE_TRIPHOP\))"
    subs = [
        (
            r"!defined\(GENRE_TECHNO\)\s*&&\s*!defined\(GENRE_BASS\)" + _no_th,
            "!defined(GENRE_TECHNO) && !defined(GENRE_TRIPHOP) && !defined(GENRE_BASS)",
        ),
        (
            r"!defined\(GENRE_FUNK\)\s*&&\s*!defined\(GENRE_TECHNO\)" + _no_th,
            "!defined(GENRE_FUNK) && !defined(GENRE_TECHNO) && !defined(GENRE_TRIPHOP)",
        ),
        (
            r"\|\|\s*defined\(GENRE_FUNK\)\s*\|\|\s*defined\(GENRE_TECHNO\)\s*\|\|\s*defined\(GENRE_BASS\)"
            r"(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "|| defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_BASS)",
        ),
        (
            r"\|\|\s*defined\(GENRE_TECHNO\)\s*\|\|\s*defined\(GENRE_BASS\)"
            r"(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "|| defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_BASS)",
        ),
        (
            r"\|\|\s*defined\(GENRE_TECHNO\)\s*\|\|\s*defined\(GENRE_JAZZ\)"
            r"(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "|| defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_JAZZ)",
        ),
        (
            r"\|\|\s*defined\(GENRE_POP\)\s*\|\|\s*defined\(GENRE_FUNK\)\s*\|\|\s*defined\(GENRE_TECHNO\)"
            r"(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "|| defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP)",
        ),
        (
            r"\|\|\s*defined\(GENRE_POP\)\s*\|\|\s*defined\(GENRE_FUNK\)"
            r"(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "|| defined(GENRE_POP) || defined(GENRE_FUNK) || defined(GENRE_TRIPHOP)",
        ),
        (
            r"defined\(GENRE_IDM\)\s*\|\|\s*defined\(GENRE_SNOWCRASH\)\s*\|\|\s*defined\(GENRE_TECHNO\)"
            r"\s*\|\|\s*defined\(GENRE_BASS\)\s*\|\|\s*defined\(GENRE_JAZZ\)"
            r"(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_BASS) || defined(GENRE_JAZZ)",
        ),
        (
            r"defined\(GENRE_IDM\)\s*\|\|\s*defined\(GENRE_SNOWCRASH\)\s*\|\|\s*defined\(GENRE_TECHNO\)"
            r"\s*\|\|\s*defined\(GENRE_BASS\)(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_BASS)",
        ),
        (
            r"defined\(GENRE_IDM\)\s*\|\|\s*defined\(GENRE_SNOWCRASH\)\s*\|\|\s*defined\(GENRE_TECHNO\)"
            r"(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP)",
        ),
        (
            r"defined\(GENRE_IDM\)\s*\|\|\s*defined\(GENRE_SNOWCRASH\)"
            r"(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)",
        ),
        (
            r"#elif defined\(GENRE_IDM\)\s*\|\|\s*defined\(GENRE_SNOWCRASH\)\s*\|\|\s*defined\(GENRE_TECHNO\)"
            r"\s*\|\|\s*defined\(GENRE_BASS\)(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_BASS)",
        ),
        (
            r"#elif defined\(GENRE_IDM\)\s*\|\|\s*defined\(GENRE_SNOWCRASH\)\s*\|\|\s*defined\(GENRE_TECHNO\)"
            r"(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP)",
        ),
        (
            r"#elif defined\(GENRE_IDM\)\s*\|\|\s*defined\(GENRE_SNOWCRASH\)"
            r"(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TRIPHOP)",
        ),
        (
            r"#if defined\(GENRE_IDM\)\s*\|\|\s*defined\(GENRE_SNOWCRASH\)\s*\|\|\s*defined\(GENRE_TECHNO\)"
            r"\s*\|\|\s*defined\(GENRE_BASS\)(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "#if defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_BASS)",
        ),
        (
            r"#if defined\(GENRE_HOUSE\)\s*\|\|\s*defined\(GENRE_FUNK\)\s*\|\|\s*defined\(GENRE_TECHNO\)"
            r"\s*\|\|\s*defined\(GENRE_BASS\)(?!\s*\|\|\s*defined\(GENRE_TRIPHOP\))",
            "#if defined(GENRE_HOUSE) || defined(GENRE_FUNK) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_BASS)",
        ),
    ]
    for pat, repl in subs:
        text = re.sub(pat, repl, text)
    while "|| defined(GENRE_TRIPHOP) || defined(GENRE_TRIPHOP)" in text:
        text = text.replace(
            "|| defined(GENRE_TRIPHOP) || defined(GENRE_TRIPHOP)",
            "|| defined(GENRE_TRIPHOP)",
        )
    while "&& !defined(GENRE_TRIPHOP) && !defined(GENRE_TRIPHOP)" in text:
        text = text.replace(
            "&& !defined(GENRE_TRIPHOP) && !defined(GENRE_TRIPHOP)",
            "&& !defined(GENRE_TRIPHOP)",
        )
    text = text.replace(
        "&& !defined(GENRE_TECHNO) && !defined(GENRE_TECHNO)",
        "&& !defined(GENRE_TECHNO)",
    )
    text = text.replace(
        "|| defined(GENRE_TECHNO) || defined(GENRE_TECHNO)",
        "|| defined(GENRE_TECHNO)",
    )
    return text


def patch_file_genre(path: Path) -> None:
    text = path.read_text()
    if path.name == "dub_dsp.h" and "triphop_dsp" in text:
        return
    new = _apply_triphop_genre_gates(text)
    if new == text:
        return
    path.write_text(new)


def write_triphop_conf() -> None:
    conf = """/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef TRIPHOP_CONF_H
#define TRIPHOP_CONF_H

#define GENRE_ERA_N      6u
#define GENRE_PRESETS_PER_ERA 10u
#define GENRE_PRESET_N   60u
#define GENRE_CFG_USER_PLATE_N 60u
#define GENRE_PRESET_N_LEGACY 48u
#define GENRE_CFG_MAGIC  0x54525031u /* TRP1 */
#define GENRE_APP_NAME   "KO TRIPHOP"
#define GENRE_FELUCCA_ID "FM-1_56412"

#define GENRE_PRESET_LABEL "TRACK"
#define GENRE_PRESET_HDR   "TRK"

#define GENRE_C_ACC RGB(210, 158, 108)

#define GENRE_BANK_COL \\
    { RGB(150, 115, 82), RGB(220, 168, 92), RGB(175, 95, 72), RGB(115, 98, 125), \\
      RGB(95, 118, 128), RGB(130, 145, 95) }

#define GENRE_ERA_COL \\
    { RGB(165, 120, 85), RGB(235, 175, 88), RGB(190, 88, 68), RGB(130, 105, 140), \\
      RGB(88, 112, 122), RGB(118, 138, 92) }

#endif
"""
    (SRC / "triphop_conf.h").write_text(conf)


def patch_genre_conf_pack() -> None:
    conf = SRC / "genre_conf.h"
    t = conf.read_text()
    if "GENRE_TRIPHOP" not in t:
        t = t.replace(
            "#elif defined(GENRE_TECHNO)\n#include \"techno_conf.h\"",
            "#elif defined(GENRE_TECHNO)\n#include \"techno_conf.h\"\n#elif defined(GENRE_TRIPHOP)\n#include \"triphop_conf.h\"",
        )
        conf.write_text(t)
    pack = SRC / "genre_pack.h"
    t = pack.read_text()
    if "triphop_clamp_loop" not in t:
        block = r'''#elif defined(GENRE_TRIPHOP)
#include "triphop_data.h"
'''
        t = t.replace(
            '#elif defined(GENRE_TECHNO)\n#include "techno_data.h"',
            '#elif defined(GENRE_TECHNO)\n#include "techno_data.h"\n' + block,
        )
        macros = r'''#elif defined(GENRE_TRIPHOP)
#define genre_clamp_era(era)       triphop_clamp_era(era)
#define genre_clamp_preset(ix)     triphop_clamp_loop(ix)
#define genre_era_name(era)        triphop_era_name(era)
#define genre_preset_name(ix)      triphop_loop_name(ix)
#define genre_preset(ix)           triphop_loop(ix)
#define genre_era_of_preset(pl)    triphop_era_of_loop(pl)
#define genre_era_first_preset(er) triphop_era_first_loop(er)
#define genre_preset_rebuild(p, era, pl) triphop_preset_rebuild(p, era, pl)
#define genre_plate_apply(p, pl, what)   triphop_loop_apply(p, pl, what)
#define genre_era_apply_base(p, era)     triphop_era_apply_base(p, era)
#define genre_apply_defaults(p)          triphop_apply_defaults(p)
'''
        t = t.replace(
            "#define genre_apply_defaults(p)          techno_apply_defaults(p)\n#elif defined(GENRE_TRIPHOP)\n#define genre_clamp_era(era)       triphop_clamp_era(era)\n#define genre_clamp_preset(ix)     triphop_clamp_loop(ix)\n#define genre_era_name(era)        triphop_era_name(era)\n#define genre_preset_name(ix)      triphop_loop_name(ix)\n#define genre_preset(ix)           triphop_loop(ix)\n#define genre_era_of_preset(pl)    triphop_era_of_loop(pl)\n#define genre_era_first_preset(er) triphop_era_first_loop(er)\n#define genre_preset_rebuild(p, era, pl) triphop_preset_rebuild(p, era, pl)\n#define genre_plate_apply(p, pl, what)   triphop_loop_apply(p, pl, what)\n#define genre_era_apply_base(p, era)     triphop_era_apply_base(p, era)\n#define genre_apply_defaults(p)          triphop_apply_defaults(p)\n#elif defined(GENRE_FUNK)",
            "#define genre_apply_defaults(p)          snowcrash_apply_defaults(p)\n" + macros + "#elif defined(GENRE_FUNK)",
        )
        pack.write_text(t)
    t = pack.read_text()
    if "GENRE_TRIPHOP)" not in t.split("genre_lead_step_bit")[0]:
        t = t.replace(
            "#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH)",
            "#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP)",
            2,
        )
        t = t.replace(
            "#elif defined(GENRE_SNOWCRASH)\n#define genre_lead_gate32(b)          snowcrash_slab_lead_gate32(b)",
            "#elif defined(GENRE_SNOWCRASH)\n#define genre_lead_gate32(b)          snowcrash_slab_lead_gate32(b)\n#elif defined(GENRE_TRIPHOP)\n#define genre_lead_gate32(b)          triphop_loop_lead_gate32(b)",
        )
        pack.write_text(t)


def patch_dub_dsp_include() -> None:
    path = SRC / "dub_dsp.h"
    t = path.read_text()
    if "triphop_dsp" not in t:
        t = t.replace(
            "#elif defined(GENRE_TECHNO)\n#include \"techno_dsp.h\"",
            "#elif defined(GENRE_TECHNO)\n#include \"techno_dsp.h\"\n#elif defined(GENRE_TRIPHOP)\n#include \"triphop_dsp.h\"",
        )
        t = t.replace(
            "raw = techno_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);",
            "raw = techno_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);\n#elif defined(GENRE_TRIPHOP)\n                raw = triphop_bass_render(s, p, inc, fc, wob_all, lfo_mod_b, model);",
        )
        t = t.replace(
            "#elif defined(GENRE_TECHNO) || defined(GENRE_SNOWCRASH) || defined(GENRE_IDM)\n    static const int8_t tab[VOICE_N][4]",
            "#elif defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_SNOWCRASH) || defined(GENRE_IDM)\n    static const int8_t tab[VOICE_N][4]",
        )
        path.write_text(t)


def patch_fm_app_triphop_ui() -> None:
    path = SRC / "fm_app.c"
    t = path.read_text()
    if "TRIPHOP_PLATE_SEQ" in t:
        return
    techno_magic = """#elif defined(GENRE_TECHNO)
#define DUB_CFG_MAGIC_S16_PREV48 0x54454330u
#define DUB_CFG_MAGIC_S16_PREV   0x54454331u
#define DUB_CFG_MAGIC_PREV       0xD4544331u
#elif defined(GENRE_TRIPHOP)
#define DUB_CFG_MAGIC_S16_PREV48 0x54525030u
#define DUB_CFG_MAGIC_S16_PREV   0x54525031u
#define DUB_CFG_MAGIC_PREV       0xD4544332u"""
    t = re.sub(
        r"#elif defined\(GENRE_TECHNO\)\n"
        r"#define DUB_CFG_MAGIC_S16_PREV48 0x54454330u\n"
        r"(?:#define DUB_CFG_MAGIC_S16_PREV[^\n]*\n)?"
        r"(?:#define DUB_CFG_MAGIC_PREV[^\n]*\n)?"
        r"(?:#elif defined\(GENRE_TRIPHOP\)\n"
        r"#define DUB_CFG_MAGIC_S16_PREV48 0x54525030u\n"
        r"#define DUB_CFG_MAGIC_S16_PREV   0x54525031u\n"
        r"#define DUB_CFG_MAGIC_PREV       0xD4544332u\n)?"
        r"(?:#elif defined\(GENRE_TRIPHOP\)\n.*?"
        r"#define DUB_CFG_MAGIC_PREV[^\n]*\n)?",
        techno_magic + "\n",
        t,
        count=1,
        flags=re.S,
    )
    m = re.search(
        r"#elif defined\(GENRE_TECHNO\)\nstatic const dub_plate_seq_t TECHNO_PLATE_SEQ\[GENRE_PRESET_N\] = \{.*?\n\};",
        t,
        re.S,
    )
    if m:
        tc_block = m.group(0)
        th_block = tc_block.replace("GENRE_TECHNO", "GENRE_TRIPHOP").replace(
            "TECHNO_PLATE_SEQ", "TRIPHOP_PLATE_SEQ"
        )
        t = t.replace(tc_block, tc_block + "\n" + th_block)
    t = t.replace(
        "#elif defined(GENRE_TECHNO)\n    return &TECHNO_PLATE_SEQ[plate];",
        "#elif defined(GENRE_TECHNO)\n    return &TECHNO_PLATE_SEQ[plate];\n#elif defined(GENRE_TRIPHOP)\n    return &TRIPHOP_PLATE_SEQ[plate];",
    )
    t = t.replace(
        '#elif defined(GENRE_TECHNO)\n#include "techno_draw.h"',
        '#elif defined(GENRE_TECHNO)\n#include "techno_draw.h"\n#elif defined(GENRE_TRIPHOP)\n#include "triphop_draw.h"',
    )
    t = t.replace(
        "#elif defined(GENRE_TECHNO)\nstatic const char *const SEQ_LANE[6] = {\"KICK\", \"SNAR\", \"HAT\", \"RAVE\", \"BASS\", \"CHORD\"};",
        "#elif defined(GENRE_TECHNO)\nstatic const char *const SEQ_LANE[6] = {\"KICK\", \"SNAR\", \"HAT\", \"RAVE\", \"BASS\", \"CHORD\"};\n#elif defined(GENRE_TRIPHOP)\nstatic const char *const SEQ_LANE[6] = {\"KICK\", \"SNAR\", \"HAT\", \"PAD\", \"BASS\", \"CHORD\"};",
    )
    t = t.replace(
        "#elif defined(GENRE_TECHNO)\n    \"PUMP\",   \"RUMBLE\", \"HOVER\",  \"TANK\",   \"STEEL\",  \"DIST\",   \"HARD\",   \"GATE\",",
        "#elif defined(GENRE_TECHNO)\n    \"PUMP\",   \"RUMBLE\", \"HOVER\",  \"TANK\",   \"STEEL\",  \"DIST\",   \"HARD\",   \"GATE\",\n#elif defined(GENRE_TRIPHOP)\n    \"DUST\",   \"VINYL\",  \"SMOKE\",  \"PAD\",    \"SUB\",    \"TAPE\",   \"DARK\",   \"FILM\",",
    )
    t = t.replace(
        "#elif defined(GENRE_TECHNO)\n    \"PHASE\",  \"DRIVE\",",
        "#elif defined(GENRE_TECHNO)\n    \"PHASE\",  \"DRIVE\",\n#elif defined(GENRE_TRIPHOP)\n    \"WARP\",   \"HAZE\",",
    )
    t = t.replace(
        "#elif defined(GENRE_TECHNO)\n    \"RIG\",",
        "#elif defined(GENRE_TECHNO)\n    \"RIG\",\n#elif defined(GENRE_TRIPHOP)\n    \"MOOD\",",
    )
    t = t.replace(
        "|| defined(GENRE_TECHNO) || defined(GENRE_JAZZ)",
        "|| defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP) || defined(GENRE_JAZZ)",
    )
    t = t.replace(
        "#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO)\n    return idm_stutter_edit_live();",
        "#elif defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_TECHNO) || defined(GENRE_TRIPHOP)\n    return idm_stutter_edit_live();",
    )
    if 'L0[4] = {"DUST", "VINYL", "SMOKE", "PAD"}' not in t:
        t = t.replace(
            '#elif defined(GENRE_TECHNO)\n        static const char *const L0[4] = {"PUMP", "RUMBLE", "HOVER", "TANK"};\n'
            '        static const char *const L1[4] = {"STEEL", "DIST", "HARD", "GATE"};\n'
            '#else\n        static const char *const L0[4] = {"SKANK", "LAYER", "WARP", "SWEEP"};',
            '#elif defined(GENRE_TECHNO)\n        static const char *const L0[4] = {"PUMP", "RUMBLE", "HOVER", "TANK"};\n'
            '        static const char *const L1[4] = {"STEEL", "DIST", "HARD", "GATE"};\n'
            '#elif defined(GENRE_TRIPHOP)\n        static const char *const L0[4] = {"DUST", "VINYL", "SMOKE", "PAD"};\n'
            '        static const char *const L1[4] = {"SUB", "TAPE", "DARK", "FILM"};\n'
            '#else\n        static const char *const L0[4] = {"SKANK", "LAYER", "WARP", "SWEEP"};',
        )
        t = t.replace(
            '#elif defined(GENRE_TECHNO)\n        static const char *const L2[4] = {"PHASE", "DRIVE", "RIG", "INVERT"};\n'
            '#else\n        static const char *const L2[4] = {"MOTION", "FLUX", "SYSTEM", "INVERT"};',
            '#elif defined(GENRE_TECHNO)\n        static const char *const L2[4] = {"PHASE", "DRIVE", "RIG", "INVERT"};\n'
            '#elif defined(GENRE_TRIPHOP)\n        static const char *const L2[4] = {"WARP", "HAZE", "MOOD", "INVERT"};\n'
            '#else\n        static const char *const L2[4] = {"MOTION", "FLUX", "SYSTEM", "INVERT"};',
        )
    path.write_text(t)


def patch_fm_seq_triphop() -> None:
    path = SRC / "fm_seq.h"
    t = path.read_text()
    if "GENRE_TRIPHOP" in t:
        return
    t = t.replace(
        "#elif defined(GENRE_TECHNO)\nstatic const char *const FM_SEQ_KNOB_P4[4] = {\"CYCLE\", \"REPEAT\", \"RIG\", \"INVERT\"};",
        "#elif defined(GENRE_TECHNO)\nstatic const char *const FM_SEQ_KNOB_P4[4] = {\"CYCLE\", \"REPEAT\", \"RIG\", \"INVERT\"};\n#elif defined(GENRE_TRIPHOP)\nstatic const char *const FM_SEQ_KNOB_P4[4] = {\"CYCLE\", \"REPEAT\", \"MOOD\", \"INVERT\"};",
    )
    path.write_text(t)


def patch_gen_views_py() -> None:
    path = ROOT / "tools" / "gen_views.py"
    t = path.read_text()
    if '"TRIPHOP"' in t:
        return
    entry = (
        '    ("TRIPHOP", "triphop_data.h", "TRIPHOP_LOOP", "TRIPHOP_LOOP_N", "triphop_conf.h", "GENRE_TRIPHOP"),\n'
    )
    t = t.replace(
        '    ("TECHNO", "techno_data.h", "TECHNO_LOOP", "TECHNO_LOOP_N", "techno_conf.h", "GENRE_TECHNO"),\n',
        '    ("TECHNO", "techno_data.h", "TECHNO_LOOP", "TECHNO_LOOP_N", "techno_conf.h", "GENRE_TECHNO"),\n'
        + entry,
    )
    t = t.replace(
        '    "TECHNO": "TECHNO_LOOP_LEAD_GATE",\n',
        '    "TECHNO": "TECHNO_LOOP_LEAD_GATE",\n    "TRIPHOP": "TRIPHOP_LOOP_LEAD_GATE",\n',
    )
    path.write_text(t)


def main() -> None:
    write_triphop_conf()
    write_apply_snippet()
    (SRC / "triphop_data.h").write_text(gen_data_h())
    gen_dsp_h()
    patch_gen_views_py()
    patch_fm_views(gen_views_section())
    for f in ["dub_dsp.h", "dub_types.h", "fm_app.c", "fm_seq.h"]:
        patch_file_genre(SRC / f)
    patch_genre_conf_pack()
    patch_dub_dsp_include()
    patch_fm_app_triphop_ui()
    patch_fm_seq_triphop()
    draw = (SRC / "idm_draw.h").read_text()
    draw = draw.replace("IDM_DRAW_H", "TRIPHOP_DRAW_H").replace("idm_", "triphop_").replace("IDM", "TRIPHOP")
    draw = draw.replace("GENRE_IDM", "GENRE_TRIPHOP")
    draw = draw.replace("triphop_piece_t", "triphop_loop_t")
    draw = draw.replace("triphop_piece(", "triphop_loop(")
    draw = draw.replace('"LEAD"', '"PAD"', 1)
    (SRC / "triphop_draw.h").write_text(draw)
    (SRC / "_triphop_apply_snippet.h").unlink(missing_ok=True)
    import subprocess

    subprocess.run([sys.executable, str(ROOT / "tools" / "gen_views.py")], check=True, cwd=ROOT)
    print("triphop pack generated")


if __name__ == "__main__":
    main()
