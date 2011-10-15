/* MateComponent component browser
 *
 * AUTHORS:
 *      Dan Siemon <dan@coverfire.com>
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

#ifndef __COMPONENT_DETAILS_H__
#define __COMPONENT_DETAILS_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COMPONENT_DETAILS_TYPE (component_details_get_type ())
#define COMPONENT_DETAILS(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), COMPONENT_DETAILS_TYPE, ComponentDetails))
#define COMPONENT_DETAILS_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), COMPONENT_DETAILS_TYPE, ComponentDetailsClass))
#define IS_COMPONENT_DETAILS(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), COMPONENT_DETAILS_TYPE))
#define IS_COMPONENT_DETAILS__CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), COMPONENT_DETAILS_TYPE))
#define COMPONENT_DETAILS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), COMPONENT_DETAILS_TYPE, ComponentDetailsClass))

typedef struct _ComponentDetails ComponentDetails;
typedef struct _ComponentDetailsClass ComponentDetailsClass;
typedef struct _ComponentDetailsPrivate ComponentDetailsPrivate;

struct _ComponentDetails {
	GtkVBox box;
	ComponentDetailsPrivate *priv;
};

struct _ComponentDetailsClass {
	GtkVBoxClass parent_class;
};

/*
  Widget functions.
*/
GType component_details_get_type (void);
GtkWidget *component_details_new (gchar *iid);

/*
  ComponentDetails widget control functions.
*/
void component_details_get_info (ComponentDetails *comp_details, gchar *iid);

G_END_DECLS

#endif
