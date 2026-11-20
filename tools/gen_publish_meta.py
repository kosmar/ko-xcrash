#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Write publish/apps/<slug>.json from built-in catalogue (run once / when copy changes)."""
from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "publish" / "apps"


CARD_COLOR = "#657a76"
EMU_BASE = "https://kosmar.github.io/ko-fm1/?app="


APPS = [
    dict(
        slug="dub",
        github_repo="ko-dub",
        display_name="KO DUB",
        identity="FM-1_56401",
        dist_file="fm_dub-0.1.fwsc",
        github_description="KO DUB — reggae and dub groovebox firmware for the M-VAVE FM-1 (Dinghy, GPL-3.0).",
        kinds=["groove"],
        what="KO DUB is a Felucca-style groovebox for the FM-1: six dub eras from Kingston to post-dub, SKANK presets, delay-heavy plates, and live space for the one-drop. Built on the shared KO genre engine with its own package identity.",
    ),
    dict(
        slug="house",
        github_repo="ko-house",
        display_name="KO HOUSE",
        identity="FM-1_56402",
        dist_file="fm_house-0.1.fwsc",
        github_description="KO HOUSE — house and club groovebox firmware for the M-VAVE FM-1.",
        kinds=["groove"],
        what="KO HOUSE drives jack, garage, acid, deep, filter, and tech plates on the KO engine: 60 TRAX, DJ fills and flams paced for club density, MIDI and USB audio like every Dinghy app.",
    ),
    dict(
        slug="heavypsych",
        github_repo="ko-hvypsy",
        display_name="KO HVYPSY",
        identity="FM-1_56403",
        dist_file="fm_hvypsy-0.1.fwsc",
        github_description="KO HVYPSY — heavy psych and doom-leaning groovebox firmware for the FM-1.",
        kinds=["groove"],
        what="KO HVYPSY trades club fills for stoner, desert, space, drone, gaze, and doom eras—slow macros, lead-focused plates, and jazz-like sparse auto-DJ pacing on the shared KO genre engine.",
    ),
    dict(
        slug="jazz",
        github_repo="ko-jazz",
        display_name="KO JAZZ",
        identity="FM-1_56404",
        dist_file="fm_jazz-0.1.fwsc",
        github_description="KO JAZZ — jazz-oriented groovebox firmware for the M-VAVE FM-1.",
        kinds=["groove"],
        what="KO JAZZ spans Spirit, Modal, Mute, Fusion, Acid, and Broken eras with 60 TUNE presets, walking bass degrees, and a deliberately sparse auto-DJ—fills and loops stay rare so the combo can breathe.",
    ),
    dict(
        slug="idm",
        github_repo="ko-idm",
        display_name="KO IDM",
        identity="FM-1_56405",
        dist_file="fm_idm-0.1.fwsc",
        github_description="KO IDM — IDM and experimental groove firmware for the FM-1.",
        kinds=["groove"],
        what="KO IDM maps Warm, Melo, Glitch, Break, Drill, and Obliq piece banks—irregular grids, glitch-friendly plates, and the same KO performance surface with USB-MIDI and the Dinghy updater.",
    ),
    dict(
        slug="hiphop",
        github_repo="ko-hiphop",
        display_name="KO HIPHOP",
        identity="FM-1_56406",
        dist_file="fm_hiphop-0.1.fwsc",
        github_description="KO HIPHOP — hip-hop and beats groovebox firmware for the FM-1.",
        kinds=["groove"],
        what="KO HIPHOP covers Park through Cloud beat eras: pocket drums, sample-styled lanes, 60 presets, and club-paced DJ gestures on the shared KO engine for the FM-1 panel.",
    ),
    dict(
        slug="pop",
        github_repo="ko-pop",
        display_name="KO POP",
        identity="FM-1_56407",
        dist_file="fm_pop-0.1.fwsc",
        github_description="KO POP — pop and synth-pop groovebox firmware for the FM-1.",
        kinds=["groove"],
        what="KO POP runs Cold, Radio, Chrome, Italo, Blue, and After plate lines—bright leads, structured drops, and the KO auto-DJ with pop-specific lead verification hooks in the same Dinghy stack.",
    ),
    dict(
        slug="funk",
        github_repo="ko-funk",
        display_name="KO FUNK",
        identity="FM-1_56408",
        dist_file="fm_funk-0.1.fwsc",
        github_description="KO FUNK — funk and boogie groovebox firmware for the FM-1.",
        kinds=["groove"],
        what="KO FUNK moves from Pocket and Clav through Slap, Disco, Boogie, and Broken—syncopated plates, clav-capable macros, and 60 presets on the shared genre groove engine.",
    ),
    dict(
        slug="techno",
        github_repo="ko-techno",
        display_name="KO TECHNO",
        identity="FM-1_56409",
        dist_file="fm_techno-0.1.fwsc",
        github_description="KO TECHNO — techno and EBM groovebox firmware for the FM-1.",
        kinds=["groove"],
        what="KO TECHNO switches Detroit, Belgium, Schranz, Hypno, Berlin, and EBM packs—four-on-the-floor bias, hypnotic plate lines, and KO DJ builds tailored for long transitions.",
    ),
    dict(
        slug="bass",
        github_repo="ko-bass",
        display_name="KO BASS",
        identity="FM-1_56410",
        dist_file="fm_bass-0.1.fwsc",
        github_description="KO BASS — bass-synth groove firmware for the M-VAVE FM-1.",
        kinds=["synth"],
        what="KO BASS is the KO engine narrowed to bass lines: Acid, Sub, Reese, FM, Fold, and Sync eras, LINE presets with user plates, tempo and clock tools—still a full FM-1 firmware with its own identity.",
    ),
    dict(
        slug="snowcrash",
        github_repo="ko-xcrash",
        display_name="KO XCRASH",
        identity="FM-1_56411",
        dist_file="fm_snowcrash-0.1.fwsc",
        github_description="KO XCRASH — breakbeat/crash-themed groove firmware for the FM-1.",
        kinds=["groove"],
        what="KO XCRASH (Snow Crash pack) cycles Snow, Gloss, Sum, Ice, Tide, and Warp crash slabs—break-leaning plates and club DJ density on the shared KO genre engine.",
    ),
    dict(
        slug="triphop",
        github_repo="ko-triphop",
        display_name="KO TRIPHOP",
        identity="FM-1_56412",
        dist_file="fm_triphop-0.1.fwsc",
        github_description="KO TRIPHOP — trip-hop and downtempo groove firmware for the FM-1.",
        kinds=["groove"],
        what="KO TRIPHOP drifts through Dust, Vinyl, Smoke, Strings, Dark, and Cinema eras—slower plates, vinyl-styled pockets, and the standard KO panel, sequencer, and Dinghy rescue path.",
    ),
    dict(
        slug="deck",
        github_repo="ko-kit",
        display_name="KO KIT",
        identity="FM-1_56413",
        dist_file="fm_deck-0.1.fwsc",
        github_description="KO KIT — drum kit and sequencer firmware for the M-VAVE FM-1.",
        kinds=["drums"],
        what="KO KIT is not a genre pack: sixteen drum voices, eleven performance pads, groove plates across kit eras, its own sequencer and store—standalone Dinghy firmware beside the KO line.",
    ),
    dict(
        slug="soul",
        github_repo="ko-soul",
        display_name="KO SOUL",
        identity="FM-1_56414",
        dist_file="fm_soul-0.1.fwsc",
        github_description="KO SOUL — soul and R&B groovebox firmware for the FM-1.",
        kinds=["groove"],
        what="KO SOUL voices Church, Detroit, Memphis, Northern, Philly, and Street trax—warm harmony lanes, REVUE-style ensemble FX labels, and 60 presets on the shared KO engine.",
    ),
    dict(
        slug="jet",
        github_repo="ko-jet",
        display_name="KO JET",
        identity="FM-1_56415",
        dist_file="fm_jet-0.1.fwsc",
        github_description="KO JET — experimental groove/sequencer firmware for the FM-1 (WIP).",
        kinds=["groove"],
        wip=True,
        what="KO JET is a separate engine with its own banks, pseudo presets, and a new sequencer—still rough; tested in the host emulator only, not yet vetted on hardware. Expect breaking changes.",
    ),
]

def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    for a in APPS:
        fmhub = {
            "what": a["what"],
            "author": "kosmar",
            "kinds": a["kinds"],
            "play": EMU_BASE + a["slug"],
            "image": "docs/card.png",
            "color": CARD_COLOR,
        }
        if a["slug"] == "jet":
            fmhub["what"] = a["what"]  # already mentions WIP
        doc = {
            "slug": a["slug"],
            "github_repo": a["github_repo"],
            "display_name": a["display_name"],
            "identity": a["identity"],
            "dist_file": a["dist_file"],
            "github_description": a["github_description"],
            "card_source": a.get("card_source", "emulator"),
            "fmhub": fmhub,
        }
        if a.get("wip"):
            doc["wip"] = True
        path = OUT / f"{a['slug']}.json"
        path.write_text(json.dumps(doc, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        assert len(a["what"]) <= 300, a["slug"]
    print(f"wrote {len(APPS)} files under {OUT}")


if __name__ == "__main__":
    main()
