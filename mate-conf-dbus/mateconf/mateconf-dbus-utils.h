/* MateConf
 * Copyright (C) 2003 Imendio HB
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef MATECONF_DBUS_UTILS_H
#define MATECONF_DBUS_UTILS_H

#include <glib.h>
#include <dbus/dbus.h>
#include <mateconf/mateconf.h>
#include <mateconf/mateconf-value.h>

#define MATECONF_DBUS_SERVICE                  "org.mate.MateConf"

#define MATECONF_DBUS_SERVER_INTERFACE         "org.mate.MateConf.Server"
#define MATECONF_DBUS_DATABASE_INTERFACE       "org.mate.MateConf.Database"

#define MATECONF_DBUS_SERVER_OBJECT            "/org/mate/MateConf/Server"

#define MATECONF_DBUS_SERVER_GET_DEFAULT_DB    "GetDefaultDatabase"
#define MATECONF_DBUS_SERVER_GET_DB            "GetDatabase"
#define MATECONF_DBUS_SERVER_SHUTDOWN          "Shutdown"

#define MATECONF_DBUS_DATABASE_LOOKUP          "Lookup"
#define MATECONF_DBUS_DATABASE_LOOKUP_EXTENDED "LookupExtended" 
#define MATECONF_DBUS_DATABASE_LOOKUP_DEFAULT  "LookupDefault" 
#define MATECONF_DBUS_DATABASE_SET             "Set"
#define MATECONF_DBUS_DATABASE_UNSET           "UnSet"
#define MATECONF_DBUS_DATABASE_RECURSIVE_UNSET "RecursiveUnset"
#define MATECONF_DBUS_DATABASE_DIR_EXISTS      "DirExists"
#define MATECONF_DBUS_DATABASE_GET_ALL_ENTRIES "AllEntries"
#define MATECONF_DBUS_DATABASE_GET_ALL_DIRS    "AllDirs"
#define MATECONF_DBUS_DATABASE_SET_SCHEMA      "SetSchema"
#define MATECONF_DBUS_DATABASE_SUGGEST_SYNC    "SuggestSync"

#define MATECONF_DBUS_DATABASE_ADD_NOTIFY      "AddNotify"
#define MATECONF_DBUS_DATABASE_REMOVE_NOTIFY   "RemoveNotify"
 
#define MATECONF_DBUS_LISTENER_NOTIFY          "Notify"

#define MATECONF_DBUS_CLIENT_SERVICE           "org.mate.MateConf.ClientService"
#define MATECONF_DBUS_CLIENT_OBJECT            "/org/mate/MateConf/Client"
#define MATECONF_DBUS_CLIENT_INTERFACE         "org.mate.MateConf.Client"

#define MATECONF_DBUS_UNSET_INCLUDING_SCHEMA_NAMES 0x1
 
#define MATECONF_DBUS_ERROR_FAILED               "org.mate.MateConf.Error.Failed"
#define MATECONF_DBUS_ERROR_NO_PERMISSION        "org.mate.MateConf.Error.NoPermission"
#define MATECONF_DBUS_ERROR_BAD_ADDRESS          "org.mate.MateConf.Error.BadAddress"
#define MATECONF_DBUS_ERROR_BAD_KEY              "org.mate.MateConf.Error.BadKey"
#define MATECONF_DBUS_ERROR_PARSE_ERROR          "org.mate.MateConf.Error.ParseError"
#define MATECONF_DBUS_ERROR_CORRUPT              "org.mate.MateConf.Error.Corrupt"
#define MATECONF_DBUS_ERROR_TYPE_MISMATCH        "org.mate.MateConf.Error.TypeMismatch"
#define MATECONF_DBUS_ERROR_IS_DIR               "org.mate.MateConf.Error.IsDir"
#define MATECONF_DBUS_ERROR_IS_KEY               "org.mate.MateConf.Error.IsKey"
#define MATECONF_DBUS_ERROR_NO_WRITABLE_DATABASE "org.mate.MateConf.Error.NoWritableDatabase"
#define MATECONF_DBUS_ERROR_IN_SHUTDOWN          "org.mate.MateConf.Error.InShutdown"
#define MATECONF_DBUS_ERROR_OVERRIDDEN           "org.mate.MateConf.Error.Overriden"
#define MATECONF_DBUS_ERROR_LOCK_FAILED          "org.mate.MateConf.Error.LockFailed"

void        mateconf_dbus_utils_append_value     (DBusMessageIter   *iter,
					       const MateConfValue  *value);
MateConfValue *mateconf_dbus_utils_get_value        (DBusMessageIter   *iter);

void        mateconf_dbus_utils_append_entry_values (DBusMessageIter   *iter,
						 const gchar       *key,
						 const MateConfValue  *value,
						 gboolean           is_default,
						 gboolean           is_writable,
						 const gchar       *schema_name);
gboolean    mateconf_dbus_utils_get_entry_values   (DBusMessageIter   *iter,
						 gchar            **key,
						 MateConfValue       **value,
						 gboolean          *is_default,
						 gboolean          *is_writable,
						 gchar            **schema_name);

void mateconf_dbus_utils_append_entries (DBusMessageIter *iter,
				      GSList          *entries);

GSList *mateconf_dbus_utils_get_entries (DBusMessageIter *iter, const gchar *dir);


#endif/* MATECONF_DBUS_UTILS_H */
