/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mate-vfs-cancellation.c - Cancellation handling for the MATE Virtual File
   System access methods.

   Copyright (C) 1999 Free Software Foundation

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@gnu.org> */

#include <config.h>

#include "mate-vfs-cancellation-private.h"

#include "mate-vfs-utils.h"
#include "mate-vfs-private-utils.h"

#ifdef USE_DAEMON
#include <mate-vfs-dbus-utils.h>
#endif

#include <unistd.h>

#ifdef G_OS_WIN32
#include <fcntl.h>
#include <io.h>
#endif

/* WARNING: this code is not general-purpose.  It is supposed to make the two
   sides of the VFS (i.e. the main process/thread and its asynchronous slave)
   talk in a simple way.  For this reason, only the main process/thread should
   be allowed to call `mate_vfs_cancellation_cancel()'.  *All* the code is
   based on this assumption.  */

struct MateVFSCancellation {
	gboolean cancelled;
	gint pipe_in;
	gint pipe_out;
#ifdef USE_DAEMON
	/* daemon handle */
	gint32 handle;
	gint32 connection;
#endif
};

G_LOCK_DEFINE_STATIC (pipes);
#ifdef USE_DAEMON
G_LOCK_DEFINE_STATIC (callback);
#endif

/**
 * mate_vfs_cancellation_new:
 * 
 * Create a new #MateVFSCancellation object for reporting cancellation to a
 * mate-vfs module.
 * 
 * Return value: A pointer to the new MateVFSCancellation object.
 */
MateVFSCancellation *
mate_vfs_cancellation_new (void)
{
	MateVFSCancellation *new;

	new = g_new (MateVFSCancellation, 1);
	new->cancelled = FALSE;
	new->pipe_in = -1;
	new->pipe_out = -1;
#ifdef USE_DAEMON
	new->connection = 0;
	new->handle = 0;
#endif	
	return new;
}

/**
 * mate_vfs_cancellation_destroy:
 * @cancellation: a #MateVFSCancellation object.
 * 
 * Destroy @cancellation.
 */
void
mate_vfs_cancellation_destroy (MateVFSCancellation *cancellation)
{
	g_return_if_fail (cancellation != NULL);

	if (cancellation->pipe_in >= 0) {
		close (cancellation->pipe_in);
		close (cancellation->pipe_out);
	}
	
	g_free (cancellation);
}

#ifdef USE_DAEMON

void
_mate_vfs_cancellation_set_handle (MateVFSCancellation *cancellation,
				    gint32 connection, gint32 handle)
{
	G_LOCK (callback);

	/* Each client call uses its own context/cancellation */
	g_assert (cancellation->handle == 0);

	cancellation->connection = connection;
	cancellation->handle = handle;

	G_UNLOCK (callback);
}

void
_mate_vfs_cancellation_unset_handle (MateVFSCancellation *cancellation)
{
	G_LOCK (callback);
	
	cancellation->connection = 0;
	cancellation->handle = 0;

	G_UNLOCK (callback);
}

#endif

/**
 * mate_vfs_cancellation_cancel:
 * @cancellation: a #MateVFSCancellation object.
 * 
 * Send a cancellation request through @cancellation.
 *
 * If called on a different thread than the one handling idle
 * callbacks, there is a small race condition where the
 * operation finished callback will be called even if you
 * cancelled the operation. Its the apps responsibility
 * to handle this. See mate_vfs_async_cancel() for more
 * discussion about this.
 */
void
mate_vfs_cancellation_cancel (MateVFSCancellation *cancellation)
{
#ifdef USE_DAEMON
	gint32 handle, connection_id;
#endif	
	g_return_if_fail (cancellation != NULL);

	if (cancellation->cancelled)
		return;

	if (cancellation->pipe_out >= 0)
		write (cancellation->pipe_out, "c", 1);
#ifdef USE_DAEMON
	handle = 0;
	connection_id = 0;

	G_LOCK (callback);
	if (cancellation->handle) {
		handle = cancellation->handle;
		connection_id = cancellation->connection;
	}
	G_UNLOCK (callback);
#endif	
	cancellation->cancelled = TRUE;

#ifdef USE_DAEMON
	if (handle != 0) {
		DBusConnection *conn;
		DBusMessage *message;
		DBusError error;

		dbus_error_init (&error);
        
		conn = _mate_vfs_get_main_dbus_connection ();

		if (conn != NULL) {
					
			message = dbus_message_new_method_call (DVD_DAEMON_SERVICE,
								DVD_DAEMON_OBJECT,
								DVD_DAEMON_INTERFACE,
								DVD_DAEMON_METHOD_CANCEL);
			dbus_message_set_auto_start (message, TRUE);
			if (!message) {
				g_error ("Out of memory");
			}

			if (!dbus_message_append_args (message,
						       DBUS_TYPE_INT32, &handle,
						       DBUS_TYPE_INT32, &connection_id,
						       DBUS_TYPE_INVALID)) {
				g_error ("Out of memory");
			}

			dbus_connection_send (conn, message, NULL);
			dbus_connection_flush (conn);
			dbus_message_unref (message);
		}
	}
#endif
}

/**
 * mate_vfs_cancellation_check:
 * @cancellation: a #MateVFSCancellation object.
 * 
 * Check for pending cancellation.
 * 
 * Return value: %TRUE if the operation should be interrupted.
 */
gboolean
mate_vfs_cancellation_check (MateVFSCancellation *cancellation)
{
	if (cancellation == NULL)
		return FALSE;

	return cancellation->cancelled;
}

/**
 * mate_vfs_cancellation_ack:
 * @cancellation: a #MateVFSCancellation object.
 * 
 * Acknowledge a cancellation.  This should be called if
 * mate_vfs_cancellation_check() returns %TRUE or if select() reports that
 * input is available on the file descriptor returned by
 * mate_vfs_cancellation_get_fd().
 */
void
mate_vfs_cancellation_ack (MateVFSCancellation *cancellation)
{
	gchar c;

	/* ALEX: What the heck is this supposed to be used for?
	 * It seems totatlly wrong, and isn't used by anything.
	 * Also, the read() seems to block if it was cancelled before
	 * the pipe was gotten.
	 */
	
	if (cancellation == NULL)
		return;

	if (cancellation->pipe_in >= 0)
		read (cancellation->pipe_in, &c, 1);

	cancellation->cancelled = FALSE;
}

/**
 * mate_vfs_cancellation_get_fd:
 * @cancellation: a #MateVFSCancellation object.
 * 
 * Get a file descriptor -based notificator for @cancellation.  When
 * @cancellation receives a cancellation request, a character will be made
 * available on the returned file descriptor for input.
 *
 * This is very useful for detecting cancellation during I/O operations: you
 * can use the select() call to check for available input/output on the file
 * you are reading/writing, and on the notificator's file descriptor at the
 * same time.  If a data is available on the notificator's file descriptor, you
 * know you have to cancel the read/write operation.
 * 
 * Return value: the notificator's file descriptor, or -1 if starved of
 *               file descriptors.
 */
gint
mate_vfs_cancellation_get_fd (MateVFSCancellation *cancellation)
{
	g_return_val_if_fail (cancellation != NULL, -1);

	G_LOCK (pipes);
	if (cancellation->pipe_in <= 0) {
		gint pipefd [2];

		if (_mate_vfs_pipe (pipefd) == -1) {
			G_UNLOCK (pipes);
			return -1;
		}

		cancellation->pipe_in = pipefd [0];
		cancellation->pipe_out = pipefd [1];
	}
	G_UNLOCK (pipes);

	return cancellation->pipe_in;
}
