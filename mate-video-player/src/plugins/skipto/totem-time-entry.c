/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2008 Philip Withnall <philip@tecnocode.co.uk>
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
 * Author: Philip Withnall <philip@tecnocode.co.uk>
 */

#include <glib.h>
#include <gtk/gtk.h>

#include "video-utils.h"
#include "totem-time-entry.h"

static gboolean output_cb (GtkSpinButton *self, gpointer user_data);
static gint input_cb (GtkSpinButton *self, gdouble *new_value, gpointer user_data);

G_DEFINE_TYPE (TotemTimeEntry, totem_time_entry, GTK_TYPE_SPIN_BUTTON)

static void
totem_time_entry_class_init (TotemTimeEntryClass *klass)
{
	/* Nothing to see here; please move along */
}

static void
totem_time_entry_init (TotemTimeEntry *self)
{
	/* Connect to signals */
	g_signal_connect (self, "output", G_CALLBACK (output_cb), NULL);
	g_signal_connect (self, "input", G_CALLBACK (input_cb), NULL);
}

GtkWidget *
totem_time_entry_new (GtkAdjustment *adjustment, gdouble climb_rate)
{
	return g_object_new (TOTEM_TYPE_TIME_ENTRY,
			     "adjustment", adjustment,
			     "climb-rate", climb_rate,
			     "digits", 0,
			     "numeric", FALSE,
			     NULL);
}

static gboolean
output_cb (GtkSpinButton *self, gpointer user_data)
{
	gchar *text;

	text = totem_time_to_string ((gint64) gtk_spin_button_get_value (self) * 1000);
	gtk_entry_set_text (GTK_ENTRY (self), text);
	g_free (text);

	return TRUE;
}

static gint
input_cb (GtkSpinButton *self, gdouble *new_value, gpointer user_data)
{
	gint64 val;

	val = totem_string_to_time (gtk_entry_get_text (GTK_ENTRY (self)));
	if (val == -1)
		return GTK_INPUT_ERROR;

	*new_value = val / 1000;
	return TRUE;
}
