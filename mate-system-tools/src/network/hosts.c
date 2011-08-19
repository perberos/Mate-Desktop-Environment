/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* Copyright (C) 2004 Carlos Garnacho
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

#include <string.h>
#include <glib/gi18n.h>
#include "hosts.h"
#include "gst.h"
#include "network-tool.h"
#include "callbacks.h"

extern GstTool *tool;

GtkActionEntry hosts_popup_menu_items [] = {
  { "Add",        GTK_STOCK_ADD,        N_("_Add"),        NULL, NULL, G_CALLBACK (on_host_aliases_add_clicked) },
  { "Properties", GTK_STOCK_PROPERTIES, N_("_Properties"), NULL, NULL, G_CALLBACK (on_host_aliases_properties_clicked) },
  { "Delete",     GTK_STOCK_DELETE,     N_("_Delete"),     NULL, NULL, G_CALLBACK (on_host_aliases_delete_clicked) }
};

const gchar *hosts_ui_description =
  "<ui>"
  "  <popup name='MainMenu'>"
  "    <menuitem action='Add'/>"
  "    <separator/>"
  "    <menuitem action='Properties'/>"
  "    <menuitem action='Delete'/>"
  "  </popup>"
  "</ui>";

static GtkTreeModel*
host_aliases_model_create (void)
{
  GtkListStore *store;

  store = gtk_list_store_new (COL_HOST_LAST,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_OBJECT,
			      OOBS_TYPE_LIST_ITER);
  return GTK_TREE_MODEL (store);
}

static GtkWidget*
popup_menu_create (GtkWidget *widget)
{
  GtkUIManager   *ui_manager;
  GtkActionGroup *action_group;
  GtkWidget      *popup;

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, NULL);
  gtk_action_group_add_actions (action_group, hosts_popup_menu_items, G_N_ELEMENTS (hosts_popup_menu_items), widget);

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  if (!gtk_ui_manager_add_ui_from_string (ui_manager, hosts_ui_description, -1, NULL))
    return NULL;

  g_object_set_data (G_OBJECT (widget), "ui-manager", ui_manager);
  popup = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

  return popup;
}

static void
add_list_columns (GtkTreeView *list)
{
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (list, -1,
					       _("IP Address"),
					       renderer,
					       "text", COL_HOST_IP,
					       NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_insert_column_with_attributes (list, -1,
					       _("Aliases"),
					       renderer,
					       "text", COL_HOST_ALIASES,
					       NULL);
}

GtkTreeView*
host_aliases_list_create (GstTool *tool)
{
  GtkWidget     *list;
  GstTablePopup *table_popup;
  GtkTreeModel  *model;

  list = gst_dialog_get_widget (tool->main_dialog, "host_aliases_list");

  model = host_aliases_model_create ();
  gtk_tree_view_set_model (GTK_TREE_VIEW (list), model);
  g_object_unref (model);

  add_list_columns (GTK_TREE_VIEW (list));

  table_popup = g_new0 (GstTablePopup, 1);
  table_popup->setup = NULL;
  table_popup->properties = on_host_aliases_properties_clicked;
  table_popup->popup = popup_menu_create (list);

  g_signal_connect (G_OBJECT (list), "button-press-event",
		    G_CALLBACK (on_table_button_press), (gpointer) table_popup);
  g_signal_connect (G_OBJECT (list), "popup_menu",
		    G_CALLBACK (on_table_popup_menu), (gpointer) table_popup);

  return GTK_TREE_VIEW (list);
}

static gchar*
concatenate_aliases (GList *aliases, gchar *sep)
{
  GString *str = NULL;
  gchar *ret_str;

  g_return_val_if_fail (aliases != NULL, NULL);

  while (aliases)
    {
      if (!str)
	str = g_string_new ((gchar*) aliases->data);
      else
	g_string_append_printf (str, "%s%s", sep, (gchar*) aliases->data);

      aliases = aliases->next;
    }

  ret_str = str->str;
  g_string_free (str, FALSE);

  return ret_str;
}

static void
host_aliases_modify_at_iter (GtkTreeIter    *iter,
			     OobsStaticHost *host,
			     OobsListIter   *list_iter)
{
  GtkTreeView  *list;
  GtkTreeModel *model;
  gchar *aliases_str;
  GList *aliases;

  list = GST_NETWORK_TOOL (tool)->host_aliases_list;
  model = gtk_tree_view_get_model (list);

  aliases = oobs_static_host_get_aliases (host);
  aliases_str = concatenate_aliases (aliases, " ");
  g_list_free (aliases);

  gtk_list_store_set (GTK_LIST_STORE (model), iter,
		      COL_HOST_IP, oobs_static_host_get_ip_address (host),
		      COL_HOST_ALIASES, aliases_str,
		      COL_HOST_OBJECT, host,
		      COL_HOST_ITER, list_iter,
		      -1);

  g_free (aliases_str);
}

void
host_aliases_add (OobsStaticHost *host, OobsListIter *list_iter)
{
  GtkTreeView  *list;
  GtkTreeModel *model;
  GtkTreeIter   iter;

  list = GST_NETWORK_TOOL (tool)->host_aliases_list;
  model = gtk_tree_view_get_model (list);

  gtk_list_store_append (GTK_LIST_STORE (model), &iter);
  host_aliases_modify_at_iter (&iter, host, list_iter);
}

void
host_aliases_clear (void)
{
  GtkTreeView  *list;
  GtkTreeModel *model;

  list = GST_NETWORK_TOOL (tool)->host_aliases_list;
  model = gtk_tree_view_get_model (list);

  gtk_list_store_clear (GTK_LIST_STORE (model));
}

void
host_aliases_check_fields (void)
{
  GtkWidget     *address, *aliases, *ok_button;
  GtkTextBuffer *buffer;
  GtkTextIter    start, end;
  gboolean       valid;
  const gchar   *addr;
  gchar         *str;
  gint           address_type;

  address   = gst_dialog_get_widget (tool->main_dialog, "host_alias_address");
  aliases   = gst_dialog_get_widget (tool->main_dialog, "host_alias_list");
  ok_button = gst_dialog_get_widget (tool->main_dialog, "host_alias_ok_button");

  buffer  = gtk_text_view_get_buffer (GTK_TEXT_VIEW (aliases));
  gtk_text_buffer_get_bounds (buffer, &start, &end);
  str = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);

  addr = gtk_entry_get_text (GTK_ENTRY (address));
  address_type = gst_filter_check_ip_address (addr);

  valid = ((address_type == GST_ADDRESS_IPV4 || address_type == GST_ADDRESS_IPV6)
	   && (str && *str));
  gtk_widget_set_sensitive (ok_button, valid);

  g_free (str);
}

static void
host_aliases_dialog_prepare (OobsStaticHost *host)
{
  GtkWidget *address, *aliases;
  GtkTextBuffer *buffer;
  GList *alias_list;
  gchar *alias_str;

  address = gst_dialog_get_widget (tool->main_dialog, "host_alias_address");
  aliases = gst_dialog_get_widget (tool->main_dialog, "host_alias_list");
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (aliases));

  if (host)
    {
      gtk_entry_set_text (GTK_ENTRY (address),
			  oobs_static_host_get_ip_address (host));

      alias_list = oobs_static_host_get_aliases (host);
      alias_str = concatenate_aliases (alias_list, "\n");
      g_list_free (alias_list);

      gtk_text_buffer_set_text (buffer, alias_str, -1);
      g_free (alias_str);
    }
  else
    {
      gtk_entry_set_text (GTK_ENTRY (address), "");
      gtk_text_buffer_set_text (buffer, "", -1);
    }

  host_aliases_check_fields ();
}

static GList*
split_buffer_content (GtkTextBuffer *buffer)
{
  GtkTextIter start, end;
  gchar *str, **arr;
  GList *list = NULL;
  gint i = 0;

  gtk_text_buffer_get_bounds (buffer, &start, &end);
  str = gtk_text_buffer_get_text (buffer, &start, &end, TRUE);
  arr = g_strsplit_set (str, " \n", -1);
  g_free (str);

  while (arr[i])
    {
      if (arr[i] && *arr[i])
	list = g_list_prepend (list, g_strdup (arr[i]));

      i++;
    }

  list = g_list_reverse (list);
  g_strfreev (arr);
  return list;
}

static void
host_aliases_dialog_save (GtkTreeIter *iter)
{
  GtkTreeView *list;
  GtkTreeModel *model;
  GtkWidget *address, *aliases;
  GtkTextBuffer *buffer;
  OobsStaticHost *host;
  GList *aliases_list;
  
  address = gst_dialog_get_widget (tool->main_dialog, "host_alias_address");
  aliases = gst_dialog_get_widget (tool->main_dialog, "host_alias_list");
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (aliases));

  aliases_list = split_buffer_content (buffer);

  if (iter)
    {
      OobsListIter *list_iter;

      list = GST_NETWORK_TOOL (tool)->host_aliases_list;
      model = gtk_tree_view_get_model (list);

      gtk_tree_model_get (model, iter,
			  COL_HOST_OBJECT, &host,
			  COL_HOST_ITER, &list_iter,
			  -1);

      oobs_static_host_set_ip_address (host, gtk_entry_get_text (GTK_ENTRY (address)));
      oobs_static_host_set_aliases (host, aliases_list);
      host_aliases_modify_at_iter (iter, host, list_iter);

      oobs_list_iter_free (list_iter);
      g_object_unref (host);
    }
  else
    {
      OobsList *list;
      OobsListIter list_iter;

      list = oobs_hosts_config_get_static_hosts (GST_NETWORK_TOOL (tool)->hosts_config);
      host = oobs_static_host_new (gtk_entry_get_text (GTK_ENTRY (address)), aliases_list);

      oobs_list_append (list, &list_iter);
      oobs_list_set (list, &list_iter, host);

      host_aliases_add (host, &list_iter);
    }

    gst_tool_commit (tool, OOBS_OBJECT (GST_NETWORK_TOOL (tool)->hosts_config));
}

void
host_aliases_run_dialog (GstNetworkTool *network_tool,
			 GtkTreeIter    *iter)
{
  GstTool *tool;
  GtkWidget *dialog;
  GtkTreeView *list;
  GtkTreeModel *model;
  gint response;
  OobsStaticHost *host = NULL;

  if (iter)
    {
      list = network_tool->host_aliases_list;
      model = gtk_tree_view_get_model (list);

      gtk_tree_model_get (model, iter,
			  COL_HOST_OBJECT, &host,
			  -1);
    }

  tool = GST_TOOL (network_tool);
  dialog = network_tool->host_aliases_dialog;
  host_aliases_dialog_prepare (host);

  gst_dialog_add_edit_dialog (tool->main_dialog, dialog);

  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (GST_TOOL (tool)->main_dialog));
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);

  gst_dialog_remove_edit_dialog (tool->main_dialog, dialog);

  if (response == GTK_RESPONSE_OK)
    host_aliases_dialog_save (iter);

  if (host)
    g_object_unref (host);
}
