#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Balance MIDI note-on/off lines from tests/host.c (MIDI out ST D1 D2)."""

from __future__ import annotations

import argparse
import re
import sys
from typing import TextIO

LINE = re.compile(
    r"^\s*(\d+)\s+ms\s+MIDI out\s+([0-9A-Fa-f]{2})\s+(\d+)\s+(\d+)\s*$"
)


def parse_stream(f: TextIO, strict: bool) -> int:
    held: dict[tuple[int, int], int] = {}
    errs = 0

    for raw in f:
        m = LINE.match(raw)
        if not m:
            continue
        t_ms = int(m.group(1))
        st = int(m.group(2), 16)
        note = int(m.group(3))
        vel = int(m.group(4))
        ch = st & 0x0F
        cmd = st & 0xF0
        key = (ch, note)

        if cmd == 0x90 and vel > 0:
            if key in held:
                if strict:
                    print(
                        f"REON ch={ch} note={note} at={t_ms} ms (held since {held[key]} ms)",
                        file=sys.stderr,
                    )
                    errs += 1
            else:
                held[key] = t_ms
        elif cmd == 0x80 or (cmd == 0x90 and vel == 0):
            held.pop(key, None)

    for (ch, note) in sorted(held.keys()):
        since = held[(ch, note)]
        print(f"HUNG ch={ch} note={note} since={since} ms")
        errs += 1

    return errs


def main() -> int:
    ap = argparse.ArgumentParser(description="Check MIDI-out note-on/off balance.")
    ap.add_argument(
        "files",
        nargs="*",
        help="MIDI log files (default: stdin)",
    )
    ap.add_argument(
        "--strict",
        action="store_true",
        help="Fail on note-on while (ch, note) still held",
    )
    args = ap.parse_args()

    total = 0
    if not args.files:
        total += parse_stream(sys.stdin, args.strict)
    else:
        for path in args.files:
            with open(path, encoding="utf-8", errors="replace") as fp:
                total += parse_stream(fp, args.strict)

    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
