/* Minimal Zephyr Bluetooth shims for host unit tests (no SDK needed).
 * SPDX-License-Identifier: MIT
 */
#ifndef ZEPHYR_BLUETOOTH_H
#define ZEPHYR_BLUETOOTH_H

#include <stddef.h>
#include <stdint.h>
#include <zephyr/bluetooth/addr.h>

#define BT_GAP_ADV_SLOW_INT_MIN 0x0640
#define BT_LE_ADV_OPT_USE_IDENTITY (1UL << 4)

#define BT_DATA_MANUFACTURER_DATA 0xFF
#define BT_DATA(_type, _data, _len)                                            \
	{                                                                      \
		.type = (_type), .data = (const uint8_t *)(_data),             \
		.data_len = (_len),                                            \
	}

struct bt_data {
	uint8_t type;
	uint8_t data_len;
	const uint8_t *data;
};

struct bt_le_adv_param {
	uint8_t id;
	uint8_t sid;
	uint8_t secondary_max_skip;
	uint32_t options;
	uint16_t interval_min;
	uint16_t interval_max;
	const void *peer;
};

#endif
