#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -e
cd "$(dirname "$0")/.."
mkdir -p build/host
COMMON="-O2 -Wall -Wno-unused-function -DDINGHY_HOST -DGENRE_POP -DPOP_LEAD_VERIFY -DFELUCCA_ID=\"FM-1_56407\" -Ifirmware/src -Ifirmware/gen"
cc $COMMON -o build/host/pop_verify firmware/src/fm_app.c
echo "=== Suite (register+QC+bench+mix+plates) ==="
build/host/pop_verify suite
