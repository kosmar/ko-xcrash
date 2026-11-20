#!/usr/bin/env python3
"""Rebuild fm_app PLATE_SEQ: 40→48 (LOVERS insert) → 60 (+2 slow/era)."""
from __future__ import annotations
import re
from pathlib import Path

FM = Path(__file__).resolve().parents[1] / "firmware" / "src" / "fm_app.c"
OLD_PER, NEW_PER = 8, 10

LOVERS_DUB = [
    "    { {0,10,14,0,4,6}, {0,0,0,0,0,-4}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,3,3,0 },",
    "    { {0,8,12,0,4,8}, {0,0,-4,0,0,-6}, {0,-6,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,0,0,3,0 },",
    "    { {0,12,16,0,4,10}, {0,0,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 11,0,3,3,0 },",
    "    { {0,10,14,0,4,6}, {0,-4,0,0,0,-6}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 10,0,0,3,3 },",
    "    { {0,8,12,0,4,8}, {0,0,0,0,0,-8}, {0,0,-4,0,0,0}, {0,0,-4,-6,0,-8}, 9,3,0,3,0 },",
    "    { {0,14,18,0,6,10}, {0,0,0,0,0,-6}, {0,-8,0,0,0,0}, {0,0,-6,-8,0,-10}, 10,0,3,0,0 },",
    "    { {0,10,16,0,4,8}, {0,0,-6,0,0,-8}, {0,0,0,0,0,0}, {0,0,-4,-6,0,-8}, 9,0,0,3,3 },",
    "    { {0,12,14,0,4,6}, {0,0,0,0,4,-6}, {0,0,0,0,0,0}, {0,0,-6,-8,0,-10}, 11,0,3,3,0 },",
]

SLOW = (
    "    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },",
    "    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },",
)


def insert_lovers(rows: list[str]) -> list[str]:
    if len(rows) != 40:
        return rows
    out = rows[0:8] + LOVERS_DUB + rows[8:40]
    return out


def expand_10(rows: list[str]) -> list[str]:
    n = len(rows)
    if n not in (48, 64):
        raise SystemExit(f"expand_10: expected 48/64 rows, got {n}")
    out: list[str] = []
    for e in range(n // OLD_PER):
        chunk = rows[e * OLD_PER : (e + 1) * OLD_PER]
        out.extend(chunk)
        out.extend(SLOW)
    return out


def patch_array(text: str, name: str, lovers: bool) -> str:
    pat = rf"(static const dub_plate_seq_t {name}\[GENRE_PRESET_N\] = \{{)(.*?)(\n\}};)"
    m = re.search(pat, text, re.S)
    if not m:
        print(f"skip {name}")
        return text
    rows = [ln for ln in m.group(2).splitlines() if ln.strip().startswith("{")]
    if lovers and len(rows) == 40:
        rows = insert_lovers(rows)
    rows = expand_10(rows)
    body = "\n".join(rows) + "\n"
    return text[: m.start(2)] + "\n" + body + text[m.end(2) :]


def main() -> None:
    t = FM.read_text()
    for name, lovers in (
        ("DUB_PLATE_SEQ", True),
        ("HEAVY_PSYCH_PLATE_SEQ", False),
        ("HOUSE_PLATE_SEQ", False),
        ("JAZZ_PLATE_SEQ", False),
        ("IDM_PLATE_SEQ", False),
        ("HIPHOP_PLATE_SEQ", False),
        ("SNOWCRASH_PLATE_SEQ", False),
    ):
        t = patch_array(t, name, lovers)
    t = t.replace("return (i / 6u) * 8u + (i % 6u);", "return (i / 6u) * GENRE_PRESETS_PER_ERA + (i % 6u);")
    t = t.replace("g.cfg.p.plate % 8u", "g.cfg.p.plate % GENRE_PRESETS_PER_ERA")
    FM.write_text(t)
    print("rebuild_plate_seq_60: OK")


if __name__ == "__main__":
    main()
