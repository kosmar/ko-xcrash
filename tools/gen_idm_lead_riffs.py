#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Regenerate IDM_PIECE_LEAD_LEN/BEND in firmware/src/idm_data.h from authored GATE/PITCH.

Lean phrase helper (HP/Jazz 32-step gate bitmap + per-step pitch/len/bend).
Eras: WARM · MELO · GLITCH · BREAK · DRILL · OBLIQ (6×8 = 48 pieces).

  python3 tools/gen_idm_lead_riffs.py          # patch idm_data.h
  python3 tools/gen_idm_lead_riffs.py --check  # exit 1 if LEN/BEND stale or off-gate len/bend

Expected header symbols:
  IDM_PIECE_N, IDM_ERA_N
  static const uint32_t IDM_PIECE_LEAD_GATE[IDM_PIECE_N];
  static const int8_t IDM_PIECE_LEAD_PITCH[IDM_PIECE_N][16];
  static const uint8_t IDM_PIECE_LEAD_LEN[IDM_PIECE_N][32];
  static const uint8_t IDM_PIECE_LEAD_BEND[IDM_PIECE_N][32];
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "firmware/src/idm_data.h"

PIECE_N = 48
ERA_N = 6
SEQ_STEPS = 32
FILL_STEPS = frozenset(range(28, 32))
ERA_NAMES = ("WARM", "MELO", "GLITCH", "BREAK", "DRILL", "OBLIQ")

LEN_16THS = (1, 2, 4, 5, 8, 11, 12, 16, 20, 24, 28, 32, 40, 48, 64)
LONG_LEN_IX_MIN = 9
LEN_IX_MAX = len(LEN_16THS) - 1
MAX_MELODY_JUMP = 5
PITCH_MIN = -5
PITCH_MAX = 5

PREFIX = "IDM_PIECE_LEAD"


def mix32(x: int) -> int:
    x &= 0xFFFFFFFF
    x ^= x >> 16
    x = (x * 0x7FEB352D) & 0xFFFFFFFF
    x ^= x >> 15
    x = (x * 0x846CA68B) & 0xFFFFFFFF
    return x ^ (x >> 16)


def parse_gate_pitch(text: str) -> tuple[list[int], list[list[int]]]:
    m = re.search(rf"{PREFIX}_GATE\[.*?\]\s*=\s*\{{(.*?)\}};", text, re.S)
    if not m:
        raise ValueError(f"missing {PREFIX}_GATE in {HEADER.name}")
    # Accept plain 0x….u or MMIO-safe ((uint32_t)lo | ((uint32_t)hi << 16))
    gates = []
    for gm in re.finditer(
        r"\(\(uint32_t\)(0x[0-9A-Fa-f]+)u\s*\|\s*\(\(uint32_t\)(0x[0-9A-Fa-f]+)u\s*<<\s*16\)\)"
        r"|(0x[0-9A-Fa-f]+)u?",
        m.group(1),
    ):
        if gm.group(1) is not None:
            gates.append(int(gm.group(1), 0) | (int(gm.group(2), 0) << 16))
        else:
            gates.append(int(gm.group(3), 0))
    m2 = re.search(rf"{PREFIX}_PITCH\[.*?\]\s*=\s*\{{(.*?)\}};", text, re.S)
    if not m2:
        raise ValueError(f"missing {PREFIX}_PITCH in {HEADER.name}")
    rows: list[list[int]] = []
    for line in m2.group(1).split("\n"):
        code = line.split("/*", 1)[0]
        nums = [int(x.strip().rstrip(",")) for x in re.findall(r"-?\d+", code)]
        if len(nums) >= 16:
            rows.append(nums[:16])
    if len(gates) != PIECE_N or len(rows) != PIECE_N:
        raise ValueError(f"expected {PIECE_N} gate/pitch rows, got {len(gates)}/{len(rows)}")
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


def pitch_at(piece: int, step: int, pitches: list[list[int]]) -> int:
    return pitches[piece][step & 15]


def next_gated_pitch_step(
    piece: int, from_step: int, g32: int, pitches: list[list[int]]
) -> tuple[int | None, int | None]:
    for k in range(1, SEQ_STEPS + 1):
        ns = (from_step + k) % SEQ_STEPS
        if not step_bit(g32, ns):
            continue
        return ns, pitch_at(piece, ns, pitches)
    return None, None


def hold_run_len(piece: int, step: int, g32: int, pitches: list[list[int]]) -> int:
    if not step_bit(g32, step):
        return 0
    p0 = pitch_at(piece, step, pitches)
    run = 1
    for k in range(1, SEQ_STEPS):
        ns = step + k
        if ns >= SEQ_STEPS:
            break
        if not step_bit(g32, ns):
            break
        if pitch_at(piece, ns, pitches) != p0:
            break
        run += 1
    return run


def sixteenths_to_len_ix(n: int) -> int:
    ix = 0
    for i, s in enumerate(LEN_16THS):
        if s <= n:
            ix = i
    return ix


def len_ix_for_step(piece: int, step: int, g32: int, pitches: list[list[int]]) -> int:
    if not step_bit(g32, step):
        return 0
    run = hold_run_len(piece, step, g32, pitches)
    ix = sixteenths_to_len_ix(run)
    if step in FILL_STEPS:
        if run >= 8:
            ix = max(ix, 13)
        elif run >= 4:
            ix = max(ix, 11)
        else:
            ix = max(ix, LONG_LEN_IX_MIN)
        return min(ix, LEN_IX_MAX)
    if run >= 2:
        if ix >= LONG_LEN_IX_MIN:
            return min(ix, LEN_IX_MAX)
        return max(1, min(ix, 7))
    return mix32((piece << 6) ^ (step << 2) ^ 0x4944) & 1


def bend_for_step(piece: int, step: int, g32: int, pitches: list[list[int]]) -> int:
    if not step_bit(g32, step):
        return 0
    cur = pitch_at(piece, step, pitches)
    _ns, nxt = next_gated_pitch_step(piece, step, g32, pitches)
    if nxt is None or nxt == cur:
        return 0
    ad = abs(nxt - cur)
    if ad < 3:
        return 0
    return min(12, ad)


def boost_melodic_legato_lens(
    piece: int, g32: int, pitches: list[list[int]], lens: list[list[int]]
) -> None:
    for step in range(SEQ_STEPS):
        if not step_bit(g32, step):
            continue
        cur = pitch_at(piece, step, pitches)
        ns, nxt = next_gated_pitch_step(piece, step, g32, pitches)
        if ns is None:
            continue
        gap = (ns - step) if ns > step else (SEQ_STEPS - step + ns)
        if gap > 4:
            continue
        if abs(nxt - cur) > MAX_MELODY_JUMP:
            continue
        li = lens[piece][step]
        if abs(nxt - cur) <= 2:
            li = max(li, LONG_LEN_IX_MIN + 1)
        elif abs(nxt - cur) <= MAX_MELODY_JUMP:
            li = max(li, LONG_LEN_IX_MIN)
        lens[piece][step] = min(li, LEN_IX_MAX)


def verify_off_gate_len_bend(
    gates: list[int], lens: list[list[int]], bends: list[list[int]]
) -> list[str]:
    """LEN must be 0 and BEND must be 0 on steps where GATE is off."""
    errs: list[str] = []
    for piece in range(PIECE_N):
        g = gates[piece]
        for step in range(SEQ_STEPS):
            if step_bit(g, step):
                continue
            if lens[piece][step] != 0:
                errs.append(
                    f"piece {piece} step {step}: len={lens[piece][step]} off-gate"
                )
            if bends[piece][step] != 0:
                errs.append(
                    f"piece {piece} step {step}: bend={bends[piece][step]} off-gate"
                )
    return errs


def build_len_bend(
    gates: list[int], pitches: list[list[int]]
) -> tuple[list[list[int]], list[list[int]]]:
    lens = [[0] * SEQ_STEPS for _ in range(PIECE_N)]
    bends = [[0] * SEQ_STEPS for _ in range(PIECE_N)]
    for piece in range(PIECE_N):
        g = gates[piece]
        for step in range(SEQ_STEPS):
            lens[piece][step] = len_ix_for_step(piece, step, g, pitches)
            bends[piece][step] = bend_for_step(piece, step, g, pitches)
        boost_melodic_legato_lens(piece, g, pitches, lens)
    return lens, bends


def build() -> tuple[list[int], list[list[int]], list[list[int]], list[list[int]]]:
    text = HEADER.read_text()
    gates, pitches = parse_gate_pitch(text)
    lens, bends = build_len_bend(gates, pitches)
    return gates, pitches, lens, bends


def format_u8_step_rows(rows: list[list[int]]) -> str:
    lines = []
    for row in rows:
        inner = ", ".join(f"{v:3d}" for v in row)
        lines.append(f"    {{ {inner} }},")
    return "\n".join(lines)


def patch_header(
    _gates: list[int],
    _pitches: list[list[int]],
    lens: list[list[int]],
    bends: list[list[int]],
) -> None:
    text = HEADER.read_text()
    len_block = format_u8_step_rows(lens)
    bend_block = format_u8_step_rows(bends)
    text = re.sub(
        rf"(static const uint8_t {PREFIX}_LEN\[IDM_PIECE_N\]\[32\] = \{{)\n.*?\n\}};",
        rf"\1\n{len_block}\n}};",
        text,
        count=1,
        flags=re.S,
    )
    text = re.sub(
        rf"(static const uint8_t {PREFIX}_BEND\[IDM_PIECE_N\]\[32\] = \{{)\n.*?\n\}};",
        rf"\1\n{bend_block}\n}};",
        text,
        count=1,
        flags=re.S,
    )
    HEADER.write_text(text)


def main() -> int:
    if not HEADER.is_file():
        print(f"{HEADER}: not found", file=sys.stderr)
        return 2

    if "--check" in sys.argv:
        gates, pitches, lens, bends = build()
        text = HEADER.read_text()
        cur_len = parse_u8_step_rows(text, f"{PREFIX}_LEN")
        cur_bend = parse_u8_step_rows(text, f"{PREFIX}_BEND")
        if cur_len != lens or cur_bend != bends:
            print(f"{HEADER.name} LEN/BEND stale (run without --check)", file=sys.stderr)
            return 1
        off_gate = verify_off_gate_len_bend(gates, cur_len, cur_bend)
        if off_gate:
            print(
                f"{HEADER.name}: {len(off_gate)} off-gate LEN/BEND violations",
                file=sys.stderr,
            )
            for msg in off_gate[:8]:
                print(f"  {msg}", file=sys.stderr)
            return 1
        return 0

    gates, pitches, lens, bends = build()
    patch_header(gates, pitches, lens, bends)
    print(f"Patched {HEADER.name}: {PIECE_N} pieces, len/bend from gate/pitch")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
