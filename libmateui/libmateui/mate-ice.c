/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* mate-ice.c - Interface between ICE and Gtk.
   Written by Tom Tromey <tromey@cygnus.com>.  */

#include <config.h>

#ifdef HAVE_LIBSM
#include <X11/ICE/ICElib.h>
#endif /* HAVE_LIBSM */

#include <unistd.h>
#include <fcntl.h>

#include <gtk/gtk.h>
#include "mate-ice.h"

#ifdef HAVE_LIBSM

static void mate_ice_io_error_handler (IceConn connection);

static void new_ice_connection (IceConn connection, IcePointer client_data,
				Bool opening, IcePointer *watch_data);

/* This is called when data is available on an ICE connection.  */
static gboolean
process_ice_messages (GIOChannel   *source,
		      GIOCondition  condition,
		      gpointer      data)
{
  IceConn connection = (IceConn) data;
  IceProcessMessagesStatus status;

  status = IceProcessMessages (connection, NULL, NULL);

  if (status == IceProcessMessagesIOError)
    {
      IcePointer context = IceGetConnectionContext (connection);

      if (context && GTK_IS_OBJECT (context))
	{
	  guint disconnect_id = g_signal_lookup ("disconnect", G_OBJECT_TYPE (context));

	  if (disconnect_id > 0)
	    g_signal_emit (context, disconnect_id, 0);
	}
      else
	{
	  IceSetShutdownNegotiation (connection, False);
	  IceCloseConnection (connection);
	}
    }

  return TRUE;
}

/* This is called when a new ICE connection is made.  It arranges for
   the ICE connection to be handled via the event loop.  */
static void
new_ice_connection (IceConn connection, IcePointer client_data, Bool opening,
		    IcePointer *watch_data)
{
  guint input_id;

  if (opening)
    {
      GIOChannel *channel;
      /* Make sure we don't pass on these file descriptors to any
         exec'ed children */
      fcntl(IceConnectionNumber(connection),F_SETFD,
	    fcntl(IceConnectionNumber(connection),F_GETFD,0) | FD_CLOEXEC);

      channel = g_io_channel_unix_new (IceConnectionNumber (connection));
      input_id = g_io_add_watch (channel,
				 G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_PRI,
				 process_ice_messages,
				 connection);
      g_io_channel_unref (channel);

      *watch_data = (IcePointer) GUINT_TO_POINTER (input_id);
    }
  else
    {
      input_id = GPOINTER_TO_UINT ((gpointer) *watch_data);

      g_source_remove (input_id);
    }
}

static IceIOErrorHandler mate_ice_installed_handler;

/* We call any handler installed before (or after) mate_ice_init but
   avoid calling the default libICE handler which does an exit() */
static void
mate_ice_io_error_handler (IceConn connection)
{
    if (mate_ice_installed_handler)
      (*mate_ice_installed_handler) (connection);
}

#endif /* HAVE_LIBSM */

/**
 * mate_ice_init:
 *
 * Initialises the ICE(Inter-Client Exchange). Used to make some initial calls into
 * the ICE library in order to accept ICE connection .
 */
void
mate_ice_init (void)
{
#ifdef HAVE_LIBSM
  static gboolean ice_init = FALSE;

  if (! ice_init)
    {
      IceIOErrorHandler default_handler;

      mate_ice_installed_handler = IceSetIOErrorHandler (NULL);
      default_handler = IceSetIOErrorHandler (mate_ice_io_error_handler);

      if (mate_ice_installed_handler == default_handler)
	mate_ice_installed_handler = NULL;

      IceAddConnectionWatch (new_ice_connection, NULL);

      ice_init = TRUE;
    }
#endif /* HAVE_LIBSM */
}
