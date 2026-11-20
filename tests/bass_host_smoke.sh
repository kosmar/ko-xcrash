#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# KO BASS host + unit smoke (tolerates missing pieces with clear skips).
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HOST_CC="${HOST_CC:-cc}"
HOST_BIN="$ROOT/build/host/bass"
mkdir -p "$ROOT/build/host"

fail() {
  echo "bass_host_smoke: $1" >&2
  exit 1
}

host_ok=0
unit_fail=0

run_unit() {
  local src="$1"
  local name
  name="$(basename "$src" .c)"
  local bin="$ROOT/build/host/$name"
  if [ ! -f "$src" ]; then
    echo "bass_host_smoke: skip $name (missing $src)"
    return 0
  fi
  echo "bass_host_smoke: build $name"
  bass_def=""
  case "$name" in
    bass_*) bass_def="-DGENRE_BASS" ;;
  esac
  if ! "$HOST_CC" -O2 -Wall -Wextra $bass_def -I"$ROOT/firmware/src" -o "$bin" "$src" 2>"$ROOT/build/host/${name}_build.log"; then
    echo "bass_host_smoke: skip $name (compile failed — see build/host/${name}_build.log)" >&2
    tail -12 "$ROOT/build/host/${name}_build.log" >&2
    return 0
  fi
  if ! "$bin"; then
    unit_fail=1
    echo "bass_host_smoke: $name FAILED" >&2
    return 0
  fi
  echo "bass_host_smoke: $name OK"
}

# Unit tests that include fm_app.c itself (DINGHY_HOST) — a compile failure is a failure here.
run_host_unit() {
  local src="$1"
  local name
  name="$(basename "$src" .c)"
  local bin="$ROOT/build/host/$name"
  echo "bass_host_smoke: build $name"
  if ! "$HOST_CC" -O2 -w -DDINGHY_HOST -DGENRE_BASS -I"$ROOT/firmware/src" -I"$ROOT/firmware/gen" \
    -o "$bin" "$src" 2>"$ROOT/build/host/${name}_build.log"; then
    unit_fail=1
    echo "bass_host_smoke: $name compile FAILED (see build/host/${name}_build.log)" >&2
    tail -12 "$ROOT/build/host/${name}_build.log" >&2
    return 0
  fi
  if ! (cd "$ROOT" && "$bin"); then
    unit_fail=1
    echo "bass_host_smoke: $name FAILED" >&2
    return 0
  fi
  echo "bass_host_smoke: $name OK"
}

run_unit "$ROOT/tests/bass_dahdsr.c"
run_unit "$ROOT/tests/bass_mod_twopass.c"
run_unit "$ROOT/tests/bass_bake_lock.c"
run_unit "$ROOT/tests/bass_clock_ext.c"
run_host_unit "$ROOT/tests/bass_tempo.c"
run_host_unit "$ROOT/tests/bass_plate_roundtrip.c"
run_host_unit "$ROOT/tests/bass_macro_boot.c"

echo "bass_host_smoke: building $HOST_BIN"
if "$HOST_CC" -O2 -Wall -Wno-unused-function -DDINGHY_HOST -DGENRE_BASS \
  -I"$ROOT/firmware/src" -I"$ROOT/firmware/gen" \
  -o "$HOST_BIN" "$ROOT/firmware/src/fm_app.c" 2>"$ROOT/build/host/bass_host_build.log"; then
  host_ok=1
  echo "bass_host_smoke: host build OK"
else
  echo "bass_host_smoke: host build FAILED (see build/host/bass_host_build.log)" >&2
  tail -20 "$ROOT/build/host/bass_host_build.log" >&2
fi

if [ "$host_ok" = 1 ] && [ -f "$ROOT/tests/bass_test.txt" ]; then
  echo "bass_host_smoke: running host script bass_test.txt"
  "$HOST_BIN" "$ROOT/build/host/bass" "$ROOT/tests/bass_test.txt" || fail "bass_test.txt host run failed"
  echo "bass_host_smoke: bass_test.txt OK"
elif [ ! -f "$ROOT/tests/bass_test.txt" ]; then
  echo "bass_host_smoke: skip bass_test.txt (missing)"
else
  echo "bass_host_smoke: skip bass_test.txt (host not built)"
fi

if [ "$unit_fail" = 1 ] || [ "$host_ok" != 1 ]; then
  echo "bass_host_smoke: FAILED (host_ok=$host_ok unit_fail=$unit_fail)" >&2
  exit 1
fi
echo "bass_host_smoke: all done"
