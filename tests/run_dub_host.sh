#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Host-run KO DUB / KO HOUSE / KO HVYPSY (firmware/src/fm_app.c + tests/host.c via platform.c).
set -e
cd "$(dirname "$0")/.."
APP="${1:-dub}"
mkdir -p build/host
COMMON="-O2 -Wall -Wno-unused-function -DDINGHY_HOST -DAPP_VER=\"${APP_VER:-dev}\" -Ifirmware/src -Ifirmware/gen"
case "$APP" in
  dub)
    cc $COMMON -o build/host/dub firmware/src/fm_app.c
    build/host/dub build/host/dub tests/dub_test.txt
    ;;
  house)
    cc $COMMON -DGENRE_HOUSE -o build/host/house firmware/src/fm_app.c
    build/host/house build/host/house tests/house_test.txt
    ;;
  hvypsy)
    cc $COMMON -DGENRE_HEAVY_PSYCH -o build/host/hvypsy firmware/src/fm_app.c
    build/host/hvypsy build/host/hvypsy tests/hvypsy_test.txt
    ;;
  snowcrash)
    cc $COMMON -DGENRE_SNOWCRASH -o build/host/snowcrash firmware/src/fm_app.c
    build/host/snowcrash build/host/snowcrash tests/house_test.txt
    ;;
  hiphop)
    cc $COMMON -DGENRE_HIPHOP -DFELUCCA_ID=\"FM-1_56406\" -o build/host/hiphop firmware/src/fm_app.c
    build/host/hiphop build/host/hiphop tests/hiphop_test.txt
    ;;
  pop)
    cc $COMMON -DGENRE_POP -DFELUCCA_ID=\"FM-1_56407\" -o build/host/pop firmware/src/fm_app.c
    build/host/pop build/host/pop tests/pop_test.txt
    ;;
  funk)
    cc $COMMON -DGENRE_FUNK -DFELUCCA_ID=\"FM-1_56408\" -o build/host/funk firmware/src/fm_app.c
    build/host/funk build/host/funk tests/funk_test.txt
    ;;
  techno)
    cc $COMMON -DGENRE_TECHNO -DFELUCCA_ID=\"FM-1_56409\" -o build/host/techno firmware/src/fm_app.c
    build/host/techno build/host/techno tests/techno_test.txt
    ;;
  triphop)
    cc $COMMON -DGENRE_TRIPHOP -DFELUCCA_ID=\"FM-1_56412\" -o build/host/triphop firmware/src/fm_app.c
    build/host/triphop build/host/triphop tests/triphop_test.txt
    ;;
  soul)
    cc $COMMON -DGENRE_SOUL -DFELUCCA_ID=\"FM-1_56414\" -o build/host/soul firmware/src/fm_app.c
    build/host/soul build/host/soul tests/soul_test.txt
    ;;
  *)
    echo "usage: $0 [dub|house|hvypsy|snowcrash|hiphop|pop|funk|techno|triphop|soul]" >&2
    exit 1
    ;;
esac
