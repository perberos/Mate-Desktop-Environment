/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2004 Carlos Garnacho Parro.
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "address-list.h"
#include "gst-tool.h"
#include "gst-filter.h"
#include "callbacks.h"

extern GstTool *tool;

struct _GstAddressListPrivate {
  GtkTreeView       *list;
  GtkTreeViewColumn *column;
  GtkButton         *add_button;
  GtkButton         *delete_button;
  guint              type;

  GstAddressListSaveFunc save_func;
  gpointer               save_func_data;
};

static void gst_address_list_class_init (GstAddressListClass *class);
static void gst_address_list_init       (GstAddressList      *list);
static void gst_address_list_finalize   (GObject             *object);

static void gst_address_list_set_property (GObject      *object,
					   guint         prop_id,
					   const GValue *value,
					   GParamSpec   *pspec);
static void gst_address_list_get_property (GObject      *object,
					   guint         prop_id,
					   GValue       *value,
					   GParamSpec   *pspec);

static void save_address_data  (GstAddressList *list);
static void setup_treeview     (GstAddressList *list);
static void on_element_deleted (GtkWidget *widget, gpointer data);
static void on_element_added   (GtkWidget *widget, gpointer data);

static void on_editing_canceled  (GtkCellRenderer *renderer, gpointer data);
static void on_editing_started   (GtkCellRenderer *renderer,
				  GtkCellEditable *editable,
				  gchar           *path,
				  GstAddressList  *list);
static void on_editing_done      (GtkCellRenderer *renderer,
				  const gchar     *path_string,
				  const gchar     *new_text,
				  gpointer         data);


GtkActionEntry address_list_popup_menu_items [] = {
  { "Add",        GTK_STOCK_ADD,        N_("_Add"),        NULL, NULL, G_CALLBACK (on_element_added) },
  { "Delete",     GTK_STOCK_DELETE,     N_("_Delete"),     NULL, NULL, G_CALLBACK (on_element_deleted) }
};

const gchar *address_list_ui_description =
  "<ui>"
  "  <popup name='MainMenu'>"
  "    <menuitem action='Add'/>"
  "    <menuitem action='Delete'/>"
  "  </popup>"
  "</ui>";

enum {
  PROP_0,
  PROP_LIST_WIDGET,
  PROP_TYPE,
  PROP_ADD_BUTTON,
  PROP_DELETE_BUTTON
};

G_DEFINE_TYPE (GstAddressList, gst_address_list, G_TYPE_OBJECT);

GType
gst_address_type_get_type (void)
{
  static GType etype = 0;

  if (!etype)
    {
      static GEnumValue values[] =
        {
	  { GST_ADDRESS_TYPE_IP,     "GST_ADDRESS_TYPE_IP",     "IP" },
	  { GST_ADDRESS_TYPE_DOMAIN, "GST_ADDRESS_TYPE_DOMAIN", "domain" }
	};

      etype = g_enum_register_static ("GstAddressType", values);
    }

  return etype;
}

static void
gst_address_list_class_init (GstAddressListClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = gst_address_list_set_property;
  object_class->get_property = gst_address_list_get_property;
  object_class->finalize = gst_address_list_finalize;

  g_object_class_install_property (object_class,
				   PROP_TYPE,
				   g_param_spec_enum ("list-type",
						      "List type",
						      "Type of addresses that the list will contain",
						      GST_TYPE_ADDRESS_TYPE,
						      GST_ADDRESS_TYPE_IP,
						      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_LIST_WIDGET,
				   g_param_spec_object ("list-widget",
							"List widget",
							"TreeView that will display the list",
							GTK_TYPE_TREE_VIEW,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_ADD_BUTTON,
				   g_param_spec_object ("add-button",
							"Add button",
							"Button for adding addresses",
							GTK_TYPE_BUTTON,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (object_class,
				   PROP_DELETE_BUTTON,
				   g_param_spec_object ("delete-button",
							"delete button",
							"Button for deleting addresses",
							GTK_TYPE_BUTTON,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
gst_address_list_init (GstAddressList *list)
{
  g_return_if_fail (GST_IS_ADDRESS_LIST (list));

  list->_priv = g_new0 (GstAddressListPrivate, 1);
  list->_priv->type = 0;
}

static void
gst_address_list_finalize (GObject *object)
{
  GstAddressList *list;

  g_return_if_fail (GST_IS_ADDRESS_LIST (object));
  list = GST_ADDRESS_LIST (object);

  if (list->_priv)
    {
      g_object_unref (list->_priv->list);
      list->_priv->list = NULL;

      g_object_unref (list->_priv->add_button);
      list->_priv->add_button = NULL;

      g_object_unref (list->_priv->delete_button);
      list->_priv->delete_button = NULL;

      g_free (list->_priv);
      list->_priv = NULL;
    }

  if (G_OBJECT_CLASS (gst_address_list_parent_class)->finalize)
    (* G_OBJECT_CLASS (gst_address_list_parent_class)->finalize) (object);
}

static void
gst_address_list_set_property (GObject      *object,
			       guint         prop_id,
			       const GValue *value,
			       GParamSpec   *pspec)
{
  GstAddressList *list;

  g_return_if_fail (GST_IS_ADDRESS_LIST (object));
  list = GST_ADDRESS_LIST (object);

  switch (prop_id)
    {
    case PROP_TYPE:
      list->_priv->type = (GstAddressType) g_value_get_enum (value);
      break;
    case PROP_LIST_WIDGET:
      list->_priv->list = GTK_TREE_VIEW (g_object_ref (g_value_get_object (value)));
      setup_treeview (list);
      break;
    case PROP_ADD_BUTTON:
      list->_priv->add_button = GTK_BUTTON (g_object_ref (g_value_get_object (value)));
      g_signal_connect (G_OBJECT (list->_priv->add_button), "clicked",
			G_CALLBACK (on_element_added), list);
      break;
    case PROP_DELETE_BUTTON:
      list->_priv->delete_button = GTK_BUTTON (g_object_ref (g_value_get_object (value)));
      g_signal_connect (G_OBJECT (list->_priv->delete_button), "clicked",
			G_CALLBACK (on_element_deleted), list);
      break;
    }
}

static void
gst_address_list_get_property (GObject    *object,
			       guint       prop_id,
			       GValue     *value,
			       GParamSpec *pspec)
{
  GstAddressList *list;

  g_return_if_fail (GST_IS_ADDRESS_LIST (object));
  list = GST_ADDRESS_LIST (object);

  switch (prop_id)
    {
    case PROP_TYPE:
      g_value_set_enum (value, list->_priv->type);
      break;
    case PROP_LIST_WIDGET:
      g_value_set_object (value, list->_priv->list);
      break;
    case PROP_ADD_BUTTON:
      g_value_set_object (value, list->_priv->add_button);
      break;
    case PROP_DELETE_BUTTON:
      g_value_set_object (value, list->_priv->delete_button);
      break;
    }
}

static GtkWidget*
popup_menu_create (GstAddressList *list)
{
  GtkUIManager   *ui_manager;
  GtkActionGroup *action_group;
  GtkWidget      *popup;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, NULL);
  gtk_action_group_add_actions (action_group, address_list_popup_menu_items, G_N_ELEMENTS (address_list_popup_menu_items), list);

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  if (!gtk_ui_manager_add_ui_from_string (ui_manager, address_list_ui_description, -1, NULL))
    return NULL;

  g_object_set_data (G_OBJECT (list), "ui-manager", ui_manager);
  popup = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

  return popup;
}

static void
on_list_drag_end (GtkWidget      *widget,
		  GdkDragContext *context,
		  gpointer data)
{
  GstAddressList *list = (GstAddressList *) data;

  save_address_data (list);
}

static void
setup_treeview (GstAddressList *list)
{
  GtkCellRenderer *renderer;
  GtkTreeModel    *model;
  GstTablePopup   *table_popup;

  model = GTK_TREE_MODEL (gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_BOOLEAN));
  gtk_tree_view_set_model (list->_priv->list, model);
  g_object_unref (model);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "editable", TRUE, NULL);

  g_signal_connect (G_OBJECT (renderer), "editing-started",
		    G_CALLBACK (on_editing_started), list);
  g_signal_connect (G_OBJECT (renderer), "editing-canceled",
		    G_CALLBACK (on_editing_canceled), list);

  g_signal_connect (G_OBJECT (renderer), "edited",
		    G_CALLBACK (on_editing_done), list);

  list->_priv->column =
    gtk_tree_view_column_new_with_attributes ("Address",
					      renderer,
					      "text", 0,
					      NULL);

  gtk_tree_view_insert_column (list->_priv->list,
			       list->_priv->column, -1);

  table_popup = g_new0 (GstTablePopup, 1);
  table_popup->setup = NULL;
  table_popup->properties = NULL;
  table_popup->popup = popup_menu_create (list);

  g_signal_connect (G_OBJECT (list->_priv->list), "button-press-event",
		    G_CALLBACK (on_table_button_press), table_popup);
  g_signal_connect (G_OBJECT (list->_priv->list), "popup_menu",
		    G_CALLBACK (on_table_popup_menu), table_popup);
  g_signal_connect (G_OBJECT (list->_priv->list), "drag-end",
		    G_CALLBACK (on_list_drag_end), list);
}

static gboolean
check_address (GstAddressList *list, const gchar *text)
{
  gint type;

  if (list->_priv->type != GST_ADDRESS_TYPE_IP)
    return TRUE;

  type = gst_filter_check_ip_address (text);

  if (type == GST_ADDRESS_IPV4 || type == GST_ADDRESS_IPV6)
    return TRUE;

  return FALSE;
}

static void
on_editable_activate (GtkWidget *widget, GstAddressList *list)
{
  gchar *text;

  text = (gchar *) gtk_entry_get_text (GTK_ENTRY (widget));

  if (!check_address (list, text))
    g_signal_stop_emission_by_name (widget, "activate");
}

static void
on_editing_started (GtkCellRenderer *renderer,
		    GtkCellEditable *editable,
		    gchar           *path,
		    GstAddressList  *list)
{
  if (GTK_IS_ENTRY (editable))
    {
      g_signal_connect (G_OBJECT (editable), "activate",
			G_CALLBACK (on_editable_activate), list);

      if (list->_priv->type == GST_ADDRESS_TYPE_IP)
	gst_filter_init  (GTK_ENTRY (editable), GST_FILTER_IP);
    }
}

static void
on_editing_canceled (GtkCellRenderer *renderer, gpointer data)
{
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;
  GstAddressList   *list;
  gboolean          is_new_row;

  list = (GstAddressList *) data;
  selection = gtk_tree_view_get_selection (list->_priv->list);

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  gtk_tree_model_get (model, &iter, 1, &is_new_row, -1);

  if (is_new_row)
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
}

static void
save_address_data (GstAddressList *list)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean valid;
  GList *l = NULL;
  gchar *str;

  g_return_if_fail (list->_priv->save_func != NULL);

  model = gtk_tree_view_get_model (list->_priv->list);
  valid = gtk_tree_model_get_iter_first (model, &iter);

  while (valid)
    {
      gtk_tree_model_get (model, &iter, 0, &str, -1);
      l = g_list_prepend (l, str);
      valid = gtk_tree_model_iter_next (model, &iter);
    }

  l = g_list_reverse (l);
  (list->_priv->save_func) (l, list->_priv->save_func_data);
}

static void
on_editing_done (GtkCellRenderer *renderer,
		 const gchar     *path_string,
		 const gchar     *new_text,
		 gpointer         data)
{
  GtkTreeModel   *model;
  GtkTreePath    *path;
  GtkTreeIter     iter;
  GstAddressList *list;
  gboolean        is_new_row;

  list  = (GstAddressList *) data;
  model = gtk_tree_view_get_model (list->_priv->list);
  path  = gtk_tree_path_new_from_string (path_string);

  gtk_tree_model_get_iter (model, &iter, path);

  if (!check_address (list, new_text))
    {
      gtk_tree_model_get (model, &iter, 1, &is_new_row, -1);

      if (is_new_row)
	gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
    }
  else
    {
      gtk_list_store_set (GTK_LIST_STORE (model), &iter,
			  0, new_text,
			  1, FALSE,
			  -1);
      save_address_data (list);
    }
}

static void
on_element_deleted (GtkWidget *widget, gpointer data)
{
  GstAddressList   *list;
  GtkTreeSelection *selection;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  list = (GstAddressList *) data;
  selection = gtk_tree_view_get_selection (list->_priv->list);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

  save_address_data (list);
}

static void
on_element_added (GtkWidget *widget, gpointer data)
{
  GstAddressList *list;
  GtkTreeModel   *model;
  GtkTreeIter     iter;
  GtkTreePath    *path;

  list  = (GstAddressList *) data;
  model = gtk_tree_view_get_model (list->_priv->list);

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      0, _("Type address"),
		      1, TRUE,
		      -1);
  gtk_widget_grab_focus (GTK_WIDGET (list->_priv->list));

  path = gtk_tree_model_get_path (model, &iter);
  gtk_tree_view_set_cursor (list->_priv->list,
			    path,
			    list->_priv->column,
			    TRUE);
  gtk_tree_path_free (path);
}

void
gst_address_list_add_address (GstAddressList *list,
			      const gchar    *address)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  
  g_return_if_fail (list != NULL);
  g_return_if_fail (address != NULL);

  /* FIXME: check address type */

  model = gtk_tree_view_get_model (list->_priv->list);
  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      0, address,
		      1, FALSE,
		      -1);
}

GSList*
gst_address_list_get_list (GstAddressList *list)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  GSList       *l = NULL;
  gboolean      valid;
  gchar        *address;

  g_return_val_if_fail (list != NULL, NULL);

  model = gtk_tree_view_get_model (list->_priv->list);
  valid = gtk_tree_model_get_iter_first (model, &iter);

  while (valid)
    {
      gtk_tree_model_get (model, &iter, 0, &address, -1);
      l = g_slist_append (l, address);
      valid = gtk_tree_model_iter_next (model, &iter);
    }

  return l;
}

void
gst_address_list_clear (GstAddressList *list)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model (list->_priv->list);
  gtk_list_store_clear (GTK_LIST_STORE (model));
}

GstAddressList*
gst_address_list_new (GtkTreeView    *treeview,
		      GtkButton      *add_button,
		      GtkButton      *delete_button,
		      GstAddressType  list_type)
{
  GstAddressList *list;

  list = g_object_new (GST_TYPE_ADDRESS_LIST,
		       "list-widget",   GTK_WIDGET (treeview),
		       "list-type",     list_type,
		       "add-button",    add_button,
		       "delete-button", delete_button,
		       NULL);
  return list;
}

void
gst_address_list_set_save_func (GstAddressList         *address_list,
				GstAddressListSaveFunc  save_func,
				gpointer                save_func_data)
{
  g_return_if_fail (address_list != NULL);

  address_list->_priv->save_func = save_func;
  address_list->_priv->save_func_data = save_func_data;
}
