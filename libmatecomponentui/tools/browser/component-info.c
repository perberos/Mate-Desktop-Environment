/* MateComponent component browser
 *
 * AUTHORS:
 *      Dan Siemon <dan@coverfire.com>
 *      Patanjali Somayaji <patanjali@morelinux.com>
 *      Rodrigo Moya <rodrigo@mate-db.org>
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

#include "oaf-helper.h"

static void
free_all_members (MateComponentComponentInfo *info)
{
	GList *temp;

	g_return_if_fail (info != NULL);

	g_free (info->component_iid);
	g_free (info->component_type);
	g_free (info->component_location);
	g_free (info->component_username);
	g_free (info->component_hostname);
	g_free (info->component_domain);
	g_free (info->component_description);
	g_free (info->component_name);

	info->component_iid = NULL;
	info->component_type = info->component_location = NULL;
	info->component_username = info->component_hostname = NULL;
	info->component_domain = info->component_description = NULL;
	info->component_name = NULL;

	/* free the list of properties */
	for (temp = info->component_properties; temp; temp = temp->next) {
		MateComponentComponentProperty *prop = temp->data;

		g_free (prop->property_name);

		if (prop->property_type == PROPERTY_TYPE_STRING)
			g_free (prop->property_value.value_string);
		g_free (prop);
	}
	g_list_free (info->component_properties);
	info->component_properties = NULL;
}

MateComponentComponentInfo *
matecomponent_component_info_new (void)
{
	MateComponentComponentInfo *info;

	info = g_new0 (MateComponentComponentInfo, 1);
	return info;
}

void
matecomponent_component_info_set_values (MateComponentComponentInfo *info,
				  MateComponent_ImplementationID iid,
				  const gchar *type,
				  const gchar *location,
				  const gchar *username,
				  const gchar *hostname,
				  const gchar *domain,
				  const gchar *description,
				  const gchar *name,
				  GList *properties)
{
	g_return_if_fail (info != NULL);

	free_all_members (info);
	info->component_iid = g_strdup (iid);
	info->component_type = g_strdup (type);
	info->component_location = g_strdup (location);
	info->component_username = g_strdup (username);
	info->component_hostname = g_strdup (hostname);
	info->component_domain = g_strdup (domain);
	info->component_description = g_strdup (description);
	info->component_name = g_strdup (name);
	info->component_properties = properties;
}

void
matecomponent_component_info_free (MateComponentComponentInfo *info)
{
	g_return_if_fail (info != NULL);

	free_all_members (info);
	g_free (info);
}
