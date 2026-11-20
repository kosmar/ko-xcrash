#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run POP lead verify suite and check thresholds."""
import math
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    sh = ROOT / "tests" / "measure_pop_lead.sh"
    out = subprocess.check_output(["sh", str(sh)], text=True, cwd=ROOT)
    print(out, end="")
    fail = 0
    m = re.search(r"MELODY_DIFF count=(\d+)", out)
    if not m or int(m.group(1)) != 0:
        print("FAIL melody diff", file=sys.stderr)
        fail = 1
    for line in out.splitlines():
        if line.startswith("PLATE ") and "diff_chg=" in line:
            chg = int(re.search(r"diff_chg=(\d+)", line).group(1))
            if chg < 30:
                print(f"FAIL timbre {line}", file=sys.stderr)
                fail = 1
            sus = int(re.search(r"amp_sus_pct=(\d+)", line).group(1))
            if sus > 85 or sus < 40:
                print(f"FAIL amp_sus {line}", file=sys.stderr)
                fail = 1
        if line.startswith("BENCH_LEAD_") and "ns=" in line:
            ns = int(line.split("ns=")[-1])
            if ns > 20:
                print(f"FAIL cpu {line}", file=sys.stderr)
                fail = 1
        if "BENCH_CHORD ns=" in line:
            ch = int(line.split("ns=")[-1])
            for l in out.splitlines():
                if l.startswith("BENCH_LEAD_"):
                    if int(l.split("ns=")[-1]) > ch + 2:
                        print(f"FAIL lead > chord {l}", file=sys.stderr)
                        fail = 1
    return fail


if __name__ == "__main__":
    sys.exit(main())
