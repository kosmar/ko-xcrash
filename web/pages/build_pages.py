#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build static GitHub Pages output for https://kosmar.github.io/ko-fm1/.

Writes ``build/pages/`` (15 KO ``.fwsc`` from ``dist/``, prebuilt WASM, patched
``index.html`` from ``web/emu/``).

Usage::

    python3 web/pages/build_pages.py
    python3 web/pages/build_pages.py --skip-wasm
    python3 web/pages/build_pages.py --base-href /ko-fm1/   # default

Local subpath test::

    mkdir -p build/pages-test/ko-fm1
    rsync -a build/pages/ build/pages-test/ko-fm1/
    python3 -m http.server 8768 --directory build/pages-test
    # http://127.0.0.1:8768/ko-fm1/?app=jazz

Publish (explicit only — clones ``git@github.com:kosmar/ko-fm1.git``, force-replaces
tree, one commit, ``git push origin main``)::

    python3 web/pages/build_pages.py --publish
"""
from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EMU_SRC = ROOT / "web" / "emu"
OUT = ROOT / "build" / "pages"
FW_OUT = OUT / "fw"
DEFAULT_BASE_HREF = "/ko-fm1/"
PUBLISH_REMOTE = "git@github.com:kosmar/ko-fm1.git"
PUBLISH_BRANCH = "main"

# slug, display label, product id, dist filename stem, wasm build arg, wasm filename
KO_APPS = [
    ("dub", "KO DUB", "FM-1_56401", "fm_dub", "dub-only", "fm_dub.wasm"),
    ("house", "KO HOUSE", "FM-1_56402", "fm_house", "house-only", "fm_house.wasm"),
    ("heavypsych", "KO HVYPSY", "FM-1_56403", "fm_hvypsy", "heavypsych-only", "fm_heavypsych.wasm"),
    ("jazz", "KO JAZZ", "FM-1_56404", "fm_jazz", "jazz-only", "fm_jazz.wasm"),
    ("idm", "KO IDM", "FM-1_56405", "fm_idm", "idm-only", "fm_idm.wasm"),
    ("hiphop", "KO HIPHOP", "FM-1_56406", "fm_hiphop", "hiphop-only", "fm_hiphop.wasm"),
    ("pop", "KO POP", "FM-1_56407", "fm_pop", "pop-only", "fm_pop.wasm"),
    ("funk", "KO FUNK", "FM-1_56408", "fm_funk", "funk-only", "fm_funk.wasm"),
    ("techno", "KO TECHNO", "FM-1_56409", "fm_techno", "techno-only", "fm_techno.wasm"),
    ("bass", "KO BASS", "FM-1_56410", "fm_bass", "bass-only", "fm_bass.wasm"),
    ("snowcrash", "KO XCRASH", "FM-1_56411", "fm_snowcrash", "snowcrash-only", "fm_snowcrash.wasm"),
    ("triphop", "KO TRIPHOP", "FM-1_56412", "fm_triphop", "triphop-only", "fm_triphop.wasm"),
    ("deck", "KO KIT", "FM-1_56413", "fm_deck", "deck-only", "fm_deck.wasm"),
    ("soul", "KO SOUL", "FM-1_56414", "fm_soul", "soul-only", "fm_soul.wasm"),
    ("jet", "KO JET", "FM-1_56415", "fm_jet", "jet-only", "fm_jet.wasm"),
]


def run_wasm_builds(skip_wasm: bool) -> None:
    if skip_wasm:
        return
    build_sh = EMU_SRC / "build.sh"
    for slug, _label, _prod, _stem, build_arg, wasm_name in KO_APPS:
        wasm_path = ROOT / "build" / "emu" / wasm_name
        if wasm_path.is_file():
            continue
        print(f"[pages] building {wasm_name} ({build_arg}) …", flush=True)
        r = subprocess.run(["sh", str(build_sh), build_arg], cwd=ROOT)
        if r.returncode != 0:
            sys.exit(r.returncode)


def copy_assets() -> None:
    emu_build = ROOT / "build" / "emu"
    if OUT.exists():
        shutil.rmtree(OUT)
    OUT.mkdir(parents=True)
    FW_OUT.mkdir(parents=True)

    for name in ("worklet.js", "fm1ota.js", "fm1pkg.js"):
        shutil.copy2(EMU_SRC / name, OUT / name)

    for _slug, _label, _prod, stem, _arg, wasm_name in KO_APPS:
        src = emu_build / wasm_name
        if not src.is_file():
            print(f"[pages] missing wasm: {src}", file=sys.stderr)
            sys.exit(1)
        shutil.copy2(src, OUT / wasm_name)

    dist = ROOT / "dist"
    for _slug, _label, _prod, stem, _arg, _wasm in KO_APPS:
        pkg = dist / f"{stem}-0.1.fwsc"
        if not pkg.is_file():
            print(f"[pages] missing dist package: {pkg}", file=sys.stderr)
            sys.exit(1)
        shutil.copy2(pkg, FW_OUT / f"{stem}.fwsc")

    inv = {
        "wasm": sorted(p.name for p in OUT.glob("*.wasm")),
        "fw": sorted(p.name for p in FW_OUT.glob("*.fwsc")),
    }
    (OUT / "inventory.json").write_text(json.dumps(inv, separators=(",", ":")), encoding="utf-8")


def flash_packages_js() -> str:
    lines = []
    for slug, label, expect, stem, _arg, _wasm in KO_APPS:
        lines.append(
            f'  {{ id: "{slug}", label: "{label}", path: "fw/{stem}.fwsc", expect: "{expect}", autoBuild: false }},'
        )
    return "const FLASH_PACKAGES = [\n" + "\n".join(lines) + "\n];"


def catalog_html() -> str:
    rows = []
    for slug, label, _exp, stem, _arg, _wasm in KO_APPS:
        wip = ' <span class="wip-badge">WIP</span>' if slug == "jet" else ""
        rows.append(
            f'    <li class="app-row" data-slug="{slug}">'
            f'<span class="app-name">{label}{wip}</span>'
            f'<span class="app-actions">'
            f'<button type="button" class="app-load" data-app="{slug}">Load in Emulator</button>'
            f'<button type="button" class="app-install" data-install="{slug}">Install on FM-1</button>'
            f'<a class="app-dl" href="fw/{stem}.fwsc" download="{stem}-0.1.fwsc">.fwsc</a>'
            f"</span></li>"
        )
    return (
        '  <section class="catalog" id="app-catalog" aria-label="KO firmware catalog">\n'
        "    <h2>KO firmwares for FM-1</h2>\n"
        '    <p class="catalog-hint">Emulator runs in this browser. '
        "Install needs Chrome or Edge (Web MIDI with SysEx). "
        "Or use <code>python3 tools/fm1_install.py dist/&lt;pkg&gt;</code> or "
        '<a href="https://thefirmwarehub.com">thefirmwarehub.com</a>.</p>\n'
        '    <ol class="app-list">\n'
        + "\n".join(rows)
        + "\n    </ol>\n"
        "  </section>\n"
    )


def patch_index(base_href: str) -> str:
    html = (EMU_SRC / "index.html").read_text(encoding="utf-8")

    catalog_css = """
.catalog { margin: 0 0 1.4rem; padding: .85rem 1rem; border: 1px solid var(--rule); border-radius: 12px; background: var(--card); }
.catalog h2 { font: 600 1rem "Barlow Semi Condensed", "Barlow", sans-serif; margin: 0 0 .55rem; letter-spacing: .04em; }
.catalog-hint { margin: 0 0 .75rem; color: var(--muted); font-size: .92rem; max-width: 72ch; }
.app-list { list-style: none; margin: 0; padding: 0; }
.app-row { display: flex; flex-wrap: wrap; align-items: center; gap: .5rem 1rem; padding: .45rem .35rem; border-top: 1px solid var(--rule); border-radius: 8px; margin: .1rem 0; transition: background .12s, box-shadow .12s; }
.app-row:first-child { border-top: 0; }
.app-row.is-active { background: color-mix(in srgb, var(--accent) 14%, var(--card)); box-shadow: inset 3px 0 0 var(--accent); }
.app-name { font: 600 .95rem "Barlow Semi Condensed", "Barlow", sans-serif; min-width: 8rem; }
.app-actions { display: flex; flex-wrap: wrap; gap: .4rem .6rem; align-items: center; }
.app-load, .app-install { font: 600 .82rem "Barlow Semi Condensed", "Barlow", sans-serif; padding: .35em .75em; border-radius: 8px; border: 1px solid var(--rule); background: var(--ground); color: var(--ink); cursor: pointer; }
.app-load:hover, .app-install:hover { border-color: var(--accent); }
.app-dl { font-size: .82rem; color: var(--muted); }
.wip-badge { font-size: .72rem; font-weight: 600; letter-spacing: .06em; text-transform: uppercase; color: var(--accent); border: 1px solid var(--accent); border-radius: 6px; padding: .1em .45em; vertical-align: middle; margin-left: .35em; }
body.pages-site .fw-row:has(#emu-pick) { display: none; }
body.pages-site .fw-row:has(#hw-flash) { display: none; }
"""
    html = html.replace("</style>", catalog_css + "\n</style>", 1)

    base_tag = f'<base href="{base_href}">\n'
    html = html.replace("<head>\n", f"<head>\n{base_tag}", 1)

    html = html.replace(
        "<title>TRIPTYCH — M-VAVE FM-1 in the browser</title>",
        "<title>KO firmwares — M-VAVE FM-1 in the browser</title>",
    )
    html = html.replace("<body>", '<body class="pages-site">', 1)
    html = re.sub(
        r"<header>.*?</header>",
        lambda m: m.group(0) + "\n" + catalog_html(),
        html,
        count=1,
        flags=re.DOTALL,
    )

    for slug in ("rytm", "ctrl", "triptych", "x0x"):
        html = re.sub(
            rf'\s*<button type="button" data-emu="{slug}"[^>]*>.*?</button>\s*',
            "",
            html,
        )

    html = re.sub(
        r"const FLASH_PACKAGES = \[[\s\S]*?\];",
        flash_packages_js(),
        html,
        count=1,
    )

    html = html.replace(
        'const qp = new URLSearchParams(location.search).get("emu");',
        'const qs = new URLSearchParams(location.search);\n'
        '    const qp = qs.get("app") || qs.get("emu");',
    )
    html = html.replace(
        'return EMU_PROFILES[s] ? s : "triptych";',
        'return EMU_PROFILES[s] ? s : "jazz";',
    )
    html = html.replace('{ return "triptych"; }', '{ return "jazz"; }')

    pages_boot = """
<script>
(function pagesCatalog() {
  const catalog = document.getElementById("app-catalog");
  if (!catalog) return;
  function syncCatalogSelection() {
    catalog.querySelectorAll(".app-row").forEach((row) => {
      row.classList.toggle("is-active", row.dataset.slug === emuProfile);
    });
  }
  function pickApp(slug) {
    if (!slug || !EMU_PROFILES[slug]) return;
    if (node && emuProfile !== slug) {
      if (!confirm("Switch emulator? This reloads the page and stops audio.")) return;
      emuProfile = slug;
      try { localStorage.setItem(EMU_LS, emuProfile); } catch {}
      const url = new URL(location.href);
      url.searchParams.set("app", slug);
      history.replaceState(null, "", url);
      location.reload();
      return;
    }
    emuProfile = slug;
    try { localStorage.setItem(EMU_LS, emuProfile); } catch {}
    const url = new URL(location.href);
    url.searchParams.set("app", slug);
    history.replaceState(null, "", url);
    applyEmuProfileUI();
    syncCatalogSelection();
    document.getElementById("device").scrollIntoView({ behavior: "smooth" });
  }
  catalog.querySelectorAll(".app-load").forEach((btn) => {
    btn.addEventListener("click", () => pickApp(btn.dataset.app));
  });
  syncCatalogSelection();
  const _apply = applyEmuProfileUI;
  applyEmuProfileUI = function () { _apply(); syncCatalogSelection(); };
})();
</script>
"""
    pages_install = """
<script type="module">
document.querySelectorAll(".app-install").forEach((btn) => {
  btn.addEventListener("click", () => {
    const flashBtn = document.querySelector('#hw-flash button[data-pkg="' + btn.dataset.install + '"]');
    if (flashBtn && !flashBtn.disabled) flashBtn.click();
    else alert("Package not ready — try downloading the .fwsc link.");
  });
});
</script>
"""
    html = html.replace("</body>", pages_boot + pages_install + "\n</body>", 1)

    return html


def normalize_base_href(raw: str) -> str:
    base = raw.strip() or DEFAULT_BASE_HREF
    if not base.startswith("/"):
        base = "/" + base
    if not base.endswith("/"):
        base += "/"
    return base


def build_pages(skip_wasm: bool, base_href: str) -> None:
    run_wasm_builds(skip_wasm)
    copy_assets()
    base = normalize_base_href(base_href)
    (OUT / "index.html").write_text(patch_index(base), encoding="utf-8")
    (OUT / ".nojekyll").write_text("", encoding="utf-8")
    total = sum(f.stat().st_size for f in OUT.rglob("*") if f.is_file())
    print(f"[pages] wrote {OUT} ({total} bytes, {len(list(OUT.rglob('*')))} files)")


def publish_to_github() -> None:
    if not OUT.is_dir():
        print("[pages] build output missing — run build first", file=sys.stderr)
        sys.exit(1)
    with tempfile.TemporaryDirectory(prefix="ko-fm1-publish-") as tmp:
        checkout = Path(tmp) / "ko-fm1"
        print(f"[pages] cloning {PUBLISH_REMOTE} …", flush=True)
        r = subprocess.run(
            ["git", "clone", "--depth", "1", "--branch", PUBLISH_BRANCH, PUBLISH_REMOTE, str(checkout)],
        )
        if r.returncode != 0:
            print("[pages] no main branch yet, starting a fresh history", flush=True)
            checkout.mkdir(parents=True, exist_ok=True)
            subprocess.run(["git", "init", "-b", PUBLISH_BRANCH], cwd=checkout, check=True)
            subprocess.run(["git", "remote", "add", "origin", PUBLISH_REMOTE], cwd=checkout, check=True)
        for child in checkout.iterdir():
            if child.name == ".git":
                continue
            if child.is_dir():
                shutil.rmtree(child)
            else:
                child.unlink()
        for item in OUT.iterdir():
            dest = checkout / item.name
            if item.is_dir():
                shutil.copytree(item, dest)
            else:
                shutil.copy2(item, dest)
        subprocess.run(["git", "add", "-A", "--force"], cwd=checkout, check=True)
        subprocess.run(
            ["git", "commit", "-m", "chore(pages): update static site"],
            cwd=checkout,
            check=True,
        )
        subprocess.run(["git", "push", "origin", PUBLISH_BRANCH], cwd=checkout, check=True)
        print("[pages] pushed to", PUBLISH_REMOTE, PUBLISH_BRANCH, flush=True)


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Build static site for https://kosmar.github.io/ko-fm1/",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=(
            "Publish (--publish): build, clone kosmar/ko-fm1, replace tree, commit, push main.\n"
            "Requires SSH access to git@github.com:kosmar/ko-fm1.git"
        ),
    )
    ap.add_argument("--skip-wasm", action="store_true", help="reuse existing build/emu/*.wasm")
    ap.add_argument(
        "--base-href",
        default=DEFAULT_BASE_HREF,
        metavar="PATH",
        help=f"Pages URL path prefix (default: {DEFAULT_BASE_HREF})",
    )
    ap.add_argument(
        "--publish",
        action="store_true",
        help=f"after build, push {OUT} to {PUBLISH_REMOTE} ({PUBLISH_BRANCH})",
    )
    args = ap.parse_args()
    build_pages(args.skip_wasm, args.base_href)
    if args.publish:
        publish_to_github()


if __name__ == "__main__":
    main()