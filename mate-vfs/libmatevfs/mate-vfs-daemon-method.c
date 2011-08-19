/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2006 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <glib.h>
#include <libmatevfs/mate-vfs.h>
#include <libmatevfs/mate-vfs-daemon-method.h>
#include <libmatevfs/mate-vfs-dbus-utils.h>
#include <libmatevfs/mate-vfs-module-shared.h>
#include <libmatevfs/mate-vfs-cancellation-private.h>
#include <libmatevfs/mate-vfs-module-callback-private.h>

#include <dbus/dbus.h>

/* This is the current max in D-BUS, any number larger are getting set to
 * 6 hours. */
#define DBUS_TIMEOUT_OPEN_CLOSE (5 * 60 * 60 * 1000)

/* Will be used for other operations */
#define DBUS_TIMEOUT_DEFAULT 30 * 1000

#define d(x)

typedef struct {
	gint32   handle_id;

	GList   *dirs;
	GList   *current;
} DirectoryHandle;

typedef struct {
	gint32   handle_id;

	/* Probably need more stuff here? */
} FileHandle;

typedef struct {
	DBusConnection *connection;
	gint conn_id;
	gint handle;
} LocalConnection;

static void              append_args_valist           (DBusMessage     *message,
						       DvdArgumentType  first_arg_type,
						       va_list          var_args);
static DBusMessage *     execute_operation            (const gchar     *method,
						       MateVFSContext *context,
						       MateVFSResult  *result,
						       gint             timeout,
						       DvdArgumentType  type,
						       ...);
static gboolean          check_if_reply_is_error      (DBusMessage     *reply,
						       MateVFSResult  *result);
static DBusMessage *     create_method_call           (const gchar     *method);
static gint32            cancellation_id_new          (MateVFSContext *context,
						       LocalConnection *conn);
static void              cancellation_id_free         (gint32           cancellation_id,
						       MateVFSContext *context);
static void              connection_unregistered_func (DBusConnection  *conn,
						       gpointer         data);
static DBusHandlerResult connection_message_func      (DBusConnection  *conn,
						       DBusMessage     *message,
						       gpointer         data);

static GStaticPrivate  local_connection_private = G_STATIC_PRIVATE_INIT;


static DBusObjectPathVTable connection_vtable = {
	connection_unregistered_func,
	connection_message_func,
	NULL
};

static void
utils_append_string_or_null (DBusMessageIter *iter,
			     const gchar     *str)
{
	if (str == NULL)
		str = "";
	
	dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &str);
}

static const gchar *
utils_peek_string_or_null (DBusMessageIter *iter, gboolean empty_is_null)
{
	const gchar *str;

	dbus_message_iter_get_basic (iter, &str);

	if (empty_is_null && *str == 0) {
		return NULL;
	} else {
		return str;
	}
}

/*
 * FileInfo messages
 */


gboolean
mate_vfs_daemon_message_iter_append_file_info (DBusMessageIter        *iter,
						const MateVFSFileInfo *info)
{
	DBusMessageIter  struct_iter;
	gint32           i;
	guint32          u;
	gint64           i64;
	gchar           *str;

	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (info != NULL, FALSE);

	if (!dbus_message_iter_open_container (iter,
					       DBUS_TYPE_STRUCT,
					       NULL, /* for struct */
					       &struct_iter)) {
		return FALSE;
	}

	i = info->valid_fields;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	str = mate_vfs_escape_path_string (info->name);
	utils_append_string_or_null (&struct_iter, str);
	g_free (str);

	i = info->type;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->permissions;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->flags;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->device;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i64 = info->inode;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT64, &i64);

	i = info->link_count;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	u = info->uid;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_UINT32, &u);

	u = info->gid;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_UINT32, &u);

	i64 = info->size;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT64, &i64);
			
	i64 = info->block_count;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT64, &i64);

	i = info->atime;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->mtime;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	i = info->ctime;
	dbus_message_iter_append_basic (&struct_iter, DBUS_TYPE_INT32, &i);

	str = mate_vfs_escape_path_string (info->symlink_name);
	utils_append_string_or_null (&struct_iter, str);
	g_free (str);

	utils_append_string_or_null (&struct_iter, info->mime_type);

	dbus_message_iter_close_container (iter, &struct_iter);
	
	return TRUE;
}

gboolean
mate_vfs_daemon_message_append_file_info (DBusMessage            *message,
					   const MateVFSFileInfo *info)
{
	DBusMessageIter iter;

	g_return_val_if_fail (message != NULL, FALSE);
	g_return_val_if_fail (info != NULL, FALSE);

	dbus_message_iter_init_append (message, &iter);
	
	return mate_vfs_daemon_message_iter_append_file_info (&iter, info);
}


MateVFSFileInfo *
mate_vfs_daemon_message_iter_get_file_info (DBusMessageIter *iter)
{
	DBusMessageIter   struct_iter;
	MateVFSFileInfo *info;
	const gchar      *str;
	gint32            i;
	guint32           u;
	gint64            i64;

	g_return_val_if_fail (iter != NULL, NULL);
	g_assert (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_STRUCT);

	dbus_message_iter_recurse (iter, &struct_iter);

	info = mate_vfs_file_info_new ();

	dbus_message_iter_get_basic (&struct_iter, &i);
	info->valid_fields = i;

	dbus_message_iter_next (&struct_iter);
	str = utils_peek_string_or_null (&struct_iter, FALSE);
	info->name = mate_vfs_unescape_string (str, NULL);
	
	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->type = i;
	
	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->permissions = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->flags = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->device = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i64);
	info->inode = i64;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->link_count = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &u);
	info->uid = u;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &u);
	info->gid = u;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i64);
	info->size = i64;
			
	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i64);
	info->block_count = i64;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->atime = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->mtime = i;

	dbus_message_iter_next (&struct_iter);
	dbus_message_iter_get_basic (&struct_iter, &i);
	info->ctime = i;

	dbus_message_iter_next (&struct_iter);
	str = utils_peek_string_or_null (&struct_iter, TRUE);
	if (str) {
		info->symlink_name = mate_vfs_unescape_string (str, NULL);
	}

	dbus_message_iter_next (&struct_iter);
	str = utils_peek_string_or_null (&struct_iter, TRUE);
	if (str) {
		info->mime_type = g_strdup (str);
	}

	return info;
}

static GList *
dbus_utils_message_get_file_info_list (DBusMessage *message)
{
	DBusMessageIter   iter, array_iter;
	MateVFSFileInfo *info;
	GList            *list;

	g_return_val_if_fail (message != NULL, NULL);

	if (!dbus_message_iter_init (message, &iter)) {
		return NULL;
	}

	/* First skip the result code (which has already been checked). */
	if (!dbus_message_iter_next (&iter)) {
		return NULL;
	}

	dbus_message_iter_recurse (&iter, &array_iter);

	list = NULL;
	if (dbus_message_iter_get_arg_type (&array_iter) != DBUS_TYPE_INVALID) {
		do {
			info = mate_vfs_daemon_message_iter_get_file_info (&array_iter);
			if (info) {
				list = g_list_prepend (list, info);
			}
		} while (dbus_message_iter_next (&array_iter));
	}

	list = g_list_reverse (list);

	return list;
}

typedef struct {
	gint32 id;
	gchar *sender;
} CancellationRequest;

static void
destroy_private_connection (gpointer data)
{
	LocalConnection *ret = data;

	dbus_connection_close (ret->connection);
	dbus_connection_unref (ret->connection);
	g_free (ret);
}

static void
private_connection_died (LocalConnection *connection)
{
	g_static_private_set (&local_connection_private, NULL, NULL);
}

static LocalConnection *
get_private_connection ()
{
	DBusMessage *message;
	DBusMessage *reply;
	DBusError error;
	DBusConnection *main_conn, *private_conn;
	gchar        *address;
	dbus_int32_t conn_id;
	LocalConnection *ret;

	ret = g_static_private_get (&local_connection_private);
	if (ret != NULL) {
		return ret;
	}
	
	dbus_error_init (&error);

	/* Use a private session connection since this happens on
	 * a non-main thread and we don't want to mess up the main thread */
	main_conn = dbus_bus_get_private (DBUS_BUS_SESSION, &error);
	if (!main_conn) {
		g_printerr ("Couldn't get main dbus connection: %s\n",
			    error.message);
		dbus_error_free (&error);
		return NULL;
	}
	
	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
						DVD_DAEMON_OBJECT,
						DVD_DAEMON_INTERFACE,
						DVD_DAEMON_METHOD_GET_CONNECTION);
	dbus_message_set_auto_start (message, TRUE);
	reply = dbus_connection_send_with_reply_and_block (main_conn,
							   message,
							   -1,
							   &error);
	dbus_message_unref (message);
	dbus_connection_close (main_conn);
	dbus_connection_unref (main_conn);
	if (!reply) {
		g_warning ("Error while getting peer-to-peer connection: %s",
			   error.message);
		dbus_error_free (&error);
		return NULL;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_STRING, &address,
			       DBUS_TYPE_INT32, &conn_id,
			       DBUS_TYPE_INVALID);
	
	private_conn = dbus_connection_open_private (address, &error);
	if (!private_conn) {
		g_warning ("Failed to connect to peer-to-peer address (%s): %s",
			   address, error.message);
		dbus_message_unref (reply);
		dbus_error_free (&error);
		return NULL;
	}
	dbus_message_unref (reply);


	if (!dbus_connection_register_object_path (private_conn,
						   DVD_CLIENT_OBJECT,
						   &connection_vtable,
						   NULL)) {
		g_warning ("Failed to register client object with the connection.");
		dbus_connection_close (private_conn);
		dbus_connection_unref (private_conn);
		return NULL;
	}
	
	
	ret = g_new (LocalConnection, 1);
	ret->connection = private_conn;
	ret->conn_id = conn_id;
	ret->handle = 0;

	g_static_private_set (&local_connection_private,
			      ret, destroy_private_connection);
	
	return ret;
}

static void
append_args_valist (DBusMessage     *message,
		    DvdArgumentType  first_arg_type,
		    va_list          var_args)
{
	DvdArgumentType type;
	DBusMessageIter iter;

	dbus_message_iter_init_append (message, &iter);

	type = first_arg_type;
	while (type != DVD_TYPE_LAST) {
		switch (type) {
		case DVD_TYPE_URI: {
			MateVFSURI *uri;
			gchar       *uri_str;

			uri = va_arg (var_args, MateVFSURI *);
			uri_str = mate_vfs_uri_to_string (uri,
							   MATE_VFS_URI_HIDE_NONE);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_STRING,
							     &uri_str)) {
				g_error ("Out of memory");
			}

			g_free (uri_str);
			break;
		}
		case DVD_TYPE_STRING: {
			const gchar *str;

			str = va_arg (var_args, const gchar *);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_STRING,
							     &str)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_INT32: {
			dbus_int32_t int32;

			int32 = va_arg (var_args, dbus_int32_t);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_INT32,
							     &int32)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_INT64: {
			dbus_int64_t int64;

			int64 = va_arg (var_args, dbus_int64_t);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_INT64,
							     &int64)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_UINT64: {
			dbus_uint64_t uint64;

			uint64 = va_arg (var_args, dbus_uint64_t);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_UINT64,
							     &uint64)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_BOOL: {
			dbus_bool_t bool_v;

			bool_v = va_arg (var_args, dbus_bool_t);
			if (!dbus_message_iter_append_basic (&iter,
							     DBUS_TYPE_BOOLEAN,
							     &bool_v)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_FILE_INFO: {
			MateVFSFileInfo *info;

			info = va_arg (var_args, MateVFSFileInfo *);
			if (!mate_vfs_daemon_message_iter_append_file_info (&iter, info)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_BYTE_ARRAY: {
			DBusMessageIter      array_iter;
			unsigned const char *data;
			gint                 len;

			len = va_arg (var_args, dbus_int32_t);
			data = va_arg (var_args, unsigned const char *);
			
			if (!dbus_message_iter_open_container (&iter,
							       DBUS_TYPE_ARRAY,
							       DBUS_TYPE_BYTE_AS_STRING,
							       &array_iter)) {
				g_error ("Out of memory");
			}

			if (!dbus_message_iter_append_fixed_array (&array_iter,
								   DBUS_TYPE_BYTE,
								   &data,
								   len)) {
				g_error ("Out of memory");
			}

			if (!dbus_message_iter_close_container (&iter, &array_iter)) {
				g_error ("Out of memory");
			}
			break;
		}
		case DVD_TYPE_LAST:
			return;
		}

		type = va_arg (var_args, DvdArgumentType);
	}
}

static void
append_args (DBusMessage     *message,
	     DvdArgumentType  first_arg_type,
	     ...)
{
	va_list var_args;
	
	va_start (var_args, first_arg_type);
	append_args_valist (message, first_arg_type, var_args);
	va_end (var_args);
}

static DBusMessage *
execute_operation (const gchar      *method,
		   MateVFSContext  *context,
		   MateVFSResult   *result,
		   gint              timeout,
		   DvdArgumentType   first_arg_type,
		   ...)
{
	DBusMessage *message;
	DBusMessage *reply;
	va_list      var_args;
	gint32       cancellation_id;
	DBusError    error;
	LocalConnection *connection;
	DBusPendingCall *pending_call;
	gint conn_id;

	connection = get_private_connection (&conn_id);
	if (connection == NULL) {
		*result = MATE_VFS_ERROR_INTERNAL;
		return NULL;
	}

	message = create_method_call (method);

	va_start (var_args, first_arg_type);
	append_args_valist (message, first_arg_type, var_args);
	va_end (var_args);

	cancellation_id = -1;
	if (context) {
		cancellation_id = cancellation_id_new (context,
						       connection);
		dbus_message_append_args (message,
					  DBUS_TYPE_INT32, &cancellation_id,
					  DBUS_TYPE_INVALID);
	}

	dbus_error_init (&error);
	d(g_print ("Executing operation '%s'... \n", method));

	if (timeout == -1) {
		timeout = DBUS_TIMEOUT_DEFAULT;
	}

	if (!dbus_connection_send_with_reply (connection->connection,
					      message, &pending_call, timeout)) {
		dbus_message_unref (message);
		*result = MATE_VFS_ERROR_INTERNAL;
		reply = NULL;
		goto out;
	}

	dbus_message_unref (message);
	
	while (!dbus_pending_call_get_completed (pending_call) &&
	       dbus_connection_read_write_dispatch (connection->connection, -1))
		;

	if (!dbus_connection_get_is_connected (connection->connection)) {
		private_connection_died (connection);
		*result = MATE_VFS_ERROR_INTERNAL;
		reply = NULL;
		goto out;
	}
	reply = dbus_pending_call_steal_reply (pending_call);

	dbus_pending_call_unref (pending_call);
	
	if (result) {
		*result = (reply == NULL) ? MATE_VFS_ERROR_TIMEOUT : MATE_VFS_OK;
	}
 out:
	if (cancellation_id != -1) {
		cancellation_id_free (cancellation_id, context);
	}

	return reply;
}

static gboolean
check_if_reply_is_error (DBusMessage *reply, MateVFSResult *result)
{
	MateVFSResult r;

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &r,
			       DBUS_TYPE_INVALID);

	d(g_print ("check_if_reply_is_error: %s\n",
		   mate_vfs_result_to_string (r)));

	if (result) {
		*result = r;
	}

	if (r == MATE_VFS_OK) {
		return FALSE;
	}

	dbus_message_unref (reply);

	return TRUE;
}

static DBusMessage *
create_method_call (const gchar *method)
{
	DBusMessage *message;

	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
                                                DVD_DAEMON_OBJECT,
                                                DVD_DAEMON_INTERFACE,
                                                method);
	if (!message) {
		g_error ("Out of memory");
	}

	return message;
}

static DirectoryHandle *
directory_handle_new (gint32 handle_id)
{
	DirectoryHandle *handle;

	handle = g_new0 (DirectoryHandle, 1);
	handle->handle_id = handle_id;

	return handle;
}

static void
directory_handle_free (DirectoryHandle *handle)
{
	mate_vfs_file_info_list_free (handle->dirs);

	g_free (handle);
}

static FileHandle *
file_handle_new (gint32 handle_id)
{
	FileHandle *handle;

	handle = g_new0 (FileHandle, 1);
	handle->handle_id = handle_id;

	return handle;
}

static void
file_handle_free (FileHandle *handle)
{
	g_free (handle);
}

static gint32
cancellation_id_new (MateVFSContext *context,
		     LocalConnection *conn)
{
	MateVFSCancellation *cancellation;

	conn->handle++;
	cancellation = mate_vfs_context_get_cancellation (context);
	if (cancellation) {
		_mate_vfs_cancellation_set_handle (
						    cancellation,
						    conn->conn_id, conn->handle);
	}

	return conn->handle;
}

static void
cancellation_id_free (gint32           cancellation_id,
		      MateVFSContext *context)
{
	MateVFSCancellation *cancellation;
	cancellation = mate_vfs_context_get_cancellation (context);
	if (cancellation != NULL) {
		_mate_vfs_cancellation_unset_handle (cancellation);
	}
}

static void
connection_unregistered_func (DBusConnection *conn,
			      gpointer        data)
{
}

#define IS_METHOD(msg,method) \
  dbus_message_is_method_call(msg,DVD_CLIENT_INTERFACE,method)

static DBusHandlerResult
connection_message_func (DBusConnection *dbus_conn,
			 DBusMessage    *message,
			 gpointer        data)
{
	g_print ("connection_message_func(): %s\n",
		 dbus_message_get_member (message));

	if (IS_METHOD (message, DVD_CLIENT_METHOD_CALLBACK)) {
		DBusMessageIter iter;
		DBusMessage *reply;
		const gchar *callback;
		
		if (!dbus_message_iter_init (message, &iter)) {
			return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
		}

		dbus_message_iter_get_basic (&iter, &callback);
		dbus_message_iter_next (&iter);
		
		g_print ("CALLBACK: %s!!!\n", callback);

		reply = dbus_message_new_method_return (message);

		_mate_vfs_module_callback_demarshal_invoke (callback,
							     &iter,
							     reply);
		dbus_connection_send (dbus_conn, reply, NULL);
		dbus_connection_flush (dbus_conn);
		dbus_message_unref (reply);
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static MateVFSResult
do_open (MateVFSMethod        *method,
	 MateVFSMethodHandle **method_handle,
	 MateVFSURI           *uri,
	 MateVFSOpenMode       mode,
	 MateVFSContext       *context)
{
	MateVFSResult  result;
	DBusMessage    *reply;
	dbus_int32_t    handle_id;
	FileHandle     *handle;

	reply = execute_operation (DVD_DAEMON_METHOD_OPEN,
				   context, &result,
				   DBUS_TIMEOUT_OPEN_CLOSE,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_INT32, mode,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_INT32, &handle_id,
			       DBUS_TYPE_INVALID);

	handle = file_handle_new (handle_id);

	*method_handle = (MateVFSMethodHandle *) handle;

	dbus_message_unref (reply);

	return result;
}

static MateVFSResult
do_create (MateVFSMethod        *method,
	   MateVFSMethodHandle **method_handle,
	   MateVFSURI           *uri,
	   MateVFSOpenMode       mode,
	   gboolean               exclusive,
	   guint                  perm,
	   MateVFSContext       *context)
{
	MateVFSResult  result;
	DBusMessage    *reply;
	dbus_int32_t    handle_id;
	FileHandle     *handle;

	reply = execute_operation (DVD_DAEMON_METHOD_CREATE,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_INT32, mode,
				   DVD_TYPE_BOOL, exclusive,
				   DVD_TYPE_INT32, perm,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_INT32, &handle_id,
			       DBUS_TYPE_INVALID);

	handle = file_handle_new (handle_id);

	*method_handle = (MateVFSMethodHandle *) handle;

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_close (MateVFSMethod       *method,
	  MateVFSMethodHandle *method_handle,
	  MateVFSContext      *context)
{
	FileHandle     *handle;
        DBusMessage    *reply;
	MateVFSResult  result;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_CLOSE,
				   context, &result,
				   DBUS_TIMEOUT_OPEN_CLOSE,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	file_handle_free (handle);

	return MATE_VFS_OK;
}

static MateVFSResult
do_read (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 gpointer buffer,
	 MateVFSFileSize num_bytes,
	 MateVFSFileSize *bytes_read,
	 MateVFSContext *context)
{
	FileHandle      *handle;
	DBusMessage     *reply;
	MateVFSResult   result;
	gint             size;
	guchar          *data;
	DBusMessageIter  iter;
	DBusMessageIter  array_iter;
	int              type;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_READ,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_UINT64, num_bytes,
				   DVD_TYPE_LAST);

	*bytes_read = 0;

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_iter_init (reply, &iter);

	dbus_message_iter_next (&iter); /* Result is already checked. */

	type = dbus_message_iter_get_arg_type (&iter);
	if (type != DBUS_TYPE_ARRAY) {
		dbus_message_unref (reply);
		return MATE_VFS_ERROR_INTERNAL;
	}

	dbus_message_iter_recurse (&iter, &array_iter);
	dbus_message_iter_get_fixed_array (&array_iter, &data, &size);
	if (size > 0) {
		memcpy (buffer, data, size);
	} 

	*bytes_read = size;

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_write (MateVFSMethod *method,
	  MateVFSMethodHandle *method_handle,
	  gconstpointer buffer,
	  MateVFSFileSize num_bytes,
	  MateVFSFileSize *bytes_written,
	  MateVFSContext *context)
{
	FileHandle       *handle;
	DBusMessage      *reply;
	MateVFSResult    result;
	dbus_uint64_t     written;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_WRITE,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_BYTE_ARRAY, (dbus_int32_t) num_bytes,
				   (const unsigned char *) buffer,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_UINT64, &written,
			       DBUS_TYPE_INVALID);

	dbus_message_unref (reply);

	*bytes_written = written;

	return MATE_VFS_OK;
}

static MateVFSResult
do_seek (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSSeekPosition whence,
	 MateVFSFileOffset offset,
	 MateVFSContext *context)
{
	FileHandle     *handle;
        DBusMessage    *reply;
	MateVFSResult  result;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_SEEK,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_INT32, whence,
				   DVD_TYPE_INT64, offset,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}
	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_tell (MateVFSMethod *method,
	 MateVFSMethodHandle *method_handle,
	 MateVFSFileSize *offset_return)
{
	FileHandle     *handle;
        DBusMessage    *reply;
	MateVFSResult  result;
	dbus_int64_t    offset;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_TELL,
				   NULL, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}
	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_INT64, &offset,
			       DBUS_TYPE_INVALID);

	*offset_return = offset;

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_truncate_handle (MateVFSMethod *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSFileSize where,
		    MateVFSContext *context)
{
	FileHandle     *handle;
	DBusMessage    *reply;
	MateVFSResult  result;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_TRUNCATE_HANDLE,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_UINT64, where,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_open_directory (MateVFSMethod           *method,
		   MateVFSMethodHandle    **method_handle,
		   MateVFSURI              *uri,
		   MateVFSFileInfoOptions   options,
		   MateVFSContext          *context)
{
        DBusMessage     *reply;
	dbus_int32_t     handle_id;
	DirectoryHandle *handle;
	MateVFSResult   result;

	/*g_print ("thread: %p", (gpointer) pthread_self ());*/

	reply = execute_operation (DVD_DAEMON_METHOD_OPEN_DIRECTORY,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_INT32, options,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_INT32, &handle_id,
			       DBUS_TYPE_INVALID);

	handle = directory_handle_new (handle_id);

	*method_handle = (MateVFSMethodHandle *) handle;

	dbus_message_unref (reply);

	return result;
}

static MateVFSResult
do_close_directory (MateVFSMethod       *method,
		    MateVFSMethodHandle *method_handle,
		    MateVFSContext      *context)
{
	DirectoryHandle *handle;
        DBusMessage     *reply;
	MateVFSResult   result;

	handle = (DirectoryHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_CLOSE_DIRECTORY,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	directory_handle_free (handle);

	return MATE_VFS_OK;
}

static MateVFSResult
do_read_directory (MateVFSMethod       *method,
		   MateVFSMethodHandle *method_handle,
		   MateVFSFileInfo     *file_info,
		   MateVFSContext      *context)
{
	DirectoryHandle  *handle;
	MateVFSFileInfo *file_info_src;
	GList            *list;

	handle = (DirectoryHandle *) method_handle;

	if (handle->dirs == NULL) {
		DBusMessage    *reply;
		MateVFSResult  result;

		reply = execute_operation (DVD_DAEMON_METHOD_READ_DIRECTORY,
					   context, &result, -1,
					   DVD_TYPE_INT32, handle->handle_id,
					   DVD_TYPE_LAST);

		if (!reply) {
			return result;
		}

		if (check_if_reply_is_error (reply, &result)) {
			if (result != MATE_VFS_ERROR_EOF) {
				return result;
			}
		}

		list = dbus_utils_message_get_file_info_list (reply);

		dbus_message_unref (reply);

		handle->dirs = list;
		handle->current = list;

		d(g_print ("got list: %d\n", g_list_length (list)));

		/* If we get OK, and an empty list, it means that there are no
		 * more files.
		 */
		if (list == NULL) {
			return MATE_VFS_ERROR_EOF;
		}
	}

	if (handle->current) {
		file_info_src = handle->current->data;
		mate_vfs_file_info_copy (file_info, file_info_src);

		handle->current = handle->current->next;

		/* If this was the last in this chunk, read more files in the
		 * next call.
		 */
		if (!handle->current) {
			handle->dirs = NULL;
		}
	} else {
		return MATE_VFS_ERROR_EOF;
	}

	return MATE_VFS_OK;
}

static MateVFSFileInfo *
get_file_info_from_message (DBusMessage *message)
{
	MateVFSFileInfo *info;
	DBusMessageIter   iter;

	dbus_message_iter_init (message, &iter);

	/* Not interested in result */
	dbus_message_iter_next (&iter);

	info = mate_vfs_daemon_message_iter_get_file_info (&iter);
	if (!info) {
		return NULL;
	}

	return info;
}

static MateVFSResult
do_get_file_info (MateVFSMethod *method,
		  MateVFSURI *uri,
		  MateVFSFileInfo *file_info,
		  MateVFSFileInfoOptions options,
		  MateVFSContext *context)
{
        DBusMessage       *reply;
	MateVFSResult     result;
	MateVFSFileInfo  *info;

	reply = execute_operation (DVD_DAEMON_METHOD_GET_FILE_INFO,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_INT32, options,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	info = get_file_info_from_message (reply);
	dbus_message_unref (reply);

	if (!info) {
		return MATE_VFS_ERROR_INTERNAL;
	}

	mate_vfs_file_info_copy (file_info, info);
	mate_vfs_file_info_unref (info);

	return MATE_VFS_OK;
}

static MateVFSResult
do_get_file_info_from_handle (MateVFSMethod *method,
			      MateVFSMethodHandle *method_handle,
			      MateVFSFileInfo *file_info,
			      MateVFSFileInfoOptions options,
			      MateVFSContext *context)
{
	FileHandle        *handle;
        DBusMessage       *reply;
	MateVFSResult     result;
	MateVFSFileInfo  *info;

	handle = (FileHandle *) method_handle;

	reply = execute_operation (DVD_DAEMON_METHOD_GET_FILE_INFO_FROM_HANDLE,
				   context, &result, -1,
				   DVD_TYPE_INT32, handle->handle_id,
				   DVD_TYPE_INT32, options,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	info = get_file_info_from_message (reply);

	dbus_message_unref (reply);

	if (!info) {
		return MATE_VFS_ERROR_INTERNAL;
	}

	mate_vfs_file_info_copy (file_info, info);
	mate_vfs_file_info_unref (info);

	return MATE_VFS_OK;
}

static gboolean
do_is_local (MateVFSMethod *method, const MateVFSURI *uri)
{
        DBusMessage    *reply;
	MateVFSResult  result;
	dbus_bool_t     is_local;

	reply = execute_operation (DVD_DAEMON_METHOD_IS_LOCAL,
				   NULL, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_BOOLEAN, &is_local,
			       DBUS_TYPE_INVALID);

	dbus_message_unref (reply);

	return is_local;
}

static MateVFSResult
do_make_directory (MateVFSMethod  *method,
		   MateVFSURI     *uri,
		   guint            perm,
		   MateVFSContext *context)
{
        DBusMessage    *reply;
	MateVFSResult  result;

	/*g_print ("thread: %p", (gpointer) pthread_self ());*/

	reply = execute_operation (DVD_DAEMON_METHOD_MAKE_DIRECTORY,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_INT32, perm,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_remove_directory (MateVFSMethod  *method,
		     MateVFSURI     *uri,
		     MateVFSContext *context)
{
        DBusMessage    *reply;
	MateVFSResult  result;

	/*g_print ("thread: %p", (gpointer) pthread_self ());*/

	reply = execute_operation (DVD_DAEMON_METHOD_REMOVE_DIRECTORY,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_move (MateVFSMethod  *method,
	 MateVFSURI     *old_uri,
	 MateVFSURI     *new_uri,
	 gboolean         force_replace,
	 MateVFSContext *context)
{
        DBusMessage    *reply;
	MateVFSResult  result;

	/*g_print ("thread: %p", (gpointer) pthread_self ());*/

	reply = execute_operation (DVD_DAEMON_METHOD_MOVE,
				   context, &result, -1,
				   DVD_TYPE_URI, old_uri,
				   DVD_TYPE_URI, new_uri,
				   DVD_TYPE_BOOL, force_replace,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_unlink (MateVFSMethod  *method,
	   MateVFSURI     *uri,
	   MateVFSContext *context)
{
        DBusMessage    *reply;
	MateVFSResult  result;

	/*g_print ("thread: %p", (gpointer) pthread_self ());*/

	reply = execute_operation (DVD_DAEMON_METHOD_UNLINK,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_check_same_fs (MateVFSMethod  *method,
		  MateVFSURI     *a,
		  MateVFSURI     *b,
		  gboolean        *same_fs_return,
		  MateVFSContext *context)
{
	DBusMessage    *reply;
	MateVFSResult  result;
	dbus_bool_t     same_fs;

	reply = execute_operation (DVD_DAEMON_METHOD_CHECK_SAME_FS,
				   context, &result, -1,
				   DVD_TYPE_URI, a,
				   DVD_TYPE_URI, b,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_BOOLEAN, &same_fs,
			       DBUS_TYPE_INVALID);

	dbus_message_unref (reply);

	*same_fs_return = same_fs;

	return MATE_VFS_OK;
}

static MateVFSResult
do_set_file_info (MateVFSMethod          *method,
		  MateVFSURI             *uri,
		  const MateVFSFileInfo  *info,
		  MateVFSSetFileInfoMask  mask,
		  MateVFSContext         *context)
{
	DBusMessage    *reply;
	MateVFSResult  result;

	reply = execute_operation (DVD_DAEMON_METHOD_SET_FILE_INFO,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_FILE_INFO, info,
				   DVD_TYPE_INT32, mask,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_truncate (MateVFSMethod   *method,
	     MateVFSURI       *uri,
	     MateVFSFileSize  where,
	     MateVFSContext  *context)
{
	DBusMessage    *reply;
	MateVFSResult  result;

	reply = execute_operation (DVD_DAEMON_METHOD_TRUNCATE,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_UINT64, where,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_find_directory (MateVFSMethod             *method,
		   MateVFSURI                *near_uri,
		   MateVFSFindDirectoryKind   kind,
		   MateVFSURI               **result_uri,
		   gboolean                    create_if_needed,
		   gboolean                    find_if_needed,
		   guint                       permissions,
		   MateVFSContext            *context)
{
	DBusMessage    *reply;
	MateVFSResult  result;
	gchar          *uri_str;

	reply = execute_operation (DVD_DAEMON_METHOD_FIND_DIRECTORY,
				   context, &result, -1,
				   DVD_TYPE_URI, near_uri,
				   DVD_TYPE_INT32, kind,
				   DVD_TYPE_BOOL, create_if_needed,
				   DVD_TYPE_BOOL, find_if_needed,
				   DVD_TYPE_INT32, permissions,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}
	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_STRING, &uri_str,
			       DBUS_TYPE_INVALID);

	dbus_message_unref (reply);

	*result_uri = mate_vfs_uri_new (uri_str);

	return MATE_VFS_OK;
}

static MateVFSResult
do_create_symbolic_link (MateVFSMethod  *method,
			 MateVFSURI     *uri,
			 const char      *target_reference,
			 MateVFSContext *context)
{
	DBusMessage    *reply;
	MateVFSResult  result;

	reply = execute_operation (DVD_DAEMON_METHOD_CREATE_SYMBOLIC_LINK,
				   context, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_STRING, target_reference,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

typedef struct {
	gint32 id;
} MonitorHandle;

static GHashTable *active_monitors;

static DBusHandlerResult
dbus_filter_func (DBusConnection *connection,
		  DBusMessage    *message,
		  void           *data)
{
	DBusMessageIter        iter;
	dbus_int32_t           id, event_type;
	char *uri_str;

	if (dbus_message_is_signal (message,
				    DVD_DAEMON_INTERFACE,
				    DVD_DAEMON_MONITOR_SIGNAL)) {
		dbus_message_iter_init (message, &iter);

		if (dbus_message_get_args (message, NULL,
					   DBUS_TYPE_INT32, &id,
					   DBUS_TYPE_STRING, &uri_str,
					   DBUS_TYPE_INT32, &event_type,
					   DBUS_TYPE_INVALID)) {
			MateVFSURI *info_uri;
			info_uri = mate_vfs_uri_new (uri_str);
			if (info_uri != NULL) {
				MonitorHandle *handle;

				handle = g_hash_table_lookup (active_monitors, GINT_TO_POINTER (id));
				if (handle) {				
					mate_vfs_monitor_callback ((MateVFSMethodHandle *)handle,
								    info_uri, event_type);
				}
				mate_vfs_uri_unref (info_uri);
			}
		}
		
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}


static void
setup_monitor (void)
{
	static gboolean initialized = FALSE;
	DBusConnection *conn;

	if (initialized)
		return;
      
	initialized = TRUE;

	active_monitors = g_hash_table_new (g_direct_hash, g_direct_equal);
	
	conn = _mate_vfs_get_main_dbus_connection ();
	if (conn == NULL) {
		return;
	}

	dbus_connection_add_filter (conn,
				    dbus_filter_func,
				    NULL,
				    NULL);
}

static MateVFSResult
do_monitor_add (MateVFSMethod        *method,
		MateVFSMethodHandle **method_handle,
		MateVFSURI           *uri,
		MateVFSMonitorType    monitor_type)
{
	DBusMessage    *reply, *message;
	MateVFSResult  result;
	dbus_int32_t    id;
	DBusConnection *conn;
	MonitorHandle *handle;

	setup_monitor ();
	
	conn = _mate_vfs_get_main_dbus_connection ();
	if (conn == NULL)
		return MATE_VFS_ERROR_INTERNAL;
	
	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
						DVD_DAEMON_OBJECT,
						DVD_DAEMON_INTERFACE,
						DVD_DAEMON_METHOD_MONITOR_ADD);
	dbus_message_set_auto_start (message, TRUE);
	append_args (message,
		     DVD_TYPE_URI, uri,
		     DVD_TYPE_INT32, monitor_type,
		     DVD_TYPE_LAST);
	
	reply = dbus_connection_send_with_reply_and_block (conn, message,
							   -1, NULL);

	dbus_message_unref (message);

	if (reply == NULL) {
		return MATE_VFS_ERROR_INTERNAL;
	}
	
	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_INT32, &id,
			       DBUS_TYPE_INVALID);

	if (result == MATE_VFS_OK) {
		handle = g_new (MonitorHandle, 1);
		handle->id = id;
		*method_handle = (MateVFSMethodHandle *)handle;

		g_hash_table_insert (active_monitors, GINT_TO_POINTER (id), handle);
		
		dbus_message_unref (reply);

		return MATE_VFS_OK;
	} else {
		return result;
	}
}

static MateVFSResult
do_monitor_cancel (MateVFSMethod       *method,
		   MateVFSMethodHandle *method_handle)
{
	DBusMessage    *reply, *message;
	MateVFSResult  result;
	DBusConnection *conn;
	MonitorHandle *handle;
	gint32 id;

	handle = (MonitorHandle *)method_handle;
	id = handle->id;
	g_hash_table_remove (active_monitors, GINT_TO_POINTER (id));
	g_free (handle);
	
	conn = _mate_vfs_get_main_dbus_connection ();
	if (conn == NULL)
		return MATE_VFS_ERROR_INTERNAL;
	
	message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
						DVD_DAEMON_OBJECT,
						DVD_DAEMON_INTERFACE,
						DVD_DAEMON_METHOD_MONITOR_CANCEL);
	dbus_message_set_auto_start (message, TRUE);
	append_args (message,
		     DVD_TYPE_INT32, id,
		     DVD_TYPE_LAST);
	
	reply = dbus_connection_send_with_reply_and_block (conn, message, -1,
							   NULL);

	dbus_message_unref (message);

	if (reply == NULL) {
		return MATE_VFS_ERROR_INTERNAL;
	}
	
	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSResult
do_get_volume_free_space (MateVFSMethod    *method,
			  const MateVFSURI *uri,
			  MateVFSFileSize  *free_space)
{
	DBusMessage    *reply;
	MateVFSResult  result;
	dbus_uint64_t    space;

	reply = execute_operation (DVD_DAEMON_METHOD_GET_VOLUME_FREE_SPACE,
				   NULL, &result, -1,
				   DVD_TYPE_URI, uri,
				   DVD_TYPE_LAST);

	if (!reply) {
		return result;
	}

	if (check_if_reply_is_error (reply, &result)) {
		return result;
	}

	dbus_message_get_args (reply, NULL,
			       DBUS_TYPE_INT32, &result,
			       DBUS_TYPE_UINT64, &space,
			       DBUS_TYPE_INVALID);

	*free_space = space;

	dbus_message_unref (reply);

	return MATE_VFS_OK;
}

static MateVFSMethod method = {
        sizeof (MateVFSMethod),

        do_open,                      /* open */
        do_create,                    /* create */
        do_close,                     /* close */
        do_read,                      /* read */
        do_write,                     /* write */
        do_seek,                      /* seek */
        do_tell,                      /* tell */
        do_truncate_handle,           /* truncate_handle */
        do_open_directory,            /* open_directory */
        do_close_directory,           /* close_directory */
        do_read_directory,            /* read_directory */
	do_get_file_info,             /* get_file_info */
        do_get_file_info_from_handle, /* get_file_info_from_handle */
        do_is_local,                  /* is_local */
        do_make_directory,            /* make_directory */
        do_remove_directory,          /* remove_directory */
        do_move,                      /* move */
        do_unlink,                    /* unlink */
        do_check_same_fs,             /* check_same_fs */
        do_set_file_info,             /* set_file_info */
        do_truncate,                  /* truncate */
	do_find_directory,            /* find_directory */
        do_create_symbolic_link,      /* create_symbolic_link */
	do_monitor_add,               /* monitor_add */
        do_monitor_cancel,            /* monitor_cancel */
	NULL,                         /* file_control */
	NULL,                         /* forget_cache */
	do_get_volume_free_space      /* get_volume_free_space */
};

MateVFSMethod *
_mate_vfs_daemon_method_get (void)
{
  return &method;
}
