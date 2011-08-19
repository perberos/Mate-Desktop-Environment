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
#include <config.h>

#ifndef MATE_DISABLE_DEPRECATED_SOURCE

/* Must be before all other mate includes!! */
#include <glib/gi18n-lib.h>

#include <libmate/mate-util.h>

#include "mate-app-util.h"

#include "mate-dialog-util.h"
#include "mate-dialog.h"
#include "mate-uidefs.h"
#include "mate-appbar.h"

#include <gtk/gtk.h>

static gboolean
mate_app_has_appbar_progress(MateApp * app)
{
  return ( (app->statusbar != NULL) &&
	   (MATE_APPBAR_HAS_PROGRESS(app->statusbar)) );
}

static gboolean
mate_app_has_appbar_status(MateApp * app)
{
  return ( (app->statusbar != NULL) &&
	   (MATE_APPBAR_HAS_STATUS(app->statusbar)) );
}

static gboolean
mate_app_interactive_statusbar(MateApp * app)
{
 return ( mate_app_has_appbar_status (app) && 0
	  /* FIXME!!!
	  && mate_preferences_get_statusbar_dialog()
	  && mate_preferences_get_statusbar_interactive() */);
}

static void mate_app_activate_statusbar(MateApp *app)
{
  gtk_window_set_focus(GTK_WINDOW(app),
		       mate_appbar_get_status(MATE_APPBAR(app->statusbar)));
  gtk_window_activate_focus(GTK_WINDOW(app));
  gdk_window_raise(GTK_WIDGET(app)->window);
}

/* ================================================================== */

/* =================================================================== */
/* Simple messages */

static void
ack_cb(MateAppBar * bar, gpointer data)
{
  mate_appbar_clear_prompt(bar);
}

static void
ack_clear_prompt_cb(MateAppBar * bar, gpointer data)
{
#ifdef MATE_ENABLE_DEBUG
  g_print("Clearing prompt (ack)\n");
#endif

  g_signal_handlers_disconnect_by_func(bar, ack_cb, data);
  g_signal_handlers_disconnect_by_func(bar, ack_clear_prompt_cb, data);
}

static void mate_app_message_bar (MateApp * app, const gchar * message)
{
  gchar * prompt = g_strconcat(message, _(" (press return)"), NULL);
  mate_appbar_set_prompt(MATE_APPBAR(app->statusbar), prompt, FALSE);
  mate_app_activate_statusbar(app);
  g_free(prompt);
  g_signal_connect(app->statusbar, "user_response", G_CALLBACK(ack_cb), NULL);
  g_signal_connect(app->statusbar, "clear_prompt", G_CALLBACK(ack_clear_prompt_cb), NULL);
}


/**
 * mate_app_message
 * @app: Pointer to MATE app object
 * @message: Text of message to be displayed
 *
 * Description: A simple message, in an OK dialog or the status bar.
 * Requires confirmation from the user before it goes away.
 *
 * Returns:
 * Pointer to dialog widget, or %NULL if error or message in
 * status bar.
 **/

GtkWidget *
mate_app_message (MateApp * app, const gchar * message)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(MATE_IS_APP(app), NULL);

  /* FIXME !!! */
  if ( 0 /*mate_preferences_get_statusbar_dialog() &&
       mate_app_interactive_statusbar(app) */) {
    mate_app_message_bar ( app, message );
    return NULL;
  }
  else {
    return mate_ok_dialog_parented(message,GTK_WINDOW(app));
  }
}

static void
mate_app_error_bar(MateApp * app, const gchar * error)
{
  gchar * s = g_strconcat(_("ERROR: "), error, NULL);
  gdk_beep();
  mate_app_message_bar(app, s);
  g_free(s);
}


/**
 * mate_app_error
 * @app: Pointer to MATE app object
 * @error: Text of error message to be displayed
 *
 * Description:
 * An important fatal error; if it appears in the statusbar,
 * it might gdk_beep() and require acknowledgement.
 *
 * Returns:
 * Pointer to dialog widget, or %NULL if error or message in
 * status bar.
 **/

GtkWidget *
mate_app_error (MateApp * app, const gchar * error)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(MATE_IS_APP(app), NULL);
  g_return_val_if_fail(error != NULL, NULL);

  if ( mate_app_interactive_statusbar(app) ) {
    mate_app_error_bar(app, error);
    return NULL;
  }
  else return mate_error_dialog_parented (error,GTK_WINDOW(app));
}

static void mate_app_warning_bar (MateApp * app, const gchar * warning)
{
  gchar * s = g_strconcat(_("Warning: "), warning, NULL);
  gdk_beep();
  mate_app_flash(app, s);
  g_free(s);
}


/**
 * mate_app_warning
 * @app: Pointer to MATE app object
 * @warning: Text of warning message to be displayed
 *
 * Description:
 * A not-so-important error, but still marked better than a flash
 *
 * Returns:
 * Pointer to dialog widget, or %NULL if error or message in
 * status bar.
 **/

GtkWidget *
mate_app_warning (MateApp * app, const gchar * warning)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(MATE_IS_APP(app), NULL);
  g_return_val_if_fail(warning != NULL, NULL);

  /* FIXME !!! */
  if ( 0 /*mate_app_has_appbar_status(app) &&
       mate_preferences_get_statusbar_dialog() */) {
    mate_app_warning_bar(app, warning);
    return NULL;
  }
  else {
    return mate_warning_dialog_parented(warning,GTK_WINDOW(app));
  }
}

/* =================================================== */
/* Flash */

struct _MessageInfo {
  MateApp * app;
  guint timeoutid;
  guint handlerid;
};

typedef struct _MessageInfo MessageInfo;

static gint
remove_message_timeout (MessageInfo * mi)
{
  GDK_THREADS_ENTER ();

  mate_appbar_refresh(MATE_APPBAR(mi->app->statusbar));
  g_signal_handler_disconnect(mi->app, mi->handlerid);
  g_free ( mi );

  GDK_THREADS_LEAVE ();

  return FALSE; /* removes the timeout */
}

/* Called if the app is destroyed before the timeout occurs. */
static void
remove_timeout_cb ( GtkWidget * app, MessageInfo * mi )
{
  g_source_remove(mi->timeoutid);
  g_free(mi);
}

static const guint32 flash_length = 3000; /* 3 seconds, I hope */


/**
 * mate_app_flash
 * @app: Pointer to MATE app object
 * @flash: Text of message to be flashed
 *
 * Description:
 * Flash the message in the statusbar for a few moments; if no
 * statusbar, do nothing (?). For trivial little status messages,
 * e.g. "Auto saving..."
 **/

void
mate_app_flash (MateApp * app, const gchar * flash)
{
  /* This is a no-op for dialogs, since these messages aren't
     important enough to pester users. */
  g_return_if_fail(app != NULL);
  g_return_if_fail(MATE_IS_APP(app));
  g_return_if_fail(flash != NULL);

  /* OK to call app_flash without a statusbar */
  if ( mate_app_has_appbar_status(app) ) {
    MessageInfo * mi;

    g_return_if_fail(MATE_IS_APPBAR(app->statusbar));

    mate_appbar_set_status(MATE_APPBAR(app->statusbar), flash);

    mi = g_new(MessageInfo, 1);

    mi->timeoutid =
      g_timeout_add (flash_length,
		     (GtkFunction) remove_message_timeout,
		      mi);

    mi->handlerid =
      g_signal_connect (G_OBJECT(app),
			"destroy",
			G_CALLBACK(remove_timeout_cb),
			mi);

    mi->app       = app;
  }
}

/* ========================================================== */

typedef struct {
  MateReplyCallback callback;
  gpointer data;
} ReplyInfo;

static void
bar_reply_cb(MateAppBar * ab, ReplyInfo * ri)
{
  gchar * response;

  response = mate_appbar_get_response(ab);
  g_return_if_fail(response != NULL);

#ifdef MATE_ENABLE_DEBUG
  g_print("Got reply: \"%s\"\n", response);
#endif

  if ( (g_ascii_strcasecmp(_("y"), response) == 0) ||
       (g_ascii_strcasecmp(_("yes"), response) == 0) ) {
    (* (ri->callback)) (MATE_YES, ri->data);
  }
  else if ( (g_ascii_strcasecmp(_("n"), response) == 0) ||
	    (g_ascii_strcasecmp(_("no"), response) == 0) ) {
    (* (ri->callback)) (MATE_NO, ri->data);
  }
  else {
    g_free(response);
    gdk_beep(); /* Kind of lame; better to give a helpful message */
    return;
  }
  g_free(response);
  mate_appbar_clear_prompt(ab);
  return;
}

/* FIXME this can all be done as a special case of request_string
   where we check the string for yes/no afterward. */

static void
reply_clear_prompt_cb(MateAppBar * ab, ReplyInfo * ri)
{
#ifdef MATE_ENABLE_DEBUG
  g_print("Clear prompt callback (reply)\n");
#endif
  g_signal_handlers_disconnect_by_func(ab, bar_reply_cb, ri);
  g_signal_handlers_disconnect_by_func(ab, reply_clear_prompt_cb, ri);
  g_free(ri);
}

static void
mate_app_reply_bar(MateApp * app, const gchar * question,
		    MateReplyCallback callback, gpointer data,
		    gboolean yes_or_ok, gboolean modal)
{
  gchar * prompt;
  ReplyInfo * ri;

  prompt = g_strconcat(question, yes_or_ok ? _(" (yes or no)") :
			  _("  - OK? (yes or no)"), NULL);
  mate_appbar_set_prompt(MATE_APPBAR(app->statusbar), prompt, modal);
  mate_app_activate_statusbar(app);
  g_free(prompt);

  ri = g_new(ReplyInfo, 1);
  ri->callback = callback;
  ri->data     = data;

  g_signal_connect(app->statusbar, "user_response",
		   G_CALLBACK(bar_reply_cb), ri);
  g_signal_connect(app->statusbar, "clear_prompt",
		   G_CALLBACK(reply_clear_prompt_cb), ri);
}


/**
 * mate_app_question
 * @app: Pointer to MATE app object
 * @question: Text of question to be displayed
 * @callback:
 * @data:
 *
 * Description:
 * Ask a yes or no question, and call the callback when it's answered.
 *
 * Returns:
 * Pointer to dialog widget, or %NULL if error or message in
 * status bar.
 **/

GtkWidget *
mate_app_question (MateApp * app, const gchar * question,
		    MateReplyCallback callback, gpointer data)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(MATE_IS_APP(app), NULL);
  g_return_val_if_fail(question != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  if ( mate_app_interactive_statusbar(app) ) {
    mate_app_reply_bar(app, question, callback, data, TRUE, FALSE);
    return NULL;
  }
  else {
    return mate_question_dialog_parented(question, callback, data,
					  GTK_WINDOW(app));
  }
}


/**
 * mate_app_question_modal
 * @app: Pointer to MATE app object
 * @question: Text of question to be displayed
 * @callback:
 * @data:
 *
 * Description:
 * Ask a yes or no question, and call the callback when it's answered.
 *
 * Returns:
 * Pointer to dialog widget, or %NULL if error or message in
 * status bar.
 **/

GtkWidget *
mate_app_question_modal (MateApp * app, const gchar * question,
			  MateReplyCallback callback, gpointer data)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(MATE_IS_APP(app), NULL);
  g_return_val_if_fail(question != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  if ( mate_app_interactive_statusbar(app) ) {
    mate_app_reply_bar(app, question, callback, data, TRUE, TRUE);
    return NULL;
  }
  else {
    return mate_question_dialog_modal_parented(question, callback, data,
						GTK_WINDOW(app));
  }
}


/**
 * mate_app_ok_cancel
 * @app: Pointer to MATE app object
 * @message: Text of message to be displayed
 * @callback:
 * @data:
 *
 * Description:
 *
 * Returns:
 * Pointer to dialog widget, or %NULL if error or message in
 * status bar.
 **/

GtkWidget *
mate_app_ok_cancel (MateApp * app, const gchar * message,
		     MateReplyCallback callback, gpointer data)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(MATE_IS_APP(app), NULL);
  g_return_val_if_fail(message != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  if ( mate_app_interactive_statusbar(app) ) {
    mate_app_reply_bar(app, message, callback, data, FALSE, FALSE);
    return NULL;
  }
  else {
    return mate_ok_cancel_dialog_parented(message, callback, data,
					   GTK_WINDOW(app));
  }
}


/**
 * mate_app_ok_cancel_modal
 * @app: Pointer to MATE app object
 * @message: Text of message to be displayed
 * @callback:
 * @data:
 *
 * Description:
 *
 * Returns:
 * Pointer to dialog widget, or %NULL if error or message in
 * status bar.
 **/

GtkWidget *
mate_app_ok_cancel_modal (MateApp * app, const gchar * message,
			   MateReplyCallback callback, gpointer data)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(MATE_IS_APP(app), NULL);
  g_return_val_if_fail(message != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  if ( mate_app_interactive_statusbar(app) ) {
    mate_app_reply_bar(app, message, callback, data, FALSE, TRUE);
    return NULL;
  }
  else {
    return mate_ok_cancel_dialog_modal_parented(message, callback, data,
						 GTK_WINDOW(app));
  }
}

typedef struct {
  MateStringCallback callback;
  gpointer data;
} RequestInfo;


static void
bar_request_cb(MateAppBar * ab, RequestInfo * ri)
{
  gchar * response;

  response = mate_appbar_get_response(ab);
  g_return_if_fail(response != NULL);

#ifdef MATE_ENABLE_DEBUG
  g_print("Got string: \"%s\"\n", response);
#endif

  /* response isn't freed because the callback expects an
     allocated string. */
  (* (ri->callback)) (response, ri->data);

  mate_appbar_clear_prompt(ab);
  return;
}

static void
request_clear_prompt_cb(MateAppBar * ab, RequestInfo * ri)
{
#ifdef MATE_ENABLE_DEBUG
  g_print("Clearing prompt (request)\n");
#endif


  g_signal_handlers_disconnect_by_func (ab, bar_request_cb, ri);
  g_signal_handlers_disconnect_by_func (ab, request_clear_prompt_cb, ri);
  g_free(ri);
}

static void
mate_app_request_bar  (MateApp * app, const gchar * prompt,
			MateStringCallback callback,
			gpointer data, gboolean password)
{
  if (password == TRUE) {
    g_warning("Password support not implemented for appbar");
  }
  else {
    RequestInfo * ri;

    mate_appbar_set_prompt(MATE_APPBAR(app->statusbar), prompt, FALSE);

    ri = g_new(RequestInfo, 1);
    ri->callback = callback;
    ri->data     = data;

    g_signal_connect(app->statusbar, "user_response",
		     G_CALLBACK(bar_request_cb), ri);
    g_signal_connect(app->statusbar, "clear_prompt",
		     G_CALLBACK(request_clear_prompt_cb), ri);
  }
}


/**
 * mate_app_request_string
 * @app: Pointer to MATE app object
 * @prompt: Text of prompt to be displayed
 * @callback:
 * @data:
 *
 * Description:
 *
 * Returns:
 * Pointer to dialog widget, or %NULL if error or message in
 * status bar.
 **/

GtkWidget *
mate_app_request_string (MateApp * app, const gchar * prompt,
			  MateStringCallback callback, gpointer data)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(MATE_IS_APP(app), NULL);
  g_return_val_if_fail(prompt != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  if ( mate_app_interactive_statusbar(app) ) {
    mate_app_request_bar(app, prompt, callback, data, FALSE);
    return NULL;
  }
  else {
    return mate_request_dialog(FALSE, prompt, NULL, 0,
				callback, data, GTK_WINDOW(app));
  }
}



/**
 * mate_app_request_password
 * @app: Pointer to MATE app object
 * @prompt: Text of prompt to be displayed
 * @callback:
 * @data:
 *
 * Description:
 *
 * Returns:
 * Pointer to dialog widget, or %NULL if error or message in
 * status bar.
 **/

GtkWidget *
mate_app_request_password (MateApp * app, const gchar * prompt,
			    MateStringCallback callback, gpointer data)
{
  g_return_val_if_fail(app != NULL, NULL);
  g_return_val_if_fail(MATE_IS_APP(app), NULL);
  g_return_val_if_fail(prompt != NULL, NULL);
  g_return_val_if_fail(callback != NULL, NULL);

  /* FIXME implement for AppBar */

  return mate_request_dialog(TRUE, prompt, NULL, 0,
			      callback, data, GTK_WINDOW(app));
}

/* ================================================== */

typedef struct {
  GtkWidget * bar; /* Progress bar, for dialog; NULL for AppBar */
  GtkWidget * widget; /* dialog or AppBar */
  MateApp * app;
  MateAppProgressFunc percentage_cb;
  MateAppProgressCancelFunc cancel_cb;
  gpointer data;
  guint timeout_tag, handler_id;
} ProgressKeyReal;

/* Invalid value, I hope. FIXME */
#define INVALID_TIMEOUT 0

/* Works for statusbar and dialog both. */
static gint progress_timeout_cb (ProgressKeyReal * key)
{
  gdouble percent;

  GDK_THREADS_ENTER ();

  percent = (* key->percentage_cb)(key->data);
  mate_app_set_progress (key, percent);

  GDK_THREADS_LEAVE ();

  return TRUE;
}


/* dialog only */
static void progress_clicked_cb (MateDialog * d, gint button,
				 ProgressKeyReal * key)
{
  if (key->cancel_cb) {
    (* key->cancel_cb)(key->data);
  }
  key->widget = NULL; /* The click closed the dialog */
  mate_app_progress_done(key);
}

static void
progress_dialog (const gchar * description, ProgressKeyReal * key)
{
  GtkWidget * d, * pb, * label;

  d = mate_dialog_new ( _("Progress"),
			 MATE_STOCK_BUTTON_CANCEL,
			 NULL );
  mate_dialog_set_close (MATE_DIALOG(d), TRUE);
  mate_dialog_set_parent(MATE_DIALOG(d), GTK_WINDOW(key->app));
  g_signal_connect (d, "clicked",
		    G_CALLBACK(progress_clicked_cb),
		    key);

  label = gtk_label_new (description);

  pb = gtk_progress_bar_new();

  gtk_box_pack_start ( GTK_BOX(MATE_DIALOG(d)->vbox),
		       label, TRUE, TRUE, MATE_PAD );
  gtk_box_pack_start ( GTK_BOX(MATE_DIALOG(d)->vbox),
		       pb, TRUE, TRUE, MATE_PAD );

  key->bar = pb;
  key->widget = d;

  gtk_widget_show_all (d);
}

static void
progress_bar (const gchar * description, ProgressKeyReal * key)
{
  key->bar = NULL;
  key->widget = key->app->statusbar;
  mate_appbar_set_status(MATE_APPBAR(key->widget), description);
}

static void
stop_progress_cb(MateApp * app, MateAppProgressKey key)
{
  mate_app_progress_done(key);
}

/* FIXME share code between manual and timeout */

/**
 * mate_app_progress_timeout
 * @app:
 * @description:
 * @interval:
 * @percentage_cb:
 * @cancel_cb:
 * @data:
 *
 * Description:
 *
 * Returns:
 **/

MateAppProgressKey
mate_app_progress_timeout (MateApp * app,
			    const gchar * description,
			    guint32 interval,
			    MateAppProgressFunc percentage_cb,
			    MateAppProgressCancelFunc cancel_cb,
			    gpointer data)
{
  ProgressKeyReal * key;

  g_return_val_if_fail (app != NULL, NULL);
  g_return_val_if_fail (MATE_IS_APP(app), NULL);
  g_return_val_if_fail (description != NULL, NULL);
  g_return_val_if_fail (percentage_cb != NULL, NULL);

  key = g_new (ProgressKeyReal, 1);

  key->app = app;
  key->percentage_cb = percentage_cb;
  key->cancel_cb = cancel_cb;
  key->data = data;

  if ( mate_app_has_appbar_progress(app) &&
  /* FIXME !!! */
       0 /* mate_preferences_get_statusbar_dialog() */) {
    progress_bar    (description, key);
  }
  else {
    progress_dialog (description, key);
  }

  key->timeout_tag = g_timeout_add (interval,
				    (GtkFunction) progress_timeout_cb,
				    key);

  /* Make sure progress stops if the app is destroyed. */
  key->handler_id = g_signal_connect(app, "destroy",
				     G_CALLBACK(stop_progress_cb),
				     key);

  return key;
}


/**
 * mate_app_progress_manual
 * @app:
 * @description:
 * @cancel_cb:
 * @data:
 *
 * Description:
 *
 * Returns:
 **/

MateAppProgressKey
mate_app_progress_manual (MateApp * app,
			   const gchar * description,
			   MateAppProgressCancelFunc cancel_cb,
			   gpointer data)
{
  ProgressKeyReal * key;

  g_return_val_if_fail (app != NULL, NULL);
  g_return_val_if_fail (MATE_IS_APP(app), NULL);
  g_return_val_if_fail (description != NULL, NULL);

  key = g_new (ProgressKeyReal, 1);

  key->app = app;
  key->cancel_cb = cancel_cb;
  key->data = data;
  key->timeout_tag = INVALID_TIMEOUT;

  if ( mate_app_has_appbar_progress(app) &&
  /* FIXME !!! */
       0 /*mate_preferences_get_statusbar_dialog() */) {
    progress_bar    (description, key);
  }
  else {
    progress_dialog (description, key);
  }

  /* Make sure progress stops if the app is destroyed. */
  key->handler_id = g_signal_connect(app, "destroy",
				     G_CALLBACK(stop_progress_cb),
				     key);

  return key;
}

/**
 * mate_app_set_progress
 * @key:
 * @percent:
 *
 * Description:
 **/

void mate_app_set_progress (MateAppProgressKey key, gdouble percent)
{
  ProgressKeyReal * real_key = (MateAppProgressKey) key;

  g_return_if_fail ( key != NULL );

  if (real_key->bar) {
    gtk_progress_bar_set_fraction ( GTK_PROGRESS_BAR(real_key->bar), percent );
  }
  else {
    mate_appbar_set_progress_percentage ( MATE_APPBAR(real_key->widget), percent );
  }
}

static void progress_timeout_remove(ProgressKeyReal * key)
{
  if (key->timeout_tag != INVALID_TIMEOUT) {
    g_source_remove (key->timeout_tag);
    key->timeout_tag = INVALID_TIMEOUT;
  }
}

/**
 * mate_app_progress_done
 * @key:
 *
 * Description:
 **/

void mate_app_progress_done (MateAppProgressKey key)
{
  ProgressKeyReal * real_key = (MateAppProgressKey) key;

  g_return_if_fail ( key != NULL );

  progress_timeout_remove((ProgressKeyReal *)key);

  g_signal_handler_disconnect(real_key->app, real_key->handler_id);

  if (real_key->bar) { /* It's a dialog */
    if (real_key->widget) mate_dialog_close(MATE_DIALOG(real_key->widget));
  }
  else {
    /* Reset the bar */
    mate_appbar_set_progress_percentage ( MATE_APPBAR(real_key->widget), 0.0 );
  }
  g_free(key);
}

#endif /* MATE_DISABLE_DEPRECATED_SOURCE */
