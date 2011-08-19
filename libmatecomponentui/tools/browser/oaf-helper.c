/* MateComponent component browser
 *
 * AUTHORS:
 *      Dan Siemon <dan@coverfire.com>
 *      Rodrigo Moya <rodrigo@mate-db.org>
 *      Patanjali Somayaji <patanjali@morelinux.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <matecomponent/matecomponent-exception.h>
#include "oaf-helper.h"

/*
 * Utility function to search through the properties list
 * looking for a string property that matches property_name.
 */
static gchar *
get_string_value (GList *properties_list,
		  const gchar *property_name)
{
        GList *temp = NULL; 
        gchar *description = NULL;
	MateComponentComponentProperty *property;

	for (temp = properties_list; temp; temp = temp->next) {
                property = temp->data;
                if (!g_ascii_strcasecmp (property_name, property->property_name) &&
                        property->property_type == PROPERTY_TYPE_STRING) {
                           description = g_strdup_printf (
				"%s", property->property_value.value_string);
                }
        }

        return description;
}

/*
 * Utility function to get the language from the env var.
 * I have no clue if this is the right way to do this or not.
 */
static gchar *
get_lang (void) {
	gchar *lang, *ret;
	gchar **search;

#ifndef G_OS_WIN32
	lang = getenv ("LANG");
#else
	lang = g_win32_getlocale ();
#endif
	if (!lang)
		return g_strdup ("");

	search = g_strsplit (lang, "_", 1);
	if (!search)
		return g_strdup ("");

	ret = g_strdup_printf ("%s", search[0]);

	g_strfreev (search);

	return ret;
}

/*
 * Return the name of this component for this locale
 * if it does not exist return the default name.
 */
static gchar *
matecomponent_component_get_locale_name (GList *property_list, gchar *lang)
{
	gchar *name;
	gchar *e_lang;

	e_lang = g_strdup_printf ("name-%s", lang);
	
	name = get_string_value (property_list, e_lang);
	if (name != NULL) {
		/* We found a name property for this locale, return it. */
		return name;
	}

	g_free (e_lang);

	/* Use the default property. */
	name = get_string_value (property_list, "name");
	if (name == NULL) {
		/* For some reason we didn't find the default property
		   either. */
		name = g_strdup_printf ("-");
	}
		
	return name;
}

/*
 * Return the description of the component for this locale
 * (if it exists).
 */
static gchar *
matecomponent_component_get_locale_desc (GList *property_list, gchar *lang)
{
	gchar *description;
	gchar *e_lang;

	e_lang = g_strdup_printf ("description-%s", lang);

	description = get_string_value (property_list, e_lang);
	if (description != NULL) {
		/* We found a description property so return it. */
		return description;
	}

	g_free (e_lang);
	
	/* Use the default description. */
	description = get_string_value (property_list, "description");
	if (description == NULL) {
		/* No default description... */
		description = g_strdup_printf ("-");
	}

	return description;
}

static GList *
get_properties_list (MateComponent_ServerInfo *server_info)
{
        MateComponent_ActivationProperty *property_list;
        MateComponentComponentProperty *property;
        GList *component_property_list = NULL;
        MateComponent_StringList stringlist;
        int i, j;

        property_list =
		(MateComponent_ActivationProperty *) server_info->props._buffer;
        for (i = 0; i < server_info->props._length;i++) {
                property = g_new0 (MateComponentComponentProperty, 1);
		property->property_name = g_strdup (property_list[i].name);

                switch (property_list[i].v._d) {
		case (MateComponent_ACTIVATION_P_STRING) :
			property->property_type = PROPERTY_TYPE_STRING;
			property->property_value.value_string =
				g_strdup (property_list[i].v._u.value_string);
			break;
		case (MateComponent_ACTIVATION_P_NUMBER) :
			property->property_type = PROPERTY_TYPE_DOUBLE;
			property->property_value.value_double
				= property_list[i].v._u.value_number;
			break;
		case (MateComponent_ACTIVATION_P_BOOLEAN) :
			property->property_type = PROPERTY_TYPE_BOOLEAN;
			property->property_value.value_boolean
				= property_list[i].v._u.value_boolean;
			break;
		case (MateComponent_ACTIVATION_P_STRINGV) :
			property->property_type = PROPERTY_TYPE_STRINGLIST;
			property->property_value.value_stringlist  = NULL;
			stringlist = property_list[i].v._u.value_stringv;
			for (j = 0; j < stringlist._length; j++) {
				property->property_value.value_stringlist =
                                        g_list_append (
						property->property_value.value_stringlist,
						g_strdup (stringlist._buffer [j]));
			}
			break;
                }
                component_property_list
                        = g_list_append (component_property_list, property);
        }

	return component_property_list;
}

/**
  Runs the query and returns the number of components
  that matched the query
*/
static gint
num_query_matches (gchar *query)
{
	MateComponent_ServerInfoList *server_info_list;
	CORBA_Environment ev;
	gint ret;

	CORBA_exception_init (&ev);
	server_info_list = matecomponent_activation_query (query, NULL, &ev);
	if (MATECOMPONENT_EX (&ev)) {
		CORBA_exception_free (&ev);
		g_assert_not_reached (); /* What would this mean? */
	}

	ret = server_info_list->_length;
	
	CORBA_free (server_info_list);
	CORBA_exception_free (&ev);

	return ret;
}

/**
  Figure out if this iid is currently active. This would not be
  necessary if MateComponent Activation had this in the result.
*/
static gboolean
is_component_active (gchar *iid)
{
	gchar *query;

	g_assert (iid != NULL);

	query = g_strdup_printf ("(iid == '%s') && (_active)", iid);

	if (num_query_matches(query) != 0) {
		g_free (query);
		return TRUE;
	} else {
		g_free (query);
		return FALSE;
	}
}

static GList *
convert_query_to_list (GList *list, const gchar *query)
{
	MateComponent_ServerInfoList *server_info_list;
	MateComponent_ServerInfo *server_info;
	CORBA_Environment ev;
	gint i;
	gchar *lang;

	g_return_val_if_fail (query != NULL, list);

	lang = get_lang ();

	CORBA_exception_init (&ev);
	server_info_list = matecomponent_activation_query (query, NULL, &ev);
	if (MATECOMPONENT_EX (&ev)) {
		CORBA_exception_free (&ev);
		return list;
	}

	/* convert the MateComponent_ServerInfoList into GList items */
	for (i = 0; i < server_info_list->_length; i++) {
		MateComponentComponentInfo *info;
		GList *property_list;
		gchar *description, *name;

		server_info = &server_info_list->_buffer[i];

		info = matecomponent_component_info_new ();
		property_list = get_properties_list (server_info);

		description = matecomponent_component_get_locale_desc (property_list,
								lang);
		name = matecomponent_component_get_locale_name (property_list, lang);

		matecomponent_component_info_set_values (
			info,
			server_info->iid,
			server_info->server_type,
			server_info->location_info,
			server_info->username,
			server_info->hostname,
			server_info->domain,
			description,
			name,
			property_list);

		info->active = is_component_active (server_info->iid);

		g_free (name);
		g_free (description);

		list = g_list_append (list, info);
	}

	g_free (lang);
	CORBA_free (server_info_list);
	CORBA_exception_free (&ev);

	return list;
}

GList *
matecomponent_browser_get_components_list (const gchar *query)
{
	GList *list = NULL;

	/* prepare queries */
	if (query == NULL) {
		query = g_strdup ("_active == FALSE || _active");
	}

	list = convert_query_to_list (NULL, query);

	return list;
}

void
matecomponent_browser_free_components_list (GList *list)
{
	GList *l;

	for (l = list; l; l = l->next)
		matecomponent_component_info_free (l->data);

	g_list_free (list);
}

/*
 * Takes a MateComponentComponentInfo structure and returns a GList
 * of the components Repo Ids. Basically a convenience function.
 */
GList *
matecomponent_component_get_repoids (MateComponentComponentInfo *info)
{
	GList *ret_list = NULL;
	GList *properties, *temp, *prop_temp;
	MateComponentComponentProperty *prop;
	gchar *str;

	/* Get the property list. */
	properties = info->component_properties;

	temp = properties;
	while (temp != NULL) {
		prop = (MateComponentComponentProperty *) temp->data;
		
		/* We only want repo_ids */
		if (g_ascii_strcasecmp (prop->property_name, "repo_ids") == 0) {
			
			prop_temp = prop->property_value.value_stringlist;
			while (prop_temp != NULL) {
				/* Get the values. */
				str = g_strdup_printf ("%s",
						       (char*)prop_temp->data);

				ret_list = g_list_append (ret_list, str);

				prop_temp = prop_temp->next;
			}
		}
		/* We got what we want. */
		break;
	}

	return ret_list;
}



/*
 * Debugging functions.
 */
void
matecomponent_component_list_print (GList *list)
{
	GList *temp;
	MateComponentComponentInfo *info;

	g_print ("---------------------------------\n");

	for (temp = list; temp != NULL; temp=temp->next) {
		info = (MateComponentComponentInfo *) temp->data;
		
		g_print ("Name: %s\n", info->component_name);
	}
	
	g_print ("---------------------------------\n");
}
