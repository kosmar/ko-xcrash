#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# KO JET host + unit smoke.
set -e
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HOST_CC="${HOST_CC:-cc}"
HOST_BIN="$ROOT/build/host/jet"
HOST_OUT="$ROOT/build/host/jet_out"
WAV="$HOST_OUT.wav"
mkdir -p "$ROOT/build/host"

fail() {
  echo "jet_host_smoke: $1" >&2
  exit 1
}

host_ok=0
unit_fail=0

run_unit_jet_dsp() {
  local src="$ROOT/tests/jet_dsp_test.c"
  local bin="$ROOT/build/host/jet_dsp_test"
  local log="$ROOT/build/host/jet_dsp_test_build.log"
  local flags="-O2 -Wall -Wextra -fsanitize=undefined -fno-sanitize-recover"

  echo "jet_host_smoke: build jet_dsp_test (UBSan)"
  if ! "$HOST_CC" $flags -I"$ROOT/firmware/src" -o "$bin" "$src" -lm 2>"$log"; then
    echo "jet_host_smoke: UBSan build failed, retry without sanitizer" >&2
    tail -8 "$log" >&2
    flags="-O2 -Wall -Wextra"
    if ! "$HOST_CC" $flags -I"$ROOT/firmware/src" -o "$bin" "$src" -lm 2>"$log"; then
      unit_fail=1
      echo "jet_host_smoke: jet_dsp_test compile FAILED" >&2
      tail -12 "$log" >&2
      return 0
    fi
  fi
  if ! "$bin"; then
    unit_fail=1
    echo "jet_host_smoke: jet_dsp_test FAILED" >&2
    return 0
  fi
  echo "jet_host_smoke: jet_dsp_test OK"
}

run_unit() {
  local src="$1"
  local name
  name="$(basename "$src" .c)"
  local bin="$ROOT/build/host/$name"
  if [ ! -f "$src" ]; then
    echo "jet_host_smoke: skip $name (missing $src)"
    return 0
  fi
  echo "jet_host_smoke: build $name"
  if ! "$HOST_CC" -O2 -Wall -Wextra -I"$ROOT/firmware/src" -o "$bin" "$src" 2>"$ROOT/build/host/${name}_build.log"; then
    unit_fail=1
    echo "jet_host_smoke: $name compile FAILED (see build/host/${name}_build.log)" >&2
    tail -12 "$ROOT/build/host/${name}_build.log" >&2
    return 0
  fi
  if ! "$bin"; then
    unit_fail=1
    echo "jet_host_smoke: $name FAILED" >&2
    return 0
  fi
  echo "jet_host_smoke: $name OK"
}

run_unit_jet_dsp
run_unit "$ROOT/tests/ko_rec.c"

echo "jet_host_smoke: building $HOST_BIN"
if "$HOST_CC" -O2 -Wall -Wno-unused-function -DDINGHY_HOST \
  -I"$ROOT/firmware/src" -I"$ROOT/firmware/gen" \
  -o "$HOST_BIN" "$ROOT/firmware/src/jet.c" 2>"$ROOT/build/host/jet_host_build.log"; then
  host_ok=1
  echo "jet_host_smoke: host build OK"
  for store_n in default 4080u; do
    store_def=""
    [ "$store_n" = default ] || store_def="-DJET_SETTINGS_N=$store_n"
    store_bin="$ROOT/build/host/jet_store_test_$store_n"
    if "$HOST_CC" -O2 -Wall -Wno-unused-function -DDINGHY_HOST $store_def \
      -I"$ROOT/firmware/src" -I"$ROOT/firmware/gen" \
      -o "$store_bin" "$ROOT/tests/jet_store_test.c" 2>"$store_bin.build.log" &&
      "$store_bin" >"$store_bin.log"; then
      grep "jet_store_test:" "$store_bin.log"
      echo "jet_host_smoke: jet_store_test ($store_n) OK"
    else
      unit_fail=1
      grep "jet_store_test:" "$store_bin.log" >&2 || true
      tail -20 "$store_bin.build.log" >&2 || true
      echo "jet_host_smoke: jet_store_test ($store_n) FAILED" >&2
    fi
  done
else
  echo "jet_host_smoke: host build FAILED (see build/host/jet_host_build.log)" >&2
  tail -20 "$ROOT/build/host/jet_host_build.log" >&2
fi

if [ "$host_ok" = 1 ] && [ -f "$ROOT/tests/jet_test.txt" ]; then
  echo "jet_host_smoke: running host script jet_test.txt"
  "$HOST_BIN" "$HOST_OUT" "$ROOT/tests/jet_test.txt" || fail "jet_test.txt host run failed"
  echo "jet_host_smoke: jet_test.txt OK"
  if [ ! -f "$WAV" ]; then
    fail "missing $WAV"
  fi
  wav_bytes="$(wc -c < "$WAV" | tr -d ' ')"
  if [ "$wav_bytes" -lt 4096 ]; then
    fail "WAV too small ($wav_bytes bytes)"
  fi
  if ! "$HOST_CC" -O2 -x c - -lm -o "$ROOT/build/host/wav_peak" <<'EOF'
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    FILE *f;
    uint8_t hdr[44];
    int32_t pk = 0;
    int16_t s;
    uint32_t i = 0, nz_head = 0, nz_tail = 0;
    if (argc < 2 || !(f = fopen(argv[1], "rb"))) return 2;
    if (fread(hdr, 1, 44, f) != 44) return 2;
    while (fread(&s, 2, 1, f) == 1) {
        int32_t a = s < 0 ? -s : s;
        uint32_t pos = (i / 2u) % 256u;
        if (a > pk) pk = a;
        if (a) {
            if (pos < 32u) nz_head++;
            else nz_tail++;
        }
        i++;
    }
    fclose(f);
    if (pk < 500) return 1;
    printf("peak=%d nonzero head=%u tail=%u\n", (int)pk, nz_head, nz_tail);
    /* tail covers 7/8 of each 256-frame block: gapless audio has tail >> head */
    if (nz_tail < nz_head * 4u) return 3;
    return 0;
}
EOF
  then
    fail "wav peak helper build failed"
  fi
  if ! "$ROOT/build/host/wav_peak" "$WAV"; then
    fail "WAV silent or gapped (app_audio must fill all HALF_FRAMES)"
  fi
  echo "jet_host_smoke: WAV OK ($WAV, ${wav_bytes} bytes)"
elif [ ! -f "$ROOT/tests/jet_test.txt" ]; then
  echo "jet_host_smoke: skip jet_test.txt (missing)"
else
  echo "jet_host_smoke: skip jet_test.txt (host not built)"
fi

# SEQ / REC: step record, live record, loop playback, save + reload. Levels and pitches are asserted per window.
run_seq() {
  name="$1"
  expect="$2"
  out="$ROOT/build/host/jet_$name"
  echo "jet_host_smoke: running host script jet_$name.txt"
  env HOST_STORE_IN="${3:-}" HOST_STORE_OUT="${4:-}" "$HOST_BIN" "$out" "$ROOT/tests/jet_$name.txt" >"$out.log" ||
    fail "jet_$name.txt host run failed"
  if ! python3 "$ROOT/tests/jet_wav_steps.py" "$out.wav" --expect "$expect"; then
    unit_fail=1
    echo "jet_host_smoke: jet_$name.txt FAILED (audio does not follow the pattern)" >&2
  else
    echo "jet_host_smoke: jet_$name.txt OK"
  fi
}

if [ "$host_ok" = 1 ]; then
  STORE="$ROOT/build/host/jet_seq.store"
  rm -f "$STORE"
  run_seq seq_step "620-700:C4,1080-1160:D#4,1690-1770:C5,3100-3900:<200,7060-7150:C4,7215-7300:E4,7520-7610:G4,8600-9400:<300,9510-9600:C4"
  run_seq seq_live "3200-3440:<600,3515-3600:C4,3700-4050:<2500,4127-4210:E4,4590-4670:G4,4700-4780:>15000,4800-4900:>15000,5100-5450:<3000" "" "$STORE"
  [ -f "$STORE" ] || fail "jet_seq_live.txt did not save settings"
  run_seq seq_reload "100-900:quiet,1310-1370:C4,1978-2060:E4,2440-2520:G4,2600-2680:G4,2760-2840:G4" "$STORE"
  run_seq seq_edit "$(sed -n 's/^# expect: //p' "$ROOT/tests/jet_seq_edit.txt" | tr -d '\n')"
  rm -f "$ROOT"/build/host/jet_banks-*.ppm
  run_seq banks "300-5000:C4,300-5000:>15000"
  shots=$(ls "$ROOT"/build/host/jet_banks-*.ppm 2>/dev/null | wc -l | tr -d ' ')
  [ "$shots" = 17 ] || { unit_fail=1; echo "jet_host_smoke: jet_banks.txt wrote $shots of 17 screenshots" >&2; }
fi

run_unit "$ROOT/tests/jet_param_live.c"

if "$HOST_CC" -O2 -w -DDINGHY_WEB -DEMU_APP_JET -I"$ROOT/firmware/src" -I"$ROOT/firmware/gen" -I"$ROOT/web/emu" \
  -o "$ROOT/build/host/web_jet_audio_harness" "$ROOT/web/emu/emu_web.c" "$ROOT/tests/web_jet_audio_harness.c" \
  2>"$ROOT/build/host/web_jet_audio_harness.log" && "$ROOT/build/host/web_jet_audio_harness"; then
  echo "jet_host_smoke: web_jet_audio_harness OK"
else
  unit_fail=1
  echo "jet_host_smoke: web_jet_audio_harness FAILED" >&2
fi

if [ "$unit_fail" = 1 ] || [ "$host_ok" != 1 ]; then
  echo "jet_host_smoke: FAILED (host_ok=$host_ok unit_fail=$unit_fail)" >&2
  exit 1
fi
echo "jet_host_smoke: all done"
