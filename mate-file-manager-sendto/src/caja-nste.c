/*
 *  Caja-sendto 
 * 
 *  Copyright (C) 2004 Free Software Foundation, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Roberto Majadas <roberto.majadas@openshine.com> 
 * 
 */

#include <config.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <libcaja-extension/caja-extension-types.h>
#include <libcaja-extension/caja-file-info.h>
#include <libcaja-extension/caja-menu-provider.h>
#include "caja-nste.h"


static GObjectClass *parent_class;

static void
sendto_callback (CajaMenuItem *item,
	      gpointer          user_data)
{
	GList            *files, *scan;
	CajaFileInfo *file;
	gchar            *uri;
	GString          *cmd;

	files = g_object_get_data (G_OBJECT (item), "files");
	file = files->data;

	cmd = g_string_new ("caja-sendto");

	for (scan = files; scan; scan = scan->next) {
		CajaFileInfo *file = scan->data;

		uri = caja_file_info_get_uri (file);
		g_string_append_printf (cmd, " \"%s\"", uri);
		g_free (uri);
	}

	g_spawn_command_line_async (cmd->str, NULL);

	g_string_free (cmd, TRUE);
}

static GList *
caja_nste_get_file_items (CajaMenuProvider *provider,
			      GtkWidget            *window,
			      GList                *files)
{
	GList    *items = NULL;
	gboolean  one_item;
	CajaMenuItem *item;

	if (files == NULL)
		return NULL;

	one_item = (files != NULL) && (files->next == NULL);
	if (one_item && 
	    !caja_file_info_is_directory ((CajaFileInfo *)files->data)) {
		item = caja_menu_item_new ("CajaNste::sendto",
					       _("Send To..."),
					       _("Send file by mail, instant message..."),
					       "document-send");
	} else {
		item = caja_menu_item_new ("CajaNste::sendto",
					       _("Send To..."),
					       _("Send files by mail, instant message..."),
					       "document-send");
	}
  g_signal_connect (item, 
      "activate",
      G_CALLBACK (sendto_callback),
      provider);
  g_object_set_data_full (G_OBJECT (item), 
      "files",
      caja_file_info_list_copy (files),
      (GDestroyNotify) caja_file_info_list_free);

  items = g_list_append (items, item);

	return items;
}


static void 
caja_nste_menu_provider_iface_init (CajaMenuProviderIface *iface)
{
	iface->get_file_items = caja_nste_get_file_items;
}


static void 
caja_nste_instance_init (CajaNste *nste)
{
}


static void
caja_nste_class_init (CajaNsteClass *class)
{
	parent_class = g_type_class_peek_parent (class);
}


static GType nste_type = 0;


GType
caja_nste_get_type (void) 
{
	return nste_type;
}


void
caja_nste_register_type (GTypeModule *module)
{
	static const GTypeInfo info = {
		sizeof (CajaNsteClass),
		(GBaseInitFunc) NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc) caja_nste_class_init,
		NULL, 
		NULL,
		sizeof (CajaNste),
		0,
		(GInstanceInitFunc) caja_nste_instance_init,
	};

	static const GInterfaceInfo menu_provider_iface_info = {
		(GInterfaceInitFunc) caja_nste_menu_provider_iface_init,
		NULL,
		NULL
	};

	nste_type = g_type_module_register_type (module,
					         G_TYPE_OBJECT,
					         "CajaNste",
					         &info, 0);

	g_type_module_add_interface (module,
				     nste_type,
				     CAJA_TYPE_MENU_PROVIDER,
				     &menu_provider_iface_info);
}
