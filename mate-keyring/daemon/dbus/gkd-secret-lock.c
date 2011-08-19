/*
 * mate-keyring
 *
 * Copyright (C) 2008 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include "gkd-secret-lock.h"
#include "gkd-secret-service.h"

#include "egg/egg-error.h"

#include "pkcs11/pkcs11i.h"

#include <gp11/gp11.h>

gboolean
gkd_secret_lock (GP11Object *collection, DBusError *derr)
{
	GError *error = NULL;
	GP11Session *session;
	GP11Object *cred;
	GList *objects, *l;

	session = gp11_object_get_session (collection);
	g_return_val_if_fail (session, FALSE);

	objects = gp11_session_find_objects (session, &error,
	                                     CKA_CLASS, GP11_ULONG, CKO_G_CREDENTIAL,
	                                     CKA_G_OBJECT, GP11_ULONG, gp11_object_get_handle (collection),
	                                     GP11_INVALID);

	if (error != NULL) {
		g_object_unref (session);
		g_warning ("couldn't search for credential objects: %s", egg_error_message (error));
		dbus_set_error (derr, DBUS_ERROR_FAILED, "Couldn't lock collection");
		g_clear_error (&error);
		return FALSE;
	}

	for (l = objects; l; l = g_list_next (l)) {
		cred = GP11_OBJECT (l->data);
		gp11_object_set_session (cred, session);
		if (!gp11_object_destroy (cred, &error)) {
			g_warning ("couldn't destroy credential object: %s", egg_error_message (error));
			g_clear_error (&error);
		}
	}

	gp11_list_unref_free (objects);
	g_object_unref (session);
	return TRUE;
}
