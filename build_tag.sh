#!/bin/bash
# SPDX-License-Identifier: MIT
# Build the Haystack tag (OpenHaystack on nRF52840 Pro Micro / nice!nano v2).
# Uses the generic out-of-tree NCS workspace at ~/nrf-ws (see README.md).
# Board files: upstream promicro_nrf52840 (NCS >= v3.x) or vendored nice_nano
# via BOARD_ROOT (legacy NCS v2.6.1 workspace).
# Usage: ./build_tag.sh [BOARD=promicro_nrf52840/nrf52840/uf2]
#        NCS_WS=~/nrf-sniffer-ws BOARD=nice_nano_v2 ./build_tag.sh  (legacy)
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
# Generic NCS workspace (v3.4.1 LTS).
WS="${NCS_WS:-$HOME/nrf-ws}"
BOARD="${BOARD:-promicro_nrf52840/nrf52840/uf2}"
# Overlay/conf file stem in app/boards/ (slashes -> underscores).
STEM="${BOARD//\//_}"

if [ ! -d "$WS" ]; then
  echo "NCS workspace not found at $WS" >&2
  echo "See README.md (west init -m https://github.com/nrfconnect/sdk-nrf --mr v3.4.1)" >&2
  exit 1
fi

# shellcheck disable=SC1091
source "$WS/.venv/bin/activate"
cd "$WS" # west finds the workspace (.west) via CWD

export ZEPHYR_TOOLCHAIN_VARIANT="${ZEPHYR_TOOLCHAIN_VARIANT:-gnuarmemb}"
export GNUARMEMB_TOOLCHAIN_PATH="${GNUARMEMB_TOOLCHAIN_PATH:-/opt/homebrew}"

BUILD_DIR="$WS/build-tag-$STEM"
west build -b "$BOARD" "$REPO_ROOT/app" -d "$BUILD_DIR" -- \
  "-DBOARD_ROOT=$REPO_ROOT/board-root" \
  "-DDTC_OVERLAY_FILE=$REPO_ROOT/app/boards/$STEM.overlay" \
  "-DEXTRA_CONF_FILE=$REPO_ROOT/app/boards/$STEM.conf" \
  -DCONFIG_BUILD_OUTPUT_UF2=y

OUT="$BUILD_DIR/app/zephyr/zephyr.uf2"
[ -f "$OUT" ] || OUT="$BUILD_DIR/zephyr/zephyr.uf2"  # pre-sysbuild layout
DEST="$REPO_ROOT/firmware/haystack-tag-$STEM.uf2"
mkdir -p "$REPO_ROOT/firmware"
cp -v "$OUT" "$DEST"
sha256sum "$DEST"
