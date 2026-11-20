#!/usr/bin/env python3
"""Tile 16-bit plate pattern masks to 32-bit (bar2 = bar1 copy) in genre data tables."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = [
    ROOT / "firmware/src/dub_data.h",
    ROOT / "firmware/src/house_data.h",
    ROOT / "firmware/src/heavy_psych_data.h",
]

HEX16 = re.compile(r"\b0x([0-9A-Fa-f]{4})\b")
ROW = re.compile(r'^\s*\{\s*"')
ALREADY32 = re.compile(r"\b0x([0-9A-Fa-f]{8})u?\b")


def tile_hex16(m: re.Match[str]) -> str:
    v = int(m.group(1), 16) & 0xFFFF
    return f"(0x{v:04X}u | ((uint32_t)0x{v:04X}u << 16))"


def process_file(path: Path) -> tuple[int, int]:
    text = path.read_text()
    lines = text.splitlines(keepends=True)
    rows_touched = 0
    masks_tiled = 0
    out: list[str] = []
    for line in lines:
        if not ROW.match(line):
            out.append(line)
            continue
        if ALREADY32.search(line) and " << 16" not in line:
            out.append(line)
            continue
        n_before = len(HEX16.findall(line))
        if n_before == 0:
            out.append(line)
            continue
        new_line, n = HEX16.subn(tile_hex16, line)
        out.append(new_line)
        rows_touched += 1
        masks_tiled += n
    path.write_text("".join(out))
    return rows_touched, masks_tiled


def main() -> int:
    total_rows = 0
    total_masks = 0
    for path in FILES:
        if not path.is_file():
            print(f"missing: {path}", file=sys.stderr)
            return 1
        rows, masks = process_file(path)
        print(f"{path.name}: {rows} rows, {masks} mask literals tiled")
        total_rows += rows
        total_masks += masks
    print(f"total: {total_rows} rows, {total_masks} masks")
    return 0


if __name__ == "__main__":
    sys.exit(main())
