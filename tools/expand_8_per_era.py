#!/usr/bin/env python3
"""One-shot: expand dub/house preset tables 6→8 per era. Run from repo root."""
from __future__ import annotations
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OLD_PER = 6
NEW_PER = 8

# --- New DUB plates (2 per era, appended after existing 6 in each era) ---
DUB_NEW = [
    # era 0
    '    { "COXSONE", 0, BASS_SUB,    VOICE_MIN7,  1, 16,  76,  108, 38, 30, 42, 46,  4, 18, 0x0101, 0x0010, 0x4444, 0x1081, 0x4104, 0x0040 },',
    '    { "JAMMY",   0, BASS_808,    VOICE_MIN9,  0, 12,  82,  116, 42, 32, 44, 52,  6, 16, 0x1110, 0x0100, 0x5555, 0x0909, 0x1010, 0x8000 },',
    # era 1
    '    { "BASIC",   1, BASS_SUB,    VOICE_MIN7,  3, 90,  126,  98, 56, 50, 50, 70, 12, 38, 0x1111, 0x0100, 0x4444, 0x0001, 0x0104, 0x0000 },',
    '    { "MK",      1, BASS_PWM,    VOICE_MIN9,  4,100,  130, 104, 50, 52, 54, 76, 20, 44, 0x1111, 0x1000, 0x5555, 0x0181, 0x4010, 0x4000 },',
    # era 2
    '    { "TRICKY",  2, BASS_REESE,  VOICE_SUS4,  3, 48,  114, 104, 40, 32, 40, 50, 16, 50, 0x1101, 0x0208, 0x2222, 0x2121, 0x0202, 0x4000 },',
    '    { "RSD",     2, BASS_808,    VOICE_MIN9,  2, 52,  120, 112, 36, 30, 38, 54, 12, 46, 0x0401, 0x1010, 0x4444, 0x1411, 0x2010, 0x0000 },',
    # era 3
    '    { "DMZ",     3, BASS_WOBBLE, VOICE_FIFTH, 1, 70,  140,  80, 38, 26, 40, 34, 80, 26, 0x0101, 0x0100, 0x0404, 0x0101, 0x0001, 0x0000 },',
    '    { "FLUX",    3, BASS_FM,     VOICE_MIN7,  2, 74,  138,  88, 36, 22, 42, 48, 90, 24, 0x0801, 0x0100, 0x5050, 0x0305, 0x0010, 0x8000 },',
    # era 4
    '    { "ROMARE",  4, BASS_WG,     VOICE_QUART, 4, 88,  118,  86, 46, 44, 50, 52, 28, 22, 0x0841, 0x4040, 0x0909, 0x0421, 0x2080, 0x4000 },',
    '    { "LEFTLD",  4, BASS_SUB,    VOICE_MIN9,  3,102,  124,  92, 48, 46, 46, 42, 22, 26, 0x0201, 0x8080, 0x2929, 0x0109, 0x0204, 0x0000 },',
]

DUB_SIDE_NEW: dict[str, list[int]] = {
    "plate_roots": [54, 56, 49, 54, 51, 48, 52, 55, 47, 51],
    "plate_scales": [6, 7, 1, 0, 2, 1, 0, 2, 3, 7],
    "plate_swings": [57, 56, 51, 52, 59, 56, 51, 50, 57, 56],
    "plate_dly_times": [60, 55, 70, 68, 61, 58, 49, 47, 82, 74],
    "plate_sub_oct": [11, 9, 30, 27, 46, 41, 85, 90, 19, 16],
    "plate_drive": [12, 14, 17, 19, 27, 23, 33, 35, 13, 11],
    "plate_cutoff_c": [77, 79, 67, 65, 73, 71, 65, 63, 87, 85],
    "plate_cutoff_b": [71, 73, 61, 59, 67, 65, 57, 55, 79, 77],
    "plate_decay_c": [95, 97, 103, 105, 89, 87, 79, 77, 111, 109],
    "plate_decay_b": [99, 101, 113, 115, 93, 91, 85, 83, 115, 113],
    "plate_timbre": [35, 37, 25, 23, 33, 31, 19, 17, 41, 43],
    "plate_reso_mode": [13, 15, 33, 35, 25, 27, 39, 41, 19, 17],
    "plate_kit_tune": [61, 59, 69, 71, 59, 57, 77, 79, 63, 61],
    "plate_kit_decay": [63, 61, 67, 65, 71, 69, 45, 43, 69, 71],
    "plate_siren_rate": [39, 41, 29, 27, 41, 43, 47, 49, 31, 29],
    "plate_kick_boom": [75, 77, 63, 61, 90, 88, 97, 95, 59, 57],
    "plate_rim_tone": [43, 45, 31, 29, 51, 49, 35, 33, 37, 35],
    "plate_hat_lvl": [49, 51, 39, 37, 55, 57, 41, 39, 43, 41],
    "plate_kit_drive": [11, 13, 15, 17, 33, 31, 25, 23, 9, 7],
    "plate_siren_pitch": [43, 45, 33, 31, 45, 47, 51, 53, 37, 35],
    "plate_rim_dly": [59, 57, 37, 35, 49, 51, 29, 27, 43, 41],
    "plate_dly_b": [19, 21, 13, 11, 21, 23, 9, 7, 25, 23],
    "plate_pan_k": [64, 63, 64, 62, 64, 65, 64, 63, 59, 58],
    "plate_pan_r": [73, 71, 69, 67, 77, 75, 71, 69, 79, 77],
    "plate_pan_h": [49, 47, 55, 53, 47, 45, 57, 55, 43, 41],
    "plate_pan_s": [81, 79, 71, 69, 75, 73, 67, 65, 87, 85],
}

# --- New HOUSE trax ---
HOUSE_NEW = [
    '    { "CHIPE",   0, 2, 1, 0, 20, 121, 117, 27, 19, 17, 35,  3,  95, 0x1111, 0x1010, 0x4848, 0x4924, 0x0408, 0x0008 },',
    '    { "CANYOU",  0, 0, 0, 0, 19, 119, 115, 25, 17, 19, 33,  2,  93, 0x9111, 0x5010, 0x6464, 0x2492, 0x4044, 0x0010 },',
    '    { "ELBOW",   1, 6, 2, 2, 41, 129, 107, 35, 29, 21, 43, 11,  87, 0x1111, 0x1010, 0x5555, 0x4488, 0x4084, 0x0040 },',
    '    { "SOULC",   1, 4, 0, 3, 45, 133, 105, 37, 31, 19, 45, 12,  85, 0x4209, 0x9010, 0xEEEE, 0x8842, 0x2410, 0x0100 },',
    '    { "ARMAND",  2, 4, 0, 1, 13, 129, 113, 21, 13, 27, 51, 70,  89, 0x1111, 0x1010, 0xBBBB, 0xD7B5, 0x0020, 0x4000 },',
    '    { "SPOOKY",  2, 9, 3, 0, 9, 131, 117, 23, 15, 31, 55, 88,  91, 0x9111, 0x1210, 0xFFFF, 0x7DF5, 0x0080, 0x0800 },',
    '    { "LHEWRD",  3, 5, 1, 0, 27, 121, 97, 35, 31, 13, 27,  8,  79, 0x1111, 0x9010, 0x5555, 0x4290, 0x0401, 0x0020 },',
    '    { "CHOSEN",  3, 0, 0, 1, 29, 123, 101, 37, 33, 15, 29, 11,  83, 0x5111, 0x1010, 0x2222, 0x0849, 0x1008, 0x0080 },',
    '    { "BOBSIN",  4, 8, 4, 4, 91, 119, 105, 41, 37, 21, 47,  6, 107, 0x1111, 0x9010, 0x5454, 0x5454, 0x4244, 0x0040 },',
    '    { "MODERN",  4, 8, 3, 5, 97, 124, 109, 47, 43, 24, 53, 10, 113, 0x1111, 0x1010, 0x4646, 0x4646, 0x2420, 0x0100 },',
    '    { "RICARD",  5, 7, 2, 2, 51, 127, 93, 29, 25, 17, 33, 19,  75, 0x1111, 0x1010, 0x4444, 0x2840, 0x2004, 0x0040 },',
    '    { "AGORIA",  5, 7, 1, 3, 55, 131, 87, 33, 29, 13, 37, 15,  69, 0x5111, 0x1090, 0x2222, 0x4806, 0x0804, 0x0080 },',
]

HOUSE_SIDE_NEW: dict[str, list] = {
    "HOUSE_TRAX_ROOT": [52, 54, 59, 61, 49, 52, 54, 56, 52, 54, 56, 58],
    "HOUSE_TRAX_SCALE": [1, 5, 4, 0, 0, 2, 1, 5, 4, 2, 0, 1],
    "HOUSE_TRAX_SWING": [62, 63, 68, 69, 57, 58, 60, 61, 55, 56, 52, 53],
    "HOUSE_TRAX_STAB_RATE": [-4, -2, -3, -1, -4, -2, -3, -1, -6, -4, -5, -3],
    "HOUSE_TRAX_KIT": [0, 2, 4, 6, 3, 3, 1, 1, 5, 5, 7, 7],
}


def expand_flat(vals: list[int], new_pairs: list[list[int]]) -> list[int]:
    eras = len(vals) // OLD_PER
    out: list[int] = []
    for e in range(eras):
        out.extend(vals[e * OLD_PER : (e + 1) * OLD_PER])
        out.extend(new_pairs[e])
    return out


def parse_brace_nums(block: str) -> list[int]:
    block = re.sub(r"/\*.*?\*/", "", block, flags=re.S)
    out: list[int] = []
    for tok in block.replace("\n", " ").split(","):
        tok = tok.strip()
        if not tok or tok.startswith("/*"):
            continue
        tok = re.sub(r"/\*.*?\*/", "", tok).strip()
        if not tok:
            continue
        m = re.match(r"^(-?\d+)u?$", tok.split()[0])
        if m:
            out.append(int(m.group(1)))
    return out


def fmt_plate_u8(name: str, vals: list[int], use_suffix: bool) -> str:
    era_names = ["KINGSTON", "CHANNEL", "BRISTOL", "CROYDON", "POSTDUB"]
    lines = [f"static const uint8_t {name}[DUB_PLATE_N] = {{"]
    for e in range(len(vals) // NEW_PER):
        chunk = vals[e * NEW_PER : (e + 1) * NEW_PER]
        if use_suffix:
            body = ", ".join(f"{v}u" for v in chunk)
        else:
            body = ", ".join(str(v) for v in chunk)
        lines.append(f"    {body}, /* {era_names[e]} */")
    lines.append("};")
    return "\n".join(lines)


def patch_dub_plates(text: str) -> str:
    m = re.search(
        r"(static const dub_plate_t DUB_PLATES\[DUB_PLATE_N\] = \{)(.*?)(\n\};)",
        text,
        re.S,
    )
    if not m:
        raise SystemExit("DUB_PLATES block not found")
    body = m.group(2)
    rows = [ln.strip() for ln in body.splitlines() if ln.strip().startswith("{")]
    if len(rows) != 30:
        raise SystemExit(f"expected 30 dub plates, got {len(rows)}")
    out_rows: list[str] = []
    ni = 0
    for e in range(5):
        out_rows.extend(rows[e * OLD_PER : (e + 1) * OLD_PER])
        out_rows.extend([DUB_NEW[ni], DUB_NEW[ni + 1]])
        ni += 2
    new_body = "\n\n".join(
        [
            "    /* Era 0: KINGSTON (Plates 0..7) */",
            "\n".join(out_rows[0:8]),
            "\n    /* Era 1: CHANNEL (Plates 8..15) */",
            "\n".join(out_rows[8:16]),
            "\n    /* Era 2: BRISTOL (Plates 16..23) */",
            "\n".join(out_rows[16:24]),
            "\n    /* Era 3: CROYDON (Plates 24..31) */",
            "\n".join(out_rows[24:32]),
            "\n    /* Era 4: POSTDUB (Plates 32..39) */",
            "\n".join(out_rows[32:40]),
        ]
    )
    return text[: m.start(2)] + "\n" + new_body + "\n" + text[m.end(2) :]


def patch_uint8_array(text: str, name: str, new_vals: list[int]) -> str:
    pat = rf"static const uint8_t {name}\[DUB_PLATE_N\] = \{{.*?\}};"
    m = re.search(pat, text, re.S)
    if not m:
        raise SystemExit(f"{name} not found")
    old = m.group(0)
    inner = old[old.index("{") + 1 : old.rindex("}")]
    nums = parse_brace_nums(inner)
    if len(nums) != 30:
        raise SystemExit(f"{name}: expected 30 vals, parsed {len(nums)}")
    pairs = [new_vals[i : i + 2] for i in range(0, len(new_vals), 2)]
    expanded = expand_flat(nums, pairs)
    use_suffix = "u" in inner and name != "plate_roots"
    if name == "plate_roots":
        repl = fmt_plate_u8(name, expanded, use_suffix=False)
    else:
        repl = fmt_plate_u8(name, expanded, use_suffix=True)
    return text.replace(old, repl, 1)


def patch_house_trax(text: str) -> str:
    m = re.search(r"(static const house_trax_t HOUSE_TRAX\[HOUSE_TRAX_N\] = \{)(.*?)(\n\};)", text, re.S)
    if not m:
        raise SystemExit("HOUSE_TRAX not found")
    rows = [ln.strip() for ln in m.group(2).splitlines() if ln.strip().startswith("{")]
    if len(rows) != 36:
        raise SystemExit(f"expected 36 house trax, got {len(rows)}")
    out_rows: list[str] = []
    ni = 0
    era_names = ["JACK", "GARAGE", "ACID", "DEEP", "FILTER", "TECH"]
    parts = []
    for e in range(6):
        chunk = rows[e * OLD_PER : (e + 1) * OLD_PER]
        chunk_ext = chunk + [HOUSE_NEW[ni], HOUSE_NEW[ni + 1]]
        ni += 2
        parts.append(f"    /* {era_names[e]} {e} */\n" + "\n".join(chunk_ext))
    new_body = "\n" + "\n".join(parts) + "\n"
    return text[: m.start(2)] + new_body + text[m.end(2) :]


def patch_house_flat(text: str, name: str, new_chunk: list) -> str:
    pat = rf"static const (?:uint8_t|int8_t) {name}\[HOUSE_TRAX_N\] = \{{.*?\}};"
    m = re.search(pat, text, re.S)
    if not m:
        raise SystemExit(f"{name} not found")
    old = m.group(0)
    is_int8 = "int8_t" in old
    inner = old[old.index("{") + 1 : old.rindex("}")]
    vals = parse_brace_nums(inner)
    if len(vals) != 36:
        raise SystemExit(f"{name}: got {len(vals)}")
    pairs = [new_chunk[i : i + 2] for i in range(0, len(new_chunk), 2)]
    expanded = expand_flat(vals, pairs)
    ty = "int8_t" if is_int8 else "uint8_t"
    line = f"static const {ty} {name}[HOUSE_TRAX_N] = {{\n" + ",".join(str(v) for v in expanded) + "\n};"
    return text.replace(old, line, 1)


def main() -> None:
    # conf headers
    dub_conf = ROOT / "firmware/src/dub_conf.h"
    t = dub_conf.read_text()
    if "GENRE_PRESET_N   40u" not in t:
        t = t.replace("#define GENRE_PRESET_N   30u", "#define GENRE_PRESET_N   40u")
        t = t.replace("#define GENRE_CFG_MAGIC  0x4455423Fu", "#define GENRE_CFG_MAGIC  0x44554240u")
    if "#define GENRE_PRESETS_PER_ERA" not in t:
        t = t.replace("#define GENRE_ERA_N      5u\n", "#define GENRE_ERA_N      5u\n#define GENRE_PRESETS_PER_ERA 8u\n")
    dub_conf.write_text(t)

    house_conf = ROOT / "firmware/src/house_conf.h"
    t = house_conf.read_text()
    if "GENRE_PRESET_N   48u" not in t:
        t = t.replace("#define GENRE_PRESET_N   36u", "#define GENRE_PRESET_N   48u")
        t = t.replace("#define GENRE_CFG_MAGIC  0x484F553Eu", "#define GENRE_CFG_MAGIC  0x484F553Fu")
    if "#define GENRE_PRESETS_PER_ERA" not in t:
        t = t.replace("#define GENRE_ERA_N      6u\n", "#define GENRE_ERA_N      6u\n#define GENRE_PRESETS_PER_ERA 8u\n")
    house_conf.write_text(t)

    hp_conf = ROOT / "firmware/src/heavy_psych_conf.h"
    t = hp_conf.read_text()
    if "#define GENRE_PRESETS_PER_ERA" not in t:
        t = t.replace("#define GENRE_ERA_N      5u\n", "#define GENRE_ERA_N      5u\n#define GENRE_PRESETS_PER_ERA 6u\n")
    hp_conf.write_text(t)

    dub_types = ROOT / "firmware/src/dub_types.h"
    t = dub_types.read_text()
    t = t.replace("DUB_PLATE_N = 30", "DUB_PLATE_N = 40")
    dub_types.write_text(t)

    dd = ROOT / "firmware/src/dub_data.h"
    t = dd.read_text()
    t = t.replace("plate / 6u", "plate / GENRE_PRESETS_PER_ERA")
    t = t.replace("era) * 6u", "era) * GENRE_PRESETS_PER_ERA")
    if "#include \"genre_conf.h\"" not in t:
        t = t.replace('#include "dub_types.h"\n', '#include "dub_types.h"\n#include "genre_conf.h"\n')
    t = patch_dub_plates(t)
    for name, newv in DUB_SIDE_NEW.items():
        t = patch_uint8_array(t, name, newv)
    dd.write_text(t)

    hd = ROOT / "firmware/src/house_data.h"
    t = hd.read_text()
    t = t.replace("trax / 6u", "trax / GENRE_PRESETS_PER_ERA")
    t = t.replace("era) * 6u", "era) * GENRE_PRESETS_PER_ERA")
    if "#include \"genre_conf.h\"" not in t:
        t = t.replace('#include "dub_types.h"\n', '#include "dub_types.h"\n#include "genre_conf.h"\n')
    t = patch_house_trax(t)
    for name, newv in HOUSE_SIDE_NEW.items():
        t = patch_house_flat(t, name, newv)
    hd.write_text(t)

    hpd = ROOT / "firmware/src/heavy_psych_data.h"
    t = hpd.read_text()
    if "#include \"genre_conf.h\"" not in t:
        t = t.replace('#include "dub_types.h"\n', '#include "dub_types.h"\n#include "genre_conf.h"\n')
    t = t.replace("trax / 6u", "trax / GENRE_PRESETS_PER_ERA")
    t = t.replace("era) * 6u", "era) * GENRE_PRESETS_PER_ERA")
    hpd.write_text(t)

    print("expand_8_per_era: conf + dub_data + house_data + heavy_psych era math OK")


if __name__ == "__main__":
    main()
