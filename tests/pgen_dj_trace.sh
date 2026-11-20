#!/bin/sh
# DJ transitions on the endless plate continuum, any FM_PGEN genre: tests/pgen_dj_trace.sh [techno|house|...].
# Each DJ turn must move n by its step (XF -8, CUEING +8, ECHO_OUT -3, DROP/FILTER_SLAM -997; 0 where the turn
# steps era instead) and keep the compliance offset within +-16. Traces fm_pgen_goto under lldb (no firmware hooks).
set -e
cd "$(dirname "$0")/.."
g="${1:-techno}"
G=$(printf '%s' "$g" | tr '[:lower:]' '[:upper:]')
mkdir -p build/host
cc -O0 -g -Wall -Wno-unused-function -DDINGHY_HOST '-DAPP_VER="dev"' "-DGENRE_$G" -Ifirmware/src -Ifirmware/gen \
    -o "build/host/${g}_dbg" firmware/src/fm_app.c
cmds="build/host/${g}_dj_pgen.lldb"
cat >"$cmds" <<'EOF'
breakpoint set -n fm_dj_on_trans
breakpoint command add 1
p (int)trans
continue
DONE
breakpoint set -n fm_pgen_goto
breakpoint command add 2
p (int)n
p (int)g.cfg.pgen.n
p (int)g.dj_comply_ofs
continue
DONE
run
quit
EOF
lldb -b -s "$cmds" -- "build/host/${g}_dbg" "build/host/${g}_dj_pgen" tests/pgen_dj_test.txt 2>&1 |
    awk '
    /^\(lldb\) +p \(int\)trans/ { want = "t"; next }
    /^\(lldb\) +p \(int\)n$/ { want = "n"; next }
    /^\(lldb\) +p \(int\)g.cfg.pgen.n/ { want = "p"; next }
    /^\(lldb\) +p \(int\)g.dj_comply_ofs/ { want = "o"; next }
    /^\(int\) / {
        v = $2 + 0
        if (want == "t") { trans = v; armed = 1 }
        else if (want == "n") n = v
        else if (want == "p") prev = v
        else if (want == "o" && armed) {
            d = n - prev
            if (trans == 1) ok = (d == -8 || d == 0)
            else if (trans == 2) ok = (d == 8)
            else if (trans == 3) ok = (d == -3 || d == 0)
            else ok = (d == -997 || d == 0)
            if (v < -16 || v > 16) ok = 0
            printf "trans %d: n %d -> %d (%+d) comply ofs %+d %s\n", trans, prev, n, d, v, ok ? "ok" : "FAIL"
            if (!ok) bad = 1
            seen++
            armed = 0
        }
        want = ""
    }
    END {
        if (seen < 3) { print "pgen_dj: too few DJ transitions (" seen ")"; exit 1 }
        if (bad) { print "pgen_dj: FAIL"; exit 1 }
        print "pgen_dj: OK (" seen " transitions)"
    }'
