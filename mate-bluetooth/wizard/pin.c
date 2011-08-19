/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2009  Bastien Nocera <hadess@hadess.net>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <bluetooth-enums.h>

#include "pin.h"

#define PIN_CODE_DB "pin-code-database.xml"
#define MAX_DIGITS_PIN_PREFIX "max:"

#define TYPE_IS(x, r) {				\
	if (g_str_equal(type, x)) return r;	\
}

static guint string_to_type(const char *type)
{
	TYPE_IS ("any", BLUETOOTH_TYPE_ANY);
	TYPE_IS ("mouse", BLUETOOTH_TYPE_MOUSE);
	TYPE_IS ("keyboard", BLUETOOTH_TYPE_KEYBOARD);
	TYPE_IS ("headset", BLUETOOTH_TYPE_HEADSET);
	TYPE_IS ("headphones", BLUETOOTH_TYPE_HEADPHONES);
	TYPE_IS ("audio", BLUETOOTH_TYPE_OTHER_AUDIO);
	TYPE_IS ("printer", BLUETOOTH_TYPE_PRINTER);
	TYPE_IS ("network", BLUETOOTH_TYPE_NETWORK);

	g_warning ("unhandled type '%s'", type);
	return BLUETOOTH_TYPE_ANY;
}

typedef struct {
	char *ret_pin;
	guint max_digits;
	guint type;
	const char *address;
	const char *name;
} PinParseData;

static void
pin_db_parse_start_tag (GMarkupParseContext *ctx,
			const gchar         *element_name,
			const gchar        **attr_names,
			const gchar        **attr_values,
			gpointer             data,
			GError             **error)
{
	PinParseData *pdata = (PinParseData *) data;

	if (pdata->ret_pin != NULL || pdata->max_digits != 0)
		return;
	if (g_str_equal (element_name, "device") == FALSE)
		return;

	while (*attr_names && *attr_values) {
		if (g_str_equal (*attr_names, "type")) {
			guint type;

			type = string_to_type (*attr_values);
			if (type != BLUETOOTH_TYPE_ANY && type != pdata->type)
				return;
		} else if (g_str_equal (*attr_names, "oui")) {
			if (g_str_has_prefix (pdata->address, *attr_values) == FALSE)
				return;
		} else if (g_str_equal (*attr_names, "name")) {
			if (g_strcmp0 (*attr_values, pdata->name) != 0)
				return;
		} else if (g_str_equal (*attr_names, "pin")) {
			if (g_str_has_prefix (*attr_values, MAX_DIGITS_PIN_PREFIX) != FALSE) {
				pdata->max_digits = strtoul (*attr_values + strlen (MAX_DIGITS_PIN_PREFIX), NULL, 0);
				g_assert (pdata->max_digits > 0 && pdata->max_digits < PIN_NUM_DIGITS);
			} else {
				pdata->ret_pin = g_strdup (*attr_values);
			}
			return;
		}

		++attr_names;
		++attr_values;
	}
}

char *
get_pincode_for_device (guint type, const char *address, const char *name, guint *max_digits)
{
	GMarkupParseContext *ctx;
	GMarkupParser parser = { pin_db_parse_start_tag, NULL, NULL, NULL, NULL };
	PinParseData data;
	char *buf;
	gsize buf_len;
	GError *err = NULL;

	g_return_val_if_fail (address != NULL, NULL);

	/* Load the PIN database and split it in lines */
	if (!g_file_get_contents(PIN_CODE_DB, &buf, &buf_len, NULL)) {
		char *filename;

		filename = g_build_filename(PKGDATADIR, PIN_CODE_DB, NULL);
		if (!g_file_get_contents(filename, &buf, &buf_len, NULL)) {
			g_warning("Could not load "PIN_CODE_DB);
			g_free (filename);
			return NULL;
		}
		g_free (filename);
	}

	data.ret_pin = NULL;
	data.max_digits = 0;
	data.type = type;
	data.address = address;
	data.name = name;

	ctx = g_markup_parse_context_new (&parser, 0, &data, NULL);

	if (!g_markup_parse_context_parse (ctx, buf, buf_len, &err)) {
		g_warning ("Failed to parse '%s': %s\n", PIN_CODE_DB, err->message);
		g_error_free (err);
	}

	g_markup_parse_context_free (ctx);
	g_free (buf);

	if (max_digits != NULL)
		*max_digits = data.max_digits;

	return data.ret_pin;
}

