/* Minimal Zephyr Bluetooth shims for host unit tests (no SDK needed).
 * Mirrors just the types/macros used by modules/openhaystack/.
 * SPDX-License-Identifier: MIT
 */
#ifndef ZEPHYR_BLUETOOTH_ADDR_H
#define ZEPHYR_BLUETOOTH_ADDR_H

#include <stdint.h>

#define BT_ADDR_LE_RANDOM 0x01

typedef struct {
	uint8_t val[6];
} bt_addr_t;

typedef struct {
	uint8_t type;
	bt_addr_t a;
} bt_addr_le_t;

#endif
