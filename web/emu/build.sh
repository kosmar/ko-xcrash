#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/../.."
ROOT="$PWD"
mkdir -p build/emu build/emu/fw

APP_ARG="${1:-}"
# Single-app wasm builds must not depend on unrelated DJ host smoke tests.
case "$APP_ARG" in
  "" ) sh "$ROOT/tests/check_dj_wired.sh" ;;
  *-only ) ;;
  * ) sh "$ROOT/tests/check_dj_wired.sh" ;;
esac

should_build() {
  if [ -z "$APP_ARG" ]; then return 0; fi
  [ "$APP_ARG" = "${1}-only" ] || [ "$APP_ARG" = "${1}" ]
}

CLANG="/nix/store/rr64nnycczvx7s1b110qmvqrlfcb6lsm-clang-21.1.8/bin/clang"
WASM_LD="/Users/kosmar/.rustup/toolchains/stable-aarch64-apple-darwin/lib/rustlib/aarch64-apple-darwin/bin/gcc-ld/wasm-ld"

if [ ! -x "$CLANG" ]; then
  CLANG=clang
fi
if [ ! -x "$WASM_LD" ]; then
  WASM_LD=wasm-ld
fi

WASM_CFLAGS="--target=wasm32-unknown-unknown -O2 -ffreestanding -nostdlib -Wall -Wno-unused-function \
  -DDINGHY_WEB -Ifirmware/src -Ifirmware/gen"
WASM_LD_FLAGS="--no-entry --export-dynamic --allow-undefined -z stack-size=262144"

build_wasm() {
  define="$1"
  obj="$2"
  out="$3"
  if [ -n "$define" ]; then
    "$CLANG" $WASM_CFLAGS -D"$define" -c web/emu/emu_web.c -o "$obj"
  else
    "$CLANG" $WASM_CFLAGS -c web/emu/emu_web.c -o "$obj"
  fi
  "$WASM_LD" $WASM_LD_FLAGS "$obj" -o "$out"
}

# Triptych (dinghy) — default browser build
if should_build triptych; then
  build_wasm "" build/emu/emu_web.o build/emu/triptych.wasm
  cp build/emu/triptych.wasm build/emu/kosmar_fm1.wasm
  echo "built $ROOT/build/emu/triptych.wasm (+ kosmar_fm1.wasm alias)"
fi

if should_build rytm; then
  build_wasm EMU_APP_RYTM build/emu/emu_web_rytm.o build/emu/fm_rytm.wasm
  echo "built $ROOT/build/emu/fm_rytm.wasm"
fi

if should_build ctrl; then
  build_wasm EMU_APP_CTRL build/emu/emu_web_ctrl.o build/emu/fm_ctrl.wasm
  echo "built $ROOT/build/emu/fm_ctrl.wasm"
fi

if should_build dub; then
  build_wasm EMU_APP_DUB build/emu/emu_web_dub.o build/emu/fm_dub.wasm
  echo "built $ROOT/build/emu/fm_dub.wasm"
fi

if should_build house; then
  build_wasm EMU_APP_HOUSE build/emu/emu_web_house.o build/emu/fm_house.wasm
  echo "built $ROOT/build/emu/fm_house.wasm"
fi

if should_build heavypsych; then
  build_wasm EMU_APP_HEAVY_PSYCH build/emu/emu_web_heavypsych.o build/emu/fm_heavypsych.wasm
  echo "built $ROOT/build/emu/fm_heavypsych.wasm"
fi

if should_build jazz; then
  if [ ! -f firmware/src/jazz_data.h ]; then
    echo "build.sh: jazz-only requested but firmware/src/jazz_data.h is missing" >&2
    exit 1
  fi
  build_wasm EMU_APP_JAZZ build/emu/emu_web_jazz.o build/emu/fm_jazz.wasm
  echo "built $ROOT/build/emu/fm_jazz.wasm"
fi

if should_build soul; then
  if [ ! -f firmware/src/soul_data.h ]; then
    echo "build.sh: soul-only requested but firmware/src/soul_data.h is missing" >&2
    exit 1
  fi
  build_wasm EMU_APP_SOUL build/emu/emu_web_soul.o build/emu/fm_soul.wasm
  echo "built $ROOT/build/emu/fm_soul.wasm"
fi

if should_build idm; then
  if [ ! -f firmware/src/idm_data.h ]; then
    echo "build.sh: idm-only requested but firmware/src/idm_data.h is missing" >&2
    exit 1
  fi
  build_wasm EMU_APP_IDM build/emu/emu_web_idm.o build/emu/fm_idm.wasm
  echo "built $ROOT/build/emu/fm_idm.wasm"
fi

if should_build hiphop; then
  if [ ! -f firmware/src/hiphop_data.h ]; then
    echo "build.sh: hiphop-only requested but firmware/src/hiphop_data.h is missing" >&2
    exit 1
  fi
  build_wasm EMU_APP_HIPHOP build/emu/emu_web_hiphop.o build/emu/fm_hiphop.wasm
  echo "built $ROOT/build/emu/fm_hiphop.wasm"
fi

if should_build pop; then
  if [ ! -f firmware/src/pop_data.h ]; then
    echo "build.sh: pop-only requested but firmware/src/pop_data.h is missing" >&2
    exit 1
  fi
  build_wasm EMU_APP_POP build/emu/emu_web_pop.o build/emu/fm_pop.wasm
  echo "built $ROOT/build/emu/fm_pop.wasm"
fi

if should_build funk; then
  if [ ! -f firmware/src/funk_data.h ]; then
    echo "build.sh: funk-only requested but firmware/src/funk_data.h is missing" >&2
    exit 1
  fi
  build_wasm EMU_APP_FUNK build/emu/emu_web_funk.o build/emu/fm_funk.wasm
  echo "built $ROOT/build/emu/fm_funk.wasm"
fi

if should_build snowcrash; then
  if [ ! -f firmware/src/snowcrash_data.h ]; then
    echo "build.sh: snowcrash-only requested but firmware/src/snowcrash_data.h is missing" >&2
    exit 1
  fi
  build_wasm EMU_APP_SNOWCRASH build/emu/emu_web_snowcrash.o build/emu/fm_snowcrash.wasm
  echo "built $ROOT/build/emu/fm_snowcrash.wasm"
fi

if should_build techno; then
  if [ ! -f firmware/src/techno_data.h ]; then
    echo "build.sh: techno-only requested but firmware/src/techno_data.h is missing" >&2
    exit 1
  fi
  build_wasm EMU_APP_TECHNO build/emu/emu_web_techno.o build/emu/fm_techno.wasm
  echo "built $ROOT/build/emu/fm_techno.wasm"
fi

if should_build triphop; then
  if [ ! -f firmware/src/triphop_data.h ]; then
    echo "build.sh: triphop-only requested but firmware/src/triphop_data.h is missing" >&2
    exit 1
  fi
  build_wasm EMU_APP_TRIPHOP build/emu/emu_web_triphop.o build/emu/fm_triphop.wasm
  echo "built $ROOT/build/emu/fm_triphop.wasm"
fi

if should_build bass; then
  if [ ! -f firmware/src/bass_data.h ]; then
    echo "build.sh: bass-only requested but firmware/src/bass_data.h is missing" >&2
    exit 1
  fi
  build_wasm EMU_APP_BASS build/emu/emu_web_bass.o build/emu/fm_bass.wasm
  echo "built $ROOT/build/emu/fm_bass.wasm"
fi

if should_build deck; then
  build_wasm EMU_APP_DECK build/emu/emu_web_deck.o build/emu/fm_deck.wasm
  echo "built $ROOT/build/emu/fm_deck.wasm"
fi

if should_build jet; then
  build_wasm EMU_APP_JET build/emu/emu_web_jet.o build/emu/fm_jet.wasm
  echo "built $ROOT/build/emu/fm_jet.wasm"
fi

cp web/emu/worklet.js build/emu/
cp web/emu/fm1ota.js build/emu/
cp web/emu/fm1pkg.js build/emu/
if [ -f web/emu/index.html ]; then
  cp web/emu/index.html build/emu/
fi

# Local .fwsc images for the WebMIDI flasher (build/emu/fw/)
if [ -f firmware/stock/FM-1_V15.fwsc ]; then
  cp -p firmware/stock/FM-1_V15.fwsc build/emu/fw/FM-1_V15.fwsc
fi
if [ -f build/fm_rytm.fwsc ]; then
  cp -p build/fm_rytm.fwsc build/emu/fw/fm_rytm.fwsc
fi
if [ -f build/fm_ctrl.fwsc ]; then
  cp -p build/fm_ctrl.fwsc build/emu/fw/fm_ctrl.fwsc
fi
if [ -f build/fm_dub.fwsc ]; then
  cp -p build/fm_dub.fwsc build/emu/fw/fm_dub.fwsc
fi
if [ -f build/fm_house.fwsc ]; then
  cp -p build/fm_house.fwsc build/emu/fw/fm_house.fwsc
fi
if [ -f build/fm_hvypsy.fwsc ]; then
  cp -p build/fm_hvypsy.fwsc build/emu/fw/fm_hvypsy.fwsc
fi
if [ -f build/fm_jazz.fwsc ]; then
  cp -p build/fm_jazz.fwsc build/emu/fw/fm_jazz.fwsc
fi
if [ -f build/fm_soul.fwsc ]; then
  cp -p build/fm_soul.fwsc build/emu/fw/fm_soul.fwsc
fi
if [ -f build/fm_idm.fwsc ]; then
  cp -p build/fm_idm.fwsc build/emu/fw/fm_idm.fwsc
fi
if [ -f build/fm_hiphop.fwsc ]; then
  cp -p build/fm_hiphop.fwsc build/emu/fw/fm_hiphop.fwsc
fi
if [ -f build/fm_pop.fwsc ]; then
  cp -p build/fm_pop.fwsc build/emu/fw/fm_pop.fwsc
fi
if [ -f build/fm_funk.fwsc ]; then
  cp -p build/fm_funk.fwsc build/emu/fw/fm_funk.fwsc
fi
if [ -f build/fm_snowcrash.fwsc ]; then
  cp -p build/fm_snowcrash.fwsc build/emu/fw/fm_snowcrash.fwsc
fi
if [ -f build/fm_techno.fwsc ]; then
  cp -p build/fm_techno.fwsc build/emu/fw/fm_techno.fwsc
fi
if [ -f build/fm_triphop.fwsc ]; then
  cp -p build/fm_triphop.fwsc build/emu/fw/fm_triphop.fwsc
fi
if [ -f build/fm_bass.fwsc ]; then
  cp -p build/fm_bass.fwsc build/emu/fw/fm_bass.fwsc
fi
if [ -f build/fm_deck.fwsc ]; then
  cp -p build/fm_deck.fwsc build/emu/fw/fm_deck.fwsc
fi
if [ -f build/fm_jet.fwsc ]; then
  cp -p build/fm_jet.fwsc build/emu/fw/fm_jet.fwsc
fi
if [ -f build/triptych.fwsc ]; then
  cp -p build/triptych.fwsc build/emu/fw/triptych.fwsc
fi
for x0x_pkg in /tmp/fm1-x0x/build/x0x.fwsc /tmp/fm1-x0x/x0x.fwsc /tmp/fm1-x0x/dist/x0x.fwsc; do
  if [ -f "$x0x_pkg" ]; then
    cp -p "$x0x_pkg" build/emu/fw/x0x.fwsc
    break
  fi
done

# sloopDX (isod89/sloop-fm1) and Felucca (hugelton/Felucca): copy sloopdx.fwsc / felucca.fwsc
# here when you build them locally — we do not clone those repos in this script.
if [ -f build/sloopdx.fwsc ]; then
  cp -p build/sloopdx.fwsc build/emu/fw/sloopdx.fwsc
fi
if [ -f build/felucca.fwsc ]; then
  cp -p build/felucca.fwsc build/emu/fw/felucca.fwsc
fi

python3 -c "
import json, pathlib
emu = pathlib.Path('build/emu')
fw = pathlib.Path('build/emu/fw')
inv = {
    'wasm': sorted(p.name for p in emu.glob('*.wasm') if p.is_file()),
    'fw': sorted(p.name for p in fw.glob('*.fwsc') if p.is_file()),
}
pathlib.Path('build/emu/inventory.json').write_text(
    json.dumps(inv, separators=(',', ':')), encoding='utf-8')
"
