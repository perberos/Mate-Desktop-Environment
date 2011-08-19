/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/* mateconf-editor
 *
 * Copyright (C) 2001, 2002 Anders Carlsson <andersca@gnu.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "mateconf-cell-renderer.h"
#include "mateconf-key-editor.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>

enum
{
  EDIT_INTEGER,
  EDIT_BOOLEAN,
  EDIT_STRING,
  EDIT_FLOAT,
  EDIT_LIST
};


static void
combo_box_changed (GtkWidget *combo_box,
		   MateConfKeyEditor *editor)
{
	int index;

	index = gtk_combo_box_get_active (GTK_COMBO_BOX (combo_box));

	editor->active_type = index;
		
	gtk_window_set_resizable (GTK_WINDOW (editor), FALSE);	
	gtk_widget_hide (editor->int_box);
	gtk_widget_hide (editor->bool_box);
	gtk_widget_hide (editor->string_box);
	gtk_widget_hide (editor->float_box);
	gtk_widget_hide (editor->list_box);
				       
	switch (index) {
		case EDIT_INTEGER:
			gtk_widget_show_all (editor->int_box);
			break;
		case EDIT_BOOLEAN:
			gtk_widget_show_all (editor->bool_box);
			break;
		case EDIT_STRING:
			gtk_widget_show_all (editor->string_box);
			break;
  		case EDIT_FLOAT:
			gtk_widget_show_all (editor->float_box);
			break;
		case EDIT_LIST:
			gtk_window_set_resizable (GTK_WINDOW (editor), TRUE);
			gtk_widget_show_all (editor->list_box);
			break;
		default:
			g_assert_not_reached ();
	}
}

static void
bool_button_toggled (GtkWidget *bool_button,
		     gpointer   data)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (bool_button)))
		gtk_label_set_label (GTK_LABEL (gtk_bin_get_child (GTK_BIN (bool_button))),
				     _("T_rue"));
	else
		gtk_label_set_label (GTK_LABEL (gtk_bin_get_child (GTK_BIN (bool_button))),
				     _("_False"));
}

static void
mateconf_key_editor_list_entry_changed (MateConfCellRenderer *cell, const gchar *path_str, MateConfValue *new_value, MateConfKeyEditor *editor)
{
	GtkTreeIter iter;
	GtkTreePath *path;

	path = gtk_tree_path_new_from_string (path_str);
        gtk_tree_model_get_iter (GTK_TREE_MODEL(editor->list_model), &iter, path);

	gtk_list_store_set (editor->list_model, &iter,
			    0, new_value,
			    -1);
}

static void
list_type_menu_changed (GtkWidget *combobox,
			MateConfKeyEditor *editor)
{
	gtk_list_store_clear (editor->list_model);
}

static GtkWidget *
mateconf_key_editor_create_combo_box (MateConfKeyEditor *editor)
{
	GtkWidget *combo_box;

	combo_box = gtk_combo_box_new_text ();

	/* These have to be ordered so the EDIT_ enum matches the
	 * menu indices
	 */

	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), _("Integer"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), _("Boolean"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), _("String"));
	/* Translators: this refers to "Floating point":
	 * see http://en.wikipedia.org/wiki/Floating_point
	 */
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), _("Float"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), _("List"));

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);

	g_signal_connect (combo_box, "changed",
			  G_CALLBACK (combo_box_changed),
			  editor);
	
	gtk_widget_show_all (combo_box);
	return combo_box;
}

static GtkWidget *
mateconf_key_editor_create_list_type_menu (MateConfKeyEditor *editor)
{
	GtkWidget *combo_box;

	combo_box = gtk_combo_box_new_text ();

	/* These have to be ordered so the EDIT_ enum matches the
	 * combobox indices
	 */

	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), _("Integer"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), _("Boolean"));
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo_box), _("String"));

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo_box), 0);
	
	g_signal_connect (combo_box, "changed",
			  G_CALLBACK (list_type_menu_changed),
			  editor);

        gtk_widget_show_all (combo_box);
        return combo_box;
}

static void
update_list_buttons (MateConfKeyEditor *editor)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;
	gint selected;
	gboolean can_edit = FALSE, can_remove = FALSE, can_go_up = FALSE, can_go_down = FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (editor->list_widget));

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		path = gtk_tree_model_get_path (model, &iter);

		selected = gtk_tree_path_get_indices (path)[0];

		can_edit = can_remove = TRUE;
		can_go_up = selected > 0;
		can_go_down =  selected < gtk_tree_model_iter_n_children (model, NULL) - 1;

		gtk_tree_path_free (path);
	}

	gtk_widget_set_sensitive (editor->remove_button, can_remove);
	gtk_widget_set_sensitive (editor->edit_button, can_edit);
	gtk_widget_set_sensitive (editor->go_up_button, can_go_up);
	gtk_widget_set_sensitive (editor->go_down_button, can_go_down);

}

static void
list_selection_changed (GtkTreeSelection *selection,
		        MateConfKeyEditor *editor)
{
	update_list_buttons (editor);
}

static void
list_add_clicked (GtkButton *button,
		  MateConfKeyEditor *editor)
{
	GtkWidget *content_area;
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *value_widget;
	GtkWidget *label;
	gint response;
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	dialog = gtk_dialog_new_with_buttons (_("Add New List Entry"),
                                              GTK_WINDOW (editor),
					      GTK_DIALOG_MODAL| GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OK, GTK_RESPONSE_OK,
					      NULL);
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_box_set_spacing (GTK_BOX (content_area), 2);
	
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("_New list value:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	switch (gtk_combo_box_get_active (GTK_COMBO_BOX (editor->list_type_menu))) {
		case EDIT_INTEGER:
			value_widget = gtk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (value_widget), 0);
			break;
		case EDIT_BOOLEAN:
			value_widget = gtk_toggle_button_new_with_mnemonic (_("_False"));
			g_signal_connect (value_widget, "toggled",
		                          G_CALLBACK (bool_button_toggled),
					  editor);
			break;
		case EDIT_STRING:
			value_widget = gtk_entry_new ();
			gtk_entry_set_activates_default (GTK_ENTRY (value_widget), TRUE);
			break;
		default:
			value_widget = NULL;
			g_assert_not_reached ();
	}

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), value_widget);
	gtk_box_pack_start (GTK_BOX (hbox), value_widget, TRUE, TRUE, 0);

	gtk_widget_show_all (hbox);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_OK) {
		MateConfValue *value;
		
	        value = NULL;

		switch (gtk_combo_box_get_active (GTK_COMBO_BOX (editor->list_type_menu))) {
			case EDIT_INTEGER:
				value = mateconf_value_new (MATECONF_VALUE_INT);
				mateconf_value_set_int (value,
						     gtk_spin_button_get_value (GTK_SPIN_BUTTON (value_widget)));
				break;

			case EDIT_BOOLEAN:
				value = mateconf_value_new (MATECONF_VALUE_BOOL);
				mateconf_value_set_bool (value,
						      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (value_widget)));
				break;

			case EDIT_STRING:
		                {
		                        char *text;
				
		                        text = gtk_editable_get_chars (GTK_EDITABLE (value_widget), 0, -1);
		                        value = mateconf_value_new (MATECONF_VALUE_STRING);
		                        mateconf_value_set_string (value, text);
		                        g_free (text);
		                }
	                	break;
			default:
				g_assert_not_reached ();
				
		}

		gtk_list_store_append (editor->list_model, &iter);
                gtk_list_store_set (editor->list_model, &iter, 0, value, -1);

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (editor->list_widget));
		gtk_tree_selection_select_iter(selection, &iter);
	}
	
	gtk_widget_destroy (dialog);
}

static void
list_edit_element (MateConfKeyEditor *editor)
{
	GtkWidget *content_area;
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *value_widget;
	GtkWidget *label;
	gint response;
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	MateConfValue *value = NULL;

	dialog = gtk_dialog_new_with_buttons (_("Edit List Entry"),
                                              GTK_WINDOW (editor),
					      GTK_DIALOG_MODAL| GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_OK, GTK_RESPONSE_OK,
					      NULL);
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_box_set_spacing (GTK_BOX (content_area), 2);
	
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	gtk_box_pack_start (GTK_BOX (content_area), hbox, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("_Edit list value:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (editor->list_widget));
	gtk_tree_selection_get_selected (selection, &model, &iter);
	gtk_tree_model_get (GTK_TREE_MODEL (model), &iter, 0, &value, -1);
	switch (gtk_combo_box_get_active (GTK_COMBO_BOX (editor->list_type_menu))) {
		case EDIT_INTEGER:
			value_widget = gtk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (value_widget), mateconf_value_get_int (value));
			break;
		case EDIT_BOOLEAN:
			value_widget = gtk_toggle_button_new_with_mnemonic (_("_False"));
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (value_widget), mateconf_value_get_bool (value));
			break;
		case EDIT_STRING:
			value_widget = gtk_entry_new ();
			gtk_entry_set_text (GTK_ENTRY (value_widget), mateconf_value_get_string (value));
			break;
		default:
			value_widget = NULL;
			g_assert_not_reached ();
	}
	mateconf_value_free (value);

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), value_widget);
	gtk_box_pack_start (GTK_BOX (hbox), value_widget, TRUE, TRUE, 0);

	gtk_widget_show_all (hbox);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_OK) {
		MateConfValue *value;
		
	        value = NULL;

		switch (gtk_combo_box_get_active (GTK_COMBO_BOX (editor->list_type_menu))) {
			case EDIT_INTEGER:
				value = mateconf_value_new (MATECONF_VALUE_INT);
				mateconf_value_set_int (value,
						     gtk_spin_button_get_value (GTK_SPIN_BUTTON (value_widget)));
				break;

			case EDIT_BOOLEAN:
				value = mateconf_value_new (MATECONF_VALUE_BOOL);
				mateconf_value_set_bool (value,
						      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (value_widget)));
				break;

			case EDIT_STRING:
		                {
		                        char *text;
				
		                        text = gtk_editable_get_chars (GTK_EDITABLE (value_widget), 0, -1);
		                        value = mateconf_value_new (MATECONF_VALUE_STRING);
		                        mateconf_value_set_string (value, text);
		                        g_free (text);
		                }
	                	break;
			default:
				g_assert_not_reached ();
				
		}
                gtk_list_store_set (editor->list_model, &iter, 0, value, -1);

	}
	
	gtk_widget_destroy (dialog);
}



static void
list_remove_clicked (GtkButton *button,
		     MateConfKeyEditor *editor)
{
	GtkTreeIter iter;
	GtkTreeIter tmp;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (editor->list_widget));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter)) {
		tmp = iter;
		if (gtk_tree_model_iter_next (GTK_TREE_MODEL (editor->list_model), &tmp)) {
			gtk_tree_selection_select_iter (selection, &tmp);
		} else {
			GtkTreePath *path;
			path = gtk_tree_model_get_path (GTK_TREE_MODEL (editor->list_model), &iter);
			if (gtk_tree_path_prev (path)) {
				gtk_tree_selection_select_path (selection, path);
			}
			gtk_tree_path_free (path);
		}
		gtk_list_store_remove (editor->list_model, &iter);
	}
}

static void
list_go_up_clicked (GtkButton *button,
		    MateConfKeyEditor *editor)
{
	GtkTreeIter iter_first;
	GtkTreeIter iter_second;
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (editor->list_widget));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter_second)) {
		MateConfValue *first;
		MateConfValue *second;
		GtkTreePath *path;
		
		path = gtk_tree_model_get_path (GTK_TREE_MODEL (editor->list_model), &iter_second);
		gtk_tree_path_prev (path);
		gtk_tree_model_get_iter (GTK_TREE_MODEL (editor->list_model), &iter_first, path);
		
		gtk_tree_model_get (GTK_TREE_MODEL (editor->list_model), &iter_first, 0, &first, -1);
		gtk_tree_model_get (GTK_TREE_MODEL (editor->list_model), &iter_second, 0, &second, -1);
				
		gtk_list_store_set (editor->list_model, &iter_first, 0, second, -1);
		gtk_list_store_set (editor->list_model, &iter_second, 0, first, -1);

		gtk_tree_path_free (path);

		gtk_tree_selection_select_iter(selection, &iter_first);
	}
}

static void
list_go_down_clicked (GtkButton *button,
		      MateConfKeyEditor *editor)
{
	GtkTreeIter iter_first;
	GtkTreeIter iter_second;
	GtkTreeSelection *selection;
	
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (editor->list_widget));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter_first)) {
		MateConfValue *first;
		MateConfValue *second;

		iter_second = iter_first;

		gtk_tree_model_iter_next (GTK_TREE_MODEL (editor->list_model), &iter_second);

		gtk_tree_model_get (GTK_TREE_MODEL (editor->list_model), &iter_first, 0, &first, -1);
		gtk_tree_model_get (GTK_TREE_MODEL (editor->list_model), &iter_second, 0, &second, -1);
				
		gtk_list_store_set (editor->list_model, &iter_first, 0, second, -1);
		gtk_list_store_set (editor->list_model, &iter_second, 0, first, -1);
		gtk_tree_selection_select_iter(selection, &iter_second);
	}
}

static void
list_edit_clicked (GtkWidget      *button,
                   MateConfKeyEditor *editor)
{
        list_edit_element (editor);
}

static void
list_view_row_activated (GtkTreeView       *tree_view,
                         GtkTreePath       *path,
                         GtkTreeViewColumn *column,
                         MateConfKeyEditor    *editor)
{
        list_edit_element (editor);
}

static void
fix_button_align (GtkWidget *button)
{
        GtkWidget *child = gtk_bin_get_child (GTK_BIN (button));

	if (GTK_IS_ALIGNMENT (child))
		g_object_set (G_OBJECT (child), "xalign", 0.0, NULL);
        else if (GTK_IS_LABEL (child))
                g_object_set (G_OBJECT (child), "xalign", 0.0, NULL);
}


static void
mateconf_key_editor_class_init (MateConfKeyEditorClass *klass)
{
}

static void
mateconf_key_editor_init (MateConfKeyEditor *editor)
{
	GtkWidget *content_area;
	GtkWidget *hbox, *vbox, *framebox;
	GtkWidget *label, *image;
	GtkWidget *value_box;
	GtkWidget *button_box, *button;
	GtkSizeGroup *size_group;
	GtkCellRenderer *cell_renderer;
	GtkTreeSelection *list_selection;
	GtkWidget *sw;

	gtk_dialog_add_buttons (GTK_DIALOG (editor),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
				GTK_STOCK_OK, GTK_RESPONSE_OK,
				NULL);
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (editor));
	gtk_dialog_set_default_response (GTK_DIALOG (editor), GTK_RESPONSE_OK);

	gtk_container_set_border_width (GTK_CONTAINER (editor), 5);
	gtk_dialog_set_has_separator (GTK_DIALOG (editor), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (editor), FALSE);
	gtk_box_set_spacing (GTK_BOX (content_area), 2);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (content_area), vbox, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_widget_show_all (vbox);

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);
	
	editor->path_box = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new (_("Path:"));
	gtk_size_group_add_widget (size_group, label);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (editor->path_box), label, FALSE, FALSE, 0);
	editor->path_label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (editor->path_label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (editor->path_box), editor->path_label, TRUE, TRUE, 0);
	gtk_widget_show_all (editor->path_box);
	gtk_box_pack_start (GTK_BOX (vbox), editor->path_box, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new_with_mnemonic (_("_Name:"));
	gtk_size_group_add_widget (size_group, label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	editor->name_entry = gtk_entry_new ();
	gtk_widget_set_size_request (editor->name_entry, 250, -1);
	gtk_entry_set_activates_default (GTK_ENTRY (editor->name_entry), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), editor->name_entry, TRUE, TRUE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), editor->name_entry);
	gtk_widget_show_all (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new_with_mnemonic (_("_Type:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_size_group_add_widget (size_group, label);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	editor->combobox = mateconf_key_editor_create_combo_box (editor);
	gtk_box_pack_start (GTK_BOX (hbox),
			    editor->combobox,
			    TRUE, TRUE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), editor->combobox);
	gtk_widget_show_all (hbox);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	g_object_unref (size_group);

	framebox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (framebox);
	gtk_box_pack_start (GTK_BOX (vbox), framebox, FALSE, FALSE, 0);

	editor->non_writable_label = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (framebox), editor->non_writable_label, FALSE, FALSE, 0);

	image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (editor->non_writable_label), image, FALSE, FALSE, 0);

	label = gtk_label_new (_("This key is not writable"));
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (editor->non_writable_label), label, FALSE, FALSE, 0);

	editor->active_type = EDIT_INTEGER;

	/* EDIT_INTEGER */
	editor->int_box = gtk_hbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (editor->int_box), label, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("_Value:"));
	editor->int_widget = gtk_spin_button_new_with_range (G_MININT, G_MAXINT, 1);

	/* Set a nicer default value */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (editor->int_widget), 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), editor->int_widget);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), editor->int_widget, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (editor->int_box), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), editor->int_box, FALSE, FALSE, 0);
	gtk_widget_show_all (editor->int_box);
	
	/* EDIT_BOOLEAN */
	editor->bool_box = gtk_hbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new ("    ");
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (editor->bool_box), label, FALSE, FALSE, 0);
	label = gtk_label_new_with_mnemonic (_("_Value:"));
	editor->bool_widget = gtk_toggle_button_new_with_mnemonic (_("_False"));
	g_signal_connect (editor->bool_widget, "toggled",
			  G_CALLBACK (bool_button_toggled),
			  editor);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), editor->bool_widget);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), editor->bool_widget, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (editor->bool_box), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), editor->bool_box, FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);
	gtk_widget_hide (editor->bool_box);

	/* EDIT_STRING */
	editor->string_box = gtk_hbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new ("    ");
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (editor->string_box), label, FALSE, FALSE, 0);
	label = gtk_label_new_with_mnemonic (_("_Value:"));
	editor->string_widget = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (editor->string_widget), TRUE);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), editor->string_widget);	
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), editor->string_widget, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (editor->string_box), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), editor->string_box, FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);
	gtk_widget_hide (editor->string_box);

	/* EDIT_FLOAT */
	editor->float_box = gtk_hbox_new (FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new ("    ");
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (editor->float_box), label, FALSE, FALSE, 0);
	label = gtk_label_new_with_mnemonic (_("_Value:"));
	editor->float_widget = gtk_spin_button_new_with_range (G_MINFLOAT, G_MAXFLOAT, 0.1);

	/* Set a nicer default value */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (editor->float_widget), 0.0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), editor->float_widget);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), editor->float_widget, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (editor->float_box), hbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), editor->float_box, FALSE, FALSE, 0);
	gtk_widget_show_all (hbox);
	gtk_widget_hide (editor->float_box);

        /* EDIT_LIST */
	editor->list_box = gtk_hbox_new (FALSE, 0);
	label = gtk_label_new ("    ");
	gtk_widget_show (label);
	gtk_box_pack_start (GTK_BOX (editor->list_box), label, FALSE, FALSE, 0);

        value_box = gtk_vbox_new (FALSE, 6);

	hbox = gtk_hbox_new (FALSE, 12);
	label = gtk_label_new_with_mnemonic (_("List _type:"));
	editor->list_type_menu = mateconf_key_editor_create_list_type_menu (editor);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), editor->list_type_menu);
	gtk_box_pack_start (GTK_BOX (value_box), hbox, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
        gtk_box_pack_start (GTK_BOX (hbox),
                            editor->list_type_menu, TRUE, TRUE, 0);

	label = gtk_label_new_with_mnemonic (_("_Values:"));
	gtk_misc_set_alignment(GTK_MISC (label), 0.0, 0.5);
  
	hbox = gtk_hbox_new (FALSE, 12);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					     GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,			      					GTK_POLICY_AUTOMATIC);

        editor->list_model = gtk_list_store_new (1, g_type_from_name("MateConfValue"));
	editor->list_widget = gtk_tree_view_new_with_model (GTK_TREE_MODEL (editor->list_model));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (editor->list_widget), FALSE);
	list_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (editor->list_widget));
	g_signal_connect (G_OBJECT (editor->list_widget), "row_activated",
			  G_CALLBACK (list_view_row_activated), editor);
	g_signal_connect (G_OBJECT (list_selection), "changed",
			  G_CALLBACK (list_selection_changed), editor);
 	
	
	cell_renderer = mateconf_cell_renderer_new ();
	g_signal_connect (G_OBJECT (cell_renderer), "changed", G_CALLBACK (mateconf_key_editor_list_entry_changed), editor);

	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (editor->list_widget), -1, NULL, cell_renderer, "value", 0, NULL);

	button_box = gtk_vbutton_box_new ();
	gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), GTK_BUTTONBOX_START);
	gtk_box_set_spacing (GTK_BOX (button_box), 6);
	button = gtk_button_new_from_stock (GTK_STOCK_ADD);
	fix_button_align (button);
	editor->remove_button = gtk_button_new_from_stock (GTK_STOCK_REMOVE);
	fix_button_align (editor->remove_button);
	editor->edit_button = gtk_button_new_from_stock (GTK_STOCK_EDIT);
	fix_button_align (editor->edit_button);
	editor->go_up_button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
	fix_button_align (editor->go_up_button);
	editor->go_down_button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
	fix_button_align (editor->go_down_button);
	g_signal_connect (G_OBJECT (button), "clicked",
			  G_CALLBACK (list_add_clicked), editor);
	g_signal_connect (G_OBJECT (editor->edit_button), "clicked",
			  G_CALLBACK (list_edit_clicked), editor);
	g_signal_connect (G_OBJECT (editor->remove_button), "clicked",
			  G_CALLBACK (list_remove_clicked), editor);
	g_signal_connect (G_OBJECT (editor->go_up_button), "clicked",
			  G_CALLBACK (list_go_up_clicked), editor);
	g_signal_connect (G_OBJECT (editor->go_down_button), "clicked",
			  G_CALLBACK (list_go_down_clicked), editor);
	gtk_box_pack_start (GTK_BOX (button_box), button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_box), editor->remove_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_box), editor->edit_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_box), editor->go_up_button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (button_box), editor->go_down_button, FALSE, FALSE, 0);

	update_list_buttons (editor);

        gtk_label_set_mnemonic_widget (GTK_LABEL (label), editor->list_widget); 
        gtk_box_pack_start (GTK_BOX (value_box), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (value_box), hbox, TRUE, TRUE, 0);
        gtk_box_pack_start (GTK_BOX (hbox), sw, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), button_box, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (sw), editor->list_widget);
	gtk_box_pack_start (GTK_BOX (editor->list_box), value_box, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), editor->list_box, TRUE, TRUE, 0);
	gtk_widget_show_all (value_box);
	gtk_widget_hide (editor->list_box);
}

GType
mateconf_key_editor_get_type (void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof (MateConfKeyEditorClass),
			NULL,		/* base_init */
			NULL,		/* base_finalize */
			(GClassInitFunc) mateconf_key_editor_class_init,
			NULL,		/* class_finalize */
			NULL,		/* class_data */
			sizeof (MateConfKeyEditor),
			0,              /* n_preallocs */
			(GInstanceInitFunc) mateconf_key_editor_init
		};

		object_type = g_type_register_static (GTK_TYPE_DIALOG, "MateConfKeyEditor", &object_info, 0);
	}

	return object_type;
}

GtkWidget *
mateconf_key_editor_new (MateConfKeyEditorAction action)
{
	MateConfKeyEditor *dialog;

	dialog = g_object_new (MATECONF_TYPE_KEY_EDITOR, NULL);

	switch (action) {
	case MATECONF_KEY_EDITOR_NEW_KEY:
		gtk_window_set_title (GTK_WINDOW (dialog), _("New Key"));
		gtk_widget_grab_focus (dialog->name_entry);
		break;
	case MATECONF_KEY_EDITOR_EDIT_KEY:
		gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Key"));

		gtk_widget_set_sensitive (dialog->name_entry, FALSE);
		gtk_widget_hide (dialog->path_box);
		break;
	default:
		break;
	}
	
	return GTK_WIDGET (dialog);
}

void
mateconf_key_editor_set_value (MateConfKeyEditor *editor, MateConfValue *value)
{
	if (value == NULL) {
		g_print ("dealing with an unset value\n");
		return;
	}

	gtk_widget_set_sensitive (editor->combobox, FALSE);
	gtk_widget_set_sensitive (editor->list_type_menu, FALSE);

	switch (value->type) {
	case MATECONF_VALUE_INT:
		gtk_combo_box_set_active (GTK_COMBO_BOX (editor->combobox), EDIT_INTEGER);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (editor->int_widget), mateconf_value_get_int (value));
		break;
	case MATECONF_VALUE_STRING:
		gtk_combo_box_set_active (GTK_COMBO_BOX (editor->combobox), EDIT_STRING);
		gtk_entry_set_text (GTK_ENTRY (editor->string_widget), mateconf_value_get_string (value));
		break;
	case MATECONF_VALUE_BOOL:
		gtk_combo_box_set_active (GTK_COMBO_BOX (editor->combobox), EDIT_BOOLEAN);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (editor->bool_widget), mateconf_value_get_bool (value));
		break;
	case MATECONF_VALUE_FLOAT:
		gtk_combo_box_set_active (GTK_COMBO_BOX (editor->combobox), EDIT_FLOAT);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (editor->float_widget), mateconf_value_get_float (value));
		break;
	case MATECONF_VALUE_LIST:
		{
			GSList* iter = mateconf_value_get_list(value);
			
			switch (mateconf_value_get_list_type(value)) {
				case MATECONF_VALUE_INT:
					gtk_combo_box_set_active (GTK_COMBO_BOX (editor->list_type_menu), EDIT_INTEGER);
					break;
				case MATECONF_VALUE_FLOAT:
					gtk_combo_box_set_active (GTK_COMBO_BOX (editor->list_type_menu), EDIT_FLOAT);
					break;
				case MATECONF_VALUE_STRING:
					gtk_combo_box_set_active (GTK_COMBO_BOX (editor->list_type_menu), EDIT_STRING);
					break;
				case MATECONF_VALUE_BOOL:
					gtk_combo_box_set_active (GTK_COMBO_BOX (editor->list_type_menu), EDIT_BOOLEAN);
					break;
				default:
					g_assert_not_reached ();
			}

			gtk_combo_box_set_active (GTK_COMBO_BOX (editor->combobox), EDIT_LIST);
				
		        while (iter != NULL) {
				MateConfValue* element = (MateConfValue*) iter->data;
				GtkTreeIter tree_iter;

			        gtk_list_store_append (editor->list_model, &tree_iter);
			        gtk_list_store_set (editor->list_model, &tree_iter, 0, element, -1);
				iter = g_slist_next(iter);
			}
		}
		break;
	default:
		g_assert_not_reached ();
		break;
	}
	
}

MateConfValue*
mateconf_key_editor_get_value (MateConfKeyEditor *editor)
{
	MateConfValue *value;

	value = NULL;
	
	switch (editor->active_type) {

	case EDIT_INTEGER:
		value = mateconf_value_new (MATECONF_VALUE_INT);
		mateconf_value_set_int (value,
				     gtk_spin_button_get_value (GTK_SPIN_BUTTON (editor->int_widget)));
		break;

	case EDIT_BOOLEAN:
		value = mateconf_value_new (MATECONF_VALUE_BOOL);
		mateconf_value_set_bool (value,
				      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (editor->bool_widget)));
		break;

	case EDIT_STRING:
		{
			char *text;
			
			text = gtk_editable_get_chars (GTK_EDITABLE (editor->string_widget), 0, -1);
			value = mateconf_value_new (MATECONF_VALUE_STRING);
			mateconf_value_set_string (value, text);
			g_free (text);
		}
		break;
	case EDIT_FLOAT:
		value = mateconf_value_new (MATECONF_VALUE_FLOAT);
		mateconf_value_set_float (value,
				       gtk_spin_button_get_value (GTK_SPIN_BUTTON (editor->float_widget)));
		break;

	case EDIT_LIST:
		{
			GSList* list = NULL;
			GtkTreeIter iter;
			GtkTreeModel* model = GTK_TREE_MODEL (editor->list_model);

			if (gtk_tree_model_get_iter_first (model, &iter)) {
				do {
					MateConfValue *element;

					gtk_tree_model_get (model, &iter, 0, &element, -1);
					list = g_slist_append (list, element);
				} while (gtk_tree_model_iter_next (model, &iter));
			}

			value = mateconf_value_new (MATECONF_VALUE_LIST);

			switch (gtk_combo_box_get_active (GTK_COMBO_BOX (editor->list_type_menu))) {
			        case EDIT_INTEGER:
					mateconf_value_set_list_type (value, MATECONF_VALUE_INT);
					break;
				case EDIT_BOOLEAN:
					mateconf_value_set_list_type (value, MATECONF_VALUE_BOOL);
					break;
				case EDIT_STRING:
					mateconf_value_set_list_type (value, MATECONF_VALUE_STRING);
					break;
				default:
					g_assert_not_reached ();
			}
			mateconf_value_set_list_nocopy (value, list);
		}
		break;
	}

	return value;
}

void
mateconf_key_editor_set_key_path (MateConfKeyEditor *editor, const char *path)
{
	gtk_label_set_text (GTK_LABEL (editor->path_label), path);
}

void
mateconf_key_editor_set_key_name (MateConfKeyEditor *editor, const char *path)
{
	gtk_entry_set_text (GTK_ENTRY (editor->name_entry), path);
}

void
mateconf_key_editor_set_writable (MateConfKeyEditor *editor, gboolean writable)
{
	if (writable)
		gtk_widget_hide (editor->non_writable_label);
	else
		gtk_widget_show (editor->non_writable_label);

	gtk_widget_set_sensitive (editor->bool_box, writable);
	gtk_widget_set_sensitive (editor->int_box, writable);
	gtk_widget_set_sensitive (editor->float_box, writable);
	gtk_widget_set_sensitive (editor->list_box, writable);
	gtk_widget_set_sensitive (editor->string_box, writable);
}

G_CONST_RETURN char *
mateconf_key_editor_get_key_name (MateConfKeyEditor *editor)
{
	return gtk_entry_get_text (GTK_ENTRY (editor->name_entry));
}

char *
mateconf_key_editor_get_full_key_path (MateConfKeyEditor *editor)
{
	char *full_key_path;
	const char *key_path;

	key_path = gtk_label_get_text (GTK_LABEL (editor->path_label));

	if (key_path[strlen(key_path) - 1] != '/') {
		full_key_path = g_strdup_printf ("%s/%s",
						 key_path,
						 gtk_entry_get_text (GTK_ENTRY (editor->name_entry)));
	}
	else {
		full_key_path = g_strdup_printf ("%s%s",
						 key_path,
						 gtk_entry_get_text (GTK_ENTRY (editor->name_entry)));

	}

	return full_key_path;
}

