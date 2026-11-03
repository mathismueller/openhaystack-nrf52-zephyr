# openhaystack-nrf52-zephyr

OpenHaystack BLE tag on nRF52840 ProMicro (nice!nano v2 footprint, LiPo),
built on the official **nRF Connect SDK** (Zephyr + `sdk-nrf`, pinned in `west.yml`).

Port of `koenvervloesem/openhaystack-zephyr` (MIT) to current NCS conventions.
Advertises the same Apple `4C 00 / 12 19` payload; no Apple ID is needed to
verify the advertisement itself.

Reuses the proven bench setup from `rtl8196e-gateway/4-Bench-Sniffer-nRF52840`:
same board (`nice_nano_v2`), same workspace pattern, same toolchain.

## Layout (Zephyr example-application + thin committable layer)

* `west.yml` — pins `sdk-nrf @ v3.4.1` (NCS LTS). Keep in sync with `~/nrf-ws`.
  One manifest per workspace; heavy checkout lives **outside** this repo.
* `app/` — firmware (`CMakeLists.txt`, `prj.conf`, `src/main.c`)
* `app/boards/` — overlays (LFCLK=RC for XTAL-less clones, UF2 output)
* `board-root/` — vendored `nice_nano` board def (BOARD_ROOT, from ZMK history,
  de-ZMK-ified; NCS v2.6.1 has no `nice_nano_v2` upstream)
* `modules/openhaystack/` — reusable Zephyr module (`module.yml` + `Kconfig`)
* `build_tag.sh` — west wrapper using `~/nrf-sniffer-ws`
* `firmware/` — committed `.uf2` artifacts (drag-drop to NICENANO)
* `openhaypatch.sh` — patch `OFFLINEFINDINGPUBLICKEYHERE!` in the `.bin` with your key

## Build / flash (USB only, no SWD)

```sh
./build_tag.sh              # BOARD=nice_nano_v2 (default), uses ~/nrf-ws
# Output: firmware/haystack-tag-nice_nano_v2.uf2
```

1. Double-press RST → `NICENANO` drive mounts.
2. Drag-drop the `.uf2` → board auto-resets.

Debug logs over USB CDC:
```sh
west build -b nice_nano_v2 app -- -DOVERLAY_CONFIG=debug-usb-uart.conf
```

Patch your key (28 bytes raw, base64):
```sh
ADV_KEY_BASE64=YOUR_KEY ./openhaypatch.sh build/zephyr/zephyr.bin tag.bin
```

## Workspace setup (one time, generic `~/nrf-ws` shared with the sniffer)

```sh
west init -m https://github.com/nrfconnect/sdk-nrf --mr v3.4.1 ~/nrf-ws
cd ~/nrf-ws && west update
uv venv --python 3.12 ~/nrf-ws/.venv
uv pip install --python ~/nrf-ws/.venv/bin/python west -r zephyr/scripts/requirements.txt -r nrf/scripts/requirements.txt -r bootloader/mcuboot/scripts/requirements.txt
export ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb GNUARMEMB_TOOLCHAIN_PATH=/opt/homebrew
```

## Notes

* Fixed static key = trackable by anyone nearby. Rotate keys for real use.
* Fetching location reports still goes through Apple's servers and is
  throttled/limited (see upstream issues). Use a throwaway Apple ID.
* LiPo charging is hardware (on-board charger); FW only sleeps between 5 s ads.
  Clones may lack the 32 kHz crystal → LFCLK=RC is set in
  `app/boards/nice_nano_v2.conf` (same as sniffer). Verify your variant's VBAT
  divider pin before enabling the ADC snippet in the overlay.
