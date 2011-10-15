/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* mate-mdi-child.h - definition of an abstract MDI child class.

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
*/

#ifndef __MATE_MDI_CHILD_H__
#define __MATE_MDI_CHILD_H__

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "libmateui/mate-app.h"
#include "libmateui/mate-app-helper.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_MDI_CHILD            (mate_mdi_child_get_type ())
#define MATE_MDI_CHILD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_MDI_CHILD, MateMDIChild))
#define MATE_MDI_CHILD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_MDI_CHILD, MateMDIChildClass))
#define MATE_IS_MDI_CHILD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_MDI_CHILD))
#define MATE_IS_MDI_CHILD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_MDI_CHILD))
#define MATE_MDI_CHILD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_MDI_CHILD, MateMDIChildClass))

typedef struct _MateMDIChild        MateMDIChild;
typedef struct _MateMDIChildClass   MateMDIChildClass;

/* MateMDIChild
 * is an abstract class. In order to use it, you have to either derive a
 * new class from it and set the proper virtual functions in its parent
 * MateMDIChildClass structure or use the MateMDIGenericChild class
 * that allows to specify the relevant functions on a per-instance basis
 * and can directly be used with MateMDI.
 */
struct _MateMDIChild
{
	GtkObject object;

	GtkObject *parent;

	gchar *name;

	GList *views;

	MateUIInfo *menu_template;

	gpointer reserved;
};

typedef GtkWidget *(*MateMDIChildViewCreator) (MateMDIChild *, gpointer);
typedef GList     *(*MateMDIChildMenuCreator) (MateMDIChild *, GtkWidget *, gpointer);
typedef gchar     *(*MateMDIChildConfigFunc)  (MateMDIChild *, gpointer);
typedef GtkWidget *(*MateMDIChildLabelFunc)   (MateMDIChild *, GtkWidget *, gpointer);

/* note that if you override the set_label virtual function, it should return
 * a new widget if its GtkWidget* parameter is NULL and modify and return the
 * old widget otherwise.
 * (see mate-mdi-child.c/mate_mdi_child_set_book_label() for an example).
 */

struct _MateMDIChildClass
{
	GtkObjectClass parent_class;

	/* these make no sense as signals, so we'll make them "virtual" functions */
	MateMDIChildViewCreator create_view;
	MateMDIChildMenuCreator create_menus;
	MateMDIChildConfigFunc  get_config_string;
	MateMDIChildLabelFunc   set_label;
};

GType         mate_mdi_child_get_type         (void);

GtkWidget     *mate_mdi_child_add_view        (MateMDIChild *mdi_child);

void          mate_mdi_child_remove_view      (MateMDIChild *mdi_child, GtkWidget *view);

void          mate_mdi_child_set_name         (MateMDIChild *mdi_child, const gchar *name);

void          mate_mdi_child_set_menu_template(MateMDIChild *mdi_child, MateUIInfo *menu_tmpl);

#ifdef __cplusplus
}
#endif

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* __MATE_MDI_CHILD_H__ */
