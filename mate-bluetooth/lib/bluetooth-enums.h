/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2005-2008  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef __BLUETOOTH_ENUMS_H
#define __BLUETOOTH_ENUMS_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * SECTION:bluetooth-enums
 * @short_description: Bluetooth related enumerations
 * @stability: Stable
 * @include: bluetooth-enums.h
 *
 * Enumerations related to Bluetooth.
 **/

/**
 * BluetoothCategory:
 * @BLUETOOTH_CATEGORY_ALL: all devices
 * @BLUETOOTH_CATEGORY_PAIRED: paired devices
 * @BLUETOOTH_CATEGORY_TRUSTED: trusted devices
 * @BLUETOOTH_CATEGORY_NOT_PAIRED_OR_TRUSTED: neither paired, nor trusted devices
 * @BLUETOOTH_CATEGORY_PAIRED_OR_TRUSTED: paired and/or trusted devices
 *
 * The category of a Bluetooth devices.
 **/
typedef enum {
	BLUETOOTH_CATEGORY_ALL,
	BLUETOOTH_CATEGORY_PAIRED,
	BLUETOOTH_CATEGORY_TRUSTED,
	BLUETOOTH_CATEGORY_NOT_PAIRED_OR_TRUSTED,
	BLUETOOTH_CATEGORY_PAIRED_OR_TRUSTED,
	BLUETOOTH_CATEGORY_NUM_CATEGORIES /*< skip >*/
} BluetoothCategory;

/**
 * BluetoothType:
 * @BLUETOOTH_TYPE_ANY: any device, or a device of an unknown type
 * @BLUETOOTH_TYPE_PHONE: a telephone (usually a cell/mobile phone)
 * @BLUETOOTH_TYPE_MODEM: a modem
 * @BLUETOOTH_TYPE_COMPUTER: a computer, can be a laptop, a wearable computer, etc.
 * @BLUETOOTH_TYPE_NETWORK: a network device, such as a router
 * @BLUETOOTH_TYPE_HEADSET: a headset (usually a hands-free device)
 * @BLUETOOTH_TYPE_HEADPHONES: headphones (covers two ears)
 * @BLUETOOTH_TYPE_OTHER_AUDIO: another type of audio device
 * @BLUETOOTH_TYPE_KEYBOARD: a keyboard
 * @BLUETOOTH_TYPE_MOUSE: a mouse
 * @BLUETOOTH_TYPE_CAMERA: a camera (still or moving)
 * @BLUETOOTH_TYPE_PRINTER: a printer
 * @BLUETOOTH_TYPE_JOYPAD: a joypad, joystick, or other game controller
 * @BLUETOOTH_TYPE_TABLET: a drawing tablet
 *
 * The type of a Bluetooth device. See also %BLUETOOTH_TYPE_INPUT and %BLUETOOTH_TYPE_AUDIO
 **/
typedef enum {
	BLUETOOTH_TYPE_ANY		= 1 << 0,
	BLUETOOTH_TYPE_PHONE		= 1 << 1,
	BLUETOOTH_TYPE_MODEM		= 1 << 2,
	BLUETOOTH_TYPE_COMPUTER		= 1 << 3,
	BLUETOOTH_TYPE_NETWORK		= 1 << 4,
	BLUETOOTH_TYPE_HEADSET		= 1 << 5,
	BLUETOOTH_TYPE_HEADPHONES	= 1 << 6,
	BLUETOOTH_TYPE_OTHER_AUDIO	= 1 << 7,
	BLUETOOTH_TYPE_KEYBOARD		= 1 << 8,
	BLUETOOTH_TYPE_MOUSE		= 1 << 9,
	BLUETOOTH_TYPE_CAMERA		= 1 << 10,
	BLUETOOTH_TYPE_PRINTER		= 1 << 11,
	BLUETOOTH_TYPE_JOYPAD		= 1 << 12,
	BLUETOOTH_TYPE_TABLET		= 1 << 13,
	BLUETOOTH_TYPE_VIDEO		= 1 << 14,
} BluetoothType;

#define _BLUETOOTH_TYPE_NUM_TYPES 15

#define BLUETOOTH_TYPE_INPUT (BLUETOOTH_TYPE_KEYBOARD | BLUETOOTH_TYPE_MOUSE | BLUETOOTH_TYPE_TABLET | BLUETOOTH_TYPE_JOYPAD)
#define BLUETOOTH_TYPE_AUDIO (BLUETOOTH_TYPE_HEADSET | BLUETOOTH_TYPE_HEADPHONES | BLUETOOTH_TYPE_OTHER_AUDIO)

/**
 * BluetoothColumn:
 * @BLUETOOTH_COLUMN_PROXY: a #DBusGProxy object
 * @BLUETOOTH_COLUMN_ADDRESS: a string representing a Bluetooth address
 * @BLUETOOTH_COLUMN_ALIAS: a string to use for display (the name of the device, or its address if the name is not known). Only available for devices.
 * @BLUETOOTH_COLUMN_NAME: a string representing the device or adapter's name
 * @BLUETOOTH_COLUMN_TYPE: the #BluetoothType of the device. Only available for devices.
 * @BLUETOOTH_COLUMN_ICON: a string representing the icon name for the device. Only available for devices.
 * @BLUETOOTH_COLUMN_DEFAULT: whether the adapter is the default one. Only available for adapters.
 * @BLUETOOTH_COLUMN_PAIRED: whether the device is paired to its parent adapter. Only available for devices.
 * @BLUETOOTH_COLUMN_TRUSTED: whether the device is trusted. Only available for devices.
 * @BLUETOOTH_COLUMN_CONNECTED: whether the device is connected. Only available for devices.
 * @BLUETOOTH_COLUMN_DISCOVERABLE: whether the adapter is discoverable/visible. Only available for adapters.
 * @BLUETOOTH_COLUMN_DISCOVERING: whether the adapter is discovering. Only available for adapters.
 * @BLUETOOTH_COLUMN_LEGACYPAIRING: whether the device does not support Bluetooth 2.1 Simple Secure Pairing. Only available for devices.
 * @BLUETOOTH_COLUMN_POWERED: whether the adapter is powered. Only available for adapters.
 * @BLUETOOTH_COLUMN_SERVICES: an array of service names and #BluetoothStatus connection statuses.
 * @BLUETOOTH_COLUMN_UUIDS: a string array of human-readable UUIDs.
 *
 * A column identifier to pass to bluetooth_chooser_get_selected_device_info().
 **/
typedef enum {
	BLUETOOTH_COLUMN_PROXY,
	BLUETOOTH_COLUMN_ADDRESS,
	BLUETOOTH_COLUMN_ALIAS,
	BLUETOOTH_COLUMN_NAME,
	BLUETOOTH_COLUMN_TYPE,
	BLUETOOTH_COLUMN_ICON,
	BLUETOOTH_COLUMN_DEFAULT,
	BLUETOOTH_COLUMN_PAIRED,
	BLUETOOTH_COLUMN_TRUSTED,
	BLUETOOTH_COLUMN_CONNECTED,
	BLUETOOTH_COLUMN_DISCOVERABLE,
	BLUETOOTH_COLUMN_DISCOVERING,
	BLUETOOTH_COLUMN_LEGACYPAIRING,
	BLUETOOTH_COLUMN_POWERED,
	BLUETOOTH_COLUMN_SERVICES,
	BLUETOOTH_COLUMN_UUIDS,
	_BLUETOOTH_NUM_COLUMNS /*< skip >*/
} BluetoothColumn;

/**
 * BluetoothStatus:
 *
 * @BLUETOOTH_STATUS_INVALID: whether the status has been set yet
 * @BLUETOOTH_STATUS_DISCONNECTED: whether the service is disconnected
 * @BLUETOOTH_STATUS_CONNECTED: whether the service is connected
 * @BLUETOOTH_STATUS_CONNECTING: whether the service is connecting
 * @BLUETOOTH_STATUS_PLAYING: whether the service is playing (only used by the audio service)
 *
 * The connection status of a service on a particular device. Note that @BLUETOOTH_STATUS_CONNECTING and @BLUETOOTH_STATUS_PLAYING might not be available for all services.
 **/
typedef enum {
	BLUETOOTH_STATUS_INVALID = 0,
	BLUETOOTH_STATUS_DISCONNECTED,
	BLUETOOTH_STATUS_CONNECTED,
	BLUETOOTH_STATUS_CONNECTING,
	BLUETOOTH_STATUS_PLAYING
} BluetoothStatus;

G_END_DECLS

#endif /* __BLUETOOTH_ENUMS_H */
