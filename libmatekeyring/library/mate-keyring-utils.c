/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-keyring-utils.c - shared utility functions

   Copyright (C) 2003 Red Hat, Inc

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

   Author: Alexander Larsson <alexl@redhat.com>
*/
#include "config.h"

#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "mate-keyring.h"
#include "mate-keyring-private.h"
#include "mate-keyring-memory.h"

#include "egg/egg-secure-memory.h"

/**
 * SECTION:mate-keyring-result
 * @title: Result Codes
 * @short_description: Mate Keyring Result Codes
 *
 * <para>
 * Result codes used through out MATE Keyring. Additional result codes may be
 * added from time to time and these should be handled gracefully.
 * </para>
 **/

/**
 * mate_keyring_free_password:
 * @password: the password to be freed
 *
 * Clears the memory used by password by filling with '\0' and frees the memory
 * after doing this. You should use this function instead of g_free() for
 * secret information.
 **/
void
mate_keyring_free_password (gchar *password)
{
	egg_secure_strfree (password);
}

/**
 * mate_keyring_string_list_free:
 * @strings: A %GList of string pointers.
 *
 * Free a list of string pointers.
 **/
void
mate_keyring_string_list_free (GList *strings)
{
	g_list_foreach (strings, (GFunc) g_free, NULL);
	g_list_free (strings);
}

/**
 * mate_keyring_result_to_message:
 * @res: A #MateKeyringResult
 *
 * The #MATE_KEYRING_RESULT_OK and #MATE_KEYRING_RESULT_CANCELLED
 * codes will return an empty string.
 *
 * Note that there are some results for which the application will need to
 * take appropriate action rather than just display an error message to
 * the user.
 *
 * Return value: a string suitable for display to the user for a given
 * #MateKeyringResult, or an empty string if the message wouldn't make
 * sense to a user.
 **/
const gchar*
mate_keyring_result_to_message (MateKeyringResult res)
{
	switch (res) {

	/* If the caller asks for messages for these, they get what they deserve */
	case MATE_KEYRING_RESULT_OK:
	case MATE_KEYRING_RESULT_CANCELLED:
		return "";

	/* Valid displayable error messages */
	case MATE_KEYRING_RESULT_DENIED:
		return _("Access Denied");
	case MATE_KEYRING_RESULT_NO_KEYRING_DAEMON:
		return _("The mate-keyring-daemon application is not running.");
	case MATE_KEYRING_RESULT_IO_ERROR:
		return _("Error communicating with mate-keyring-daemon");
	case MATE_KEYRING_RESULT_ALREADY_EXISTS:
		return _("A keyring with that name already exists");
	case MATE_KEYRING_RESULT_BAD_ARGUMENTS:
		return _("Programmer error: The application sent invalid data.");
	case MATE_KEYRING_RESULT_NO_MATCH:
		return _("No matching results");
	case MATE_KEYRING_RESULT_NO_SUCH_KEYRING:
		return _("A keyring with that name does not exist.");

	/*
	 * This would be a dumb message to display to the user, we never return
	 * this from the daemon, only here for compatibility
	 */
	case MATE_KEYRING_RESULT_ALREADY_UNLOCKED:
		return _("The keyring has already been unlocked.");

	default:
		g_return_val_if_reached (NULL);
	};
}

/**
 * mate_keyring_found_free():
 * @found: a #MateKeyringFound
 *
 * Free the memory used by a #MateKeyringFound item.
 *
 * You usually want to use mate_keyring_found_list_free() on the list of
 * results.
 **/
void
mate_keyring_found_free (MateKeyringFound *found)
{
	if (found == NULL)
		return;
	g_free (found->keyring);
	mate_keyring_free_password (found->secret);
	mate_keyring_attribute_list_free (found->attributes);
	g_free (found);
}

/**
 * mate_keyring_found_list_free:
 * @found_list: a #GList of #MateKeyringFound
 *
 * Free the memory used by the #MateKeyringFound items in @found_list.
 **/
void
mate_keyring_found_list_free (GList *found_list)
{
	g_list_foreach (found_list, (GFunc) mate_keyring_found_free, NULL);
	g_list_free (found_list);
}

/**
 * SECTION:mate-keyring-attributes
 * @title: Item Attributes
 * @short_description: Attributes of individual keyring items.
 *
 * Attributes allow various other pieces of information to be associated with an item.
 * These can also be used to search for relevant items. Use mate_keyring_item_get_attributes()
 * or mate_keyring_item_set_attributes().
 *
 * Each attribute has either a string, or unsigned integer value.
 **/

/**
 * mate_keyring_attribute_list_append_string:
 * @attributes: A #MateKeyringAttributeList
 * @name: The name of the new attribute
 * @value: The value to store in @attributes
 *
 * Store a key-value-pair with a string value in @attributes.
 **/
void
mate_keyring_attribute_list_append_string (MateKeyringAttributeList *attributes,
                                            const char *name, const char *value)
{
	MateKeyringAttribute attribute;

	g_return_if_fail (attributes);
	g_return_if_fail (name);

	attribute.name = g_strdup (name);
	attribute.type = MATE_KEYRING_ATTRIBUTE_TYPE_STRING;
	attribute.value.string = g_strdup (value);

	g_array_append_val (attributes, attribute);
}

/**
 * mate_keyring_attribute_list_append_uint32:
 * @attributes: A #MateKeyringAttributeList
 * @name: The name of the new attribute
 * @value: The value to store in @attributes
 *
 * Store a key-value-pair with an unsigned 32bit number value in @attributes.
 **/
void
mate_keyring_attribute_list_append_uint32 (MateKeyringAttributeList *attributes,
                                            const char *name, guint32 value)
{
	MateKeyringAttribute attribute;

	g_return_if_fail (attributes);
	g_return_if_fail (name);

	attribute.name = g_strdup (name);
	attribute.type = MATE_KEYRING_ATTRIBUTE_TYPE_UINT32;
	attribute.value.integer = value;
	g_array_append_val (attributes, attribute);
}

/**
 * mate_keyring_attribute_list_free:
 * @attributes: A #MateKeyringAttributeList
 *
 * Free the memory used by @attributes.
 *
 * If a %NULL pointer is passed, it is ignored.
 **/
void
mate_keyring_attribute_list_free (MateKeyringAttributeList *attributes)
{
	MateKeyringAttribute *array;
	int i;

	if (attributes == NULL)
		return;

	array = (MateKeyringAttribute *)attributes->data;
	for (i = 0; i < attributes->len; i++) {
		g_free (array[i].name);
		if (array[i].type == MATE_KEYRING_ATTRIBUTE_TYPE_STRING) {
			g_free (array[i].value.string);
		}
	}

	g_array_free (attributes, TRUE);
}

/**
 * mate_keyring_attribute_list_copy:
 * @attributes: A #MateKeyringAttributeList to copy.
 *
 * Copy a list of item attributes.
 *
 * Return value: The new #MateKeyringAttributeList
 **/
MateKeyringAttributeList *
mate_keyring_attribute_list_copy (MateKeyringAttributeList *attributes)
{
	MateKeyringAttribute *array;
	MateKeyringAttributeList *copy;
	int i;

	if (attributes == NULL)
		return NULL;

	copy = g_array_sized_new (FALSE, FALSE, sizeof (MateKeyringAttribute), attributes->len);

	copy->len = attributes->len;
	memcpy (copy->data, attributes->data, sizeof (MateKeyringAttribute) * attributes->len);

	array = (MateKeyringAttribute *)copy->data;
	for (i = 0; i < copy->len; i++) {
		array[i].name = g_strdup (array[i].name);
		if (array[i].type == MATE_KEYRING_ATTRIBUTE_TYPE_STRING) {
			array[i].value.string = g_strdup (array[i].value.string);
		}
	}
	return copy;
}

/**
 * SECTION:mate-keyring-keyring-info
 * @title: Keyring Info
 * @short_description: Keyring Information
 *
 * Use mate_keyring_get_info() or mate_keyring_get_info_sync() to get a #MateKeyringInfo
 * pointer to use with these functions.
 **/

/**
 * mate_keyring_info_free:
 * @keyring_info: The keyring info to free.
 *
 * Free a #MateKeyringInfo object. If a %NULL pointer is passed
 * nothing occurs.
 **/
void
mate_keyring_info_free (MateKeyringInfo *keyring_info)
{
	g_free (keyring_info);
}

/**
 * SECTION:mate-keyring-item-info
 * @title: Item Information
 * @short_description: Keyring Item Info
 *
 * #MateKeyringItemInfo represents the basic information about a keyring item.
 * Use mate_keyring_item_get_info() or mate_keyring_item_set_info().
 **/

/**
 * mate_keyring_info_copy:
 * @keyring_info: The keyring info to copy.
 *
 * Copy a #MateKeyringInfo object.
 *
 * Return value: The newly allocated #MateKeyringInfo. This must be freed with
 * mate_keyring_info_free()
 **/
MateKeyringInfo *
mate_keyring_info_copy (MateKeyringInfo *keyring_info)
{
	MateKeyringInfo *copy;

	if (keyring_info == NULL)
		return NULL;

	copy = g_new (MateKeyringInfo, 1);
	memcpy (copy, keyring_info, sizeof (MateKeyringInfo));

	return copy;
}

/**
 * mate_keyring_item_info_free:
 * @item_info: The keyring item info pointer.
 *
 * Free the #MateKeyringItemInfo object.
 *
 * A %NULL pointer may be passed, in which case it will be ignored.
 **/
void
mate_keyring_item_info_free (MateKeyringItemInfo *item_info)
{
	if (item_info != NULL) {
		g_free (item_info->display_name);
		mate_keyring_free_password (item_info->secret);
		g_free (item_info);
	}
}

/**
 * mate_keyring_item_info_new:
 *
 * Create a new #MateKeyringItemInfo object.
 * Free the #MateKeyringItemInfo object.
 *
 * Return value: A keyring item info pointer.
 **/
MateKeyringItemInfo *
mate_keyring_item_info_new (void)
{
	MateKeyringItemInfo *info;

	info = g_new0 (MateKeyringItemInfo, 1);

	info->type = MATE_KEYRING_ITEM_NO_TYPE;

	return info;
}

/**
 * mate_keyring_item_info_copy:
 * @item_info: A keyring item info pointer.
 *
 * Copy a #MateKeyringItemInfo object.
 *
 * Return value: A keyring item info pointer.
 **/
MateKeyringItemInfo *
mate_keyring_item_info_copy (MateKeyringItemInfo *item_info)
{
	MateKeyringItemInfo *copy;

	if (item_info == NULL)
		return NULL;

	copy = g_new (MateKeyringItemInfo, 1);
	memcpy (copy, item_info, sizeof (MateKeyringItemInfo));

	copy->display_name = g_strdup (copy->display_name);
	copy->secret = egg_secure_strdup (copy->secret);

	return copy;
}

/**
 * mate_keyring_application_ref_new:
 *
 * Create a new application reference.
 *
 * Return value: A new #MateKeyringApplicationRef pointer.
 **/
MateKeyringApplicationRef *
mate_keyring_application_ref_new (void)
{
	MateKeyringApplicationRef *app_ref;

	app_ref = g_new0 (MateKeyringApplicationRef, 1);

	return app_ref;
}

/**
 * mate_keyring_application_ref_free:
 * @app: A #MateKeyringApplicationRef pointer
 *
 * Free an application reference.
 **/
void
mate_keyring_application_ref_free (MateKeyringApplicationRef *app)
{
	if (app) {
		g_free (app->display_name);
		g_free (app->pathname);
		g_free (app);
	}
}

/**
 * mate_keyring_application_ref_copy:
 * @app: A #MateKeyringApplicationRef pointer
 *
 * Copy an application reference.
 *
 * Return value: A new #MateKeyringApplicationRef pointer.
 **/
MateKeyringApplicationRef *
mate_keyring_application_ref_copy (const MateKeyringApplicationRef *app)
{
	MateKeyringApplicationRef *copy;

	if (app == NULL)
		return NULL;

	copy = g_new (MateKeyringApplicationRef, 1);
	copy->display_name = g_strdup (app->display_name);
	copy->pathname = g_strdup (app->pathname);

	return copy;
}

/**
 * mate_keyring_access_control_new:
 * @application: A #MateKeyringApplicationRef pointer
 * @types_allowed: Access types allowed.
 *
 * Create a new access control for an item. Combine the various access
 * rights allowed.
 *
 * Return value: The new #MateKeyringAccessControl pointer. Use
 * mate_keyring_access_control_free() to free the memory.
 **/
MateKeyringAccessControl *
mate_keyring_access_control_new (const MateKeyringApplicationRef *application,
                                  MateKeyringAccessType types_allowed)
{
	MateKeyringAccessControl *ac;
	ac = g_new (MateKeyringAccessControl, 1);

	ac->application = mate_keyring_application_ref_copy (application);
	ac->types_allowed = types_allowed;

	return ac;
}

/**
 * mate_keyring_access_control_free:
 * @ac: A #MateKeyringAccessControl pointer
 *
 * Free an access control for an item.
 **/
void
mate_keyring_access_control_free (MateKeyringAccessControl *ac)
{
	if (ac == NULL)
		return;
	mate_keyring_application_ref_free (ac->application);
	g_free (ac);
}

/**
 * mate_keyring_access_control_copy:
 * @ac: A #MateKeyringAcessControl pointer
 *
 * Copy an access control for an item.
 *
 * Return value: The new #MateKeyringAccessControl pointer. Use
 * mate_keyring_access_control_free() to free the memory.
 **/
MateKeyringAccessControl *
mate_keyring_access_control_copy (MateKeyringAccessControl *ac)
{
	MateKeyringAccessControl *ret;

	if (ac == NULL)
		return NULL;

	ret = mate_keyring_access_control_new (mate_keyring_application_ref_copy (ac->application), ac->types_allowed);

	return ret;
}

/**
 * mate_keyring_acl_copy:
 * @list: A list of #MateKeyringAccessControl pointers.
 *
 * Copy an access control list.
 *
 * Return value: A new list of #MateKeyringAccessControl items. Use
 * mate_keyring_acl_free() to free the memory.
 **/
GList *
mate_keyring_acl_copy (GList *list)
{
	GList *ret, *l;

	ret = g_list_copy (list);
	for (l = ret; l != NULL; l = l->next) {
		l->data = mate_keyring_access_control_copy (l->data);
	}

	return ret;
}

/**
 * mate_keyring_acl_free:
 * @acl: A list of #MateKeyringAccessControl pointers.
 *
 * Free an access control list.
 **/
void
mate_keyring_acl_free (GList *acl)
{
	g_list_foreach (acl, (GFunc)mate_keyring_access_control_free, NULL);
	g_list_free (acl);
}

/**
 * mate_keyring_info_set_lock_on_idle:
 * @keyring_info: The keyring info.
 * @value: Whether to lock or not.
 *
 * Set whether or not to lock a keyring after a certain amount of idle time.
 *
 * See also mate_keyring_info_set_lock_timeout().
 **/
void
mate_keyring_info_set_lock_on_idle (MateKeyringInfo *keyring_info,
                                     gboolean          value)
{
	g_return_if_fail (keyring_info);
	keyring_info->lock_on_idle = value;
}

/**
 * mate_keyring_info_get_lock_on_idle:
 * @keyring_info: The keyring info.
 *
 * Get whether or not to lock a keyring after a certain amount of idle time.
 *
 * See also mate_keyring_info_get_lock_timeout().
 *
 * Return value: Whether to lock or not.
 **/
gboolean
mate_keyring_info_get_lock_on_idle (MateKeyringInfo *keyring_info)
{
	g_return_val_if_fail (keyring_info, FALSE);
	return keyring_info->lock_on_idle;
}

/**
 * mate_keyring_info_set_lock_timeout:
 * @keyring_info: The keyring info.
 * @value: The lock timeout in seconds.
 *
 * Set the idle timeout, in seconds, after which to lock the keyring.
 *
 * See also mate_keyring_info_set_lock_on_idle().
 **/
void
mate_keyring_info_set_lock_timeout (MateKeyringInfo *keyring_info,
                                     guint32           value)
{
	g_return_if_fail (keyring_info);
	keyring_info->lock_timeout = value;
}

/**
 * mate_keyring_info_get_lock_timeout:
 * @keyring_info: The keyring info.
 *
 * Get the idle timeout, in seconds, after which to lock the keyring.
 *
 * See also mate_keyring_info_get_lock_on_idle().
 *
 * Return value: The idle timeout, in seconds.
 **/
guint32
mate_keyring_info_get_lock_timeout (MateKeyringInfo *keyring_info)
{
	g_return_val_if_fail (keyring_info, 0);
	return keyring_info->lock_timeout;
}

/**
 * mate_keyring_info_get_mtime:
 * @keyring_info: The keyring info.
 *
 * Get the time at which the keyring was last modified.
 *
 * Return value: The last modified time.
 **/
time_t
mate_keyring_info_get_mtime (MateKeyringInfo *keyring_info)
{
	g_return_val_if_fail (keyring_info, 0);
	return keyring_info->mtime;
}

/**
 * mate_keyring_info_get_ctime:
 * @keyring_info: The keyring info.
 *
 * Get the time at which the keyring was created.
 *
 * Return value: The created time.
 **/
time_t
mate_keyring_info_get_ctime (MateKeyringInfo *keyring_info)
{
	g_return_val_if_fail (keyring_info, 0);
	return keyring_info->ctime;
}

/**
 * mate_keyring_info_get_is_locked:
 * @keyring_info: The keyring info.
 *
 * Get whether the keyring is locked or not.
 *
 * Return value: Whether the keyring is locked or not.
 **/
gboolean
mate_keyring_info_get_is_locked (MateKeyringInfo *keyring_info)
{
	g_return_val_if_fail (keyring_info, FALSE);
	return keyring_info->is_locked;
}

/**
 * mate_keyring_item_info_get_type:
 * @item_info: A keyring item info pointer.
 *
 * Get the item type.
 *
 * Return value: The item type
 **/
MateKeyringItemType
mate_keyring_item_info_get_type (MateKeyringItemInfo *item_info)
{
	g_return_val_if_fail (item_info, 0);
	return item_info->type;
}

/**
 * mate_keyring_item_info_set_type:
 * @item_info: A keyring item info pointer.
 * @type: The new item type
 *
 * Set the type on an item info.
 **/
void
mate_keyring_item_info_set_type (MateKeyringItemInfo *item_info,
                                  MateKeyringItemType  type)
{
	g_return_if_fail (item_info);
	item_info->type = type;
}

/**
 * mate_keyring_item_info_get_secret:
 * @item_info: A keyring item info pointer.
 *
 * Get the item secret.
 *
 * Return value: The newly allocated string containing the item secret.
 **/
char *
mate_keyring_item_info_get_secret (MateKeyringItemInfo *item_info)
{
	/* XXXX For compatibility reasons we can't use secure memory here */
	g_return_val_if_fail (item_info, NULL);
	return g_strdup (item_info->secret);
}

/**
 * mate_keyring_item_info_set_secret:
 * @item_info: A keyring item info pointer.
 * @value: The new item secret
 *
 * Set the secret on an item info.
 **/
void
mate_keyring_item_info_set_secret (MateKeyringItemInfo *item_info,
                                    const char           *value)
{
	g_return_if_fail (item_info);
	mate_keyring_free_password (item_info->secret);
	item_info->secret = mate_keyring_memory_strdup (value);
}

/**
 * mate_keyring_item_info_get_display_name:
 * @item_info: A keyring item info pointer.
 *
 * Get the item display name.
 *
 * Return value: The newly allocated string containing the item display name.
 **/
char *
mate_keyring_item_info_get_display_name (MateKeyringItemInfo *item_info)
{
	g_return_val_if_fail (item_info, NULL);
	return g_strdup (item_info->display_name);
}

/**
 * mate_keyring_item_info_set_display_name:
 * @item_info: A keyring item info pointer.
 * @value: The new display name.
 *
 * Set the display name on an item info.
 **/
void
mate_keyring_item_info_set_display_name (MateKeyringItemInfo *item_info,
                                          const char           *value)
{
	g_return_if_fail (item_info);
	g_free (item_info->display_name);
	item_info->display_name = g_strdup (value);
}

/**
 * mate_keyring_item_info_get_mtime:
 * @item_info: A keyring item info pointer.
 *
 * Get the item last modified time.
 *
 * Return value: The item last modified time.
 **/
time_t
mate_keyring_item_info_get_mtime (MateKeyringItemInfo *item_info)
{
	g_return_val_if_fail (item_info, 0);
	return item_info->mtime;
}

/**
 * mate_keyring_item_info_get_ctime:
 * @item_info: A keyring item info pointer.
 *
 * Get the item created time.
 *
 * Return value: The item created time.
 **/
time_t
mate_keyring_item_info_get_ctime (MateKeyringItemInfo *item_info)
{
	g_return_val_if_fail (item_info, 0);
	return item_info->ctime;
}

/**
 * SECTION:mate-keyring-acl
 * @title: Item ACLs
 * @short_description: Access control lists for keyring items.
 *
 * Each item has an access control list, which specifies the applications that
 * can read, write or delete an item. The read access applies only to reading the secret.
 * All applications can read other parts of the item. ACLs are accessed and changed
 * mate_keyring_item_get_acl() and mate_keyring_item_set_acl().
 **/

/**
 * mate_keyring_item_ac_get_display_name:
 * @ac: A #MateKeyringAccessControl pointer.
 *
 * Get the access control application's display name.
 *
 * Return value: A newly allocated string containing the display name.
 **/
char *
mate_keyring_item_ac_get_display_name (MateKeyringAccessControl *ac)
{
	g_return_val_if_fail (ac, NULL);
	return g_strdup (ac->application->display_name);
}

/**
 * mate_keyring_item_ac_set_display_name:
 * @ac: A #MateKeyringAcccessControl pointer.
 * @value: The new application display name.
 *
 * Set the access control application's display name.
 **/
void
mate_keyring_item_ac_set_display_name (MateKeyringAccessControl *ac,
                                        const char                *value)
{
	g_return_if_fail (ac);
	g_free (ac->application->display_name);
	ac->application->display_name = g_strdup (value);
}

/**
 * mate_keyring_item_ac_get_path_name:
 * @ac: A #MateKeyringAccessControl pointer.
 *
 * Get the access control application's full path name.
 *
 * Return value: A newly allocated string containing the display name.
 **/
char *
mate_keyring_item_ac_get_path_name (MateKeyringAccessControl *ac)
{
	g_return_val_if_fail (ac, NULL);
	return g_strdup (ac->application->pathname);
}

/**
 * mate_keyring_item_ac_set_path_name:
 * @ac: A #MateKeyringAccessControl pointer
 * @value: The new application full path.
 *
 * Set the access control application's full path name.
 **/
void
mate_keyring_item_ac_set_path_name (MateKeyringAccessControl *ac,
                                     const char                *value)
{
	g_return_if_fail (ac);
	g_free (ac->application->pathname);
	ac->application->pathname = g_strdup (value);
}

/**
 * mate_keyring_item_ac_get_access_type:
 * @ac: A #MateKeyringAccessControl pointer.
 *
 * Get the application access rights for the access control.
 *
 * Return value: The access rights.
 **/
MateKeyringAccessType
mate_keyring_item_ac_get_access_type (MateKeyringAccessControl *ac)
{
	g_return_val_if_fail (ac, 0);
	return ac->types_allowed;
}

/**
 * mate_keyring_item_ac_set_access_type:
 * @ac: A #MateKeyringAccessControl pointer.
 * @value: The new access rights.
 *
 * Set the application access rights for the access control.
 **/
void
mate_keyring_item_ac_set_access_type (MateKeyringAccessControl *ac,
                                       const MateKeyringAccessType value)
{
	g_return_if_fail (ac);
	ac->types_allowed = value;
}
