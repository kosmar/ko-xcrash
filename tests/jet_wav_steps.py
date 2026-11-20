#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Window a host-sim WAV (left channel): print or assert level and pitch per window.

  jet_wav_steps.py out.wav [--win MS] [--from MS] [--to MS]
      one line per window: start ms, peak, pitch (Hz, autocorrelation) and nearest note

  jet_wav_steps.py out.wav --expect "T0-T1:spec,T0-T1:spec,…"
      spec = note name (C4, D#3: peak >= 1500 and pitch within 60 cents), "<N" / ">N" (peak),
             "quiet" (peak < 200), "loud" (peak >= 1500) or "vib" (pitch swings > 80 cents).
      Exit 1 on any mismatch.
"""
import math
import struct
import sys
import wave

NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def load(path):
    w = wave.open(path)
    n = w.getnframes()
    d = struct.unpack("<%dh" % (n * w.getnchannels()), w.readframes(n))
    return w.getframerate(), d[0 :: w.getnchannels()]


def pitch(s, fs):
    """Fundamental by normalised autocorrelation, 70..1500 Hz; 0 if unclear."""
    s = s[: min(len(s), fs // 16)]
    n = len(s)
    lo, hi = fs // 1500, min(fs // 70, n // 2)
    if hi <= lo:
        return 0.0
    mean = sum(s) / n
    s = [x - mean for x in s]
    e0 = sum(x * x for x in s)
    if e0 <= 0:
        return 0.0
    best, r = 0, []
    for lag in range(lo, hi):
        acc = 0.0
        for i in range(0, n - lag, 2):
            acc += s[i] * s[i + lag]
        r.append(acc * 2.0 / e0 * n / (n - lag))
    top = max(r)
    if top < 0.3:
        return 0.0
    for i in range(1, len(r) - 1):  # first clear peak, not an octave below
        if r[i] >= 0.93 * top and r[i] >= r[i - 1] and r[i] >= r[i + 1]:
            best = i
            break
    return fs / float(best + lo) if best else 0.0


def note_of(hz):
    if hz <= 0:
        return "-", 0.0
    m = 69 + 12 * math.log2(hz / 440.0)
    k = int(round(m))
    return "%s%d" % (NAMES[k % 12], k // 12 - 1), (m - k) * 100.0


def measure(samples, fs, t0, t1):
    s = samples[fs * t0 // 1000 : fs * t1 // 1000]
    if len(s) < 2:
        return 0, 0.0
    return max(abs(x) for x in s), pitch(s, fs)


def main():
    a = sys.argv[1:]
    if not a:
        print(__doc__)
        return 2
    fs, l = load(a[0])
    opt = dict(zip(a[1::2], a[2::2]))
    total = len(l) * 1000 // fs
    if "--expect" in opt:
        bad = 0
        for item in opt["--expect"].split(","):
            rng, spec = item.strip().split(":")
            t0, t1 = (int(x) for x in rng.split("-"))
            pk, hz = measure(l, fs, t0, t1)
            nm, cents = note_of(hz)
            if spec == "quiet":
                ok = pk < 200
            elif spec == "loud":
                ok = pk >= 1500
            elif spec == "vib":  # pitch moves by more than 80 cents inside the window
                cs = []
                for t in range(t0, t1 - 30, 30):
                    h = measure(l, fs, t, t + 30)[1]
                    if h > 0:
                        cs.append(1200 * math.log2(h / 440.0))
                ok = pk >= 1500 and len(cs) > 3 and max(cs) - min(cs) > 80
            elif spec[0] == "<":
                ok = pk < int(spec[1:])
            elif spec[0] == ">":
                ok = pk > int(spec[1:])
            else:
                ok = pk >= 1500 and nm == spec and abs(cents) <= 60
            print("%s %5d-%5d ms  want %-5s  peak %5d  %s %+.0f ct" % ("ok  " if ok else "FAIL", t0, t1, spec, pk, nm, cents))
            bad |= not ok
        return 1 if bad else 0
    win = int(opt.get("--win", 125))
    t = int(opt.get("--from", 0))
    end = min(total, int(opt.get("--to", total)))
    print("fs %d  %d ms  peak %d" % (fs, total, max(abs(x) for x in l) if l else 0))
    while t + win <= end:
        pk, hz = measure(l, fs, t, t + win)
        nm, cents = note_of(hz)
        print("%6d ms  peak %5d  %6.1f Hz  %s %+.0f ct" % (t, pk, hz, nm, cents))
        t += win
    return 0


if __name__ == "__main__":
    sys.exit(main())
