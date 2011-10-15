/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* mate-mdi.h - definition of a Mate MDI object

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

#ifndef __MATE_MDI_H__
#define __MATE_MDI_H__

#ifndef MATE_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "mate-app.h"
#include "mate-app-helper.h"
#include "mate-mdi-child.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATE_TYPE_MDI            (mate_mdi_get_type ())
#define MATE_MDI(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_MDI, MateMDI))
#define MATE_MDI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_MDI, MateMDIClass))
#define MATE_IS_MDI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_MDI))
#define MATE_IS_MDI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_MDI))
#define MATE_MDI_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_MDI, MateMDIClass))

typedef struct _MateMDI        MateMDI;
typedef struct _MateMDIClass   MateMDIClass;

typedef enum {
	MATE_MDI_NOTEBOOK,
	MATE_MDI_TOPLEVEL,
	MATE_MDI_MODAL,
	MATE_MDI_DEFAULT_MODE = 42
} MateMDIMode;

struct _MateMDI {
	GtkObject object;

	MateMDIMode mode;

	GtkPositionType tab_pos;

	guint signal_id;
	guint in_drag : 1;

	gchar *appname, *title;

	MateUIInfo *menu_template;
	MateUIInfo *toolbar_template;

    /* probably only one of these would do, but... redundancy rules ;) */
	MateMDIChild *active_child;
	GtkWidget *active_view;
	MateApp *active_window;

	GList *windows;     /* toplevel windows - MateApp widgets */
	GList *children;    /* children - MateMDIChild objects*/

	GSList *registered; /* see comment for mate_mdi_(un)register() functions below for an explanation. */

    /* paths for insertion of mdi_child specific menus and mdi_child list menu via
       mate-app-helper routines */
	gchar *child_menu_path;
	gchar *child_list_path;

	gpointer reserved;
};

struct _MateMDIClass {
	GtkObjectClass parent_class;

	gint        (*add_child)(MateMDI *, MateMDIChild *);
	gint        (*remove_child)(MateMDI *, MateMDIChild *);
	gint        (*add_view)(MateMDI *, GtkWidget *);
	gint        (*remove_view)(MateMDI *, GtkWidget *);
	void        (*child_changed)(MateMDI *, MateMDIChild *);
	void        (*view_changed)(MateMDI *, GtkWidget *);
	void        (*app_created)(MateMDI *, MateApp *);
};

/*
 * description of MateMDI signals:
 *
 * gint add_child(MateMDI *, MateMDIChild *)
 * gint add_view(MateMDI *, GtkWidget *)
 *   are called before actually adding a mdi_child or a view to the MDI. if the handler returns
 *   TRUE, the action proceeds otherwise the mdi_child or view are not added.
 *
 * gint remove_child(MateMDI *, MateMDIChild *)
 * gint remove_view(MateMDI *, GtkWidget *)
 *   are called before removing mdi_child or view. the handler should return true if the object
 *   is to be removed from MDI
 *
 * void child_changed(MateMDI *, MateMDIChild *)
 *   gets called each time when active child is changed with the second argument
 *   pointing to the old child. mdi->active_view and mdi->active_child still already
 *   hold the new values
 *
 * void view_changed(MateMDI *, GtkWidget *)
 *   is emitted whenever a view is changed, regardless of it being the view of the same child as
 *   the old view or not. the second argument points to the old view, mdi->active_view and
 *   mdi->active_child hold the new values. if the child has also been changed, this signal is
 *   emitted after the child_changed signal.
 *
 * void app_created(MateMDI *, MateApp *)
 *   is called with each newly created MateApp to allow the MDI user to customize it (add a
 *   statusbar, toolbars or menubar if the method with MateUIInfo templates is not sufficient,
 *   etc.).
 *   no contents may be set since MateMDI uses them for storing either a view of a child
 *   or a notebook
 */

GType          mate_mdi_get_type            (void);

GtkObject     *mate_mdi_new                (const gchar *appname, const gchar *title);

void          mate_mdi_set_mode            (MateMDI *mdi, MateMDIMode mode);

/* setting the menu and toolbar stuff */
void          mate_mdi_set_menubar_template(MateMDI *mdi, MateUIInfo *menu_tmpl);
void          mate_mdi_set_toolbar_template(MateMDI *mdi, MateUIInfo *tbar_tmpl);
void          mate_mdi_set_child_menu_path (MateMDI *mdi, const gchar *path);
void          mate_mdi_set_child_list_path (MateMDI *mdi, const gchar *path);

/* manipulating views */
gint          mate_mdi_add_view            (MateMDI *mdi, MateMDIChild *child);
gint          mate_mdi_add_toplevel_view   (MateMDI *mdi, MateMDIChild *child);
gint          mate_mdi_remove_view         (MateMDI *mdi, GtkWidget *view, gint force);

GtkWidget     *mate_mdi_get_active_view    (MateMDI *mdi);
void          mate_mdi_set_active_view     (MateMDI *mdi, GtkWidget *view);

/* manipulating children */
gint          mate_mdi_add_child           (MateMDI *mdi, MateMDIChild *child);
gint          mate_mdi_remove_child        (MateMDI *mdi, MateMDIChild *child, gint force);
gint          mate_mdi_remove_all          (MateMDI *mdi, gint force);

void          mate_mdi_open_toplevel       (MateMDI *mdi);

void          mate_mdi_update_child        (MateMDI *mdi, MateMDIChild *child);

MateMDIChild *mate_mdi_get_active_child   (MateMDI *mdi);
MateMDIChild *mate_mdi_find_child         (MateMDI *mdi, const gchar *name);

MateApp      *mate_mdi_get_active_window  (MateMDI *mdi);

/*
 * the following two functions are here to make life easier if an application
 * creates objects (like opening a window) that should "keep the application
 * alive" even if there are no MDI windows open. any such windows should be
 * registered with the MDI: as long as there is a window registered, the MDI
 * will not destroy itself (even if the last of its windows is closed). on the
 * other hand, closing the last MDI window when no objects are registered
 * with the MDI will result in MDI being gtk_object_destroy()ed.
 */
void          mate_mdi_register            (MateMDI *mdi, GtkObject *object);
void          mate_mdi_unregister          (MateMDI *mdi, GtkObject *object);

/*
 * convenience functions for retrieveing MateMDIChild and MateApp
 * objects associated with a particular view and for retrieveing the
 * visible view of a certain MateApp.
 */
MateApp      *mate_mdi_get_app_from_view    (GtkWidget *view);
MateMDIChild *mate_mdi_get_child_from_view  (GtkWidget *view);
GtkWidget     *mate_mdi_get_view_from_window (MateMDI *mdi, MateApp *app);

/* the following functions are used to obtain pointers to the MateUIInfo
 * structures for a specified MDI MateApp widget. this might be useful for
 * enabling/disabling menu items (via ui_info[i]->widget) when certain events
 * happen or selecting the default menuitem in a radio item group. these
 * MateUIInfo structures are exact copies of the template MateUIInfo trees
 * and are non-NULL only if templates are used for menu/toolbar creation.
 */
MateUIInfo   *mate_mdi_get_menubar_info     (MateApp *app);
MateUIInfo   *mate_mdi_get_toolbar_info     (MateApp *app);
MateUIInfo   *mate_mdi_get_child_menu_info  (MateApp *app);

G_END_DECLS

#endif /* MATE_DISABLE_DEPRECATED */

#endif /* __MATE_MDI_H__ */
