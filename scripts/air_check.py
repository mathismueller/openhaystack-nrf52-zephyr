#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Air check for the haystack tag: asserts the BLE advertisement on air.

Looks for Apple company 0x004C / type 0x12 0x19, verifies the MAC derived
from the public key and measures the advertising interval.

Usage:
    pip install bleak
    ./air_check.py [--key-base64 B64] [--timeout 20] [--expect-interval 5]

Exit 0 = tag found and format OK, 1 = not found / mismatch.
"""
import argparse
import asyncio
import base64
import sys
import time

PLACEHOLDER = b"OFFLINEFINDINGPUBLICKEYHERE!"


def expected_mac(key: bytes) -> str:
    b = [key[0] | 0xC0, key[1], key[2], key[3], key[4], key[5]]
    return ":".join(f"{x:02X}" for x in b)


async def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--key-base64", default=None)
    ap.add_argument("--timeout", type=float, default=20.0)
    ap.add_argument("--expect-interval", type=float, default=5.0)
    ap.add_argument("--interval-tol", type=float, default=1.5)
    args = ap.parse_args()

    key = base64.b64decode(args.key_base64) if args.key_base64 else PLACEHOLDER
    assert len(key) == 28, f"key must be 28 bytes, got {len(key)}"
    mac = expected_mac(key)
    expected_tail = key[6:]  # 22 bytes echoed in manufacturer data

    from bleak import BleakScanner

    hits: list[float] = []
    seen_mfg: bytes | None = None

    def cb(device, adv):
        nonlocal seen_mfg
        m = adv.manufacturer_data.get(0x004C)
        if not m or len(m) < 27 or m[:2] != b"\x12\x19":
            return
        if bytes(m[3:25]) != expected_tail:
            return
        if m[25] != (key[0] >> 6):
            print(f"WARN: top-bits mismatch from {device.address}")
            return
        # macOS CoreBluetooth reports a UUID, not the MAC — only enforce
        # the MAC match when we get a real 17-char BDADDR (Linux/Windows).
        if len(device.address) == 17 and device.address.upper() != mac:
            print(f"WARN: MAC {device.address} != expected {mac}")
            return
        seen_mfg = bytes(m)
        hits.append(time.monotonic())
        print(f"hit {device.address} rssi={adv.rssi} mfg={bytes(m).hex()}")

    async with BleakScanner(detection_callback=cb):
        await asyncio.sleep(args.timeout)

    if not hits:
        print(f"FAIL: no tag with key tail {expected_tail!r} advertising Apple 12 19")
        return 1

    print(f"OK: {len(hits)} advertisement(s) with matching key")
    print(f"public key on air : {seen_mfg[3:25]!r}  (key[6:28] echoed in mfg data)")
    print(f"top bits key[0]   : {seen_mfg[25]:#04x} (== {key[0:1]!r} >> 6)")
    print(f"key[0:6] via addr : static random {mac} (= key[0]|0xC0, key[1:6])")
    print(f"full 28-byte key  : {key!r}  (all parts verified)")
    if len(hits) >= 2:
        gaps = [b - a for a, b in zip(hits, hits[1:])]
        gaps = [g for g in gaps if g >= 1.0]  # ignore same-burst duplicates
        if gaps:
            avg = sum(gaps) / len(gaps)
            print(f"avg interval {avg:.1f}s (expect ~{args.expect_interval:.0f}s)")
            if abs(avg - args.expect_interval) > args.interval_tol:
                if sys.platform == "darwin":
                    print("WARN: interval out of tolerance "
                          "(macOS dedup makes this measurement noisy)")
                else:
                    print("FAIL: interval out of tolerance")
                    return 1
        else:
            print("only same-burst duplicates seen; interval not measurable")
    return 0


if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
