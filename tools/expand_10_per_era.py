#!/usr/bin/env python3
"""Expand ROM presets 8→10 per era (+2 slow slots). Run from repo root."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "firmware" / "src"
OLD_PER, NEW_PER = 8, 10

# Slow plate names (≤7 chars, uppercase) — 2 per era, genre order matches era index
SLOW_NAMES: dict[str, list[tuple[str, str]]] = {
    "dub": [
        ("ISLAND", "KINGST"), ("BALLAD", "LULLABY"), ("ECHOES", "SPACED"),
        ("FOGGED", "RUSTED"), ("DEEPHT", "GHOST"), ("AFTERM", "DRIFTG"),
    ],
    "house": [
        ("SLOWJK", "SMOOTH"), ("SWAY", "GARAGE"), ("ACIDBM", "LULL"),
        ("DEEPER", "FLOAT"), ("FILTER", "WARM"), ("TECHNO", "MINIMAL"),
    ],
    "jazz": [
        ("BALLAD", "REPOSE"), ("MODAL", "STILL"), ("WHISPR", "CUPSO"),
        ("LATE", "BLUE"), ("COOL", "MISTY"), ("BROKEN", "HUSH"),
    ],
    "idm": [
        ("GLITCH", "PAUSE"), ("AMBI", "WISP"), ("CLICK", "SPACE"),
        ("TEXTUR", "GRAIN"), ("BROKEN", "LEVIT"), ("MINMAL", "HUSH"),
    ],
    "hiphop": [
        ("BOOM", "LAZY"), ("SOUL", "VIBE"), ("TRAP", "SLOW"),
        ("LOFI", "DUST"), ("DRUNK", "SWAY"), ("CHILL", "HUSH"),
    ],
    "heavy_psych": [
        ("DOOM", "Haze"), ("DESERT", "DRONE"), ("SPACE", "VOID"),
        ("FUZZ", "SLOW"), ("STONER", "Haze"), ("PSYCH", "TRIP"),
    ],
    "snowcrash": [
        ("SNOW", "DRIFT"), ("GLOSS", "MIST"), ("SUM", "Haze"),
        ("SUN", "GLOW"), ("ICE", "STILL"), ("TIDE", "SLOW"),
        ("HOOL", "VOID"), ("WARP", "DREAM"),
    ],
}

CONF_BUMPS = {
    "dub_conf.h": ("48u", "60u", "0x44554241u", "0x44554242u", "10 plates/era (60 total)"),
    "house_conf.h": ("48u", "60u", "0x484F553Fu", "0x484F5540u", "10 trax/era (60 total)"),
    "jazz_conf.h": ("48u", "60u", "0x4A415A5Au", "0x4A415A5Bu", "10 trax/era (60 total)"),
    "idm_conf.h": ("48u", "60u", "0x49444D40u", "0x49444D41u", "10 piece/era (60 total)"),
    "hiphop_conf.h": ("48u", "60u", "0x48495048u", "0x48495049u", "10 beat/era (60 total)"),
    "heavy_psych_conf.h": ("48u", "60u", "0x4856593Fu", "0x48565940u", "10 trax/era (60 total)"),
    "snowcrash_conf.h": ("64u", "80u", "0x534E4331u", "0x534E4332u", "10 slab/era (80 ROM)"),
}

PLATE_TABLES = {
    "dub_data.h": ("dub_plate_t", "DUB_PLATES", "DUB_PLATE_N"),
    "house_data.h": ("house_trax_t", "HOUSE_TRAX", "HOUSE_TRAX_N"),
    "jazz_data.h": ("jazz_trax_t", "JAZZ_TRAX", "JAZZ_TRAX_N"),
    "idm_data.h": ("idm_piece_t", "IDM_PIECE", "IDM_PIECE_N"),
    "hiphop_data.h": ("hiphop_beat_t", "HIPHOP_BEAT", "HIPHOP_BEAT_N"),
    "heavy_psych_data.h": ("heavy_psych_trax_t", "HEAVY_PSYCH_TRAX", "HEAVY_PSYCH_TRAX_N"),
    "snowcrash_data.h": ("snowcrash_slab_t", "SNOWCRASH_SLAB", "SNOWCRASH_SLAB_N"),
}

GENRE_KEY = {
    "dub_data.h": "dub",
    "house_data.h": "house",
    "jazz_data.h": "jazz",
    "idm_data.h": "idm",
    "hiphop_data.h": "hiphop",
    "heavy_psych_data.h": "heavy_psych",
    "snowcrash_data.h": "snowcrash",
}

BASS_WALK = {
    "dub_data.h": "DUB_BASS_WALK",
    "house_data.h": "HOUSE_BASS_WALK",
    "jazz_data.h": "JAZZ_BASS_WALK",
    "idm_data.h": "IDM_BASS_WALK",
    "hiphop_data.h": "HIPHOP_BASS_WALK",
    "heavy_psych_data.h": "HEAVY_PSYCH_BASS_WALK",
}

SLOW_WALK_A = [0, 0, 0, 0, 0, 0, 0, 0]
SLOW_WALK_B = [0, 0, 3, 0, 0, 0, 4, 0]

PLATE_SEQ_SLOW = (
    "    {{ {0},18,24,0,6,14 }}, {{0,0,0,0,0,14}}, {{0,0,0,0,0,0}}, {{0,0,10,-8,0,20}}, 10,0,3,0,0 }},"
    "    {{ {0},16,22,0,4,16 }}, {{0,0,0,0,0,12}}, {{0,-6,0,0,0,0}}, {{0,0,8,-10,0,18}}, 9,0,0,3,0 }},"
)


def parse_brace_nums(block: str) -> list[int]:
    block = re.sub(r"/\*.*?\*/", "", block, flags=re.S)
    out: list[int] = []
    for tok in block.replace("\n", " ").split(","):
        tok = tok.strip()
        if not tok:
            continue
        tok = re.sub(r"/\*.*?\*/", "", tok).strip()
        m = re.match(r"^(-?\d+)u?$", tok.split()[0])
        if m:
            out.append(int(m.group(1)))
    return out


def expand_flat(vals: list[int], per_old: int = OLD_PER, per_new: int = NEW_PER) -> list[int]:
    eras = len(vals) // per_old
    out: list[int] = []
    for e in range(eras):
        chunk = vals[e * per_old : (e + 1) * per_old]
        out.extend(chunk)
        a, b = chunk[6], chunk[7]
        out.append(slow_side(a))
        out.append(slow_side(b))
    return out


def slow_side(v: int) -> int:
    return max(0, min(127, v + 6))


def sparsify_mask_hex(m: int) -> int:
    """Thin 16-step gate mask: keep ~half the bits."""
    lo = m & 0xFFFF
    hi = (m >> 16) & 0xFFFF
    lo = lo & ((lo >> 1) | (lo << 1) | 0x8888) & 0xAAAA
    hi = hi & ((hi >> 1) | (hi << 1) | 0x8888) & 0xAAAA
    if lo == 0:
        lo = 0x0101
    return lo | (hi << 16)


def slowify_row(line: str, new_name: str) -> str:
    line = re.sub(r'"[A-Z][A-Z0-9_]{0,6}"', f'"{new_name}"', line, count=1)
    # bpm / density (6th and 7th numeric fields after name)
    nums = list(re.finditer(r",\s*(\d+)\s*,", line))
    if len(nums) >= 7:
        # fields: era,bass,voicing,tap,tank,bpm,density — find bpm at index 5 from first comma after name
        pass
    parts = line.split(",")
    # dub_plate: name, era, bass, voice, tap, tank, bpm, density, ...
    try:
        idx = 6  # bpm position in split after { "name"
        for field_i in (6, 7):
            if field_i < len(parts):
                m = re.search(r"(\d+)", parts[field_i])
                if m:
                    v = int(m.group(1))
                    if field_i == 6:
                        v = max(54, v - 14)
                    else:
                        v = max(58, v - 20)
                    parts[field_i] = re.sub(r"\d+", str(v), parts[field_i], count=1)
        for field_i in (8, 9):  # dly_c, rev_c
            if field_i < len(parts):
                m = re.search(r"(\d+)", parts[field_i])
                if m:
                    v = min(127, int(m.group(1)) + 12)
                    parts[field_i] = re.sub(r"\d+", str(v), parts[field_i], count=1)
    except (IndexError, ValueError):
        pass
    line = ",".join(parts)
    # sparsify hat mask (3rd hex from end before siren — rough: last 0x fields)
    for hm in re.finditer(r"0x([0-9A-Fa-f]+)", line):
        pass
    hexes = list(re.finditer(r"(0x[0-9A-Fa-f]+|0x[0-9A-Fa-f]+u\s*\|\s*\(\(uint32_t\)0x[0-9A-Fa-f]+u\s*<<\s*16\))", line))
    if hexes and "hat" not in line.lower():
        # last three 0x in dub row are sub,chord,siren — hat is 3rd mask field
        simple = [m.group(0) for m in re.finditer(r"0x[0-9A-Fa-f]+", line)]
        if len(simple) >= 3:
            hat_i = simple[2]
            if "<<" not in hat_i:
                v = int(hat_i, 16)
                nv = sparsify_mask_hex(v)
                line = line.replace(hat_i, f"0x{nv:04X}", 1)
    return line


def extract_plate_rows(body: str) -> list[str]:
    rows: list[str] = []
    for ln in body.splitlines():
        s = ln.strip()
        if not s or s.startswith("/*") and "{" not in s:
            if s.startswith("/*") and rows and rows[-1].rstrip().endswith("},"):
                continue
        if "{" in s and '"' in s:
            rows.append(ln.rstrip())
    return rows


def expand_plate_table(text: str, ty: str, name: str, count_sym: str, genre: str) -> str:
    pat = rf"(static const {re.escape(ty)} {name}\[{count_sym}\] = \{{)(.*?)(\n\}};)"
    m = re.search(pat, text, re.S)
    if not m:
        raise SystemExit(f"{name} table not found in genre {genre}")
    rows = extract_plate_rows(m.group(2))
    if len(rows) != 48 and len(rows) != 64:
        raise SystemExit(f"{name}: expected 48 or 64 rows, got {len(rows)}")
    per = OLD_PER
    eras = len(rows) // per
    names_list = SLOW_NAMES[genre]
    if len(names_list) != eras:
        raise SystemExit(f"{genre}: name eras {len(names_list)} != {eras}")
    out_rows: list[str] = []
    for e in range(eras):
        chunk = rows[e * per : (e + 1) * per]
        out_rows.extend(chunk)
        n0, n1 = names_list[e]
        out_rows.append(slowify_row(chunk[6], n0))
        out_rows.append(slowify_row(chunk[7], n1))
    new_body = "\n".join(out_rows) + "\n"
    return text[: m.start(2)] + "\n" + new_body + text[m.end(2) :]


def patch_flat_array(text: str, array_pat: str) -> str:
    m = re.search(array_pat, text, re.S)
    if not m:
        return text
    old = m.group(0)
    inner = old[old.index("{") + 1 : old.rindex("}")]
    vals = parse_brace_nums(inner)
    if len(vals) not in (48, 64):
        return text
    expanded = expand_flat(vals)
    # preserve formatting style
    if "\n" in inner and inner.count("\n") > 2:
        eras = len(vals) // OLD_PER
        lines = []
        per_line = NEW_PER
        for e in range(eras):
            chunk = expanded[e * per_line : (e + 1) * per_line]
            lines.append("    " + ", ".join(f"{v}u" if "u" in inner else str(v) for v in chunk) + ",")
        ty_m = re.match(r"static const (\w+) (\w+)", old)
        repl = f"static const {ty_m.group(1)} {ty_m.group(2)}[{ty_m.group(2).split('_')[0]}_N or placeholder] = {{\n" + "\n".join(lines) + "\n};"
        # simpler: single line if was compact
    sep = ", "
    if "u" in inner:
        body = sep.join(f"{v}u" for v in expanded)
    else:
        body = sep.join(str(v) for v in expanded)
    repl = old[: old.index("{") + 1] + body + old[old.rindex("}") :]
    return text.replace(old, repl, 1)


def patch_all_flat_arrays(text: str, count_syms: tuple[str, ...]) -> str:
    for cs in count_syms:
        for m in list(re.finditer(rf"static const (\w+) (\w+)\[{cs}\] = \{{[^}}]*\}};", text, re.S)):
            old = m.group(0)
            if "BASS_WALK" in old or "LEAD_PITCH" in old or "LEAD_LEN" in old or "LEAD_BEND" in old:
                continue
            inner = old[old.index("{") + 1 : old.rindex("}")]
            vals = parse_brace_nums(inner)
            if len(vals) not in (48, 64):
                continue
            expanded = expand_flat(vals)
            if "u" in inner or "uint8_t" in old or "uint32_t" in old:
                body = ", ".join(f"{v}u" for v in expanded)
            else:
                body = ", ".join(str(v) for v in expanded)
            repl = old[: old.index("{") + 1] + body + old[old.rindex("}") :]
            text = text.replace(old, repl, 1)
    return text


def expand_bass_walk(text: str, sym: str) -> str:
    pat = rf"(static const uint8_t {sym}\[[^\]]+\]\[[^\]]+\]\[8\] = \{{)(.*?)(\n\}};)"
    m = re.search(pat, text, re.S)
    if not m:
        return text
    inner = m.group(2)
    era_blocks = re.findall(r"\{[^{}]*(?:\{[^{}]*\}[^{}]*)*\}", inner, re.S)
    if not era_blocks:
        return text
    new_blocks = []
    for eb in era_blocks:
        walks = re.findall(r"\{[^{}]+\}", eb)
        if len(walks) != OLD_PER:
            raise SystemExit(f"{sym}: era has {len(walks)} walks")
        new_eb = "    { " + ", ".join(walks) + ", "
        new_eb += "{ " + ", ".join(str(x) for x in SLOW_WALK_A) + " }, "
        new_eb += "{ " + ", ".join(str(x) for x in SLOW_WALK_B) + " } }"
        new_blocks.append(new_eb)
    new_inner = "\n".join(new_blocks) + "\n"
    return text[: m.start(2)] + "\n" + new_inner + text[m.end(2) :]


def expand_lead_2d(text: str, name: str, cols: int) -> str:
    pat = rf"(static const \w+ {name}\[[^\]]+\]\[{cols}\] = \{{)(.*?)(\n\}};)"
    m = re.search(pat, text, re.S)
    if not m:
        return text
    rows_raw = re.findall(r"\{[^{}]+\}", m.group(2))
    if len(rows_raw) not in (48, 64):
        return text
    out = []
    for e in range(len(rows_raw) // OLD_PER):
        chunk = rows_raw[e * OLD_PER : (e + 1) * OLD_PER]
        out.extend(chunk)
        out.append(chunk[6])
        out.append(chunk[7])
    new_body = "\n    " + ",\n    ".join(out) + ",\n"
    return text[: m.start(2)] + new_body + text[m.end(2) :]


def patch_conf(path: Path, old_n: str, new_n: str, old_magic: str, new_magic: str, comment: str) -> None:
    t = path.read_text()
    t = re.sub(r"#define GENRE_PRESETS_PER_ERA\s+\d+u", "#define GENRE_PRESETS_PER_ERA 10u", t)
    t = t.replace(f"#define GENRE_PRESET_N   {old_n}", f"#define GENRE_PRESET_N   {new_n}")
    t = t.replace(old_magic, f"{new_magic} /* bump: {comment} */")
    if "snowcrash_conf" in path.name:
        if "GENRE_CFG_USER_PLATE_N" in t:
            t = re.sub(
                r"(#define GENRE_CFG_USER_PLATE_N \d+u).*",
                r"\1 /* ROM 80 SLAB; flash user saves ≤64 (uint64_t mask) */",
                t,
            )
    path.write_text(t)


def expand_plate_seq_fm_app(text: str) -> str:
    pat = r"(static const dub_plate_seq_t (\w+)\[GENRE_PRESET_N\] = \{)(.*?)(\n\};)"
    for m in list(re.finditer(pat, text, re.S)):
        rows = [ln for ln in m.group(3).splitlines() if ln.strip().startswith("{")]
        if len(rows) not in (48, 64):
            continue
        out: list[str] = []
        for e in range(len(rows) // OLD_PER):
            chunk = rows[e * OLD_PER : (e + 1) * OLD_PER]
            out.extend(chunk)
            out.append(PLATE_SEQ_SLOW[0].format(0))
            out.append(PLATE_SEQ_SLOW[1].format(0))
        body = "\n".join(out) + "\n"
        text = text[: m.start(3)] + "\n" + body + text[m.end(3) :]
    return text


def main() -> None:
    for conf_name, (old_n, new_n, old_mag, new_mag, cmt) in CONF_BUMPS.items():
        patch_conf(SRC / conf_name, old_n, new_n, old_mag, new_mag, cmt)

    dt = (SRC / "dub_types.h").read_text()
    dt = dt.replace("DUB_PLATE_N = 48", "DUB_PLATE_N = 60")
    (SRC / "dub_types.h").write_text(dt)

    count_syms = ("DUB_PLATE_N", "HOUSE_TRAX_N", "JAZZ_TRAX_N", "IDM_PIECE_N", "HIPHOP_BEAT_N",
                  "HEAVY_PSYCH_TRAX_N", "SNOWCRASH_SLAB_N", "GENRE_PRESET_N")

    for fname, (ty, tname, csym) in PLATE_TABLES.items():
        path = SRC / fname
        t = path.read_text()
        genre = GENRE_KEY[fname]
        t = expand_plate_table(t, ty, tname, csym, genre)
        t = patch_all_flat_arrays(t, count_syms)
        if fname in BASS_WALK:
            t = expand_bass_walk(t, BASS_WALK[fname])
        if fname in ("jazz_data.h", "idm_data.h", "hiphop_data.h", "heavy_psych_data.h", "snowcrash_data.h"):
            for lead in ("LEAD_PITCH", "LEAD_LEN", "LEAD_BEND"):
                if lead == "LEAD_PITCH":
                    t = expand_lead_2d(t, f"{tname.split('_')[0]}_{'PIECE' if 'idm' in fname else 'TRAX' if 'jazz' in fname or 'heavy' in fname else 'BEAT' if 'hiphop' in fname else 'SLAB'}_{lead}", 16 if lead == "LEAD_PITCH" else 32)
            # fix lead table names per file
            prefix = {
                "jazz_data.h": "JAZZ_TRAX",
                "idm_data.h": "IDM_PIECE",
                "hiphop_data.h": "HIPHOP_BEAT",
                "heavy_psych_data.h": "HEAVY_PSYCH_TRAX",
                "snowcrash_data.h": "SNOWCRASH_SLAB",
            }[fname]
            t = expand_lead_2d(t, f"{prefix}_LEAD_PITCH", 16)
            t = expand_lead_2d(t, f"{prefix}_LEAD_LEN", 32)
            t = expand_lead_2d(t, f"{prefix}_LEAD_BEND", 32)
        path.write_text(t)
        print(f"OK {fname}")

    fm = (SRC / "fm_app.c").read_text()
    fm = fm.replace("return (i / 6u) * 8u + (i % 6u);", "return (i / 6u) * GENRE_PRESETS_PER_ERA + (i % 6u);")
    fm = fm.replace("g.cfg.p.plate % 8u", "g.cfg.p.plate % GENRE_PRESETS_PER_ERA")
    fm = expand_plate_seq_fm_app(fm)
    # Dub magic chain: prev = 48-plate magic
    fm = fm.replace("#define DUB_CFG_MAGIC_PREV 0x4455423Fu", "#define DUB_CFG_MAGIC_PREV 0x44554241u")
    (SRC / "fm_app.c").write_text(fm)

    print("expand_10_per_era: done — run gen_views.py and add 48→60 remap in fm_app.c if missing")


if __name__ == "__main__":
    main()
