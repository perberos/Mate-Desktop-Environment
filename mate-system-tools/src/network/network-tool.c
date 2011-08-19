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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "gst.h"
#include "network-tool.h"
#include "ifaces-list.h"
#include "connection.h"
#include "callbacks.h"
#include "hosts.h"
#include "locations-combo.h"

static void gst_network_tool_class_init (GstNetworkToolClass *class);
static void gst_network_tool_init       (GstNetworkTool      *tool);
static void gst_network_tool_finalize   (GObject             *object);

static GObject* gst_network_tool_constructor (GType                  type,
					      guint                  n_construct_properties,
					      GObjectConstructParam *construct_params);

static void gst_network_tool_update_gui    (GstTool *tool);


G_DEFINE_TYPE (GstNetworkTool, gst_network_tool, GST_TYPE_TOOL);

static void
gst_network_tool_class_init (GstNetworkToolClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GstToolClass *tool_class = GST_TOOL_CLASS (class);

  object_class->constructor = gst_network_tool_constructor;
  object_class->finalize = gst_network_tool_finalize;
  tool_class->update_gui = gst_network_tool_update_gui;
}

static void
gst_network_tool_init (GstNetworkTool *tool)
{
  tool->hosts_config = OOBS_HOSTS_CONFIG (oobs_hosts_config_get ());
  gst_tool_add_configuration_object (GST_TOOL (tool), OOBS_OBJECT (tool->hosts_config), TRUE);
  tool->ifaces_config = OOBS_IFACES_CONFIG (oobs_ifaces_config_get ());
  gst_tool_add_configuration_object (GST_TOOL (tool), OOBS_OBJECT (tool->ifaces_config), TRUE);

  tool->bus_connection = dbus_bus_get (DBUS_BUS_SYSTEM, NULL);

  g_signal_connect_swapped (tool->ifaces_config, "changed",
			    G_CALLBACK (gst_tool_update_async), tool);
}

static void
gst_network_tool_finalize (GObject *object)
{
  GstNetworkTool *tool;

  g_return_if_fail (GST_IS_NETWORK_TOOL (object));

  tool = GST_NETWORK_TOOL (object);

  g_object_unref (tool->dns);
  g_object_unref (tool->search);
  g_object_unref (tool->interfaces_model);
  g_object_unref (tool->location);
  g_free (tool->dialog);

  (* G_OBJECT_CLASS (gst_network_tool_parent_class)->finalize) (object);
}

static void
save_dns (GList *list, gpointer data)
{
  GstNetworkTool *tool = (GstNetworkTool *) data;

  oobs_hosts_config_set_dns_servers (tool->hosts_config, list);
  gst_tool_commit (GST_TOOL (tool), OOBS_OBJECT (tool->hosts_config));
}

static void
save_search_domains (GList *list, gpointer data)
{
  GstNetworkTool *tool = (GstNetworkTool *) data;

  oobs_hosts_config_set_search_domains (tool->hosts_config, list);
  gst_tool_commit (GST_TOOL (tool), OOBS_OBJECT (tool->hosts_config));
}

static GObject*
gst_network_tool_constructor (GType                  type,
			      guint                  n_construct_properties,
			      GObjectConstructParam *construct_params)
{
  GObject *object;
  GstNetworkTool *tool;
  GtkWidget *widget, *add_button, *delete_button;

  object = (* G_OBJECT_CLASS (gst_network_tool_parent_class)->constructor) (type,
									    n_construct_properties,
									    construct_params);
  tool = GST_NETWORK_TOOL (object);

  widget = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "dns_list");
  add_button = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "dns_list_add");
  delete_button = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "dns_list_delete");
  tool->dns = gst_address_list_new (GTK_TREE_VIEW (widget),
				    GTK_BUTTON (add_button),
				    GTK_BUTTON (delete_button),
				    GST_ADDRESS_TYPE_IP);
  gst_address_list_set_save_func (tool->dns, save_dns, tool);

  widget = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "search_domain_list");
  add_button = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "search_domain_add");
  delete_button = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "search_domain_delete");
  tool->search = gst_address_list_new (GTK_TREE_VIEW (widget),
				       GTK_BUTTON (add_button),
				       GTK_BUTTON (delete_button),
				       GST_ADDRESS_TYPE_DOMAIN);
  gst_address_list_set_save_func (tool->search, save_search_domains, tool);

  widget = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "hostname");
  tool->hostname = GTK_ENTRY (widget);

  widget = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "domain");
  tool->domain = GTK_ENTRY (widget);

  tool->interfaces_model = ifaces_model_create ();
  tool->interfaces_list = ifaces_list_create (GST_TOOL (tool));
  tool->host_aliases_list = host_aliases_list_create (GST_TOOL (tool));

  widget = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "locations_combo");
  add_button = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "add_location");
  delete_button = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "remove_location");
  tool->location = gst_locations_combo_new (GST_TOOL (tool), widget, add_button, delete_button);

  tool->dialog = connection_dialog_init (GST_TOOL (tool));
  tool->host_aliases_dialog = gst_dialog_get_widget (GST_TOOL (tool)->main_dialog, "host_aliases_edit_dialog");

  return object;
}

static void
update_address_list (GstAddressList *address_list,
		     GList          *list)
{
  gst_address_list_clear (address_list);

  while (list)
    {
      gst_address_list_add_address (address_list, (const gchar*) list->data);
      list = list->next;
    }
}

static void
update_hosts_list (OobsList *list)
{
  GObject *host;
  OobsListIter iter;
  gboolean valid;

  host_aliases_clear ();
  valid = oobs_list_get_iter_first (list, &iter);

  while (valid)
    {
      host = oobs_list_get (list, &iter);
      host_aliases_add (OOBS_STATIC_HOST (host), &iter);
      g_object_unref (host);

      valid = oobs_list_iter_next (list, &iter);
    }
}

static void
add_interfaces (GtkTreeView *ifaces_list, OobsList *list)
{
  OobsListIter iter;
  GObject *iface;
  gboolean valid;
  gint n_items;

  valid = oobs_list_get_iter_first (list, &iter);
  n_items = oobs_list_get_n_items (list);

  while (valid)
    {
      iface = oobs_list_get (list, &iter);
      ifaces_model_add_interface (OOBS_IFACE (iface), (n_items > 1));

      g_object_unref (iface);
      valid = oobs_list_iter_next (list, &iter);
    }
}

static void
add_all_interfaces (GstNetworkTool *network_tool)
{
  OobsList *ifaces_list;

  ifaces_list = oobs_ifaces_config_get_ifaces (network_tool->ifaces_config, OOBS_IFACE_TYPE_ETHERNET);
  add_interfaces (network_tool->interfaces_list, ifaces_list);

  ifaces_list = oobs_ifaces_config_get_ifaces (network_tool->ifaces_config, OOBS_IFACE_TYPE_WIRELESS);
  add_interfaces (network_tool->interfaces_list, ifaces_list);

  ifaces_list = oobs_ifaces_config_get_ifaces (network_tool->ifaces_config, OOBS_IFACE_TYPE_IRLAN);
  add_interfaces (network_tool->interfaces_list, ifaces_list);

  ifaces_list = oobs_ifaces_config_get_ifaces (network_tool->ifaces_config, OOBS_IFACE_TYPE_PLIP);
  add_interfaces (network_tool->interfaces_list, ifaces_list);

  ifaces_list = oobs_ifaces_config_get_ifaces (network_tool->ifaces_config, OOBS_IFACE_TYPE_PPP);
  add_interfaces (network_tool->interfaces_list, ifaces_list);
}

static void
set_entry_text (GtkWidget *entry, const gchar *text)
{
  gtk_entry_set_text (GTK_ENTRY (entry), (text) ? text : "");
}

static void
gst_network_tool_update_gui (GstTool *tool)
{
  GstNetworkTool *network_tool;
  GList *dns, *search_domains;
  OobsList *hosts_list;

  network_tool = GST_NETWORK_TOOL (tool);

  dns = oobs_hosts_config_get_dns_servers (network_tool->hosts_config);
  update_address_list (network_tool->dns, dns);
  g_list_free (dns);

  search_domains = oobs_hosts_config_get_search_domains (network_tool->hosts_config);
  update_address_list (network_tool->search, search_domains);
  g_list_free (search_domains);

  hosts_list = oobs_hosts_config_get_static_hosts (network_tool->hosts_config);
  update_hosts_list (hosts_list);

  g_signal_handlers_block_by_func (network_tool->hostname, on_entry_changed, tool->main_dialog);
  set_entry_text (GTK_WIDGET (network_tool->hostname),
		  oobs_hosts_config_get_hostname (network_tool->hosts_config));
  g_signal_handlers_unblock_by_func (network_tool->hostname, on_entry_changed, tool->main_dialog);

  g_signal_handlers_block_by_func (network_tool->domain, on_entry_changed, tool->main_dialog);
  set_entry_text (GTK_WIDGET (network_tool->domain),
		  oobs_hosts_config_get_domainname (network_tool->hosts_config));
  g_signal_handlers_unblock_by_func (network_tool->domain, on_entry_changed, tool->main_dialog);

  gtk_list_store_clear (GTK_LIST_STORE (network_tool->interfaces_model));
  add_all_interfaces (network_tool);

  connection_dialog_update (network_tool->dialog);
}

GstTool*
gst_network_tool_new (void)
{
  return g_object_new (GST_TYPE_NETWORK_TOOL,
		       "name", "network",
		       "title", _("Network Settings"),
		       "icon", "preferences-system-network",
		       NULL);
}
