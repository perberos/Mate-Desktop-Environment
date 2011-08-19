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

#ifndef __OAF_HELPER_H__
#define __OAF_HELPER_H__

#include <matecomponent-activation/matecomponent-activation.h>
#include <matecomponent-activation/MateComponent_Activation_types.h>
#include <matecomponent/matecomponent-i18n.h>

/*
 *      MateComponentComponentInfo is used to store the data read from
 *      oaf about each component
 */

typedef enum {
        PROPERTY_TYPE_STRING,
        PROPERTY_TYPE_DOUBLE,
        PROPERTY_TYPE_BOOLEAN,
        PROPERTY_TYPE_STRINGLIST
} MateComponentComponentPropertyType;

typedef struct {
        gchar *property_name;
	MateComponentComponentPropertyType property_type;
        union {
                gchar *value_string;
		double value_double;
		gboolean value_boolean;
		GList *value_stringlist;
        } property_value;
} MateComponentComponentProperty;

typedef struct {
        MateComponent_ImplementationID component_iid;
        gchar *component_type;
        gchar *component_location;
        gchar *component_username;
        gchar *component_hostname;
        gchar *component_domain;
        gchar *component_description;
        gchar *component_name;
        GList *component_properties;
	gboolean active;
} MateComponentComponentInfo;

MateComponentComponentInfo *matecomponent_component_info_new (void);

void matecomponent_component_info_set_values (
	MateComponentComponentInfo *info,
	MateComponent_ImplementationID iid,
	const gchar *type,
	const gchar *location,
	const gchar *username,
	const gchar *hostname,
	const gchar *domain,
	const gchar *description,
	const gchar *name,
	GList *properties);

void matecomponent_component_info_free (MateComponentComponentInfo *info);

GList *matecomponent_browser_get_components_list (const gchar *query);

void matecomponent_browser_free_components_list (GList *list);

void matecomponent_component_list_print (GList *list);

GList *matecomponent_component_get_repoids (MateComponentComponentInfo *info);

#endif
