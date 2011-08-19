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

#include <glib/gi18n.h>

#include "gst.h"
#include "network-tool.h"
#include "ifaces-list.h"
#include "callbacks.h"
#include "hosts.h"
#include "nm-integration.h"
#include "connection.h"

extern GstTool *tool;

void
on_table_selection_changed (GtkTreeSelection *selection, gpointer data)
{
  GtkWidget    *properties;

  properties = gst_dialog_get_widget (tool->main_dialog, "properties_button");
  gtk_widget_set_sensitive (properties, gtk_tree_selection_get_selected_rows (selection, NULL) != 0);
}

void
on_iface_properties_clicked (GtkWidget *widget, gpointer data)
{
  GstConnectionDialog *dialog;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  OobsIface *iface;

  selection = gtk_tree_view_get_selection (GST_NETWORK_TOOL (tool)->interfaces_list);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter,
			  COL_OBJECT, &iface,
			  -1);

      dialog = GST_NETWORK_TOOL (tool)->dialog;
      connection_dialog_prepare (dialog, iface);
      gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog), GTK_WINDOW (tool->main_dialog));

      gst_dialog_add_edit_dialog (tool->main_dialog, dialog->dialog);
      gtk_widget_show (dialog->dialog);
    }
}

void
on_iface_active_changed (GtkWidget *widget, gpointer data)
{
  GstConnectionDialog *dialog;
  GtkWidget *roaming_active;
  gboolean active;

  dialog = GST_NETWORK_TOOL (tool)->dialog;
  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  connection_dialog_set_sensitive (dialog, active);

  roaming_active = gst_dialog_get_widget (tool->main_dialog, "connection_device_roaming");
  g_signal_handlers_block_by_func (roaming_active, on_iface_roaming_changed, data);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (roaming_active), !active);
  g_signal_handlers_unblock_by_func (roaming_active, on_iface_roaming_changed, data);
}

void
on_iface_roaming_changed (GtkWidget *widget, gpointer data)
{
  GstConnectionDialog *dialog;
  GtkWidget *device_active;
  gboolean active;

  dialog = GST_NETWORK_TOOL (tool)->dialog;
  active = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  /* roaming changed and device active have just inverted logics */
  connection_dialog_set_sensitive (dialog, !active);

  device_active = gst_dialog_get_widget (tool->main_dialog, "connection_device_active");
  g_signal_handlers_block_by_func (device_active, on_iface_active_changed, data);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (device_active), !active);
  g_signal_handlers_unblock_by_func (device_active, on_iface_active_changed, data);
}

void
on_bootproto_changed (GtkWidget *widget, gpointer data)
{
  GstConnectionDialog *dialog;
  gchar *method;
  gboolean enabled;

  dialog = GST_NETWORK_TOOL (tool)->dialog;
  method = connection_combo_get_value (GTK_COMBO_BOX (dialog->bootproto_combo));
  enabled = (method && strcmp (method, "static") == 0);

  gtk_widget_set_sensitive (dialog->address, enabled);
  gtk_widget_set_sensitive (dialog->netmask, enabled);
  gtk_widget_set_sensitive (dialog->gateway, enabled);

  g_free (method);
}

void
on_ppp_type_changed (GtkWidget *widget, gpointer data)
{
  GstConnectionDialog *dialog;
  gchar *type;

  dialog = GST_NETWORK_TOOL (tool)->dialog;
  type = connection_combo_get_value (GTK_COMBO_BOX (dialog->ppp_type_combo));

  on_dialog_changed (widget, data);

  if (!type)
    return;

  if (strcmp (type, "modem") == 0)
    {
      gtk_widget_show (dialog->modem_page);
      gtk_widget_show (dialog->isp_frame);
      gtk_widget_show (dialog->modem_settings_table);
      gtk_widget_show (dialog->modem_isp_table);
      gtk_widget_hide (dialog->pppoe_settings_table);
      gtk_widget_hide (dialog->gprs_isp_table);
    }
  else if (strcmp (type, "isdn") == 0)
    {
      gtk_widget_show (dialog->isp_frame);
      gtk_widget_show (dialog->modem_isp_table);
      gtk_widget_hide (dialog->modem_page);
      gtk_widget_hide (dialog->gprs_isp_table);
    }
  else if (strcmp (type, "pppoe") == 0)
    {
      gtk_widget_show (dialog->modem_page);
      gtk_widget_show (dialog->pppoe_settings_table);
      gtk_widget_show (dialog->modem_isp_table);
      gtk_widget_hide (dialog->isp_frame);
      gtk_widget_hide (dialog->modem_settings_table);
    }
  else if (strcmp (type, "gprs") == 0)
    {
      gtk_widget_show (dialog->modem_page);
      gtk_widget_show (dialog->isp_frame);
      gtk_widget_show (dialog->modem_settings_table);
      gtk_widget_show (dialog->gprs_isp_table);
      gtk_widget_hide (dialog->modem_isp_table);
      gtk_widget_hide (dialog->pppoe_settings_table);
    }
}

static void
cancel_connection_dialog (GstTool *tool)
{
  GstConnectionDialog *dialog;

  dialog = GST_NETWORK_TOOL (tool)->dialog;
  g_object_unref (dialog->iface);
  connection_dialog_hide (dialog);

  if (dialog->standalone)
    gtk_main_quit ();
}

static gboolean
toggle_nm (gpointer data)
{
  GstNetworkTool *tool = GST_NETWORK_TOOL (data);
  NMState state;

  state = nm_integration_get_state (tool);

  if (state == NM_STATE_DISCONNECTED ||
      state == NM_STATE_CONNECTING ||
      state == NM_STATE_CONNECTED)
    {
      /* "reboot" NM */
      nm_integration_sleep (tool);
      nm_integration_wake (tool);
    }

  return FALSE;
}

static void
on_configuration_changed (OobsObject *object,
			  OobsResult  result,
			  gpointer    data)
{
  /* suckety suck, but NM seems not to realize
   * immediately that the interface status changed */
  g_timeout_add (2000, toggle_nm, data);
}

static void
accept_connection_dialog (GstTool *tool)
{
  GstConnectionDialog *dialog;
  GtkTreeSelection    *selection;
  GtkTreeIter          iter;

  dialog = GST_NETWORK_TOOL (tool)->dialog;
  connection_dialog_hide (dialog);

  if (dialog->changed)
    {
      connection_save (dialog);
      selection = gtk_tree_view_get_selection (GST_NETWORK_TOOL (tool)->interfaces_list);

      if (gtk_tree_selection_get_selected (selection, NULL, &iter))
        {
	  ifaces_model_modify_interface_at_iter (&iter);
	  g_signal_emit_by_name (G_OBJECT (selection), "changed");
	}

      gst_tool_commit_async (tool, OOBS_OBJECT (GST_NETWORK_TOOL (tool)->ifaces_config),
			     _("Changing interface configuration"),
			     on_configuration_changed, tool);
    }

  g_object_unref (dialog->iface);

  if (dialog->standalone)
    gtk_main_quit ();
}

void
on_connection_response (GtkWidget *widget,
			gint       response,
			gpointer   data)
{
  GstConnectionDialog *connection_dialog;
  GstDialog *dialog = GST_DIALOG (data);
  GstTool *tool = gst_dialog_get_tool (dialog);

  if (response == GTK_RESPONSE_OK)
    accept_connection_dialog (tool);
  else
    cancel_connection_dialog (tool);

  connection_dialog = GST_NETWORK_TOOL (tool)->dialog;
  gst_dialog_remove_edit_dialog (tool->main_dialog, connection_dialog->dialog);
}

void
on_detect_modem_clicked (GtkWidget *widget, gpointer data)
{
  GstNetworkTool      *network_tool;
  GstConnectionDialog *dialog;
  GtkWidget           *d;
  GdkCursor           *cursor;
  gchar               *dev = NULL;

  network_tool = GST_NETWORK_TOOL (tool);
  dialog = network_tool->dialog;

  /* give some feedback to let know the user that the tool is busy */
  gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX (dialog->serial_port)))), "");
  gtk_widget_set_sensitive (dialog->serial_port, FALSE);
  gtk_widget_set_sensitive (dialog->detect_modem, FALSE);

  cursor = gdk_cursor_new (GDK_WATCH);
  gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (dialog->dialog)), cursor);
  gdk_cursor_unref (cursor);

  dev = connection_detect_modem ();

  /* remove the user feedback */
  gtk_widget_set_sensitive (dialog->detect_modem, TRUE);
  gtk_widget_set_sensitive (dialog->serial_port, TRUE);
  gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (dialog->dialog)), NULL);

  if (!dev || !*dev)
    {
      d = gtk_message_dialog_new (GTK_WINDOW (dialog->dialog),
				  GTK_DIALOG_MODAL,
				  GTK_MESSAGE_INFO,
				  GTK_BUTTONS_CLOSE,
				  _("Could not autodetect modem device"),
				  NULL);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (d),
						_("Check that the device is not busy and "
						  "that is correctly attached to the computer."),
						NULL);
      gtk_dialog_run (GTK_DIALOG (d));
      gtk_widget_destroy (d);
    }
  else
    gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (GTK_COMBO_BOX (dialog->serial_port)))), dev);

  g_free (dev);
}

static void
do_popup_menu (GtkWidget *table, GstTablePopup *table_popup, GdkEventButton *event)
{
  gint button, event_time;

  if (!table_popup)
    return;

  if (event)
    {
      button     = event->button;
      event_time = event->time;
    }
  else
    {
      button     = 0;
      event_time = gtk_get_current_event_time ();
    }

  if (table_popup->setup)
    (table_popup->setup) (table);

  gtk_menu_popup (GTK_MENU (table_popup->popup), NULL, NULL, NULL, NULL,
		  button, event_time);
}

gboolean
on_table_button_press (GtkWidget *table, GdkEventButton *event, gpointer data)
{
  GtkTreePath      *path;
  GstTablePopup    *table_popup;
  GtkTreeSelection *selection;

  table_popup = (GstTablePopup *) data;
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));

  if (event->button == 3)
    {
      gtk_widget_grab_focus (table);

      if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (table),
					 event->x, event->y,
					 &path, NULL, NULL, NULL))
        {
	  gtk_tree_selection_unselect_all (selection);
	  gtk_tree_selection_select_path (selection, path);

	  do_popup_menu (table, table_popup, event);
	}

      return TRUE;
    }

  if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS)
    {
      if (table_popup->properties)
	(table_popup->properties) (NULL, NULL);

      return TRUE;
    }

  return FALSE;
}

gboolean
on_table_popup_menu (GtkWidget *widget, gpointer data)
{
  GstTablePopup *table_popup = (GstTablePopup *) data;

  do_popup_menu (widget, table_popup, NULL);
  return TRUE;
}

void
on_host_aliases_add_clicked (GtkWidget *widget, gpointer data)
{
  host_aliases_run_dialog (GST_NETWORK_TOOL (tool), NULL);
}

void
on_host_aliases_properties_clicked (GtkWidget *widget, gpointer data)
{
  GtkTreeSelection *selection;
  GtkTreeView      *list;
  GtkTreeModel     *model;
  GtkTreeIter       iter;

  list  = GST_NETWORK_TOOL (tool)->host_aliases_list;
  selection = gtk_tree_view_get_selection (list);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    host_aliases_run_dialog (GST_NETWORK_TOOL (tool), &iter);
}

void
on_host_aliases_delete_clicked (GtkWidget *widget, gpointer data)
{
  GtkTreeSelection *selection;
  GtkTreeView *list;
  GtkTreeModel *model;
  GtkTreeIter iter;
  OobsList *hosts_list;
  OobsListIter *list_iter;

  list  = GST_NETWORK_TOOL (tool)->host_aliases_list;
  selection = gtk_tree_view_get_selection (list);

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, COL_HOST_ITER, &list_iter, -1);
      hosts_list = oobs_hosts_config_get_static_hosts (GST_NETWORK_TOOL (tool)->hosts_config);
      oobs_list_remove (hosts_list, list_iter);
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);

      oobs_list_iter_free (list_iter);
      gst_tool_commit (tool, OOBS_OBJECT (GST_NETWORK_TOOL (tool)->hosts_config));
    }
}

void
on_host_aliases_dialog_changed (GtkWidget *widget, gpointer data)
{
  host_aliases_check_fields ();
}

void
on_dialog_changed (GtkWidget *widget, gpointer data)
{
  GstNetworkTool *network_tool = GST_NETWORK_TOOL (tool);

  network_tool->dialog->changed = TRUE;
  connection_check_fields (network_tool->dialog);
}

gboolean
on_ip_address_focus_out (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
  GstConnectionDialog *dialog = GST_NETWORK_TOOL (tool)->dialog;

  connection_check_netmask (dialog->address, dialog->netmask);
  return FALSE;
}

void
on_iface_toggled (GtkCellRendererToggle *renderer,
		  gchar                 *path_str,
		  gpointer               data)
{
  GtkTreePath *path;
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreeIter iter;
  gboolean active, inconsistent;
  OobsIface *iface;

  path = gtk_tree_path_new_from_string (path_str);

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      gtk_tree_model_get (model, &iter,
			  COL_ACTIVE, &active,
			  COL_INCONSISTENT, &inconsistent,
			  COL_OBJECT, &iface,
			  -1);

      if (!inconsistent)
	{
	  active ^= 1;

	  oobs_iface_set_active (iface, active);
	  oobs_iface_set_auto (iface, active);
	  ifaces_model_modify_interface_at_iter (&iter);

	  gst_tool_commit_async (tool, OOBS_OBJECT (GST_NETWORK_TOOL (tool)->ifaces_config),
				 _("Changing interface configuration"),
				 on_configuration_changed, tool);
	}

      g_object_unref (iface);
    }

  gtk_tree_path_free (path);
}

void
on_entry_changed (GtkWidget *widget, gpointer data)
{
  g_object_set_data (G_OBJECT (widget), "content-changed", GINT_TO_POINTER (TRUE));
}

gboolean
on_hostname_focus_out (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
  GstNetworkTool *tool;
  gboolean changed;
  const gchar *hostname;

  tool = GST_NETWORK_TOOL (gst_dialog_get_tool (GST_DIALOG (data)));
  changed = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "content-changed"));
  hostname = gtk_entry_get_text (GTK_ENTRY (widget));

  if (changed && hostname && *hostname)
    {
      GtkWidget *dialog;
      gint res;

      dialog = gtk_message_dialog_new (GTK_WINDOW (GST_TOOL (tool)->main_dialog),
				       GTK_DIALOG_MODAL,
				       GTK_MESSAGE_WARNING,
				       GTK_BUTTONS_NONE,
				       _("The host name has changed"),
				       NULL);
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						_("This will prevent you "
						  "from launching new applications, and so you will "
						  "have to log in again. Continue anyway?"),
						NULL);
      gtk_dialog_add_buttons (GTK_DIALOG (dialog),
			      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			      _("Change _Host name"), GTK_RESPONSE_ACCEPT,
			      NULL);

      res = gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);

      if (res == GTK_RESPONSE_ACCEPT)
	{
	  oobs_hosts_config_set_hostname (tool->hosts_config, hostname);
	  gst_tool_commit (GST_TOOL (tool), OOBS_OBJECT (tool->hosts_config));
	}
      else
	{
	  gtk_entry_set_text (GTK_ENTRY (widget),
			      oobs_hosts_config_get_hostname (tool->hosts_config));
	}
    }

  g_object_set_data (G_OBJECT (widget), "content-changed", GINT_TO_POINTER (FALSE));

  return FALSE;
}

gboolean
on_domain_focus_out (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
  GstNetworkTool *tool;
  gboolean changed;
  const gchar *domain;

  tool = GST_NETWORK_TOOL (gst_dialog_get_tool (GST_DIALOG (data)));
  changed = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "content-changed"));
  domain = gtk_entry_get_text (GTK_ENTRY (widget));

  if (changed)
    {
      oobs_hosts_config_set_domainname (tool->hosts_config, domain);
      gst_tool_commit (GST_TOOL (tool), OOBS_OBJECT (tool->hosts_config));
    }

  g_object_set_data (G_OBJECT (widget), "content-changed", GINT_TO_POINTER (FALSE));

  return FALSE;
}
