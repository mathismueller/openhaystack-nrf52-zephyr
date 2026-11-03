/*
 * Host unit tests for the Find My advertisement encoding.
 * Run: west twister -T tests/openhaystack -p native_posix
 *
 * SPDX-License-Identifier: MIT
 */

#include <string.h>

#include <zephyr/ztest.h>

#include "openhaystack.h"

ZTEST_SUITE(openhaystack_key_encoding, NULL, NULL, NULL, NULL, NULL);

/* Default placeholder key used by the firmware (must stay non-const there
 * so openhaypatch.sh can replace it in the .bin). */
static char default_key[28] = "OFFLINEFINDINGPUBLICKEYHERE!";

ZTEST(openhaystack_key_encoding, test_address_from_default_key)
{
	/* firmware inits .type at declaration, helper fills .a.val */
	bt_addr_le_t addr = {.type = BT_ADDR_LE_RANDOM};

	of_set_address_from_key(&addr, default_key);

	/* key[0..5] = O F F L I N = 4F 46 46 4C 49 4E, top bits forced 0b11 */
	uint8_t expect[6] = {0x4E, 0x49, 0x4C, 0x46, 0x46, 0xCF};

	zassert_equal(addr.type, BT_ADDR_LE_RANDOM, "must be random static");
	zassert_mem_equal(addr.a.val, expect, sizeof(expect), "MAC mismatch");
	zassert_equal(addr.a.val[5] & 0xC0, 0xC0, "top bits must be 11");
}

ZTEST(openhaystack_key_encoding, test_manufacturer_data_from_default_key)
{
	uint8_t mfg[29];

	of_set_manufacturer_data_from_key(of_manufacturer_data, default_key);
	memcpy(mfg, of_manufacturer_data, sizeof(mfg));

	zassert_equal(mfg[0], 0x4C, "Apple company ID lo");
	zassert_equal(mfg[1], 0x00, "Apple company ID hi");
	zassert_equal(mfg[2], 0x12, "Offline Finding type");
	zassert_equal(mfg[3], 0x19, "Offline Finding length");
	zassert_equal(mfg[4], 0x00, "status byte");

	/* bytes 6..27 of the key -> mfg[5..26] */
	zassert_mem_equal(&mfg[5], &default_key[6], 22, "key copy mismatch");
	/* 'O' = 0x4F, >> 6 = 0x01 */
	zassert_equal(mfg[27], (uint8_t)(default_key[0] >> 6), "hint bits");
	zassert_equal(mfg[28], 0x00, "hint byte");
}

ZTEST(openhaystack_key_encoding, test_hint_bits_edge_cases)
{
	bt_addr_le_t addr = {0};
	uint8_t mfg[29];
	/* key[0] = 0xFF -> addr top stays 0xFF, hint = 0x03 */
	char key[28];

	memset(key, 0x00, sizeof(key));
	key[0] = (char)0xFF;

	of_set_address_from_key(&addr, key);
	zassert_equal(addr.a.val[5], 0xFF, "0xFF|0xC0 must stay 0xFF");

	of_set_manufacturer_data_from_key(mfg, key);
	zassert_equal(mfg[27], 0x03, "0xFF >> 6 must be 0x03");
}
