#!/bin/bash
# Build the Haystack tag (OpenHaystack on nRF52840 Pro Micro / nice!nano v2).
# Reuses the proven out-of-tree NCS workspace at ~/nrf-sniffer-ws (NCS v2.6.1,
# gnuarmemb) and the vendored board-root from 4-Bench-Sniffer-nRF52840.
# Usage: ./build_tag.sh [BOARD=nice_nano_v2]
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
WS="${NCS_WS:-$HOME/nrf-sniffer-ws}"
BOARD="${BOARD:-nice_nano_v2}"

if [ ! -d "$WS" ]; then
  echo "NCS workspace not found at $WS" >&2
  echo "See README.md (west init -m https://github.com/nrfconnect/sdk-nrf --mr v2.6.1)" >&2
  exit 1
fi

# shellcheck disable=SC1091
source "$WS/.venv/bin/activate"
cd "$WS" # west finds the workspace (.west) via CWD

export ZEPHYR_TOOLCHAIN_VARIANT="${ZEPHYR_TOOLCHAIN_VARIANT:-gnuarmemb}"
export GNUARMEMB_TOOLCHAIN_PATH="${GNUARMEMB_TOOLCHAIN_PATH:-/opt/homebrew}"

west build -b "$BOARD" "$REPO_ROOT/app" -d "$WS/build-tag-$BOARD" -- \
  "-DBOARD_ROOT=$REPO_ROOT/board-root" \
  -DCONFIG_BUILD_OUTPUT_UF2=y

OUT="$WS/build-tag-$BOARD/zephyr/zephyr.uf2"
DEST="$REPO_ROOT/firmware/haystack-tag-$BOARD.uf2"
mkdir -p "$REPO_ROOT/firmware"
cp -v "$OUT" "$DEST"
sha256sum "$DEST"
