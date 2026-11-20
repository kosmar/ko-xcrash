#!/bin/sh
# One-click / one-command restore to official M-VAVE FM-1 Stock V15 firmware (FM-1_015).
set -e
cd "$(dirname "$0")"
STOCK="firmware/stock/FM-1_V15.fwsc"
EXPECTED_SHA="db1642b2b6fa5c2cccb11ffd13878068bb28601678d3644049f99dc40e7edb8a"

if [ ! -f "$STOCK" ]; then
    echo "Downloading official M-VAVE FM-1 V15 firmware..."
    mkdir -p firmware/stock
    curl -fsSL "https://yms-file-store.oss-cn-hongkong.aliyuncs.com/software/firmware/FM-1.fwsc" -o "$STOCK"
fi

ACTUAL_SHA="$(shasum -a 256 "$STOCK" | awk '{print $1}')"
if [ "$ACTUAL_SHA" != "$EXPECTED_SHA" ]; then
    echo "ERROR: SHA-256 mismatch on $STOCK ($ACTUAL_SHA != $EXPECTED_SHA)"
    exit 1
fi

echo "Verified M-VAVE FM-1 Stock V15 ($STOCK, SHA-256 OK)."
exec python3 tools/fm1_install.py --yes "$STOCK" "$@"
