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

#ifndef MATE_DIALOG_UTIL_H
#define MATE_DIALOG_UTIL_H

#ifndef MATE_DISABLE_DEPRECATED

/****
  Sugar functions to pop up dialogs in a hurry. These are probably
  too sugary, but they're used in mate-app-util anyway so they may as
  well be here for others to use when there's no MateApp.
  The mate-app-util functions are preferred if there's a MateApp
  to use them with, because they allow configurable statusbar messages
  instead of a dialog.
  ****/


#include "mate-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The GtkWidget * return values were added in retrospect; sometimes
   you might want to connect to the "close" signal of the dialog, or
   something, the return value makes the functions more
   flexible. However, there is nothing especially guaranteed about
   these dialogs except that they will be dialogs, so don't count on
   anything. */


/* A little OK box */
GtkWidget * mate_ok_dialog             (const gchar * message);
GtkWidget * mate_ok_dialog_parented    (const gchar * message,
					 GtkWindow * parent);

/* Operation failed fatally. In an OK dialog. */
GtkWidget * mate_error_dialog          (const gchar * error);
GtkWidget * mate_error_dialog_parented (const gchar * error,
					 GtkWindow * parent);

/* Just a warning. */
GtkWidget * mate_warning_dialog           (const gchar * warning);
GtkWidget * mate_warning_dialog_parented  (const gchar * warning,
					    GtkWindow * parent);

/* Look in mate-types.h for the callback types. */

/* Ask a yes or no question, and call the callback when it's answered. */
GtkWidget * mate_question_dialog                 (const gchar * question,
						   MateReplyCallback callback,
						   gpointer data);

GtkWidget * mate_question_dialog_parented        (const gchar * question,
						   MateReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent);

GtkWidget * mate_question_dialog_modal           (const gchar * question,
						   MateReplyCallback callback,
						   gpointer data);

GtkWidget * mate_question_dialog_modal_parented  (const gchar * question,
						   MateReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent);


/* OK-Cancel question. */
GtkWidget * mate_ok_cancel_dialog                (const gchar * message,
						   MateReplyCallback callback,
						   gpointer data);

GtkWidget * mate_ok_cancel_dialog_parented       (const gchar * message,
						   MateReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent);

GtkWidget * mate_ok_cancel_dialog_modal          (const gchar * message,
						   MateReplyCallback callback,
						   gpointer data);

GtkWidget * mate_ok_cancel_dialog_modal_parented (const gchar * message,
						   MateReplyCallback callback,
						   gpointer data,
						   GtkWindow * parent);

/* This function is deprecated; use mate_request_dialog() instead. */
GtkWidget * mate_request_string_dialog           (const gchar * prompt,
						   MateStringCallback callback,
						   gpointer data);

/* This function is deprecated; use mate_request_dialog() instead. */
GtkWidget * mate_request_string_dialog_parented  (const gchar * prompt,
						   MateStringCallback callback,
						   gpointer data,
						   GtkWindow * parent);

/* This function is deprecated; use mate_request_dialog() instead. */
GtkWidget * mate_request_password_dialog         (const gchar * prompt,
						   MateStringCallback callback,
						   gpointer data);

/* This function is deprecated; use mate_request_dialog() instead. */
GtkWidget * mate_request_password_dialog_parented(const gchar * prompt,
						   MateStringCallback callback,
						   gpointer data,
						   GtkWindow * parent);

/* Dialog containing a prompt and a text entry field for a response */
GtkWidget * mate_request_dialog (gboolean password,
                                  const gchar * prompt,
                                  const gchar * default_text,
                                  const guint16 max_length,
                                  MateStringCallback callback,
                                  gpointer data,
                                  GtkWindow * parent);

#ifdef __cplusplus
}
#endif

#endif /* MATE_DISABLE_DEPRECATED */

#endif

