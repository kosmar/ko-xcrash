#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate firmware/src/fm_views_data.h: SEQ views S2..S8 for every ROM plate of KO DUB, KO HOUSE, KO HVYPSY.

S1 is the plate itself (one tiled 16-step page per lane) and stays in the plate tables. S2..S8 are
deterministic variants of S1, seeded from plate index and view index, one fixed role per view:

  S2 ANSWER  the second half answers: beat 3/4 hat swap or one late hat nudge, one late bass anticipation
  S3 LIFT    density +15 % on hats and rim ghosts, leaning to the end of the bar
  S4 TURN    turnaround fill in the last beat: hats, rim, bass passing note (at most +20 %)
  S5 THIN    density -25 %: weak 16ths first, half bar kept symmetric
  S6 SHIFT   beat 2 trades with beat 3 or 4, siren one beat earlier or later, early bass anticipation, density -10 %
  S7 BUILD   density +25 % across hats, rim, chord, bass, leaning late
  S8 FILL    phrase-end fill: hat roll, rim roll, kick pickup, bass passing note, chord push, siren echo (at most +30 %)

What keeps it musical: kick quarters, rim backbeats and bass on 1 and 3 are anchors (never removed or moved);
kick, rim and chord never gain a quarter-note hit; empty lanes stay empty; an added hit prefers the half-bar
partner of an existing hit, a 16th slot already used in another beat and the neighbour of an existing hit
(motifs, ghosts, doubles); a removal takes weak 16ths first and keeps the half bar symmetric.
No velocity: S1 has none either, VEL +- stays a runtime knob.
The GENRE_BASS block is hand-authored (see PRESERVE): kept verbatim on write, ignored by --check.

  python3 tools/gen_views.py                 write the header, print sizes and density per view
  python3 tools/gen_views.py --check         exit 1 if the header is stale
  python3 tools/gen_views.py --show MIXROOT  print S1..S8 of a plate as a grid
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "firmware" / "src"
OUT = SRC / "fm_views_data.h"

STEPS = 16
VIEWS = 8
K, R, H, S, B, C = range(6)  # SEQ lane = gate bit
LANES = "KRHSBC"
STRUCT_LANE = (K, R, H, B, C, S)  # dub_plate_t masks: kick, rim, hat, sub, chord, siren
ROLES = ("S1", "ANSWER", "LIFT", "TURN", "THIN", "SHIFT", "BUILD", "FILL")

# app, data header, plate table, plate count, conf header, genre macro (None = #else, like genre_pack.h)
APPS = (
    ("HVYPSY", "heavy_psych_data.h", "HEAVY_PSYCH_TRAX", "HEAVY_PSYCH_TRAX_N", "heavy_psych_conf.h", "GENRE_HEAVY_PSYCH"),
    ("HOUSE", "house_data.h", "HOUSE_TRAX", "HOUSE_TRAX_N", "house_conf.h", "GENRE_HOUSE"),
    ("JAZZ", "jazz_data.h", "JAZZ_TRAX", "JAZZ_TRAX_N", "jazz_conf.h", "GENRE_JAZZ"),
    ("IDM", "idm_data.h", "IDM_PIECE", "IDM_PIECE_N", "idm_conf.h", "GENRE_IDM"),
    ("SNOWCRASH", "snowcrash_data.h", "SNOWCRASH_SLAB", "SNOWCRASH_SLAB_N", "snowcrash_conf.h", "GENRE_SNOWCRASH"),
    ("HIPHOP", "hiphop_data.h", "HIPHOP_BEAT", "HIPHOP_BEAT_N", "hiphop_conf.h", "GENRE_HIPHOP"),
    ("POP", "pop_data.h", "POP_PLATE", "POP_PLATE_N", "pop_conf.h", "GENRE_POP"),
    ("FUNK", "funk_data.h", "FUNK_GROOVE", "FUNK_GROOVE_N", "funk_conf.h", "GENRE_FUNK"),
    ("TECHNO", "techno_data.h", "TECHNO_LOOP", "TECHNO_LOOP_N", "techno_conf.h", "GENRE_TECHNO"),
    ("TRIPHOP", "triphop_data.h", "TRIPHOP_LOOP", "TRIPHOP_LOOP_N", "triphop_conf.h", "GENRE_TRIPHOP"),
    ("SOUL", "soul_data.h", "SOUL_SIDE", "SOUL_SIDE_N", "soul_conf.h", "GENRE_SOUL"),
    ("BASS", "bass_data.h", "BASS_LINE", "BASS_LINE_N", "bass_conf.h", "GENRE_BASS"),
    ("DUB", "dub_data.h", "DUB_PLATES", "DUB_PLATE_N", "dub_conf.h", None),
)

# ROM lead phrase gates (uint32 table → S1 lane S); plate siren mask is not the phrase grid.
LEAD_GATE_TABLE = {
    "HVYPSY": "HEAVY_PSYCH_TRAX_LEAD_GATE",
    "JAZZ": "JAZZ_TRAX_LEAD_GATE",
    "IDM": "IDM_PIECE_LEAD_GATE",
    "SNOWCRASH": "SNOWCRASH_SLAB_LEAD_GATE",
    "HIPHOP": "HIPHOP_BEAT_LEAD_GATE",
    "TECHNO": "TECHNO_LOOP_LEAD_GATE",
    "TRIPHOP": "TRIPHOP_LOOP_LEAD_GATE",
    "SOUL": "SOUL_SIDE_LEAD_GATE",
    "BASS": "BASS_LINE_LEAD_GATE",
}

# Genre macros whose fm_views_data.h block is never generated: KO BASS views are hand-authored, so the
# existing block is copied verbatim on write and ignored by --check.
PRESERVE = {"GENRE_BASS"}

M32 = 0xFFFFFFFF
JITTER = 90

ROW = re.compile(r'^\s*\{\s*"([^"]*)"\s*,(.*)\}\s*,?\s*$')
TILED = re.compile(
    r"^(?:\(\s*)?(?:\(\s*uint32_t\s*\)\s*)?0x([0-9A-Fa-f]+)u?\s*\|\s*"
    r"\(\s*\(uint32_t\)\s*0x([0-9A-Fa-f]+)u?\s*<<\s*16\s*\)\s*\)?$"
)
PLAIN = re.compile(r"^(0x[0-9A-Fa-f]+|\d+)[uU]?$")
COMMENT = re.compile(r"/\*.*?\*/|//.*$")


# ---- plate tables

def parse_mask(expr: str, where: str) -> int:
    e = expr.strip()
    while len(e) >= 2 and e[0] == "(" and e[-1] == ")":
        inner = e[1:-1].strip()
        if inner.count("(") < e.count("(") - 1:
            break
        e = inner
    m = TILED.match(e)
    if m:
        return int(m.group(1), 16) | (int(m.group(2), 16) << 16)
    m = PLAIN.match(e)
    if m:
        return int(m.group(1), 0)
    raise SystemExit(f"{where}: cannot read mask {e!r}")


def read_lead_gates(header: str, table: str) -> list[int]:
    """Parse lead GATE uint32 table; return full 32-bit masks per plate."""
    text = (SRC / header).read_text()
    m = re.search(
        rf"static const uint32_t {table}\[[^\]]*\]\s*=\s*\{{(.*?)\}};",
        text,
        re.S,
    )
    if not m:
        raise SystemExit(f"{header}: {table} not found")
    body = COMMENT.sub("", m.group(1))
    gates = []
    for expr in body.split(","):
        expr = expr.strip()
        if not expr:
            continue
        gates.append(parse_mask(expr, f"{header}: {table}") & 0xFFFFFFFF)
    if len(gates) not in (48, 60, 64, 80):
        raise SystemExit(f"{header}: {table} has {len(gates)} gates, expected 48, 60, 64, or 80")
    return gates


def read_plates(header: str, table: str, conf: str, hp_lead_gates: list[int] | None = None) -> list[tuple[str, list[int], list[int]]]:
    text = (SRC / header).read_text()
    m = re.search(rf"static const \w+ {table}\[[^\]]*\]\s*=\s*\{{(.*?)\n\}};", text, re.S)
    if not m:
        raise SystemExit(f"{header}: {table} not found")
    plates = []
    for line in m.group(1).splitlines():
        line = COMMENT.sub("", line)
        row = ROW.match(line)
        if not row:
            if line.strip().startswith("{"):
                raise SystemExit(f"{header}: cannot read row {line.strip()[:60]!r}")
            continue
        name = row.group(1)
        fields = [f.strip() for f in row.group(2).split(",") if f.strip()]
        if len(fields) != 19:
            raise SystemExit(f"{header}: {name}: {len(fields)} fields after the name, dub_plate_t has 19")
        lanes = [0] * 6
        lanes_hi = [0] * 6
        for si, expr in enumerate(fields[13:]):
            mask = parse_mask(expr, f"{header}: {name}")
            lo, hi = mask & 0xFFFF, (mask >> 16) & 0xFFFF
            lanes[STRUCT_LANE[si]] = lo
            lanes_hi[STRUCT_LANE[si]] = hi
        plates.append((name, lanes, lanes_hi))
    want = re.search(r"#define\s+GENRE_PRESET_N\s+(\d+)u", (SRC / conf).read_text())
    if not want or int(want.group(1)) != len(plates):
        raise SystemExit(f"{header}: {len(plates)} plates, {conf} says {want.group(1) if want else '?'}")
    if hp_lead_gates is not None:
        if len(hp_lead_gates) != len(plates):
            raise SystemExit(f"{header}: {len(hp_lead_gates)} lead gates, {len(plates)} trax")
        for pi, (_, lanes, lanes_hi) in enumerate(plates):
            g32 = hp_lead_gates[pi]
            lo, hi = g32 & 0xFFFF, (g32 >> 16) & 0xFFFF
            lanes[S] = lo
            lanes_hi[S] = hi if hi else lo
    return plates


# ---- deterministic randomness

def mix32(x: int) -> int:
    x &= M32
    x ^= x >> 16
    x = (x * 0x7FEB352D) & M32
    x ^= x >> 15
    x = (x * 0x846CA68B) & M32
    return x ^ (x >> 16)


class Rng:
    """xorshift32 seeded from plate index and view index"""

    def __init__(self, plate: int, view: int):
        self.s = mix32((plate << 8) | view) or 0x9E3779B9

    def next(self) -> int:
        x = self.s
        x ^= (x << 13) & M32
        x ^= x >> 17
        x ^= (x << 5) & M32
        self.s = x
        return x

    def below(self, n: int) -> int:
        return self.next() % n


# ---- step rules

def bits(m: int) -> list[int]:
    return [i for i in range(STEPS) if m >> i & 1]


def hits(m: int) -> int:
    return bin(m).count("1")


def on(m: int, i: int) -> bool:
    return 0 <= i < STEPS and bool(m >> i & 1)


def weight(i: int) -> int:
    """metric weight in a 16-step bar: 1 > 3 > 2 and 4 > 8th offbeats > 16ths"""
    if i == 0:
        return 4
    if i == 8:
        return 3
    if i % 4 == 0:
        return 2
    return 1 if i % 2 == 0 else 0


def anchored(lane: int, i: int) -> bool:
    """never removed or moved"""
    if lane in (K, R):
        return weight(i) >= 2
    return lane == B and i in (0, 8)


def no_new(lane: int, i: int) -> bool:
    """never added"""
    if lane in (K, R, C):
        return weight(i) >= 2
    return lane in (B, S) and i == 0


def run_with(m: int, i: int) -> int:
    m |= 1 << i
    lo = hi = i
    while on(m, lo - 1):
        lo -= 1
    while on(m, hi + 1):
        hi += 1
    return hi - lo + 1


def add_score(m: int, m0: int, lane: int, i: int, late: bool, fill: bool) -> int:
    grid = 2 if lane in (H, B) and not fill and not any(j & 1 for j in bits(m0)) else 1
    s = 0
    if on(m, i ^ 8):
        s += 200  # repeats the other half bar
    if any(on(m, j) for j in range(i & 3, STEPS, 4) if j != i):
        s += 100  # 16th slot already used in another beat
    if on(m, i - grid) or on(m, i + grid):
        s += 100  # ghost, double, roll
    if grid == 2 and i & 1:
        s -= 120  # an 8th-grid lane stays on 8ths
    if late and i >= 8:
        s += 60
    s -= (20 if lane == H else 60) * weight(i)
    if lane != H and run_with(m, i) >= 3:
        s -= 150
    return s


def drop_score(m: int, i: int) -> int:
    s = 100 * (3 - min(weight(i), 3))  # a weight class outranks the jitter
    if not on(m, i ^ 8):
        s += 150  # the half bar stays symmetric
    if not any(on(m, j) for j in range(i & 3, STEPS, 4) if j != i):
        s += 80  # lone ornament
    return s


# ---- operations on one lane

def add(m: int, m0: int, lane: int, n: int, rng: Rng, late=False, region=range(STEPS), fill=False) -> int:
    for _ in range(n):
        best = None
        for i in region:
            if on(m, i) or no_new(lane, i):
                continue
            s = add_score(m, m0, lane, i, late, fill) + rng.below(JITTER)
            if best is None or s > best[0]:
                best = (s, i)
        if best is None:
            break
        m |= 1 << best[1]
    return m


def drop(m: int, lane: int, n: int, rng: Rng) -> int:
    for _ in range(n):
        if hits(m) <= 1:
            break
        best = None
        for i in bits(m):
            if anchored(lane, i):
                continue
            s = drop_score(m, i) + rng.below(JITTER)
            if best is None or s > best[0]:
                best = (s, i)
        if best is None:
            break
        m &= ~(1 << best[1])
    return m


def nudge(m: int, lane: int, n: int, rng: Rng, dirs=(-1, 1), half=None) -> int:
    """n hits a 16th off; half 0 / 1 limits the moved hits to the first / second half bar"""
    moved, left = set(), set()
    for _ in range(n):
        best = None
        for i in bits(m):
            if anchored(lane, i) or i in moved or (half is not None and i // 8 != half):
                continue
            for d in dirs:
                t = i + d
                if not 0 < t < STEPS or on(m, t) or t in left or no_new(lane, t):
                    continue
                s = 100 * (3 - min(weight(i), 3)) + (0 if on(m, i ^ 8) else 60) + rng.below(JITTER)
                if best is None or s > best[0]:
                    best = (s, i, t)
        if best is None:
            break
        _, i, t = best
        m = (m & ~(1 << i)) | (1 << t)
        moved.add(t)
        left.add(i)
    return m


def swap(m: int, lane: int, pairs, rng: Rng) -> int:
    """two beats trade their off-quarter figures: only beats whose quarter hit matches, so the pulse stays"""
    pairs = list(pairs)
    if len(pairs) > 1 and rng.below(2):
        pairs.reverse()
    for a, b in pairs:
        ga, gb = (m >> 4 * a) & 15, (m >> 4 * b) & 15
        if ga == gb or (ga & 1) != (gb & 1):
            continue
        return (m & ~(15 << 4 * a) & ~(15 << 4 * b)) | (gb << 4 * a) | (ga << 4 * b)
    return m


def move_beat(m: int, lane: int, rng: Rng) -> int:
    """a sparse lane's hit one beat earlier or later, same 16th slot"""
    cands = [i for i in bits(m) if not anchored(lane, i)]
    if not cands or hits(m) >= 4:
        return m
    i = cands[rng.below(len(cands))]
    for t in ((i - 4, i + 4) if rng.below(2) else (i + 4, i - 4)):
        if 0 < t < STEPS and not on(m, t) and not no_new(lane, t):
            return (m & ~(1 << i)) | (1 << t)
    return m


def echo(m: int, lane: int) -> int:
    """a sparse lane's last hit answered a dotted 8th later, else just before it"""
    if not m or hits(m) >= 4:
        return m
    h = bits(m)[-1]
    for t in (h + 3, h + 2, h - 3, h - 2, h - 4):
        if 0 < t < STEPS and not on(m, t) and not no_new(lane, t):
            return m | (1 << t)
    return m


def pickup(m: int, m0: int, rng: Rng) -> int:
    """kick into the next bar on the last 8th or 16th; only over a downbeat kick, never in a one-drop"""
    if not on(m, 0) or hits(m) < 2:
        return m
    return add(m, m0, K, 1, rng, region=(14, 15), fill=True)


# ---- views

def pick(pool, rng: Rng) -> int:
    r = rng.below(sum(w for _, w in pool))
    for lane, w in pool:
        if r < w:
            return lane
        r -= w
    return pool[-1][0]


def density(ls: list[int], s1: list[int], pct: int, weights, rng: Rng, late=False) -> None:
    """pct % of the plate's hits added (pct > 0) or removed, spread over the weighted lanes"""
    n = max(1, (sum(hits(m) for m in s1) * abs(pct) + 50) // 100)
    done = [0] * 6
    for _ in range(n):
        pool = []
        for lane, w in weights:
            m, m0 = ls[lane], s1[lane]
            if not m0:
                continue
            if pct > 0:
                if done[lane] >= max(1, (hits(m0) + 1) // 2) or (lane == S and hits(m0) < 4):
                    continue
                if not any(not on(m, i) and not no_new(lane, i) for i in range(STEPS)):
                    continue
            else:
                if done[lane] >= (1 if lane == K else hits(m0) // 2) or hits(m) <= 1:
                    continue
                if all(anchored(lane, i) for i in bits(m)):
                    continue
            pool.append((lane, w))
        if not pool:
            break
        lane = pick(pool, rng)
        ls[lane] = add(ls[lane], s1[lane], lane, 1, rng, late=late) if pct > 0 else drop(ls[lane], lane, 1, rng)
        done[lane] += 1


def fill(ls: list[int], s1: list[int], plan, pct: int, rng: Rng) -> None:
    """plan steps in order until pct % of the plate's hits are spent"""
    budget = max(2, (sum(hits(m) for m in s1) * pct + 50) // 100)
    for kind, lane, region, n in plan:
        if budget <= 0:
            break
        m = ls[lane]
        if not s1[lane] or (lane == B and hits(s1[B]) < 2):
            continue
        if kind == "add":
            m = add(m, s1[lane], lane, min(n, budget), rng, region=region, fill=True)
        elif kind == "pickup":
            m = pickup(m, s1[lane], rng)
        else:
            m = echo(m, lane)
        budget -= hits(m) - hits(ls[lane])
        ls[lane] = m


def make_view(s1: list[int], v: int, rng: Rng) -> list[int]:
    ls = list(s1)
    if v == 1:  # ANSWER
        m = swap(ls[H], H, ((2, 3),), rng)
        ls[H] = m if m != ls[H] else nudge(ls[H], H, 1, rng, half=1)
        if hits(ls[B]) >= 2:
            ls[B] = nudge(ls[B], B, 1, rng, dirs=(-1,), half=1)
        if s1[S]:
            m = swap(ls[S], S, ((2, 3),), rng)
            ls[S] = m if m != ls[S] else nudge(ls[S], S, 1, rng, half=1)
    elif v == 2:  # LIFT
        density(ls, s1, 15, ((H, 5), (R, 3), (S, 3)), rng, late=True)
    elif v == 3:  # TURN
        fill(ls, s1, (("add", H, range(12, 16), 2), ("add", R, range(13, 16), 1),
                      ("add", B, range(13, 16), 1), ("add", S, range(13, 16), 1)), 20, rng)
    elif v == 4:  # THIN
        density(ls, s1, -25, ((H, 5), (R, 2), (C, 2), (B, 1), (K, 1), (S, 1)), rng)
    elif v == 5:  # SHIFT
        for lane in (H, R, C):
            ls[lane] = swap(ls[lane], lane, ((1, 3), (1, 2)), rng)
        ls[S] = move_beat(ls[S], S, rng)
        if hits(ls[B]) >= 2:
            ls[B] = nudge(ls[B], B, 1, rng, dirs=(-1,), half=0)
        density(ls, s1, -10, ((H, 3), (R, 1), (C, 1)), rng)
    elif v == 6:  # BUILD
        density(ls, s1, 25, ((H, 5), (R, 2), (C, 2), (B, 1), (S, 3)), rng, late=True)
    else:  # FILL
        fill(ls, s1, (("add", H, range(12, 16), 3), ("add", R, range(13, 16), 2), ("pickup", K, None, 1),
                      ("add", B, range(11, 16), 1), ("add", C, range(12, 16), 1), ("echo", S, None, 1),
                      ("add", H, range(8, 12), 1)), 30, rng)
    return ls


def fallback(ls: list[int], s1: list[int], k: int, rng: Rng) -> None:
    """one more small change when a view came out equal to S1 or to an earlier view"""
    ops = (
        (H, lambda m: nudge(m, H, 1, rng, half=1)),
        (C, lambda m: add(m, s1[C], C, 1, rng, late=True)),
        (B, lambda m: nudge(m, B, 1, rng, dirs=(-1,))),
        (R, lambda m: add(m, s1[R], R, 1, rng, late=True)),
        (S, lambda m: move_beat(m, S, rng)),
        (H, lambda m: nudge(m, H, 1, rng, half=0)),
        (H, lambda m: add(m, s1[H], H, 1, rng, late=True)),
        (C, lambda m: nudge(m, C, 1, rng)),
        (H, lambda m: drop(m, H, 1, rng)),
    )
    lane, op = ops[k % len(ops)]
    if s1[lane]:
        ls[lane] = op(ls[lane])


def plate_views(pi: int, s1: list[int], s1_hi: list[int], stats: dict) -> list[list[int]]:
    views = [list(s1)]
    for v in range(1, VIEWS):
        rng = Rng(pi, v)
        ls = make_view(s1, v, rng)
        if v == 1:
            for ln in range(6):
                if s1_hi[ln]:
                    ls[ln] = s1_hi[ln]
        k = 0
        while (ls in views) and k < 3 * 9:
            fallback(ls, s1, k, rng)
            k += 1
        if ls == s1:
            raise SystemExit(f"plate {pi}: {ROLES[v]} stays equal to S1")
        stats["fallback"] += k > 0
        stats["dupes"] += ls in views
        views.append(ls)
    return views


# ---- output

def build() -> tuple[str, list]:
    tables = []
    for app, header, table, count, conf, macro in APPS:
        stats = {"fallback": 0, "dupes": 0}
        lead_sym = LEAD_GATE_TABLE.get(app)
        hp_lead = read_lead_gates(header, lead_sym) if lead_sym else None
        plates = read_plates(header, table, conf, hp_lead)
        tables.append((app, count, macro, plates,
                       [plate_views(pi, s1, hi, stats) for pi, (_, s1, hi) in enumerate(plates)],
                       stats))
    o = [
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "/* Generated by tools/gen_views.py from the ROM plate tables, do not edit (rules: see the tool).",
        " * SEQ views S2..S8 of every plate, variants of S1 = the plate itself, which is read from the plate table.",
        " * One uint16_t per plate, lane and view: bit n = step n of the 16-step page.",
        " * Lanes in SEQ gate-bit order: 0 K, 1 R, 2 H, 3 S (siren), 4 B (sub), 5 C (chord). */",
        "#ifndef FM_VIEWS_DATA_H",
        "#define FM_VIEWS_DATA_H",
        "",
        "#include <stdint.h>",
        '#include "genre_pack.h"',
        "",
        "#define FM_VIEW_N FM_SEQ_MAX_PAGES /* S1..S8 */",
        "#define FM_VIEW_LANE_N 6u",
        "",
    ]
    for ti, (app, count, macro, plates, views, _) in enumerate(tables):
        o.append(f"#{'if' if ti == 0 else 'elif'} defined({macro})" if macro else "#else")
        o.append(f"#define FM_VIEW_PLATE_N {count}")
        o.append("static const uint16_t FM_VIEWS[][FM_VIEW_LANE_N][FM_VIEW_N - 1u] = {")
        for pi, ((name, _, _), vs) in enumerate(zip(plates, views)):
            o.append(f"    /* {pi} {name} */")
            o.append("    {")
            for lane in range(6):
                vals = ", ".join(f"0x{vs[v][lane]:04X}u" for v in range(1, VIEWS))
                o.append(f"        {{ {vals} }}, /* {LANES[lane]} */")
            o.append("    },")
        o.append("};")
    o += [
        "#endif",
        "",
        '_Static_assert(sizeof FM_VIEWS / sizeof FM_VIEWS[0] == FM_VIEW_PLATE_N, "one view set per ROM plate");',
        '_Static_assert(FM_SEQ_PAGE_STEPS == 16u && FM_VIEW_N == 8u, "a view is one uint16_t page, S1..S8");',
        "",
        "static inline uint16_t fm_view_plate_s1(uint8_t plate, uint8_t lane)",
        "{",
        "    const dub_plate_t *pl = genre_preset(plate);",
        "",
        "#if defined(GENRE_HEAVY_PSYCH) || defined(GENRE_JAZZ) || defined(GENRE_IDM) || defined(GENRE_SNOWCRASH) || defined(GENRE_HIPHOP) || defined(GENRE_SOUL)",
        "    if (lane == 3u)",
        "        return (uint16_t)(genre_lead_gate32(plate) & 0xFFFFu);",
        "#endif",
        "    return (uint16_t)(lane == 0u ? pl->kick : lane == 1u ? pl->rim : lane == 2u ? pl->hat :",
        "                      lane == 3u ? pl->siren : lane == 4u ? pl->sub : pl->chord);",
        "}",
        "",
        "/* view 0 = S1 (the plate), 1..7 = S2..S8 */",
        "static inline uint16_t fm_view_data_get(uint8_t plate, uint8_t lane, uint8_t view)",
        "{",
        "    plate = genre_clamp_preset(plate);",
        "    lane = (uint8_t)(lane % FM_VIEW_LANE_N);",
        "    view = (uint8_t)(view % FM_VIEW_N);",
        "    if (view == 0u)",
        "        return fm_view_plate_s1(plate, lane);",
        "#if defined(FM_PGEN_ON)",
        "    if (genre_pgen_views_hit(plate))",
        "        return genre_pgen_view(lane, view - 1u);",
        "#endif",
        "    return FM_VIEWS[plate][lane][view - 1u];",
        "}",
        "",
        "/* st = absolute SEQ step 0..127: view st / 16, step st % 16 */",
        "static inline uint8_t fm_view_step_bit(uint8_t plate, uint8_t lane, uint32_t st)",
        "{",
        "    st %= FM_VIEW_N * FM_SEQ_PAGE_STEPS;",
        "    return (uint8_t)((fm_view_data_get(plate, lane, (uint8_t)(st / FM_SEQ_PAGE_STEPS)) >>",
        "                      (st % FM_SEQ_PAGE_STEPS)) & 1u);",
        "}",
        "",
        "#endif",
        "",
    ]
    return "\n".join(o), tables


def genre_blocks(text: str) -> dict:
    """macro -> (start, end) line span of its `#if/#elif defined(MACRO)` block in the FM_VIEWS chain."""
    lines = text.split("\n")
    spans, cur = {}, None
    for i, ln in enumerate(lines):
        m = re.match(r"#(?:if|elif) defined\((GENRE_\w+)\)$", ln)
        if m or ln in ("#else", "#endif"):
            if cur:
                spans[cur[0]] = (cur[1], i)
                cur = None
            if m:
                cur = (m.group(1), i)
            elif ln == "#endif" and spans:
                break
    return spans


def splice_preserved(text: str, old: str) -> str:
    """Replace the PRESERVE blocks of the generated text with those of the existing header."""
    new_l, old_l = text.split("\n"), old.split("\n")
    new_b, old_b = genre_blocks(text), genre_blocks(old)
    for macro in sorted(PRESERVE, key=lambda k: -new_b[k][0]):
        if macro not in old_b:
            raise SystemExit(f"{OUT.relative_to(ROOT)}: no {macro} block to preserve")
        (ns, ne), (os_, oe) = new_b[macro], old_b[macro]
        new_l[ns:ne] = old_l[os_:oe]
    return "\n".join(new_l)


def grid(m: int) -> str:
    return "|".join("".join("x" if on(m, b * 4 + i) else "." for i in range(4)) for b in range(4))


def show(tables, name: str) -> int:
    found = 0
    for app, _, _, plates, views, _ in tables:
        for pi, ((pname, _, _), vs) in enumerate(zip(plates, views)):
            if pname != name.upper():
                continue
            found += 1
            print(f"{app} {pi} {pname}")
            for lane in range(6):
                print(f"  {LANES[lane]}  " + "  ".join(f"{grid(vs[v][lane])}" for v in range(VIEWS)))
            print("     " + "  ".join(f"{f'S{v + 1} {ROLES[v]}':<19}" for v in range(VIEWS)))
    if not found:
        print(f"no plate {name}", file=sys.stderr)
    return 0 if found else 1


def report(tables) -> None:
    for app, _, _, plates, views, stats in tables:
        entries = len(plates) * 6 * (VIEWS - 1)
        print(f"{app}: {len(plates)} plates x 6 lanes x {VIEWS - 1} views = {entries} entries, {entries * 2} B;"
              f" fallback {stats['fallback']}, duplicate views {stats['dupes']}")
        cols = []
        for v in range(1, VIEWS):
            d = [100 * (sum(hits(m) for m in vs[v]) - sum(hits(m) for m in vs[0])) / max(1, sum(hits(m) for m in vs[0]))
                 for vs in views]
            ch = [sum(bin(a ^ b).count("1") for a, b in zip(vs[v], vs[0])) for vs in views]
            cols.append(f"{ROLES[v]} {sum(d) / len(d):+.0f}% [{min(d):+.0f}..{max(d):+.0f}] {sum(ch) / len(ch):.1f}st")
        print("  density avg [min..max], changed steps: " + "; ".join(cols))


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--check", action="store_true", help="exit 1 if the header is stale")
    ap.add_argument("--show", metavar="PLATE", help="print S1..S8 of a plate")
    a = ap.parse_args()
    text, tables = build()
    if a.show:
        return show(tables, a.show)
    if not OUT.is_file():
        raise SystemExit(f"{OUT.relative_to(ROOT)} missing: the {', '.join(sorted(PRESERVE))} block is hand-authored")
    text = splice_preserved(text, OUT.read_text())
    if a.check:
        hp_chk = subprocess.run(
            [sys.executable, str(ROOT / "tools" / "gen_hp_lead_riffs.py"), "--check"],
            cwd=ROOT,
        )
        if hp_chk.returncode != 0:
            return hp_chk.returncode
        if OUT.read_text() != text:
            print(f"{OUT.relative_to(ROOT)} is stale, run tools/gen_views.py", file=sys.stderr)
            return 1
        return 0
    OUT.write_text(text)
    print(f"wrote {OUT.relative_to(ROOT)} ({len(text.encode())} B source)")
    report(tables)
    return 0


if __name__ == "__main__":
    sys.exit(main())
