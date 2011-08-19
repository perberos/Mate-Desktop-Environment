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
#include <gdk/gdk.h>
#include "gst.h"
#include "callbacks.h"
#include "ifaces-list.h"
#include "network-tool.h"
#include "nm-integration.h"

extern GstTool *tool;

GtkActionEntry popup_menu_items [] = {
  { "Properties",  GTK_STOCK_PROPERTIES, N_("_Properties"), NULL, NULL, G_CALLBACK (on_iface_properties_clicked) },
};

const gchar *ui_description =
  "<ui>"
  "  <popup name='MainMenu'>"
  "    <menuitem action='Properties'/>"
  "  </popup>"
  "</ui>";

GtkTreeModel*
ifaces_model_create (void)
{
  GtkListStore *store;

  store = gtk_list_store_new (COL_LAST,
			      G_TYPE_BOOLEAN,
			      GDK_TYPE_PIXBUF,
			      G_TYPE_STRING,
			      G_TYPE_OBJECT,
			      G_TYPE_STRING,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN);
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
  gtk_action_group_add_actions (action_group, popup_menu_items, G_N_ELEMENTS (popup_menu_items), widget);

  ui_manager = gtk_ui_manager_new ();
  gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);

  if (!gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL))
    return NULL;

  g_object_set_data (G_OBJECT (widget), "ui-manager", ui_manager);
  popup = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");

  return popup;
}

static gint
iface_to_priority (OobsIface *iface)
{
  if (OOBS_IS_IFACE_WIRELESS (iface))
    return 5;
  else if (OOBS_IS_IFACE_IRLAN (iface))
    return 2;
  else if (OOBS_IS_IFACE_ETHERNET (iface))
    return 4;
  else if (OOBS_IS_IFACE_PPP (iface))
    return 3;
  else
    return 1;
}

static gint
ifaces_list_sort (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer data)
{
  OobsIface *iface_a, *iface_b;

  gtk_tree_model_get (model, a, COL_OBJECT, &iface_a, -1);
  gtk_tree_model_get (model, b, COL_OBJECT, &iface_b, -1);

  return (iface_to_priority (iface_a) - iface_to_priority (iface_b));
}

static void
add_list_columns (GtkTreeView  *table,
		  GtkTreeModel *model)
{
  GtkCellRenderer *renderer;

  renderer = gtk_cell_renderer_toggle_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), -1,
					       "Enable",
					       renderer,
					       "active", COL_ACTIVE,
					       "inconsistent", COL_INCONSISTENT,
					       NULL);
  g_signal_connect (G_OBJECT (renderer), "toggled",
		    G_CALLBACK (on_iface_toggled), model);
  
  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), -1,
					       "Interface",
					       renderer,
					       "pixbuf", COL_IMAGE,
					       "sensitive", COL_NOT_INCONSISTENT,
					       NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  
  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (table), -1,
					       "Interface",
					       renderer,
					       "markup", COL_DESC,
					       NULL);
}

GtkTreeView*
ifaces_list_create (GstTool *tool)
{
  GtkWidget        *table = gst_dialog_get_widget (tool->main_dialog, "interfaces_list");
  GstTablePopup    *table_popup;
  GtkTreeSelection *selection;
  GtkTreeModel     *model;

  model = GST_NETWORK_TOOL (tool)->interfaces_model;
  gtk_tree_view_set_model (GTK_TREE_VIEW (table), model);

  gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (GTK_LIST_STORE (model)),
					   ifaces_list_sort,
					   NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (GTK_LIST_STORE (model)),
					GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					GTK_SORT_DESCENDING);

  add_list_columns (GTK_TREE_VIEW (table), model);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (table));
  g_signal_connect (G_OBJECT (selection), "changed",
		    G_CALLBACK (on_table_selection_changed), NULL);

  table_popup = g_new0 (GstTablePopup, 1);
  table_popup->properties = on_iface_properties_clicked;
  table_popup->popup = popup_menu_create (table);

  g_signal_connect (G_OBJECT (table), "button-press-event",
		    G_CALLBACK (on_table_button_press), (gpointer) table_popup);
  g_signal_connect (G_OBJECT (table), "popup_menu",
		    G_CALLBACK (on_table_popup_menu), (gpointer) table_popup);

  return GTK_TREE_VIEW (table);
}

const gchar*
iface_to_type (OobsIface *iface)
{
  if (OOBS_IS_IFACE_WIRELESS (iface))
    return "wireless";
  else if (OOBS_IS_IFACE_IRLAN (iface))
    return "irlan";
  else if (OOBS_IS_IFACE_ETHERNET (iface))
    return "ethernet";
  else if (OOBS_IS_IFACE_PLIP (iface))
    return "plip";
  else if (OOBS_IS_IFACE_PPP (iface))
    return "ppp";

  return "unknown";
}

OobsIface*
ifaces_model_search_iface (IfaceSearchTerm search_term, const gchar *term)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gboolean      valid;
  gchar        *dev, *item;
  OobsIface    *iface = NULL;

  g_return_val_if_fail (term != NULL, NULL);

  model = GST_NETWORK_TOOL (tool)->interfaces_model;
  valid = gtk_tree_model_get_iter_first (model, &iter);

  while (valid)
    {
      gtk_tree_model_get (model, &iter,
			  COL_OBJECT, &iface,
			  COL_DEV,    &dev,
			  -1);

      if (search_term == SEARCH_DEV)
	item = dev;
      else
	item = (gchar *) iface_to_type (iface);

      if (strcmp (term, item) == 0)
	valid = FALSE;
      else
        {
          valid = gtk_tree_model_iter_next (model, &iter);
	  g_object_unref (iface);
	  iface = NULL;
	}

      g_free (dev);
    }

  return iface;
}

static GdkPixbuf*
get_iface_pixbuf (OobsIface *iface)
{
  GdkPixbuf *pixbuf = NULL;
  const gchar *icon = NULL;

  if (OOBS_IS_IFACE_WIRELESS (iface))
    icon = "mate-dev-wavelan";
  else if (OOBS_IS_IFACE_IRLAN (iface))
    icon = "irda";
  else if (OOBS_IS_IFACE_ETHERNET (iface))
    icon = "mate-dev-ethernet";
  else if (OOBS_IS_IFACE_PLIP (iface))
    icon = "plip";
  else if (OOBS_IS_IFACE_PPP (iface))
    icon = "mate-modem";

  if (icon)
    pixbuf = gtk_icon_theme_load_icon (gst_tool_get_icon_theme (tool),
				       icon, 48, 0, NULL);

  /* fallback to a "generic" icon */
  if (!pixbuf)
    pixbuf = gtk_icon_theme_load_icon (gst_tool_get_icon_theme (tool),
				       "preferences-system-network", 48, 0, NULL);
  return pixbuf;
}

static gchar*
get_iface_secondary_text (OobsIface *iface)
{
  GString *str;
  gchar *text;
  NMState state;

  str = g_string_new ("");
  state = nm_integration_get_state (GST_NETWORK_TOOL (tool));

  if (!oobs_iface_get_configured (iface))
    {
      if (!nm_integration_iface_supported (iface) ||
	  state == NM_STATE_UNKNOWN ||
	  state == NM_STATE_ASLEEP)
	str = g_string_append (str, _("This network interface is not configured"));
      else
	str = g_string_append (str, _("Roaming mode enabled"));
    }
  else if (OOBS_IS_IFACE_ETHERNET (iface))
    {
      const gchar *config_method;

      if (OOBS_IS_IFACE_WIRELESS (iface))
	g_string_append_printf (str, _("<b>Essid:</b> %s "),
				oobs_iface_wireless_get_essid (OOBS_IFACE_WIRELESS (iface)));

      config_method = oobs_iface_ethernet_get_configuration_method (OOBS_IFACE_ETHERNET (iface));

      if (config_method && strcmp (config_method, "static") == 0)
	g_string_append_printf (str, "<b>%s</b> %s </b>%s</b> %s",
				_("Address:"),
				oobs_iface_ethernet_get_ip_address (OOBS_IFACE_ETHERNET (iface)),
				 _("Subnet mask:"),
				oobs_iface_ethernet_get_network_mask (OOBS_IFACE_ETHERNET (iface)));
      else
	g_string_append_printf (str, "<b>%s</b> %s", _("Address:"), config_method);
    }
  else if (OOBS_IS_IFACE_PLIP (iface))
    {
      g_string_append_printf (str, "<b>%s</b> %s </b>%s</b> %s",
			      _("Address:"),
			      oobs_iface_plip_get_address (OOBS_IFACE_PLIP (iface)),
			      _("Remote address:"),
			      oobs_iface_plip_get_remote_address (OOBS_IFACE_PLIP (iface)));
    }
  else if (OOBS_IS_IFACE_PPP (iface))
    {
      const gchar *type;

      type = oobs_iface_ppp_get_connection_type (OOBS_IFACE_PPP (iface));

      if (strcmp (type, "modem") == 0 ||
	  strcmp (type, "isdn") == 0)
	g_string_append_printf (str,"<b>%s</b> %s </b>%s</b> %s",
				_("Type:"), type,
				_("Phone number:"),
				oobs_iface_ppp_get_phone_number (OOBS_IFACE_PPP (iface)));
      else if (strcmp (type, "gprs") == 0)
	g_string_append_printf (str, "<b>%s</b> %s </b>%s</b> %s",
				_("Type:"), type,
				_("Access point name:"),
				oobs_iface_ppp_get_apn (OOBS_IFACE_PPP (iface)));
      else if (strcmp (type, "pppoe") == 0)
	{
	  OobsIfaceEthernet *ethernet;

	  ethernet = oobs_iface_ppp_get_ethernet (OOBS_IFACE_PPP (iface));
	  g_string_append_printf (str, "<b>%s</b> %s </b>%s</b> %s",
				  _("Type:"), type,
				  _("Ethernet interface:"),
				  oobs_iface_get_device_name (OOBS_IFACE (ethernet)));
	}
      else
	g_string_append_printf (str, "<b>%s</b> %s", _("Type:"), type);
    }

  text = str->str;
  g_string_free (str, FALSE);

  return text;
}

static gchar*
get_iface_desc (OobsIface *iface, gboolean show_name)
{
  gchar *primary_text, *secondary_text, *full_text;

  primary_text = secondary_text = full_text = NULL;

  if (OOBS_IS_IFACE_WIRELESS (iface))
    primary_text = N_("Wireless connection");
  else if (OOBS_IS_IFACE_IRLAN (iface))
    primary_text = N_("Infrared connection");
  else if (OOBS_IS_IFACE_ETHERNET (iface))
    primary_text = N_("Wired connection");
  else if (OOBS_IS_IFACE_PLIP (iface))
    primary_text = N_("Parallel port connection");
  else if (OOBS_IS_IFACE_PPP (iface))
    primary_text = N_("Point to point connection");

  secondary_text = get_iface_secondary_text (iface);

  if (primary_text)
    {
      if (show_name)
	full_text = g_strdup_printf ("<span size=\"larger\" weight=\"bold\">%s (%s)</span>\n%s",
				     _(primary_text), oobs_iface_get_device_name (iface), secondary_text);
      else
	full_text = g_strdup_printf ("<span size=\"larger\" weight=\"bold\">%s</span>\n%s", _(primary_text), secondary_text);
    }
  else
    full_text = g_strdup_printf ("Unknown interface type");

  g_free (secondary_text);
  return full_text;
}

void
ifaces_model_modify_interface_at_iter (GtkTreeIter *iter)
{
  GtkTreeModel *model;
  OobsIface *iface;
  gchar *desc;
  gboolean show_name, configured;
  GdkPixbuf *pixbuf;

  model = GST_NETWORK_TOOL (tool)->interfaces_model;

  gtk_tree_model_get (model, iter,
		      COL_OBJECT, &iface,
		      COL_SHOW_IFACE_NAME, &show_name,
		      -1);
  desc = get_iface_desc (OOBS_IFACE (iface), show_name);
  pixbuf = get_iface_pixbuf (OOBS_IFACE (iface));
  configured = oobs_iface_get_configured (OOBS_IFACE (iface));

  gtk_list_store_set (GTK_LIST_STORE (model), iter,
		      COL_ACTIVE, oobs_iface_get_active (OOBS_IFACE (iface)),
		      COL_IMAGE, pixbuf,
		      COL_DESC, desc,
		      COL_DEV, oobs_iface_get_device_name (OOBS_IFACE (iface)),
		      COL_INCONSISTENT, !configured,
		      COL_NOT_INCONSISTENT, configured,
		      -1);

  if (pixbuf)
    g_object_unref (pixbuf);

  g_object_unref (iface);
  g_free (desc);
}

static void
ifaces_model_set_interface_at_iter (OobsIface *iface, GtkTreeIter *iter, gboolean show_name)
{
  GtkTreeModel *model;

  g_return_if_fail (iface != NULL);

  model = GST_NETWORK_TOOL (tool)->interfaces_model;

  gtk_list_store_set (GTK_LIST_STORE (model), iter,
		      COL_OBJECT, iface,
		      COL_SHOW_IFACE_NAME, show_name,
		      -1);
  ifaces_model_modify_interface_at_iter (iter);
}

static void
iface_state_changed (OobsIface *iface,
		     gpointer   user_data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  OobsIface *model_iface;
  gboolean valid, found;

  found = FALSE;
  model = GST_NETWORK_TOOL (tool)->interfaces_model;
  valid = gtk_tree_model_get_iter_first (model, &iter);

  while (valid && !found)
    {
      gtk_tree_model_get (model, &iter,
			  COL_OBJECT, &model_iface,
			  -1);

      if (iface == model_iface)
	{
	  ifaces_model_modify_interface_at_iter (&iter);
	  found = TRUE;
	}

      g_object_unref (iface);
      valid = gtk_tree_model_iter_next (model, &iter);
    }
}

void
ifaces_model_add_interface (OobsIface *iface, gboolean show_name)
{
  GtkTreeModel *model;
  GtkTreeIter   it;

  model = GST_NETWORK_TOOL (tool)->interfaces_model;

  gtk_list_store_append (GTK_LIST_STORE (model), &it);
  ifaces_model_set_interface_at_iter (iface, &it, show_name);

  g_signal_connect (iface, "state-changed",
		    G_CALLBACK (iface_state_changed), tool);
}

void
ifaces_model_clear (void)
{
  GtkTreeModel *model;

  model = GST_NETWORK_TOOL (tool)->interfaces_model;
  gtk_list_store_clear (GTK_LIST_STORE (model));
}

