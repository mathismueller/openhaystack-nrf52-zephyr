#!/bin/sh
# SPDX-License-Identifier: MIT
# Patch the default key OFFLINEFINDINGPUBLICKEYHERE! in the built .bin with your own 28-byte key.
# Usage: ADV_KEY_BASE64=<base64> ./openhaypatch.sh build/zephyr/zephyr.bin out.bin
set -e
IN=${1:?usage: $0 input.bin output.bin (ADV_KEY_BASE64 env required)}
OUT=${2:?usage: $0 input.bin output.bin (ADV_KEY_BASE64 env required)}
[ -n "$ADV_KEY_BASE64" ] || { echo "ADV_KEY_BASE64 not set"; exit 1; }
KEY_HEX=$(printf '%s' "$ADV_KEY_BASE64" | base64 -d | xxd -p | tr -d '\n')
xxd -p -c 1000000 < "$IN" | \
  sed "s/$(printf 'OFFLINEFINDINGPUBLICKEYHERE!' | xxd -p | tr -d '\n')/$KEY_HEX/" | \
  xxd -r -p > "$OUT"
echo "wrote $OUT"
