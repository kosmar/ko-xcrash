#!/usr/bin/env python3
PRESETS_PER_ERA = 10
# SPDX-License-Identifier: GPL-3.0-only
"""Rewrite IDM_PIECE[].sub with 48 unique 32-step gate masks (era-appropriate density)."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
IDM_H = ROOT / "firmware" / "src" / "idm_data.h"

M32 = 0xFFFFFFFF


def mix32(x: int) -> int:
    x &= M32
    x ^= x >> 16
    x = (x * 0x7FEB352D) & M32
    x ^= x >> 15
    x = (x * 0x846CA68B) & M32
    return x ^ (x >> 16)


def pop32(m: int) -> int:
    m &= M32
    c = 0
    while m:
        c += m & 1
        m >>= 1
    return c


def fmt_mask(m: int) -> str:
    m &= M32
    lo, hi = m & 0xFFFF, (m >> 16) & 0xFFFF
    return f"0x{lo:04X}u | ((uint32_t)0x{hi:04X}u << 16)"


def density_range(era: int, slot: int) -> tuple[int, int]:
    if era in (0, 1):  # WARM, MELO
        return 8, 14
    if era in (2, 3):  # GLITCH, BREAK
        return 10, 16
    if era == 4:  # DRILL
        return 14, 20
    # OBLIQ: alternate sparse / dense by slot
    if slot & 1:
        return 6, 11
    return 15, 20


def gen_sub_mask(piece: int, era: int, slot: int, used: set[int]) -> int:
    for attempt in range(512):
        seed = mix32(piece * 0x9E3779B1 + attempt * 0x517CC1B7 + era * 0x85EBCA6B)
        lo_target = mix32(seed ^ 0xA2B4C6D8) % 17
        hi_target = mix32(seed ^ 0x13579BDF) % 17
        lo_min, lo_max = density_range(era, slot)
        hi_min, hi_max = lo_min, lo_max
        if era == 5 and slot & 1:
            hi_min, hi_max = 6, 11
        else:
            hi_min, hi_max = lo_min, lo_max
        want_lo = lo_min + (mix32(seed) % (lo_max - lo_min + 1))
        want_hi = hi_min + (mix32(seed >> 8) % (hi_max - hi_min + 1))

        lo, hi = 0, 0
        # Anchor: step 0 often on for bass feel (not always for sparse OBLIQ)
        if era != 5 or slot % 3 != 1:
            lo |= 1
        candidates = list(range(32))
        order = sorted(candidates, key=lambda s: mix32(seed + s * 7919))
        for step in order:
            bit = step & 15
            word = lo if step < 16 else hi
            if word & (1 << bit):
                continue
            r = mix32(seed + step * 0xC2B2AE35) & 255
            thresh = 90 if era >= 4 else 72 if era >= 2 else 58
            if r < thresh:
                if step < 16:
                    lo |= 1 << bit
                else:
                    hi |= 1 << bit
        # Trim / boost to target density
        for _ in range(64):
            total = pop32(lo | (hi << 16))
            want = want_lo + want_hi
            if total == want:
                break
            if total > want:
                step = mix32(seed + _) % 32
                bit = step & 15
                if step == 0 and era < 5:
                    continue
                if step < 16:
                    if lo & (1 << bit):
                        lo &= ~(1 << bit)
                elif hi & (1 << bit):
                    hi &= ~(1 << bit)
            else:
                step = mix32(seed + 0x1000 + _) % 32
                bit = step & 15
                if step < 16:
                    lo |= 1 << bit
                else:
                    hi |= 1 << bit
        mask = lo | (hi << 16)
        if mask == 0:
            mask = 1
        if mask not in used:
            used.add(mask)
            return mask
    raise SystemExit(f"could not find unique sub for piece {piece}")


ROW = re.compile(r'^\s*(?:/\*.*?\*/\s*)?\{\s*"([^"]+)"\s*,(.*)\}\s*,?\s*$')
COMMENT = re.compile(r"/\*.*?\*/|//.*$")
MASK = re.compile(
    r"0x[0-9A-Fa-f]+u\s*\|\s*\(\(uint32_t\)0x[0-9A-Fa-f]+u\s*<<\s*16\)"
)


def patch_subs(text: str) -> tuple[str, list[int]]:
    used: set[int] = set()
    subs: list[int] = []

    lines = text.splitlines()
    out_lines = []
    in_table = False
    for line in lines:
        if "static const idm_piece_t IDM_PIECE" in line:
            in_table = True
            out_lines.append(line)
            continue
        if in_table and line.strip() == "};":
            in_table = False
            out_lines.append(line)
            continue
        if in_table:
            clean = COMMENT.sub("", line)
            row = ROW.match(clean)
            if row:
                fields = [f.strip() for f in row.group(2).split(",") if f.strip()]
                if len(fields) != 19:
                    raise SystemExit(f"IDM_PIECE {row.group(1)}: {len(fields)} fields")
                piece = len(subs)
                era = piece // PRESETS_PER_ERA
                slot = piece % PRESETS_PER_ERA
                mask = gen_sub_mask(piece, era, slot, used)
                subs.append(mask)
                fields[16] = fmt_mask(mask)
                indent = re.match(r"^(\s*)", line).group(1)
                comment = re.search(r"/\*.*?\*/", line)
                cmt = (comment.group(0) + " ") if comment else ""
                inner = ", ".join(fields)
                out_lines.append(f'{indent}{cmt}{{ "{row.group(1)}", {inner} }},')
                continue
        out_lines.append(line)
    if len(subs) != 48:
        raise SystemExit(f"expected 48 pieces, patched {len(subs)}")
    return "\n".join(out_lines) + ("\n" if text.endswith("\n") else ""), subs


def main() -> int:
    text = IDM_H.read_text()
    block = text.split("static const idm_piece_t IDM_PIECE")[1].split("};")[0]
    hexes = re.findall(
        r"(0x[0-9A-Fa-f]+u)\s*\|\s*\(\(uint32_t\)(0x[0-9A-Fa-f]+u)\s*<<\s*16\)", block
    )

    def parse_u(h: str) -> int:
        return int(h[:-1], 16)

    before = []
    for i in range(0, len(hexes), 6):
        lo, hi = hexes[i + 3]
        before.append(parse_u(lo) | (parse_u(hi) << 16))
    print(f"unique sub before: {len(set(before))}/48")

    new_text, after = patch_subs(text)
    print(f"unique sub after:  {len(set(after))}/48")
    IDM_H.write_text(new_text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
