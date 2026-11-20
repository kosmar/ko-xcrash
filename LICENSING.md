# Licensing

Dinghy is free software under the GNU General Public License, version 3 only (`GPL-3.0-only`, full text in
`LICENSE`). You may use, study, change and share it. If you give someone a firmware built from it, give them its
complete source under the same licence too.

## Who made what

| | |
| --- | --- |
| The platform: `firmware/hal/`, `firmware/loader/`, `crt0.S`, `app.ld`, `firmware/src/` (`usb.c`, `ota.c`, `recovery.c`, `lcd.c`, `gfx.c`, `libc.c`, `bootguard.h`, `uac_fir.h`), `tools/` | Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments ([Felucca](https://github.com/hugelton/Felucca)), with the changes of [SLOOP](https://github.com/isod89/sloop-fm1) (isod89) |
| The app `firmware/src/dinghy.c`, the wiring `platform.c`, the host mode `tests/`, the upstream Dinghy docs | zvenson ([Dinghy](https://github.com/zvenson/dinghy)) |
| KO genre apps: `firmware/src/fm_app.c`, genre `*_conf.h`, shared plate data | kosmar |
| KO KIT: `firmware/src/deck.c` and related headers | kosmar |
| KO JET: `firmware/src/jet.c` and related headers | kosmar |
| KO RYTM: `firmware/src/rytm.c` | kosmar |
| KO CTRL: `firmware/src/ctrl.c` | kosmar |
| Plate pack and view generators: `tools/gen_*.py` | kosmar |
| Browser emulator: `web/emu/` | kosmar |

## Third-party material

| What | Licence | Where |
| --- | --- | --- |
| Terminus font 8x16 (ter-u16n), rendered into `firmware/gen/felucca_font.h` | SIL OFL 1.1 | `assets/fonts/Terminus-LICENSE.txt` |
| JieLi AC79 SDK (default Dinghy build): `uboot.boot`, `cfg_tool.bin`, `cfg/eq_cfg_hw.bin` and related blobs are read from your SDK checkout at build time and placed in the package; they are not committed here | Apache-2.0 | <https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK> |

## The logo

`assets/logo.svg` / `logo.png` carry the designburg signet: Copyright (C) designburg, all rights reserved, not under
the GPL. It marks Dinghy itself; a fork uses its own logo.

## Trademarks

"Felucca" and "Hügelton Instruments" are names of Hügelton Instruments; "M-VAVE" and "FM-1" belong to their
owners. Dinghy is not made or endorsed by them.

KO VOICES (first apps: KO KIT, KO BASS): KO KIT ships a synthesised drum kit only — no third-party drum
samples and no Linn (or other) sample ROMs.
