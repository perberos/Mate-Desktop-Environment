/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*- */
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdlib.h>
#include <glib/gi18n.h>

#include "gst.h"
#include "network-tool.h"
#include "ifaces-list.h"
#include "callbacks.h"

GstTool *tool;

static GstDialogSignal signals[] = {
  /* connections tab */
  { "properties_button",            "clicked", G_CALLBACK (on_iface_properties_clicked) },
  /* general tab */
  { "domain",                       "focus-out-event", G_CALLBACK (on_domain_focus_out) },
  { "hostname",                     "changed", G_CALLBACK (on_entry_changed) },
  { "domain",                       "changed", G_CALLBACK (on_entry_changed) },
  /* host aliases tab */
  { "host_aliases_add",             "clicked", G_CALLBACK (on_host_aliases_add_clicked) },
  { "host_aliases_properties",      "clicked", G_CALLBACK (on_host_aliases_properties_clicked) },
  { "host_aliases_delete",          "clicked", G_CALLBACK (on_host_aliases_delete_clicked) },
  /* host aliases dialog */
  { "host_alias_address",           "changed", G_CALLBACK (on_host_aliases_dialog_changed) },
  /* connection dialog */
  { "connection_config_dialog",     "response", G_CALLBACK (on_connection_response) },
  { "connection_config_dialog",     "delete-event", G_CALLBACK (gtk_true) },
  { "connection_device_active",     "clicked", G_CALLBACK (on_iface_active_changed) },
  { "connection_device_roaming",    "clicked", G_CALLBACK (on_iface_roaming_changed) },
  { "connection_bootproto",         "changed", G_CALLBACK (on_bootproto_changed) },
  { "connection_detect_modem",      "clicked", G_CALLBACK (on_detect_modem_clicked) },
  /* dialog changing detection */
  { "connection_address",           "focus-out-event", G_CALLBACK (on_ip_address_focus_out) },
  { "connection_ppp_type",          "changed", G_CALLBACK (on_ppp_type_changed) },
  { "connection_device_active",     "toggled", G_CALLBACK (on_dialog_changed) },
  { "connection_essid",             "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_wep_key_type",      "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_wep_key",           "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_bootproto",         "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_address",           "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_netmask",           "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_gateway",           "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_local_address",     "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_remote_address",    "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_serial_port",       "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_dial_type",         "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_volume",            "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_phone_number",      "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_dial_prefix",       "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_login",             "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_password",          "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_default_gw",        "toggled", G_CALLBACK (on_dialog_changed) },
  { "connection_persist",           "toggled", G_CALLBACK (on_dialog_changed) },
  { "connection_apn",               "changed", G_CALLBACK (on_dialog_changed) },
  { "connection_pppoe_ethernet",    "changed", G_CALLBACK (on_dialog_changed) },
  { NULL }
};

static GstDialogSignal signals_after[] = {
  { "hostname",                     "focus-out-event", G_CALLBACK (on_hostname_focus_out) },
  { NULL }
};

static const gchar *policy_widgets [] = {
	"locations_combo",
	"add_location",
	"remove_location",
	"interfaces_list",
	"properties_button",
	"hostname",
	"domain",
	"dns_list",
	"dns_list_add",
	"dns_list_delete",
	"search_domain_list",
	"search_domain_add",
	"search_domain_delete",
	"host_aliases_list",
	"host_aliases_add",
	"host_aliases_properties",
	"host_aliases_delete",
	NULL
};

static void
init_standalone_dialog (GstTool         *tool,
			IfaceSearchTerm  search_term,
			const gchar     *term)
{
  GstNetworkTool *network_tool;
  OobsIface      *iface;
  GtkWidget      *d;

  network_tool = GST_NETWORK_TOOL (tool);
  gst_tool_update_gui (tool);
  iface = ifaces_model_search_iface (search_term, term);

  if (iface)
    {
      connection_dialog_prepare (network_tool->dialog, iface);
      network_tool->dialog->standalone = TRUE;
      g_object_unref (iface);

      gtk_widget_show (network_tool->dialog->dialog);
    }
  else
    {
      d = gtk_message_dialog_new (GTK_WINDOW (tool->main_dialog),
                                  GTK_DIALOG_MODAL,
                                  GTK_MESSAGE_WARNING,
                                  GTK_BUTTONS_CLOSE,
                                  _("The interface does not exist"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (d),
                                                _("Check that it is correctly typed "
                                                  "and that it is correctly supported "
                                                  "by your system."),
                                                NULL);
      gtk_dialog_run (GTK_DIALOG (d));
      gtk_widget_destroy (d);
      exit (-1);
    }
}

static void
init_filters (void)
{
  gst_filter_init (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "host_alias_address")), GST_FILTER_IP);

  gst_filter_init (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "connection_address")), GST_FILTER_IPV4);
  gst_filter_init (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "connection_netmask")), GST_FILTER_IPV4);
  gst_filter_init (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "connection_gateway")), GST_FILTER_IPV4);

  gst_filter_init (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "connection_local_address")), GST_FILTER_IPV4);
  gst_filter_init (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "connection_remote_address")), GST_FILTER_IPV4);

  gst_filter_init (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "connection_phone_number")), GST_FILTER_PHONE);
  gst_filter_init (GTK_ENTRY (gst_dialog_get_widget (tool->main_dialog, "connection_dial_prefix")), GST_FILTER_PHONE);
}

static void
set_text_buffers_callback (void)
{
  GtkWidget *textview;
  GtkTextBuffer *buffer;

  textview = gst_dialog_get_widget (tool->main_dialog, "host_alias_list");
  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (textview));

  g_signal_connect (G_OBJECT (buffer), "changed",
		    G_CALLBACK (on_host_aliases_dialog_changed), NULL);
}

int
main (int argc, gchar *argv[])
{
  gchar *interface = NULL;
  gchar *type = NULL;

  GOptionEntry entries[] = {
    { "configure",      'c', 0, G_OPTION_ARG_STRING, &interface, N_("Configure a network interface"), N_("INTERFACE") },
    { "configure-type", 't', 0, G_OPTION_ARG_STRING, &type,      N_("Configure the first network interface with a specific type"), N_("TYPE") },
    { NULL }
  };

  g_thread_init (NULL);
  gst_init_tool ("network-admin", argc, argv, entries);
  tool = gst_network_tool_new ();

  gst_dialog_require_authentication_for_widgets (tool->main_dialog, policy_widgets);
  gst_dialog_connect_signals (tool->main_dialog, signals);
  gst_dialog_connect_signals_after (tool->main_dialog, signals_after);
  set_text_buffers_callback ();
  init_filters ();

  if (interface)
    init_standalone_dialog (tool, SEARCH_DEV, interface);
  else if (type)
    init_standalone_dialog (tool, SEARCH_TYPE, type);
  else
    gtk_widget_show (GTK_WIDGET (tool->main_dialog));

  gtk_main ();
  return 0;
}
