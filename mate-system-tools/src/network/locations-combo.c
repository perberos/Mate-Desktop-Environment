/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2006 Carlos Garnacho
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
 * Authors: Carlos Garnacho Parro  <carlosg@mate.org>
 */

#include <glib.h>
#include <glib/gi18n.h>
#include "locations-combo.h"
#include "gst.h"

#define GST_LOCATIONS_COMBO_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GST_TYPE_LOCATIONS_COMBO, GstLocationsComboPrivate))

typedef struct _GstLocationsComboPrivate GstLocationsComboPrivate;

struct _GstLocationsComboPrivate
{
  GstTool *tool;
  GtkTreeModel *model;
  GtkWidget *combo;
  GtkWidget *save_button;
  GtkWidget *delete_button;

  GtkWidget *save_dialog;
  GtkWidget *location_entry;
};

enum {
  PROP_0,
  PROP_TOOL,
  PROP_COMBO,
  PROP_SAVE,
  PROP_DELETE
};

static void gst_locations_combo_class_init   (GstLocationsComboClass *class);
static void gst_locations_combo_init         (GstLocationsCombo *combo);
static void gst_locations_combo_finalize     (GObject *object);
static void gst_locations_combo_set_property (GObject         *object,
					      guint            prop_id,
					      const GValue    *value,
					      GParamSpec      *pspec);
static void gst_locations_combo_get_property (GObject         *object,
					      guint            prop_id,
					      GValue          *value,
					      GParamSpec      *pspec);
static GObject* gst_locations_combo_constructor (GType                  type,
						 guint                  n_construct_properties,
						 GObjectConstructParam *construct_params);
static void gst_locations_combo_changed      (GstNetworkLocations *locations);


G_DEFINE_TYPE (GstLocationsCombo, gst_locations_combo, GST_TYPE_NETWORK_LOCATIONS);


static void
gst_locations_combo_class_init (GstLocationsComboClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GstNetworkLocationsClass *locations_class = GST_NETWORK_LOCATIONS_CLASS (class);

  object_class->finalize = gst_locations_combo_finalize;
  object_class->get_property = gst_locations_combo_get_property;
  object_class->set_property = gst_locations_combo_set_property;
  object_class->constructor = gst_locations_combo_constructor;

  locations_class->changed = gst_locations_combo_changed;

  g_object_class_install_property (object_class,
				   PROP_TOOL,
				   g_param_spec_object ("tool",
							"Tool",
							"Tool",
							GST_TYPE_TOOL,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_COMBO,
				   g_param_spec_object ("combo",
							"Combo",
							"Combo",
							GTK_TYPE_COMBO_BOX,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_SAVE,
				   g_param_spec_object ("save",
							"Save",
							"Save",
							GTK_TYPE_BUTTON,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_DELETE,
				   g_param_spec_object ("delete",
							"Delete",
							"Delete",
							GTK_TYPE_BUTTON,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_type_class_add_private (object_class,
			    sizeof (GstLocationsComboPrivate));
}

static void
gst_locations_combo_init (GstLocationsCombo *combo)
{
  GstLocationsComboPrivate *priv;

  combo->_priv = priv = GST_LOCATIONS_COMBO_GET_PRIVATE (combo);
  priv->model = GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING));
}

static void
gst_locations_combo_finalize (GObject *object)
{
  GstLocationsComboPrivate *priv;

  priv = GST_LOCATIONS_COMBO_GET_PRIVATE (object);

  if (priv->combo)
    g_object_unref (priv->combo);

  if (priv->save_button)
    g_object_unref (priv->save_button);

  if (priv->delete_button)
    g_object_unref (priv->delete_button);

  if (priv->model)
    g_object_unref (priv->model);
}

static void
gst_locations_combo_set_property (GObject         *object,
				  guint            prop_id,
				  const GValue    *value,
				  GParamSpec      *pspec)
{
  GstLocationsComboPrivate *priv;

  g_return_if_fail (GST_IS_LOCATIONS_COMBO (object));
  priv = GST_LOCATIONS_COMBO_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_TOOL:
      priv->tool = GST_TOOL (g_value_get_object (value));
      break;
    case PROP_COMBO:
      priv->combo = GTK_WIDGET (g_value_dup_object (value));
      break;
    case PROP_SAVE:
      priv->save_button = GTK_WIDGET (g_value_dup_object (value));
      break;
    case PROP_DELETE:
      priv->delete_button = GTK_WIDGET (g_value_dup_object (value));
      break;
    }
}

static void
gst_locations_combo_get_property (GObject         *object,
				  guint            prop_id,
				  GValue          *value,
				  GParamSpec      *pspec)
{
  GstLocationsComboPrivate *priv;

  g_return_if_fail (GST_IS_LOCATIONS_COMBO (object));
  priv = GST_LOCATIONS_COMBO_GET_PRIVATE (object);

  switch (prop_id)
    {
    case PROP_TOOL:
      g_value_set_object (value, priv->tool);
      break;
    case PROP_COMBO:
      g_value_set_object (value, priv->combo);
      break;
    case PROP_SAVE:
      g_value_set_object (value, priv->save_button);
      break;
    case PROP_DELETE:
      g_value_set_object (value, priv->delete_button);
      break;
    }
}

static void
on_combo_changed (GtkWidget *widget, gpointer data)
{
  GstLocationsComboPrivate *priv;
  GstNetworkLocations *locations;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *str;

  locations = GST_NETWORK_LOCATIONS (data);
  priv = GST_LOCATIONS_COMBO (data)->_priv;
  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    {
      gtk_tree_model_get (model, &iter, 0, &str, -1);
      gst_network_locations_set_location (locations, str);
      gst_tool_update_gui (priv->tool);

      gst_tool_commit (priv->tool, locations->hosts_config);
      gst_tool_commit_async (priv->tool, locations->ifaces_config,
			     _("Changing network location"), NULL, NULL);
      g_free (str);
    }
}

static void
on_dialog_entry_changed (GtkWidget *widget, gpointer data)
{
  const gchar *text;

  text = gtk_entry_get_text (GTK_ENTRY (widget));
  gtk_widget_set_sensitive (GTK_WIDGET (data), (text && *text));
}

static gboolean
check_save_location (GstLocationsCombo *combo, const gchar *name)
{
  GstLocationsComboPrivate *priv;
  GList *names, *elem;
  gboolean found = FALSE;
  GtkWidget *dialog;
  gint response;

  priv = (GstLocationsComboPrivate *) combo->_priv;
  names = elem = gst_network_locations_get_names (GST_NETWORK_LOCATIONS (combo));

  while (elem && !found)
    {
      if (strcmp (elem->data, name) == 0)
	found = TRUE;

      elem = elem->next;
    }

  g_list_foreach (names, (GFunc) g_free, NULL);
  g_list_free (names);

  if (!found)
    return TRUE;

  dialog = gtk_message_dialog_new (GTK_WINDOW (priv->tool->main_dialog),
				   GTK_DIALOG_MODAL,
				   GTK_MESSAGE_QUESTION,
				   GTK_BUTTONS_YES_NO,
				   _("There is already a location with the same name"));
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
					    _("Overwrite it?"));
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return (response == GTK_RESPONSE_YES);
}

static void
select_matching_profile (GstLocationsCombo *combo)
{
  GstLocationsComboPrivate *priv;
  GtkTreeIter iter;
  gchar *current;
  gchar *profile;
  gboolean valid;

  priv = combo->_priv;
  valid = gtk_tree_model_get_iter_first (priv->model, &iter);
  current = gst_network_locations_get_current (GST_NETWORK_LOCATIONS (combo));

  while (valid)
    {
      gtk_tree_model_get (priv->model, &iter, 0, &profile, -1);

      if (profile && current && strcmp (profile, current) == 0)
	{
	  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->combo), &iter);
	  valid = FALSE;
	}
      else
	valid = gtk_tree_model_iter_next (priv->model, &iter);

      g_free (profile);
    }

  g_free (current);
}

static void
on_save_dialog_response (GtkWidget *widget, gint response, gpointer data)
{
  GstLocationsCombo *combo = GST_LOCATIONS_COMBO (data);
  GstLocationsComboPrivate *priv = GST_LOCATIONS_COMBO_GET_PRIVATE (combo);
  const gchar *name;

  if (response == GTK_RESPONSE_OK)
    {
      name = gtk_entry_get_text (GTK_ENTRY (priv->location_entry));

      /* check if the location already exists
       * and ask the user if necessary */
      if (check_save_location (combo, name))
	{
	  /* save the data and hide the dialog */
	  gtk_widget_hide (priv->save_dialog);
	  gst_network_locations_save_current (GST_NETWORK_LOCATIONS (combo), name);
	  select_matching_profile (combo);
	}
      else
	gtk_widget_grab_focus (priv->location_entry);
    }
  else
    gtk_widget_hide (priv->save_dialog);
}

static gboolean
on_save_dialog_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  /* block the event */
  return TRUE;
}

static void
on_save_button_clicked (GtkWidget *widget, gpointer data)
{
  GstLocationsCombo *combo;
  GstLocationsComboPrivate *priv;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *name;

  combo = GST_LOCATIONS_COMBO (data);
  priv = GST_LOCATIONS_COMBO_GET_PRIVATE (combo);

  if (!priv->save_dialog)
    {
      GtkWidget *hbox, *label, *button;

      priv->save_dialog = gtk_dialog_new_with_buttons (_("Save location"),
						       GTK_WINDOW (priv->tool->main_dialog),
						       GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,
						       NULL);
      gtk_dialog_add_button (GTK_DIALOG (priv->save_dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
      button = gtk_dialog_add_button (GTK_DIALOG (priv->save_dialog), GTK_STOCK_SAVE, GTK_RESPONSE_OK);
      gtk_dialog_set_default_response (GTK_DIALOG (priv->save_dialog), GTK_RESPONSE_OK);

      gtk_widget_set_sensitive (button, FALSE);

      g_signal_connect (G_OBJECT (priv->save_dialog), "response",
			G_CALLBACK (on_save_dialog_response), combo);
      g_signal_connect (G_OBJECT (priv->save_dialog), "delete-event",
			G_CALLBACK (on_save_dialog_delete_event), combo);

      label = gtk_label_new_with_mnemonic (_("_Location name:"));
      priv->location_entry = gtk_entry_new ();
      gtk_entry_set_activates_default (GTK_ENTRY (priv->location_entry), TRUE);
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), priv->location_entry);

      g_signal_connect (G_OBJECT (priv->location_entry), "changed",
			G_CALLBACK (on_dialog_entry_changed), button);

      hbox = gtk_hbox_new (FALSE, 6);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_box_pack_start (GTK_BOX (hbox), priv->location_entry, TRUE, TRUE, 0);

      gtk_widget_show_all (hbox);
      gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (priv->save_dialog))), hbox);
    }

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->combo));

  /* set the current location by default */
  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->combo), &iter))
    {
      gtk_tree_model_get (model, &iter, 0, &name, -1);
      gtk_entry_set_text (GTK_ENTRY (priv->location_entry), name);
      g_free (name);
    }
  else
    gtk_entry_set_text (GTK_ENTRY (priv->location_entry), "");

  gtk_widget_grab_focus (priv->location_entry);
  gtk_widget_show (priv->save_dialog);
}

static void
on_delete_button_clicked (GtkWidget *widget, gpointer data)
{
  GstLocationsComboPrivate *priv;
  GtkTreeIter iter;
  GtkWidget *dialog;
  gint response;
  gchar *name;

  priv = GST_LOCATIONS_COMBO_GET_PRIVATE (data);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->combo), &iter))
    {
      gtk_tree_model_get (priv->model, &iter, 0, &name, -1);

      dialog = gtk_message_dialog_new (GTK_WINDOW (priv->tool->main_dialog),
				       GTK_DIALOG_MODAL,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_YES_NO,
				       _("Do you want to delete location \"%s\"?"),
				       name);
      response = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      if (response == GTK_RESPONSE_YES)
	gst_network_locations_delete_location (GST_NETWORK_LOCATIONS (data), name);
    }
}

static void
fill_model (GstLocationsCombo *combo,
	    GtkTreeModel      *model)
{
  GList *names;
  GtkTreeIter iter;

  gtk_list_store_clear (GTK_LIST_STORE (model));
  names = gst_network_locations_get_names (GST_NETWORK_LOCATIONS (combo));

  while (names)
    {
      gtk_list_store_append (GTK_LIST_STORE (model), &iter);
      gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, names->data, -1);
      names = names->next;
    }

  g_list_foreach (names, (GFunc) g_free, NULL);
  g_list_free (names);
}

static GObject*
gst_locations_combo_constructor (GType                  type,
				 guint                  n_construct_properties,
				 GObjectConstructParam *construct_params)
{
  GObject *object;
  GstLocationsComboPrivate *priv;
  GtkCellRenderer *renderer;

  object = (* G_OBJECT_CLASS (gst_locations_combo_parent_class)->constructor) (type,
									       n_construct_properties,
									       construct_params);
  priv = GST_LOCATIONS_COMBO_GET_PRIVATE (object);

  gtk_combo_box_set_model (GTK_COMBO_BOX (priv->combo), priv->model);
  fill_model (GST_LOCATIONS_COMBO (object), priv->model);
  select_matching_profile (GST_LOCATIONS_COMBO (object));

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->combo), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->combo), renderer,
				  "text", 0, NULL);

  g_signal_connect (G_OBJECT (priv->combo), "changed",
		    G_CALLBACK (on_combo_changed), object);
  g_signal_connect (G_OBJECT (priv->save_button), "clicked",
		    G_CALLBACK (on_save_button_clicked), object);
  g_signal_connect (G_OBJECT (priv->delete_button), "clicked",
		    G_CALLBACK (on_delete_button_clicked), object);

  return object;
}

static void
gst_locations_combo_changed (GstNetworkLocations *locations)
{
  GstLocationsComboPrivate *priv;

  priv = GST_LOCATIONS_COMBO_GET_PRIVATE (locations);

  fill_model (GST_LOCATIONS_COMBO (locations), priv->model);
  g_signal_handlers_block_by_func (priv->combo, on_combo_changed, locations);
  select_matching_profile (GST_LOCATIONS_COMBO (locations));
  g_signal_handlers_unblock_by_func (priv->combo, on_combo_changed, locations);
}

GstLocationsCombo*
gst_locations_combo_new (GstTool   *tool,
			 GtkWidget *combo,
			 GtkWidget *save,
			 GtkWidget *delete)
{
  return g_object_new (GST_TYPE_LOCATIONS_COMBO,
		       "tool", tool,
		       "combo", combo,
		       "save", save,
		       "delete", delete,
		       NULL);
}
