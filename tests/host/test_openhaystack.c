/* Host runner for the advertisement-encoding assertions.
 * Mirrors tests/openhaystack/src/openhaystack_test.c without ztest,
 * so it builds with plain gcc/clang and no Zephyr SDK.
 * SPDX-License-Identifier: MIT
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "openhaystack.h"

static char default_key[28]; /* filled in main(): 28 bytes, no NUL */

static void test_address_from_default_key(void)
{
	bt_addr_le_t addr = {.type = BT_ADDR_LE_RANDOM, .a.val = {0}};

	of_set_address_from_key(&addr, default_key);

	uint8_t expect[6] = {0x4E, 0x49, 0x4C, 0x46, 0x46, 0xCF};

	assert(addr.type == BT_ADDR_LE_RANDOM);
	assert(memcmp(addr.a.val, expect, sizeof(expect)) == 0);
	assert((addr.a.val[5] & 0xC0) == 0xC0);
	printf("ok: address_from_default_key\n");
}

static void test_manufacturer_data_from_default_key(void)
{
	uint8_t mfg[29];

	of_set_manufacturer_data_from_key(of_manufacturer_data, default_key);
	memcpy(mfg, of_manufacturer_data, sizeof(mfg));

	assert(mfg[0] == 0x4C && mfg[1] == 0x00);
	assert(mfg[2] == 0x12 && mfg[3] == 0x19 && mfg[4] == 0x00);
	assert(memcmp(&mfg[5], &default_key[6], 22) == 0);
	assert(mfg[27] == (uint8_t)(default_key[0] >> 6));
	assert(mfg[28] == 0x00);
	/* adv interval is 5 s: 5 * 0x640 units of 0.625 ms = 8000 = 5000 ms */
	assert(of_adv_param.interval_min == 5 * BT_GAP_ADV_SLOW_INT_MIN);
	assert(of_adv_param.interval_max == 5 * BT_GAP_ADV_SLOW_INT_MIN);
	printf("ok: manufacturer_data_from_default_key (+5s interval)\n");
}

static void test_hint_bits_edge_cases(void)
{
	bt_addr_le_t addr = {0};
	uint8_t mfg[29];
	char key[28];

	memset(key, 0x00, sizeof(key));
	key[0] = (char)0xFF;

	of_set_address_from_key(&addr, key);
	assert(addr.a.val[5] == 0xFF);

	of_set_manufacturer_data_from_key(mfg, key);
	assert(mfg[27] == 0x03);
	printf("ok: hint_bits_edge_cases\n");
}

int main(void)
{
	/* 28 raw key bytes, no NUL terminator (same as firmware) */
	memcpy(default_key, "OFFLINEFINDINGPUBLICKEYHERE!", 28);
	test_address_from_default_key();
	test_manufacturer_data_from_default_key();
	test_hint_bits_edge_cases();
	printf("all host tests passed\n");
	return 0;
}
