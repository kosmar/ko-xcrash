#!/usr/bin/env bash
# Endless plate continuum check for one genre: tests/pgen_check.sh <genre-flag-suffix>, e.g. techno, house.
# Uses tests/pgen_golden/<genre>.txt (locked hash) and tests/pgen_rules_<genre>.h (extra hard rules) if present.
set -euo pipefail
cd "$(dirname "$0")/.."
g="${1:?genre, e.g. techno}"
G=$(printf '%s' "$g" | tr '[:lower:]' '[:upper:]')
flags=(-O2 -Wall -Wno-unused-function "-DGENRE_$G" -Ifirmware/src -Ifirmware/gen -Itests)
[ -f "tests/pgen_golden/$g.txt" ] && flags+=("-DPGEN_GOLDEN=$(tr -d '[:space:]' < "tests/pgen_golden/$g.txt")")
[ -f "tests/pgen_rules_$g.h" ] && flags+=("-DPGEN_RULES=\"pgen_rules_$g.h\"")
mkdir -p build/host
cc "${flags[@]}" -o "build/host/pgen_$g" tests/plate_gen_drift.c
"build/host/pgen_$g"
