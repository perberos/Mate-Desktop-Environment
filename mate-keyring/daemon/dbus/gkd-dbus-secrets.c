/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* gkd-dbus-secrets.c - dbus secret service

   Copyright (C) 2009, Stefan Walter

   The Mate Keyring Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Keyring Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Stef Walter <stef@memberwebs.com>
*/

#include "config.h"

#include "gkd-dbus.h"
#include "gkd-dbus-private.h"
#include "gkd-secret-service.h"

#include "daemon/gkd-pkcs11.h"

#include "egg/egg-cleanup.h"

#include "gp11/gp11.h"

static DBusConnection *dbus_conn = NULL;
static GkdSecretService *secrets_service = NULL;

static GP11Slot*
calculate_secrets_slot (void)
{
	GP11Slot *slot = NULL;
	GP11Module *module;
	GList *slots, *l;
	GP11SlotInfo *info;

	module = gp11_module_new (gkd_pkcs11_get_functions ());
	g_return_val_if_fail (module, NULL);

	/*
	 * Find the right slot.
	 *
	 * TODO: This isn't necessarily the best way to do this.
	 * A good function could be added to gp11 library.
	 * But needs more thought on how to do this.
	 */
	slots = gp11_module_get_slots (module, TRUE);
	for (l = slots; !slot && l; l = g_list_next (l)) {
		info = gp11_slot_get_info (l->data);
		if (g_ascii_strcasecmp ("Secret Store", info->slot_description) == 0)
			slot = g_object_ref (l->data);
		gp11_slot_info_free (info);
	}
	gp11_list_unref_free (slots);

	return slot;
}

gboolean
gkd_dbus_secrets_startup (void)
{
	DBusError error = DBUS_ERROR_INIT;
	dbus_uint32_t result = 0;
	const gchar *service = NULL;
	unsigned int flags = 0;
	GP11Slot *slot;

	g_return_val_if_fail (dbus_conn, FALSE);

#ifdef WITH_TESTS
	service = g_getenv ("MATE_KEYRING_TEST_SERVICE");
	if (service && service[0])
		flags = DBUS_NAME_FLAG_ALLOW_REPLACEMENT | DBUS_NAME_FLAG_REPLACE_EXISTING;
	else
#endif
		service = SECRET_SERVICE;

	/* Figure out which slot to use */
	slot = calculate_secrets_slot ();
	g_return_val_if_fail (slot, FALSE);

	/* Try and grab our name */
	result = dbus_bus_request_name (dbus_conn, service, flags, &error);
	if (dbus_error_is_set (&error)) {
		g_message ("couldn't request name '%s' on session bus: %s",
		           service, error.message);
		dbus_error_free (&error);

	} else {
		switch (result) {

		/* We acquired the service name */
		case DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER:
			break;

		/* We already acquired the service name. */
		case DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER:
			break;

		/* Another daemon is running */
		case DBUS_REQUEST_NAME_REPLY_IN_QUEUE:
		case DBUS_REQUEST_NAME_REPLY_EXISTS:
			g_message ("another secret service is running");
			break;

		default:
			g_return_val_if_reached (FALSE);
			break;
		};
	}

	g_return_val_if_fail (!secrets_service, FALSE);
	secrets_service = g_object_new (GKD_SECRET_TYPE_SERVICE,
	                                "connection", dbus_conn, "pkcs11-slot", slot, NULL);

	g_object_unref (slot);
	return TRUE;
}

static void
cleanup_dbus_conn (gpointer unused)
{
	g_assert (dbus_conn);
	dbus_connection_unref (dbus_conn);
	dbus_conn = NULL;
}

void
gkd_dbus_secrets_init (DBusConnection *conn)
{
	dbus_conn = dbus_connection_ref (conn);
	egg_cleanup_register (cleanup_dbus_conn, NULL);
}

void
gkd_dbus_secrets_cleanup (DBusConnection *conn)
{
	if (secrets_service) {
		g_object_run_dispose (G_OBJECT (secrets_service));
		g_object_unref (secrets_service);
		secrets_service = NULL;
	}
}
