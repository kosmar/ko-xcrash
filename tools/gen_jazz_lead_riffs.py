#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Regenerate JAZZ_TRAX_LEAD_GATE/PITCH/LEN/BEND/GATE_CS in firmware/src/jazz_data.h.

32-step (2-bar) horn tables. Low-16 = call, high-16 = response.
Pitches are scale-legal semitone offsets from JAZZ_TRAX_ROOT+12 (same
scale_deg_note as bass). Complementary vs kick / bass-root / chord hits.

Firmware path: SPIRIT/FUSION/BROKEN also use jazz_lead_phrase_span in
fm_app.c (POP-like hold). MUTE/ACID/MODAL stay table-only short/sparse cells.

  python3 tools/gen_jazz_lead_riffs.py          # patch jazz_data.h
  python3 tools/gen_jazz_lead_riffs.py --check  # exit 1 if tables stale
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "firmware/src/jazz_data.h"

PRESETS_PER_ERA = 10
TRAX_N = 60
ERA_N = 6
SEQ_STEPS = 32
SPIRIT, MODAL, MUTE, FUSION, ACID, BROKEN = range(6)
ERA_NAMES = ("SPIRIT", "MODAL", "MUTE", "FUSION", "ACID", "BROKEN")

LEN_16THS = (1, 2, 4, 5, 8, 11, 12, 16, 20, 24, 28, 32, 40, 48, 64)
LEN_IX_MAX = len(LEN_16THS) - 1
PITCH_MIN = -24
PITCH_MAX = 24
PREFIX = "JAZZ_TRAX_LEAD"

SCALE_LEN = (6, 7, 7, 7, 7, 7, 5, 6, 7)
SCALE_SEMIS = (
    (0, 2, 3, 5, 7, 10, 0),
    (0, 2, 3, 5, 7, 8, 10),
    (0, 2, 3, 5, 7, 9, 10),
    (0, 1, 3, 5, 7, 8, 10),
    (0, 2, 4, 5, 7, 9, 11),
    (0, 2, 4, 5, 7, 9, 10),
    (0, 3, 5, 7, 10, 0, 0),
    (0, 3, 5, 6, 7, 10, 0),
    (0, 2, 3, 5, 7, 8, 11),
)

# 0–15 step cells. Response is placed at +16. Kick-heavy 0/12 avoided except ACID.
SPIRIT_RHY = (
    ((1, 3, 5, 8, 10), (3, 6, 8, 11)),
    ((2, 3, 6, 9, 11, 13), (1, 4, 7, 10)),
    ((1, 2, 4, 7, 10), (2, 5, 9, 11, 13)),
    ((3, 5, 6, 10, 13), (1, 3, 8, 14)),
    ((1, 4, 5, 8, 11, 14), (2, 6, 9)),
    ((2, 4, 7, 9, 10), (4, 5, 8, 11, 13)),
    ((1, 3, 6, 8, 12), (2, 7, 10, 14)),
    ((2, 5, 8, 9, 13), (1, 4, 6, 11)),
)
SPIRIT_BALLAD = (
    ((2, 6, 10), (4, 11)),
    ((1, 5, 8, 13), (3, 9)),
)
MODAL_RHY = (
    ((0, 8), (10,)),
    ((4,), (8,)),
    ((2, 10), (5,)),
    ((0, 6), (13,)),
    ((8,), (2,)),
    ((2, 8), (14,)),
    ((4, 12), (6,)),
    ((0,), (11,)),
)
MODAL_BALLAD = (
    ((4,), (12,)),
    ((0, 8), (6,)),
)
MUTE_RHY = (
    ((2, 5), (4, 7)),
    ((3, 4, 6), (9,)),
    ((1, 6), (3, 5)),
    ((2, 3, 7), (10,)),
    ((5, 8), (1, 6)),
    ((3, 9), (4, 11)),
    ((1, 4, 8), (6,)),
    ((2, 7), (5, 9)),
)
MUTE_BALLAD = (
    ((4, 10), (6,)),
    ((2, 8), (11,)),
)
FUSION_RHY = (
    ((1, 2, 3, 5, 6, 8, 9, 10), (1, 2, 4, 5, 6, 8, 9, 11)),
    ((1, 3, 4, 6, 7, 9, 11, 13, 14), (2, 3, 5, 7, 8, 10, 13)),
    ((2, 3, 4, 6, 8, 10, 11, 13), (1, 3, 4, 7, 9, 10, 14)),
    ((1, 2, 5, 6, 7, 9, 10, 13, 14), (2, 4, 5, 8, 9, 11, 14)),
    ((1, 3, 5, 6, 8, 9, 11, 14), (1, 2, 3, 6, 8, 10, 11, 13)),
    ((2, 4, 5, 7, 8, 10, 11, 13, 14), (1, 4, 6, 7, 9, 13)),
    ((1, 2, 4, 7, 8, 9, 11, 13), (2, 3, 5, 6, 8, 10, 14)),
    ((1, 3, 4, 5, 8, 10, 11, 14), (1, 2, 5, 7, 8, 9, 11, 13)),
)
FUSION_BALLAD = (
    ((2, 4, 7, 9, 11), (3, 6, 8, 13)),
    ((1, 5, 6, 10, 13), (2, 7, 9, 14)),
)
ACID_RHY = (
    ((0, 3, 6, 10), (0, 3, 7, 10)),
    ((1, 4, 5, 8), (1, 4, 8, 13)),
    ((0, 2, 6, 9, 11), (2, 6, 9, 14)),
    ((3, 4, 7, 11), (3, 7, 8, 11)),
    ((0, 5, 8, 13), (0, 5, 10, 13)),
    ((1, 2, 6, 10), (1, 3, 6, 10, 14)),
    ((0, 4, 7, 8, 12), (4, 7, 8, 15)),
    ((2, 5, 9, 14), (2, 5, 11, 14)),
)
ACID_BALLAD = (
    ((0, 6, 10), (0, 7, 10)),
    ((4, 8, 13), (4, 9, 13)),
)
BROKEN_RHY = (
    ((1, 6, 7, 14), (2, 9, 15)),
    ((2, 3, 10, 13), (5, 8, 14)),
    ((1, 8, 9), (3, 4, 11, 15)),
    ((5, 6, 14), (1, 7, 10, 13)),
    ((2, 9, 11, 15), (4, 5, 12)),
    ((1, 4, 13), (6, 7, 14, 15)),
    ((3, 8, 10, 14), (1, 9, 15)),
    ((2, 7, 15), (4, 8, 11, 13)),
)
BROKEN_BALLAD = (
    ((3, 11), (6, 14)),
    ((1, 8, 14), (5, 13)),
)


def mix32(x: int) -> int:
    x &= 0xFFFFFFFF
    x ^= x >> 16
    x = (x * 0x7FEB352D) & 0xFFFFFFFF
    x ^= x >> 15
    x = (x * 0x846CA68B) & 0xFFFFFFFF
    return x ^ (x >> 16)


def scale_deg_note(root: int, scale: int, deg: int) -> int:
    sid = scale % 9
    length = SCALE_LEN[sid]
    oct_add = deg // length
    d = deg % length
    oct_ = root // 12
    n = (oct_ + oct_add) * 12 + (root % 12) + SCALE_SEMIS[sid][d]
    return max(0, min(127, n))


def lead_semi(root: int, scale: int, deg: int) -> int:
    """Scale-legal offset from root+12 (same convention as jazz_trax_lead_pitch)."""
    sid = scale % 9
    length = SCALE_LEN[sid]
    n = scale_deg_note(root, scale, deg + length)
    return n - (root + 12)


def clamp_pitch(p: int) -> int:
    return max(PITCH_MIN, min(PITCH_MAX, p))


def parse_trax_roots_scales(text: str) -> tuple[list[int], list[int]]:
    m = re.search(r"JAZZ_TRAX_ROOT\[JAZZ_TRAX_N\]\s*=\s*\{([^}]+)\}", text)
    m2 = re.search(r"JAZZ_TRAX_SCALE\[JAZZ_TRAX_N\]\s*=\s*\{([^}]+)\}", text)
    if not m or not m2:
        raise ValueError("missing JAZZ_TRAX_ROOT/SCALE")
    roots = [int(x) for x in re.findall(r"-?\d+", m.group(1))]
    scales = [int(x) for x in re.findall(r"-?\d+", m2.group(1))]
    if len(roots) != TRAX_N or len(scales) != TRAX_N:
        raise ValueError(f"expected {TRAX_N} root/scale, got {len(roots)}/{len(scales)}")
    return roots, scales


def parse_bass_walk(text: str) -> list[list[list[int]]]:
    m = re.search(r"JAZZ_BASS_WALK\[.*?\]\s*=\s*\{(.*?)\};", text, re.S)
    if not m:
        raise ValueError("missing JAZZ_BASS_WALK")
    nums = [int(x) for x in re.findall(r"\d+", m.group(1))]
    per = PRESETS_PER_ERA * 8
    if len(nums) < ERA_N * per:
        raise ValueError("JAZZ_BASS_WALK too short")
    out: list[list[list[int]]] = []
    ix = 0
    for _era in range(ERA_N):
        era_rows: list[list[int]] = []
        for _piece in range(PRESETS_PER_ERA):
            era_rows.append(nums[ix : ix + 8])
            ix += 8
        out.append(era_rows)
    return out


def parse_trax_masks(text: str) -> list[dict[str, int]]:
    masks: list[dict[str, int]] = []
    block = re.search(r"static const jazz_trax_t JAZZ_TRAX\[.*?\]\s*=\s*\{(.*?)\n\};", text, re.S)
    if not block:
        raise ValueError("missing JAZZ_TRAX")
    for line in block.group(1).split("\n"):
        hexes = [int(h, 16) for h in re.findall(r"0x([0-9A-Fa-f]+)", line)]
        if len(hexes) < 12:
            continue

        def pack(i: int) -> int:
            return hexes[i] | (hexes[i + 1] << 16)

        masks.append(
            {
                "kick": pack(0),
                "rim": pack(2),
                "hat": pack(4),
                "sub": pack(6),
                "chord": pack(8),
            }
        )
    if len(masks) != TRAX_N:
        raise ValueError(f"expected {TRAX_N} trax masks, got {len(masks)}")
    return masks


def drum_bit(mask32: int, step: int) -> int:
    return (mask32 >> (step & 31)) & 1


def lead_step_bit(g32: int, step: int) -> int:
    bar4 = (step >> 4) & 3
    s16 = step & 15
    if bar4 in (0, 2):
        return (g32 >> s16) & 1
    if bar4 == 1:
        return (g32 >> (16 + s16)) & 1
    if s16 < 8:
        return 0
    return (g32 >> (16 + s16)) & 1


def set_lead_bit(g32: int, step: int, on: int) -> int:
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


def rhythm_for(era: int, slot: int) -> tuple[tuple[int, ...], tuple[int, ...]]:
    ballad = slot >= 8
    tables = {
        SPIRIT: (SPIRIT_BALLAD if ballad else SPIRIT_RHY),
        MODAL: (MODAL_BALLAD if ballad else MODAL_RHY),
        MUTE: (MUTE_BALLAD if ballad else MUTE_RHY),
        FUSION: (FUSION_BALLAD if ballad else FUSION_RHY),
        ACID: (ACID_BALLAD if ballad else ACID_RHY),
        BROKEN: (BROKEN_BALLAD if ballad else BROKEN_RHY),
    }
    rows = tables[era]
    return rows[slot - 8 if ballad else slot]


def placement_cost(
    trax: int,
    step: int,
    era: int,
    bass_walk: list[list[list[int]]],
    masks: list[dict[str, int]],
) -> int:
    slot = trax % PRESETS_PER_ERA
    m = masks[trax]
    cost = 0
    kick = drum_bit(m["kick"], step)
    chord = drum_bit(m["chord"], step)
    sub = drum_bit(m["sub"], step)
    rim = drum_bit(m["rim"], step)
    bass_deg = bass_walk[era][slot][step & 7]
    avoid_kick = era not in (ACID, BROKEN)
    if kick:
        cost += 12 if avoid_kick else 1
    if rim and era in (SPIRIT, MUTE, MODAL):
        cost += 2
    if sub and era != ACID:
        cost += 2
    if bass_deg == 0:
        cost += 5 if era != MODAL else 0
    else:
        cost -= 1
    if chord:
        if era in (SPIRIT, MUTE):
            cost += 3
        elif era == FUSION:
            cost -= 1
        elif era == MODAL:
            cost -= 2
    s16 = step & 15
    if s16 in (0, 8) and era in (SPIRIT, MUTE):
        cost += 2
    if s16 in (1, 3, 5, 6, 10, 11, 13, 14):
        cost -= 1
    return cost


def nudge_step(
    trax: int,
    step: int,
    half: int,
    era: int,
    used: set[int],
    bass_walk: list[list[list[int]]],
    masks: list[dict[str, int]],
) -> int | None:
    lo = 16 if half else 0
    hi = 32 if half else 16
    best = step
    best_c = placement_cost(trax, step, era, bass_walk, masks)
    for d in (0, 1, -1, 2, -2, 3):
        ns = step + d
        if ns < lo or ns >= hi or ns in used:
            continue
        c = placement_cost(trax, ns, era, bass_walk, masks)
        if d != 0:
            c += abs(d)
        if c < best_c:
            best_c = c
            best = ns
    if best in used:
        return None
    return best


def place_half(
    steps16: tuple[int, ...],
    half: int,
    trax: int,
    era: int,
    bass_walk: list[list[list[int]]],
    masks: list[dict[str, int]],
    used: set[int],
) -> list[int]:
    out: list[int] = []
    base = 16 if half else 0
    for s in steps16:
        raw = base + (s & 15)
        ns = nudge_step(trax, raw, half, era, used, bass_walk, masks)
        if ns is None:
            continue
        out.append(ns)
        used.add(ns)
    return out


def contour_degs(era: int, slot: int, n_call: int, n_resp: int) -> tuple[list[int], list[int]]:
    """Scale degrees from tonic. Response answers (invert / fifth / delay)."""
    seed = mix32((era << 16) | (slot << 8) | n_call * 17 + n_resp)
    if era == SPIRIT:
        src = [0, 2, 4, 5, 7, 4, 2, 5, 7, 9]
        rot = slot % 6
        src = src[rot:] + src[:rot]
        call = src[:n_call]
        if slot & 1:
            axis = 4
            resp = [max(0, min(10, 2 * axis - d + (slot & 1))) for d in call[:n_resp]]
        else:
            resp = list(reversed(call[:n_resp])) if n_resp <= n_call else list(reversed(call)) + [2] * (
                n_resp - n_call
            )
            resp = resp[:n_resp]
        if slot >= 8:
            call = [d if d <= 5 else 4 for d in call]
            resp = [4 if i == 0 else 2 for i in range(n_resp)]
        return call, resp
    if era == MODAL:
        pedal = 0 if (slot % 3) != 2 else 4
        ans = 4 if pedal == 0 else 0
        if slot >= 8:
            ans = 4
        return [pedal] * n_call, [ans] * n_resp
    if era == MUTE:
        cells = (
            [0, 2, 3, 0],
            [2, 4, 2],
            [0, 3, 5, 3],
            [4, 2, 0],
            [0, 2, 4, 7],
            [3, 0, 2],
            [2, 3, 2, 0],
            [0, 4, 3],
            [0, 2],
            [2, 0],
        )[slot]
        call = (cells * 3)[:n_call]
        resp = [cells[(i + 2) % len(cells)] for i in range(n_resp)]
        return call, resp
    if era == FUSION:
        src = [0, 2, 4, 5, 7, 9, 7, 5, 4, 2, 3, 5, 7, 4, 9, 7]
        rot = (slot * 3) % 8
        src = src[rot:] + src[:rot]
        call = src[:n_call]
        resp = [min(10, d + 2) for d in src[1 : 1 + n_resp]]
        if len(resp) < n_resp:
            extra = [min(10, d + 1) for d in reversed(call)]
            resp = (resp + extra)[:n_resp]
        if slot >= 8:
            call = [0, 2, 4, 5, 4][:n_call]
            resp = [7, 5, 4, 2][:n_resp]
        return call, resp
    if era == ACID:
        cells = (
            [0, 0, 3, 0],
            [0, 3, 7, 3],
            [0, 4, 0, 7],
            [7, 0, 3, 0],
            [0, 3, 0, 5],
            [0, 7, 3, 7],
            [3, 0, 0, 7],
            [0, 5, 3, 0],
            [0, 0, 4],
            [0, 4, 0],
        )[slot]
        call = (cells * 4)[:n_call]
        resp = [(cells[(i + 1) % len(cells)] if i < len(cells) else cells[i % len(cells)]) for i in range(n_resp)]
        if slot in (1, 5, 7):
            resp = [d if i != len(resp) - 1 else (d + 4) % 8 for i, d in enumerate(resp)]
        return call, resp
    # BROKEN — leaps
    leaps = (
        [0, 6, 2, 7, 1],
        [7, 0, 4, 1, 8],
        [2, 8, 0, 5],
        [4, 0, 7, 1, 6],
        [1, 7, 3, 8],
        [6, 0, 2, 9, 1],
        [0, 5, 8, 2],
        [3, 9, 0, 6, 1],
        [0, 7],
        [4, 0, 8],
    )[slot]
    call = (leaps * 3)[:n_call]
    resp = [leaps[(i + 2) % len(leaps)] ^ 1 for i in range(n_resp)]
    resp = [max(0, min(10, d)) for d in resp]
    _ = seed
    return call, resp


def sixteenths_to_len_ix(n: int) -> int:
    ix = 0
    for i, s in enumerate(LEN_16THS):
        if s <= n:
            ix = i
    return ix


def len_for_gap(era: int, slot: int, gap: int, is_resp: bool, is_tail: bool) -> int:
    ballad = slot >= 8
    if era == MODAL and is_resp:
        return 9 if not ballad else 11
    if era == MODAL:
        return 4 if not ballad else 6
    if era == MUTE:
        return min(2, sixteenths_to_len_ix(max(1, gap)))
    if era == ACID:
        return min(2, sixteenths_to_len_ix(max(1, min(gap, 2))))
    if era == SPIRIT:
        ix = sixteenths_to_len_ix(max(1, min(gap, 4 if not ballad else 6)))
        if is_tail:
            ix = max(ix, 4 if not ballad else 7)
        if is_resp:
            ix = min(LEN_IX_MAX, ix + 1)
        return ix
    if era == FUSION:
        ix = sixteenths_to_len_ix(max(2, min(gap, 6 if not ballad else 8)))
        if is_tail or is_resp:
            ix = max(ix, 7 if not ballad else 8)
        return min(ix, 9 if not ballad else 11)
    # BROKEN
    ix = sixteenths_to_len_ix(max(1, min(gap, 3 if not ballad else 5)))
    if is_resp and is_tail:
        ix = max(ix, 4)
    return ix


def gate_cs_for(era: int, slot: int) -> int:
    ballad = slot >= 8
    base = {
        SPIRIT: 38,
        MODAL: 34,
        MUTE: 22,
        FUSION: 34,
        ACID: 14,
        BROKEN: 26,
    }[era]
    if ballad:
        base += 8 if era in (SPIRIT, MODAL, FUSION) else 4
    return min(56, base + (slot % 5) - 2)


def parse_gate_pitch(text: str) -> tuple[list[int], list[list[int]]]:
    m = re.search(rf"{PREFIX}_GATE\[.*?\]\s*=\s*\{{(.*?)\}};", text, re.S)
    if not m:
        raise ValueError(f"missing {PREFIX}_GATE in {HEADER.name}")
    gates = [int(x.rstrip("uU"), 0) for x in re.findall(r"0x[0-9A-Fa-f]+u?", m.group(1))]
    m2 = re.search(rf"{PREFIX}_PITCH\[.*?\]\s*=\s*\{{(.*?)\}};", text, re.S)
    if not m2:
        raise ValueError(f"missing {PREFIX}_PITCH")
    rows: list[list[int]] = []
    for line in m2.group(1).split("\n"):
        code = line.split("/*", 1)[0]
        nums = [int(x.strip().rstrip(",")) for x in re.findall(r"-?\d+", code)]
        if len(nums) >= SEQ_STEPS:
            rows.append(nums[:SEQ_STEPS])
    if len(gates) != TRAX_N or len(rows) != TRAX_N:
        raise ValueError(f"expected {TRAX_N} gate/pitch rows, got {len(gates)}/{len(rows)}")
    return gates, rows


def parse_u8_step_rows(text: str, sym: str) -> list[list[int]]:
    m = re.search(rf"{sym}\[.*?\]\s*=\s*\{{(.*?)\}};", text, re.S)
    if not m:
        raise ValueError(f"missing {sym}")
    rows: list[list[int]] = []
    for line in m.group(1).split("\n"):
        nums = [int(x.strip().rstrip(",")) for x in re.findall(r"\d+", line)]
        if len(nums) == SEQ_STEPS:
            rows.append(nums)
    return rows


def parse_gate_cs(text: str) -> list[int]:
    m = re.search(rf"{PREFIX}_GATE_CS\[JAZZ_TRAX_N\]\s*=\s*\{{([^}}]+)\}}", text)
    if not m:
        raise ValueError("missing GATE_CS")
    vals = [int(x) for x in re.findall(r"\d+", m.group(1))]
    if len(vals) != TRAX_N:
        raise ValueError(f"expected {TRAX_N} GATE_CS, got {len(vals)}")
    return vals


def gated_steps(g32: int) -> list[int]:
    return [s for s in range(SEQ_STEPS) if lead_step_bit(g32, s)]


def trax_signature(gates: list[int], pitches: list[list[int]], trax: int) -> tuple[int, tuple[int, ...]]:
    active = tuple(pitches[trax][s] for s in range(SEQ_STEPS) if lead_step_bit(gates[trax], s))
    return gates[trax], active


def assign_line(
    trax: int,
    steps: list[int],
    degs: list[int],
    pitches: list[list[int]],
    roots: list[int],
    scales: list[int],
) -> None:
    root = roots[trax]
    scale = scales[trax]
    for i, step in enumerate(steps):
        deg = degs[i] if i < len(degs) else degs[-1]
        pitches[trax][step] = clamp_pitch(lead_semi(root, scale, deg))


def build() -> tuple[list[int], list[list[int]], list[list[int]], list[list[int]], list[int]]:
    text = HEADER.read_text()
    roots, scales = parse_trax_roots_scales(text)
    bass_walk = parse_bass_walk(text)
    masks = parse_trax_masks(text)

    gates = [0] * TRAX_N
    pitches = [[0] * SEQ_STEPS for _ in range(TRAX_N)]
    lens = [[0] * SEQ_STEPS for _ in range(TRAX_N)]
    bends = [[0] * SEQ_STEPS for _ in range(TRAX_N)]
    gate_cs = [0] * TRAX_N
    seen: set[tuple[int, tuple[int, ...]]] = set()

    for trax in range(TRAX_N):
        era = trax // PRESETS_PER_ERA
        slot = trax % PRESETS_PER_ERA
        call_r, resp_r = rhythm_for(era, slot)
        used: set[int] = set()
        call = place_half(call_r, 0, trax, era, bass_walk, masks, used)
        resp = place_half(resp_r, 1, trax, era, bass_walk, masks, used)
        if era == FUSION and slot < 8:
            want = 13
            fill_from = [1, 3, 5, 6, 9, 10, 11, 13, 14, 17, 19, 21, 22, 25, 26, 27, 29, 30]
            for st in fill_from:
                if len(call) + len(resp) >= want:
                    break
                if st in used:
                    continue
                if placement_cost(trax, st, era, bass_walk, masks) > 8:
                    continue
                used.add(st)
                (resp if st >= 16 else call).append(st)
            call.sort()
            resp.sort()
        if not call:
            call = [1 + (slot % 5)]
        if not resp and era != MUTE:
            resp = [17 + (slot % 7)]
        cdegs, rdegs = contour_degs(era, slot, len(call), max(1, len(resp)))
        assign_line(trax, call, cdegs, pitches, roots, scales)
        if resp:
            assign_line(trax, resp, rdegs, pitches, roots, scales)

        g = 0
        for st in call + resp:
            g = set_lead_bit(g, st, 1)
        lo, hi = g & 0xFFFF, (g >> 16) & 0xFFFF
        if era in (SPIRIT, FUSION, BROKEN) and lo == hi:
            g ^= 1 << (16 + ((slot * 3 + 2) % 15))
        gates[trax] = g

        ordered = gated_steps(g)
        for i, st in enumerate(ordered):
            nxt = ordered[i + 1] if i + 1 < len(ordered) else st + (8 if slot >= 8 else 4)
            gap = max(1, nxt - st)
            is_resp = st >= 16
            is_tail = i == len(ordered) - 1 or (not is_resp and (i + 1 >= len(ordered) or ordered[i + 1] >= 16))
            lens[trax][st] = len_for_gap(era, slot, gap, is_resp, is_tail)
            if i + 1 < len(ordered):
                ad = abs(pitches[trax][ordered[i + 1]] - pitches[trax][st])
                bends[trax][st] = min(12, ad) if ad >= (3 if era != MUTE else 2) else 0
            elif is_tail and era in (SPIRIT, FUSION, MODAL):
                bends[trax][st] = 2 if era != ACID else 0

        gate_cs[trax] = gate_cs_for(era, slot)

        sig = trax_signature(gates, pitches, trax)
        if sig in seen:
            flip = 16 + ((mix32(trax * 0x9E3779B9) >> 8) % 15)
            gates[trax] = set_lead_bit(gates[trax], flip, 1)
            pitches[trax][flip] = clamp_pitch(pitches[trax][flip] + 2)
            sig = trax_signature(gates, pitches, trax)
        seen.add(sig)

    return gates, pitches, lens, bends, gate_cs


def format_gate_row(vals: list[int]) -> str:
    parts = [f"0x{v:08X}u" for v in vals]
    lines = []
    for i in range(0, len(parts), 8):
        chunk = ", ".join(parts[i : i + 8])
        lines.append(f"    {chunk},")
    return "\n".join(lines)


def format_pitch_rows(rows: list[list[int]]) -> str:
    lines = []
    for trax, row in enumerate(rows):
        era = trax // PRESETS_PER_ERA
        slot = trax % PRESETS_PER_ERA
        a = ", ".join(f"{v:4d}" for v in row[:16])
        b = ", ".join(f"{v:4d}" for v in row[16:])
        lines.append(f"    {{ {a}, {b} }}, /* {ERA_NAMES[era]} {slot} */")
    return "\n".join(lines)


def format_u8_step_rows(rows: list[list[int]]) -> str:
    lines = []
    for row in rows:
        inner = ", ".join(f"{v:3d}" for v in row)
        lines.append(f"    {{ {inner} }},")
    return "\n".join(lines)


def format_gate_cs(vals: list[int]) -> str:
    parts = [f"{v}u" for v in vals]
    return "    " + ", ".join(parts)


def patch_header(
    gates: list[int],
    pitches: list[list[int]],
    lens: list[list[int]],
    bends: list[list[int]],
    gate_cs: list[int],
) -> None:
    text = HEADER.read_text()
    gate_block = format_gate_row(gates)
    pitch_block = format_pitch_rows(pitches)
    len_block = format_u8_step_rows(lens)
    bend_block = format_u8_step_rows(bends)
    cs_block = format_gate_cs(gate_cs)
    text = re.sub(
        rf"(static const uint32_t {PREFIX}_GATE\[JAZZ_TRAX_N\] = \{{)\s*.*?\s*\}};",
        rf"\1\n{gate_block}\n}};",
        text,
        count=1,
        flags=re.S,
    )
    text = re.sub(
        rf"(static const int8_t {PREFIX}_PITCH\[JAZZ_TRAX_N\]\[32\] = \{{)\n.*?\n\}};",
        rf"\1\n{pitch_block}\n}};",
        text,
        count=1,
        flags=re.S,
    )
    text = re.sub(
        rf"(static const uint8_t {PREFIX}_LEN\[JAZZ_TRAX_N\]\[32\] = \{{)\n.*?\n\}};",
        rf"\1\n{len_block}\n}};",
        text,
        count=1,
        flags=re.S,
    )
    text = re.sub(
        rf"(static const uint8_t {PREFIX}_BEND\[JAZZ_TRAX_N\]\[32\] = \{{)\n.*?\n\}};",
        rf"\1\n{bend_block}\n}};",
        text,
        count=1,
        flags=re.S,
    )
    text = re.sub(
        rf"(static const uint8_t {PREFIX}_GATE_CS\[JAZZ_TRAX_N\] = \{{)\s*[^}}]*\}};",
        rf"\1\n{cs_block}\n}};",
        text,
        count=1,
        flags=re.S,
    )
    HEADER.write_text(text)


def era_gate_stats(gates: list[int]) -> list[tuple[str, float]]:
    out: list[tuple[str, float]] = []
    for era in range(ERA_N):
        lo = era * PRESETS_PER_ERA
        hi = lo + PRESETS_PER_ERA
        avg = sum(len(gated_steps(gates[t])) for t in range(lo, hi)) / float(PRESETS_PER_ERA)
        out.append((ERA_NAMES[era], avg))
    return out


def sanity(gates: list[int], pitches: list[list[int]]) -> list[str]:
    errs: list[str] = []
    seen: dict[tuple[int, tuple[int, ...]], int] = {}
    for trax in range(TRAX_N):
        sig = trax_signature(gates, pitches, trax)
        if sig in seen:
            errs.append(f"duplicate GATE+PITCH trax {seen[sig]} and {trax}")
        else:
            seen[sig] = trax
        era = trax // PRESETS_PER_ERA
        slot = trax % PRESETS_PER_ERA
        n = len(gated_steps(gates[trax]))
        if slot < 8:
            if era == SPIRIT and not (6 <= n <= 10):
                errs.append(f"SPIRIT {slot}: {n} notes (want 6–10)")
            if era == FUSION and not (11 <= n <= 16):
                errs.append(f"FUSION {slot}: {n} notes (want 11–16)")
            if era == MUTE and not (2 <= n <= 4):
                errs.append(f"MUTE {slot}: {n} notes (want 2–4)")
            if era == MODAL and not (2 <= n <= 4):
                errs.append(f"MODAL {slot}: {n} notes (want 2–4)")
        if slot >= 8:
            prev = trax - 2
            if trax_signature(gates, pitches, prev) == sig:
                errs.append(f"ballad {trax} copies slot {prev % PRESETS_PER_ERA}")
        lo, hi = gates[trax] & 0xFFFF, (gates[trax] >> 16) & 0xFFFF
        if era in (SPIRIT, FUSION, BROKEN) and lo == hi:
            errs.append(f"{ERA_NAMES[era]} {slot}: identical gate halves")
    return errs


def main() -> int:
    if not HEADER.is_file():
        print(f"{HEADER}: not found", file=sys.stderr)
        return 2 if "--check" in sys.argv else 0

    gates, pitches, lens, bends, gate_cs = build()
    errs = sanity(gates, pitches)
    if errs and "--check" not in sys.argv:
        for e in errs:
            print(f"warn: {e}", file=sys.stderr)

    if "--check" in sys.argv:
        text = HEADER.read_text()
        cur_g, cur_p = parse_gate_pitch(text)
        cur_len = parse_u8_step_rows(text, f"{PREFIX}_LEN")
        cur_bend = parse_u8_step_rows(text, f"{PREFIX}_BEND")
        cur_cs = parse_gate_cs(text)
        if cur_g != gates or cur_p != pitches or cur_len != lens or cur_bend != bends or cur_cs != gate_cs:
            print(f"{HEADER.name} lead tables stale (run without --check)", file=sys.stderr)
            return 1
        if errs:
            for e in errs:
                print(f"sanity: {e}", file=sys.stderr)
            return 1
        return 0

    patch_header(gates, pitches, lens, bends, gate_cs)
    print(f"Patched {HEADER.name}: {TRAX_N} trax, gate/pitch/len/bend/gate_cs")
    for name, avg in era_gate_stats(gates):
        print(f"  {name}: avg {avg:.1f} gates/trax")
    print(f"  SPIRIT0 call/resp: {hex(gates[0] & 0xFFFF)} / {hex(gates[0] >> 16)}")
    if errs:
        print(f"  {len(errs)} sanity warn(s)")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
