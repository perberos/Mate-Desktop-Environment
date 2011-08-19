/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Copyright (C) 2004 Fernando Herrera <fherrera@onirica.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include "mateconf-search.h"
#include "mateconf-search-dialog.h"
#include "mateconf-editor-window.h"
#include "mateconf-tree-model.h"
#include "mateconf-list-model.h"
#include "gedit-output-window.h"
#include <mateconf/mateconf-client.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>

#include "mateconf-stock-icons.h"

G_DEFINE_TYPE(MateConfSearchDialog, mateconf_search_dialog, GTK_TYPE_DIALOG)

static void
mateconf_search_dialog_response (GtkDialog *dialog, gint response_id)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
mateconf_search_dialog_class_init (MateConfSearchDialogClass *klass)
{
	GtkDialogClass *dialog_class;

	dialog_class = (GtkDialogClass *)klass;

	dialog_class->response = mateconf_search_dialog_response;
}


static void
mateconf_search_not_found_dialog (MateConfSearchDialog *dialog) 
{
	GtkWidget *not_found_dialog;
	
	not_found_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_MESSAGE_ERROR,
						   GTK_BUTTONS_CLOSE,
						   _("Pattern not found"));
	gtk_dialog_run (GTK_DIALOG (not_found_dialog));
	gtk_widget_destroy (GTK_WIDGET (not_found_dialog));
}

static void
mateconf_search_entry_changed (GtkEntry *entry, MateConfSearchDialog *dialog)
{
        gboolean find_sensitive;
        const gchar *text;

        text = gtk_entry_get_text (GTK_ENTRY (entry));
        find_sensitive = text != NULL && text[0] != '\0';

        gtk_widget_set_sensitive (dialog->search_button, find_sensitive);
}

static void
mateconf_search_dialog_search (GtkWidget *button, MateConfSearchDialog *dialog)
{
	MateConfEditorWindow *window;
	GdkCursor *cursor;
	GdkWindow *dialog_window;

	gchar *pattern;
	int res;

	window = g_object_get_data (G_OBJECT (dialog), "editor-window");
	gedit_output_window_clear (GEDIT_OUTPUT_WINDOW (window->output_window));
	window->output_window_type = MATECONF_EDITOR_WINDOW_OUTPUT_WINDOW_SEARCH;

	pattern = g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->entry)));
	dialog_window = gtk_widget_get_window (GTK_WIDGET (dialog));

	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (dialog_window, cursor);
	gdk_cursor_unref (cursor);
	gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (dialog)));

	res = mateconf_tree_model_build_match_list  (MATECONF_TREE_MODEL (window->tree_model),
						  GEDIT_OUTPUT_WINDOW (window->output_window), 
						  pattern,
						  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->search_in_keys)),
						  gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->search_in_values)),
						  G_OBJECT (dialog));

	g_free (pattern);

	if (dialog != NULL && GTK_IS_WIDGET (dialog)) {
		gdk_window_set_cursor (dialog_window, NULL);
		gdk_display_flush (gtk_widget_get_display (GTK_WIDGET (dialog)));

		if (res != 0) {
			gtk_widget_destroy (GTK_WIDGET (dialog));
		} else {
			mateconf_search_not_found_dialog (dialog);
		}
	}
}

static void
mateconf_search_dialog_init (MateConfSearchDialog *dialog)
{
	GtkWidget *action_area, *content_area;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *label;

	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	action_area = gtk_dialog_get_action_area (GTK_DIALOG (dialog));

	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (content_area), 2);
	
	hbox = gtk_hbox_new (FALSE, 12);
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);

	gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);

	gtk_window_set_title (GTK_WINDOW (dialog), _("Find"));

	label = gtk_label_new_with_mnemonic (_("_Search for: "));
			
	dialog->entry = gtk_entry_new ();
	g_signal_connect (dialog->entry, "changed",
			  G_CALLBACK (mateconf_search_entry_changed), dialog);
	gtk_label_set_mnemonic_widget (GTK_LABEL(label), GTK_WIDGET(dialog->entry));
	
	dialog->search_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
	gtk_widget_set_sensitive (dialog->search_button, FALSE);
	g_signal_connect (dialog->search_button, "clicked",
			  G_CALLBACK (mateconf_search_dialog_search), dialog);

	gtk_widget_set_can_default (dialog->search_button, TRUE);
	gtk_window_set_default(GTK_WINDOW(dialog), dialog->search_button);
	gtk_entry_set_activates_default(GTK_ENTRY(dialog->entry), TRUE);
	
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), dialog->entry, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

	dialog->search_in_keys = gtk_check_button_new_with_mnemonic (_("Search also in key _names"));
	gtk_box_pack_start (GTK_BOX (vbox), dialog->search_in_keys, TRUE, TRUE, 0);

	dialog->search_in_values = gtk_check_button_new_with_mnemonic (_("Search also in key _values"));
	gtk_box_pack_start (GTK_BOX (vbox), dialog->search_in_values, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (content_area), vbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (action_area), dialog->search_button, TRUE, TRUE, 0);
	gtk_widget_show_all (action_area);
	gtk_widget_show_all (vbox);
	
	gtk_widget_show_all (hbox);
}

GtkWidget *
mateconf_search_dialog_new (GtkWindow *parent)
{
	GtkWidget *dialog;

	dialog = g_object_new (MATECONF_TYPE_SEARCH_DIALOG, NULL);
	g_object_set_data (G_OBJECT (dialog), "editor-window", parent);
	gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

	return dialog;
}
