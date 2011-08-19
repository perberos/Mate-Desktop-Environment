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

#include "gkd-dbus-util.h"

#include "gkd-secret-error.h"
#include "gkd-secret-objects.h"
#include "gkd-secret-property.h"
#include "gkd-secret-secret.h"
#include "gkd-secret-service.h"
#include "gkd-secret-session.h"
#include "gkd-secret-types.h"
#include "gkd-secret-util.h"

#include "egg/egg-error.h"

#include "pkcs11/pkcs11i.h"

#include <string.h>

enum {
	PROP_0,
	PROP_PKCS11_SLOT,
	PROP_SERVICE
};

struct _GkdSecretObjects {
	GObject parent;
	GkdSecretService *service;
	GP11Slot *pkcs11_slot;
	GHashTable *aliases;
};

G_DEFINE_TYPE (GkdSecretObjects, gkd_secret_objects, G_TYPE_OBJECT);

/* -----------------------------------------------------------------------------
 * INTERNAL
 */

static gboolean
parse_object_path (GkdSecretObjects *self, const gchar *path, gchar **collection, gchar **item)
{
	const gchar *replace;

	g_assert (self);
	g_assert (path);
	g_assert (collection);

	if (!gkd_secret_util_parse_path (path, collection, item))
		return FALSE;

	if (g_str_has_prefix (path, SECRET_ALIAS_PREFIX)) {
		replace = g_hash_table_lookup (self->aliases, *collection);
		if (!replace) {

			/*
			 * TODO: As a special case, always treat login keyring as
			 * default. This logic should be moved, once we have better
			 * support for aliases.
			 */

			if (g_str_equal (*collection, "default")) {
				replace = "login";

			/* No such alias, return nothing */
			} else {
				g_free (*collection);
				*collection = NULL;
				if (item) {
					g_free (*item);
					*item = NULL;
				}
				return FALSE;
			}
		}
		g_free (*collection);
		*collection = g_strdup (replace);
	}

	return TRUE;
}

static void
iter_append_item_path (const gchar *base, GP11Object *object, DBusMessageIter *iter)
{
	GError *error = NULL;
	gpointer identifier;
	gsize n_identifier;
	gchar *path;
	gchar *alloc = NULL;

	if (base == NULL) {
		identifier = gp11_object_get_data (object, CKA_G_COLLECTION, &n_identifier, &error);
		if (!identifier) {
			g_warning ("couldn't get item collection identifier: %s", egg_error_message (error));
			g_clear_error (&error);
			return;
		}

		base = alloc = gkd_secret_util_build_path (SECRET_COLLECTION_PREFIX, identifier, n_identifier);
		g_free (identifier);
	}

	identifier = gp11_object_get_data (object, CKA_ID, &n_identifier, &error);
	if (identifier == NULL) {
		g_warning ("couldn't get item identifier: %s", egg_error_message (error));
		g_clear_error (&error);
	} else {
		path = gkd_secret_util_build_path (base, identifier, n_identifier);
		g_free (identifier);
		dbus_message_iter_append_basic (iter, DBUS_TYPE_OBJECT_PATH, &path);
		g_free (path);
	}

	g_free (alloc);
}

static void
iter_append_item_paths (const gchar *base, GList *items, DBusMessageIter *iter)
{
	DBusMessageIter array;
	GList *l;

	dbus_message_iter_open_container (iter, DBUS_TYPE_ARRAY, "o", &array);

	for (l = items; l; l = g_list_next (l))
		iter_append_item_path (base, l->data, &array);

	dbus_message_iter_close_container (iter, &array);
}

static void
iter_append_collection_paths (GList *collections, DBusMessageIter *iter)
{
	DBusMessageIter array;
	gpointer identifier;
	gsize n_identifier;
	GError *error = NULL;
	gchar *path;
	GList *l;

	dbus_message_iter_open_container (iter, DBUS_TYPE_ARRAY, "o", &array);

	for (l = collections; l; l = g_list_next (l)) {

		identifier = gp11_object_get_data (l->data, CKA_ID, &n_identifier, &error);
		if (identifier == NULL) {
			g_warning ("couldn't get collection identifier: %s", egg_error_message (error));
			g_clear_error (&error);
			continue;
		}

		path = gkd_secret_util_build_path (SECRET_COLLECTION_PREFIX, identifier, n_identifier);
		g_free (identifier);

		dbus_message_iter_append_basic (&array, DBUS_TYPE_OBJECT_PATH, &path);
		g_free (path);
	}

	dbus_message_iter_close_container (iter, &array);
}


static DBusMessage*
object_property_get (GP11Object *object, DBusMessage *message,
                     const gchar *prop_name)
{
	DBusMessageIter iter;
	GError *error = NULL;
	DBusMessage *reply;
	GP11Attribute attr;
	gsize length;

	if (!gkd_secret_property_get_type (prop_name, &attr.type))
		return dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                      "Object does not have the '%s' property", prop_name);

	/* Retrieve the actual attribute */
	attr.value = gp11_object_get_data (object, attr.type, &length, &error);
	if (error != NULL) {
		reply = dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                       "Couldn't retrieve '%s' property: %s",
		                                       prop_name, egg_error_message (error));
		g_clear_error (&error);
		return reply;
	}

	/* Marshall the data back out */
	attr.length = length;
	reply = dbus_message_new_method_return (message);
	dbus_message_iter_init_append (reply, &iter);
	gkd_secret_property_append_variant (&iter, &attr);
	g_free (attr.value);
	return reply;
}

static DBusMessage*
object_property_set (GP11Object *object, DBusMessage *message,
                     DBusMessageIter *iter, const gchar *prop_name)
{
	DBusMessage *reply;
	GP11Attributes *attrs;
	GP11Attribute *attr;
	GError *error = NULL;
	gulong attr_type;

	g_return_val_if_fail (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_VARIANT, NULL);

	/* What type of property is it? */
	if (!gkd_secret_property_get_type (prop_name, &attr_type))
		return dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                      "Object does not have the '%s' property", prop_name);

	attrs = gp11_attributes_new ();
	gp11_attributes_add_empty (attrs, attr_type);
	attr = gp11_attributes_at (attrs, 0);

	/* Retrieve the actual attribute value */
	if (!gkd_secret_property_parse_variant (iter, prop_name, attr)) {
		gp11_attributes_unref (attrs);
		return dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                      "The property type or value was invalid: %s", prop_name);
	}

	gp11_object_set_full (object, attrs, NULL, &error);
	gp11_attributes_unref (attrs);

	if (error != NULL) {
		if (g_error_matches (error, GP11_ERROR, CKR_USER_NOT_LOGGED_IN))
			reply = dbus_message_new_error (message, SECRET_ERROR_IS_LOCKED,
			                                "Cannot set property on a locked object");
		else
			reply = dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
			                                       "Couldn't set '%s' property: %s",
			                                       prop_name, egg_error_message (error));
		g_clear_error (&error);
		return reply;
	}

	return dbus_message_new_method_return (message);
}

static DBusMessage*
item_property_get (GP11Object *object, DBusMessage *message)
{
	const gchar *interface;
	const gchar *name;

	if (!dbus_message_get_args (message, NULL, DBUS_TYPE_STRING, &interface,
	                            DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
		return NULL;

	if (!gkd_dbus_interface_match (SECRET_ITEM_INTERFACE, interface))
		return dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                      "Object does not have properties on interface '%s'",
		                                      interface);

	return object_property_get (object, message, name);
}

static DBusMessage*
item_property_set (GP11Object *object, DBusMessage *message)
{
	DBusMessageIter iter;
	const char *interface;
	const char *name;

	if (!dbus_message_has_signature (message, "ssv"))
		return NULL;

	dbus_message_iter_init (message, &iter);
	dbus_message_iter_get_basic (&iter, &interface);
	dbus_message_iter_next (&iter);
	dbus_message_iter_get_basic (&iter, &name);
	dbus_message_iter_next (&iter);

	if (!gkd_dbus_interface_match (SECRET_ITEM_INTERFACE, interface))
		return dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                      "Object does not have properties on interface '%s'",
		                                      interface);

	return object_property_set (object, message, &iter, name);
}

static DBusMessage*
item_property_getall (GP11Object *object, DBusMessage *message)
{
	GP11Attributes *attrs;
	DBusMessageIter iter;
	DBusMessageIter array;
	GError *error = NULL;
	DBusMessage *reply;
	const gchar *interface;

	if (!dbus_message_get_args (message, NULL, DBUS_TYPE_STRING, &interface, DBUS_TYPE_INVALID))
		return NULL;

	if (!gkd_dbus_interface_match (SECRET_ITEM_INTERFACE, interface))
		return dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                      "Object does not have properties on interface '%s'",
		                                      interface);

	attrs = gp11_object_get (object, &error,
	                         CKA_LABEL,
	                         CKA_G_SCHEMA,
	                         CKA_G_LOCKED,
	                         CKA_G_CREATED,
	                         CKA_G_MODIFIED,
	                         CKA_G_FIELDS,
	                         GP11_INVALID);

	if (error != NULL)
		return dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                      "Couldn't retrieve properties: %s",
		                                      egg_error_message (error));

	reply = dbus_message_new_method_return (message);

	dbus_message_iter_init_append (reply, &iter);
	dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "{sv}", &array);
	gkd_secret_property_append_all (&array, attrs);
	dbus_message_iter_close_container (&iter, &array);
	return reply;
}

static DBusMessage*
item_method_delete (GkdSecretObjects *self, GP11Object *object, DBusMessage *message)
{
	GError *error = NULL;
	DBusMessage *reply;
	const gchar *prompt;

	if (!dbus_message_get_args (message, NULL, DBUS_TYPE_INVALID))
		return NULL;

	if (!gp11_object_destroy (object, &error)) {
		if (g_error_matches (error, GP11_ERROR, CKR_USER_NOT_LOGGED_IN))
			reply = dbus_message_new_error_printf (message, SECRET_ERROR_IS_LOCKED,
			                                       "Cannot delete a locked item");
		else
			reply = dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
			                                       "Couldn't delete collection: %s",
			                                       egg_error_message (error));
		g_clear_error (&error);
		return reply;
	}

	prompt = "/"; /* No prompt necessary */
	reply = dbus_message_new_method_return (message);
	dbus_message_append_args (reply, DBUS_TYPE_OBJECT_PATH, &prompt, DBUS_TYPE_INVALID);
	return reply;
}

static DBusMessage*
item_method_get_secret (GkdSecretObjects *self, GP11Object *item, DBusMessage *message)
{
	DBusError derr = DBUS_ERROR_INIT;
	GkdSecretSession *session;
	GkdSecretSecret *secret;
	DBusMessage *reply;
	DBusMessageIter iter;
	const char *path;

	if (!dbus_message_get_args (message, NULL, DBUS_TYPE_OBJECT_PATH, &path, DBUS_TYPE_INVALID))
		return NULL;

	session = gkd_secret_service_lookup_session (self->service, path, dbus_message_get_sender (message));
	if (session == NULL)
		return dbus_message_new_error (message, SECRET_ERROR_NO_SESSION, "The session does not exist");

	secret = gkd_secret_session_get_item_secret (session, item, &derr);
	if (secret == NULL)
		return gkd_secret_error_to_reply (message, &derr);

	reply = dbus_message_new_method_return (message);
	dbus_message_iter_init_append (reply, &iter);
	gkd_secret_secret_append (secret, &iter);
	gkd_secret_secret_free (secret);
	return reply;
}

static DBusMessage*
item_method_set_secret (GkdSecretObjects *self, GP11Object *item, DBusMessage *message)
{
	DBusError derr = DBUS_ERROR_INIT;
	DBusMessageIter iter;
	GkdSecretSecret *secret;
	const char *caller;

	if (!dbus_message_has_signature (message, "(oayay)"))
		return NULL;
	dbus_message_iter_init (message, &iter);
	secret = gkd_secret_secret_parse (self->service, message, &iter, &derr);
	if (secret == NULL)
		return gkd_secret_error_to_reply (message, &derr);

	caller = dbus_message_get_sender (message);
	g_return_val_if_fail (caller, NULL);

	gkd_secret_session_set_item_secret (secret->session, item, secret, &derr);
	gkd_secret_secret_free (secret);

	if (dbus_error_is_set (&derr))
		return gkd_secret_error_to_reply (message, &derr);

	return dbus_message_new_method_return (message);
}

static DBusMessage*
item_message_handler (GkdSecretObjects *self, GP11Object *object, DBusMessage *message)
{
	/* org.freedesktop.Secrets.Item.Delete() */
	if (dbus_message_is_method_call (message, SECRET_ITEM_INTERFACE, "Delete"))
		return item_method_delete (self, object, message);

	/* org.freedesktop.Secrets.Session.GetSecret() */
	else if (dbus_message_is_method_call (message, SECRET_ITEM_INTERFACE, "GetSecret"))
		return item_method_get_secret (self, object, message);

	/* org.freedesktop.Secrets.Session.SetSecret() */
	else if (dbus_message_is_method_call (message, SECRET_ITEM_INTERFACE, "SetSecret"))
		return item_method_set_secret (self, object, message);

	/* org.freedesktop.DBus.Properties.Get */
	if (dbus_message_is_method_call (message, DBUS_INTERFACE_PROPERTIES, "Get"))
		return item_property_get (object, message);

	/* org.freedesktop.DBus.Properties.Set */
	else if (dbus_message_is_method_call (message, DBUS_INTERFACE_PROPERTIES, "Set"))
		return item_property_set (object, message);

	/* org.freedesktop.DBus.Properties.GetAll */
	else if (dbus_message_is_method_call (message, DBUS_INTERFACE_PROPERTIES, "GetAll"))
		return item_property_getall (object, message);

	else if (dbus_message_has_interface (message, DBUS_INTERFACE_INTROSPECTABLE))
		return gkd_dbus_introspect_handle (message, "item");

	return NULL;
}

static void
item_cleanup_search_results (GP11Session *session, GList *items,
                             GList **locked, GList **unlocked)
{
	GError *error = NULL;
	gpointer value;
	gsize n_value;
	GList *l;

	*locked = NULL;
	*unlocked = NULL;

	for (l = items; l; l = g_list_next (l)) {

		gp11_object_set_session (l->data, session);
		value = gp11_object_get_data (l->data, CKA_G_LOCKED, &n_value, &error);
		if (value == NULL) {
			if (!g_error_matches (error, GP11_ERROR, CKR_OBJECT_HANDLE_INVALID))
				g_warning ("couldn't check if item is locked: %s", egg_error_message (error));
			g_clear_error (&error);

		/* Is not locked */
		} if (n_value == 1 && *((CK_BBOOL*)value) == CK_FALSE) {
			*unlocked = g_list_prepend (*unlocked, l->data);

		/* Is locked */
		} else {
			*locked = g_list_prepend (*locked, l->data);
		}

		g_free (value);
	}
}

static DBusMessage*
collection_property_get (GkdSecretObjects *self, GP11Object *object, DBusMessage *message)
{
	DBusMessageIter iter;
	DBusMessage *reply;
	const gchar *interface;
	const gchar *name;

	if (!dbus_message_get_args (message, NULL, DBUS_TYPE_STRING, &interface,
	                            DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
		return NULL;

	if (!gkd_dbus_interface_match (SECRET_COLLECTION_INTERFACE, interface))
		return dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                      "Object does not have properties on interface '%s'",
		                                      interface);

	/* Special case, the Items property */
	if (g_str_equal (name, "Items")) {
		reply = dbus_message_new_method_return (message);
		dbus_message_iter_init_append (reply, &iter);
		gkd_secret_objects_append_item_paths (self, dbus_message_get_path (message), &iter, message);
		return reply;
	}

	return object_property_get (object, message, name);
}

static DBusMessage*
collection_property_set (GkdSecretObjects *self, GP11Object *object, DBusMessage *message)
{
	DBusMessageIter iter;
	const char *interface;
	const char *name;

	if (!dbus_message_has_signature (message, "ssv"))
		return NULL;

	dbus_message_iter_init (message, &iter);
	dbus_message_iter_get_basic (&iter, &interface);
	dbus_message_iter_next (&iter);
	dbus_message_iter_get_basic (&iter, &name);
	dbus_message_iter_next (&iter);

	if (!gkd_dbus_interface_match (SECRET_COLLECTION_INTERFACE, interface))
		return dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                      "Object does not have properties on interface '%s'",
		                                      interface);

	return object_property_set (object, message, &iter, name);
}

static DBusMessage*
collection_property_getall (GkdSecretObjects *self, GP11Object *object, DBusMessage *message)
{
	GP11Attributes *attrs;
	DBusMessageIter iter;
	DBusMessageIter array;
	DBusMessageIter dict;
	GError *error = NULL;
	DBusMessage *reply;
	const gchar *name;
	const gchar *interface;

	if (!dbus_message_get_args (message, NULL, DBUS_TYPE_STRING, &interface, DBUS_TYPE_INVALID))
		return NULL;

	if (!gkd_dbus_interface_match (SECRET_COLLECTION_INTERFACE, interface))
		return dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                      "Object does not have properties on interface '%s'",
		                                      interface);

	attrs = gp11_object_get (object, &error,
	                         CKA_LABEL,
	                         CKA_G_LOCKED,
	                         CKA_G_CREATED,
	                         CKA_G_MODIFIED,
	                         GP11_INVALID);

	if (error != NULL)
		return dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                      "Couldn't retrieve properties: %s",
		                                      egg_error_message (error));

	reply = dbus_message_new_method_return (message);

	dbus_message_iter_init_append (reply, &iter);
	dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "{sv}", &array);

	/* Append all the usual properties */
	gkd_secret_property_append_all (&array, attrs);

	/* Append the Items property */
	dbus_message_iter_open_container (&array, DBUS_TYPE_DICT_ENTRY, NULL, &dict);
	name = "Items";
	dbus_message_iter_append_basic (&dict, DBUS_TYPE_STRING, &name);
	gkd_secret_objects_append_item_paths (self, dbus_message_get_path (message), &dict, message);
	dbus_message_iter_close_container (&array, &dict);

	dbus_message_iter_close_container (&iter, &array);
	return reply;
}

static DBusMessage*
collection_method_search_items (GkdSecretObjects *self, GP11Object *object, DBusMessage *message)
{
	return gkd_secret_objects_handle_search_items (self, message, dbus_message_get_path (message));
}

static GP11Object*
collection_find_matching_item (GkdSecretObjects *self, GP11Session *session,
                               const gchar *identifier, GP11Attribute *fields)
{
	GP11Attributes *attrs;
	GP11Object *result = NULL;
	GError *error = NULL;
	GP11Object *search;
	gpointer data;
	gsize n_data;

	/* Find items matching the collection and fields */
	attrs = gp11_attributes_new ();
	gp11_attributes_add (attrs, fields);
	gp11_attributes_add_string (attrs, CKA_G_COLLECTION, identifier);
	gp11_attributes_add_ulong (attrs, CKA_CLASS, CKO_G_SEARCH);
	gp11_attributes_add_boolean (attrs, CKA_TOKEN, FALSE);

	/* Create the search object */
	search = gp11_session_create_object_full (session, attrs, NULL, &error);
	gp11_attributes_unref (attrs);

	if (error != NULL) {
		g_warning ("couldn't search for matching item: %s", egg_error_message (error));
		g_clear_error (&error);
		return NULL;
	}

	/* Get the matched item handles, and delete the search object */
	gp11_object_set_session (search, session);
	data = gp11_object_get_data (search, CKA_G_MATCHED, &n_data, NULL);
	gp11_object_destroy (search, NULL);
	g_object_unref (search);

	if (n_data >= sizeof (CK_OBJECT_HANDLE)) {
		result = gp11_object_from_handle (gp11_session_get_slot (session),
		                                  *((CK_OBJECT_HANDLE_PTR)data));
		gp11_object_set_session (result, session);
	}

	g_free (data);
	return result;
}

static DBusMessage*
collection_method_create_item (GkdSecretObjects *self, GP11Object *object, DBusMessage *message)
{
	GP11Session *pkcs11_session = NULL;
	DBusError derr = DBUS_ERROR_INIT;
	GkdSecretSecret *secret = NULL;
	dbus_bool_t replace = FALSE;
	GP11Attributes *attrs = NULL;
	GP11Attribute *fields;
	DBusMessageIter iter, array;
	GP11Object *item = NULL;
	const gchar *prompt;
	const gchar *base;
	GError *error = NULL;
	DBusMessage *reply = NULL;
	gchar *path = NULL;
	gchar *identifier;
	gboolean created = FALSE;

	/* Parse the message */
	if (!dbus_message_has_signature (message, "a{sv}(oayay)b"))
		return NULL;
	if (!dbus_message_iter_init (message, &iter))
		g_return_val_if_reached (NULL);
	attrs = gp11_attributes_new ();
	dbus_message_iter_recurse (&iter, &array);
	if (!gkd_secret_property_parse_all (&array, attrs)) {
		reply = dbus_message_new_error (message, DBUS_ERROR_INVALID_ARGS,
		                                "Invalid properties argument");
		goto cleanup;
	}
	dbus_message_iter_next (&iter);
	secret = gkd_secret_secret_parse (self->service, message, &iter, &derr);
	if (secret == NULL) {
		reply = gkd_secret_error_to_reply (message, &derr);
		goto cleanup;
	}
	dbus_message_iter_next (&iter);
	dbus_message_iter_get_basic (&iter, &replace);

	base = dbus_message_get_path (message);
	if (!parse_object_path (self, base, &identifier, NULL))
		g_return_val_if_reached (NULL);
	g_return_val_if_fail (identifier, NULL);

	pkcs11_session = gp11_object_get_session (object);
	g_return_val_if_fail (pkcs11_session, NULL);

	if (replace) {
		fields = gp11_attributes_find (attrs, CKA_G_FIELDS);
		if (fields)
			item = collection_find_matching_item (self, pkcs11_session, identifier, fields);
	}

	/* Replace the item */
	if (item) {
		if (!gp11_object_set_full (item, attrs, NULL, &error))
			goto cleanup;

	/* Create a new item */
	} else {
		gp11_attributes_add_string (attrs, CKA_G_COLLECTION, identifier);
		gp11_attributes_add_ulong (attrs, CKA_CLASS, CKO_SECRET_KEY);
		item = gp11_session_create_object_full (pkcs11_session, attrs, NULL, &error);
		if (item == NULL)
			goto cleanup;
		gp11_object_set_session (item, pkcs11_session);
		created = TRUE;
	}

	/* Set the secret */
	if (!gkd_secret_session_set_item_secret (secret->session, item, secret, &derr)) {
		if (created) /* If we created, then try to destroy on failure */
			gp11_object_destroy (item, NULL);
		goto cleanup;
	}

	/* Build up the item identifier */
	reply = dbus_message_new_method_return (message);
	dbus_message_iter_init_append (reply, &iter);
	iter_append_item_path (base, item, &iter);
	prompt = "/";
	dbus_message_iter_append_basic (&iter, DBUS_TYPE_OBJECT_PATH, &prompt);

cleanup:
	if (error) {
		if (!reply) {
			if (g_error_matches (error, GP11_ERROR, CKR_USER_NOT_LOGGED_IN))
				reply = dbus_message_new_error_printf (message, SECRET_ERROR_IS_LOCKED,
				                                       "Cannot create an item in a locked collection");
			else
				reply = dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
				                                       "Couldn't create item: %s", egg_error_message (error));
		}
		g_clear_error (&error);
	}

	if (dbus_error_is_set (&derr)) {
		if (!reply)
			reply = dbus_message_new_error (message, derr.name, derr.message);
		dbus_error_free (&derr);
	}

	gkd_secret_secret_free (secret);
	gp11_attributes_unref (attrs);
	if (item)
		g_object_unref (item);
	if (pkcs11_session)
		g_object_unref (pkcs11_session);
	g_free (path);

	return reply;
}

static DBusMessage*
collection_method_delete (GkdSecretObjects *self, GP11Object *object, DBusMessage *message)
{
	GError *error = NULL;
	DBusMessage *reply;
	const gchar *prompt;

	if (!dbus_message_get_args (message, NULL, DBUS_TYPE_INVALID))
		return NULL;

	if (!gp11_object_destroy (object, &error)) {
		reply = dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                       "Couldn't delete collection: %s",
		                                       egg_error_message (error));
		g_clear_error (&error);
		return reply;
	}

	prompt = "/";
	reply = dbus_message_new_method_return (message);
	dbus_message_append_args (reply, DBUS_TYPE_OBJECT_PATH, &prompt, DBUS_TYPE_INVALID);
	return reply;
}

static DBusMessage*
collection_message_handler (GkdSecretObjects *self, GP11Object *object, DBusMessage *message)
{
	/* org.freedesktop.Secrets.Collection.Delete() */
	if (dbus_message_is_method_call (message, SECRET_COLLECTION_INTERFACE, "Delete"))
		return collection_method_delete (self, object, message);

	/* org.freedesktop.Secrets.Collection.SearchItems() */
	if (dbus_message_is_method_call (message, SECRET_COLLECTION_INTERFACE, "SearchItems"))
		return collection_method_search_items (self, object, message);

	/* org.freedesktop.Secrets.Collection.CreateItem() */
	if (dbus_message_is_method_call (message, SECRET_COLLECTION_INTERFACE, "CreateItem"))
		return collection_method_create_item (self, object, message);

	/* org.freedesktop.DBus.Properties.Get() */
	if (dbus_message_is_method_call (message, DBUS_INTERFACE_PROPERTIES, "Get"))
		return collection_property_get (self, object, message);

	/* org.freedesktop.DBus.Properties.Set() */
	else if (dbus_message_is_method_call (message, DBUS_INTERFACE_PROPERTIES, "Set"))
		return collection_property_set (self, object, message);

	/* org.freedesktop.DBus.Properties.GetAll() */
	else if (dbus_message_is_method_call (message, DBUS_INTERFACE_PROPERTIES, "GetAll"))
		return collection_property_getall (self, object, message);

	else if (dbus_message_has_interface (message, DBUS_INTERFACE_INTROSPECTABLE))
		return gkd_dbus_introspect_handle (message, "collection");

	return NULL;
}

/* -----------------------------------------------------------------------------
 * OBJECT
 */

static GObject*
gkd_secret_objects_constructor (GType type, guint n_props, GObjectConstructParam *props)
{
	GkdSecretObjects *self = GKD_SECRET_OBJECTS (G_OBJECT_CLASS (gkd_secret_objects_parent_class)->constructor(type, n_props, props));

	g_return_val_if_fail (self, NULL);
	g_return_val_if_fail (self->pkcs11_slot, NULL);
	g_return_val_if_fail (self->service, NULL);

	return G_OBJECT (self);
}

static void
gkd_secret_objects_init (GkdSecretObjects *self)
{
	self->aliases = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
}

static void
gkd_secret_objects_dispose (GObject *obj)
{
	GkdSecretObjects *self = GKD_SECRET_OBJECTS (obj);

	if (self->pkcs11_slot) {
		g_object_unref (self->pkcs11_slot);
		self->pkcs11_slot = NULL;
	}

	if (self->service) {
		g_object_remove_weak_pointer (G_OBJECT (self->service),
		                              (gpointer*)&(self->service));
		self->service = NULL;
	}

	G_OBJECT_CLASS (gkd_secret_objects_parent_class)->dispose (obj);
}

static void
gkd_secret_objects_finalize (GObject *obj)
{
	GkdSecretObjects *self = GKD_SECRET_OBJECTS (obj);

	g_hash_table_destroy (self->aliases);
	g_assert (!self->pkcs11_slot);
	g_assert (!self->service);

	G_OBJECT_CLASS (gkd_secret_objects_parent_class)->finalize (obj);
}

static void
gkd_secret_objects_set_property (GObject *obj, guint prop_id, const GValue *value,
                                 GParamSpec *pspec)
{
	GkdSecretObjects *self = GKD_SECRET_OBJECTS (obj);

	switch (prop_id) {
	case PROP_PKCS11_SLOT:
		g_return_if_fail (!self->pkcs11_slot);
		self->pkcs11_slot = g_value_dup_object (value);
		g_return_if_fail (self->pkcs11_slot);
		break;
	case PROP_SERVICE:
		g_return_if_fail (!self->service);
		self->service = g_value_get_object (value);
		g_return_if_fail (self->service);
		g_object_add_weak_pointer (G_OBJECT (self->service),
		                           (gpointer*)&(self->service));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
gkd_secret_objects_get_property (GObject *obj, guint prop_id, GValue *value,
                                     GParamSpec *pspec)
{
	GkdSecretObjects *self = GKD_SECRET_OBJECTS (obj);

	switch (prop_id) {
	case PROP_PKCS11_SLOT:
		g_value_set_object (value, gkd_secret_objects_get_pkcs11_slot (self));
		break;
	case PROP_SERVICE:
		g_value_set_object (value, self->service);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
		break;
	}
}

static void
gkd_secret_objects_class_init (GkdSecretObjectsClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->constructor = gkd_secret_objects_constructor;
	gobject_class->dispose = gkd_secret_objects_dispose;
	gobject_class->finalize = gkd_secret_objects_finalize;
	gobject_class->set_property = gkd_secret_objects_set_property;
	gobject_class->get_property = gkd_secret_objects_get_property;

	g_object_class_install_property (gobject_class, PROP_PKCS11_SLOT,
	        g_param_spec_object ("pkcs11-slot", "Pkcs11 Slot", "PKCS#11 slot that we use for secrets",
	                             GP11_TYPE_SLOT, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (gobject_class, PROP_SERVICE,
		g_param_spec_object ("service", "Service", "Service which owns this objects",
		                     GKD_SECRET_TYPE_SERVICE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

/* -----------------------------------------------------------------------------
 * PUBLIC
 */

GP11Slot*
gkd_secret_objects_get_pkcs11_slot (GkdSecretObjects *self)
{
	g_return_val_if_fail (GKD_SECRET_IS_OBJECTS (self), NULL);
	return self->pkcs11_slot;
}

DBusMessage*
gkd_secret_objects_dispatch (GkdSecretObjects *self, DBusMessage *message)
{
	DBusMessage *reply = NULL;
	GError *error = NULL;
	GList *objects;
	GP11Session *session;
	gchar *c_ident;
	gchar *i_ident;
	gboolean is_item;
	const char *path;

	g_return_val_if_fail (GKD_SECRET_IS_OBJECTS (self), NULL);
	g_return_val_if_fail (message, NULL);

	path = dbus_message_get_path (message);
	g_return_val_if_fail (path, NULL);

	if (!parse_object_path (self, path, &c_ident, &i_ident) || !c_ident)
		return gkd_secret_error_no_such_object (message);

	/* The session we're using to access the object */
	session = gkd_secret_service_get_pkcs11_session (self->service, dbus_message_get_sender (message));
	g_return_val_if_fail (session, NULL);

	if (i_ident) {
		is_item = TRUE;
		objects = gp11_session_find_objects (session, &error,
		                                     CKA_CLASS, GP11_ULONG, CKO_SECRET_KEY,
		                                     CKA_G_COLLECTION, strlen (c_ident), c_ident,
		                                     CKA_ID, strlen (i_ident), i_ident,
		                                     GP11_INVALID);
	} else {
		is_item = FALSE;
		objects = gp11_session_find_objects (session, &error,
		                                     CKA_CLASS, GP11_ULONG, CKO_G_COLLECTION,
		                                     CKA_ID, strlen (c_ident), c_ident,
		                                     GP11_INVALID);
	}

	g_free (c_ident);
	g_free (i_ident);

	if (error != NULL) {
		g_warning ("couldn't lookup object: %s: %s", path, egg_error_message (error));
		g_clear_error (&error);
	}

	if (!objects)
		return gkd_secret_error_no_such_object (message);

	gp11_object_set_session (objects->data, session);
	if (is_item)
		reply = item_message_handler (self, objects->data, message);
	else
		reply = collection_message_handler (self, objects->data, message);

	gp11_list_unref_free (objects);
	return reply;
}

GP11Object*
gkd_secret_objects_lookup_collection (GkdSecretObjects *self, const gchar *caller,
                                      const gchar *path)
{
	GP11Object *object = NULL;
	GError *error = NULL;
	GList *objects;
	GP11Session *session;
	gchar *identifier;

	g_return_val_if_fail (GKD_SECRET_IS_OBJECTS (self), NULL);
	g_return_val_if_fail (caller, NULL);
	g_return_val_if_fail (path, NULL);

	if (!parse_object_path (self, path, &identifier, NULL))
		return NULL;

	/* The session we're using to access the object */
	session = gkd_secret_service_get_pkcs11_session (self->service, caller);
	g_return_val_if_fail (session, NULL);

	objects = gp11_session_find_objects (session, &error,
	                                     CKA_CLASS, GP11_ULONG, CKO_G_COLLECTION,
	                                     CKA_ID, strlen (identifier), identifier,
	                                     GP11_INVALID);

	g_free (identifier);

	if (error != NULL) {
		g_warning ("couldn't lookup collection: %s: %s", path, egg_error_message (error));
		g_clear_error (&error);
	}

	if (objects) {
		object = g_object_ref (objects->data);
		gp11_object_set_session (object, session);
	}

	gp11_list_unref_free (objects);
	return object;
}

GP11Object*
gkd_secret_objects_lookup_item (GkdSecretObjects *self, const gchar *caller,
                                const gchar *path)
{
	GP11Object *object = NULL;
	GError *error = NULL;
	GList *objects;
	GP11Session *session;
	gchar *collection;
	gchar *identifier;

	g_return_val_if_fail (GKD_SECRET_IS_OBJECTS (self), NULL);
	g_return_val_if_fail (caller, NULL);
	g_return_val_if_fail (path, NULL);

	if (!parse_object_path (self, path, &collection, &identifier))
		return NULL;

	/* The session we're using to access the object */
	session = gkd_secret_service_get_pkcs11_session (self->service, caller);
	g_return_val_if_fail (session, NULL);

	objects = gp11_session_find_objects (session, &error,
	                                     CKA_CLASS, GP11_ULONG, CKO_SECRET_KEY,
	                                     CKA_ID, strlen (identifier), identifier,
	                                     CKA_G_COLLECTION, strlen (collection), collection,
	                                     GP11_INVALID);

	g_free (identifier);
	g_free (collection);

	if (error != NULL) {
		g_warning ("couldn't lookup item: %s: %s", path, egg_error_message (error));
		g_clear_error (&error);
	}

	if (objects) {
		object = g_object_ref (objects->data);
		gp11_object_set_session (object, session);
	}

	gp11_list_unref_free (objects);
	return object;
}

void
gkd_secret_objects_append_item_paths (GkdSecretObjects *self, const gchar *base,
                                      DBusMessageIter *iter, DBusMessage *message)
{
	DBusMessageIter variant;
	GP11Session *session;
	GError *error = NULL;
	gchar *identifier;
	GList *items;

	g_return_if_fail (GKD_SECRET_IS_OBJECTS (self));
	g_return_if_fail (base);
	g_return_if_fail (iter);
	g_return_if_fail (message);

	/* The session we're using to access the object */
	session = gkd_secret_service_get_pkcs11_session (self->service, dbus_message_get_sender (message));
	g_return_if_fail (session);

	if (!parse_object_path (self, base, &identifier, NULL))
		g_return_if_reached ();

	items = gp11_session_find_objects (session, &error,
	                                   CKA_CLASS, GP11_ULONG, CKO_SECRET_KEY,
	                                   CKA_G_COLLECTION, strlen (identifier), identifier,
	                                   GP11_INVALID);

	if (error == NULL) {
		dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, "ao", &variant);
		iter_append_item_paths (base, items, &variant);
		dbus_message_iter_close_container (iter, &variant);
	} else {
		g_warning ("couldn't lookup items in '%s' collection: %s", identifier, egg_error_message (error));
		g_clear_error (&error);
	}

	gp11_list_unref_free (items);
	g_free (identifier);
}

void
gkd_secret_objects_append_collection_paths (GkdSecretObjects *self, DBusMessageIter *iter,
                                            DBusMessage *message)
{
	DBusMessageIter variant;
	GError *error = NULL;
	GP11Session *session;
	GList *colls;

	g_return_if_fail (GKD_SECRET_IS_OBJECTS (self));
	g_return_if_fail (iter && message);

	/* The session we're using to access the object */
	session = gkd_secret_service_get_pkcs11_session (self->service, dbus_message_get_sender (message));
	g_return_if_fail (session);

	colls = gp11_session_find_objects (session, &error,
	                                   CKA_CLASS, GP11_ULONG, CKO_G_COLLECTION,
	                                   GP11_INVALID);

	if (error != NULL) {
		g_warning ("couldn't lookup collections: %s", egg_error_message (error));
		g_clear_error (&error);
		return;
	}

	dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, "ao", &variant);
	iter_append_collection_paths (colls, &variant);
	dbus_message_iter_close_container (iter, &variant);
	gp11_list_unref_free (colls);
}

DBusMessage*
gkd_secret_objects_handle_search_items (GkdSecretObjects *self, DBusMessage *message,
                                        const gchar *base)
{
	GP11Attributes *attrs;
	GP11Attribute *attr;
	DBusMessageIter iter;
	GP11Object *search;
	GP11Session *session;
	DBusMessage *reply;
	GError *error = NULL;
	gchar *identifier;
	gpointer data;
	gsize n_data;
	GList *locked, *unlocked;
	GList *items;

	g_return_val_if_fail (GKD_SECRET_IS_OBJECTS (self), NULL);
	g_return_val_if_fail (message, NULL);

	if (!dbus_message_has_signature (message, "a{ss}"))
		return NULL;

	attrs = gp11_attributes_new ();
	attr = gp11_attributes_add_empty (attrs, CKA_G_FIELDS);

	dbus_message_iter_init (message, &iter);
	if (!gkd_secret_property_parse_fields (&iter, attr)) {
		gp11_attributes_unref (attrs);
		return dbus_message_new_error (message, DBUS_ERROR_FAILED,
		                               "Invalid data in attributes argument");
	}

	if (base != NULL) {
		if (!parse_object_path (self, base, &identifier, NULL))
			g_return_val_if_reached (NULL);
		gp11_attributes_add_string (attrs, CKA_G_COLLECTION, identifier);
		g_free (identifier);
	}

	gp11_attributes_add_ulong (attrs, CKA_CLASS, CKO_G_SEARCH);
	gp11_attributes_add_boolean (attrs, CKA_TOKEN, FALSE);

	/* The session we're using to access the object */
	session = gkd_secret_service_get_pkcs11_session (self->service, dbus_message_get_sender (message));
	g_return_val_if_fail (session, NULL);

	/* Create the search object */
	search = gp11_session_create_object_full (session, attrs, NULL, &error);
	gp11_attributes_unref (attrs);

	if (error != NULL) {
		reply = dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                       "Couldn't search for items: %s",
		                                       egg_error_message (error));
		g_clear_error (&error);
		return reply;
	}

	/* Get the matched item handles, and delete the search object */
	gp11_object_set_session (search, session);
	data = gp11_object_get_data (search, CKA_G_MATCHED, &n_data, &error);
	gp11_object_destroy (search, NULL);
	g_object_unref (search);

	if (error != NULL) {
		reply = dbus_message_new_error_printf (message, DBUS_ERROR_FAILED,
		                                       "Couldn't retrieve matched items: %s",
		                                       egg_error_message (error));
		g_clear_error (&error);
		return reply;
	}

	/* Build a list of object handles */
	items = gp11_objects_from_handle_array (gp11_session_get_slot (session),
	                                        data, n_data / sizeof (CK_OBJECT_HANDLE));
	g_free (data);

	/* Filter out the locked items */
	item_cleanup_search_results (session, items, &locked, &unlocked);

	/* Prepare the reply message */
	reply = dbus_message_new_method_return (message);
	dbus_message_iter_init_append (reply, &iter);
	iter_append_item_paths (NULL, unlocked, &iter);
	iter_append_item_paths (NULL, locked, &iter);

	g_list_free (locked);
	g_list_free (unlocked);
	gp11_list_unref_free (items);

	return reply;
}

DBusMessage*
gkd_secret_objects_handle_get_secrets (GkdSecretObjects *self, DBusMessage *message)
{
	DBusError derr = DBUS_ERROR_INIT;
	GkdSecretSession *session;
	GkdSecretSecret *secret;
	DBusMessage *reply;
	GP11Object *item;
	DBusMessageIter iter, array, dict;
	const char *session_path;
	const char *caller;
	char **paths;
	int n_paths, i;

	if (!dbus_message_get_args (message, NULL,
	                            DBUS_TYPE_ARRAY, DBUS_TYPE_OBJECT_PATH, &paths, &n_paths,
	                            DBUS_TYPE_OBJECT_PATH, &session_path,
	                            DBUS_TYPE_INVALID))
		return NULL;

	caller = dbus_message_get_sender (message);
	g_return_val_if_fail (caller, NULL);

	session = gkd_secret_service_lookup_session (self->service, session_path,
	                                             dbus_message_get_sender (message));
	if (session == NULL)
		return dbus_message_new_error (message, SECRET_ERROR_NO_SESSION, "The session does not exist");

	reply = dbus_message_new_method_return (message);
	dbus_message_iter_init_append (reply, &iter);
	dbus_message_iter_open_container (&iter, DBUS_TYPE_ARRAY, "{o(oayay)}", &array);

	for (i = 0; i < n_paths; ++i) {

		/* Try to find the item, if it doesn't exist, just ignore */
		item = gkd_secret_objects_lookup_item (self, caller, paths[i]);
		if (!item)
			continue;

		secret = gkd_secret_session_get_item_secret (session, item, &derr);
		g_object_unref (item);

		if (secret == NULL) {
			/* We ignore is locked, and just leave out from response */
			if (dbus_error_has_name (&derr, SECRET_ERROR_IS_LOCKED)) {
				dbus_error_free (&derr);
				continue;

			/* All other errors stop the operation */
			} else {
				dbus_message_unref (reply);
				reply = dbus_message_new_error (message, derr.name, derr.message);
				dbus_error_free (&derr);
				break;
			}
		}

		dbus_message_iter_open_container (&array, DBUS_TYPE_DICT_ENTRY, NULL, &dict);
		dbus_message_iter_append_basic (&dict, DBUS_TYPE_OBJECT_PATH, &(paths[i]));
		gkd_secret_secret_append (secret, &dict);
		gkd_secret_secret_free (secret);
		dbus_message_iter_close_container (&array, &dict);
	}

	if (i == n_paths)
		dbus_message_iter_close_container (&iter, &array);
	dbus_free_string_array (paths);

	return reply;
}

const gchar*
gkd_secret_objects_get_alias (GkdSecretObjects *self, const gchar *alias)
{
	g_return_val_if_fail (GKD_SECRET_IS_OBJECTS (self), NULL);
	g_return_val_if_fail (alias, NULL);
	return g_hash_table_lookup (self->aliases, alias);
}

void
gkd_secret_objects_set_alias (GkdSecretObjects *self, const gchar *alias,
                              const gchar *identifier)
{
	g_return_if_fail (GKD_SECRET_IS_OBJECTS (self));
	g_return_if_fail (alias);
	g_hash_table_replace (self->aliases, g_strdup (alias), g_strdup (identifier));
}
