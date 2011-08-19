/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  oafd: OAF CORBA dameon.
 *
 *  Copyright (C) 2000 Eazel, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this library; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Mathieu Lacage <mathieu@eazel.com>
 *
 */

#include "config.h"
#include <string.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h> 
#include <glib.h>

#include <glib/gi18n.h>
#include "object-directory-config-file.h"

#ifdef G_OS_WIN32
#include "matecomponent-activation/matecomponent-activation-private.h"
#undef SERVER_CONFDIR
#define SERVER_CONFDIR _matecomponent_activation_win32_get_server_confdir ()
#endif

static xmlDocPtr
object_directory_load_xml_file (void)
{
        xmlDocPtr doc = NULL;
        char *matecomponent_activation_config_file;

        matecomponent_activation_config_file = g_strconcat (
                SERVER_CONFDIR, SERVER_CONFIG_FILE, NULL);
#ifdef G_OS_WIN32
        {
                gchar *contents;
                gsize length;

                if (g_file_get_contents (matecomponent_activation_config_file,
                                         &contents, &length, NULL)) {
                        doc = xmlParseMemory (contents, length);
                        g_free (contents);
                }
        }
#else
        doc = xmlParseFile (matecomponent_activation_config_file);
#endif
       /* check if the document was read successfully. */
        if (doc == NULL) {
                g_warning (_("The MateComponent Activation configuration file was not read "
                             "successfully. Please, check it is valid in: %s"),
                           matecomponent_activation_config_file);
		g_free (matecomponent_activation_config_file);
                return NULL;
        }

	g_free (matecomponent_activation_config_file);

        return doc;
}

char *
object_directory_load_config_file (void)
{
        char *result;
        xmlDocPtr doc;
        xmlNodePtr search_node;

        doc = object_directory_load_xml_file ();

        if (doc == NULL || doc->xmlRootNode == NULL)
		search_node = NULL;
	else
		search_node = doc->xmlRootNode->xmlChildrenNode;
        result = g_strdup ("");
        while (search_node != NULL) {
                if (strcmp (search_node->name, "searchpath") == 0) {
                        xmlNodePtr item_node;
                        item_node = search_node->xmlChildrenNode;
                        while (item_node != NULL) {
                                if (strcmp (item_node->name, "item") == 0) {
                                        char *directory;
                                        char *old_result = result;

                                        directory = xmlNodeGetContent (item_node);
                                        if (directory) {
                                                result = g_strconcat (old_result, G_SEARCHPATH_SEPARATOR_S, directory, NULL);
                                                xmlFree (directory);
                                                g_free (old_result);
                                        }
                                }
                                item_node = item_node->next;
                        }
                }
                search_node = search_node->next;
        }

        xmlFreeDoc (doc);

        return result;
}


