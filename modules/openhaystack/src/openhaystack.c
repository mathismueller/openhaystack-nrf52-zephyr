/*
 * SPDX-License-Identifier: MIT
 */

#include "openhaystack.h"
#include <string.h>

/* Advertising interval: 5 s (Offline Finding spec wants 2 s; 5 s halves
 * average radio energy at the cost of sparser finder pickups) */
struct bt_le_adv_param of_adv_param = {
	.id = 1,
	.sid = 0,
	.secondary_max_skip = 0,
	.options = BT_LE_ADV_OPT_USE_IDENTITY,
	.interval_min = 5 * BT_GAP_ADV_SLOW_INT_MIN,
	.interval_max = 5 * BT_GAP_ADV_SLOW_INT_MIN,
	.peer = NULL,
};

/* Manufacturer data template: Apple 0x004C, type 0x12, len 0x19 */
uint8_t of_manufacturer_data[] = {
	0x4c, 0x00, /* Company ID (Apple) */
	0x12,       /* Offline Finding type */
	0x19,       /* len */
	0x00,       /* Status (e.g. battery) */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* key bytes 6..27 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, /* First two bits of public key */
	0x00, /* Hint */
};

struct bt_data of_advertising_data[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, of_manufacturer_data,
		sizeof(of_manufacturer_data)),
};

void of_set_address_from_key(bt_addr_le_t *address, char key[28])
{
	/* cast: plain char is unsigned on ARM but signed on x86 hosts;
	 * bitwise ops must not depend on it (key bytes are arbitrary) */
	address->a.val[5] = (uint8_t)key[0] | 0b11000000;
	address->a.val[4] = key[1];
	address->a.val[3] = key[2];
	address->a.val[2] = key[3];
	address->a.val[1] = key[4];
	address->a.val[0] = key[5];
}

void of_set_manufacturer_data_from_key(uint8_t data[29], char key[28])
{
	memcpy(&data[5], &key[6], 22);
	data[27] = (uint8_t)key[0] >> 6;
}
