#!/usr/bin/env python3
"""Per-era statistics of a genre's curated ROM plates + a draft fm_pgen_genome_t table.

Starting point for a genre's endless plate genome (docs/plate-gen-recipe.md): ranges are the curated min..max,
pools the curated sets, lane densities the curated on-rate, must = steps every curated plate of the era has
(AND), ban candidates = steps no curated plate has (shown, not emitted). Musical hard rules are a human call;
edit the draft, do not paste it blindly.

  tools/pgen_era_stats.py house          # stats + draft
  tools/pgen_era_stats.py house --draft  # draft only
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_views as gv  # noqa: E402

SRC = gv.SRC
FIELDS = ("era", "bass_model", "voicing", "tap", "tank", "bpm", "density", "dly_c", "rev_c", "feedback",
          "loop_filt", "wobble", "pump")
MASKS = ("kick", "rim", "hat", "sub", "chord", "siren")
ALIASES = {"hvypsy": "HVYPSY", "heavy_psych": "HVYPSY", "heavypsych": "HVYPSY"}


def int_table(text: str, sym: str) -> list[int] | None:
    m = re.search(rf"static const u?int(?:8|16|32)_t {sym}\[[^\]]*\]\s*=\s*\{{(.*?)\}};", text, re.S)
    if not m:
        return None
    body = gv.COMMENT.sub("", m.group(1))
    out = []
    for e in (x.strip() for x in body.split(",")):
        if e:
            out.append(-int(e[1:], 0) if e.startswith("-") else gv.parse_mask(e, sym))
    return out


def enum_values(text: str) -> dict[str, int]:
    vals: dict[str, int] = {}
    for body in re.findall(r"enum\s*\{(.*?)\}", gv.COMMENT.sub("", text), re.S):
        nxt = 0
        for item in (x.strip() for x in body.split(",")):
            m = re.match(r"^([A-Z_][A-Z0-9_]*)\s*(?:=\s*(-?\w+))?$", item)
            if not m:
                continue
            if m.group(2):
                try:
                    nxt = int(m.group(2), 0)
                except ValueError:
                    nxt = vals.get(m.group(2), 0)
            vals[m.group(1)] = nxt
            nxt += 1
    return vals


def read_rows(header: str, table: str) -> list[dict]:
    text = (SRC / header).read_text()
    ev = enum_values((SRC / "dub_types.h").read_text() + text)
    m = re.search(rf"static const \w+ {table}\[[^\]]*\]\s*=\s*\{{(.*?)\n\}};", text, re.S)
    if not m:
        raise SystemExit(f"{header}: {table} not found")
    rows = []
    for line in m.group(1).splitlines():
        row = gv.ROW.match(gv.COMMENT.sub("", line))
        if not row:
            continue
        f = [x.strip() for x in row.group(2).split(",") if x.strip()]
        r = {"name": row.group(1)}
        for i, k in enumerate(FIELDS):
            r[k] = ev[f[i]] if f[i] in ev else int(f[i], 0)
        for i, k in enumerate(MASKS):
            r[k] = gv.parse_mask(f[13 + i], f"{header}: {row.group(1)}")
        rows.append(r)
    return rows


def rng(v: list[int]) -> str:
    return f"{min(v)}..{max(v)}" if v else "-"


def pool(v: list[int]) -> str:
    return " | ".join(f"B({x})" for x in sorted(set(v))) if v else "0u"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("genre")
    ap.add_argument("--draft", action="store_true")
    a = ap.parse_args()
    want = ALIASES.get(a.genre.lower(), a.genre.upper())
    app = next((x for x in gv.APPS if x[0] == want), None)
    if not app:
        raise SystemExit(f"unknown genre {a.genre}; one of {', '.join(x[0] for x in gv.APPS)}")
    name, header, table, _, _, _ = app
    text = (SRC / header).read_text()
    rows = read_rows(header, table)
    per = 10
    era_names = re.search(r"_ERA_NAMES\[[^\]]*\]\s*=\s*\{(.*?)\};", text, re.S)
    eras = re.findall(r'"([^"]+)"', era_names.group(1)) if era_names else [str(i) for i in range(6)]
    pre = table[:-1] if table.endswith("S") and not table.endswith("SS") else table
    tabs = {k: int_table(text, f"{pre}_{k}") for k in ("ROOT", "SCALE", "SWING", "KIT", "LEAD_RATE", "STAB_RATE",
                                                       "LEAD_GATE", "LEAD_GATE_CS")}
    rate = tabs["LEAD_RATE"] or tabs["STAB_RATE"]
    steps = 16 if all(r[k] < 0x10000 for r in rows for k in MASKS) else 32
    full = (1 << steps) - 1
    print(f"/* {name}: {header} {table}, {len(rows)} plates, {steps}-step masks; per-slot tables: "
          + ", ".join(k for k, v in tabs.items() if v) + " */")
    drafts = []
    for e in range(len(rows) // per):
        rs = rows[e * per:(e + 1) * per]
        sl = range(e * per, (e + 1) * per)
        col = {k: [r[k] for r in rs] for k in FIELDS + MASKS}
        t = {k: ([v[i] for i in sl] if v else []) for k, v in tabs.items()}
        rt = [rate[i] for i in sl] if rate else []
        lanes = list(MASKS) + (["lead"] if t["LEAD_GATE"] else [])
        dens, must, never = {}, {}, {}
        for ln in lanes:
            ms = t["LEAD_GATE"] if ln == "lead" else col[ln]
            dens[ln] = round(255 * sum(bin(m & full).count("1") for m in ms) / (len(ms) * steps))
            a_ = full
            o_ = 0
            for m in ms:
                a_ &= m
                o_ |= m
            must[ln], never[ln] = a_, full & ~o_
        en = eras[e] if e < len(eras) else str(e)
        if not a.draft:
            print(f"\n{en}: bpm {rng(col['bpm'])} dens {rng(col['density'])} swing {rng(t['SWING'])} "
                  f"pump {rng(col['pump'])} tank {rng(col['tank'])} wob {rng(col['wobble'])} dly {rng(col['dly_c'])} "
                  f"rev {rng(col['rev_c'])} fb {rng(col['feedback'])} filt {rng(col['loop_filt'])} "
                  f"root {rng(t['ROOT'])} rate {rng(rt)} tap {sorted(set(col['tap']))}")
            print(f"  scale {sorted(set(t['SCALE']))} kit {sorted(set(t['KIT']))} bass {sorted(set(col['bass_model']))} "
                  f"voice {sorted(set(col['voicing']))} gate_cs {rng(t['LEAD_GATE_CS'])}")
            for ln in lanes:
                print(f"  {ln:<5} dens {dens[ln]:3d}  all {must[ln]:#0{steps // 4 + 2}x}  never {never[ln]:#0{steps // 4 + 2}x}")
        sw = t["SWING"] or [50]
        rr = rt or [0]
        rt_ = t["ROOT"] or [48]
        d7 = [dens.get(k, 0) for k in MASKS] + [dens.get("lead", 0)]
        m7 = [must.get(k, 0) for k in MASKS] + [must.get("lead", 0)]
        drafts.append(
            f"    /* {en} */\n"
            f"    {{ {min(col['bpm'])}, {max(col['bpm'])}, {min(col['density'])}, {max(col['density'])}, "
            f"{min(sw)}, {max(sw)}, {min(col['pump'])}, {max(col['pump'])}, {min(col['tank'])}, {max(col['tank'])}, "
            f"{min(col['wobble'])}, {max(col['wobble'])}, {min(col['dly_c'])}, {max(col['dly_c'])}, "
            f"{min(col['rev_c'])}, {max(col['rev_c'])}, {min(col['feedback'])}, {max(col['feedback'])}, "
            f"{round(sum(col['loop_filt']) / per)}, {min(rt_)}, {max(rt_)},\n"
            f"      {pool(t['SCALE'] or [0])}, {pool(t['KIT'] or [0])},\n"
            f"      {pool(col['bass_model'])}, {pool(col['voicing'])},\n"
            f"      {{ {', '.join(map(str, d7))} }},\n"
            f"      {{ {', '.join(f'0x{m:X}u' for m in m7)} }}, {{ 0u }}, 0, "
            f"{round(sum(t['LEAD_GATE_CS']) / per) if t['LEAD_GATE_CS'] else 0}, {min(rr)}, {max(rr)} }},")
    print("\n/* draft genome (B(x) = FM_PGEN_B(x)); review hard rules before use */")
    print("\n".join(drafts))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
