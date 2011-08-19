/*
 * mate-keyring
 *
 * Copyright (C) 2009 Stefan Walter
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

#include "gkd-login.h"

#include "daemon/gkd-pkcs11.h"

#include "egg/egg-error.h"
#include "egg/egg-secure-memory.h"

#include "pkcs11/pkcs11i.h"
#include "pkcs11/wrap-layer/gkm-wrap-layer.h"

#include <glib/gi18n.h>

#include <string.h>

static GP11Module*
module_instance (void)
{
	GP11Module *module = gp11_module_new (gkd_pkcs11_get_base_functions ());
	gp11_module_set_pool_sessions (module, FALSE);
	gp11_module_set_auto_authenticate (module, FALSE);
	g_return_val_if_fail (module, NULL);
	return module;
}

static GP11Session*
open_and_login_session (GP11Slot *slot, CK_USER_TYPE user_type, GError **error)
{
	GP11Session *session;
	GError *err = NULL;

	g_return_val_if_fail (GP11_IS_SLOT (slot), NULL);

	if (!error)
		error = &err;

	session = gp11_slot_open_session (slot, CKF_RW_SESSION, error);
	if (session != NULL) {
		if (!gp11_session_login (session, user_type, NULL, 0, error)) {
			if (g_error_matches (*error, GP11_ERROR, CKR_USER_ALREADY_LOGGED_IN)) {
				g_clear_error (error);
			} else {
				g_object_unref (session);
				session = NULL;
			}
		}
	}

	return session;
}

static GP11Session*
lookup_login_session (GP11Module *module)
{
	GP11Slot *slot = NULL;
	GError *error = NULL;
	GP11Session *session;
	GP11SlotInfo *info;
	GList *slots;
	GList *l;

	g_assert (GP11_IS_MODULE (module));

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

	g_return_val_if_fail (slot, NULL);

	session = open_and_login_session (slot, CKU_USER, &error);
	if (error) {
		g_warning ("couldn't open pkcs11 session for login: %s", egg_error_message (error));
		g_clear_error (&error);
	}

	g_object_unref (slot);

	return session;
}

static GP11Object*
lookup_login_keyring (GP11Session *session)
{
	GError *error = NULL;
	GP11Object *login = NULL;
	GList *objects;
	guint length;

	g_return_val_if_fail (GP11_IS_SESSION (session), NULL);

	objects = gp11_session_find_objects (session, &error,
	                                     CKA_CLASS, GP11_ULONG, CKO_G_COLLECTION,
	                                     CKA_TOKEN, GP11_BOOLEAN, TRUE,
	                                     CKA_ID, (gsize)5, "login",
	                                     GP11_INVALID);

	if (error) {
		g_warning ("couldn't search for login keyring: %s", egg_error_message (error));
		g_clear_error (&error);
		return NULL;
	}

	length = g_list_length (objects);
	if (length == 1) {
		login = g_object_ref (objects->data);
		gp11_object_set_session (login, session);
	} else if (length > 1) {
		g_warning ("more than one login keyring exists");
	}

	gp11_list_unref_free (objects);
	return login;
}

static GP11Object*
create_login_keyring (GP11Session *session, GP11Object *cred, GError **error)
{
	GP11Object *login;
	const gchar *label;

	g_return_val_if_fail (GP11_IS_SESSION (session), NULL);
	g_return_val_if_fail (GP11_IS_OBJECT (cred), NULL);

	/* TRANSLATORS: This is the display label for the login keyring */
	label = _("Login");

	login = gp11_session_create_object (session, error,
	                                    CKA_CLASS, GP11_ULONG, CKO_G_COLLECTION,
	                                    CKA_ID, (gsize)5, "login",
	                                    CKA_LABEL, strlen (label), label,
	                                    CKA_G_CREDENTIAL, GP11_ULONG, gp11_object_get_handle (cred),
	                                    CKA_TOKEN, GP11_BOOLEAN, TRUE,
	                                    GP11_INVALID);

	if (login != NULL)
		gp11_object_set_session (login, session);
	return login;
}

static GP11Object*
create_credential (GP11Session *session, GP11Object *object,
                   const gchar *secret, GError **error)
{
	GP11Attributes *attrs;
	GP11Object *cred;

	g_return_val_if_fail (GP11_IS_SESSION (session), NULL);
	g_return_val_if_fail (!object || GP11_IS_OBJECT (object), NULL);

	if (!secret)
		secret = "";

	attrs = gp11_attributes_newv (CKA_CLASS, GP11_ULONG, CKO_G_CREDENTIAL,
	                              CKA_VALUE, strlen (secret), secret,
	                              CKA_MATE_TRANSIENT, GP11_BOOLEAN, TRUE,
	                              CKA_TOKEN, GP11_BOOLEAN, TRUE,
	                              GP11_INVALID);

	if (object)
		gp11_attributes_add_ulong (attrs, CKA_G_OBJECT,
		                           gp11_object_get_handle (object));

	cred = gp11_session_create_object_full (session, attrs, NULL, error);
	gp11_attributes_unref (attrs);

	if (cred != NULL)
		gp11_object_set_session (cred, session);

	return cred;
}

static gboolean
unlock_or_create_login (GP11Module *module, const gchar *master)
{
	GError *error = NULL;
	GP11Session *session;
	GP11Object *login;
	GP11Object *cred;

	g_return_val_if_fail (GP11_IS_MODULE (module), FALSE);
	g_return_val_if_fail (master, FALSE);

	/* Find the login object */
	session = lookup_login_session (module);
	login = lookup_login_keyring (session);

	/* Create credentials for login object */
	cred = create_credential (session, login, master, &error);

	/* Failure, bad password? */
	if (cred == NULL) {
		if (login && g_error_matches (error, GP11_ERROR, CKR_PIN_INCORRECT))
			gkm_wrap_layer_hint_login_unlock_failure ();
		else
			g_warning ("couldn't create login credential: %s", egg_error_message (error));
		g_clear_error (&error);

	/* Non login keyring, create it */
	} else if (!login) {
		login = create_login_keyring (session, cred, &error);
		if (login == NULL && error) {
			g_warning ("couldn't create login keyring: %s", egg_error_message (error));
			g_clear_error (&error);
		}

	/* The unlock succeeded yay */
	} else {
		gkm_wrap_layer_hint_login_unlock_success ();
	}

	if (cred)
		g_object_unref (cred);
	if (login)
		g_object_unref (login);
	if (session)
		g_object_unref (session);

	return cred && login;
}

static gboolean
init_pin_for_uninitialized_slots (GP11Module *module, const gchar *master)
{
	GError *error = NULL;
	GList *slots, *l;
	gboolean initialize;
	GP11TokenInfo *info;
	GP11Session *session;

	g_return_val_if_fail (GP11_IS_MODULE (module), FALSE);
	g_return_val_if_fail (master, FALSE);

	slots = gp11_module_get_slots (module, TRUE);
	for (l = slots; l; l = g_list_next (l)) {
		info = gp11_slot_get_token_info (l->data);
		initialize = (info && !(info->flags & CKF_USER_PIN_INITIALIZED));

		if (initialize) {
			session = open_and_login_session (l->data, CKU_SO, NULL);
			if (session != NULL) {
				if (!gp11_session_init_pin (session, (const guchar*)master, strlen (master), &error)) {
					if (!g_error_matches (error, GP11_ERROR, CKR_FUNCTION_NOT_SUPPORTED))
						g_warning ("couldn't initialize slot with master password: %s",
						           egg_error_message (error));
					g_clear_error (&error);
				}
				g_object_unref (session);
			}
		}

		gp11_token_info_free (info);
	}
	gp11_list_unref_free (slots);
	return TRUE;
}

gboolean
gkd_login_unlock (const gchar *master)
{
	GP11Module *module;
	gboolean result;

	/* We don't support null or empty master passwords */
	if (!master || !master[0])
		return FALSE;

	module = module_instance ();

	result = unlock_or_create_login (module, master);
	if (result == TRUE)
		init_pin_for_uninitialized_slots (module, master);

	g_object_unref (module);
	return result;
}

static gboolean
change_or_create_login (GP11Module *module, const gchar *original, const gchar *master)
{
	GError *error = NULL;
	GP11Session *session;
	GP11Object *login = NULL;
	GP11Object *ocred = NULL;
	GP11Object *mcred = NULL;
	gboolean success = FALSE;

	g_return_val_if_fail (GP11_IS_MODULE (module), FALSE);
	g_return_val_if_fail (original, FALSE);
	g_return_val_if_fail (master, FALSE);

	/* Find the login object */
	session = lookup_login_session (module);
	login = lookup_login_keyring (session);

	/* Create the new credential we'll be changing to */
	mcred = create_credential (session, NULL, master, &error);
	if (mcred == NULL) {
		g_warning ("couldn't create new login credential: %s", egg_error_message (error));
		g_clear_error (&error);

	/* Create original credentials */
	} else if (login) {
		ocred = create_credential (session, login, original, &error);
		if (ocred == NULL) {
			if (g_error_matches (error, GP11_ERROR, CKR_PIN_INCORRECT)) {
				g_message ("couldn't change login master password, "
				           "original password was wrong: %s",
				           egg_error_message (error));
				gkm_wrap_layer_hint_login_unlock_failure ();
			} else {
				g_warning ("couldn't create original login credential: %s",
				           egg_error_message (error));
			}
			g_clear_error (&error);
		}
	}

	/* No keyring? try to create */
	if (!login && mcred) {
		login = create_login_keyring (session, mcred, &error);
		if (login == NULL) {
			g_warning ("couldn't create login keyring: %s", egg_error_message (error));
			g_clear_error (&error);
		} else {
			success = TRUE;
		}

	/* Change the master password */
	} else if (login && ocred && mcred) {
		if (!gp11_object_set (login, &error,
		                      CKA_G_CREDENTIAL, GP11_ULONG, gp11_object_get_handle (mcred),
		                      GP11_INVALID)) {
			g_warning ("couldn't change login master password: %s", egg_error_message (error));
			g_clear_error (&error);
		} else {
			success = TRUE;
		}
	}

	if (ocred) {
		gp11_object_destroy (ocred, NULL);
		g_object_unref (ocred);
	}
	if (mcred)
		g_object_unref (mcred);
	if (login)
		g_object_unref (login);
	if (session)
		g_object_unref (session);

	return success;
}

static gboolean
set_pin_for_any_slots (GP11Module *module, const gchar *original, const gchar *master)
{
	GError *error = NULL;
	GList *slots, *l;
	gboolean initialize;
	GP11TokenInfo *info;
	GP11Session *session;

	g_return_val_if_fail (GP11_IS_MODULE (module), FALSE);
	g_return_val_if_fail (original, FALSE);
	g_return_val_if_fail (master, FALSE);

	slots = gp11_module_get_slots (module, TRUE);
	for (l = slots; l; l = g_list_next (l)) {

		/* Set pin for any that are initialized, and not pap */
		info = gp11_slot_get_token_info (l->data);
		initialize = (info && (info->flags & CKF_USER_PIN_INITIALIZED));

		if (initialize) {
			session = open_and_login_session (l->data, CKU_USER, NULL);
			if (session != NULL) {
				if (!gp11_session_set_pin (session, (const guchar*)original, strlen (original),
				                           (const guchar*)master, strlen (master), &error)) {
					if (!g_error_matches (error, GP11_ERROR, CKR_PIN_INCORRECT) &&
					    !g_error_matches (error, GP11_ERROR, CKR_FUNCTION_NOT_SUPPORTED))
						g_warning ("couldn't change slot master password: %s",
						           egg_error_message (error));
					g_clear_error (&error);
				}
				g_object_unref (session);
			}
		}

		gp11_token_info_free (info);
	}
	gp11_list_unref_free (slots);
	return TRUE;
}

gboolean
gkd_login_change_lock (const gchar *original, const gchar *master)
{
	GP11Module *module;
	gboolean result;

	/* We don't support null or empty master passwords */
	if (!master || !master[0])
		return FALSE;
	if (original == NULL)
		original = "";

	module = module_instance ();

	result = change_or_create_login (module, original, master);
	if (result == TRUE)
		set_pin_for_any_slots (module, original, master);

	g_object_unref (module);
	return result;
}
