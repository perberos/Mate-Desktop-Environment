/* Eye of Mate - Side bar
 *
 * Copyright (C) 2004 Red Hat, Inc.
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@mate.org>
 *
 * Based on evince code (shell/ev-sidebar.h) by:
 * 	- Jonathan Blandford <jrb@alum.mit.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __EOG_SIDEBAR_H__
#define __EOG_SIDEBAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _EogSidebar EogSidebar;
typedef struct _EogSidebarClass EogSidebarClass;
typedef struct _EogSidebarPrivate EogSidebarPrivate;

#define EOG_TYPE_SIDEBAR	    (eog_sidebar_get_type())
#define EOG_SIDEBAR(obj)	    (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_SIDEBAR, EogSidebar))
#define EOG_SIDEBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_SIDEBAR, EogSidebarClass))
#define EOG_IS_SIDEBAR(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_SIDEBAR))
#define EOG_IS_SIDEBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOG_TYPE_SIDEBAR))
#define EOG_SIDEBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOG_TYPE_SIDEBAR, EogSidebarClass))

struct _EogSidebar {
	GtkVBox base_instance;

	EogSidebarPrivate *priv;
};

struct _EogSidebarClass {
	GtkVBoxClass base_class;

	void (* page_added)   (EogSidebar *eog_sidebar,
			       GtkWidget  *main_widget);

	void (* page_removed) (EogSidebar *eog_sidebar,
			       GtkWidget  *main_widget);
};

GType      eog_sidebar_get_type     (void);

GtkWidget *eog_sidebar_new          (void);

void       eog_sidebar_add_page     (EogSidebar  *eog_sidebar,
				     const gchar *title,
				     GtkWidget   *main_widget);

void       eog_sidebar_remove_page  (EogSidebar  *eog_sidebar,
				     GtkWidget   *main_widget);

void       eog_sidebar_set_page     (EogSidebar  *eog_sidebar,
				     GtkWidget   *main_widget);

gint       eog_sidebar_get_n_pages  (EogSidebar  *eog_sidebar);

gboolean   eog_sidebar_is_empty     (EogSidebar  *eog_sidebar);

G_END_DECLS

#endif /* __EOG_SIDEBAR_H__ */


