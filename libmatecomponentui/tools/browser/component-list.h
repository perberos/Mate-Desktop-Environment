/* MateComponent component browser
 *
 * AUTHORS:
 *      Patanjali Somayaji <patanjali@morelinux.com>
 *      Dan Siemon <dan@coverfire.com>
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

#ifndef __COMPONENT_LIST_H__
#define __COMPONENT_LIST_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COMPONENT_LIST_TYPE        (component_list_get_type ())
#define COMPONENT_LIST(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), COMPONENT_LIST_TYPE, ComponentList))
#define COMPONENT_LIST_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), COMPONENT_LIST_TYPE, ComponentListClass))
#define IS_COMPONENT_LIST(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), COMPONENT_LIST_TYPE))
#define IS_COMPONENT_LIST__CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), COMPONENT_LIST_TYPE))
#define COMPONENT_LIST_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), COMPONENT_LIST_TYPE, ComponentListClass))

typedef struct _ComponentList        ComponentList;
typedef struct _ComponentListClass   ComponentListClass;
typedef struct _ComponentListPrivate ComponentListPrivate;

struct _ComponentList {
	GtkVBox box;
	ComponentListPrivate *priv;
};

struct _ComponentListClass {
	GtkVBoxClass parent_class;
	void (* component_details)(ComponentList *click);
};

/*
  Widget functions
*/
GType      component_list_get_type (void);
GtkWidget *component_list_new (void);

/*
  ComponentList widget control functions
*/
void component_list_show (ComponentList *comp_list, gchar *query);
gchar *component_list_get_selected_iid (ComponentList *comp_list);

G_END_DECLS

#endif
