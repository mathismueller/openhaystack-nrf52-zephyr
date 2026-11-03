/*
 * SPDX-License-Identifier: MIT
 */

#ifndef _OPENHAYSTACK_H_
#define _OPENHAYSTACK_H_

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>

extern struct bt_le_adv_param of_adv_param;
extern uint8_t of_manufacturer_data[29];
extern struct bt_data of_advertising_data[1];

void of_set_address_from_key(bt_addr_le_t *address, char key[28]);
void of_set_manufacturer_data_from_key(uint8_t data[29], char key[28]);

#endif /* _OPENHAYSTACK_H_ */
