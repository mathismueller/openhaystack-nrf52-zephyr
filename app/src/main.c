/*
 * haystack tag - OpenHaystack on nRF52840 (Zephyr / nRF Connect SDK)
 * Ported from koenvervloesem/openhaystack-zephyr (MIT) + acalatrava/openhaystack-firmware
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "openhaystack.h"

LOG_MODULE_REGISTER(haystack_tag, LOG_LEVEL_INF);

/* Don't make `const` so the key can be patched in the .bin (see openhaypatch.sh) */
static char public_key[28] = "OFFLINEFINDINGPUBLICKEYHERE!";
BUILD_ASSERT(sizeof(public_key) == 28, "public key must be 28 bytes");

static bt_addr_le_t device_address = {
	.type = BT_ADDR_LE_RANDOM,
	.a.val = {0xFF, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
};

int main(void)
{
	int err;

	LOG_INF("Starting haystack tag...");

	of_set_manufacturer_data_from_key(of_manufacturer_data, public_key);
	of_set_address_from_key(&device_address, public_key);

	/* id=1 keeps the default identity (0) untouched, see of_adv_param */
	err = bt_id_create(&device_address, NULL);
	if (err < 0) {
		LOG_ERR("Can't create identity (err %d)", err);
		return 0;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return 0;
	}
	LOG_INF("Bluetooth initialized");

	/* Find My uses only the primary (non-connectable) packet, which stays
	 * byte-identical. Scannable mode + scan response carrying the local
	 * name lets common scanner apps (nRF Connect, LightBlue) list the tag
	 * by name; Apple uploaders ignore the scan response. */
	of_adv_param.options |= BT_LE_ADV_OPT_SCANNABLE;

	struct bt_data sd[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
		BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME,
			(sizeof(CONFIG_BT_DEVICE_NAME) - 1)),
	};

	err = bt_le_adv_start(&of_adv_param, of_advertising_data,
			      ARRAY_SIZE(of_advertising_data), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return 0;
	}
	LOG_INF("Advertising started, interval ~5 s");

	/* Advertiser runs in controller; park app thread in low power */
	while (1) {
		k_sleep(K_FOREVER);
	}
}
