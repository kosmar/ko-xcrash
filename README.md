<!-- publish-github-snapshot -->
**Source snapshot of KO XCRASH** — one firmware in the KO family for the M-VAVE FM-1 (package identity `FM-1_56411`). Issues and questions welcome in this repository.

**Other KO firmwares:**
- [KO BASS](https://github.com/kosmar/ko-bass)
- [KO KIT](https://github.com/kosmar/ko-kit)
- [KO DUB](https://github.com/kosmar/ko-dub)
- [KO FUNK](https://github.com/kosmar/ko-funk)
- [KO HVYPSY](https://github.com/kosmar/ko-hvypsy)
- [KO HIPHOP](https://github.com/kosmar/ko-hiphop)
- [KO HOUSE](https://github.com/kosmar/ko-house)
- [KO IDM](https://github.com/kosmar/ko-idm)
- [KO JAZZ](https://github.com/kosmar/ko-jazz)
- [KO JET](https://github.com/kosmar/ko-jet)
- [KO POP](https://github.com/kosmar/ko-pop)
- [KO XCRASH](https://github.com/kosmar/ko-xcrash)
- [KO SOUL](https://github.com/kosmar/ko-soul)
- [KO TECHNO](https://github.com/kosmar/ko-techno)
- [KO TRIPHOP](https://github.com/kosmar/ko-triphop)

**Try in the browser:** [https://kosmar.github.io/ko-fm1/?app=snowcrash](https://kosmar.github.io/ko-fm1/?app=snowcrash)

---

<p align="center"><img src="assets/logo.svg" width="96" alt="Dinghy"></p>

# KO firmwares for the M-VAVE FM-1

One repository, many firmwares, one platform. They share the installer, the USB rescue, USB-MIDI, USB audio, the keys, knobs and screen. What you install is one instrument: a genre, KO BASS, or KO KIT.

The platform is [Dinghy](https://github.com/zvenson/dinghy), which keeps [Felucca](https://github.com/hugelton/Felucca) (Leo Kuroshita, Hügelton Instruments) the way [SLOOP](https://github.com/isod89/sloop-fm1) carries it. The apps on top are the KO firmwares.

## How it fits together

- **Platform.** Boot, USB update, rescue, MIDI, audio, the panel, the screen, settings. The same code under every app. Read it in [docs/API.md](docs/API.md). The chip’s rules are in [docs/RULES.md](docs/RULES.md).
- **Genre plates.** DUB, HOUSE, HVYPSY, JAZZ, IDM, HIPHOP, POP, FUNK, TECHNO, TRIPHOP, SOUL and XCRASH are one engine with a different plate pack. Each is still its own firmware, with its own package identity, so the installer can tell them apart.
- **KO BASS.** That same engine, narrowed to bass lines.
- **KO KIT.** Its own kit and sequencer firmware, not a genre pack.
- **KO JET.** Its own engine. **WIP:** sequencer and recording are new and tested in the simulator and emulator only, not yet on hardware. Expect rough edges. Install with `python3 tools/fm1_install.py dist/fm_jet-0.1.fwsc`.
- **Also in the tree, not in this drop.** TRIPTYCH, KO RYTM and KO CTRL. Build them with the same `./build.sh --app` switch. They are not in `dist/`.

A package identity looks like `FM-1_56404`. It stays fixed for that firmware. The version, 0.1, is the filename and the version string, not a second identity. Two firmwares with the same identity look like one.

## Install 0.1

The `.fwsc` files in [`dist/`](dist/) are ready to install. From a clone of this repo, with the FM-1 plugged in:

```sh
python3 tools/fm1_install.py dist/fm_jazz-0.1.fwsc
python3 tools/fm1_install.py --info
```

The installer asks before it writes, then the FM-1 restarts into that firmware. Swap `fm_jazz-0.1.fwsc` for any other file in the table.

If it will not start, hold **OCT-** while switching on. That is the USB rescue: no sound, only the updater. Install another package from there. Twice crashing at boot goes there on its own. Installing firmware is at your own risk.

These 0.1 files were built with a fixed identity per app. They use Dinghy’s updater. They are not a fresh device pass of every genre.

| Firmware | Identity | Package |
| --- | --- | --- |
| KO DUB | FM-1_56401 | [fm_dub-0.1.fwsc](dist/fm_dub-0.1.fwsc) |
| KO HOUSE | FM-1_56402 | [fm_house-0.1.fwsc](dist/fm_house-0.1.fwsc) |
| KO HVYPSY | FM-1_56403 | [fm_hvypsy-0.1.fwsc](dist/fm_hvypsy-0.1.fwsc) |
| KO JAZZ | FM-1_56404 | [fm_jazz-0.1.fwsc](dist/fm_jazz-0.1.fwsc) |
| KO IDM | FM-1_56405 | [fm_idm-0.1.fwsc](dist/fm_idm-0.1.fwsc) |
| KO HIPHOP | FM-1_56406 | [fm_hiphop-0.1.fwsc](dist/fm_hiphop-0.1.fwsc) |
| KO POP | FM-1_56407 | [fm_pop-0.1.fwsc](dist/fm_pop-0.1.fwsc) |
| KO FUNK | FM-1_56408 | [fm_funk-0.1.fwsc](dist/fm_funk-0.1.fwsc) |
| KO TECHNO | FM-1_56409 | [fm_techno-0.1.fwsc](dist/fm_techno-0.1.fwsc) |
| KO BASS | FM-1_56410 | [fm_bass-0.1.fwsc](dist/fm_bass-0.1.fwsc) |
| KO XCRASH | FM-1_56411 | [fm_snowcrash-0.1.fwsc](dist/fm_snowcrash-0.1.fwsc) |
| KO TRIPHOP | FM-1_56412 | [fm_triphop-0.1.fwsc](dist/fm_triphop-0.1.fwsc) |
| KO KIT | FM-1_56413 | [fm_deck-0.1.fwsc](dist/fm_deck-0.1.fwsc) |
| KO SOUL | FM-1_56414 | [fm_soul-0.1.fwsc](dist/fm_soul-0.1.fwsc) |
| KO JET | FM-1_56415 | WIP — [fm_jet-0.1.fwsc](dist/fm_jet-0.1.fwsc) |

## Build one from source

Linux on x86-64, or macOS with Docker. Python 3 with Pillow, the JieLi toolchain, and the AC79 SDK:

```sh
pip3 install Pillow mido python-rtmidi
tools/get_toolchain.sh
git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 \
    https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK
```

Then, for any app in the table (`jazz`, `bass`, `deck`, …):

```sh
./build.sh --app jazz
python3 tools/fm1_install.py build/fm_jazz.fwsc
```

`./build.sh` with no `--app` builds TRIPTYCH. Slugs: `triptych`, `rytm`, `ctrl`, `dub`, `house`, `heavypsych`, `jazz`, `idm`, `hiphop`, `snowcrash`, `pop`, `funk`, `techno`, `triphop`, `soul`, `bass`, `deck`, `jet`.

Host mode runs the same firmware on your computer, as a WAV and screen images, with no FM-1:

```sh
tests/run_host.sh
```

The sound is written in `firmware/src/`. For TRIPTYCH that file is `dinghy.c`. Genre apps share `fm_app.c`. [docs/API.md](docs/API.md) lists the calls. [docs/MAKE-IT-YOURS.md](docs/MAKE-IT-YOURS.md) is how a copy becomes its own firmware and how the [FM-1 Firmware Hub](https://thefirmwarehub.com/) tells packages apart.

The build refuses what would brick an FM-1: a 64-bit division, a call into the chip ROM, too much RAM, a register touched outside `hal/`. [docs/RULES.md](docs/RULES.md) says why.
