/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2007 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *
 * The Totem project hereby grant permission for non-gpl compatible GStreamer
 * plugins to be used and distributed together with GStreamer and Totem. This
 * permission are above and beyond the permissions granted by the GPL license
 * Totem is covered by.
 *
 * Monday 7th February 2005: Christian Schaller: Add excemption clause.
 * See license_change file for details.
 *
 * Author: Bastien Nocera <hadess@hadess.net>, Philip Withnall <philip@tecnocode.co.uk>
 */

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

#include "totem-skipto.h"
#include "totem-skipto-plugin.h"
#include "totem-uri.h"
#include "video-utils.h"
#include "bacon-video-widget.h"

static void totem_skipto_dispose	(GObject *object);

/* Callback functions for GtkBuilder */
G_MODULE_EXPORT void time_entry_activate_cb (GtkEntry *entry, TotemSkipto *skipto);
G_MODULE_EXPORT void tstw_adjustment_value_changed_cb (GtkAdjustment *adjustment, TotemSkipto *skipto);

struct TotemSkiptoPrivate {
	GtkBuilder *xml;
	GtkWidget *time_entry;
	GtkLabel *seconds_label;
	gint64 time;
	Totem *totem;
};

TOTEM_PLUGIN_DEFINE_TYPE (TotemSkipto, totem_skipto, GTK_TYPE_DIALOG)
#define TOTEM_SKIPTO_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TOTEM_TYPE_SKIPTO, TotemSkiptoPrivate))

static void
totem_skipto_class_init (TotemSkiptoClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (TotemSkiptoPrivate));

	object_class->dispose = totem_skipto_dispose;
}

static void
totem_skipto_response_cb (GtkDialog *dialog, gint response_id, gpointer data)
{
	TotemSkipto *skipto;

	skipto = TOTEM_SKIPTO (dialog);
	gtk_spin_button_update (GTK_SPIN_BUTTON (skipto->priv->time_entry));
}

static void
totem_skipto_init (TotemSkipto *skipto)
{
	skipto->priv = G_TYPE_INSTANCE_GET_PRIVATE (skipto, TOTEM_TYPE_SKIPTO, TotemSkiptoPrivate);

	gtk_dialog_set_default_response (GTK_DIALOG (skipto), GTK_RESPONSE_OK);
	g_signal_connect (skipto, "response",
				G_CALLBACK (totem_skipto_response_cb), NULL);
}

static void
totem_skipto_dispose (GObject *object)
{
	TotemSkiptoPrivate *priv = TOTEM_SKIPTO_GET_PRIVATE (object);

	if (priv->xml != NULL) {
		g_object_unref (priv->xml);
		priv->xml = NULL;
	}

	G_OBJECT_CLASS (totem_skipto_parent_class)->dispose (object);
}

void
totem_skipto_update_range (TotemSkipto *skipto, gint64 _time)
{
	g_return_if_fail (TOTEM_IS_SKIPTO (skipto));

	if (_time == skipto->priv->time)
		return;

	gtk_spin_button_set_range (GTK_SPIN_BUTTON (skipto->priv->time_entry),
			0, (gdouble) _time / 1000);
	skipto->priv->time = _time;
}

gint64
totem_skipto_get_range (TotemSkipto *skipto)
{
	gint64 _time;

	g_return_val_if_fail (TOTEM_IS_SKIPTO (skipto), 0);

	_time = gtk_spin_button_get_value (GTK_SPIN_BUTTON (skipto->priv->time_entry)) * 1000;

	return _time;
}

void
totem_skipto_set_seekable (TotemSkipto *skipto, gboolean seekable)
{
	g_return_if_fail (TOTEM_IS_SKIPTO (skipto));

	gtk_dialog_set_response_sensitive (GTK_DIALOG (skipto),
			GTK_RESPONSE_OK, seekable);
}

void
totem_skipto_set_current (TotemSkipto *skipto, gint64 _time)
{
	g_return_if_fail (TOTEM_IS_SKIPTO (skipto));

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (skipto->priv->time_entry),
			(gdouble) (_time / 1000));
}

void
time_entry_activate_cb (GtkEntry *entry, TotemSkipto *skipto)
{
	gtk_dialog_response (GTK_DIALOG (skipto), GTK_RESPONSE_OK);
}

void
tstw_adjustment_value_changed_cb (GtkAdjustment *adjustment, TotemSkipto *skipto)
{
	/* Update the "seconds" label so that it always has the correct singular/plural form */
	/* Translators: label for the seconds selector in the "Skip to" dialogue */
	gtk_label_set_label (skipto->priv->seconds_label, ngettext ("second", "seconds", (int) gtk_adjustment_get_value (adjustment)));
}

GtkWidget *
totem_skipto_new (TotemSkiptoPlugin *plugin)
{
	TotemSkipto *skipto;
	GtkWidget *container;

	skipto = TOTEM_SKIPTO (g_object_new (TOTEM_TYPE_SKIPTO, NULL));

	skipto->priv->totem = plugin->totem;
	skipto->priv->xml = totem_plugin_load_interface (TOTEM_PLUGIN (plugin),
							 "skipto.ui", TRUE,
							 NULL, skipto);

	if (skipto->priv->xml == NULL) {
		g_object_unref (skipto);
		return NULL;
	}
	skipto->priv->time_entry = GTK_WIDGET (gtk_builder_get_object
		(skipto->priv->xml, "tstw_skip_time_entry"));
	skipto->priv->seconds_label = GTK_LABEL (gtk_builder_get_object
		(skipto->priv->xml, "tstw_seconds_label"));

	/* Fix the label width at the maximum necessary for the plural labels, to prevent it changing size when we change the spinner value */
	gtk_label_set_width_chars (skipto->priv->seconds_label, MAX (strlen (_("second")), strlen (_("seconds"))));

	/* Set the initial "seconds" label */
	tstw_adjustment_value_changed_cb (GTK_ADJUSTMENT (gtk_builder_get_object
		(skipto->priv->xml, "tstw_skip_adjustment")), skipto);

	gtk_window_set_title (GTK_WINDOW (skipto), _("Skip to"));
	gtk_dialog_add_buttons (GTK_DIALOG (skipto),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);

	/* Skipto dialog */
	g_signal_connect (G_OBJECT (skipto), "delete-event",
			  G_CALLBACK (gtk_widget_destroy), skipto);

	container = GTK_WIDGET (gtk_builder_get_object (skipto->priv->xml,
				"tstw_skip_vbox"));
	gtk_container_set_border_width (GTK_CONTAINER (skipto), 5);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (skipto))),
			    container,
			    TRUE,       /* expand */
			    TRUE,       /* fill */
			    0);         /* padding */

	gtk_window_set_transient_for (GTK_WINDOW (skipto),
				      totem_get_main_window (plugin->totem));

	gtk_widget_show_all (GTK_WIDGET (skipto));

	return GTK_WIDGET (skipto);
}
