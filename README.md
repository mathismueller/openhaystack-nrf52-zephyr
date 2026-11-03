# openhaystack-nrf52-zephyr

OpenHaystack BLE tag on nRF52840 ProMicro (nice!nano v2 compatible) with LiPo,
built on the official **nRF Connect SDK** (Zephyr + `sdk-nrf`, pinned in `west.yml`).

Port of `koenvervloesem/openhaystack-zephyr` (MIT) to current NCS conventions.
Advertises the same Apple `4C 00 / 12 19` payload; no Apple ID is needed to
verify the advertisement itself.

## Layout (Zephyr example-application)

* `west.yml` — west manifest, pins `sdk-nrf` (=> Zephyr) by hash, e.g. `v2.9.0`
* `app/` — the firmware (`CMakeLists.txt`, `prj.conf`, `src/main.c`)
* `app/boards/` — board overlays (NOT a custom board; uses upstream `nice_nano_v2`)
* `modules/openhaystack/` — reusable Zephyr module (`module.yml` + `Kconfig`)
* `openhaypatch.sh` — patch `OFFLINEFINDINGPUBLICKEYHERE!` in the `.bin` with your key

## West workspace

```sh
# from the parent of this repo:
west init -m openhaystack-nrf52-zephyr --mr main workspace
cd workspace && west update
```

Needs `nrfutil` toolchain (`nrfutil toolchain-manager install`). Pinned SDK =
reproducible builds; no git submodules for the SDK (see `west.yml`).

## Build / flash (nice!nano v2, UF2)

```sh
cd workspace/openhaystack-nrf52-zephyr
west build -p auto -b nice_nano_v2 -s app
# UF2 at build/zephyr/zephyr.uf2 — double-tap reset, drag & drop
west build -b nice_nano_v2 -s app -- -DOVERLAY_CONFIG=debug-usb-uart.conf  # USB logs
```

Patch your key (28 bytes raw, base64):

```sh
ADV_KEY_BASE64=YOUR_KEY ./openhaypatch.sh build/zephyr/zephyr.bin tag.bin
```

## Notes

* Fixed static key = trackable by anyone nearby. Rotate keys for real use.
* Fetching location reports still goes through Apple's servers and is
  throttled/limited (see upstream issues). Use a throwaway Apple ID.
* LiPo charging is hardware (on-board charger); FW only sleeps between 2 s ads.
  Verify your variant's VBAT divider pin before enabling the ADC snippet in
  `app/boards/nice_nano_v2.overlay`.
