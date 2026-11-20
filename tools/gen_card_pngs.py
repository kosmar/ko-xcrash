#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host-mode screen → 240×240 docs/card.png per app (publish/assets/cards/)."""
from __future__ import annotations

import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "publish" / "assets" / "cards"
MINI = "50 end\n"

APPS: list[tuple[str, list[str], Path, str | None]] = [
    ("dub", [], ROOT / "firmware/src/fm_app.c", None),
    ("house", ["-DGENRE_HOUSE"], ROOT / "firmware/src/fm_app.c", None),
    ("heavypsych", ["-DGENRE_HEAVY_PSYCH"], ROOT / "firmware/src/fm_app.c", None),
    ("jazz", ["-DGENRE_JAZZ"], ROOT / "firmware/src/fm_app.c", None),
    ("idm", ["-DGENRE_IDM"], ROOT / "firmware/src/fm_app.c", None),
    ("hiphop", ["-DGENRE_HIPHOP"], ROOT / "firmware/src/fm_app.c", None),
    ("pop", ["-DGENRE_POP"], ROOT / "firmware/src/fm_app.c", None),
    ("funk", ["-DGENRE_FUNK"], ROOT / "firmware/src/fm_app.c", None),
    ("techno", ["-DGENRE_TECHNO"], ROOT / "firmware/src/fm_app.c", None),
    ("bass", ["-DGENRE_BASS"], ROOT / "firmware/src/fm_app.c", None),
    ("snowcrash", ["-DGENRE_SNOWCRASH"], ROOT / "firmware/src/fm_app.c", None),
    ("triphop", ["-DGENRE_TRIPHOP"], ROOT / "firmware/src/fm_app.c", None),
    ("soul", ["-DGENRE_SOUL"], ROOT / "firmware/src/fm_app.c", None),
    ("deck", ["-DLAYOUT_APP_DECK"], ROOT / "firmware/src/deck.c", None),
    ("jet", [], ROOT / "firmware/src/jet.c", None),
]


def ppm_to_png_240(ppm: Path, png: Path) -> None:
    from PIL import Image  # noqa: PLC0415

    with ppm.open("rb") as f:
        header = f.readline()
        if header.strip() != b"P6":
            raise ValueError(f"{ppm}: not P6")
        dims = f.readline().decode().strip()
        while dims.startswith("#"):
            dims = f.readline().decode().strip()
        w, h = map(int, dims.split())
        maxval = int(f.readline().strip())
        if maxval != 255:
            raise ValueError(f"{ppm}: maxval {maxval}")
        raw = f.read(w * h * 3)
    img = Image.frombytes("RGB", (w, h), raw)
    img = img.resize((240, 240), Image.NEAREST)
    png.parent.mkdir(parents=True, exist_ok=True)
    img.save(png, format="PNG", optimize=True)
    if png.stat().st_size > 300 * 1024:
        img.save(png, format="PNG", optimize=True, compress_level=9)


def run_app(slug: str, cflags: list[str], src: Path) -> Path:
    build = ROOT / "build" / "cardgen" / slug
    build.parent.mkdir(parents=True, exist_ok=True)
    common = [
        "cc",
        "-O2",
        "-w",
        "-DDINGHY_HOST",
        "-Ifirmware/src",
        "-Ifirmware/gen",
        '-DAPP_VER="dev"',
        *cflags,
        "-o",
        str(build),
        str(src.relative_to(ROOT)),
    ]
    subprocess.run(common, cwd=ROOT, check=True)
    prefix = ROOT / "build" / "cardgen" / f"{slug}.out"
    for old in (ROOT / "build" / "cardgen").iterdir():
        if old.name.startswith(f"{slug}.out") and old.suffix == ".ppm":
            old.unlink()
    script = ROOT / "build" / "cardgen" / f"{slug}.mini.txt"
    script.write_text(MINI, encoding="utf-8")
    subprocess.run([str(build), str(prefix), str(script)], cwd=ROOT, check=True)
    ppms = sorted(ROOT.glob(f"build/cardgen/{slug}.out*.ppm"))
    if not ppms:
        raise RuntimeError(f"{slug}: no ppm")
    return ppms[-1]


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    ok: list[str] = []
    fail: list[str] = []
    for slug, cflags, src, _ in APPS:
        try:
            ppm = run_app(slug, cflags, src)
            png = OUT / f"{slug}.png"
            ppm_to_png_240(ppm, png)
            ok.append(f"{slug} ← {ppm.name} ({png.stat().st_size} B)")
        except (subprocess.CalledProcessError, OSError, ValueError, RuntimeError) as e:
            fail.append(f"{slug}: {e}")
    print("\n".join(ok))
    if fail:
        print("FAILED:", file=sys.stderr)
        print("\n".join(fail), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())