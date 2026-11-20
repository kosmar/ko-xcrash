#!/usr/bin/env python3
"""Expand dub_plate_seq_t tables 8→10 per era in fm_app.c."""
from __future__ import annotations
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FM = ROOT / "firmware" / "src" / "fm_app.c"
OLD, NEW = 8, 10

SLOW = (
    "    { {0,18,24,0,6,14}, {0,0,0,0,0,14}, {0,0,0,0,0,0}, {0,0,10,-8,0,20}, 10,0,3,0,0 },",
    "    { {0,16,22,0,4,16}, {0,0,0,0,0,12}, {0,-6,0,0,0,0}, {0,0,8,-10,0,18}, 9,0,0,3,0 },",
)


def expand_array(text: str, name: str) -> str:
    pat = rf"(static const dub_plate_seq_t {name}\[GENRE_PRESET_N\] = \{{)(.*?)(\n\}};)"
    m = re.search(pat, text, re.S)
    if not m:
        print(f"skip {name}")
        return text
    rows = [ln for ln in m.group(2).splitlines() if ln.strip().startswith("{")]
    n = len(rows)
    if n not in (48, 64):
        raise SystemExit(f"{name}: expected 48/64 rows, got {n}")
    out: list[str] = []
    for e in range(n // OLD):
        chunk = rows[e * OLD : (e + 1) * OLD]
        out.extend(chunk)
        out.extend(SLOW)
    body = "\n".join(out) + "\n"
    return text[: m.start(2)] + "\n" + body + text[m.end(2) :]


def main() -> None:
    t = FM.read_text()
    for name in (
        "DUB_PLATE_SEQ",
        "HEAVY_PSYCH_PLATE_SEQ",
        "HOUSE_PLATE_SEQ",
        "JAZZ_PLATE_SEQ",
        "IDM_PLATE_SEQ",
        "HIPHOP_PLATE_SEQ",
        "SNOWCRASH_PLATE_SEQ",
    ):
        t = expand_array(t, name)
    t = t.replace("return (i / 6u) * 8u + (i % 6u);", "return (i / 6u) * GENRE_PRESETS_PER_ERA + (i % 6u);")
    t = t.replace("g.cfg.p.plate % 8u", "g.cfg.p.plate % GENRE_PRESETS_PER_ERA")
    FM.write_text(t)
    print("expand_plate_seq_10: fm_app.c OK")


if __name__ == "__main__":
    main()
