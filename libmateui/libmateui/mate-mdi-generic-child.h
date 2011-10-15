/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* mate-mdi-generic-child.h - definition of a generic MDI child class

   Copyright (C) 1997 - 2001 Free Software Foundation

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Jaka Mocnik <jaka@gnu.org>
   Interp modifications: James Henstridge <james@daa.com.au>
*/

#ifndef __MATE_MDI_GENERIC_CHILD_H__
#define __MATE_MDI_GENERIC_CHILD_H__

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "mate-mdi-child.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_MDI_GENERIC_CHILD            (mate_mdi_generic_child_get_type ())
#define MATE_MDI_GENERIC_CHILD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_MDI_GENERIC_CHILD, MateMDIGenericChild))
#define MATE_MDI_GENERIC_CHILD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_MDI_GENERIC_CHILD, MateMDIGenericChildClass))
#define MATE_IS_MDI_GENERIC_CHILD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_MDI_GENERIC_CHILD))
#define MATE_IS_MDI_GENERIC_CHILD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_MDI_GENERIC_CHILD))
#define MATE_MDI_GENERIC_CHILD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_MDI_GENERIC_CHILD, MateMDIGenericChildClass))
/* The source backward-compatibility macro MATE_IS_MDI_MDI_CHILD(obj)
   is in mate-compat.h.  */

typedef struct _MateMDIGenericChild        MateMDIGenericChild;
typedef struct _MateMDIGenericChildClass   MateMDIGenericChildClass;

struct _MateMDIGenericChild {
	MateMDIChild mdi_child;

	/* if any of these are set they override the virtual functions
	   in MateMDIChildClass. create_view is mandatory, as no default
	   handler is provided, others may be NULL */
	MateMDIChildViewCreator create_view;
	MateMDIChildMenuCreator create_menus;
	MateMDIChildConfigFunc  get_config_string;
	MateMDIChildLabelFunc   set_label;

	GtkCallbackMarshal create_view_cbm, create_menus_cbm,
		               get_config_string_cbm, set_label_cbm;
	GDestroyNotify   create_view_dn, create_menus_dn,
		               get_config_string_dn, set_label_dn;
	gpointer           create_view_data, create_menus_data,
		               get_config_string_data, set_label_data;

	gpointer reserved;
};

struct _MateMDIGenericChildClass {
	MateMDIChildClass parent_class;
};

GType                 mate_mdi_generic_child_get_type (void);

MateMDIGenericChild *mate_mdi_generic_child_new     (const gchar *name);

void mate_mdi_generic_child_set_view_creator     (MateMDIGenericChild *child,
												   MateMDIChildViewCreator func,
                                                   gpointer data);
void mate_mdi_generic_child_set_view_creator_full(MateMDIGenericChild *child,
												   MateMDIChildViewCreator func,
												   GtkCallbackMarshal marshal,
												   gpointer data,
												   GDestroyNotify notify);
void mate_mdi_generic_child_set_menu_creator     (MateMDIGenericChild *child,
												   MateMDIChildMenuCreator func,
                                                   gpointer data);
void mate_mdi_generic_child_set_menu_creator_full(MateMDIGenericChild *child,
												   MateMDIChildMenuCreator func,
												   GtkCallbackMarshal marshal,
												   gpointer data,
												   GDestroyNotify notify);
void mate_mdi_generic_child_set_config_func      (MateMDIGenericChild *child,
												   MateMDIChildConfigFunc func,
                                                   gpointer data);
void mate_mdi_generic_child_set_config_func_full (MateMDIGenericChild *child,
												   MateMDIChildConfigFunc func,
												   GtkCallbackMarshal marshal,
												   gpointer data,
												   GDestroyNotify notify);
void mate_mdi_generic_child_set_label_func       (MateMDIGenericChild *child,
												   MateMDIChildLabelFunc func,
                                                   gpointer data);
void mate_mdi_generic_child_set_label_func_full  (MateMDIGenericChild *child,
												   MateMDIChildLabelFunc func,
												   GtkCallbackMarshal marshal,
												   gpointer data,
												   GDestroyNotify notify);


#ifdef __cplusplus
}
#endif

#endif /* MATE_DISABLE_DEPRECATED */

#endif
