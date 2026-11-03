/*
 * haystack tag - OpenHaystack on nRF52840 (Zephyr / nRF Connect SDK)
 * Ported from koenvervloesem/openhaystack-zephyr (MIT) + acalatrava/openhaystack-firmware
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

#include <string.h>

#include <hal/nrf_power.h>

#include "openhaystack.h"

LOG_MODULE_REGISTER(haystack_tag, LOG_LEVEL_INF);

/* Visual diagnostics: only compiled in when logging is enabled (debug
 * builds). Release stays dark and parked in k_sleep for battery life. */
#if defined(CONFIG_LOG)
#define DBG_BLINK 1
static const struct gpio_dt_spec tag_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

static void blink(int times, int on_ms, int off_ms)
{
	if (!gpio_is_ready_dt(&tag_led)) {
		return;
	}
	gpio_pin_configure_dt(&tag_led, GPIO_OUTPUT_INACTIVE);
	for (int i = 0; i < times; i++) {
		gpio_pin_set_dt(&tag_led, 1);
		k_msleep(on_ms);
		gpio_pin_set_dt(&tag_led, 0);
		k_msleep(off_ms);
	}
}

/* Solid LED + halt: a fatal Bluetooth stage was reached. */
static void die(void)
{
	if (gpio_is_ready_dt(&tag_led)) {
		gpio_pin_configure_dt(&tag_led, GPIO_OUTPUT_ACTIVE);
	}
	while (1) {
		k_sleep(K_FOREVER);
	}
}
#else
#define DBG_BLINK 0
#define blink(...) ((void)0)
#define die() return 0
#endif

/* Software entry to the Adafruit UF2 bootloader: the bootloader checks
 * GPREGRET on reset, 0x57 = UF2+CDC mode. Triggered via the shell `uf2`
 * command (debug), an Arduino-style 1200-baud touch, or a "UF2!" line on
 * the CDC port (release), so no double-press of RST is needed to reflash. */
#define UF2_MAGIC 0x57

static void enter_uf2(void)
{
	nrf_power_gpregret_set(NRF_POWER, 0, UF2_MAGIC);
	sys_reboot(SYS_REBOOT_COLD);
	while (1) {
	}
}

#if defined(CONFIG_SHELL)
static int cmd_uf2(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "rebooting into UF2 bootloader...");
	k_msleep(300);
	enter_uf2();
	return 0;
}

SHELL_CMD_REGISTER(uf2, NULL, "Reboot into UF2 flashing mode", cmd_uf2);
#endif

#if defined(CONFIG_CDC_ACM_SERIAL_INITIALIZE_AT_BOOT)
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/usb/usbd.h>

static void check_uf2_touch(void)
{
	uint32_t baud = 0;

	if (uart_line_ctrl_get(DEVICE_DT_GET(DT_CHOSEN(zephyr_console)),
			       UART_LINE_CTRL_BAUD_RATE, &baud) == 0 &&
	    baud == 1200) {
		LOG_INF("1200-baud touch, rebooting to UF2...");
		k_msleep(300);
		enter_uf2();
	}
}

/* Release (manual USB mode): enable the CDC ACM device only while VBUS is
 * present. On battery the USB block never powers up; on USB power the
 * 1200-baud touch is live. Debug (auto-enable) skips the management. */
#if !defined(CONFIG_CDC_ACM_SERIAL_ENABLE_AT_BOOT)
/* Context owned by the board's cdc-acm-snippet init (descriptors, class,
 * usbd_init at boot); the app only owns enable/disable. The context symbol
 * itself is file-static, so take the first iterable section entry (single
 * CDC ACM instance in this app). Pinned NCS v3.4.1. */
static struct usbd_context *usb_ctx(void)
{
	struct usbd_context *ctx;

	STRUCT_SECTION_GET(usbd_context, 0, &ctx);
	return ctx;
}

static bool usb_active;

static void usb_vbus_poll(void)
{
	bool vbus = nrf_power_usbregstatus_vbusdet_get(NRF_POWER);
	int err;

	if (vbus && !usb_active) {
		err = usbd_enable(usb_ctx());
		if (err == 0) {
			usb_active = true;
			LOG_INF("USB enabled (VBUS present)");
		} else {
			LOG_ERR("usbd_enable failed (err %d)", err);
		}
	} else if (!vbus && usb_active) {
		err = usbd_disable(usb_ctx());
		if (err == 0) {
			usb_active = false;
			LOG_INF("USB disabled (VBUS removed)");
		} else {
			LOG_ERR("usbd_disable failed (err %d)", err);
		}
	}
}

static inline bool usb_touch_armed(void)
{
	return usb_active;
}
#else
static inline void usb_vbus_poll(void)
{
}
static inline bool usb_touch_armed(void)
{
	return true;
}
#endif /* manual vs auto USB enable */
#else
#define check_uf2_touch() ((void)0)
static inline void usb_vbus_poll(void)
{
}
static inline bool usb_touch_armed(void)
{
	return false;
}
#endif

/* RX-magic entry: reboot to UF2 when the CDC port receives a line "UF2!".
 * Unlike SET_LINE_CODING, bulk-OUT data forces the host to resume a
 * suspended bus, so this works even when the tag is otherwise silent.
 * Release-only (debug has the shell `uf2` command instead, and the shell
 * owns RX there). Only polled while USB is up, i.e. VBUS-powered. */
#if defined(CONFIG_CDC_ACM_SERIAL_INITIALIZE_AT_BOOT) && \
	!defined(CONFIG_CDC_ACM_SERIAL_ENABLE_AT_BOOT) && \
	!defined(CONFIG_SHELL)
static void check_uf2_rx(void)
{
	const struct device *cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	static char line[16];
	static size_t len;
	unsigned char c;

	while (uart_poll_in(cons, &c) == 0) {
		if (c == '\n' || c == '\r') {
			line[len] = '\0';
			if (strcmp(line, "UF2!") == 0 || strcmp(line, "uf2") == 0) {
				LOG_INF("RX magic, rebooting to UF2...");
				k_msleep(300);
				enter_uf2();
			}
			len = 0;
		} else if (len < sizeof(line) - 1) {
			line[len++] = (char)c;
		} else {
			len = 0; /* overflow: resync */
		}
	}
}
#else
#define check_uf2_rx() ((void)0)
#endif

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

	/* 3 quick blinks = main() reached (visible even if logs are lost) */
	blink(3, 120, 120);

	LOG_INF("Starting haystack tag...");

	of_set_manufacturer_data_from_key(of_manufacturer_data, public_key);
	of_set_address_from_key(&device_address, public_key);

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		die();
	}
	LOG_INF("Bluetooth initialized");

	/* Create the tag identity AFTER bt_enable: pre-enable creation would
	 * take slot 0 (the default identity), while of_adv_param.id = 1
	 * expects the default identity to already exist. Use the returned id
	 * instead of assuming it. */
	err = bt_id_create(&device_address, NULL);
	if (err < 0) {
		LOG_ERR("Can't create identity (err %d)", err);
		die();
	}
	of_adv_param.id = err;

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
		die();
	}
	LOG_INF("Advertising started, interval ~5 s");

	/* Advertiser runs in controller; park app thread in low power.
	 * Debug builds also blink + log a heartbeat so a silent tag can be
	 * told apart from a dead one (compiled out when CONFIG_LOG=n).
	 * Every 5 s the VBUS poll runs (no-op unless manual-gated USB mode)
	 * and the UF2 touch is checked when USB is up. */
	while (1) {
		usb_vbus_poll();
#if DBG_BLINK
		static uint32_t uptime_s;
		uptime_s += 5;
		LOG_INF("tag alive, uptime %u s", uptime_s);
		blink(1, 100, 0);
		{
			uint32_t baud = 0;
			int rc = uart_line_ctrl_get(
				DEVICE_DT_GET(DT_CHOSEN(zephyr_console)),
				UART_LINE_CTRL_BAUD_RATE, &baud);
			LOG_INF("dbg baud=%u rc=%d usb_armed=%d", baud, rc,
				(int)usb_touch_armed());
		}
#endif
		if (usb_touch_armed()) {
			check_uf2_touch();
			check_uf2_rx();
		}
		k_sleep(K_SECONDS(5));
	}
}
