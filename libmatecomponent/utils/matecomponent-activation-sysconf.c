/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/*
 *  matecomponent-activation-sysconf: a simple utility to manipulate
 *                             activation configuration files.
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
#include <locale.h>
#include <glib.h>
#include <libxml/tree.h>   
#include <libxml/parser.h>  
#include <libxml/xmlmemory.h>

#include <glib/gi18n.h>

#include <matecomponent-activation/matecomponent-activation.h>
#include "activation-server/object-directory-config-file.h"

#ifdef G_OS_WIN32
#include "matecomponent-activation/matecomponent-activation-private.h"
#undef SERVER_LOCALEDIR
#define SERVER_LOCALEDIR _matecomponent_activation_win32_get_localedir ()
#undef SERVER_CONFDIR
#define SERVER_CONFDIR _matecomponent_activation_win32_get_server_confdir ()
#endif


static xmlDocPtr
open_file (void)
{
        char *config_file;
        xmlDocPtr doc = NULL;

        config_file = g_strconcat (
                SERVER_CONFDIR, SERVER_CONFIG_FILE, NULL);
        
#ifdef G_OS_WIN32
        {
                gchar *contents;
                gsize length;

                if (g_file_get_contents (config_file, &contents, &length, NULL)) {
                        doc = xmlParseMemory (contents, length);
                        g_free (contents);
                }
        }
#else
        doc = xmlParseFile (config_file);
#endif
        return doc;
}


static void 
save_file (xmlDocPtr doc)
{
        char *config_file;
        

        config_file = g_strconcat (
                SERVER_CONFDIR, SERVER_CONFIG_FILE, NULL);
        if (xmlSaveFile (config_file, doc) == -1) {
                g_print (_("Could not save configuration file.\n"));
                g_print (_("Please, make sure you have permissions to write "
                           "to '%s'.\n"), config_file);
        } else {
                g_print (_("Successfully wrote configuration file.\n"));
        }
        g_free (config_file);

}

static gboolean
display_config_path (const gchar *option_name,
                     const gchar *value,
                     gpointer data,
                     GError **error)
{
        char *config_file;

        config_file = g_strconcat (
                SERVER_CONFDIR, SERVER_CONFIG_FILE, NULL);

        g_print (_("configuration file is:\n    %s\n"), config_file);
        
        g_free (config_file);
        return TRUE;
}

static xmlNodePtr
get_root_first_child (xmlDocPtr doc)
{
	if (doc == NULL)
		return NULL;
	if (doc->xmlRootNode == NULL)
		return NULL;
	return doc->xmlRootNode->xmlChildrenNode;
}

static gboolean
add_directory (const gchar *option_name,
               const gchar *directory,
               gpointer data,
               GError **error)
{
        xmlDocPtr doc;
        xmlNodePtr search_node;
        gboolean is_already_there;
        
        is_already_there = FALSE;
        doc = open_file ();

        /* make sure the directory we want to add is not already
           in the config file */
        search_node = get_root_first_child (doc);
        while (search_node != NULL) {
                if (strcmp (search_node->name, "searchpath") == 0) {
                        xmlNodePtr item_node;
                        item_node = search_node->xmlChildrenNode;
                        while (item_node != NULL) {
                                if (strcmp (item_node->name, "item") == 0) {
                                        char *dir_path;
                                        dir_path = xmlNodeGetContent (item_node);
                                        if (strcmp (dir_path, directory) == 0) {
                                                is_already_there = TRUE;
                                                g_print (_("%s already in configuration file\n"), 
                                                         directory);
                                                
                                        }
                                        xmlFree (dir_path);
                                }
                                item_node = item_node->next; 
                        }
                }
                search_node = search_node->next;
        }


        if (!is_already_there) {
                xmlNodePtr new_node;

                /* add the directory to the config file */
                search_node = get_root_first_child (doc);

                if (search_node == NULL)
			g_print (_("there is not a properly structured configuration file\n"));
		else {
			/* go to the first searchpath node */
			while (strcmp (search_node->name, "searchpath") != 0) {
				search_node = search_node->next;                        
			}
			new_node = xmlNewDocNode (doc, NULL, "item", directory);
			xmlAddChild (search_node, new_node);
			
			save_file (doc);
		}
        }
        
        xmlFreeDoc (doc);
        return TRUE;
}

static gboolean
remove_directory (const gchar *option_name,
                  const gchar *directory,
                  gpointer data,
                  GError **error)
{
        xmlDocPtr doc;
        xmlNodePtr search_node;

        doc = open_file ();

        search_node = get_root_first_child (doc);
        while (search_node != NULL) {
                if (strcmp (search_node->name, "searchpath") == 0) {
                        xmlNodePtr item_node;
                        item_node = search_node->xmlChildrenNode;
                        while (item_node != NULL) {
                                if (strcmp (item_node->name, "item") == 0) {
                                        char *dir_path;
                                        dir_path = xmlNodeGetContent (item_node);
                                        if (strcmp (dir_path, directory) == 0) {
                                                if (strcmp (dir_path, directory) == 0) {
                                                        xmlDocPtr doc;
                                                        doc = item_node->doc;
                                                        xmlUnlinkNode (item_node);
                                                        xmlFreeNode (item_node);
                                                        save_file (doc);
                                                        xmlFree (dir_path);
                                                        return TRUE;
                                                }
                                        }
                                        xmlFree (dir_path);
                                }
                                item_node = item_node->next; 
                        }
                }
                search_node = search_node->next;
        }

        xmlFreeDoc (doc);                                                
        return TRUE;
}

static gboolean
display_directories (const gchar *option_name,
                     const gchar *value,
                     gpointer data,
                     GError **error)
{
        xmlDocPtr doc;
        xmlNodePtr search_node;

        doc = open_file ();

        g_print (_("MateComponent-activation configuration file contains:\n"));

	search_node = get_root_first_child (doc);
        while (search_node != NULL) {
                if (strcmp (search_node->name, "searchpath") == 0) {
                        xmlNodePtr item_node;
                        item_node = search_node->xmlChildrenNode;
                        while (item_node != NULL) {
                                if (strcmp (item_node->name, "item") == 0) {
                                        char *dir_path;
                                        dir_path = xmlNodeGetContent (item_node);
                                        g_print ("    %s\n", dir_path);
                                        xmlFree (dir_path);
                                }
                                item_node = item_node->next; 
                        }
                }
                search_node = search_node->next;
        }
        xmlFreeDoc (doc);                                                
        return TRUE;
}



static const GOptionEntry oaf_sysconf_goption_options[] = {
        /* add- and remove-directory could be marked as FILENAMEs, but
           they are just added directly to the XML file, so we leave
           them as UTF-8. */
        {"remove-directory", '\0', 0, G_OPTION_ARG_CALLBACK, remove_directory, 
         N_("Directory to remove from configuration file"), N_("PATH")},
        {"add-directory", '\0', 0, G_OPTION_ARG_CALLBACK, add_directory, 
         N_("Directory to add to configuration file"), N_("PATH")},
        {"display-directories", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, display_directories, 
         N_("Display directories in configuration file"), NULL},
        {"config-file-path", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, display_config_path, 
         N_("Display path to configuration file"), NULL},
        {NULL}
};

int main (int argc, char **argv)
{
        GOptionContext *context;
        GError *error = NULL;
        gboolean do_usage_exit = FALSE;

        setlocale (LC_ALL, "");

        /* init nls */
        bindtextdomain (GETTEXT_PACKAGE, SERVER_LOCALEDIR);
        textdomain (GETTEXT_PACKAGE);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

        /* init goption */
	g_set_prgname ("matecomponent-activation-sysconf");
	context = g_option_context_new (NULL);
	g_option_context_add_main_entries (context, oaf_sysconf_goption_options, GETTEXT_PACKAGE);

	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		g_printerr ("%s\n", error->message);
		g_error_free (error);
		do_usage_exit = TRUE;
	}
	g_option_context_free (context);

	if (do_usage_exit) {
		g_printerr (_("Run '%s --help' to see a full list of available command line options.\n"), g_get_prgname ());
		exit (1);
	}

        return 0;
}
