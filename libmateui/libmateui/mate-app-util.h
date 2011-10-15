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

#ifndef MATE_APP_UTIL_H
#define MATE_APP_UTIL_H
/****
  Convenience UI functions for use with MateApp
  ****/

#ifndef MATE_DISABLE_DEPRECATED

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "mate-app.h"
#include "mate-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================
  Simple messages and questions to the user; use dialogs for now, but
   ultimately can use either dialogs or statusbar. Supposed to be a
   "semantic markup" kind of thing. */
/* If the function returns GtkWidget *, it will return any dialog
   created, or NULL if none */

/* A simple message, in an OK dialog or the status bar. Requires
   confirmation from the user before it goes away. */
GtkWidget *
mate_app_message (MateApp * app, const gchar * message);

/* Flash the message in the statusbar for a few moments; if no
   statusbar, do nothing (?). For trivial little status messages,
   e.g. "Auto saving..." */
void
mate_app_flash (MateApp * app, const gchar * flash);

/* An important fatal error; if it appears in the statusbar,
   it might gdk_beep() and require acknowledgement. */
GtkWidget *
mate_app_error (MateApp * app, const gchar * error);

/* A not-so-important error, but still marked better than a flash */
GtkWidget *
mate_app_warning (MateApp * app, const gchar * warning);

/* =============================================================== */

/* For these, the user can cancel without ever calling the callback,
   e.g. by clicking the dialog's "close" button. So don't count on the
   callback ever being called. */

/* Ask a yes or no question, and call the callback when it's answered. */
GtkWidget *
mate_app_question (MateApp * app, const gchar * question,
		    MateReplyCallback callback, gpointer data);

GtkWidget *
mate_app_question_modal (MateApp * app, const gchar * question,
			  MateReplyCallback callback, gpointer data);

/* OK-Cancel question. */
GtkWidget *
mate_app_ok_cancel (MateApp * app, const gchar * message,
		     MateReplyCallback callback, gpointer data);

GtkWidget *
mate_app_ok_cancel_modal (MateApp * app, const gchar * message,
			   MateReplyCallback callback, gpointer data);

/* Get a string. */
GtkWidget *
mate_app_request_string (MateApp * app, const gchar * prompt,
			  MateStringCallback callback, gpointer data);

/* Request a string, but don't echo to the screen. */
GtkWidget *
mate_app_request_password (MateApp * app, const gchar * prompt,
			    MateStringCallback callback, gpointer data);


/* ========================================================== */

/* Returns the fraction of the task done at a given time. */
typedef gdouble (* MateAppProgressFunc) (gpointer data);

/* What to call if the operation is canceled. */
typedef void (* MateAppProgressCancelFunc) (gpointer data);

/* A key used to refer to the callback. DO NOT FREE.
   It's freed by _done or when the progress is canceled. */
typedef gpointer MateAppProgressKey;

/* These will be a progress bar dialog or the progress bar in the AppBar. */

/* Call percentage_cb every interval to set the progress indicator.
   Both callbacks get the data arg. */
MateAppProgressKey
mate_app_progress_timeout (MateApp * app,
			    const gchar * description,
			    guint32 interval,
			    MateAppProgressFunc percentage_cb,
			    MateAppProgressCancelFunc cancel_cb,
			    gpointer data);

/* Just create a callback key; it then has to be updated
   with _update() */
MateAppProgressKey
mate_app_progress_manual (MateApp * app,
			   const gchar * description,
			   MateAppProgressCancelFunc cancel_cb,
			   gpointer data);

/* Only makes sense with manual. */
void
mate_app_set_progress (MateAppProgressKey key, gdouble percent);

/* Call this when the progress meter should go away. Automatically
   called if progress is cancelled. */
void
mate_app_progress_done (MateAppProgressKey key);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED_SOURCE */

#endif
