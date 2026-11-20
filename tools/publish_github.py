#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Publish one KO FM-1 app as a full-source snapshot to its public GitHub repo."""
from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
APPS_DIR = ROOT / "publish" / "apps"
CARDS_DIR = ROOT / "publish" / "assets" / "cards"
GITHUB_OWNER = "kosmar"
EMU_HUB = "https://kosmar.github.io/ko-fm1"


def run(cmd: list[str], *, cwd: Path | None = None, check: bool = True) -> subprocess.CompletedProcess:
    return subprocess.run(cmd, cwd=cwd or ROOT, check=check, text=True, capture_output=True)


def load_apps(slug: str | None) -> list[dict]:
    if slug:
        path = APPS_DIR / f"{slug}.json"
        if not path.is_file():
            sys.exit(f"unknown app {slug!r} (missing {path})")
        return [json.loads(path.read_text(encoding="utf-8"))]
    apps = []
    for path in sorted(APPS_DIR.glob("*.json")):
        apps.append(json.loads(path.read_text(encoding="utf-8")))
    if not apps:
        sys.exit(f"no apps under {APPS_DIR}")
    return apps


def git_head() -> str:
    r = run(["git", "rev-parse", "HEAD"])
    return r.stdout.strip()


def archive_snapshot(dest: Path) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    proc = subprocess.Popen(
        ["git", "archive", "--format=tar", "HEAD"],
        cwd=ROOT,
        stdout=subprocess.PIPE,
    )
    assert proc.stdout is not None
    unarchive = subprocess.Popen(
        ["tar", "-xf", "-", "-C", str(dest)],
        stdin=proc.stdout,
        cwd=ROOT,
    )
    proc.stdout.close()
    rc = unarchive.wait()
    proc.wait()
    if rc != 0 or proc.returncode != 0:
        sys.exit("git archive failed")


def ko_firmware_index() -> list[dict]:
    out = []
    for path in sorted(APPS_DIR.glob("*.json")):
        out.append(json.loads(path.read_text(encoding="utf-8")))
    return out


def readme_head(meta: dict) -> str:
    slug = meta["slug"]
    wip = (
        "\n\n> **Work in progress:** tested in the browser emulator only—not yet on hardware. Expect breaking changes.\n"
        if meta.get("wip")
        else ""
    )
    family = []
    for m in ko_firmware_index():
        url = f"https://github.com/{GITHUB_OWNER}/{m['github_repo']}"
        family.append(f"- [{m['display_name']}]({url})")
    family_md = "\n".join(family)
    emu = f"{EMU_HUB}/?app={slug}"
    return (
        f"**Source snapshot of {meta['display_name']}** — one firmware in the KO family for the "
        f"M-VAVE FM-1 (package identity `{meta['identity']}`). Issues and questions welcome in this repository."
        f"{wip}\n\n"
        f"**Other KO firmwares:**\n{family_md}\n\n"
        f"**Try in the browser:** [{EMU_HUB}/?app={slug}]({emu})\n\n---\n\n"
    )


def patch_readme(snap: Path, meta: dict) -> None:
    readme = snap / "README.md"
    body = readme.read_text(encoding="utf-8") if readme.is_file() else ""
    head = readme_head(meta)
    marker = "<!-- publish-github-snapshot -->"
    if marker in body:
        rest = body.split(marker, 1)[1].split("\n", 1)[-1]
        if rest.startswith("---\n\n"):
            rest = rest[4:]
        readme.write_text(f"{marker}\n{head}{rest}", encoding="utf-8")
        return
    if body.startswith("**Source snapshot of") or body.startswith("**Snapshot repo for"):
        parts = body.split("---\n\n", 1)
        tail = parts[1] if len(parts) == 2 else body
        readme.write_text(f"{marker}\n{head}{tail}", encoding="utf-8")
        return
    readme.write_text(f"{marker}\n{head}{body}", encoding="utf-8")


def trim_dist(snap: Path, dist_name: str) -> None:
    dist = snap / "dist"
    if not dist.is_dir():
        raise FileNotFoundError(f"{dist}: missing")
    keep = dist / dist_name
    if not keep.is_file():
        raise FileNotFoundError(f"package missing: {keep}")
    for f in list(dist.iterdir()):
        if f.name != dist_name:
            f.unlink()


def write_fmhub(snap: Path, meta: dict) -> None:
    (snap / "fmhub.json").write_text(
        json.dumps(meta["fmhub"], indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )


def write_card(snap: Path, slug: str) -> None:
    src = CARDS_DIR / f"{slug}.png"
    if not src.is_file():
        src = ROOT / "docs" / "card.png"
    dst_dir = snap / "docs"
    dst_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst_dir / "card.png")


def remove_publish_tree(snap: Path) -> None:
    pub = snap / "publish"
    if pub.exists():
        shutil.rmtree(pub)


def build_snapshot(meta: dict, out_parent: Path) -> Path:
    slug = meta["slug"]
    snap = out_parent / f"github-snapshot-{slug}"
    if snap.exists():
        shutil.rmtree(snap)
    archive_snapshot(snap)
    remove_publish_tree(snap)
    write_fmhub(snap, meta)
    write_card(snap, slug)
    patch_readme(snap, meta)
    trim_dist(snap, meta["dist_file"])
    return snap


def push_snapshot(meta: dict, snap: Path, tag: str | None, release: bool) -> None:
    repo = meta["github_repo"]
    remote = f"git@github.com:{GITHUB_OWNER}/{repo}.git"
    with tempfile.TemporaryDirectory(prefix="ghpub-") as td:
        work = Path(td) / "repo"
        init = run(["git", "clone", remote, str(work)], check=False)
        if init.returncode != 0:
            run(["git", "init", "-b", "main"], cwd=work)
            run(["git", "remote", "add", "origin", remote], cwd=work)
        else:
            run(["git", "checkout", "main"], cwd=work, check=False)
        for item in work.iterdir():
            if item.name == ".git":
                continue
            if item.is_dir():
                shutil.rmtree(item)
            else:
                item.unlink()
        for item in snap.iterdir():
            dest = work / item.name
            if item.is_dir():
                shutil.copytree(item, dest, symlinks=True)
            else:
                shutil.copy2(item, dest)
        run(["git", "add", "-A", "--force"], cwd=work)
        msg = f"chore: publish {meta['display_name']} snapshot ({git_head()[:12]})"
        status = run(["git", "status", "--porcelain"], cwd=work)
        if not status.stdout.strip():
            print(f"{repo}: nothing to commit", file=sys.stderr)
            return
        run(["git", "commit", "-m", msg], cwd=work)
        run(["git", "push", "-u", "origin", "main"], cwd=work)
        if tag:
            run(["git", "tag", tag], cwd=work, check=False)
            run(["git", "push", "origin", tag], cwd=work)
            if release:
                fwsc = work / "dist" / meta["dist_file"]
                prerelease = tag.startswith("v0.") or "alpha" in tag or "beta" in tag or "rc" in tag
                args = [
                    "gh",
                    "release",
                    "create",
                    tag,
                    str(fwsc),
                    "--repo",
                    f"{GITHUB_OWNER}/{repo}",
                    "--title",
                    tag,
                ]
                if prerelease:
                    args.append("--prerelease")
                run(args, cwd=work)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument("--app", metavar="SLUG")
    g.add_argument("--all", action="store_true")
    ap.add_argument("--dry-run", action="store_true", help="build snapshot dir only")
    ap.add_argument("--no-push", action="store_true", help="build but do not push")
    ap.add_argument("--tag", metavar="vX.Y.Z")
    ap.add_argument("--release", action="store_true", help="with --tag: gh release upload .fwsc")
    ap.add_argument(
        "--out-dir",
        type=Path,
        default=ROOT / "build" / "publish",
        help="parent directory for snapshots",
    )
    args = ap.parse_args()
    if args.release and not args.tag:
        ap.error("--release needs --tag")
    apps = load_apps(args.app if args.app else None)
    args.out_dir.mkdir(parents=True, exist_ok=True)
    for meta in apps:
        snap = build_snapshot(meta, args.out_dir)
        print(snap)
        if args.dry_run or args.no_push:
            continue
        push_snapshot(meta, snap, args.tag, args.release)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
