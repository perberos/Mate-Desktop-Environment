/*
 *
 *  Copyright (C) 2009 Bastien Nocera <hadess@hadess.net>
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

#ifndef __BLUETOOTH_INPUT_H
#define __BLUETOOTH_INPUT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BLUETOOTH_TYPE_INPUT (bluetooth_input_get_type())
#define BLUETOOTH_INPUT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					BLUETOOTH_TYPE_INPUT, BluetoothInput))
#define BLUETOOTH_INPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					BLUETOOTH_TYPE_INPUT, BluetoothInputClass))
#define BLUETOOTH_IS_INPUT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
							BLUETOOTH_TYPE_INPUT))
#define BLUETOOTH_IS_INPUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
							BLUETOOTH_TYPE_INPUT))
#define BLUETOOTH_GET_INPUT_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					BLUETOOTH_TYPE_INPUT, BluetoothInputClass))

typedef struct _BluetoothInput BluetoothInput;
typedef struct _BluetoothInputClass BluetoothInputClass;

struct _BluetoothInput {
	GObject parent;
};

struct _BluetoothInputClass {
	GObjectClass parent_class;

	void (* keyboard_appeared) (BluetoothInput *listener);
	void (* keyboard_disappeared) (BluetoothInput *listener);
	void (* mouse_appeared) (BluetoothInput *listener);
	void (* mouse_disappeared) (BluetoothInput *listener);
};

GType bluetooth_input_get_type(void);

BluetoothInput *bluetooth_input_new(void);

void bluetooth_input_check_for_devices (BluetoothInput *input);

G_END_DECLS

#endif /* __BLUETOOTH_INPUT_H */
