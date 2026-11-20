#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Regenerate HEAVY_PSYCH_TRAX_LEAD_GATE/PITCH/LEN/BEND: keep bcf4c42 melody, era riffs in gaps."""
from __future__ import annotations

PRESETS_PER_ERA = 10

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "firmware/src/heavy_psych_data.h"
BASE_REV = "bcf4c42"
TRAX_N = 60
ERA_N = 6
STONER, DESERT, SPACE, DRONE, GAZE, DOOM = range(6)
SEQ_STEPS = 32
FILL_STEPS = frozenset(range(28, 32))
# HP_LEAD_LEN ix → sixteenth note length (matches HP_LEAD_LEN_16THS in heavy_psych_data.h)
LEN_16THS = (1, 2, 4, 5, 8, 11, 12, 16, 20, 24, 28, 32, 40, 48, 64)
LONG_LEN_IX_MIN = 9  # 3/2 (24 sixteenths) and longer
MED_LEN_IX = 6  # 3/4 bar (12 sixteenths)
LEN_IX_MAX = len(LEN_16THS) - 1
# Era bias: extra LEN ix on sustained / doom-ish material (DOOM, DRONE, …).
ERA_LEN_BIAS = (0, 0, 1, 2, 0, 2)
MAX_MELODY_JUMP = 4  # max |Δ| semitone between consecutive gated steps
PITCH_MIN = -5
PITCH_MAX = 4
PITCH_BIAS = -3  # band register — lower vs chord root

# Semitone pools before PITCH_BIAS (rock: pent / blues / natural minor).
ROCK_PENT = (0, 2, 3, 5, -2, -3)
ROCK_BLUES = (0, 2, 3, 5, 7, -2, -3, -5)
ROCK_MINOR = (0, 2, 3, 5, -2, -3, 7)
CHROMATIC_AVOID = frozenset({1, -1, 4, -4, 6, -6})

# Rock phrase on 32 timeline steps: bar4=0 call (T1), bar4=1 answer; T3/T4 = beats 3–4 of bar4=1.
T1 = (0, (0, 2, 4, 6, 8, 10))
T3 = (1, (4, 5, 6))
T4 = (1, (12, 13, 14, 15))


def mix32(x: int) -> int:
    x &= 0xFFFFFFFF
    x ^= x >> 16
    x = (x * 0x7FEB352D) & 0xFFFFFFFF
    x ^= x >> 15
    x = (x * 0x846CA68B) & 0xFFFFFFFF
    return x ^ (x >> 16)


def parse_gate_pitch(text: str) -> tuple[list[int], list[list[int]]]:
    m = re.search(r"HEAVY_PSYCH_TRAX_LEAD_GATE\[.*?\]\s*=\s*\{(.*?)\};", text, re.S)
    gates = [int(x.rstrip("uU"), 0) for x in re.findall(r"0x[0-9A-Fa-f]+u?", m.group(1))]
    m2 = re.search(r"HEAVY_PSYCH_TRAX_LEAD_PITCH\[.*?\]\s*=\s*\{(.*?)\};", text, re.S)
    rows: list[list[int]] = []
    for line in m2.group(1).split("\n"):
        nums = [int(x.strip().rstrip(",")) for x in re.findall(r"-?\d+", line)]
        if len(nums) == 16:
            rows.append(nums)
    return gates, rows


def step_bit(g32: int, step: int) -> int:
    bar4 = (step >> 4) & 3
    s16 = step & 15
    if bar4 in (0, 2):
        return (g32 >> s16) & 1
    if bar4 == 1:
        return (g32 >> (16 + s16)) & 1
    if s16 < 8:
        return 0
    return (g32 >> (16 + s16)) & 1


def set_step_bit(g32: int, step: int, on: int) -> int:
    bar4 = (step >> 4) & 3
    s16 = step & 15
    if bar4 in (0, 2):
        sh = s16
    elif bar4 == 1:
        sh = 16 + s16
    else:
        if s16 < 8:
            return g32
        sh = 16 + s16
    if on:
        return g32 | (1 << sh)
    return g32 & ~(1 << sh)


# Era riff character: candidate zones and semitone pools (relative tune, like existing table).
ERA_RIFF = {
    STONER: {
        "zones": (T1, T4),
        "pool": ROCK_PENT + (0, 5),
        "density": 2,
    },
    DESERT: {
        "zones": (T1, (1, (8, 11)), T4),
        "pool": ROCK_MINOR,
        "density": 2,
    },
    SPACE: {
        "zones": (T1, (0, (14,)), (1, (15,))),
        "pool": ROCK_PENT,
        "density": 1,
    },
    DRONE: {
        "zones": ((0, (0, 15)), (1, (15,))),
        "pool": (0, 0, -3, -5, 2, 3),
        "density": 1,
    },
    GAZE: {
        "zones": (T1, T3, (1, (10, 14))),
        "pool": ROCK_BLUES,
        "density": 2,
    },
    DOOM: {
        "zones": ((0, (0,)), (1, (8,)), T4),
        "pool": ROCK_MINOR + (-5,),
        "density": 1,
    },
}


def era_rock_pool(era: int) -> tuple[int, ...]:
    return ERA_RIFF[era]["pool"]


def snap_to_rock(p: int, era: int, prev: int | None) -> int:
    pool = era_rock_pool(era)
    if prev is not None:
        candidates = [v for v in pool if abs(v - prev) <= MAX_MELODY_JUMP]
        if not candidates:
            candidates = list(pool)
        return min(candidates, key=lambda v: (abs(v - p), abs(v - prev)))
    return min(pool, key=lambda v: abs(v - p))


def era_name(era: int) -> str:
    return ("STONER", "DESERT", "SPACE", "DRONE", "GAZE", "DOOM")[era]


def candidates_for_era(era: int, trax: int) -> list[tuple[int, int]]:
    spec = ERA_RIFF[era]
    slot = trax % PRESETS_PER_ERA
    seed = mix32((era << 8) | (slot << 4) | 0xA51F)
    out: list[tuple[int, int]] = []
    for zone in spec["zones"]:
        bar4, steps = zone
        for s16 in steps:
            out.append((bar4, s16))
    # Deterministic shuffle / pick by era+slot, not trax%7.
    keyed = [(mix32(seed ^ (bar4 << 8) ^ s16), bar4, s16) for bar4, s16 in out]
    keyed.sort()
    take = spec["density"] + (slot & 1)
    picked = [(b, s) for _, b, s in keyed[: take + 2]]
    return picked


def pitch_for_riff(era: int, trax: int, s16: int, pi: int, prev: int | None) -> int:
    pool = ERA_RIFF[era]["pool"]
    slot = trax % PRESETS_PER_ERA
    i = (mix32((era + 1) * 0x9E3779B9 ^ (trax << 4) ^ (s16 << 2)) + pi) % len(pool)
    v = snap_to_rock(pool[i], era, prev)
    if v < PITCH_MIN:
        v = PITCH_MIN
    if v > PITCH_MAX:
        v = PITCH_MAX
    return v


def smooth_trax_pitches(trax: int, g32: int, pitches: list[list[int]]) -> None:
    prev: int | None = None
    for step in range(SEQ_STEPS):
        if not step_bit(g32, step):
            continue
        s16 = step & 15
        era = trax // PRESETS_PER_ERA
        p = pitches[trax][s16]
        if p < PITCH_MIN:
            p = PITCH_MIN
        elif p > PITCH_MAX:
            p = PITCH_MAX
        p = snap_to_rock(p, era, prev)
        if prev is not None:
            if p - prev > MAX_MELODY_JUMP:
                p = prev + MAX_MELODY_JUMP
            elif p - prev < -MAX_MELODY_JUMP:
                p = prev - MAX_MELODY_JUMP
        p += PITCH_BIAS
        if p < PITCH_MIN:
            p = PITCH_MIN
        elif p > PITCH_MAX:
            p = PITCH_MAX
        pitches[trax][s16] = p
        prev = p


def s16_used_in_orig(g32: int, s16: int) -> bool:
    if (g32 >> s16) & 1:
        return True
    if (g32 >> (16 + s16)) & 1:
        return True
    return False


def pitch_at(trax: int, step: int, pitches: list[list[int]]) -> int:
    return pitches[trax][step & 15]


def next_gated_pitch_step(
    trax: int, from_step: int, g32: int, pitches: list[list[int]]
) -> tuple[int | None, int | None]:
    """Next gated step after from_step (wrap) with a pitch; (step, pitch) or (None, None)."""
    for k in range(1, SEQ_STEPS + 1):
        ns = (from_step + k) % SEQ_STEPS
        if not step_bit(g32, ns):
            continue
        return ns, pitch_at(trax, ns, pitches)
    return None, None


def hold_run_len(trax: int, step: int, g32: int, pitches: list[list[int]]) -> int:
    if not step_bit(g32, step):
        return 0
    p0 = pitch_at(trax, step, pitches)
    run = 1
    for k in range(1, SEQ_STEPS):
        ns = step + k
        if ns >= SEQ_STEPS:
            break
        if not step_bit(g32, ns):
            break
        if pitch_at(trax, ns, pitches) != p0:
            break
        run += 1
    return run


def sixteenths_to_len_ix(n: int) -> int:
    ix = 0
    for i, s in enumerate(LEN_16THS):
        if s <= n:
            ix = i
    return ix


def rock_single_hit_len_ix(trax: int, step: int, era: int) -> int:
    """One gated 16th: mix staccato, medium, and accent lengths (full 1/16…3/2 palette)."""
    v = mix32((trax << 6) ^ (step << 2) ^ (era << 10) ^ 0x51E0)
    bucket = v % 100
    if bucket < 32:
        ix = (v >> 8) % 3  # 1/16 … 1/4
    elif bucket < 68:
        ix = 3 + ((v >> 10) % 4)  # 5/16 … 3/4
    else:
        ix = 7 + ((v >> 12) % 3)  # 1/1 … 3/2
    if (step & 15) == 0:
        ix = min(ix + 2, LEN_IX_MAX)
    elif (step & 15) in (4, 8, 12):
        ix = min(ix + 1, LEN_IX_MAX)
    return ix


def len_ix_for_step(trax: int, step: int, g32: int, pitches: list[list[int]]) -> int:
    if not step_bit(g32, step):
        return 0
    era = trax // PRESETS_PER_ERA
    run = hold_run_len(trax, step, g32, pitches)
    v = mix32((trax << 5) ^ (step << 3) ^ 0xA7C3)
    if run <= 1:
        ix = rock_single_hit_len_ix(trax, step, era)
    else:
        ix = sixteenths_to_len_ix(run)
        if run >= 3:
            ix = max(ix, MED_LEN_IX - 1 + (v & 1))
        if run >= 5:
            ix = max(ix, 7 + (v % 3))  # 1/1 … 3/2
        if run >= 8:
            ix = max(ix, LONG_LEN_IX_MIN + (v % 2))
        if run >= 12:
            ix = max(ix, 11 + (v % 3))  # 2/1 … 5/2
    ix += ERA_LEN_BIAS[era]
    if step in FILL_STEPS:
        ix = max(ix, 10 + (v % 4))
    if (v & 15) == 0 and ix > 0:
        ix = min(ix + 1, LEN_IX_MAX)
    return min(max(ix, 0), LEN_IX_MAX)


def bend_for_step(trax: int, step: int, g32: int, pitches: list[list[int]]) -> int:
    if not step_bit(g32, step):
        return 0
    cur = pitch_at(trax, step, pitches)
    _ns, nxt = next_gated_pitch_step(trax, step, g32, pitches)
    if nxt is None or nxt == cur:
        return 0
    d = nxt - cur
    ad = abs(d)
    if ad < 3:
        return 0
    return min(12, ad)


def legato_len_floor(gap: int, semitones: int) -> int:
    """Target LEN ix from spacing to next gated note (overlap, not all 3/2)."""
    ad = abs(semitones)
    if gap <= 1:
        return 7 if ad <= 2 else 6
    if gap == 2:
        return 8 if ad <= 2 else 7
    if gap == 3:
        return 9 if ad <= MAX_MELODY_JUMP else 8
    if gap == 4:
        return 10 if ad <= 2 else 9
    return 0


def boost_melodic_legato_lens(
    trax: int, g32: int, pitches: list[list[int]], lens: list[list[int]]
) -> None:
    """Longer gates when the next note is stepwise — overlap → legato."""
    for step in range(SEQ_STEPS):
        if not step_bit(g32, step):
            continue
        cur = pitch_at(trax, step, pitches)
        ns, nxt = next_gated_pitch_step(trax, step, g32, pitches)
        if ns is None:
            continue
        gap = (ns - step) if ns > step else (SEQ_STEPS - step + ns)
        if gap > 4:
            continue
        if abs(nxt - cur) > MAX_MELODY_JUMP:
            continue
        floor_ix = legato_len_floor(gap, nxt - cur)
        if floor_ix:
            lens[trax][step] = min(max(lens[trax][step], floor_ix), LEN_IX_MAX)


def build_len_bend(
    gates: list[int], pitches: list[list[int]]
) -> tuple[list[list[int]], list[list[int]]]:
    lens = [[0] * SEQ_STEPS for _ in range(TRAX_N)]
    bends = [[0] * SEQ_STEPS for _ in range(TRAX_N)]
    for trax in range(TRAX_N):
        g = gates[trax]
        for step in range(SEQ_STEPS):
            lens[trax][step] = len_ix_for_step(trax, step, g, pitches)
            bends[trax][step] = bend_for_step(trax, step, g, pitches)
        boost_melodic_legato_lens(trax, g, pitches, lens)
    return lens, bends


def melody_steps_lost(orig_g: list[int], cur_g: list[int]) -> list[tuple[int, int]]:
    lost: list[tuple[int, int]] = []
    for trax in range(TRAX_N):
        for step in range(SEQ_STEPS):
            if step_bit(orig_g[trax], step) and not step_bit(cur_g[trax], step):
                lost.append((trax, step))
    return lost


def build() -> tuple[list[int], list[list[int]], list[list[int]], list[list[int]]]:
    base = subprocess.check_output(["git", "show", f"{BASE_REV}:firmware/src/heavy_psych_data.h"], text=True)
    og, op = parse_gate_pitch(base)
    gates = list(og)
    pitches = [row[:] for row in op]
    if len(gates) == 48 and TRAX_N == 60:
        g2, p2 = [], []
        for e in range(len(gates) // 8):
            chunk_g = gates[e * 8 : (e + 1) * 8]
            chunk_p = pitches[e * 8 : (e + 1) * 8]
            g2.extend(chunk_g)
            p2.extend(chunk_p)
            g2.extend([chunk_g[6], chunk_g[7]])
            p2.extend([chunk_p[6][:], chunk_p[7][:]])
        gates, pitches = g2, p2
        og = list(gates)

    for trax in range(TRAX_N):
        era = trax // PRESETS_PER_ERA
        g = gates[trax]
        added = 0
        riff_prev: int | None = None
        for bar4, s16 in candidates_for_era(era, trax):
            step = (bar4 << 4) | s16
            if step >= 32:
                continue
            if step_bit(g, step):
                continue
            if step_bit(og[trax], step):
                continue
            if not s16_used_in_orig(og[trax], s16):
                pitches[trax][s16] = pitch_for_riff(era, trax, s16, added, riff_prev)
                riff_prev = pitches[trax][s16]
            g = set_step_bit(g, step, 1)
            added += 1
        gates[trax] = g

    for trax in range(TRAX_N):
        smooth_trax_pitches(trax, gates[trax], pitches)
        for s16 in range(16):
            p = pitches[trax][s16]
            if p < PITCH_MIN:
                pitches[trax][s16] = PITCH_MIN
            elif p > PITCH_MAX:
                pitches[trax][s16] = PITCH_MAX

    lens, bends = build_len_bend(gates, pitches)
    return gates, pitches, lens, bends


def format_gate_row(vals: list[int]) -> str:
    parts = [f"0x{v:08X}u" for v in vals]
    lines = []
    for i in range(0, len(parts), 8):
        chunk = ", ".join(parts[i : i + 8])
        lines.append(f"    {chunk},")
    return "\n".join(lines)


def format_pitch_rows(rows: list[list[int]]) -> str:
    lines = []
    for row in rows:
        inner = ", ".join(f"{v:4d}" for v in row)
        lines.append(f"    {{ {inner} }},")
    return "\n".join(lines)


def format_u8_step_rows(rows: list[list[int]]) -> str:
    lines = []
    for row in rows:
        inner = ", ".join(f"{v:3d}" for v in row)
        lines.append(f"    {{ {inner} }},")
    return "\n".join(lines)


def parse_u8_step_rows(text: str, name: str) -> list[list[int]]:
    m = re.search(rf"{name}\[.*?\]\s*=\s*\{{(.*?)\}};", text, re.S)
    if not m:
        return []
    rows: list[list[int]] = []
    for line in m.group(1).split("\n"):
        nums = [int(x.strip().rstrip(",")) for x in re.findall(r"\d+", line)]
        if len(nums) == SEQ_STEPS:
            rows.append(nums)
    return rows


def patch_header(
    gates: list[int],
    pitches: list[list[int]],
    lens: list[list[int]],
    bends: list[list[int]],
) -> None:
    text = HEADER.read_text()
    gate_block = format_gate_row(gates)
    pitch_block = format_pitch_rows(pitches)
    text = re.sub(
        r"(static const uint32_t HEAVY_PSYCH_TRAX_LEAD_GATE\[HEAVY_PSYCH_TRAX_N\] = \{)\n.*?\n\};",
        rf"\1\n{gate_block}\n}};",
        text,
        count=1,
        flags=re.S,
    )
    text = re.sub(
        r"(static const int8_t HEAVY_PSYCH_TRAX_LEAD_PITCH\[HEAVY_PSYCH_TRAX_N\]\[16\] = \{)\n.*?\n\};",
        rf"\1\n{pitch_block}\n}};",
        text,
        count=1,
        flags=re.S,
    )
    len_block = format_u8_step_rows(lens)
    bend_block = format_u8_step_rows(bends)
    if "HEAVY_PSYCH_TRAX_LEAD_LEN" in text:
        text = re.sub(
            r"(static const uint8_t HEAVY_PSYCH_TRAX_LEAD_LEN\[HEAVY_PSYCH_TRAX_N\]\[32\] = \{)\n.*?\n\};",
            rf"\1\n{len_block}\n}};",
            text,
            count=1,
            flags=re.S,
        )
        text = re.sub(
            r"(static const uint8_t HEAVY_PSYCH_TRAX_LEAD_BEND\[HEAVY_PSYCH_TRAX_N\]\[32\] = \{)\n.*?\n\};",
            rf"\1\n{bend_block}\n}};",
            text,
            count=1,
            flags=re.S,
        )
    else:
        insert = (
            "/* Per-step lead length (HP_LEAD_LEN ix) and bend depth (0..12 semitones); legato via LEN overlap. */\n"
            f"static const uint8_t HEAVY_PSYCH_TRAX_LEAD_LEN[HEAVY_PSYCH_TRAX_N][32] = {{\n{len_block}\n}};\n\n"
            f"static const uint8_t HEAVY_PSYCH_TRAX_LEAD_BEND[HEAVY_PSYCH_TRAX_N][32] = {{\n{bend_block}\n}};\n\n"
        )
        text = text.replace(
            "static const heavy_psych_lead_voice_t HEAVY_PSYCH_TRAX_LEAD_VOICE",
            insert + "static const heavy_psych_lead_voice_t HEAVY_PSYCH_TRAX_LEAD_VOICE",
            1,
        )
    HEADER.write_text(text)


def main() -> int:
    gates, pitches, lens, bends = build()
    base = subprocess.check_output(["git", "show", f"{BASE_REV}:firmware/src/heavy_psych_data.h"], text=True)
    og, _op = parse_gate_pitch(base)
    if len(og) == 48 and TRAX_N == 60:
        og_exp: list[int] = []
        for e in range(len(og) // 8):
            chunk = og[e * 8 : (e + 1) * 8]
            og_exp.extend(chunk)
            og_exp.extend([chunk[6], chunk[7]])
        og = og_exp
    lost = melody_steps_lost(og, gates)
    if lost:
        print(f"bcf4c42 melody loss: {len(lost)} steps", file=sys.stderr)
        return 1
    if "--check" in sys.argv:
        text = HEADER.read_text()
        cur_g, cur_p = parse_gate_pitch(text)
        cur_len = parse_u8_step_rows(text, "HEAVY_PSYCH_TRAX_LEAD_LEN")
        cur_bend = parse_u8_step_rows(text, "HEAVY_PSYCH_TRAX_LEAD_BEND")
        if len(cur_g) != TRAX_N or len(cur_p) != TRAX_N or len(cur_len) != TRAX_N or len(cur_bend) != TRAX_N:
            print("heavy_psych_data.h lead tables stale (run without --check)", file=sys.stderr)
            return 1
        if lost:
            return 1
        return 0
    patch_header(gates, pitches, lens, bends)
    print(f"Patched {HEADER.name}: {TRAX_N} trax, gate/pitch/len/bend")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
