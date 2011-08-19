/* -*- mode: c; style: linux -*- */
/* -*- c-basic-offset: 2 -*- */

/* pipeline-tests.c
 * Copyright (C) 2002 Jan Schmidt
 * Copyright (C) 2005 Tim-Philipp Müller <tim centricular net>
 *
 * Written by: Jan Schmidt <thaytan@mad.scientist.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <locale.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gst/gst.h>

#include "pipeline-tests.h"
#define WID(s) gtk_builder_get_object (builder, s)
static guint timeout_tag;

static GstElement *gst_test_pipeline;

static void pipeline_error_dlg (GtkWindow * parent,
    GSTPPipelineDescription * pipeline_desc, const gchar * error_message);

/* User responded in the dialog */
static void
user_test_pipeline_response (GtkDialog * widget, gint response_id,
    GtkBuilder * dialog)
{
  /* Close the window causing the test to end */
  gtk_widget_hide (GTK_WIDGET (widget));
}

/* Timer timeout has been occurred */
static gint
user_test_pipeline_timeout (gpointer data)
{
  gtk_progress_bar_pulse (GTK_PROGRESS_BAR (data));
  return TRUE;
}

gchar *
gst_pipeline_string_from_desc (GSTPPipelineDescription * pipeline_desc)
{
	gchar *pipeline = NULL;
  if (pipeline_desc->device != NULL && *pipeline_desc->device != '\0') {
	  pipeline = g_strdup_printf ("%s device=\"%s\"", pipeline_desc->pipeline,
                                pipeline_desc->device);
  }
  else
    pipeline = pipeline_desc->pipeline;

  return pipeline;
}

gchar *
gst_pipeline_string_get_property_value (const gchar *pipeline_str, const gchar *propertyname)
{
  gchar **pipeline_nodes = NULL;
  gchar *node = NULL;
  gchar *node_value = NULL;
  gchar **node_split = NULL;
  gint i = 0;

  g_assert (pipeline_str != NULL);

  pipeline_nodes = g_strsplit (pipeline_str, " ", -1);

  while ((node = pipeline_nodes[i++])) {
    /* Split into key = value pair */
    node_split = g_strsplit_set (node, "=", -1);
    if (node_split != NULL && node_split[1] != NULL) {
      if (!strcmp (node_split[0], propertyname)) {
        node_value = g_shell_unquote(node_split[1], NULL);
      }
    }
    g_strfreev (node_split);

    if (node_value != NULL) break;
  }

  g_strfreev (pipeline_nodes);

  return node_value;
}

/* Build the pipeline */
static gboolean
build_test_pipeline (GSTPPipelineDescription * pipeline_desc, GError ** p_err)
{
  const gchar *in_between = NULL;
  gboolean return_val = FALSE;
  gchar *test_pipeline_str = NULL;
  gchar *full_pipeline_str = NULL;

  g_assert (p_err != NULL);

  switch (pipeline_desc->test_type) {
    case TEST_PIPE_AUDIOSINK:
      test_pipeline_str = gst_properties_mateconf_get_string ("default/audiosink");
      break;
    case TEST_PIPE_VIDEOSINK:
      test_pipeline_str = gst_properties_mateconf_get_string ("default/videosink");
      break;
    case TEST_PIPE_SUPPLIED:
      test_pipeline_str = g_strdup (pipeline_desc->test_pipe);
      break;
  }

  switch (pipeline_desc->type) {
    case PIPE_TYPE_AUDIOSINK:
    case PIPE_TYPE_AUDIOSRC:
      in_between = "audioconvert ! audioresample";
      break;
    default:
      in_between = "ffmpegcolorspace";
      break;
  }

  switch (pipeline_desc->type) {
    case PIPE_TYPE_AUDIOSINK:
    case PIPE_TYPE_VIDEOSINK:
      full_pipeline_str = g_strdup_printf ("%s ! %s ! %s",
          test_pipeline_str, in_between, gst_pipeline_string_from_desc(pipeline_desc));
      break;
    case PIPE_TYPE_AUDIOSRC:
    case PIPE_TYPE_VIDEOSRC:
      full_pipeline_str = g_strdup_printf ("%s ! %s ! %s",
          gst_pipeline_string_from_desc(pipeline_desc), in_between, test_pipeline_str);
      break;
  }

  if (full_pipeline_str) {
    gst_test_pipeline = gst_parse_launch (full_pipeline_str, p_err);

    if (*p_err == NULL && gst_test_pipeline != NULL)
      return_val = TRUE;
  }

  g_free (test_pipeline_str);
  g_free (full_pipeline_str);

  return return_val;
}

static void
pipeline_error_dlg (GtkWindow * parent,
    GSTPPipelineDescription * pipeline_desc, const gchar * error_message)
{
  gchar *errstr;

  if (error_message) {
    errstr = g_strdup_printf ("%s: %s", pipeline_desc->name, error_message);
  } else {
    errstr = g_strdup_printf (_("Failed to construct test pipeline for '%s'"),
        pipeline_desc->name);
  }

  if (parent == NULL) {
    g_printerr ("%s", errstr);
  } else {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", errstr);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }

  g_free (errstr);
}

/* Construct and run the pipeline. Use the indicated parent
 * for any user interaction window.
 */
void
user_test_pipeline (GtkBuilder * builder,
    GtkWindow * parent, GSTPPipelineDescription * pipeline_desc)
{
  GstStateChangeReturn ret;
  GtkDialog *dialog = NULL;
  GstMessage *msg;
  GError *err = NULL;
  GstBus *bus;

  gst_test_pipeline = NULL;

  /* Build the pipeline */
  if (!build_test_pipeline (pipeline_desc, &err)) {
    /* Show the error pipeline */
    pipeline_error_dlg (parent, pipeline_desc, (err) ? err->message : NULL);
    if (err)
      g_error_free (err);
    return;
  }

  /* Setup the 'click ok when done' dialog */
  if (parent) {
    dialog = GTK_DIALOG (WID ("test_pipeline"));
    /* g_return_if_fail(dialog != NULL); */
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
    g_signal_connect (G_OBJECT (dialog), "response",
        (GCallback) user_test_pipeline_response, builder);
  }

  /* Start the pipeline and wait for max. 3 seconds for it to start up */
  gst_element_set_state (gst_test_pipeline, GST_STATE_PLAYING);
  ret = gst_element_get_state (gst_test_pipeline, NULL, NULL, 3 * GST_SECOND);

  /* Check if any error messages were posted on the bus */
  bus = gst_element_get_bus (gst_test_pipeline);
  msg = gst_bus_poll (bus, GST_MESSAGE_ERROR, 0);
  gst_object_unref (bus);

  if (msg != NULL) {
    gchar *dbg = NULL;

    gst_message_parse_error (msg, &err, &dbg);
    gst_message_unref (msg);

    g_message ("Error running pipeline '%s': %s [%s]", pipeline_desc->name,
        (err) ? err->message : "(null error)",
        (dbg) ? dbg : "no additional debugging details");
    pipeline_error_dlg (parent, pipeline_desc, err->message);
    g_error_free (err);
    g_free (dbg);
  } else if (ret != GST_STATE_CHANGE_SUCCESS && ret != GST_STATE_CHANGE_ASYNC) {
    pipeline_error_dlg (parent, pipeline_desc, NULL);
  } else {
    /* Show the dialog */
    if (dialog) {
      gtk_window_present (GTK_WINDOW (dialog));
      timeout_tag =
          g_timeout_add (50, user_test_pipeline_timeout,
          WID ("test_pipeline_progress"));
      gtk_dialog_run (GTK_DIALOG (dialog));
      g_source_remove (timeout_tag);
      gtk_widget_hide (GTK_WIDGET (dialog));
    } else {
      gint secs;

      /* A bit hacky: No parent dialog, run in limited test mode */
      for (secs = 0; secs < 5; ++secs) {
        g_print (".");
        g_usleep (G_USEC_PER_SEC);      /* 1 second */
      }
    }
  }

  if (gst_test_pipeline) {
    gst_element_set_state (gst_test_pipeline, GST_STATE_NULL);
    gst_object_unref (gst_test_pipeline);
    gst_test_pipeline = NULL;
  }
}
