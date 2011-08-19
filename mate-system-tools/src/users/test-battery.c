/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2006 Carlos Garnacho
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: Carlos Garnacho Parro <carlosg@mate.org>.
 */

#include "test-battery.h"

static void
show_error_message (GtkWindow *parent, gchar *primary_text, gchar *secondary_text)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (parent,
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 "%s", primary_text);
	gtk_message_dialog_format_secondary_markup (GTK_MESSAGE_DIALOG (dialog),
						    "%s", secondary_text);
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

gboolean
test_battery_run (TestBattery *battery,
		  GtkWindow   *parent,
		  gpointer     data)
{
	gchar *primary_text, *secondary_text;
	gint count = 0;

	primary_text = secondary_text = NULL;

	while (battery[count] && !primary_text) {
		(battery[count]) (&primary_text, &secondary_text, data);
		count++;
	}

	if (primary_text) {
		show_error_message (parent, primary_text, secondary_text);
		g_free (primary_text);
		g_free (secondary_text);

		return FALSE;
	}

	return TRUE;
}
