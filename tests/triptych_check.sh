#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# TRIPTYCH host regression: WAV, PPM, MIDI-out log, struct layout probe.
# usage: tests/triptych_check.sh save|check LABEL [--no-ui] [--no-midi]
set -e
cd "$(dirname "$0")/.."
ROOT=build/triptych_check
RUN=$ROOT/run
BIN=$ROOT/dinghy
LAY=$ROOT/layout

run_all() {
    rm -rf "$RUN"
    mkdir -p "$RUN" "$ROOT"
    cc -O2 -Wall -w -DDINGHY_HOST -DAPP_VER='"dev"' -Ifirmware/src -Ifirmware/gen \
        -o "$BIN" firmware/src/dinghy.c
    "$BIN" "$RUN/default" >"$RUN/default.log" 2>&1
    grep 'MIDI out' "$RUN/default.log" >"$RUN/default.midi.txt" || true
    "$BIN" "$RUN/midi" tests/triptych_midi_test.txt >"$RUN/midi.log" 2>&1
    grep 'MIDI out' "$RUN/midi.log" >"$RUN/midi.midi.txt" || true
    "$BIN" "$RUN/clock" tests/triptych_clock_test.txt >"$RUN/clock.log" 2>&1
    grep 'MIDI out' "$RUN/clock.log" >"$RUN/clock.midi.txt" || true
    "$BIN" "$RUN/bank" tests/triptych_bank_test.txt >"$RUN/bank.log" 2>&1
    grep 'MIDI out' "$RUN/bank.log" >"$RUN/bank.midi.txt" || true
    cc -O2 -Wall -w -DDINGHY_HOST -Ifirmware/src -Ifirmware/gen \
        -o "$LAY" tests/triptych_layout.c
    "$LAY" >"$RUN/layout.txt"
    (
        cd "$RUN" && shasum -a 256 *.wav *.ppm *.midi.txt 2>/dev/null
        cat layout.txt
    ) >"$RUN/run.sha"
}

case ${1:-} in
save)
    LABEL=$2
    [ -n "$LABEL" ] || { echo "usage: $0 save LABEL" >&2; exit 1; }
    run_all
    cp "$RUN/run.sha" "$ROOT/$LABEL.sha"
    echo "saved $ROOT/$LABEL.sha"
    ;;
check)
    shift
    LABEL=$1
    shift
    [ -n "$LABEL" ] || { echo "usage: $0 check LABEL [--no-ui] [--no-midi]" >&2; exit 1; }
    NO_UI=0
    NO_MIDI=0
    while [ $# -gt 0 ]; do
        case $1 in
        --no-ui) NO_UI=1 ;;
        --no-midi) NO_MIDI=1 ;;
        esac
        shift
    done
    run_all
    want="$ROOT/$LABEL.sha"
    [ -f "$want" ] || { echo "missing $want (run save first)" >&2; exit 1; }
    cp "$want" "$RUN/want.sha"
    cp "$RUN/run.sha" "$RUN/got.sha"
    if [ "$NO_UI" = 1 ]; then
        grep -v '\.ppm$' "$RUN/want.sha" >"$RUN/want.f" || true
        mv "$RUN/want.f" "$RUN/want.sha"
        grep -v '\.ppm$' "$RUN/got.sha" >"$RUN/got.f" || true
        mv "$RUN/got.f" "$RUN/got.sha"
    fi
    if [ "$NO_MIDI" = 1 ]; then
        grep -v '\.midi\.txt$' "$RUN/want.sha" >"$RUN/want.f" || true
        mv "$RUN/want.f" "$RUN/want.sha"
        grep -v '\.midi\.txt$' "$RUN/got.sha" >"$RUN/got.f" || true
        mv "$RUN/got.f" "$RUN/got.sha"
    fi
    diff -u "$RUN/want.sha" "$RUN/got.sha"
    echo "triptych check OK"
    ;;
*)
    echo "usage: $0 save|check LABEL [--no-ui] [--no-midi]" >&2
    exit 1
    ;;
esac
