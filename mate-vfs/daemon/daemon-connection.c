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
#include <glib.h>
#include <libmatevfs/mate-vfs.h>
#include <libmatevfs/mate-vfs-cancellable-ops.h>
#include <libmatevfs/mate-vfs-dbus-utils.h>
#include <libmatevfs/mate-vfs-daemon-method.h>

#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE 1
#endif
#include <dbus/dbus-glib-lowlevel.h>

#include "dbus-utils.h"
#include "daemon-connection.h"

#define d(x) 

#define READDIR_CHUNK_SIZE 60

typedef struct _DaemonConnection DaemonConnection;

struct _DaemonConnection {
	DBusConnection *conn;
	gint32          conn_id;

	GMainContext   *main_context;
	GMainLoop      *main_loop;
};

typedef struct {
	MateVFSContext *context;
	gint32           id;
	gint32           conn_id;
} CancellationHandle;

typedef struct {
	DaemonConnection        *last_connection;
        MateVFSDirectoryHandle *vfs_handle;
	gint32                   id;
} DirectoryHandle;

typedef struct {
	DaemonConnection *last_connection;
        MateVFSHandle   *vfs_handle;
	gint32            id;
} FileHandle;

static GStaticMutex cancellations_lock = G_STATIC_MUTEX_INIT;
static GHashTable *cancellations;

static GStaticMutex directory_handles_lock = G_STATIC_MUTEX_INIT;
static GHashTable *directory_handles;
static guint next_directory_id = 1;

static GStaticMutex file_handles_lock = G_STATIC_MUTEX_INIT;
static GHashTable *file_handles;
static guint next_file_id = 1;

static DaemonConnection *  connection_new                      (DBusConnection          *dbus_conn,
								gint32                   conn_id);
static void                connection_destroy                  (DaemonConnection        *conn);
static CancellationHandle *cancellation_handle_new             (gint32                   cancellation_id,
								gint32                   conn_id);
static void                cancellation_handle_free            (CancellationHandle      *handle);
static CancellationHandle *connection_add_cancellation         (DaemonConnection        *conn,
								gint                     id);
static void                connection_remove_cancellation      (DaemonConnection        *conn,
								CancellationHandle      *handle);
static DirectoryHandle *   directory_handle_new                (DaemonConnection        *daemon_connection,
								MateVFSDirectoryHandle *vfs_handle,
								gint32                   handle_id);
static void                directory_handle_free               (DirectoryHandle         *handle);
static DirectoryHandle *   add_directory_handle                (DaemonConnection        *conn,
								MateVFSDirectoryHandle *vfs_handle);
static void                remove_directory_handle             (DirectoryHandle         *handle);
static DirectoryHandle *   get_directory_handle                (DaemonConnection        *conn,
								gint32                   id);
static FileHandle *        file_handle_new                     (DaemonConnection *daemon_connection,
								MateVFSHandle          *vfs_handle,
								gint32                   handle_id);
static void                file_handle_free                    (FileHandle              *handle);
static FileHandle *        add_file_handle                     (DaemonConnection        *conn,
								MateVFSHandle          *vfs_handle);
static void                remove_file_handle                  (FileHandle              *handle);
static FileHandle *        get_file_handle                     (DaemonConnection        *conn,
								gint32                   id);
static void                connection_reply_ok                 (DaemonConnection        *conn,
								DBusMessage             *message);
static void                connection_reply_result             (DaemonConnection        *conn,
								DBusMessage             *message,
								MateVFSResult           result);
static void                connection_reply_id                 (DaemonConnection        *conn,
								DBusMessage             *message,
								gint32                   id);
static gboolean            connection_check_and_reply_error    (DaemonConnection        *conn,
								DBusMessage             *message,
								MateVFSResult           result);
static void                connection_unregistered_func        (DBusConnection          *conn,
								gpointer                 data);
static DBusHandlerResult   connection_message_func             (DBusConnection          *conn,
								DBusMessage             *message,
								gpointer                 data);
static DBusHandlerResult   connection_message_filter           (DBusConnection          *conn,
								DBusMessage             *message,
								gpointer                 user_data);



static gboolean            get_operation_args                  (DBusMessage             *message,
								gint32                  *cancellation_id,
								DvdArgumentType          first_arg_type,
								...);


static DBusObjectPathVTable connection_vtable = {
	connection_unregistered_func,
	connection_message_func,
	NULL
};


static DaemonConnection *
connection_new (DBusConnection *dbus_conn,
		gint32 conn_id)
{
	DaemonConnection *conn;

	conn = g_new0 (DaemonConnection, 1);
	conn->conn_id = conn_id;

	conn->conn = dbus_conn;

	conn->main_context = g_main_context_new ();
	conn->main_loop = g_main_loop_new (conn->main_context, FALSE);

	if (!dbus_connection_register_object_path (dbus_conn,
						   DVD_DAEMON_OBJECT,
						   &connection_vtable,
						   conn)) {
		g_error ("Out of memory.");
	}

	dbus_connection_add_filter (dbus_conn, connection_message_filter,
				    conn, NULL);

	dbus_connection_setup_with_g_main (dbus_conn, conn->main_context);

	return conn;
}

static gpointer
connection_thread_func (DaemonConnection *conn)
{
        d(g_print ("New thread\n"));
	g_main_loop_run (conn->main_loop);
        d(g_print ("Thread done: Cleaning up\n"));

	connection_destroy (conn);

	return NULL;
}

void
daemon_connection_setup (DBusConnection               *dbus_conn,
			 gint32                        conn_id)
{
	DaemonConnection *conn;
	GThread          *thread;

	if (!dbus_connection_get_is_connected (dbus_conn)) {
		g_warning ("New connection is not connected.");
		return;
	}

	dbus_connection_ref (dbus_conn);
	conn = connection_new (dbus_conn, conn_id);

	thread = g_thread_create ((GThreadFunc)connection_thread_func,
                                  conn, FALSE, NULL);
}

static void
connection_shutdown (DaemonConnection *conn)
{
	g_main_loop_quit (conn->main_loop);
}

static gboolean
directory_handle_last_conn_is (gpointer  key,
                               gpointer  value,
                               gpointer  user_data)
{
	DaemonConnection *conn = user_data;
	DirectoryHandle *handle = value;

	return handle->last_connection == conn;
}

static gboolean
file_handle_last_conn_is (gpointer  key,
			  gpointer  value,
			  gpointer  user_data)
{
	DaemonConnection *conn = user_data;
	FileHandle *handle = value;

	return handle->last_connection == conn;
}

static void
connection_destroy (DaemonConnection *conn)
{
	d(g_print ("Connection destroy\n"));

	if (dbus_connection_get_is_connected (conn->conn)) {
		dbus_connection_close (conn->conn);
	}
	dbus_connection_unref (conn->conn);

	g_static_mutex_lock (&directory_handles_lock);
	if (directory_handles) {
		g_hash_table_foreach_remove (directory_handles,
					     directory_handle_last_conn_is,
					     conn);
	}
	g_static_mutex_unlock (&directory_handles_lock);
	
	g_static_mutex_lock (&file_handles_lock);
	if (file_handles) {
		g_hash_table_foreach_remove (file_handles,
					     file_handle_last_conn_is,
					     conn);
	}
	g_static_mutex_unlock (&file_handles_lock);
	
	g_assert (!g_main_loop_is_running (conn->main_loop));

	g_main_loop_unref (conn->main_loop);
	g_main_context_unref (conn->main_context);

	g_free (conn);
}

static CancellationHandle *
cancellation_handle_new (gint32 id, gint32 conn_id)
{
	CancellationHandle *handle;

	handle = g_new0 (CancellationHandle, 1);
	handle->id = id;
	handle->conn_id = conn_id;

	handle->context = mate_vfs_context_new ();

	return handle;
}

static void
cancellation_handle_free (CancellationHandle *handle)
{
        d(g_print ("Freeing cancellation handle\n"));

	if (handle->context) {
		mate_vfs_context_free (handle->context);
		handle->context = NULL;
	}
	
	g_free (handle);
}

static guint
cancellation_handle_hash (gconstpointer  key)
{
	const CancellationHandle *h;

	h = key;

	return h->id << 16 | h->conn_id;
}

static gboolean
cancellation_handle_equal (gconstpointer  a,
			   gconstpointer  b)
{
	const CancellationHandle *aa, *bb;
	aa = a;
	bb = b;

	return aa->id == bb->id && aa->conn_id == bb->conn_id;
}

static CancellationHandle *
connection_add_cancellation (DaemonConnection *conn, gint32 id)
{
	CancellationHandle *handle;

	d(g_print ("Adding cancellation handle %d (%p)\n", id, conn));

	handle = cancellation_handle_new (id, conn->conn_id);
	
	g_static_mutex_lock (&cancellations_lock);
	if (cancellations == NULL) {
		cancellations = g_hash_table_new_full (cancellation_handle_hash, cancellation_handle_equal,
						       NULL, (GDestroyNotify) cancellation_handle_free);
	}
	
	g_hash_table_insert (cancellations, handle, handle);
	g_static_mutex_unlock (&cancellations_lock);

	return handle;
}

static void
connection_remove_cancellation (DaemonConnection   *conn,
				CancellationHandle *handle)
{
	d(g_print ("Removing cancellation handle %d\n", handle->id));

	g_static_mutex_lock (&cancellations_lock);
	if (!g_hash_table_remove (cancellations, handle)) {
		g_warning ("Could't remove cancellation.");
	}
	g_static_mutex_unlock (&cancellations_lock);
}

/* Note: This is called from the main thread. */
void
daemon_connection_cancel (gint32 conn_id, gint32 cancellation_id)
{
	CancellationHandle *handle, lookup;
	MateVFSCancellation *cancellation;

	handle = NULL;

	g_static_mutex_lock (&cancellations_lock);
	
	lookup.conn_id = conn_id;
	lookup.id = cancellation_id;
	if (cancellations != NULL) {
		handle = g_hash_table_lookup (cancellations, &lookup);
	}
	if (handle != NULL) {
		cancellation = mate_vfs_context_get_cancellation (handle->context);
		if (cancellation) {
			mate_vfs_cancellation_cancel (cancellation);
		}
	}
	
	g_static_mutex_unlock (&cancellations_lock);
}


/*
 * DirectoryHandle functions.
 */

static DirectoryHandle *
directory_handle_new (DaemonConnection        *daemon_connection,
		      MateVFSDirectoryHandle *vfs_handle,
		      gint32                   handle_id)
{
	DirectoryHandle *handle;

	handle = g_new0 (DirectoryHandle, 1);
	handle->last_connection = daemon_connection;
	handle->vfs_handle = vfs_handle;
	handle->id = handle_id;

	return handle;
}

static void
directory_handle_free (DirectoryHandle *handle)
{
	if (handle->vfs_handle) {
		mate_vfs_directory_close (handle->vfs_handle);
		handle->vfs_handle = NULL;
	}

	g_free (handle);
}

static DirectoryHandle *
add_directory_handle (DaemonConnection        *conn,
		      MateVFSDirectoryHandle *vfs_handle)
{
	DirectoryHandle *handle;

	g_static_mutex_lock (&directory_handles_lock);
	handle = directory_handle_new (conn, vfs_handle, next_directory_id++);

	if (directory_handles == NULL) {
		directory_handles = g_hash_table_new_full (g_direct_hash, g_direct_equal,
							   NULL, (GDestroyNotify) directory_handle_free);
	}
	
	g_hash_table_insert (directory_handles,
			     GINT_TO_POINTER (handle->id), handle);
	g_static_mutex_unlock (&directory_handles_lock);

	return handle;
}

static void
remove_directory_handle (DirectoryHandle  *handle)
{
	g_static_mutex_lock (&directory_handles_lock);
	if (!g_hash_table_remove (directory_handles,
				  GINT_TO_POINTER (handle->id))) {
		g_warning ("Couldn't remove directory handle %d\n",
			   handle->id);
	}
	g_static_mutex_unlock (&directory_handles_lock);
}

static DirectoryHandle *
get_directory_handle (DaemonConnection *conn,
		      gint32            id)
{
	DirectoryHandle *handle;
	
	g_static_mutex_lock (&directory_handles_lock);
	handle = g_hash_table_lookup (directory_handles,
				      GINT_TO_POINTER (id));
	if (handle) {
		handle->last_connection = conn;
	}
	g_static_mutex_unlock (&directory_handles_lock);
	return handle;
}

/*
 * FileHandle functions.
 */

static FileHandle *
file_handle_new (DaemonConnection *daemon_connection,
		 MateVFSHandle   *vfs_handle,
		 gint32            handle_id)
{
	FileHandle *handle;

	handle = g_new0 (FileHandle, 1);
	handle->last_connection = daemon_connection;
	handle->vfs_handle = vfs_handle;
	handle->id = handle_id;

	return handle;
}

static void
file_handle_free (FileHandle *handle)
{
	if (handle->vfs_handle) {
		mate_vfs_close (handle->vfs_handle);
		handle->vfs_handle = NULL;
	}
	
	g_free (handle);
}

static FileHandle *
add_file_handle (DaemonConnection *conn,
		 MateVFSHandle   *vfs_handle)
{
	FileHandle *handle;

	g_static_mutex_lock (&file_handles_lock);
	handle = file_handle_new (conn, vfs_handle, next_file_id++);

	if (file_handles == NULL) {
		file_handles = g_hash_table_new_full (g_direct_hash, g_direct_equal,
						      NULL, (GDestroyNotify) file_handle_free);
	}
	
	g_hash_table_insert (file_handles,
			     GINT_TO_POINTER (handle->id), handle);
	g_static_mutex_unlock (&file_handles_lock);

	return handle;
}

static void
remove_file_handle (FileHandle       *handle)
{
	g_static_mutex_lock (&file_handles_lock);
	if (!g_hash_table_remove (file_handles,
				  GINT_TO_POINTER (handle->id))) {
		g_warning ("Couldn't remove file handle %d\n", handle->id);
	}
	g_static_mutex_unlock (&file_handles_lock);
}

static FileHandle *
get_file_handle (DaemonConnection *conn,
		 gint32            id)
{
	FileHandle *handle;

	g_static_mutex_lock (&file_handles_lock);
	handle = g_hash_table_lookup (file_handles,
				      GINT_TO_POINTER (id));
	if (handle) {
		handle->last_connection = conn;
	}
	g_static_mutex_unlock (&file_handles_lock);
	
	return handle;
}

/*
 * Reply functions.
 */

static DBusMessage *
create_reply_helper (DBusMessage    *message,
		     MateVFSResult  result)
{
	DBusMessage     *reply;
	DBusMessageIter  iter;
	gint32           i;

	reply = dbus_message_new_method_return (message);
	if (!reply) {
		g_error ("Out of memory");
	}

	i = result;

	dbus_message_iter_init_append (reply, &iter);
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_INT32,
					     &i)) {
		g_error ("Out of memory");
	}

	return reply;
}

static DBusMessage *
connection_create_reply_ok (DBusMessage *message)
{
	return create_reply_helper (message, MATE_VFS_OK);
}

static void
connection_reply_ok (DaemonConnection *conn,
		     DBusMessage      *message)
{
	DBusMessage *reply;

	reply = connection_create_reply_ok (message);


	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_reply_result (DaemonConnection *conn,
			 DBusMessage      *message,
			 MateVFSResult    result)
{
	DBusMessage *reply;

	reply = create_reply_helper (message, result);

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_reply_id (DaemonConnection *conn,
		     DBusMessage      *message,
		     gint32            id)
{
	DBusMessage     *reply;
	DBusMessageIter  iter;

	reply = create_reply_helper (message, MATE_VFS_OK);

	dbus_message_iter_init_append (reply, &iter);
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_INT32,
					     &id)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static gboolean
connection_check_and_reply_error (DaemonConnection *conn,
				  DBusMessage      *message,
				  MateVFSResult    result)
{
	if (result == MATE_VFS_OK) {
		return FALSE;
	}

	d(g_print ("ERROR: %s\n", mate_vfs_result_to_string (result)));
	connection_reply_result (conn, message, result);

	return TRUE;
}


/*
 * Daemon protocol implementation
 */

static void
connection_handle_open (DaemonConnection *conn,
			DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              mode;
	MateVFSURI        *uri;
	MateVFSHandle     *vfs_handle;
	MateVFSResult      result;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &mode,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_open_uri_cancellable (&vfs_handle,
						 uri,
						 mode,
						 context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (uri);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	handle = add_file_handle (conn, vfs_handle);

	connection_reply_id (conn, message, handle->id);
}

static void
connection_handle_create (DaemonConnection *conn,
			  DBusMessage      *message)
{
	gint32              cancellation_id;
	MateVFSURI        *uri;
	gint32              mode;
	gboolean            exclusive;
	gint32              perm;
	MateVFSHandle     *vfs_handle;
	MateVFSResult      result;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &mode,
				 DVD_TYPE_BOOL, &exclusive,
				 DVD_TYPE_INT32, &perm,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("create: %s, %d, %d, %d (%d)\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE),
		   mode, exclusive, perm, cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_create_uri_cancellable (&vfs_handle,
						   uri,
						   mode,
						   exclusive,
						   perm,
						   context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (uri);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	handle = add_file_handle (conn, vfs_handle);

	connection_reply_id (conn, message, handle->id);
}

static void
connection_handle_close (DaemonConnection *conn,
			 DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	MateVFSResult      result;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("close: %d (%d)\n", handle_id, cancellation_id));

	handle = get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_close_cancellable (handle->vfs_handle,
					      context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	/* Clear the handle so we don't close it twice. */
	handle->vfs_handle = NULL;
	
	remove_file_handle (handle);

	connection_reply_ok (conn, message);
}

static void
connection_handle_read (DaemonConnection *conn,
			DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	guint64             num_bytes;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	MateVFSResult      result;
	MateVFSFileSize    bytes_read;
	DBusMessage        *reply;
	DBusMessageIter     iter;
	DBusMessageIter     array_iter;
	gpointer            buf;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_UINT64, &num_bytes,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("read: %d, %llu, (%d)\n",
		   handle_id, num_bytes, cancellation_id));

	handle = get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	buf = g_malloc (num_bytes);

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_read_cancellable (handle->vfs_handle,
					     buf,
					     num_bytes,
					     &bytes_read,
					     context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	if (connection_check_and_reply_error (conn, message, result)) {
		g_free (buf);
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	if (!dbus_message_iter_open_container (&iter,
					       DBUS_TYPE_ARRAY,
					       DBUS_TYPE_BYTE_AS_STRING,
					       &array_iter)) {
		g_error ("Out of memory");
	}
	
	if (!dbus_message_iter_append_fixed_array (&array_iter,
						   DBUS_TYPE_BYTE,
						   &buf,
						   bytes_read)) {
		g_error ("Out of memory");
	}

	if (!dbus_message_iter_close_container (&iter, &array_iter)) {
		g_error ("Out of memory");
	}
	
	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);

	g_free (buf);
}

static void
connection_handle_write (DaemonConnection *conn,
			 DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	MateVFSResult      result;
	DBusMessage        *reply;
	DBusMessageIter     iter;
	unsigned char      *buf;
	gint                len;
	MateVFSFileSize    bytes_written;
	guint64             ui64;
	MateVFSContext    *context;
	
	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_BYTE_ARRAY, &buf, &len,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("write: %d, %d (%d)\n", handle_id, len, cancellation_id));

	handle = get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_write_cancellable (handle->vfs_handle,
					      buf,
					      len,
					      &bytes_written,
					      context);
	mate_vfs_daemon_set_current_connection (NULL);
	

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	ui64 = bytes_written;
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_UINT64,
					     &ui64)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_seek (DaemonConnection *conn,
			DBusMessage      *message)
{
	gint32              handle_id;
	gint32              cancellation_id;
	gint32              whence;
	gint64              offset;
	CancellationHandle *cancellation;
	FileHandle         *handle;
	MateVFSResult      result;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_INT32, &whence,
				 DVD_TYPE_INT64, &offset,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("seek: %d, %d, %llu\n", handle_id, whence, offset));

	handle = get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_seek_cancellable (handle->vfs_handle,
					     whence,
					     offset,
					     context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	connection_reply_result (conn, message, result);
}

static void
connection_handle_tell (DaemonConnection *conn,
			DBusMessage      *message)
{
	gint32            handle_id;
	FileHandle       *handle;
	MateVFSResult    result;
	MateVFSFileSize  offset;
	DBusMessage      *reply;
	DBusMessageIter   iter;
	gint64            i64;

	if (!get_operation_args (message, NULL,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("tell: %d\n", handle_id));

	handle = get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_tell (handle->vfs_handle, &offset);
	mate_vfs_daemon_set_current_connection (NULL);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	i64 = offset;
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_INT64,
					     &i64)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_truncate_handle (DaemonConnection *conn,
				   DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	guint64             where;
	CancellationHandle *cancellation;
	FileHandle         *handle;
	MateVFSResult      result;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_UINT64, &where,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("truncate_handle: %d, %llu\n", handle_id, where));

	handle = get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_truncate_handle_cancellable (handle->vfs_handle,
							where,
							context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	connection_reply_result (conn, message, result);
}

static void
connection_handle_open_directory (DaemonConnection *conn,
				  DBusMessage      *message)
{
	gint                     options;
	gint32                   cancellation_id;
	MateVFSURI             *uri;
	MateVFSDirectoryHandle *vfs_handle;
	MateVFSResult           result;
	DirectoryHandle         *handle;
	CancellationHandle      *cancellation;
	MateVFSContext         *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &options,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("open_directory: %s, %d (%d)\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE),
		   options, cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_directory_open_from_uri_cancellable (&vfs_handle,
								uri,
								options,
								context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (uri);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	handle = add_directory_handle (conn, vfs_handle);

	connection_reply_id (conn, message, handle->id);
}

static void
connection_handle_close_directory (DaemonConnection *conn,
				   DBusMessage      *message)
{
	gint32           handle_id;
	gint32           cancellation_id;
	DirectoryHandle *handle;
	MateVFSResult   result;

	/* Note: We get a cancellation id but don't use it. */

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("close_directory: %d\n", handle_id));

	handle = get_directory_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_directory_close (handle->vfs_handle);
	mate_vfs_daemon_set_current_connection (NULL);

	if (result == MATE_VFS_OK) {
		/* Clear the handle so we don't close it twice. */
		handle->vfs_handle = NULL;
		remove_directory_handle (handle);
	}

	connection_reply_result (conn, message, result);
}

static void
connection_handle_read_directory (DaemonConnection *conn,
				  DBusMessage      *message)
{
	gint32              handle_id;
	gint32              cancellation_id;
	DirectoryHandle    *handle;
	DBusMessage        *reply;
	MateVFSFileInfo   *info;
	MateVFSResult      result;
	CancellationHandle *cancellation;
	MateVFSContext    *context;
	gint                num_entries;
	DBusMessageIter     iter;
	DBusMessageIter     array_iter;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("read_directory: %d (%d)\n",
		   handle_id, cancellation_id));

	handle = get_directory_handle (conn, handle_id);

	if (!handle) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	reply = connection_create_reply_ok (message);

	info = mate_vfs_file_info_new ();

	dbus_message_iter_init_append (reply, &iter);
	if (!dbus_message_iter_open_container (&iter,
					       DBUS_TYPE_ARRAY,
					       MATE_VFS_FILE_INFO_DBUS_TYPE,
					       &array_iter)) {
		g_error ("Out of memory");
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	
	result = MATE_VFS_OK;
	num_entries = 0;
	while ((result = mate_vfs_directory_read_next_cancellable (handle->vfs_handle, info, context)) == MATE_VFS_OK) {
		mate_vfs_daemon_message_iter_append_file_info (&array_iter, info);
		mate_vfs_file_info_clear (info);

		if (context && mate_vfs_context_check_cancellation (context)) {
			result = MATE_VFS_ERROR_CANCELLED;
			break;
		}

		if (num_entries++ == READDIR_CHUNK_SIZE) {
			break;
		}
	}

	mate_vfs_daemon_set_current_connection (NULL);
	
	if (!dbus_message_iter_close_container (&iter, &array_iter)) {
		g_error ("Out of memory");
	}
	
	mate_vfs_file_info_unref (info);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}
	
	if (result == MATE_VFS_OK || result == MATE_VFS_ERROR_EOF) {
		dbus_connection_send (conn->conn, reply, NULL);
		dbus_message_unref (reply);
	} else {
		dbus_message_unref (reply);
		connection_reply_result (conn, message, result);
	}
}

static void
connection_handle_get_file_info (DaemonConnection *conn,
				 DBusMessage      *message)
{
	gint32              cancellation_id;
	MateVFSURI        *uri;
	gint32              options;
	DBusMessage        *reply;
	MateVFSFileInfo   *info;
	MateVFSResult      result;
	CancellationHandle *cancellation;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &options,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("get_file_info: %s (%d)\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE),
		   cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	info = mate_vfs_file_info_new ();

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_get_file_info_uri_cancellable (uri,
							  info,
							  options,
							  context);
	mate_vfs_uri_unref (uri);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	if (connection_check_and_reply_error (conn, message, result)) {
		mate_vfs_file_info_unref (info);
		return;
	}

	reply = connection_create_reply_ok (message);

	mate_vfs_daemon_message_append_file_info (reply, info);

	mate_vfs_file_info_unref (info);

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_get_file_info_from_handle (DaemonConnection *conn,
					     DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	gint32              options;
	DBusMessage        *reply;
	FileHandle         *handle;
	CancellationHandle *cancellation;
	MateVFSFileInfo   *info;
	MateVFSResult      result;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_INT32, &options,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("get_file_info_from_handle: %d (%d)\n",
		   handle_id, cancellation_id));

	handle = get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	info = mate_vfs_file_info_new ();

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_get_file_info_from_handle_cancellable (
		handle->vfs_handle, info, options, context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	if (connection_check_and_reply_error (conn, message, result)) {
		mate_vfs_file_info_unref (info);
		return;
	}

	reply = connection_create_reply_ok (message);

	mate_vfs_daemon_message_append_file_info (reply, info);

	mate_vfs_file_info_unref (info);

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_is_local (DaemonConnection *conn,
			    DBusMessage      *message)
{
	MateVFSURI     *uri;
	gboolean         is_local;
	DBusMessage     *reply;
	DBusMessageIter  iter;

	if (!get_operation_args (message, NULL,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("is_local: %s\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE)));

	mate_vfs_daemon_set_current_connection (conn->conn);
	is_local = mate_vfs_uri_is_local (uri);
	mate_vfs_daemon_set_current_connection (NULL);
	mate_vfs_uri_unref (uri);

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_BOOLEAN,
					     &is_local)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_connection_flush (conn->conn);
	dbus_message_unref (reply);
}

static void
connection_handle_make_directory (DaemonConnection *conn,
				  DBusMessage      *message)
{
	gint32              cancellation_id;
	MateVFSURI        *uri;
	gint                perm;
	MateVFSResult      result;
	CancellationHandle *cancellation;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &perm,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("make_directory: %s, %d (%d)\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE),
		   perm, cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_make_directory_for_uri_cancellable (
		uri, perm, context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_remove_directory (DaemonConnection *conn,
				    DBusMessage      *message)
{
	gint32              cancellation_id;
	MateVFSURI        *uri;
	MateVFSResult      result;
	CancellationHandle *cancellation;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("remove_directory: %s, (%d)\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE),
		   cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_remove_directory_from_uri_cancellable (
		uri, context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_move (DaemonConnection *conn,
			DBusMessage      *message)
{
	gint32              cancellation_id;
	MateVFSURI        *old_uri;
	MateVFSURI        *new_uri;
	gboolean            force_replace;
	MateVFSResult      result;
	CancellationHandle *cancellation;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &old_uri,
				 DVD_TYPE_URI, &new_uri,
				 DVD_TYPE_BOOL, &force_replace,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("move: %s, %s %d (%d)\n",
		   mate_vfs_uri_to_string (old_uri, MATE_VFS_URI_HIDE_NONE),
		   mate_vfs_uri_to_string (new_uri, MATE_VFS_URI_HIDE_NONE),
		   force_replace, cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_move_uri_cancellable (old_uri,
						 new_uri,
						 force_replace,
						 context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (old_uri);
	mate_vfs_uri_unref (new_uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_unlink (DaemonConnection *conn,
			  DBusMessage      *message)
{
	gint32              cancellation_id;
	MateVFSURI        *uri;
	MateVFSResult      result;
	CancellationHandle *cancellation;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("unlink: %s (%d)\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE),
		   cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_unlink_from_uri_cancellable (uri, context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_check_same_fs (DaemonConnection *conn,
				 DBusMessage      *message)
{
	gint                cancellation_id;
	MateVFSURI        *uri_a;
	MateVFSURI        *uri_b;
	CancellationHandle *cancellation;
	gboolean            is_same;
	MateVFSResult      result;
	DBusMessage        *reply;
	DBusMessageIter     iter;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri_a,
				 DVD_TYPE_URI, &uri_b,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("check_same_fs: %s, %s\n",
		   mate_vfs_uri_to_string (uri_a, MATE_VFS_URI_HIDE_NONE),
		   mate_vfs_uri_to_string (uri_b, MATE_VFS_URI_HIDE_NONE)));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_check_same_fs_uris_cancellable (uri_a, uri_b,
							   &is_same,
							   context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (uri_a);
	mate_vfs_uri_unref (uri_b);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_BOOLEAN,
					     &is_same)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_set_file_info (DaemonConnection *conn,
				 DBusMessage      *message)
{
	gint32              cancellation_id;
	MateVFSURI        *uri;
	gint32              mask;
	MateVFSResult      result;
	CancellationHandle *cancellation;
	MateVFSFileInfo   *info;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_FILE_INFO, &info,
				 DVD_TYPE_INT32, &mask,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("set_file_info: %s, %d (%d)\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE),
		   mask,
		   cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_set_file_info_cancellable (uri, info, mask, context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_truncate (DaemonConnection *conn,
			    DBusMessage      *message)
{
	gint32              cancellation_id;
	MateVFSURI        *uri;
	guint64             where;
	MateVFSResult      result;
	CancellationHandle *cancellation;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_UINT64, &where,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("truncate: %s %llu (%d)\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE),
		   where,
		   cancellation_id));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_truncate_uri_cancellable (uri, where, context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (uri);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_find_directory (DaemonConnection *conn,
				  DBusMessage      *message)
{
	gint                cancellation_id;
	MateVFSURI        *uri, *result_uri;
	gint32              kind;
	gboolean            create_if_needed;
	gboolean            find_if_needed;
	gint32              perm;
	CancellationHandle *cancellation;
	MateVFSResult      result;
	DBusMessage        *reply;
	DBusMessageIter     iter;
	gchar              *str;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_INT32, &kind,
				 DVD_TYPE_BOOL, &create_if_needed,
				 DVD_TYPE_BOOL, &find_if_needed,
				 DVD_TYPE_INT32, &perm,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("find_directory: %s\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE)));

	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_find_directory_cancellable (uri,
						       kind,
						       &result_uri,
						       create_if_needed,
						       find_if_needed,
						       perm,
						       context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (uri);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_message_iter_init_append (reply, &iter);

	str = mate_vfs_uri_to_string (result_uri, MATE_VFS_URI_HIDE_NONE);
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_STRING,
					     &str)) {
		g_error ("Out of memory");
	}
	g_free (str);

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_create_symbolic_link (DaemonConnection *conn,
					DBusMessage      *message)
{
	gint32              cancellation_id;
	MateVFSURI        *uri;
	gchar              *target;
	MateVFSResult      result;
	CancellationHandle *cancellation;
	MateVFSContext    *context;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_STRING, &target,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("create_symbolic_link: %s %s (%d)\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE),
		   target,
		   cancellation_id));


	if (cancellation_id != -1) {
		cancellation = connection_add_cancellation (conn, cancellation_id);
		context = cancellation->context;
	} else {
		cancellation = NULL;
		context = NULL;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_create_symbolic_link_cancellable (uri,
							     target,
							     context);
	mate_vfs_daemon_set_current_connection (NULL);

	if (cancellation) {
		connection_remove_cancellation (conn, cancellation);
	}

	mate_vfs_uri_unref (uri);
	g_free (target);

	connection_reply_result (conn, message, result);
}

static void
connection_handle_forget_cache (DaemonConnection *conn,
				DBusMessage      *message)
{
	gint32              cancellation_id;
	gint32              handle_id;
	gint64              offset;
	guint64             size;
	FileHandle         *handle;
	MateVFSResult      result;
	DBusMessage        *reply;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_INT32, &handle_id,
				 DVD_TYPE_INT64, &offset,
				 DVD_TYPE_UINT64, &size,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);

		return;
	}

	d(g_print ("forget cache: %d, %lld, %llu, (%d)\n",
		   handle_id, offset, size, cancellation_id));

	handle = get_file_handle (conn, handle_id);
	if (!handle) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_forget_cache (handle->vfs_handle,
					 offset,
					 size);
	mate_vfs_daemon_set_current_connection (NULL);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_handle_get_volume_free_space (DaemonConnection *conn,
					 DBusMessage      *message)
{
	gint32            cancellation_id;
	MateVFSURI      *uri;
	MateVFSResult    result;
	MateVFSFileSize  size;
	DBusMessage      *reply;
	DBusMessageIter   iter;
	guint64           ui64;

	if (!get_operation_args (message, &cancellation_id,
				 DVD_TYPE_URI, &uri,
				 DVD_TYPE_LAST)) {
		connection_reply_result (conn, message,
					 MATE_VFS_ERROR_INTERNAL);
		return;
	}

	d(g_print ("get_volume_free_space: %s (%d)\n",
		   mate_vfs_uri_to_string (uri, MATE_VFS_URI_HIDE_NONE),
		   cancellation_id));

	mate_vfs_daemon_set_current_connection (conn->conn);
	result = mate_vfs_get_volume_free_space (uri, &size);
	mate_vfs_daemon_set_current_connection (NULL);

	mate_vfs_uri_unref (uri);

	if (connection_check_and_reply_error (conn, message, result)) {
		return;
	}

	reply = connection_create_reply_ok (message);
	dbus_message_iter_init_append (reply, &iter);

	ui64 = size;
	if (!dbus_message_iter_append_basic (&iter,
					     DBUS_TYPE_UINT64,
					     &ui64)) {
		g_error ("Out of memory");
	}

	dbus_connection_send (conn->conn, reply, NULL);
	dbus_message_unref (reply);
}

static void
connection_unregistered_func (DBusConnection *conn,
			      gpointer        data)
{
}

#define IS_METHOD(msg,method) \
  dbus_message_is_method_call(msg,DVD_DAEMON_INTERFACE,method)

static DBusHandlerResult
connection_message_func (DBusConnection *dbus_conn,
			 DBusMessage    *message,
			 gpointer        data)
{
	DaemonConnection *conn;

	conn = data;

	/*g_print ("connection_message_func(): %s\n",
	  dbus_message_get_member (message));*/

	if (IS_METHOD (message, DVD_DAEMON_METHOD_OPEN)) {
		connection_handle_open (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_CREATE)) {
		connection_handle_create (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_CLOSE)) {
		connection_handle_close (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_READ)) {
		connection_handle_read (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_WRITE)) {
		connection_handle_write (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_SEEK)) {
		connection_handle_seek (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_TELL)) {
		connection_handle_tell (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_TRUNCATE_HANDLE)) {
		connection_handle_truncate_handle (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_OPEN_DIRECTORY)) {
		connection_handle_open_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_READ_DIRECTORY)) {
		connection_handle_read_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_CLOSE_DIRECTORY)) {
		connection_handle_close_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_GET_FILE_INFO)) {
		connection_handle_get_file_info (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_GET_FILE_INFO_FROM_HANDLE)) {
		connection_handle_get_file_info_from_handle (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_IS_LOCAL)) {
		connection_handle_is_local (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_MAKE_DIRECTORY)) {
		connection_handle_make_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_REMOVE_DIRECTORY)) {
		connection_handle_remove_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_MOVE)) {
		connection_handle_move (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_UNLINK)) {
		connection_handle_unlink (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_CHECK_SAME_FS)) {
		connection_handle_check_same_fs (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_SET_FILE_INFO)) {
		connection_handle_set_file_info (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_TRUNCATE)) {
		connection_handle_truncate (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_FIND_DIRECTORY)) {
		connection_handle_find_directory (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_CREATE_SYMBOLIC_LINK)) {
		connection_handle_create_symbolic_link (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_FORGET_CACHE)) {
		connection_handle_forget_cache (conn, message);
	}
	else if (IS_METHOD (message, DVD_DAEMON_METHOD_GET_VOLUME_FREE_SPACE)) {
		connection_handle_get_volume_free_space (conn, message);
	} else {
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
connection_message_filter (DBusConnection *conn,
			   DBusMessage    *message,
			   gpointer        user_data)
{
	DaemonConnection *connection;

	connection = user_data;

	/*g_print ("connection_message_filter: %s\n",
	  dbus_message_get_member (message));*/

	if (dbus_message_is_signal (message,
				    DBUS_INTERFACE_LOCAL,
				    "Disconnected")) {
		d(g_print ("Disconnected ***\n"));
		connection_shutdown (connection);
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean
get_operation_args (DBusMessage     *message,
		    gint32          *cancellation_id,
		    DvdArgumentType  first_arg_type,
		    ...)
{
	DBusMessageIter iter;
	va_list         args;
	DvdArgumentType type;
	gint            dbus_type;
	gboolean        has_next;

	dbus_message_iter_init (message, &iter);

	va_start (args, first_arg_type);

	has_next = TRUE;
	type = first_arg_type;
	while (type != DVD_TYPE_LAST) {
		dbus_type = dbus_message_iter_get_arg_type (&iter);

		switch (type) {
		case DVD_TYPE_URI: {
			MateVFSURI **uri;
			gchar        *str;

			if (dbus_type != DBUS_TYPE_STRING) {
				goto fail;
			}

			uri = va_arg (args, MateVFSURI **);

			dbus_message_iter_get_basic (&iter, &str);

			*uri = mate_vfs_uri_new (str);

			if (!*uri) {
				goto fail;
			}

			break;
		}
		case DVD_TYPE_STRING: {
			gchar *str;
			gchar **ret_val;

			if (dbus_type != DBUS_TYPE_STRING) {
				goto fail;
			}

			ret_val = va_arg (args, gchar **);
			dbus_message_iter_get_basic (&iter, &str);
			if (!str) {
				g_error ("Out of memory");
			}

			*ret_val = g_strdup (str);
			break;
		}
		case DVD_TYPE_INT32: {
			gint32 *ret_val;

			if (dbus_type != DBUS_TYPE_INT32) {
				goto fail;
			}

			ret_val = va_arg (args, gint32 *);
			dbus_message_iter_get_basic (&iter, ret_val);
			break;
		}
		case DVD_TYPE_INT64: {
			gint64 *ret_val;

			if (dbus_type != DBUS_TYPE_INT64) {
				goto fail;
			}

			ret_val = va_arg (args, gint64 *);
			dbus_message_iter_get_basic (&iter, ret_val);
			break;
		}
		case DVD_TYPE_UINT64: {
			guint64 *ret_val;

			if (dbus_type != DBUS_TYPE_UINT64) {
				goto fail;
			}

			ret_val = va_arg (args, guint64 *);
			dbus_message_iter_get_basic (&iter, ret_val);
			break;
		}
		case DVD_TYPE_BOOL: {
			gboolean *ret_val;

			if (dbus_type != DBUS_TYPE_BOOLEAN) {
				goto fail;
			}

			ret_val = va_arg (args, gboolean *);
			dbus_message_iter_get_basic (&iter, ret_val);
			break;
		}
		case DVD_TYPE_FILE_INFO: {
			MateVFSFileInfo **ret_val;

			ret_val = va_arg (args, MateVFSFileInfo **);
			*ret_val = mate_vfs_daemon_message_iter_get_file_info (&iter);

			if (!*ret_val) {
				goto fail;
			}
			break;
		}
		case DVD_TYPE_BYTE_ARRAY: {
			DBusMessageIter   array_iter;
			unsigned char   **ret_data;
			gint             *ret_len;

			if (dbus_type != DBUS_TYPE_ARRAY) {
				goto fail;
			}

			if (dbus_message_iter_get_element_type (&iter) != DBUS_TYPE_BYTE) {
				goto fail;
			}

			ret_data = va_arg (args, unsigned char **);
			ret_len = va_arg (args, gint *);

			dbus_message_iter_recurse (&iter, &array_iter);
			dbus_message_iter_get_fixed_array (&array_iter,
							   ret_data,
							   ret_len);
			break;
		}
		case DVD_TYPE_LAST:
			break;
		}

		has_next = dbus_message_iter_has_next (&iter);

		dbus_message_iter_next (&iter);
		type = va_arg (args, DvdArgumentType);
	}

	va_end (args);

	if (cancellation_id && has_next) {
		if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {
			d(g_print ("No cancellation id (%c)\n",
				   dbus_message_iter_get_arg_type (&iter)));
			/* Note: Leaks here... */
			return FALSE;
		}

		dbus_message_iter_get_basic (&iter, cancellation_id);
	}
	else if (cancellation_id) {
		*cancellation_id = -1;
	}

	return TRUE;

 fail:
	d(g_print ("Get args: couldn't get type: %d.\n", type));

	va_end (args);
	return FALSE;
}

