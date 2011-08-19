/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@prettypeople.org>
 *           Johan Dahlin <johan@mate.org>
 *           Tim-Philipp Müller <tim centricular net>
 *
 *  Copyright 2002 Iain Holmes
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
 *
 * 4th Februrary 2005: Christian Schaller: changed license to LGPL with
 * permission of Iain Holmes, Ronald Bultje, Leontine Binchy (SUN), Johan Dalhin
 * and Joe Marcus Clarke
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <gst/gst.h>
#include <gst/interfaces/mixer.h>

#include <profiles/mate-media-profiles.h>

#include "gsr-window.h"

GST_DEBUG_CATEGORY_STATIC (gsr_debug);
#define GST_CAT_DEFAULT gsr_debug

extern GtkWidget * gsr_open_window (const char *filename);
extern void gsr_quit (void);

extern MateConfClient *mateconf_client;

extern void gsr_add_recent (gchar *filename);

#define MATECONF_DIR           "/apps/mate-sound-recorder/"
#define KEY_OPEN_DIR        MATECONF_DIR "system-state/open-file-directory"
#define KEY_SAVE_DIR        MATECONF_DIR "system-state/save-file-directory"
#define KEY_LAST_PROFILE_ID MATECONF_DIR "last-profile-id"
#define KEY_LAST_INPUT      MATECONF_DIR "last-input"
#define EBUSY_TRY_AGAIN     3    /* Empirical data */

typedef struct _GSRWindowPipeline {
	GstElement *pipeline;
	GstState    state;     /* last seen (async) pipeline state */
	GstBus     *bus;

	GstElement *src;
	GstElement *sink;

	guint       tick_id;
} GSRWindowPipeline;

enum {
	PROP_0,
	PROP_LOCATION
};

static GtkWindowClass *parent_class = NULL;

#define GSR_WINDOW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSR_TYPE_WINDOW, GSRWindowPrivate))

struct _GSRWindowPrivate {
	GtkWidget *main_vbox;
	GtkWidget *scale;
	GtkWidget *profile, *input;
	GtkWidget *rate, *time_sec, *format, *channels;
	GtkWidget *input_label;
	GtkWidget *name_label;
	GtkWidget *length_label;
	GtkWidget *align;
	GtkWidget *volume_label;
	GtkWidget *level;

	gulong seek_id;

	GtkUIManager *ui_manager;
	GtkActionGroup *action_group;
	GtkWidget *recent_view;
	GtkRecentFilter *recent_filter;

	/* statusbar */
	GtkWidget *statusbar;
	guint status_message_cid;
	guint tip_message_cid;

	/* Pipelines */
	GSRWindowPipeline *play;
	GSRWindowPipeline *record;
	char *record_filename;
	char *filename;
	char *extension;
	char *working_file; /* Working file: Operations only occur on the
			       working file. The result of that operation then
			       becomes the new working file. */
	int record_fd;

	/* File info */
	int len_secs; /* In seconds */
	int get_length_attempts;

	/* ATOMIC access */
	struct {
		gint n_channels;
		gint bitrate;
		gint samplerate;
	} atomic;

	gboolean has_file;
	gboolean saved;
	gboolean dirty;
	gboolean seek_in_progress;

	gboolean quit_after_save;

	guint32 tick_id; /* tick_callback timeout ID */
	guint32 record_id; /* record idle callback timeout ID */

	GstElement *ebusy_pipeline;  /* which pipeline we're trying to start */
	guint       ebusy_timeout_id;

	GstElement *source;
	GstMixer *mixer;
};

static gboolean            make_record_source      (GSRWindow         *window);
static void                fill_record_input       (GSRWindow         *window, gchar *selected);
static GSRWindowPipeline * make_record_pipeline    (GSRWindow         *window);
static GSRWindowPipeline * make_play_pipeline      (GSRWindow         *window);

static void
show_error_dialog (GtkWindow   *window,
                   const gchar *debug_message,
                   const gchar *format, ...) G_GNUC_PRINTF (3,4);

static void
show_error_dialog (GtkWindow *win, const gchar *dbg, const gchar * format, ...)
{
	GtkWidget *dialog;
	va_list args;
	gchar *s;

	va_start (args, format);
	s = g_strdup_vprintf (format, args);
	va_end (args);

	dialog = gtk_message_dialog_new (win,
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 "%s",
					 s);

	if (dbg != NULL) {
		g_printerr ("ERROR: %s\nDEBUG MESSAGE: %s\n", s, dbg);
	}

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
	g_free (s);
}

static void
show_missing_known_element_error (GtkWindow *win, gchar *description,
	gchar *element, gchar *plugin, gchar *module)
{
	show_error_dialog (win, NULL,
            _("Could not create the GStreamer %s element.\n"
	      "Please install the '%s' plugin from the '%s' module.\n"
	      "Verify that the installation is correct by running\n"
              "    gst-inspect-0.10 %s\n"
	      "and then restart mate-sound-recorder."),
            description, plugin, module, element);
}

static void
show_profile_error (GtkWindow *win, gchar *debug, gchar *description,
	const char *profile)
{
	gchar *first;

	first = g_strdup_printf (description, profile);
	show_error_dialog (win, debug, "%s%s", first,
		  _("Please verify its settings.\n"
		    "You may be missing the necessary plugins."));
	g_free (first);
}
/* Why do we need this? when a bin changes from READY => NULL state, its
 * bus is set to flushing and we're unlikely to ever see any of its messages
 * if the bin's state reaches NULL before we/the watch in the main thread
 * collects them. That's why we set the state to READY first, process all
 * messages 'manually', and then finally set it to NULL. This makes sure
 * our state-changed handler actually gets to see all the state changes */

static void
set_pipeline_state_to_null (GstElement *pipeline)
{
	GstMessage *msg;
	GstState cur_state, pending;
	GstBus *bus;

	gst_element_get_state (pipeline, &cur_state, &pending, 0);

	if (cur_state == GST_STATE_NULL && pending == GST_STATE_VOID_PENDING)
		return;

	if (cur_state == GST_STATE_NULL && pending != GST_STATE_VOID_PENDING) {
		gst_element_set_state (pipeline, GST_STATE_NULL);
		return;
	}

	gst_element_set_state (pipeline, GST_STATE_READY);
	gst_element_get_state (pipeline, NULL, NULL, -1);

	bus = gst_element_get_bus (pipeline);
	while ((msg = gst_bus_pop (bus))) {
		gst_bus_async_signal_func (bus, msg, NULL);
	}
	gst_object_unref (bus);

	gst_element_set_state (pipeline, GST_STATE_NULL);
	/* maybe we should be paranoid and do _get_state() and check for
	 * the return value here, but then errors in shutdown should be
	 * rather unlikely */
}


static void
shutdown_pipeline (GSRWindowPipeline *pipe)
{
	gst_bus_set_flushing (pipe->bus, TRUE);
	gst_bus_remove_signal_watch (pipe->bus);
	gst_element_set_state (pipe->pipeline, GST_STATE_NULL);
	gst_object_unref (pipe->pipeline);	
	gst_object_unref (pipe->bus);
}

static char *
seconds_to_string (guint seconds)
{
	int hour, min, sec;
	
	min = (seconds / 60);
	hour = min / 60;
	min -= (hour * 60);
	sec = seconds - ((hour * 3600) + (min * 60));

	if (hour > 0) {
		return g_strdup_printf ("%d:%02d:%02d", hour, min, sec);
	} else {
		return g_strdup_printf ("%d:%02d", min, sec);
	}
}	

static char *
seconds_to_full_string (guint seconds)
{
	long days, hours, minutes;
	char *time = NULL;
	const char *minutefmt;
	const char *hourfmt;
	const char *secondfmt;

	days    = seconds / (60 * 60 * 24);
	hours   = (seconds / (60 * 60));
	minutes = (seconds / 60) - ((days * 24 * 60) + (hours * 60));
	seconds = seconds % 60;

	minutefmt = ngettext ("%ld minute", "%ld minutes", minutes);
	hourfmt = ngettext ("%ld hour", "%ld hours", hours);
	secondfmt = ngettext ("%ld second", "%ld seconds", seconds);

	if (hours > 0) {
		if (minutes > 0)
			if (seconds > 0) {
				char *fmt;
				/* Translators: the format is "X hours, X minutes and X seconds" */
				fmt = g_strdup_printf (_("%s, %s and %s"), hourfmt, minutefmt, secondfmt);
				time = g_strdup_printf (fmt, hours, minutes, seconds);
				g_free (fmt);
			} else {
				char *fmt;
				/* Translators: the format is "X hours and X minutes" */
				fmt = g_strdup_printf (_("%s and %s"), hourfmt, minutefmt);
				time = g_strdup_printf (fmt, hours, minutes);
				g_free (fmt);
			}
		else
			if (seconds > 0) {
				char *fmt;
				/* Translators: the format is "X minutes and X seconds" */
				fmt = g_strdup_printf (_("%s and %s"), minutefmt, secondfmt);
				time = g_strdup_printf (fmt, minutes, seconds);
				g_free (fmt);
			} else {
				time = g_strdup_printf (minutefmt, minutes);
			}
	} else {
		if (minutes > 0) {
			if (seconds > 0) {
				char *fmt;
				/* Translators: the format is "X minutes and X seconds" */
				fmt = g_strdup_printf (_("%s and %s"), minutefmt, secondfmt);
				time = g_strdup_printf (fmt, minutes, seconds);
				g_free (fmt);
			} else {
				time = g_strdup_printf (minutefmt, minutes);
			}

		} else {
			time = g_strdup_printf (secondfmt, seconds);
		}
	}

	return time;
}

static void
set_action_sensitive (GSRWindow  *window,
		      const char *name,
		      gboolean    sensitive)
{
	GtkAction *action = gtk_action_group_get_action (window->priv->action_group,
							 name);
	gtk_action_set_sensitive (action, sensitive);
}

static void
file_new_cb (GtkAction *action,
	     GSRWindow *window)
{
	gsr_open_window (NULL);
}

static void
file_open_cb (GtkAction *action,
	      GSRWindow *window)
{
	GtkWidget *file_chooser;
	gchar *directory;
	gchar *locale_directory = NULL;
	gint response;

	g_return_if_fail (GSR_IS_WINDOW (window));

	file_chooser = gtk_file_chooser_dialog_new (_("Open a File"),
						    GTK_WINDOW (window),
						    GTK_FILE_CHOOSER_ACTION_OPEN,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						    NULL);

	directory = mateconf_client_get_string (mateconf_client, KEY_OPEN_DIR, NULL);

	if (directory != NULL && *directory != 0) {
		locale_directory = g_filename_from_utf8 (directory, -1, NULL, NULL, NULL);
		if (!locale_directory || !g_file_test (locale_directory, G_FILE_TEST_EXISTS))
			locale_directory = g_strdup (directory);
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser),
		                                     locale_directory);
		g_free (locale_directory);
	}

	response = gtk_dialog_run (GTK_DIALOG (file_chooser));

	if (response == GTK_RESPONSE_OK) {
		gchar *name;
		gchar *utf8_name = NULL;

		name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
		if (name) {
			gchar *dirname;

			utf8_name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
			dirname = g_path_get_dirname (utf8_name);
			mateconf_client_set_string (mateconf_client, KEY_OPEN_DIR, dirname, NULL);
			g_free (dirname);
			g_free (utf8_name);

			if (window->priv->has_file == TRUE) {
				/* Just open a new window with the file */
				gsr_open_window (name);
			} else {
				/* Set the file in this window */
				g_object_set (G_OBJECT (window), "location", name, NULL);
				window->priv->dirty = FALSE;
			}
		
			g_free (name);
		}
    }

	gtk_widget_destroy (file_chooser);
	g_free (directory);
}

static void
file_open_recent_cb (GtkRecentChooser *chooser,
		     GSRWindow *window)
{
	gchar *uri;
	gchar *filename;

	uri = gtk_recent_chooser_get_current_uri (chooser);
	g_return_if_fail (uri != NULL);

	if (!g_str_has_prefix (uri, "file://"))
		return;

	filename = g_filename_from_uri (uri, NULL, NULL);
	if (filename == NULL)
		goto out;

	if (!g_file_test (filename, G_FILE_TEST_EXISTS)) {
		gchar *filename_utf8;
		GtkWidget *dlg;

		filename_utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
		dlg = gtk_message_dialog_new (GTK_WINDOW (window),
					      GTK_DIALOG_MODAL,
					      GTK_MESSAGE_ERROR,
					      GTK_BUTTONS_OK,
					      _("Unable to load file:\n%s"), filename_utf8);

		gtk_widget_show (dlg);
		gtk_dialog_run (GTK_DIALOG (dlg));
		gtk_widget_destroy (dlg);

		gtk_recent_manager_remove_item (gtk_recent_manager_get_default (), uri, NULL);

		g_free (filename_utf8);
		goto out;
  	}

	if (window->priv->has_file == TRUE) {
		/* Just open a new window with the file */
		gsr_open_window (filename);
	} else {
		/* Set the file in this window */
		g_object_set (G_OBJECT (window), "location", filename, NULL);
		window->priv->dirty = FALSE;
	}

 out:
	g_free (filename);
	g_free (uri);
}

#if 0
static gboolean
cb_iterate (GstBin  *bin,
	    gpointer data)
{
	src = gst_element_get_child (bin, "sink");
	sink = gst_element_get_child (bin, "sink");
	
	if (src && sink) {
		gint64 pos, tot, enc;
		GstFormat fmt = GST_FORMAT_BYTES;

		gst_element_query (src, GST_QUERY_POSITION, &fmt, &pos);
		gst_element_query (src, GST_QUERY_TOTAL, &fmt, &tot);
		gst_element_query (sink, GST_QUERY_POSITION, &fmt, &enc);

		g_print ("Iterate: %lld/%lld -> %lld\n", pos, tot, enc);
	} else
		g_print ("Iterate ?\n");

	/* we don't do anything here */
	return FALSE;
}
#endif

static gboolean
handle_ebusy_error (GSRWindow *window)
{
	g_return_val_if_fail (window->priv->ebusy_pipeline != NULL, FALSE);

	gst_element_set_state (window->priv->ebusy_pipeline, GST_STATE_NULL);
	gst_element_get_state (window->priv->ebusy_pipeline, NULL, NULL, -1);
	gst_element_set_state (window->priv->ebusy_pipeline, GST_STATE_PLAYING);

	/* Try only once */
	return FALSE;
}

static GstElement *
notgst_element_get_toplevel (GstElement * element)
{
	g_return_val_if_fail (element != NULL, NULL);
	g_return_val_if_fail (GST_IS_ELEMENT (element), NULL);

	do {
		GstElement *parent;

		parent = (GstElement *) gst_element_get_parent (element);

		if (parent == NULL)
			break;

		gst_object_unref (parent);
		element = parent;
	} while (1);

	return element;
}

static void
pipeline_error_cb (GstBus * bus, GstMessage * msg, GSRWindow * window)
{
	GstElement *pipeline;
	GError *error = NULL;
	gchar *dbg = NULL;

	g_return_if_fail (GSR_IS_WINDOW (window));

	gst_message_parse_error (msg, &error, &dbg);
	g_return_if_fail (error != NULL);

	pipeline = notgst_element_get_toplevel (GST_ELEMENT (msg->src));

	if (error->code == GST_RESOURCE_ERROR_BUSY) {
		if (window->priv->ebusy_timeout_id == 0) {
			set_action_sensitive (window, "FileSave", FALSE);
			set_action_sensitive (window, "FileSaveAs", FALSE);
			set_action_sensitive (window, "Play", FALSE);
			set_action_sensitive (window, "Record", FALSE);

			window->priv->ebusy_pipeline = pipeline;

			window->priv->ebusy_timeout_id = 
				g_timeout_add_seconds (EBUSY_TRY_AGAIN,
						       (GSourceFunc) handle_ebusy_error,
						       window);

			g_error_free (error);
			g_free (dbg);
			return;
		}
	}

	if (window->priv->ebusy_timeout_id) {
		g_source_remove (window->priv->ebusy_timeout_id);
		window->priv->ebusy_timeout_id = 0;
		window->priv->ebusy_pipeline = NULL;
	}


	/* set pipeline to NULL before showing error dialog to make sure
	 * the audio device is freed, in case any accessability software
	 * wants to make use of it to read out the error message */
	set_pipeline_state_to_null (pipeline);

	show_error_dialog (GTK_WINDOW (window), dbg, "%s", error->message);

	gdk_window_set_cursor (gtk_widget_get_window (window->priv->main_vbox), NULL);

	set_action_sensitive (window, "Stop", FALSE);
	set_action_sensitive (window, "Play", TRUE);
	set_action_sensitive (window, "Record", TRUE);
	set_action_sensitive (window, "FileSave", TRUE);
	set_action_sensitive (window, "FileSaveAs", TRUE);
	gtk_widget_set_sensitive (window->priv->scale, TRUE);

	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->status_message_cid);
	gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
			    window->priv->status_message_cid,
			    _("Ready"));

	g_error_free (error);
	g_free (dbg);
}

static GtkWidget *
gsr_dialog_add_button (GtkDialog *dialog,
		       const gchar *text,
		       const gchar *stock_id,
		       gint response_id)
{
	GtkWidget *button;

	g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (stock_id != NULL, NULL);

	button = gtk_button_new_with_mnemonic (text);
	gtk_button_set_image (GTK_BUTTON (button),
			      gtk_image_new_from_stock (stock_id,
							GTK_ICON_SIZE_BUTTON));

	gtk_widget_set_can_default (button, TRUE);

	gtk_widget_show (button);

	gtk_dialog_add_action_widget (dialog, button, response_id);

	return button;
}

static gboolean
replace_dialog (GtkWindow *parent,
		const gchar *message,
		const gchar *file_name)
{
	GtkWidget *message_dialog;
	gint ret;

	g_return_val_if_fail (file_name != NULL, FALSE);

	message_dialog = gtk_message_dialog_new (parent,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_QUESTION,
						 GTK_BUTTONS_NONE,
						 message,
						 file_name);
	/* Add cancel button */
	gtk_dialog_add_button (GTK_DIALOG (message_dialog),
			       GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);
	/* Add replace button */
	gsr_dialog_add_button (GTK_DIALOG (message_dialog), _("_Replace"),
			       GTK_STOCK_REFRESH,
			       GTK_RESPONSE_YES);

	gtk_dialog_set_default_response (GTK_DIALOG (message_dialog), GTK_RESPONSE_CANCEL);
	gtk_window_set_resizable (GTK_WINDOW (message_dialog), FALSE);
	ret = gtk_dialog_run (GTK_DIALOG (message_dialog));
	gtk_widget_destroy (GTK_WIDGET (message_dialog));

	return (ret == GTK_RESPONSE_YES);
}

static gboolean
replace_existing_file (GtkWindow *parent,
		      const gchar *file_name) 
{
	return replace_dialog (parent,
			       _("A file named \"%s\" already exists. \n"
			       "Do you want to replace it with the "
			       "one you are saving?"),
			       file_name);
}

static void
do_save_file (GSRWindow *window,
	      const char *_name)
{
	GSRWindowPrivate *priv;
	char *name;
	GFile *src, *dst;
	GError *error = NULL;

	priv = window->priv;

	if (window->priv->extension == NULL ||
	    g_str_has_suffix (_name, window->priv->extension))
		name = g_strdup (_name);
	else
		name = g_strdup_printf ("%s.%s", _name,
				window->priv->extension);
	if (g_file_test (name, G_FILE_TEST_EXISTS)) {
		char *utf8_name;
		utf8_name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
		if (!replace_existing_file (GTK_WINDOW (window), utf8_name)) {
			g_free (utf8_name);
			return;
		}
		g_free (utf8_name);
	}
	src = g_file_new_for_path(priv->record_filename);
	dst = g_file_new_for_path(name);

	/* TODO: Show progress? Where? */
	if (g_file_copy(src, dst, G_FILE_COPY_OVERWRITE,
	                NULL, NULL, NULL, &error)) {
		g_object_set (G_OBJECT (window), "location", name, NULL);
		priv->dirty = FALSE;
		window->priv->saved = TRUE;
		if (window->priv->quit_after_save == TRUE) {
			gsr_window_close (window);
		}
	} else {
		char *utf8_name;
		utf8_name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
		show_error_dialog (GTK_WINDOW (window), NULL,
			           _("Could not save the file \"%s\""), utf8_name);
		g_free (utf8_name);
	}

	g_object_unref(src);
	g_object_unref(dst);
	g_free (name);
}

static void
file_save_as_cb (GtkAction *action,
		 GSRWindow *window)
{
	GtkWidget *file_chooser;
	gchar *directory;
	gchar *locale_directory = NULL;
	gint response;

	g_return_if_fail (GSR_IS_WINDOW (window));

	file_chooser = gtk_file_chooser_dialog_new (_("Save file as"),
						    GTK_WINDOW (window),
						    GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE, GTK_RESPONSE_OK,
						    NULL);

	directory = mateconf_client_get_string (mateconf_client, KEY_SAVE_DIR, NULL);
	if (directory != NULL && *directory != 0) {
		locale_directory = g_filename_from_utf8 (directory, -1, NULL, NULL, NULL);
		if (!locale_directory || !g_file_test (locale_directory, G_FILE_TEST_EXISTS))
			locale_directory = g_strdup (directory);
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), locale_directory);
		g_free (locale_directory);
	}
	g_free (directory);

	if (window->priv->filename != NULL) {
		char *locale_basename;
		char *basename = NULL;
		gchar *filename, *filename_ext, *extension;
		gint length;

		locale_basename = g_path_get_basename (window->priv->filename);
		basename = g_filename_to_utf8 (locale_basename, -1, NULL, NULL, NULL);
		length = strlen (basename);
		extension = g_strrstr (basename, ".");

		if (extension != NULL) {
			length = length - strlen (extension);
		}

		filename = g_strndup (basename,length);
		if (window->priv->extension)
			filename_ext = g_strdup_printf ("%s.%s", filename,
						window->priv->extension);
		else
			filename_ext = g_strdup (filename);
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_chooser),
						   filename_ext);
		g_free (filename);
		g_free (filename_ext);
		g_free (basename);
		g_free (locale_basename);
	}

	response = gtk_dialog_run (GTK_DIALOG (file_chooser));

	if (response == GTK_RESPONSE_OK) {
		gchar *name;
		gchar *utf8_name = NULL;

		name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
		if (name) {
			gchar *dirname;

			utf8_name= g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
			dirname = g_path_get_dirname (utf8_name);
			mateconf_client_set_string (mateconf_client, KEY_SAVE_DIR, dirname, NULL);
			g_free (dirname);
			g_free (utf8_name);
	
			do_save_file (window, name);
			g_free (name);
	    }
	}

	gtk_widget_destroy (file_chooser);
}

static void
file_save_cb (GtkAction *action,
	      GSRWindow *window)
{
	if (!window->priv->has_file) {
		file_save_as_cb (NULL, window);
	} else {
		do_save_file (window, window->priv->filename);
	}
}

static void
run_mixer_cb (GtkAction *action,
	       GSRWindow *window)
{
	char *mixer_path;
	char *argv[4] = {NULL, "--page", "recording",  NULL};
	GError *error = NULL;
	gboolean ret;

	/* Open the mixer */
	mixer_path = g_find_program_in_path ("mate-volume-control");
	if (mixer_path == NULL) {
		show_error_dialog (GTK_WINDOW (window), NULL,
		                   _("%s is not installed in the path."),
		                   "mate-volume-control");
		return;
	}

	argv[0] = mixer_path;
	ret = g_spawn_async (NULL, argv, NULL, 0, NULL, NULL, NULL, &error);
	if (ret == FALSE) {
		show_error_dialog (GTK_WINDOW (window), NULL,
		                   _("There was an error starting %s: %s"),
		                   mixer_path, error->message);
		g_error_free (error);
	}

	g_free (mixer_path);
}

gboolean
gsr_window_is_saved (GSRWindow *window)
{
	return window->priv->saved;
}

gboolean
gsr_discard_confirmation_dialog (GSRWindow *window, gboolean closing)
{
	GtkWidget *confirmation_dialog;
	AtkObject *atk_obj;
	gint response_id;
	gboolean ret = TRUE;

	confirmation_dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
								  GTK_DIALOG_MODAL,
								  GTK_MESSAGE_WARNING,
								  GTK_BUTTONS_NONE,
								  "<span weight=\"bold\" size=\"larger\">%s</span>",
								  closing ?
								    _("Save recording before closing?") :
								    _("Save recording?"));

	gtk_dialog_add_buttons (GTK_DIALOG (confirmation_dialog),
				closing ?
				  _("Close _without Saving") :
				  _("Continue _without Saving"),
				GTK_RESPONSE_YES,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE_AS, GTK_RESPONSE_NO, NULL);
	gtk_dialog_set_default_response (GTK_DIALOG (confirmation_dialog),
				GTK_RESPONSE_NO);

	gtk_window_set_title (GTK_WINDOW (confirmation_dialog), "");

	atk_obj = gtk_widget_get_accessible (confirmation_dialog);
	atk_object_set_name (atk_obj, _("Question"));

	response_id = gtk_dialog_run (GTK_DIALOG (confirmation_dialog));

	switch (response_id) {
		case GTK_RESPONSE_NO:
		/* hiding the confirmation dialog allows the user to
		see only one dialog at a time if the user click cancel
		in the file dialog, they won't expect to return to the
		confirmation dialog*/
			gtk_widget_hide (confirmation_dialog);
			file_save_as_cb (NULL, window);
			ret = window->priv->has_file;
			break;

		case GTK_RESPONSE_YES:
			ret = TRUE;
			break;

		case GTK_RESPONSE_CANCEL:
		default:
			ret = FALSE;
			break;
	}

	gtk_widget_destroy (confirmation_dialog);

	return ret;
}

static GtkWidget *
make_title_label (const char *text)
{
	GtkWidget *label;
	char *fulltext;
	
	fulltext = g_strdup_printf ("<span weight=\"bold\">%s</span>", text);
	label = gtk_label_new (fulltext);
	g_free (fulltext);

	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.0);
	return label;
}

static GtkWidget *
make_info_label (const char *text)
{
	GtkWidget *label;

	label = gtk_label_new (text);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (label), GTK_WRAP_WORD);

	return label;
}

static void
pack_table_widget (GtkWidget *table,
		   GtkWidget *widget,
		   int left,
		   int top)
{
	gtk_table_attach (GTK_TABLE (table), widget,
			  left, left + 1, top, top + 1,
			  GTK_FILL, GTK_FILL, 0, 0);
}

struct _file_props {
	GtkWidget *dialog;

	GtkWidget *dirname;
	GtkWidget *filename;
	GtkWidget *size;
	GtkWidget *length;
	GtkWidget *samplerate;
	GtkWidget *channels;
	GtkWidget *bitrate;
};

static void
fill_in_information (GSRWindow *window,
		     struct _file_props *fp)
{
	struct stat buf;
	guint64 file_size = 0;
	gchar *text, *name;
	gchar *utf8_name = NULL;
	gint n_channels, bitrate, samplerate;

	/* dirname */
	if (window->priv->dirty) {
		gtk_label_set_text (GTK_LABEL (fp->dirname), "");
	} else {
		name = g_path_get_dirname (window->priv->filename);
		text = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);
		gtk_label_set_text (GTK_LABEL (fp->dirname), text);
		g_free (text);
		g_free (name);
	}

	/* filename */
	name = g_path_get_basename (window->priv->filename);
	utf8_name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);

	if (window->priv->dirty) {
		text = g_strdup_printf (_("%s (Has not been saved)"), utf8_name);
	} else {
		text = g_strdup (utf8_name);
	}
	gtk_label_set_text (GTK_LABEL (fp->filename), text);
	g_free (text);
	g_free (utf8_name);
	g_free (name);
	
	/* Size */
	if (stat (window->priv->working_file, &buf) == 0) {
		gchar *human;

		file_size = (guint64) buf.st_size;
		human = g_format_size_for_display (file_size);

		text = g_strdup_printf (ngettext ("%s (%llu byte)", "%s (%llu bytes)",
		                        file_size), human, file_size);
		g_free (human);
	} else {
		text = g_strdup (_("Unknown size"));
	}
	gtk_label_set_text (GTK_LABEL (fp->size), text);
	g_free (text);

	/* FIXME: Set up and run our own pipeline 
	 *        till we can get the info */
	/* Length */
	if (window->priv->len_secs == 0) {
		text = g_strdup (_("Unknown"));
	} else {
		text = seconds_to_full_string (window->priv->len_secs);
	}
	gtk_label_set_text (GTK_LABEL (fp->length), text);
	g_free (text);

	/* sample rate */
	samplerate = g_atomic_int_get (&window->priv->atomic.samplerate);
	if (samplerate == 0) {
		text = g_strdup (_("Unknown"));
	} else {
		text = g_strdup_printf (_("%.1f kHz"), (float) samplerate / 1000);
	}
	gtk_label_set_text (GTK_LABEL (fp->samplerate), text);
	g_free (text);

	/* bit rate */
	bitrate = g_atomic_int_get (&window->priv->atomic.bitrate);
	if (bitrate > 0) {
		text = g_strdup_printf (_("%.0f kb/s"), (float) bitrate / 1000);
	} else if (window->priv->len_secs > 0 && file_size > 0) {
		bitrate = (file_size * 8.0) / window->priv->len_secs;
		text = g_strdup_printf (_("%.0f kb/s (Estimated)"),
		                        (float) bitrate / 1000);
	} else {
		text = g_strdup (_("Unknown"));
	}
	gtk_label_set_text (GTK_LABEL (fp->bitrate), text);
	g_free (text);

	/* channels */
	n_channels = g_atomic_int_get (&window->priv->atomic.n_channels);
	switch (n_channels) {
	case 0:
		text = g_strdup (_("Unknown"));
		break;
	case 1:
		text = g_strdup (_("1 (mono)"));
		break;
	case 2:
		text = g_strdup (_("2 (stereo)"));
		break;
	default:
		text = g_strdup_printf ("%d", n_channels);
		break;
	}
	gtk_label_set_text (GTK_LABEL (fp->channels), text);
	g_free (text);
}

static void
dialog_closed_cb (GtkDialog *dialog,
		  guint response_id,
		  struct _file_props *fp)
{
	gtk_widget_destroy (fp->dialog);
	g_free (fp);
}

static void
file_properties_cb (GtkAction *action,
		    GSRWindow *window)
{
	GtkWidget *dialog, *vbox, *inner_vbox, *hbox, *table, *label;
	char *title, *shortname;
	struct _file_props *fp;
	shortname = g_path_get_basename (window->priv->filename);
	title = g_strdup_printf (_("%s Information"), shortname);
	g_free (shortname);

	dialog = gtk_dialog_new_with_buttons (title, GTK_WINDOW (window),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	g_free (title);
#if !GTK_CHECK_VERSION (2, 21, 8)
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
#endif
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 2);
	fp = g_new (struct _file_props, 1);
	fp->dialog = dialog;

	g_signal_connect (G_OBJECT (dialog), "response",
			  G_CALLBACK (dialog_closed_cb), fp);

	vbox = gtk_vbox_new (FALSE, 18);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), vbox, TRUE, TRUE, 0);

	inner_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), inner_vbox, FALSE, FALSE,0);

	label = make_title_label (_("File Information"));
	gtk_box_pack_start (GTK_BOX (inner_vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (inner_vbox), hbox, TRUE, TRUE, 0);

	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	/* File properties */	
	table = gtk_table_new (3, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

	label = make_info_label (_("Folder:"));
	pack_table_widget (table, label, 0, 0);

	fp->dirname = make_info_label ("");
	pack_table_widget (table, fp->dirname, 1, 0);

	label = make_info_label (_("Filename:"));
	pack_table_widget (table, label, 0, 1);

	fp->filename = make_info_label ("");
	pack_table_widget (table, fp->filename, 1, 1);

	label = make_info_label (_("File size:"));
	pack_table_widget (table, label, 0, 2);

	fp->size = make_info_label ("");
	pack_table_widget (table, fp->size, 1, 2);

	inner_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), inner_vbox, FALSE, FALSE, 0);

	label = make_title_label (_("Audio Information"));
	gtk_box_pack_start (GTK_BOX (inner_vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (inner_vbox), hbox, TRUE, TRUE, 0);

	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	/* Audio info */
	table = gtk_table_new (4, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

	label = make_info_label (_("File duration:"));
	pack_table_widget (table, label, 0, 0);

	fp->length = make_info_label ("");
	pack_table_widget (table, fp->length, 1, 0);

	label = make_info_label (_("Number of channels:"));
	pack_table_widget (table, label, 0, 1);

	fp->channels = make_info_label ("");
	pack_table_widget (table, fp->channels, 1, 1);

	label = make_info_label (_("Sample rate:"));
	pack_table_widget (table, label, 0, 2);

	fp->samplerate = make_info_label ("");
	pack_table_widget (table, fp->samplerate, 1, 2);

	label = make_info_label (_("Bit rate:"));
	pack_table_widget (table, label, 0, 3);

	fp->bitrate = make_info_label ("");
	pack_table_widget (table, fp->bitrate, 1, 3);

	fill_in_information (window, fp);
	gtk_widget_show_all (dialog);
}

void
gsr_window_close (GSRWindow *window)
{
	gtk_widget_destroy (GTK_WIDGET (window));
}

static void
file_close_cb (GtkAction *action,
	       GSRWindow *window)
{
	if (gsr_window_is_saved (window) || gsr_discard_confirmation_dialog (window, TRUE))
		gsr_window_close (window);
}

static void
quit_cb (GtkAction *action,
	 GSRWindow *window)
{
	gsr_quit ();
}

static void
help_contents_cb (GtkAction *action,
	          GSRWindow *window)
{
	GError *error = NULL;

	gtk_show_uri (gtk_window_get_screen (GTK_WINDOW (window)),
		      "ghelp:mate-sound-recorder",
		      gtk_get_current_event_time (), &error);

	if (error != NULL)
	{
		g_warning ("%s", error->message);

		g_error_free (error);
	}
}

static void
about_cb (GtkAction *action,
	  GSRWindow *window)
{
	const char * const authors[] = {"Iain Holmes <iain@prettypeople.org>", 
		"Ronald Bultje <rbultje@ronald.bitfreak.net>", 
		"Johan Dahlin  <johan@mate.org>", 
		"Tim-Philipp M\303\274ller <tim centricular net>",
		NULL};
	const char * const documenters[] = {"Sun Microsystems", NULL};
 
	gtk_show_about_dialog (GTK_WINDOW (window),
			       "name", _("Sound Recorder"),
			       "version", VERSION,
			       "copyright", "Copyright \xc2\xa9 2002 Iain Holmes",
			       "comments", _("A sound recorder for MATE\n mate-multimedia@mate.org"),
			       "authors", authors,
			       "documenters", documenters,
			       "logo-icon-name", "mate-sound-recorder",
			       NULL);
}

static void
play_cb (GtkAction *action,
	 GSRWindow *window)
{
	GSRWindowPrivate *priv = window->priv;

	if (priv->has_file == FALSE && !priv->working_file)
		return;

	if (priv->play) {
		shutdown_pipeline (priv->play);
	}

	if ((priv->play = make_play_pipeline (window))) {
		gchar *uri;
		gchar *usefile;
		GFile *file;

		if(priv->has_file == FALSE && priv->working_file) usefile = priv->working_file;
		else usefile = priv->filename;

		file = g_file_new_for_commandline_arg (usefile);
		uri = g_file_get_uri (file);
		g_object_unref (file);
		g_object_set (window->priv->play->pipeline, "uri", uri, NULL);
		g_free (uri);

		if (priv->record && priv->record->state == GST_STATE_PLAYING) {
			set_pipeline_state_to_null (priv->record->pipeline);
		}

		gst_element_set_state (priv->play->pipeline, GST_STATE_PLAYING);
	}
}

static void
stop_cb (GtkAction *action,
	 GSRWindow *window)
{
	GSRWindowPrivate *priv = window->priv;

	/* Work out what's playing */
	if (priv->play && priv->play->state >= GST_STATE_PAUSED) {
		GST_DEBUG ("Stopping play pipeline");
		set_pipeline_state_to_null (priv->play->pipeline);
	} else if (priv->record && priv->record->state == GST_STATE_PLAYING) {
		GST_DEBUG ("Stopping recording source");
		/* GstBaseSrc will automatically send an EOS when stopping */
		gst_element_set_state (priv->record->src, GST_STATE_NULL);
		gst_element_get_state (priv->record->src, NULL, NULL, -1);
		gst_element_set_locked_state (priv->record->src, TRUE);

		GST_DEBUG ("Stopping recording pipeline");
		set_pipeline_state_to_null (priv->record->pipeline);
		gtk_widget_set_sensitive (window->priv->level, FALSE);
		gtk_widget_set_sensitive (window->priv->volume_label, FALSE);
		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (window->priv->level), 0.0);
	}
}

static void
record_cb (GtkAction *action,
	   GSRWindow *window)
{
	if (!gsr_window_is_saved(window) && !gsr_discard_confirmation_dialog (window, FALSE))
		return;

	GSRWindowPrivate *priv = window->priv;

	if (priv->record) {
		char *current_source;
		shutdown_pipeline (priv->record);
		if (!make_record_source (window))
			return;
		current_source = gtk_combo_box_get_active_text (GTK_COMBO_BOX (window->priv->input));
		fill_record_input (window, current_source);
	}

	if ((priv->record = make_record_pipeline (window))) {
		window->priv->len_secs = 0;
		window->priv->saved = FALSE;

		g_print ("%s", priv->record_filename);
		g_object_set (G_OBJECT (priv->record->sink),
			      "location", priv->record_filename,
			      NULL);

		gst_element_set_state (priv->record->pipeline, GST_STATE_PLAYING);
		gtk_widget_set_sensitive (window->priv->level, TRUE);
		gtk_widget_set_sensitive (window->priv->volume_label, TRUE);

	}
}

static gboolean
seek_started (GtkRange *range,
	      GdkEventButton *event,
	      GSRWindow *window)
{
	g_return_val_if_fail (window->priv != NULL, FALSE);

	window->priv->seek_in_progress = TRUE;
	return FALSE;
}

static gboolean
seek_to (GtkRange *range,
	 GdkEventButton *gdkevent,
	 GSRWindow *window)
{
	gdouble value;
	gint64 time;

	if (window->priv->play->state < GST_STATE_PLAYING)
		return FALSE;

	value = gtk_adjustment_get_value (gtk_range_get_adjustment (range));
	time = ((value / 100.0) * window->priv->len_secs) * GST_SECOND;

	gst_element_seek (window->priv->play->pipeline, 1.0, GST_FORMAT_TIME,
	                  GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, time,
	                  GST_SEEK_TYPE_NONE, 0);

	window->priv->seek_in_progress = FALSE;

	return FALSE;
}

static gboolean
play_tick_callback (GSRWindow *window)
{
	GstElement *playbin;
	GstFormat format = GST_FORMAT_TIME;
	gint64 val = -1;

	g_return_val_if_fail (window->priv->play != NULL, FALSE);
	g_return_val_if_fail (window->priv->play->pipeline != NULL, FALSE);

	playbin = window->priv->play->pipeline;

	/* This check stops us from doing an unnecessary query */
	if (window->priv->play->state != GST_STATE_PLAYING) {
		GST_DEBUG ("pipeline in wrong state: %s",
			gst_element_state_get_name (window->priv->play->state));
		window->priv->play->tick_id = 0;
		return FALSE;
	}

	if (gst_element_query_duration (playbin, &format, &val) && val != -1) {
		gchar *len_str;

		window->priv->len_secs = val / GST_SECOND;

		len_str = seconds_to_full_string (window->priv->len_secs);
		gtk_label_set_text (GTK_LABEL (window->priv->length_label),
				    len_str);
		g_free (len_str);
	} else {
		if (window->priv->get_length_attempts <= 0) {
			/* Attempts to get length ran out. */
			gtk_label_set_text (GTK_LABEL (window->priv->length_label), _("Unknown"));
		} else {
			--window->priv->get_length_attempts;
		}
	}

	if (window->priv->seek_in_progress) {
		GST_DEBUG ("seek in progress, try again later");
		return TRUE;
	}

	if (window->priv->len_secs == 0) {
		GST_DEBUG ("no duration, try again later");
		return TRUE;
	}

	if (gst_element_query_position (playbin, &format, &val) && val != -1) {
		gdouble pos, len, percentage;

		pos = (gdouble) (val - (val % GST_SECOND));
		len = (gdouble) window->priv->len_secs * GST_SECOND;
		percentage = pos / len * 100.0;

		gtk_adjustment_set_value (gtk_range_get_adjustment (GTK_RANGE (window->priv->scale)),
		                          CLAMP (percentage + 0.5, 0.0, 100.0));
	} else {
		GST_DEBUG ("failed to query position");
	}

	return TRUE;
}

static gboolean
record_tick_callback (GSRWindow *window)
{
	GstElement *pipeline;
	GstFormat format = GST_FORMAT_TIME;
	gint64 val = -1;
	gint secs;

	/* This check stops us from doing an unnecessary query */
	if (window->priv->record->state != GST_STATE_PLAYING) {
		GST_DEBUG ("pipeline in wrong state: %s",
			gst_element_state_get_name (window->priv->record->state));
		return FALSE;
    }

	if (window->priv->seek_in_progress)
		return TRUE;

	pipeline = window->priv->record->pipeline;

	if (gst_element_query_position (pipeline, &format, &val) && val != -1) {
		gchar* len_str;

		secs = val / GST_SECOND;
		
		len_str = seconds_to_full_string (secs);
		window->priv->len_secs = secs;
		gtk_label_set_text (GTK_LABEL (window->priv->length_label),
				    len_str);
		g_free (len_str);
	} else {
		GST_DEBUG ("failed to query position");
	}

	return TRUE;
}

static void
play_state_changed_cb (GstBus * bus, GstMessage * msg, GSRWindow * window)
{
	GstState new_state;

	gst_message_parse_state_changed (msg, NULL, &new_state, NULL);

	g_return_if_fail (GSR_IS_WINDOW (window));

	/* we are only interested in state changes of the top-level pipeline */
	if (msg->src != GST_OBJECT (window->priv->play->pipeline))
		return;

	window->priv->play->state = new_state;

	GST_DEBUG ("playbin state: %s", gst_element_state_get_name (new_state));

	switch (new_state) {
	case GST_STATE_PLAYING:
		if (window->priv->play->tick_id == 0) {
			window->priv->play->tick_id =
				g_timeout_add (200, (GSourceFunc) play_tick_callback, window);
		}

		set_action_sensitive (window, "Stop", TRUE);
		set_action_sensitive (window, "Play", FALSE);
		set_action_sensitive (window, "Record", FALSE);
		set_action_sensitive (window, "FileSave", FALSE);
		set_action_sensitive (window, "FileSaveAs", FALSE);
		gtk_widget_set_sensitive (window->priv->scale, TRUE);

		gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
				   window->priv->status_message_cid);
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->status_message_cid,
				    _("Playing…"));

		if (window->priv->ebusy_timeout_id) {
			g_source_remove (window->priv->ebusy_timeout_id);
			window->priv->ebusy_timeout_id = 0;
			window->priv->ebusy_pipeline = NULL;
		}
		break;

	case GST_STATE_READY:
		if (window->priv->play->tick_id > 0) {
			g_source_remove (window->priv->play->tick_id);
			window->priv->play->tick_id = 0;
		}
		gtk_adjustment_set_value (gtk_range_get_adjustment (GTK_RANGE (window->priv->scale)), 0.0);
		gtk_widget_set_sensitive (window->priv->scale, FALSE);
		/* fallthrough */
	case GST_STATE_PAUSED:
		set_action_sensitive (window, "Stop", FALSE);
		set_action_sensitive (window, "Play", TRUE);
		set_action_sensitive (window, "Record", TRUE);
		set_action_sensitive (window, "FileSave", TRUE);
		set_action_sensitive (window, "FileSaveAs", TRUE);

		gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
				   window->priv->status_message_cid);
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->status_message_cid,
				    _("Ready"));
		break;
	default:
		break;
	}
}

static void
pipeline_deep_notify_caps_cb (GstObject  *pipeline,
                              GstObject  *object,
                              GParamSpec *pspec,
                              GSRWindow  *window)
{
	GSRWindowPrivate *priv;
	GstPadDirection direction;

	if (!GST_IS_PAD (object))
		return;

	priv = window->priv;
	if (priv->play && pipeline == GST_OBJECT_CAST (priv->play->pipeline)) {
		direction = GST_PAD_SRC;
	} else if (priv->record && pipeline == GST_OBJECT_CAST (priv->record->pipeline)) {
		direction = GST_PAD_SINK;
	} else {
		g_return_if_reached ();
	}

	if (GST_PAD_DIRECTION (object) == direction) {
		GstObject *pad_parent;

		pad_parent = gst_object_get_parent (object);
		if (GST_IS_ELEMENT (pad_parent)) {
			GstElementFactory *factory;
			GstElement *element;
			const gchar *klass;

			element = GST_ELEMENT_CAST (pad_parent);
			if ((factory = gst_element_get_factory (element)) &&
			    (klass = gst_element_factory_get_klass (factory)) &&
			    strstr (klass, "Audio") &&
			    (strstr (klass, "Decoder") || strstr (klass, "Encoder"))) {
				GstCaps *caps;

				caps = gst_pad_get_negotiated_caps (GST_PAD_CAST (object));
				if (caps) {
					GstStructure *s;
					gint val;

					s = gst_caps_get_structure (caps, 0);
					if (gst_structure_get_int (s, "channels", &val)) {
						gst_atomic_int_set (&priv->atomic.n_channels, val);
					}
					if (gst_structure_get_int (s, "rate", &val)) {
						gst_atomic_int_set (&priv->atomic.samplerate, val);
					}
					gst_caps_unref (caps);
				}
			}
		}
		if (pad_parent)
			gst_object_unref (pad_parent);
	}
}

/* callback for when the recording profile has been changed */
static void
profile_changed_cb (GObject *object, GSRWindow *window)
{
	GMAudioProfile *profile;
	gchar *id;

	g_return_if_fail (GTK_IS_COMBO_BOX (object));

	profile = gm_audio_profile_choose_get_active (GTK_WIDGET (object));

	if (profile != NULL) {
		id = g_strdup (gm_audio_profile_get_id (profile));
		GST_DEBUG ("profile changed to %s", GST_STR_NULL (id));
		mateconf_client_set_string (mateconf_client, KEY_LAST_PROFILE_ID, id, NULL);
		g_free (id);
	}
}

static void
play_eos_msg_cb (GstBus * bus, GstMessage * msg, GSRWindow * window)
{
	g_return_if_fail (GSR_IS_WINDOW (window));

	GST_DEBUG ("EOS");

	stop_cb (NULL, window);
}

static GSRWindowPipeline *
make_play_pipeline (GSRWindow *window)
{
	GSRWindowPipeline *obj;
	GstElement *playbin;
	GstElement *audiosink;

	audiosink = gst_element_factory_make ("mateconfaudiosink", "sink");
	if (audiosink == NULL) {
		show_missing_known_element_error (NULL,
		    _("MateConf audio output"), "mateconfaudiosink", "mateconfelements",
		    "gst-plugins-good");
		return NULL;
	}

	playbin = gst_element_factory_make ("playbin", "playbin");
	if (playbin == NULL) {
		gst_object_unref (audiosink);
		show_missing_known_element_error (NULL,
		    _("Playback"), "playbin", "playback",
		    "gst-plugins-base");
		return NULL;
	}

	obj = g_new0 (GSRWindowPipeline, 1);
	obj->pipeline = playbin;
	obj->src = NULL; /* don't need that for playback */
	obj->sink = NULL; /* don't need that for playback */

	g_object_set (playbin, "audio-sink", audiosink, NULL);

	/* we ultimately want to find out the caps on the decoder's source pad */
	g_signal_connect (playbin, "deep-notify::caps",
	                  G_CALLBACK (pipeline_deep_notify_caps_cb),
	                  window);

	obj->bus = gst_element_get_bus (playbin);

	gst_bus_add_signal_watch_full (obj->bus, G_PRIORITY_HIGH);

	g_signal_connect (obj->bus, "message::state-changed",
	                  G_CALLBACK (play_state_changed_cb),
	                  window);

	g_signal_connect (obj->bus, "message::error",
	                  G_CALLBACK (pipeline_error_cb),
	                  window);

	g_signal_connect (obj->bus, "message::eos",
	                  G_CALLBACK (play_eos_msg_cb),
	                  window);
	
	return obj;
}

static void
record_eos_msg_cb (GstBus * bus, GstMessage * msg, GSRWindow * window)
{
	g_return_if_fail (GSR_IS_WINDOW (window));

	GST_DEBUG ("EOS. Finished recording");

	/* FIXME: this was READY before (why?) */
	set_pipeline_state_to_null (window->priv->record->pipeline);

	g_free (window->priv->working_file);
	window->priv->working_file = g_strdup (window->priv->record_filename);

	g_free (window->priv->filename);
	window->priv->filename = g_strdup (window->priv->record_filename);

	window->priv->has_file = TRUE;
}

extern int gsr_sample_count;

static gboolean
record_start (gpointer user_data) 
{
	GSRWindow *window = GSR_WINDOW (user_data);
	gchar *name;

	g_assert (window->priv->tick_id == 0);

	window->priv->get_length_attempts = 16;
	window->priv->tick_id = g_timeout_add (200, (GSourceFunc) record_tick_callback, window);

	set_action_sensitive (window, "Stop", TRUE);
	set_action_sensitive (window, "Play", FALSE);
	set_action_sensitive (window, "Record", FALSE);
	set_action_sensitive (window, "FileSave", FALSE);
	set_action_sensitive (window, "FileSaveAs", FALSE);
	gtk_widget_set_sensitive (window->priv->scale, FALSE);

	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->status_message_cid);
	gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
			    window->priv->status_message_cid,
			    _("Recording…"));

	window->priv->record_id = 0;

	/* Translator comment: untitled here implies that
	 * there is no active sound sample. Any newly
	 * recorded samples will be saved to disk with this
	 * name as default value. */
	if (gsr_sample_count == 1) {
		name = g_strdup (_("Untitled"));
	} else {
		name = g_strdup_printf (_("Untitled-%d"), gsr_sample_count);
	}
	++gsr_sample_count;
	gtk_window_set_title (GTK_WINDOW(window), name);

	g_free (name);

	return FALSE;
}

static void
record_state_changed_cb (GstBus *bus, GstMessage *msg, GSRWindow *window)
{
	GstState  new_state;
	GMAudioProfile *profile;

	gst_message_parse_state_changed (msg, NULL, &new_state, NULL);

	g_return_if_fail (GSR_IS_WINDOW (window));

	/* we are only interested in state changes of the top-level pipeline */
	if (msg->src != GST_OBJECT (window->priv->record->pipeline))
		return;

	window->priv->record->state = new_state;

	GST_DEBUG ("record pipeline state: %s", gst_element_state_get_name (new_state));

	switch (new_state) {
	case GST_STATE_PLAYING:
		window->priv->record_id = g_idle_add (record_start, window);
		g_free (window->priv->extension);
		profile = gm_audio_profile_choose_get_active (window->priv->profile);
		window->priv->extension = g_strdup (profile ? gm_audio_profile_get_extension (profile) : NULL);
		gtk_widget_set_sensitive (window->priv->profile, FALSE);
		gtk_widget_set_sensitive (window->priv->input, FALSE);
		break;
	case GST_STATE_READY:
		gtk_adjustment_set_value (gtk_range_get_adjustment (GTK_RANGE (window->priv->scale)), 0.0);
		gtk_widget_set_sensitive (window->priv->scale, FALSE);
		gtk_widget_set_sensitive (window->priv->profile, TRUE);
		gtk_widget_set_sensitive (window->priv->input, GST_IS_MIXER (window->priv->mixer));
		/* fall through */
	case GST_STATE_PAUSED:
		set_action_sensitive (window, "Stop", FALSE);
		set_action_sensitive (window, "Play", TRUE);
		set_action_sensitive (window, "Record", TRUE);
		set_action_sensitive (window, "FileSave", TRUE);
		set_action_sensitive (window, "FileSaveAs", TRUE);
		gtk_widget_set_sensitive (window->priv->scale, FALSE);
		gtk_widget_set_sensitive (window->priv->profile, TRUE);
		gtk_widget_set_sensitive (window->priv->input, TRUE);

		gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
				   window->priv->status_message_cid);
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->status_message_cid,
				    _("Ready"));
		if (window->priv->tick_id > 0) {
			g_source_remove (window->priv->tick_id);
			window->priv->tick_id = 0;
		}
		break;
	default:
		break;
	}
}

/* create the mateconf-based source for recording.
 * store the source and the mixer in it in our window-private data
 */
static gboolean
make_record_source (GSRWindow *window)
{
	GstElement *source, *e;

	source = gst_element_factory_make ("mateconfaudiosrc", "mateconfaudiosource");
	if (source == NULL) {
		show_missing_known_element_error (NULL,
		    _("MateConf audio recording"), "mateconfaudiosrc",
		    "mateconfelements", "gst-plugins-good");
		return FALSE;
	}

	/* instantiate the underlying element so we can query it */
	/* FIXME: maybe we want to trap errors in this case ? */
        if (!gst_element_set_state (source, GST_STATE_READY)) {
		show_error_dialog (NULL, NULL,
			_("Your audio capture settings are invalid. "
			  "Please correct them with the \"Sound Preferences\" "
			  "under the System Preferences menu."));
		return FALSE;
	}
	window->priv->source = source;
	e = gst_bin_get_by_interface (GST_BIN (source), GST_TYPE_MIXER);
	window->priv->mixer = GST_MIXER (e);

	return TRUE;
}

static void
record_input_changed_cb (GtkComboBox *input, GSRWindow *window)
{
	const gchar *text;
	const GList *l;
	GstMixerTrack *t = NULL, *new = NULL;
	static GstMixerTrack *selected = NULL;

	text = gtk_combo_box_get_active_text (input);
	GST_DEBUG ("record input changed to '%s'", GST_STR_NULL (text));

	if (text == NULL)
		return;

	/* The pipeline has been destroyed already, we'll try and remember
	 * the input for the next record run in fill_record_input() */
	if (GST_IS_MIXER (window->priv->mixer) == FALSE)
		return;

	for (l = gst_mixer_list_tracks (window->priv->mixer);
	     l != NULL; l = l->next) {
		t = l->data;
		if (t == NULL || t->label == NULL)
			continue;
		if ((g_str_equal (t->label, text)) &&
		    (t->flags & GST_MIXER_TRACK_INPUT)) {
			if (new == NULL)
				new = g_object_ref (t);
		/* FIXME selected == t is equivalent to NULL == t in this case,
		 * selected, after its initialization to NULL, was never written to
		 * before this read access to it
		 * and NULL == t is equivalent to FALSE, because of the check
		 * "if (t == NULL || t->label == NULL)" above
		 */
		} else if (selected == t)
			/* re-mute old one */
			gst_mixer_set_record (window->priv->mixer,
					      selected, FALSE);
	}

	/* FIXME selected _is_ NULL always at this point - same as 5 lines above*/
	if (selected != NULL)
		g_object_unref (selected);
	if (!(selected = new))
		return;

	gst_mixer_set_record (window->priv->mixer, selected, TRUE);
	GST_DEBUG ("input changed to: %s\n", selected->label);
	mateconf_client_set_string (mateconf_client, KEY_LAST_INPUT, selected->label, NULL);
}

static void
fill_record_input (GSRWindow *window, gchar *selected)
{
	const GList *l;
	int i = 0;
	int last_possible_i = 0;
	GtkTreeModel *model;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (window->priv->input));

	if (model) 
		gtk_list_store_clear (GTK_LIST_STORE (model));

	if (GST_IS_MIXER (window->priv->mixer) == FALSE
	    || gst_mixer_list_tracks (window->priv->mixer) == NULL) {
		gtk_widget_hide (window->priv->input);
		gtk_widget_hide (window->priv->input_label);
		return;
	}

	gtk_widget_set_sensitive (window->priv->input, GST_IS_MIXER (window->priv->mixer));
	if (!GST_IS_MIXER (window->priv->mixer))
		return;

	for (l = gst_mixer_list_tracks (window->priv->mixer); l != NULL; l = l->next) {
		GstMixerTrack *t = l->data;
		if (t->label == NULL)
			continue;
		if (t->flags & GST_MIXER_TRACK_INPUT) {
			gtk_combo_box_append_text (GTK_COMBO_BOX (window->priv->input), t->label);
			++i;
		}
		if (t->flags & GST_MIXER_TRACK_RECORD) {
			if (selected == NULL) {
				gtk_combo_box_set_active (GTK_COMBO_BOX (window->priv->input), i - 1);
			} else {
				last_possible_i = i;
			}
		}
		if ((selected != NULL) && g_str_equal (selected, t->label)) {
			gtk_combo_box_set_active (GTK_COMBO_BOX (window->priv->input), i - 1);
		}
	}

	if (gtk_combo_box_get_active (GTK_COMBO_BOX (window->priv->input)) == -1) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (window->priv->input), last_possible_i - 1);
	}

	gtk_widget_show (window->priv->input);
	gtk_widget_show (window->priv->input_label);
}

static gboolean
level_message_handler_cb (GstBus * bus, GstMessage * message, GSRWindow *window)
{
  GSRWindowPrivate *priv = window->priv;

  if (message->type == GST_MESSAGE_ELEMENT) {
    const GstStructure *s = gst_message_get_structure (message);
    const gchar *name = gst_structure_get_name (s);

    if (g_str_equal (name, "level")) {
      gint channels;
      gdouble peak_dB;
      gdouble myind;
      const GValue *list;
      const GValue *value;

      gint i;
      /* we can get the number of channels as the length of any of the value
       * lists */

      list = gst_structure_get_value (s, "rms");
      channels = gst_value_list_get_size (list);

      for (i = 0; i < channels; ++i) {
	list = gst_structure_get_value (s, "peak");
        value = gst_value_list_get_value (list, i);
        peak_dB = g_value_get_double (value);
	myind = exp (peak_dB / 20);
	if (myind > 1.0)
		myind = 1.0;
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->level), myind);
      }
    }
  }
  /* we handled the message we want, and ignored the ones we didn't want.
   * so the core can unref the message for us */
  return TRUE;
}

static GSRWindowPipeline *
make_record_pipeline (GSRWindow *window)
{
	GSRWindowPipeline *pipeline;
	GMAudioProfile *profile;
	const gchar *profile_pipeline_desc;
	GstElement *encoder, *source, *filesink, *level;
	GError *err = NULL;
	gchar *pipeline_desc;
	const char *name;

	source = window->priv->source;

	/* Any reason we are not using matevfssink here? (tpm) */
	filesink = gst_element_factory_make ("filesink", "sink");
	if (filesink == NULL)
	{
		show_missing_known_element_error (NULL,
		    _("file output"), "filesink", "coreelements",
		    "gstreamer");
		gst_object_unref (source);
		return NULL;
	}

	pipeline = g_new (GSRWindowPipeline, 1);

	pipeline->pipeline = gst_pipeline_new ("record-pipeline");
	pipeline->src = source;
	pipeline->sink = filesink;

	gst_bin_add (GST_BIN (pipeline->pipeline), source);

	level = gst_element_factory_make ("level", "level");
	if (level == NULL)
	{
		show_missing_known_element_error (NULL,
		    _("level"), "level", "level",
		    "gstreamer");
		gst_object_unref (source);
		return NULL;
	}
	gst_element_set_name (level, "level");

	profile = gm_audio_profile_choose_get_active (window->priv->profile);
	if (profile == NULL)
		return NULL;
	profile_pipeline_desc = gm_audio_profile_get_pipeline (profile);
	name = gm_audio_profile_get_name (profile);

	GST_DEBUG ("encoder profile pipeline: '%s'",
		GST_STR_NULL (profile_pipeline_desc));

	pipeline_desc = g_strdup_printf ("audioconvert ! %s", profile_pipeline_desc);
	GST_DEBUG ("making encoder bin from description '%s'", pipeline_desc);
	encoder = gst_parse_bin_from_description (pipeline_desc, TRUE, &err);
	g_free (pipeline_desc);
	pipeline_desc = NULL;

	if (err) {
		show_profile_error (NULL, err->message,
			_("Could not parse the '%s' audio profile. "), name);
		g_printerr ("Failed to create GStreamer encoder plugins [%s]: %s\n",
		           profile_pipeline_desc, err->message);
		g_error_free (err);
		gst_object_unref (pipeline->pipeline);
		gst_object_unref (filesink);
		g_free (pipeline);
		return NULL;
	}

	gst_bin_add (GST_BIN (pipeline->pipeline), level);
	gst_bin_add (GST_BIN (pipeline->pipeline), encoder);
	gst_bin_add (GST_BIN (pipeline->pipeline), filesink);

	/* now link it all together */
	if (!(gst_element_link_many (source, level, encoder, NULL))) {
		show_profile_error (NULL, NULL,
			_("Could not capture using the '%s' audio profile. "),
			name);
		gst_object_unref (pipeline->pipeline);
		g_free (pipeline);
		return NULL;
	}

	if (!gst_element_link (encoder, filesink)) {
		show_profile_error (NULL, NULL,
			_("Could not write to a file using the '%s' audio profile. "),
			name);
		gst_object_unref (pipeline->pipeline);
		g_free (pipeline);
		return NULL;
	}

	/* we ultimately want to find out the caps on the encoder's source pad */
	g_signal_connect (pipeline->pipeline, "deep-notify::caps",
	                  G_CALLBACK (pipeline_deep_notify_caps_cb),
	                  window);

	pipeline->bus = gst_element_get_bus (pipeline->pipeline);

	gst_bus_add_signal_watch (pipeline->bus);

	g_signal_connect (pipeline->bus, "message::element",
	                  G_CALLBACK (level_message_handler_cb),
	                  window);

	g_signal_connect (pipeline->bus, "message::state-changed",
	                  G_CALLBACK (record_state_changed_cb),
	                  window);

	g_signal_connect (pipeline->bus, "message::error",
	                  G_CALLBACK (pipeline_error_cb),
	                  window);

	g_signal_connect (pipeline->bus, "message::eos",
	                  G_CALLBACK (record_eos_msg_cb),
	                  window);

	return pipeline;
}

static char *
calculate_format_value (GtkScale *scale,
			double value,
			GSRWindow *window)
{
	gint seconds;

	if (window->priv->record && window->priv->record->state == GST_STATE_PLAYING) {
		seconds = value;
		return seconds_to_string (seconds);
	} else {
		seconds = window->priv->len_secs * (value / 100);
		return seconds_to_string (seconds);
	}
}
	
static const GtkActionEntry menu_entries[] =
{
	/* File menu.  */
	{ "File", NULL, N_("_File") },
	{ "FileNew", GTK_STOCK_NEW, NULL, NULL,
	  N_("Create a new sample"), G_CALLBACK (file_new_cb) },
	{ "FileOpen", GTK_STOCK_OPEN, NULL, NULL,
	  N_("Open a file"), G_CALLBACK (file_open_cb) },
	{ "FileSave", GTK_STOCK_SAVE, NULL, NULL,
	  N_("Save the current file"), G_CALLBACK (file_save_cb) },
	{ "FileSaveAs", GTK_STOCK_SAVE_AS, NULL, "<shift><control>S",
	  N_("Save the current file with a different name"), G_CALLBACK (file_save_as_cb) },
	{ "RunMixer", GTK_STOCK_EXECUTE, N_("Open Volu_me Control"), NULL,
	  N_("Open the audio mixer"), G_CALLBACK (run_mixer_cb) },
	{ "FileProperties", GTK_STOCK_PROPERTIES, NULL, "<control>I",
	  N_("Show information about the current file"), G_CALLBACK (file_properties_cb) },
	{ "FileClose", GTK_STOCK_CLOSE, NULL, NULL,
	  N_("Close the current file"), G_CALLBACK (file_close_cb) },
	{ "Quit", GTK_STOCK_QUIT, NULL, NULL,
	  N_("Quit the program"), G_CALLBACK (quit_cb) },

	/* Control menu */
	{ "Control", NULL, N_("_Control") },
	{ "Record", GTK_STOCK_MEDIA_RECORD, NULL, "<control>R",
	  N_("Record sound"), G_CALLBACK (record_cb) },
	{ "Play", GTK_STOCK_MEDIA_PLAY, NULL, "<control>P",
	  N_("Play sound"), G_CALLBACK (play_cb) },
	{ "Stop", GTK_STOCK_MEDIA_STOP, NULL, "<control>X",
	  N_("Stop sound"), G_CALLBACK (stop_cb) },

	/* Help menu */
	{ "Help", NULL, N_("_Help") },
	{"HelpContents", GTK_STOCK_HELP, N_("Contents"), "F1",
	 N_("Open the manual"), G_CALLBACK (help_contents_cb) },
	{ "About", GTK_STOCK_ABOUT, NULL, NULL,
	 N_("About this application"), G_CALLBACK (about_cb) }
};

static void
menu_item_select_cb (GtkMenuItem *proxy,
                     GSRWindow *window)
{
	GtkAction *action;
	char *message;

	action = g_object_get_data (G_OBJECT (proxy),  "gtk-action");
	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
	if (message) {
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->tip_message_cid, message);
		g_free (message);
	}
}

static void
menu_item_deselect_cb (GtkMenuItem *proxy,
                       GSRWindow *window)
{
	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->tip_message_cid);
}
 
static void
connect_proxy_cb (GtkUIManager *manager,
                  GtkAction *action,
                  GtkWidget *proxy,
                  GSRWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), window);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
disconnect_proxy_cb (GtkUIManager *manager,
                     GtkAction *action,
                     GtkWidget *proxy,
                     GSRWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), window);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), window);
	}
}

/* find the given filename in the uninstalled or installed ui dir */
static gchar *
find_ui_file (const gchar * filename)
{
	gchar * path;

	path = g_build_filename (GSR_UIDIR_UNINSTALLED, filename, NULL);
	if (g_file_test (path, G_FILE_TEST_EXISTS))
		return path;

	g_free (path);
	path = g_build_filename (GSR_UIDIR, filename, NULL);
	if (g_file_test (path, G_FILE_TEST_EXISTS))
		return path;

	g_free (path);
	return NULL;
}

static void
gsr_window_init (GSRWindow *window)
{
	GSRWindowPrivate *priv;
	GError *error = NULL;
	GtkWidget *main_vbox;
	GtkWidget *menubar;
	GtkWidget *file_menu;
	GtkWidget *submenu;
	GtkWidget *rec_menu;
	GtkWidget *toolbar;
	GtkWidget *content_vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *table;
	GtkWidget *align;
	GtkWidget *frame;
	gchar *id;
	gchar *last_input;
	gchar *path;
	GtkAction *action;
	GtkShadowType shadow_type;
	window->priv = GSR_WINDOW_GET_PRIVATE (window);
	priv = window->priv;

	/* treat mateconf client as a singleton */
	if (mateconf_client == NULL)
		mateconf_client = mateconf_client_get_default ();

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);
	priv->main_vbox = main_vbox;
	gtk_widget_show (main_vbox);

	/* menu & toolbar */
	priv->ui_manager = gtk_ui_manager_new ();

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (priv->ui_manager));

	path = find_ui_file ("ui.xml");
	gtk_ui_manager_add_ui_from_file (priv->ui_manager, path, &error);

	if (error != NULL)
	{
		show_error_dialog (GTK_WINDOW (window), error->message,
			_("Could not load UI file. The program may not be properly installed."));
		g_error_free (error);
		exit (1);
	}
	g_free (path);

	/* show tooltips in the statusbar */
	g_signal_connect (priv->ui_manager, "connect_proxy",
			  G_CALLBACK (connect_proxy_cb), window);
	g_signal_connect (priv->ui_manager, "disconnect_proxy",
			 G_CALLBACK (disconnect_proxy_cb), window);

	priv->action_group = gtk_action_group_new ("GSRWindowActions");
	gtk_action_group_set_translation_domain (priv->action_group, NULL);
	gtk_action_group_add_actions (priv->action_group,
				      menu_entries,
				      G_N_ELEMENTS (menu_entries),
				      window);

	gtk_ui_manager_insert_action_group (priv->ui_manager, priv->action_group, 0);

	/* set short labels to use in the toolbar */
	action = gtk_action_group_get_action (priv->action_group, "FileOpen");
	g_object_set (action, "short_label", _("Open"), NULL);
	action = gtk_action_group_get_action (priv->action_group, "FileSave");
	g_object_set (action, "short_label", _("Save"), NULL);
	action = gtk_action_group_get_action (priv->action_group, "FileSaveAs");
	g_object_set (action, "short_label", _("Save As"), NULL);

	set_action_sensitive (window, "FileSave", FALSE);
	set_action_sensitive (window, "FileSaveAs", FALSE);
	set_action_sensitive (window, "Play", FALSE);
	set_action_sensitive (window, "Stop", FALSE);

	menubar = gtk_ui_manager_get_widget (priv->ui_manager, "/MenuBar");
	gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, FALSE, 0);
	gtk_widget_show (menubar);

	toolbar = gtk_ui_manager_get_widget (priv->ui_manager, "/ToolBar");
	gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar), FALSE);
	gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);
	gtk_widget_show (toolbar);

	/* recent files */
	file_menu = gtk_ui_manager_get_widget (priv->ui_manager,
					      "/MenuBar/FileMenu");
	submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (file_menu));
	rec_menu = gtk_ui_manager_get_widget (priv->ui_manager,
					      "/MenuBar/FileMenu/FileRecentMenu");
	priv->recent_view = gtk_recent_chooser_menu_new ();
	gtk_recent_chooser_set_local_only (GTK_RECENT_CHOOSER (priv->recent_view), TRUE);
	gtk_recent_chooser_set_limit (GTK_RECENT_CHOOSER (priv->recent_view), 5);
	priv->recent_filter = gtk_recent_filter_new ();
	gtk_recent_filter_add_application (priv->recent_filter, g_get_application_name ());
	gtk_recent_chooser_set_filter (GTK_RECENT_CHOOSER (priv->recent_view), priv->recent_filter);
	g_signal_connect (priv->recent_view, "item-activated",
			  G_CALLBACK (file_open_recent_cb), window);

	/* window content: hscale, labels, etc */
	content_vbox = gtk_vbox_new (FALSE, 7);
	gtk_container_set_border_width (GTK_CONTAINER (content_vbox), 6);
	gtk_box_pack_start (GTK_BOX (main_vbox), content_vbox, TRUE, TRUE, 0);
	gtk_widget_show (content_vbox);

	priv->scale = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 100, 1, 1, 0)));
	priv->seek_in_progress = FALSE;
	g_signal_connect (priv->scale, "format-value",
			  G_CALLBACK (calculate_format_value), window);
	g_signal_connect (priv->scale, "button-press-event",
			  G_CALLBACK (seek_started), window);
	g_signal_connect (priv->scale, "button-release-event",
			  G_CALLBACK (seek_to), window);

	gtk_scale_set_value_pos (GTK_SCALE (window->priv->scale), GTK_POS_BOTTOM);
	/* We can't seek until we find out the length */
	gtk_widget_set_sensitive (window->priv->scale, FALSE);
	gtk_box_pack_start (GTK_BOX (content_vbox), priv->scale, FALSE, FALSE, 6);
	gtk_widget_show (window->priv->scale);

        /* create source and choose mixer input */
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (content_vbox), hbox, FALSE, FALSE, 0);

	priv->input_label = gtk_label_new_with_mnemonic (_("Record from _input:"));
	gtk_misc_set_alignment (GTK_MISC (priv->input_label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), priv->input_label, FALSE, FALSE, 0);

	priv->input = gtk_combo_box_new_text ();
	gtk_label_set_mnemonic_widget (GTK_LABEL (priv->input_label), priv->input);
	gtk_box_pack_start (GTK_BOX (hbox), priv->input, TRUE, TRUE, 0);

	if (!make_record_source (window))
		exit (1);

	g_signal_connect (priv->input, "changed",
			  G_CALLBACK (record_input_changed_cb), window);

	/* choose profile */
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (content_vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("_Record as:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	priv->profile = gm_audio_profile_choose_new ();
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->profile);
	gtk_box_pack_start (GTK_BOX (hbox), window->priv->profile, TRUE, TRUE, 0);
	gtk_widget_show (window->priv->profile);

	atk_object_add_relationship (gtk_widget_get_accessible (GTK_WIDGET (priv->profile)),
				ATK_RELATION_LABELLED_BY,
				gtk_widget_get_accessible (GTK_WIDGET (label)));

	id = mateconf_client_get_string (mateconf_client, KEY_LAST_PROFILE_ID, NULL);
	if (id) {
		gm_audio_profile_choose_set_active (window->priv->profile, id);
		g_free (id);
	}

        g_signal_connect (priv->profile, "changed",
                          G_CALLBACK (profile_changed_cb), window);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (content_vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new ("    "); /* FIXME: better padding? */
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	table = gtk_table_new (3, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

	label = make_title_label (_("File Information"));

	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 2, 0, 1,
			  GTK_FILL, 0, 0, 0);

	label = gtk_label_new (_("Filename:"));

	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 1, 2,
			  GTK_FILL, 0, 0, 0);

	priv->name_label = gtk_label_new (_("<none>"));
	gtk_label_set_selectable (GTK_LABEL (priv->name_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (priv->name_label), GTK_WRAP_WORD);
	gtk_misc_set_alignment (GTK_MISC (priv->name_label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (table), priv->name_label,
			  1, 2, 1, 2,
			  GTK_FILL  | GTK_EXPAND, 0,
			  0, 0);

	atk_object_add_relationship (gtk_widget_get_accessible (GTK_WIDGET (priv->name_label)),
				ATK_RELATION_LABELLED_BY,
				gtk_widget_get_accessible (GTK_WIDGET (label)));


	label = gtk_label_new (_("Length:"));

	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 2, 3,
			  GTK_FILL, 0, 0, 0);

	priv->length_label = gtk_label_new ("");
	gtk_label_set_selectable (GTK_LABEL (priv->length_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (priv->length_label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (table), priv->length_label,
			  1, 2, 2, 3,
			  GTK_FILL | GTK_EXPAND, 0,
			  0, 0);

	atk_object_add_relationship (gtk_widget_get_accessible (GTK_WIDGET (priv->length_label)),
				ATK_RELATION_LABELLED_BY,
				gtk_widget_get_accessible (GTK_WIDGET (label)));

	/* statusbar */
	priv->statusbar = gtk_statusbar_new ();
	gtk_widget_set_can_focus (priv->statusbar, TRUE);
	gtk_box_pack_end (GTK_BOX (main_vbox), priv->statusbar, FALSE, FALSE, 0);
	gtk_widget_show (priv->statusbar);

	/* hack to get the same shadow as the status bar.. */
	gtk_widget_style_get (GTK_WIDGET (priv->statusbar), "shadow-type", &shadow_type, NULL);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), shadow_type);
	gtk_widget_show (frame);

	gtk_box_pack_end (GTK_BOX (priv->statusbar), frame, FALSE, TRUE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_box_set_spacing (GTK_BOX (hbox), 6);

	priv->volume_label = gtk_label_new (_("Level:"));
	gtk_box_pack_start (GTK_BOX (hbox), priv->volume_label, FALSE, TRUE, 0);

	/* initialize priv->level */
	align = gtk_aspect_frame_new ("", 0.0, 0.0, 20, FALSE);
	gtk_frame_set_shadow_type (GTK_FRAME (align), GTK_SHADOW_NONE);
	gtk_widget_show (align);
	gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, FALSE, 0);

	priv->level = gtk_progress_bar_new ();
	gtk_container_add (GTK_CONTAINER (align), priv->level);

	gtk_widget_set_sensitive (window->priv->volume_label, FALSE);
	gtk_widget_set_sensitive (window->priv->level, FALSE);

	priv->status_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (priv->statusbar), "status_message");
	priv->tip_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (priv->statusbar), "tip_message");

	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
			    priv->status_message_cid,
			    _("Ready"));

	gtk_widget_show_all (main_vbox);
	last_input = mateconf_client_get_string (mateconf_client, KEY_LAST_INPUT, NULL);
	fill_record_input (window, last_input);
	if (last_input) {
		g_free (last_input);
	}

	/* Make the pipelines */
	priv->play = NULL;
	priv->record = NULL; 

	priv->len_secs = 0;
	priv->get_length_attempts = 16;
	priv->dirty = TRUE;
}

static void
gsr_window_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	GSRWindow *window;
	GSRWindowPrivate *priv;
	struct stat buf;
	char *title, *short_name;
	char *utf8_name = NULL;
	const char *ext;

	window = GSR_WINDOW (object);
	priv = window->priv;

	switch (prop_id) {
	case PROP_LOCATION:
		if (priv->filename != NULL) {
			if (g_value_get_string (value) == NULL)
				return;
			if (g_str_equal (g_value_get_string (value), priv->filename)) {
				return;
			}
		}

		g_free (priv->filename);
		g_free (priv->working_file);

		priv->filename = g_value_dup_string (value);
		priv->working_file = g_strdup (priv->filename);
		priv->len_secs = 0;

		short_name = g_path_get_basename (priv->filename);
		if (stat (priv->filename, &buf) == 0) {
			window->priv->has_file = TRUE;
		} else {
			window->priv->has_file = FALSE;
		}

		g_free (window->priv->extension);
		if ((ext = strrchr (short_name, '.')) && ext[1] != '\0')
			window->priv->extension = g_strdup (&ext[1]);
		else
			window->priv->extension = NULL;

		utf8_name = g_filename_to_utf8 (short_name, -1, NULL, NULL, NULL);
		if (priv->name_label != NULL) {
			gtk_label_set_text (GTK_LABEL (priv->name_label),
					    utf8_name);
		}

		gsr_add_recent (priv->filename);

		/*Translators: this is the window title, %s is the currently open file's name or Untitled*/
		title = g_strdup_printf (_("%s — Sound Recorder"), utf8_name);
		gtk_window_set_title (GTK_WINDOW (window), title);
		g_free (title);
		g_free (utf8_name);
		g_free (short_name);

		set_action_sensitive (window, "Play", window->priv->has_file ? TRUE : FALSE);
		set_action_sensitive (window, "Stop", FALSE);
		set_action_sensitive (window, "Record", TRUE);
		set_action_sensitive (window, "FileSave", window->priv->has_file ? TRUE : FALSE);
		set_action_sensitive (window, "FileSaveAs", TRUE);
		break;
	default:
		break;
	}
}

static void
gsr_window_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_LOCATION:
		g_value_set_string (value, GSR_WINDOW (object)->priv->filename);
		break;

	default:
		break;
	}
}

static void
gsr_window_finalize (GObject *object)
{
	GSRWindow *window;
	GSRWindowPrivate *priv;

	window = GSR_WINDOW (object);
	priv = window->priv;

	GST_DEBUG ("finalizing ...");

	if (priv == NULL) {
		return;
	}

	if (priv->ui_manager) {
		g_object_unref (priv->ui_manager);
		priv->ui_manager = NULL;
	}

	if (priv->action_group) {
		g_object_unref (priv->action_group);
		priv->action_group = NULL;
	}

	if (priv->tick_id > 0) { 
		g_source_remove (priv->tick_id);
		window->priv->play->tick_id = 0;
	}

	if (priv->record_id > 0) {
		g_source_remove (priv->record_id);
	}

	if (priv->ebusy_timeout_id > 0) {
		g_source_remove (window->priv->ebusy_timeout_id);
	}

	g_idle_remove_by_data (window);

	if (priv->play != NULL) {
		shutdown_pipeline (priv->play);
		g_free (priv->play);
	}

	if (priv->record != NULL) {
		shutdown_pipeline (priv->record);
		g_free (priv->record);
	}

	unlink (priv->record_filename);
	g_free (priv->record_filename);

	g_free (priv->working_file);
	g_free (priv->filename);

	G_OBJECT_CLASS (parent_class)->finalize (object);

	window->priv = NULL;
}

static void
gsr_window_class_init (GSRWindowClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gsr_window_finalize;
	object_class->set_property = gsr_window_set_property;
	object_class->get_property = gsr_window_get_property;

	parent_class = g_type_class_peek_parent (klass);

	g_object_class_install_property (object_class,
					 PROP_LOCATION,
					 g_param_spec_string ("location",
							      "Location",
							      "",
	/* Translator comment: default trackname is 'untitled', which
	 * has as effect that the user cannot save to this file. The
	 * 'save' action will open the save-as dialog instead to give
	 * a proper filename. See mate-record.c:94. */
							      _("Untitled"),
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GSRWindowPrivate));

	GST_DEBUG_CATEGORY_INIT (gsr_debug, "gsr", 0, "Mate Sound Recorder");
}

GType
gsr_window_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		GTypeInfo info = {
			sizeof (GSRWindowClass),
			NULL, NULL,
			(GClassInitFunc) gsr_window_class_init,
			NULL, NULL,
			sizeof (GSRWindow), 0,
			(GInstanceInitFunc) gsr_window_init
		};

		type = g_type_register_static (GTK_TYPE_WINDOW,
					       "GSRWindow",
					       &info, 0);
	}

	return type;
}

GtkWidget *
gsr_window_new (const char *filename)
{
	GSRWindow *window;
	char *template;

	/* filename has been changed to be without extension */
	window = g_object_new (GSR_TYPE_WINDOW, 
			       "location", filename,
			       NULL);
        /* FIXME: check extension too */
	window->priv->filename = g_strdup (filename);
	if (g_file_test (filename, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) != FALSE) {
		window->priv->has_file = TRUE;
		window->priv->dirty = FALSE;
	} else {
		window->priv->has_file = FALSE;
	}

	template = g_strdup_printf ("gsr-record-%s-%d.XXXXXX", filename, getpid ());
	window->priv->record_fd = g_file_open_tmp (template, &window->priv->record_filename, NULL);
	g_free (template);
	close (window->priv->record_fd);

	if (window->priv->has_file == FALSE) {
		g_free (window->priv->working_file);
		window->priv->working_file = g_strdup (window->priv->record_filename);
	} else {
		g_free (window->priv->working_file);
		window->priv->working_file = g_strdup (filename);
	}

	window->priv->saved = TRUE;

	gtk_window_set_default_size (GTK_WINDOW (window), 512, 200);

	return GTK_WIDGET (window);
}
