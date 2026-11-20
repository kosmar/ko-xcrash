#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Per-lane PACE check (host, KO DUB): lane 1 = 1/32, lane 2 = 1/2, the rest 1/16.
# Counts LANE trace lines in the window [T0, T1) and compares them with the
# expected count from the lane's own samples-per-step (tolerance +-1 event).
set -e
cd "$(dirname "$0")/.."
T0="${T0:-2000}"
T1="${T1:-16000}"
mkdir -p build/host
cc -O2 -Wall -Wno-unused-function -DDINGHY_HOST -DDUB_LANE_TRACE -include stdio.h -DAPP_VER=\"dev\" \
   -Ifirmware/src -Ifirmware/gen -o build/host/lane_pace firmware/src/fm_app.c 2>/dev/null
build/host/lane_pace build/host/pace_lane tests/lane_pace_test.txt 2> build/host/lane_pace_trace.txt >/dev/null
awk -v t0="$T0" -v t1="$T1" '
  $1 == "LANE" && $3 >= t0 && $3 < t1 { cnt[$2]++; spp[$2] = $5 }
  END {
    fail = 0
    for (l = 0; l < 6; l++) {
      if (!(l in cnt)) { printf "lane %d: no events\n", l; fail = 1; continue }
      ex = (t1 - t0) * 44.1 / spp[l]
      d = cnt[l] - ex
      if (d < 0) d = -d
      ok = d <= 1.0
      if (!ok) fail = 1
      printf "lane %d: spp=%d events=%d expected=%.1f %s\n", l, spp[l], cnt[l], ex, ok ? "OK" : "FAIL"
    }
    if (cnt[1] > 0 && cnt[0] > 0) printf "ratio lane1/lane0 = %.2f (want 2.00)\n", cnt[1] / cnt[0]
    if (cnt[2] > 0 && cnt[0] > 0) printf "ratio lane0/lane2 = %.2f (want 8.00)\n", cnt[0] / cnt[2]
    exit fail
  }' build/host/lane_pace_trace.txt
echo "lane_pace_check: PASS"
