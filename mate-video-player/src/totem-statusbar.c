/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * TotemStatusbar Copyright (C) 1998 Shawn T. Amundson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301  USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "config.h"

#include <glib/gi18n.h>

#include <gtk/gtk.h>

#include "totem-statusbar.h"
#include "video-utils.h"

#define SPACING 4
#define NORMAL_CONTEXT "text"
#define BUFFERING_CONTEXT "buffering"
#define HELP_CONTEXT "help"

static void totem_statusbar_dispose          (GObject             *object);
static void totem_statusbar_sync_description (TotemStatusbar      *statusbar);

G_DEFINE_TYPE(TotemStatusbar, totem_statusbar, GTK_TYPE_STATUSBAR)

static void
totem_statusbar_class_init (TotemStatusbarClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = totem_statusbar_dispose;
}

static void
totem_statusbar_init (TotemStatusbar *statusbar)
{
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);
  GtkWidget *packer, *hbox, *vbox, *label;
  GList *children_list;

  statusbar->time = 0;
  statusbar->length = -1;

  hbox = gtk_statusbar_get_message_area (gstatusbar);
  children_list = gtk_container_get_children (GTK_CONTAINER (hbox));
  label = children_list->data;

  gtk_box_set_child_packing (GTK_BOX (hbox), label,
			     FALSE, FALSE, 0, GTK_PACK_START);
  gtk_label_set_ellipsize (GTK_LABEL (label), FALSE);

  /* progressbar for network streams */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, TRUE, 0);
  gtk_widget_show (vbox);

  statusbar->progress = gtk_progress_bar_new ();
  gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (statusbar->progress),
				    gtk_widget_get_direction (statusbar->progress) == GTK_TEXT_DIR_LTR ?
				    GTK_PROGRESS_LEFT_TO_RIGHT : GTK_PROGRESS_RIGHT_TO_LEFT);
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (statusbar->progress), 0.);
  gtk_box_pack_start (GTK_BOX (vbox), statusbar->progress, TRUE, TRUE, 1);
  gtk_widget_set_size_request (statusbar->progress, 150, 10);
  //gtk_widget_hide (statusbar->progress);

  packer = gtk_vseparator_new ();
  gtk_box_pack_start (GTK_BOX (hbox), packer, FALSE, FALSE, 0);
  gtk_widget_show (packer);

  statusbar->time_label = gtk_label_new (_("0:00 / 0:00"));
  gtk_misc_set_alignment (GTK_MISC (statusbar->time_label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox), statusbar->time_label, FALSE, FALSE, 0);
  gtk_widget_show (statusbar->time_label);

  totem_statusbar_set_text (statusbar, _("Stopped"));
}

GtkWidget* 
totem_statusbar_new (void)
{
  return g_object_new (TOTEM_TYPE_STATUSBAR, NULL);
}

static void
totem_statusbar_update_time (TotemStatusbar *statusbar)
{
  char *time_string, *length, *label;

  time_string = totem_time_to_string (statusbar->time * 1000);

  if (statusbar->length < 0) {
    label = g_strdup_printf (_("%s (Streaming)"), time_string);
  } else {
    length = totem_time_to_string
	    (statusbar->length == -1 ? 0 : statusbar->length * 1000);

    if (statusbar->seeking == FALSE)
      /* Elapsed / Total Length */
      label = g_strdup_printf (_("%s / %s"), time_string, length);
    else
      /* Seeking to Time / Total Length */
      label = g_strdup_printf (_("Seek to %s / %s"), time_string, length);

    g_free (length);
  }
  g_free (time_string);

  gtk_label_set_text (GTK_LABEL (statusbar->time_label), label);
  g_free (label);

  totem_statusbar_sync_description (statusbar);
}

void
totem_statusbar_set_text (TotemStatusbar *statusbar, const char *label)
{
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);
  guint id;

  id = gtk_statusbar_get_context_id (gstatusbar, NORMAL_CONTEXT);
  gtk_statusbar_pop (gstatusbar, id);
  gtk_statusbar_push (gstatusbar, id, label);

  totem_statusbar_sync_description (statusbar);
}

void
totem_statusbar_set_time (TotemStatusbar *statusbar, gint _time)
{
  g_return_if_fail (TOTEM_IS_STATUSBAR (statusbar));

  if (statusbar->time == _time)
    return;

  statusbar->time = _time;
  totem_statusbar_update_time (statusbar);
}

/* Set a help message to be displayed in the status bar. */
void
totem_statusbar_push_help (TotemStatusbar *statusbar, const char *message)
{
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);
  guint id;

  id = gtk_statusbar_get_context_id (gstatusbar, HELP_CONTEXT);
  gtk_statusbar_push (gstatusbar, id, message);
}

/* Remove the last help message of the status bar. */
void
totem_statusbar_pop_help (TotemStatusbar *statusbar)
{
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);
  guint id;

  id = gtk_statusbar_get_context_id (gstatusbar, HELP_CONTEXT);
  gtk_statusbar_pop (gstatusbar, id);
}

static gboolean
totem_statusbar_timeout_pop (TotemStatusbar *statusbar)
{
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);

  if (--statusbar->timeout_ticks > 0)
    return TRUE;

  statusbar->pushed = FALSE;

  gtk_statusbar_pop (gstatusbar,
                     gtk_statusbar_get_context_id (gstatusbar, BUFFERING_CONTEXT));

  gtk_widget_hide (statusbar->progress);

  totem_statusbar_sync_description (statusbar);

  statusbar->percentage = 101;

  statusbar->timeout = 0;

  return FALSE;
}

void
totem_statusbar_push (TotemStatusbar *statusbar, guint percentage)
{
  GtkStatusbar *gstatusbar = GTK_STATUSBAR (statusbar);
  char *label;
  gboolean need_update = FALSE;

  if (statusbar->pushed == FALSE)
  {
    gtk_statusbar_push (gstatusbar,
                        gtk_statusbar_get_context_id (gstatusbar, BUFFERING_CONTEXT),
                        _("Buffering"));
    statusbar->pushed = TRUE;

    need_update = TRUE;
  }

  if (statusbar->percentage != percentage)
  {
    statusbar->percentage = percentage;

    /* eg: 75 % */
    label = g_strdup_printf (_("%d %%"), percentage);
    gtk_progress_bar_set_text (GTK_PROGRESS_BAR (statusbar->progress), label);
    g_free (label);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (statusbar->progress),
                                  percentage / 100.);
    gtk_widget_show (statusbar->progress);

    need_update = TRUE;
  }

  statusbar->timeout_ticks = 3;

  if (statusbar->timeout == 0)
  {
    statusbar->timeout = g_timeout_add_seconds (1, (GSourceFunc) totem_statusbar_timeout_pop, statusbar);
  }

  if (need_update)
    totem_statusbar_sync_description (statusbar);
}

void
totem_statusbar_pop (TotemStatusbar *statusbar)
{
  if (statusbar->pushed != FALSE)
  {
    g_source_remove (statusbar->timeout);
    totem_statusbar_timeout_pop (statusbar);
  }
}

void
totem_statusbar_set_time_and_length (TotemStatusbar *statusbar,
				     gint _time, gint length)
{
  g_return_if_fail (TOTEM_IS_STATUSBAR (statusbar));

  if (_time != statusbar->time ||
      length != statusbar->length) {
    statusbar->time = _time;
    statusbar->length = length;

    totem_statusbar_update_time (statusbar);
  }
}

void
totem_statusbar_set_seeking (TotemStatusbar *statusbar,
			     gboolean seeking)
{
  g_return_if_fail (TOTEM_IS_STATUSBAR (statusbar));

  if (statusbar->seeking == seeking)
    return;

  statusbar->seeking = seeking;

  totem_statusbar_update_time (statusbar);
}

static void
totem_statusbar_sync_description (TotemStatusbar *statusbar)
{
  GtkWidget *message_area, *label;
  AtkObject *obj;
  GList *children_list;
  char *text;

  message_area = gtk_statusbar_get_message_area (GTK_STATUSBAR (statusbar));
  children_list = gtk_container_get_children (GTK_CONTAINER (message_area));
  label = children_list->data;

  obj = gtk_widget_get_accessible (GTK_WIDGET (statusbar));
  if (statusbar->pushed == FALSE) {
    /* eg: Paused, 0:32 / 1:05 */
    text = g_strdup_printf (_("%s, %s"),
	gtk_label_get_text (GTK_LABEL (label)),
	gtk_label_get_text (GTK_LABEL (statusbar->time_label)));
  } else {
    /* eg: Buffering, 75 % */
    text = g_strdup_printf (_("%s, %d %%"),
	gtk_label_get_text (GTK_LABEL (label)),
	statusbar->percentage);
  }

  atk_object_set_name (obj, text);
  g_free (text);
}

static void
totem_statusbar_dispose (GObject *object)
{
  TotemStatusbar *statusbar = TOTEM_STATUSBAR (object);

  if (statusbar->timeout != 0) {
    g_source_remove (statusbar->timeout);
    statusbar->timeout = 0;
  }

  G_OBJECT_CLASS (totem_statusbar_parent_class)->dispose (object);
}

/*
 * vim: sw=2 ts=8 cindent noai bs=2
 */
