#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Dev HTTP server for build/emu on 127.0.0.1:8767 with on-demand mtime builds.

Rebuilds .wasm or .fwsc files automatically before serving if any source in
firmware/ or web/emu/ is newer than the artifact. Also serves index.html,
worklet.js, fm1ota.js, and fm1pkg.js synced from web/emu/.
"""
import http.server
import json
import os
import shutil
import subprocess
import sys
import threading
from pathlib import Path
from urllib.parse import urlparse

ROOT = Path(__file__).resolve().parents[2]
EMU_SRC = ROOT / "web" / "emu"
EMU_OUT = ROOT / "build" / "emu"
FW_OUT = EMU_OUT / "fw"

BUILD_LOCK = threading.Lock()

WASM_TARGETS = {
    "/triptych.wasm": ("triptych-only", EMU_OUT / "triptych.wasm"),
    "/kosmar_fm1.wasm": ("triptych-only", EMU_OUT / "kosmar_fm1.wasm"),
    "/fm_rytm.wasm": ("rytm-only", EMU_OUT / "fm_rytm.wasm"),
    "/fm_ctrl.wasm": ("ctrl-only", EMU_OUT / "fm_ctrl.wasm"),
    "/fm_dub.wasm": ("dub-only", EMU_OUT / "fm_dub.wasm"),
    "/fm_house.wasm": ("house-only", EMU_OUT / "fm_house.wasm"),
    "/fm_heavypsych.wasm": ("heavypsych-only", EMU_OUT / "fm_heavypsych.wasm"),
    "/fm_jazz.wasm": ("jazz-only", EMU_OUT / "fm_jazz.wasm"),
    "/fm_soul.wasm": ("soul-only", EMU_OUT / "fm_soul.wasm"),
    "/fm_idm.wasm": ("idm-only", EMU_OUT / "fm_idm.wasm"),
    "/fm_snowcrash.wasm": ("snowcrash-only", EMU_OUT / "fm_snowcrash.wasm"),
    "/fm_hiphop.wasm": ("hiphop-only", EMU_OUT / "fm_hiphop.wasm"),
    "/fm_pop.wasm": ("pop-only", EMU_OUT / "fm_pop.wasm"),
    "/fm_funk.wasm": ("funk-only", EMU_OUT / "fm_funk.wasm"),
    "/fm_techno.wasm": ("techno-only", EMU_OUT / "fm_techno.wasm"),
    "/fm_triphop.wasm": ("triphop-only", EMU_OUT / "fm_triphop.wasm"),
    "/fm_bass.wasm": ("bass-only", EMU_OUT / "fm_bass.wasm"),
    "/fm_deck.wasm": ("deck-only", EMU_OUT / "fm_deck.wasm"),
    "/fm_jet.wasm": ("jet-only", EMU_OUT / "fm_jet.wasm"),
}

FWSC_TARGETS = {
    "/fw/triptych.fwsc": ("triptych", ROOT / "build" / "triptych.fwsc", FW_OUT / "triptych.fwsc"),
    "/fw/fm_rytm.fwsc": ("rytm", ROOT / "build" / "fm_rytm.fwsc", FW_OUT / "fm_rytm.fwsc"),
    "/fw/fm_ctrl.fwsc": ("ctrl", ROOT / "build" / "fm_ctrl.fwsc", FW_OUT / "fm_ctrl.fwsc"),
    "/fw/fm_dub.fwsc": ("dub", ROOT / "build" / "fm_dub.fwsc", FW_OUT / "fm_dub.fwsc"),
    "/fw/fm_house.fwsc": ("house", ROOT / "build" / "fm_house.fwsc", FW_OUT / "fm_house.fwsc"),
    "/fw/fm_hvypsy.fwsc": ("heavypsych", ROOT / "build" / "fm_hvypsy.fwsc", FW_OUT / "fm_hvypsy.fwsc"),
    "/fw/fm_jazz.fwsc": ("jazz", ROOT / "build" / "fm_jazz.fwsc", FW_OUT / "fm_jazz.fwsc"),
    "/fw/fm_soul.fwsc": ("soul", ROOT / "build" / "fm_soul.fwsc", FW_OUT / "fm_soul.fwsc"),
    "/fw/fm_idm.fwsc": ("idm", ROOT / "build" / "fm_idm.fwsc", FW_OUT / "fm_idm.fwsc"),
    "/fw/fm_snowcrash.fwsc": ("snowcrash", ROOT / "build" / "fm_snowcrash.fwsc", FW_OUT / "fm_snowcrash.fwsc"),
    "/fw/fm_hiphop.fwsc": ("hiphop", ROOT / "build" / "fm_hiphop.fwsc", FW_OUT / "fm_hiphop.fwsc"),
    "/fw/fm_pop.fwsc": ("pop", ROOT / "build" / "fm_pop.fwsc", FW_OUT / "fm_pop.fwsc"),
    "/fw/fm_funk.fwsc": ("funk", ROOT / "build" / "fm_funk.fwsc", FW_OUT / "fm_funk.fwsc"),
    "/fw/fm_techno.fwsc": ("techno", ROOT / "build" / "fm_techno.fwsc", FW_OUT / "fm_techno.fwsc"),
    "/fw/fm_triphop.fwsc": ("triphop", ROOT / "build" / "fm_triphop.fwsc", FW_OUT / "fm_triphop.fwsc"),
    "/fw/fm_bass.fwsc": ("bass", ROOT / "build" / "fm_bass.fwsc", FW_OUT / "fm_bass.fwsc"),
    "/fw/fm_deck.fwsc": ("deck", ROOT / "build" / "fm_deck.fwsc", FW_OUT / "fm_deck.fwsc"),
    "/fw/fm_jet.fwsc": ("jet", ROOT / "build" / "fm_jet.fwsc", FW_OUT / "fm_jet.fwsc"),
}

# Copied into build/emu/fw/ when present (no auto-build via tools/build.py).
FW_OPTIONAL_COPY = {
    "/fw/x0x.fwsc": [
        Path("/tmp/fm1-x0x/build/x0x.fwsc"),
        Path("/tmp/fm1-x0x/x0x.fwsc"),
        Path("/tmp/fm1-x0x/dist/x0x.fwsc"),
        ROOT / "build" / "x0x.fwsc",
    ],
    "/fw/sloopdx.fwsc": [ROOT / "build" / "sloopdx.fwsc"],
    "/fw/felucca.fwsc": [ROOT / "build" / "felucca.fwsc"],
}

STATIC_SYNC = {
    "/": ("index.html",),
    "/index.html": ("index.html",),
    "/worklet.js": ("worklet.js",),
    "/fm1ota.js": ("fm1ota.js",),
    "/fm1pkg.js": ("fm1pkg.js",),
}


def newest_mtime(paths):
    latest = 0.0
    for p in paths:
        if p.is_dir():
            for sub in p.rglob("*"):
                if sub.is_file() and not sub.name.startswith("."):
                    try:
                        t = sub.stat().st_mtime
                        if t > latest:
                            latest = t
                    except OSError:
                        pass
        elif p.is_file():
            try:
                t = p.stat().st_mtime
                if t > latest:
                    latest = t
            except OSError:
                pass
    return latest


def wasm_sources_mtime():
    return newest_mtime([
        ROOT / "firmware" / "src",
        ROOT / "firmware" / "gen",
        EMU_SRC / "emu_web.c",
        EMU_SRC / "emu_platform.h",
        EMU_SRC / "build.sh",
    ])


def fwsc_sources_mtime():
    return newest_mtime([
        ROOT / "firmware" / "src",
        ROOT / "firmware" / "hal",
        ROOT / "firmware" / "loader",
        ROOT / "firmware" / "gen",
        ROOT / "firmware" / "crt0.S",
        ROOT / "firmware" / "app.ld",
        ROOT / "tools" / "build.py",
        ROOT / "tools" / "fm1pkg_make.py",
    ])


def write_inventory():
    EMU_OUT.mkdir(parents=True, exist_ok=True)
    FW_OUT.mkdir(parents=True, exist_ok=True)
    inv = {
        "wasm": sorted(p.name for p in EMU_OUT.glob("*.wasm") if p.is_file()),
        "fw": sorted(p.name for p in FW_OUT.glob("*.fwsc") if p.is_file()),
    }
    dst = EMU_OUT / "inventory.json"
    body = json.dumps(inv, separators=(",", ":"))
    if not dst.is_file() or dst.read_text(encoding="utf-8") != body:
        dst.write_text(body, encoding="utf-8")


def sync_optional_fw():
    FW_OUT.mkdir(parents=True, exist_ok=True)
    for _url, candidates in FW_OPTIONAL_COPY.items():
        name = Path(_url).name
        dst = FW_OUT / name
        if dst.is_file():
            continue
        for src in candidates:
            if src.is_file():
                shutil.copy2(src, dst)
                break


def sync_static():
    EMU_OUT.mkdir(parents=True, exist_ok=True)
    FW_OUT.mkdir(parents=True, exist_ok=True)
    for name in ("index.html", "worklet.js", "fm1ota.js", "fm1pkg.js"):
        src = EMU_SRC / name
        dst = EMU_OUT / name
        if src.is_file():
            if not dst.is_file() or src.stat().st_mtime > dst.stat().st_mtime:
                shutil.copy2(src, dst)
    stock = ROOT / "firmware" / "stock" / "FM-1_V15.fwsc"
    stock_dst = FW_OUT / "FM-1_V15.fwsc"
    if stock.is_file():
        if not stock_dst.is_file() or stock.stat().st_mtime > stock_dst.stat().st_mtime:
            shutil.copy2(stock, stock_dst)
    sync_optional_fw()
    write_inventory()


def ensure_wasm(arg, target_path):
    with BUILD_LOCK:
        src_t = wasm_sources_mtime()
        if target_path.is_file() and target_path.stat().st_mtime >= src_t:
            return True, ""
        print(f"[serve] rebuilding WASM ({arg}) ...", flush=True)
        r = subprocess.run(
            ["sh", str(EMU_SRC / "build.sh"), arg],
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if r.returncode != 0:
            msg = (r.stdout + "\n" + r.stderr).strip()
            print(f"[serve] WASM build failed:\n{msg}", file=sys.stderr, flush=True)
            if target_path.is_file():
                print(f"[serve] serving existing {target_path.name} (stale)", flush=True)
                return True, ""
            return False, msg
        return True, ""


def ensure_fwsc(app, build_pkg, fw_pkg):
    with BUILD_LOCK:
        src_t = fwsc_sources_mtime()
        # Trust build/ output from tools/build.py — never fw_pkg alone (wasm build may cp -p now,
        # but a stale copy used to get a fresh mtime from plain cp).
        if build_pkg.is_file() and build_pkg.stat().st_mtime >= src_t:
            FW_OUT.mkdir(parents=True, exist_ok=True)
            shutil.copy2(build_pkg, fw_pkg)
            return True, ""
        print(f"[serve] rebuilding FWSC (--app {app}) ...", flush=True)
        cmd = [str(ROOT / "build.sh"), "--app", app]
        r = subprocess.run(
            cmd,
            cwd=ROOT,
            capture_output=True,
            text=True,
        )
        if r.returncode != 0:
            msg = (r.stdout + "\n" + r.stderr).strip()
            print(f"[serve] FWSC build failed:\n{msg}", file=sys.stderr, flush=True)
            return False, msg
        if not build_pkg.is_file():
            return False, f"missing package after build: {build_pkg}"
        FW_OUT.mkdir(parents=True, exist_ok=True)
        shutil.copy2(build_pkg, fw_pkg)
        return True, ""


class DevHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(EMU_OUT), **kwargs)

    def end_headers(self):
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
        super().end_headers()

    def _prepare(self):
        path = urlparse(self.path).path
        if path in STATIC_SYNC or path == "/inventory.json" or path.startswith("/fw/"):
            sync_static()
        elif path in WASM_TARGETS:
            arg, target = WASM_TARGETS[path]
            ok, err = ensure_wasm(arg, target)
            if not ok:
                self._send_build_error(err)
                return False
        elif path in FWSC_TARGETS:
            # For HEAD requests at page load, if a package doesn't exist yet or is stale,
            # build it so the button is enabled and fresh.
            app, build_pkg, fw_pkg = FWSC_TARGETS[path]
            ok, err = ensure_fwsc(app, build_pkg, fw_pkg)
            if not ok:
                self._send_build_error(err)
                return False
        return True

    def _send_build_error(self, err):
        body = f"Build failed:\n{err}\n".encode("utf-8", errors="replace")
        self.send_response(500)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def do_GET(self):
        if not self._prepare():
            return
        super().do_GET()

    def do_HEAD(self):
        if not self._prepare():
            return
        super().do_HEAD()


def main():
    port = int(os.environ.get("PORT", "8767"))
    bind = os.environ.get("BIND", "127.0.0.1")
    sync_static()
    http.server.ThreadingHTTPServer.allow_reuse_address = True
    httpd = http.server.ThreadingHTTPServer((bind, port), DevHandler)
    print(f"Serving {EMU_OUT} on http://{bind}:{port}/ (on-demand auto-build active)", flush=True)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
