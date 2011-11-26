/*
 *  Authors: Luca Cavalli <loopback@slackit.org>
 *
 *  Copyright 2005-2006 Luca Cavalli
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of version 2 of the GNU General Public License
 *  as published by the Free Software Foundation
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <libxml/parser.h>

#include "mate-da-capplet.h"
#include "mate-da-xml.h"
#include "mate-da-item.h"


static gboolean
mate_da_xml_get_bool (const xmlNode *parent, const gchar *val_name)
{
    xmlNode *element;
    gboolean ret_val = FALSE;
    xmlChar *xml_val_name;
    gint len;

    g_return_val_if_fail (parent != NULL, FALSE);
    g_return_val_if_fail (parent->children != NULL, ret_val);
    g_return_val_if_fail (val_name != NULL, FALSE);

    xml_val_name = xmlCharStrdup (val_name);
    len = xmlStrlen (xml_val_name);

    for (element = parent->children; element != NULL; element = element->next) {
	if (!xmlStrncmp (element->name, xml_val_name, len)) {
	    xmlChar *cont = xmlNodeGetContent (element);

	    if (!xmlStrcasecmp (cont, "true") || !xmlStrcasecmp (cont, "1"))
		ret_val = TRUE;
	    else
		ret_val = FALSE;

	    xmlFree (cont);
	}
    }

    xmlFree (xml_val_name);
    return ret_val;
}

static gchar*
mate_da_xml_get_string (const xmlNode *parent, const gchar *val_name)
{
    const gchar * const *sys_langs;
    xmlChar *node_lang;
    xmlNode *element;
    gchar *ret_val = NULL;
    xmlChar *xml_val_name;
    gint len;
    gint i;

    g_return_val_if_fail (parent != NULL, ret_val);
    g_return_val_if_fail (parent->children != NULL, ret_val);
    g_return_val_if_fail (val_name != NULL, ret_val);

#if GLIB_CHECK_VERSION (2, 6, 0)
    sys_langs = g_get_language_names ();
#endif

    xml_val_name = xmlCharStrdup (val_name);
    len = xmlStrlen (xml_val_name);

    for (element = parent->children; element != NULL; element = element->next) {
	if (!xmlStrncmp (element->name, xml_val_name, len)) {
	    node_lang = xmlNodeGetLang (element);

	    if (node_lang == NULL) {
		ret_val = (gchar *) xmlNodeGetContent (element);
	    }
	    else {
		for (i = 0; sys_langs[i] != NULL; i++) {
		    if (!strcmp (sys_langs[i], node_lang)) {
			ret_val = (gchar *) xmlNodeGetContent (element);
			/* since sys_langs is sorted from most desirable to
			 * least desirable, exit at first match
			 */
			break;
		    }
		}
	    }
	    xmlFree (node_lang);
	}
    }

    xmlFree (xml_val_name);
    return ret_val;
}

static gboolean
is_executable_valid (gchar *executable)
{
    gchar *path;

    path = g_find_program_in_path (executable);

    if (path) {
	g_free (path);
	return TRUE;
    }

    return FALSE;
}

static void
mate_da_xml_load_xml (MateDACapplet *capplet, const gchar * filename)
{
    xmlDoc *xml_doc;
    xmlNode *root, *section, *element;
    gchar *executable;
    MateDAWebItem *web_item;
    MateDASimpleItem *mail_item;
    MateDASimpleItem *media_item;
    MateDATermItem *term_item;
    MateDAVisualItem *visual_item;
    MateDAMobilityItem *mobility_item;

    xml_doc = xmlParseFile (filename);

    if (!xml_doc)
	return;

    root = xmlDocGetRootElement (xml_doc);

    for (section = root->children; section != NULL; section = section->next) {
	if (!xmlStrncmp (section->name, "web-browsers", 12)) {
	    for (element = section->children; element != NULL; element = element->next) {
		if (!xmlStrncmp (element->name, "web-browser", 11)) {
		    executable = mate_da_xml_get_string (element, "executable");
		    if (is_executable_valid (executable)) {
			web_item = mate_da_web_item_new ();

			web_item->generic.name = mate_da_xml_get_string (element, "name");
			web_item->generic.executable = executable;
			web_item->generic.command = mate_da_xml_get_string (element, "command");
			web_item->generic.icon_name = mate_da_xml_get_string (element, "icon-name");

			web_item->run_in_terminal = mate_da_xml_get_bool (element, "run-in-terminal");
			web_item->netscape_remote = mate_da_xml_get_bool (element, "netscape-remote");
			if (web_item->netscape_remote) {
			    web_item->tab_command = mate_da_xml_get_string (element, "tab-command");
			    web_item->win_command = mate_da_xml_get_string (element, "win-command");
			}

			capplet->web_browsers = g_list_append (capplet->web_browsers, web_item);
		    }
		    else
			g_free (executable);
		}
	    }
	}
	else if (!xmlStrncmp (section->name, "mail-readers", 12)) {
	    for (element = section->children; element != NULL; element = element->next) {
		if (!xmlStrncmp (element->name, "mail-reader", 11)) {
		    executable = mate_da_xml_get_string (element, "executable");
		    if (is_executable_valid (executable)) {
			mail_item = mate_da_simple_item_new ();

			mail_item->generic.name = mate_da_xml_get_string (element, "name");
			mail_item->generic.executable = executable;
			mail_item->generic.command = mate_da_xml_get_string (element, "command");
			mail_item->generic.icon_name = mate_da_xml_get_string (element, "icon-name");

			mail_item->run_in_terminal = mate_da_xml_get_bool (element, "run-in-terminal");

			capplet->mail_readers = g_list_append (capplet->mail_readers, mail_item);
		    }
		    else
			g_free (executable);
		}
	    }
	}
	else if (!xmlStrncmp (section->name, "terminals", 9)) {
	    for (element = section->children; element != NULL; element = element->next) {
		if (!xmlStrncmp (element->name, "terminal", 8)) {
		    executable = mate_da_xml_get_string (element, "executable");
		    if (is_executable_valid (executable)) {
			term_item = mate_da_term_item_new ();

			term_item->generic.name = mate_da_xml_get_string (element, "name");
			term_item->generic.executable = executable;
			term_item->generic.command = mate_da_xml_get_string (element, "command");
			term_item->generic.icon_name = mate_da_xml_get_string (element, "icon-name");

			term_item->exec_flag = mate_da_xml_get_string (element, "exec-flag");

			capplet->terminals = g_list_append (capplet->terminals, term_item);
		    }
		    else
			g_free (executable);
		}
	    }
	}
	else if (!xmlStrncmp (section->name, "media-players", 13)) {
	    for (element = section->children; element != NULL; element = element->next) {
		if (!xmlStrncmp (element->name, "media-player", 12)) {
		    executable = mate_da_xml_get_string (element, "executable");
		    if (is_executable_valid (executable)) {
			media_item = mate_da_simple_item_new ();

			media_item->generic.name = mate_da_xml_get_string (element, "name");
			media_item->generic.executable = executable;
			media_item->generic.command = mate_da_xml_get_string (element, "command");
			media_item->generic.icon_name = mate_da_xml_get_string (element, "icon-name");

			media_item->run_in_terminal = mate_da_xml_get_bool (element, "run-in-terminal");

			capplet->media_players = g_list_append (capplet->media_players, media_item);
		    }
		    else
			g_free (executable);
		}
	    }
	}
	else if (!xmlStrncmp (section->name, "a11y-visual", 11)) {
	    for (element = section->children; element != NULL; element = element->next) {
		if (!xmlStrncmp (element->name, "visual", 6)) {
		    executable = mate_da_xml_get_string (element,"executable");
		    if (is_executable_valid (executable)) {
			visual_item = mate_da_visual_item_new ();

			visual_item->generic.name = mate_da_xml_get_string (element, "name");
			visual_item->generic.executable = executable;
			visual_item->generic.command = mate_da_xml_get_string (element, "command");
			visual_item->generic.icon_name = mate_da_xml_get_string (element, "icon-name");

			visual_item->run_at_startup = mate_da_xml_get_bool (element, "run-at-startup");

			capplet->visual_ats = g_list_append (capplet->visual_ats, visual_item);
		    }
                    else
                        g_free (executable);
		}
	    }
	}
	else if (!xmlStrncmp (section->name, "a11y-mobility", 13)) {
	    for (element = section->children; element != NULL; element = element->next) {
		if (!xmlStrncmp (element->name, "mobility", 8)) {
		    executable = mate_da_xml_get_string (element,"executable");
		    if (is_executable_valid (executable)) {
			mobility_item = mate_da_mobility_item_new ();

			mobility_item->generic.name = mate_da_xml_get_string (element, "name");
			mobility_item->generic.executable = executable;
			mobility_item->generic.command = mate_da_xml_get_string (element, "command");
			mobility_item->generic.icon_name = mate_da_xml_get_string (element, "icon-name");

			mobility_item->run_at_startup = mate_da_xml_get_bool (element, "run-at-startup");

			capplet->mobility_ats = g_list_append (capplet->mobility_ats, mobility_item);
		    }
                    else
                        g_free (executable);
		}
	    }
	}
    }

    xmlFreeDoc (xml_doc);
}

void mate_da_xml_load_list(MateDACapplet* capplet)
{
	GDir* app_dir = g_dir_open(MATECC_APPS_DIR, 0, NULL);

	if (app_dir != NULL)
	{
		const gchar* extra_file;
		gchar* filename;

		while ((extra_file = g_dir_read_name(app_dir)) != NULL)
		{
			filename = g_build_filename(MATECC_APPS_DIR, extra_file, NULL);

			if (g_str_has_suffix(filename, ".xml"))
			{
				mate_da_xml_load_xml(capplet, filename);
			}

			g_free(filename);
		}

		g_dir_close(app_dir);
	}
}

void
mate_da_xml_free (MateDACapplet *capplet)
{
    g_list_foreach (capplet->web_browsers, (GFunc) mate_da_web_item_free, NULL);
    g_list_foreach (capplet->mail_readers, (GFunc) mate_da_simple_item_free, NULL);
    g_list_foreach (capplet->terminals, (GFunc) mate_da_term_item_free, NULL);
    g_list_foreach (capplet->media_players, (GFunc) mate_da_simple_item_free, NULL);
    g_list_foreach (capplet->visual_ats, (GFunc) mate_da_visual_item_free, NULL);
    g_list_foreach (capplet->mobility_ats, (GFunc) mate_da_mobility_item_free, NULL);

    g_list_free (capplet->web_browsers);
    g_list_free (capplet->mail_readers);
    g_list_free (capplet->terminals);
    g_list_free (capplet->media_players);
    g_list_free (capplet->visual_ats);
    g_list_free (capplet->mobility_ats);

    g_object_unref (capplet->builder);
    g_free (capplet);
}
