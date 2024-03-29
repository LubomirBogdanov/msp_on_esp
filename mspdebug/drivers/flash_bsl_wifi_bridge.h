/* MSPDebug - debugging tool for MSP430 MCUs
 * Copyright (C) 2009, 2010 Daniel Beer
 * Copyright (C) 2010 Andrew Armenia
 * Copyright (C) 2019 Lubomir Bogdanov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef FLASH_BSL_WIFI_BRIDGE_H_
#define FLASH_BSL_WIFI_BRIDGE_H_

#include "device.h"

#define WUB_CMD_TIMEOUT_MS	5000

/* MSP430 Flash bootloader implementation. */
extern const struct device_class device_flash_bsl_wifi_bridge;

#endif
