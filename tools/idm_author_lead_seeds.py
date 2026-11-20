#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Author IDM_PIECE_LEAD_GATE/PITCH (48 hybrid stutter seeds) into idm_data.h."""
from __future__ import annotations

PRESETS_PER_ERA = 10

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "firmware/src/idm_data.h"

PIECE_N = 60
ERA_N = 6
SEQ_STEPS = 32
PITCH_MIN = -5
PITCH_MAX = 5
MAX_JUMP = 5
ERA_NAMES = ("WARM", "MELO", "GLITCH", "BREAK", "DRILL", "OBLIQ")

# step_bit / set_step_bit — same layout as idm_data.h idm_piece_lead_step_bit
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


def mix32(x: int) -> int:
    x &= 0xFFFFFFFF
    x ^= x >> 16
    x = (x * 0x7FEB352D) & 0xFFFFFFFF
    x ^= x >> 15
    x = (x * 0x846CA68B) & 0xFFFFFFFF
    return x ^ (x >> 16)


# Era motif zones (bar4, sixteenth indices) — short cells for live stutter chop
ERA_LEAD = {
    0: {  # WARM — soft broken, sparse
        "zones": (
            (0, (0, 4, 8, 12)),
            (1, (0, 6, 10)),
            (2, (4, 12)),
            (3, (12, 14)),
        ),
        "keep": 5,
        "pool": (0, -2, 2, 3, -1, 5, -3),
    },
    1: {  # MELO — singable pluck
        "zones": (
            (0, (0, 2, 4, 8, 12)),
            (1, (0, 4, 8, 14)),
            (2, (0, 8)),
            (3, (14, 15)),
        ),
        "keep": 6,
        "pool": (0, 2, 4, 5, -1, 7, 3),
    },
    2: {  # GLITCH — chop skips
        "zones": (
            (0, (2, 6, 10, 14)),
            (1, (1, 5, 9, 13)),
            (2, (8, 10, 12)),
            (3, (8, 10, 12, 14)),
        ),
        "keep": 7,
        "pool": (0, 1, -1, 2, 3, -2, 4),
    },
    3: {  # BREAK — snare-adjacent syncopation
        "zones": (
            (0, (0, 4, 12)),
            (1, (2, 6, 10, 14)),
            (2, (0, 8, 12)),
            (3, (9, 11, 13, 15)),
        ),
        "keep": 7,
        "pool": (0, 3, 5, -2, 7, 2, -3),
    },
    4: {  # DRILL — hyper grid fragments
        "zones": (
            (0, tuple(range(0, 16, 2))),
            (1, tuple(range(1, 16, 2))),
            (2, (0, 4, 8, 12)),
            (3, (8, 9, 10, 11, 12, 13, 14, 15)),
        ),
        "keep": 10,
        "pool": (0, 1, 2, -1, 3, -2, 4),
    },
    5: {  # OBLIQ — oblique sparse
        "zones": (
            (0, (0, 7, 15)),
            (1, (3, 11)),
            (2, (5, 13)),
            (3, (8, 12, 15)),
        ),
        "keep": 4,
        "pool": (0, 4, -3, 7, 2, -5, 5),
    },
}


def gate_for_piece(piece: int) -> int:
    era = piece // PRESETS_PER_ERA
    slot = piece % PRESETS_PER_ERA
    spec = ERA_LEAD[era]
    g = 0
    # Per-piece stochastic gate density (slot + piece skew → 48 unique patterns).
    thresh = 5 + (slot & 3) + (era if era < 4 else 6)
    for bar4, steps in spec["zones"]:
        for s16 in steps:
            step = (bar4 << 4) | s16
            if step >= SEQ_STEPS:
                continue
            roll = mix32(piece * 0x9E3779B9 ^ step ^ (era << 20)) & 15
            if roll >= thresh:
                continue
            g = set_step_bit(g, step, 1)
    # Guarantee a short motif skeleton per slot (era-specific cell).
    skeleton = (
        (0, ((0, 4, 8, 12)[slot % 4],)),
        (1, ((0, 4, 8, 14)[slot % 4],)),
        (2, ((slot * 2) & 15,)),
        (3, (12 + (slot & 3),)),
    )
    for bar4, steps in skeleton:
        for s16 in steps:
            step = (bar4 << 4) | s16
            if step < SEQ_STEPS:
                g = set_step_bit(g, step, 1)
    return g


def pitch_for_piece(piece: int, g32: int) -> list[int]:
    era = piece // PRESETS_PER_ERA
    pool = ERA_LEAD[era]["pool"]
    pitches = [0] * 16
    used: list[int] = []
    for step in range(SEQ_STEPS):
        if step_bit(g32, step):
            s16 = step & 15
            if s16 not in used:
                used.append(s16)
    prev: int | None = None
    for i, s16 in enumerate(sorted(used)):
        pi = mix32((piece + 1) * 0x9E3779B9 ^ (s16 << 3) ^ i) % len(pool)
        p = pool[pi]
        if prev is not None:
            if p - prev > MAX_JUMP:
                p = prev + MAX_JUMP
            elif p - prev < -MAX_JUMP:
                p = prev - MAX_JUMP
        p = max(PITCH_MIN, min(PITCH_MAX, p))
        pitches[s16] = p
        prev = p
    return pitches


def build_tables() -> tuple[list[int], list[list[int]]]:
    gates: list[int] = []
    pitches: list[list[int]] = []
    seen: set[int] = set()
    for piece in range(PIECE_N):
        g = gate_for_piece(piece)
        attempt = 0
        while g in seen and attempt < 48:
            s16 = (piece + attempt * 3) & 15
            bar4 = (attempt + piece // PRESETS_PER_ERA) & 3
            step = (bar4 << 4) | s16
            if step < SEQ_STEPS:
                g = set_step_bit(g, step, 1)
            attempt += 1
        seen.add(g)
        gates.append(g)
        pitches.append(pitch_for_piece(piece, g))
    return gates, pitches


def format_gate_val(v: int) -> str:
    """Avoid tools/build.py MMIO_LIT false positives on low 0x0001….u bitmasks."""
    lo = v & 0xFFFF
    hi = (v >> 16) & 0xFFFF
    return f"((uint32_t)0x{lo:04X}u | ((uint32_t)0x{hi:04X}u << 16))"


def format_gate(vals: list[int]) -> str:
    parts = [format_gate_val(v) for v in vals]
    lines = []
    for i in range(0, len(parts), 4):
        lines.append("    " + ", ".join(parts[i : i + 4]) + ",")
    return "\n".join(lines)


def format_pitch(rows: list[list[int]]) -> str:
    lines = []
    for piece, row in enumerate(rows):
        era = piece // PRESETS_PER_ERA
        slot = piece % PRESETS_PER_ERA
        inner = ", ".join(f"{v:4d}" for v in row)
        lines.append(
            f"    {{ {inner} }}, /* {ERA_NAMES[era]} {slot} */"
        )
    return "\n".join(lines)


ERA_PUMP = (4, 4, 8, 10, 14, 6)
ERA_DENSITY = (74, 78, 96, 100, 112, 80)
ERA_WOBBLE = (10, 12, 18, 22, 28, 14)


def patch_plates(text: str) -> str:
    """Content rules: low pump, era-scaled density/wobble."""
    lines = text.splitlines()
    out: list[str] = []
    piece = 0
    for line in lines:
        m = re.match(r"(\s*/\* [A-Z]+ \d+ \*/ \{ \"([^\"]+)\",\s*)(.+)", line)
        if m and piece < PIECE_N and "0x" in line:
            era = piece // PRESETS_PER_ERA
            slot = piece % PRESETS_PER_ERA
            prefix, name, rest = m.group(1), m.group(2), m.group(3)
            ix = rest.find("0x")
            head, masks = rest[:ix], rest[ix:].rstrip().rstrip(",").rstrip().rstrip("}")
            nums = [x.strip() for x in head.split(",") if x.strip()]
            if len(nums) >= 13:
                dens = ERA_DENSITY[era] + (slot & 3) * 2
                pump = ERA_PUMP[era] + (slot & 1)
                wob = ERA_WOBBLE[era] + slot
                nums[6] = str(min(120, dens))
                nums[11] = str(min(40, wob))
                nums[12] = str(min(20, pump))
                line = prefix + ", ".join(nums) + ", " + masks + " },"
            piece += 1
        out.append(line)
    return "\n".join(out) + "\n"


def patch_header(gates: list[int], pitches: list[list[int]]) -> None:
    text = HEADER.read_text()
    gate_block = format_gate(gates)
    pitch_block = format_pitch(pitches)
    text = re.sub(
        r"(static const uint32_t IDM_PIECE_LEAD_GATE\[IDM_PIECE_N\] = \{)\n.*?\n\};",
        rf"\1\n{gate_block}\n}};",
        text,
        count=1,
        flags=re.S,
    )
    text = re.sub(
        r"(static const int8_t IDM_PIECE_LEAD_PITCH\[IDM_PIECE_N\]\[16\] = \{)\n.*?\n\};",
        rf"\1\n{pitch_block}\n}};",
        text,
        count=1,
        flags=re.S,
    )
    text = patch_plates(text)
    HEADER.write_text(text)


def main() -> int:
    gates, pitches = build_tables()
    ug = len(set(gates))
    up = len(set(tuple(r) for r in pitches))
    if ug != PIECE_N or up != PIECE_N:
        print(f"error: unique gates={ug} pitches={up} (need {PIECE_N})", file=sys.stderr)
        return 1
    patch_header(gates, pitches)
    print(f"Patched {HEADER.name}: {PIECE_N} gate/pitch seeds ({ug} unique gates, {up} unique pitch rows)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
