#!/bin/sh
# TRIPTYCH settings: v0/TRP1 migrate on boot, TRP2 SAVE roundtrip (needs S3 HOST_STORE_*).
set -e
ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
OUT_DIR="build/triptych_check"
mkdir -p "$OUT_DIR"

CC="${CC:-cc}"
COMMON="-O2 -Wall -w -DDINGHY_HOST -DAPP_VER='\"dev\"' -Ifirmware/src -Ifirmware/gen"

"$CC" $COMMON -o "$OUT_DIR/triptych_v0_store" tests/triptych_v0_store.c
"$CC" $COMMON -o "$OUT_DIR/triptych_trp1_store" tests/triptych_trp1_store.c
"$CC" $COMMON -o "$OUT_DIR/dinghy_save" firmware/src/dinghy.c

V0="$OUT_DIR/tryp_v0.bin"
TRP1="$OUT_DIR/tryp_trp1_seed.bin"
TRP2="$OUT_DIR/tryp_trp2.bin"
LOG_MIG="$OUT_DIR/save_migrate.log"
LOG_P2="$OUT_DIR/save_p2.log"
LOG_RELOAD="$OUT_DIR/save_p2_reload.log"
LOG_TRP1="$OUT_DIR/save_trp1_migrate.log"

rm -f "$V0" "$TRP1" "$TRP2"
"$OUT_DIR/triptych_v0_store" "$V0"
"$OUT_DIR/triptych_trp1_store" "$TRP1"

# v0 -> TRP2 on first SAVE
HOST_STORE_IN="$V0" HOST_STORE_OUT="$TRP2" "$OUT_DIR/dinghy_save" "$OUT_DIR/save_run" tests/triptych_save_test.txt \
    >"$LOG_MIG" 2>&1 || true
grep '^cfg ' "$LOG_MIG" | head -1 >"$OUT_DIR/save_migrate.cfg" || true

if [ ! -s "$TRP2" ]; then
    echo "triptych_save: FAIL no TRP2 store after v0 migration" >&2
    exit 1
fi
if ! grep -q 'magic=54525032' "$OUT_DIR/save_migrate.cfg"; then
    echo "triptych_save: FAIL expected TRP2 magic after v0 migration" >&2
    exit 1
fi

# TRP2 roundtrip: LFO, SEQ, CONTURA scale -> CYCLIC -> SAVE -> reload
HOST_STORE_IN="$TRP2" HOST_STORE_OUT="$TRP2" "$OUT_DIR/dinghy_save" "$OUT_DIR/save_p2_run" tests/triptych_save_p2_test.txt \
    >"$LOG_P2" 2>&1 || true
grep '^cfg ' "$LOG_P2" | tail -1 >"$OUT_DIR/save_p2.cfg" || true

HOST_STORE_IN="$TRP2" "$OUT_DIR/dinghy_save" "$OUT_DIR/save_p2_reload" tests/triptych_save_p2_reload.txt \
    >"$LOG_RELOAD" 2>&1 || true
grep '^cfg ' "$LOG_RELOAD" | tail -1 >"$OUT_DIR/save_p2_reload.cfg" || true

P2_SCALE=$(sed -n 's/.*contura_scale=\([0-9]*\).*/\1/p' "$OUT_DIR/save_p2.cfg" | head -1)
RELOAD_AXIS=$(sed -n 's/.*live_axis=\([0-9]*\).*/\1/p' "$OUT_DIR/save_p2_reload.cfg" | head -1)
if [ -z "$P2_SCALE" ] || [ -z "$RELOAD_AXIS" ] || [ "$P2_SCALE" != "$RELOAD_AXIS" ]; then
    echo "triptych_save: FAIL CONTURA scale roundtrip (saved=$P2_SCALE reload=$RELOAD_AXIS)" >&2
    exit 1
fi
if ! grep -q 'lfo_r0=[1-9]' "$OUT_DIR/save_p2.cfg"; then
    echo "triptych_save: FAIL LFO rate slot not persisted" >&2
    exit 1
fi
if ! grep -qE 'seq_s0=-?[1-9]' "$OUT_DIR/save_p2.cfg"; then
    echo "triptych_save: FAIL SEQ step not persisted" >&2
    exit 1
fi

# TRP1 blob -> TRP2 on load (no script SAVE needed)
HOST_STORE_IN="$TRP1" "$OUT_DIR/dinghy_save" "$OUT_DIR/save_trp1_run" tests/triptych_save_reload.txt \
    >"$LOG_TRP1" 2>&1 || true
grep '^cfg ' "$LOG_TRP1" | head -1 >"$OUT_DIR/save_trp1_migrate.cfg" || true
if ! grep -q 'magic=54525032' "$OUT_DIR/save_trp1_migrate.cfg"; then
    echo "triptych_save: FAIL TRP1->TRP2 migration magic" >&2
    exit 1
fi
if ! grep -q 'timbre=55' "$OUT_DIR/save_trp1_migrate.cfg"; then
    echo "triptych_save: FAIL TRP1 prefix not migrated" >&2
    exit 1
fi

echo "triptych_save: OK"
