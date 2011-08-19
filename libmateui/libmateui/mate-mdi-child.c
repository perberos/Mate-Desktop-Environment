/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* mate-mdi-child.c - implementation of an abstract class for MDI children

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

#include <config.h>
#include <libmate/mate-macros.h>

#ifndef MATE_DISABLE_DEPRECATED_SOURCE

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <glib/gi18n-lib.h>

#include "mate-app.h"
#include "mate-app-helper.h"
#include "mate-mdi-child.h"
#include "mate-mdi.h"

static void       mate_mdi_child_class_init       (MateMDIChildClass *klass);
static void       mate_mdi_child_instance_init    (MateMDIChild *);
static void       mate_mdi_child_destroy          (GtkObject *);
static void       mate_mdi_child_finalize         (GObject *);

static GtkWidget *mate_mdi_child_set_label        (MateMDIChild *, GtkWidget *, gpointer);
static GtkWidget *mate_mdi_child_create_view      (MateMDIChild *);

/* declare the functions from mate-mdi.c that we need but are not public */
void _mate_mdi_child_list_menu_remove_item (MateMDI *, MateMDIChild *);
void _mate_mdi_child_list_menu_add_item    (MateMDI *, MateMDIChild *);

MATE_CLASS_BOILERPLATE (MateMDIChild, mate_mdi_child,
						 GtkObject, GTK_TYPE_OBJECT)


static void mate_mdi_child_class_init (MateMDIChildClass *klass)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass*)klass;
	gobject_class = (GObjectClass*)klass;

	object_class->destroy = mate_mdi_child_destroy;
	gobject_class->finalize = mate_mdi_child_finalize;

	klass->create_view = NULL;
	klass->create_menus = NULL;
	klass->get_config_string = NULL;
	klass->set_label = mate_mdi_child_set_label;
}

static void mate_mdi_child_instance_init (MateMDIChild *mdi_child)
{
	mdi_child->name = NULL;
	mdi_child->parent = NULL;
	mdi_child->views = NULL;
}

/* the default set_label function: returns a GtkLabel with child->name
 * if you provide your own, it should return a new widget if its old_label
 * parameter is NULL and modify and return the old widget otherwise. it
 * should (obviously) NOT call the parent class handler!
 */
static GtkWidget *mate_mdi_child_set_label (MateMDIChild *child, GtkWidget *old_label, gpointer data)
{
#ifdef MATE_ENABLE_DEBUG
	g_message("MateMDIChild: default set_label handler called!\n");
#endif

	if(old_label) {
		gtk_label_set_text(GTK_LABEL(old_label), child->name);
		return old_label;
	}
	else {
		GtkWidget *label;

		label = gtk_label_new(child->name);
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

		return label;
	}
}

static void mate_mdi_child_finalize (GObject *obj)
{
	MateMDIChild *mdi_child;

#ifdef MATE_ENABLE_DEBUG
	g_message("child finalization\n");
#endif

	mdi_child = MATE_MDI_CHILD(obj);

	g_free(mdi_child->name);
	mdi_child->name = NULL;

	if(G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize)(obj);
}

static void mate_mdi_child_destroy (GtkObject *obj)
{
	MateMDIChild *mdi_child;

#ifdef MATE_ENABLE_DEBUG
	g_message("MateMDIChild: destroying!\n");
#endif

	mdi_child = MATE_MDI_CHILD(obj);

	while(mdi_child->views)
		mate_mdi_child_remove_view(mdi_child, GTK_WIDGET(mdi_child->views->data));

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(obj);
}

/**
 * mate_mdi_child_add_view:
 * @mdi_child: A pointer to a MateMDIChild object.
 *
 * Description:
 * Creates a new view of a child (a GtkWidget) adds it to the list
 * of the views and returns a pointer to it. Virtual function
 * that has to be specified for classes derived from MateMDIChild
 * is used to create the new view.
 *
 * Return value:
 * A pointer to the new view.
 **/
GtkWidget *mate_mdi_child_add_view (MateMDIChild *mdi_child)
{
	GtkWidget *view = NULL;

	view = mate_mdi_child_create_view(mdi_child);

	if(view) {
		mdi_child->views = g_list_append(mdi_child->views, view);

		g_object_set_data(G_OBJECT(view), "MateMDIChild", mdi_child);
	}

	return view;
}

/**
 * mate_mdi_child_remove_view:
 * @mdi_child: A pointer to a MateMDIChild object.
 * @view: View to be removed.
 *
 * Description:
 * Removes view @view from the list of @mdi_child's views and
 * unrefs it.
 **/
void mate_mdi_child_remove_view(MateMDIChild *mdi_child, GtkWidget *view)
{
	mdi_child->views = g_list_remove(mdi_child->views, view);

	g_object_unref(view);
}

/**
 * mate_mdi_child_set_name:
 * @mdi_child: A pointer to a MateMDIChild object.
 * @name: String containing the new name for the child.
 *
 * Description:
 * Changes name of @mdi_child to @name. @name is duplicated and stored
 * in @mdi_child. If @mdi_child has already been added to MateMDI,
 * it also takes care of updating it.
 **/
void mate_mdi_child_set_name(MateMDIChild *mdi_child, const gchar *name)
{
	gchar *old_name = mdi_child->name;

	if(mdi_child->parent)
		_mate_mdi_child_list_menu_remove_item(MATE_MDI(mdi_child->parent), mdi_child);

	mdi_child->name = (gchar *)g_strdup(name);
	g_free(old_name);

	if(mdi_child->parent) {
		_mate_mdi_child_list_menu_add_item(MATE_MDI(mdi_child->parent), mdi_child);
		mate_mdi_update_child(MATE_MDI(mdi_child->parent), mdi_child);
	}
}

/**
 * mate_mdi_child_set_menu_template:
 * @mdi_child: A pointer to a MateMDIChild object.
 * @menu_tmpl: A MateUIInfo array describing the child specific menus.
 *
 * Description:
 * Sets the template for menus that are added and removed when differrent
 * children get activated. This way, each child can modify the MDI menubar
 * to suit its needs. If no template is set, the create_menus virtual
 * function will be used for creating these menus (it has to return a
 * GList of menu items). If no such function is specified, the menubar will
 * be unchanged by MDI children.
 **/
void mate_mdi_child_set_menu_template (MateMDIChild *mdi_child, MateUIInfo *menu_tmpl)
{
	mdi_child->menu_template = menu_tmpl;
}

static GtkWidget *mate_mdi_child_create_view (MateMDIChild *child)
{
	if(MATE_MDI_CHILD_GET_CLASS (child)->create_view)
		return MATE_MDI_CHILD_GET_CLASS(child)->create_view(child, NULL);

	return NULL;
}

#endif /* MATE_DISABLE_DEPRECATED_SOURCE */
