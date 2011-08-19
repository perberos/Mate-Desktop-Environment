/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Red Hat Software, The Free
 * Software Foundation, Miguel de Icaza, Federico Menu, Chris Toshok.
 * All rights reserved.
 *
 * This file is part of the Mate Library.
 *
 * The Mate Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Mate Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Mate Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  */
/*
  @NOTATION@
 */

/*
 *
 * Originally by Elliot Lee,
 *
 * improvements and rearrangement by Miguel,
 * and I don't know what you other people did :)
 *
 * Even more changes by Federico Mena.
 */

#ifndef MATE_APP_H
#define MATE_APP_H

#include <gtk/gtk.h>

#include <matecomponent/matecomponent-dock.h>

G_BEGIN_DECLS

#define MATE_APP_MENUBAR_NAME "Menubar"
#define MATE_APP_TOOLBAR_NAME "Toolbar"


#define MATE_TYPE_APP            (mate_app_get_type ())
#define MATE_APP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATE_TYPE_APP, MateApp))
#define MATE_APP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATE_TYPE_APP, MateAppClass))
#define MATE_IS_APP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATE_TYPE_APP))
#define MATE_IS_APP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATE_TYPE_APP))
#define MATE_APP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATE_TYPE_APP, MateAppClass))


typedef struct _MateApp        MateApp;
typedef struct _MateAppPrivate MateAppPrivate;
typedef struct _MateAppClass   MateAppClass;

struct _MateApp {
	GtkWindow parent_object;

	/* Application name. */
	gchar *name;

	/* Prefix for mate-config (used to save the layout).  */
	gchar *prefix;

        /* The dock.  */
        GtkWidget *dock;

	/* The status bar.  */
        GtkWidget *statusbar;

	/* The vbox widget that ties them.  */
	GtkWidget *vbox;

	/* The menubar.  This is a pointer to a widget contained into
           the dock.  */
	GtkWidget *menubar;

	/* The contents.  This is a pointer to dock->client_area.  */
	GtkWidget *contents;

	/* Dock layout.  */
	MateComponentDockLayout *layout;

	/* Main accelerator group for this window (hotkeys live here).  */
	GtkAccelGroup *accel_group;

	/* If TRUE, the application uses mate-config to retrieve and
           save the docking configuration automagically.  */
	guint enable_layout_config : 1;

	/*< private >*/
	MateAppPrivate *_priv;
};

struct _MateAppClass {
	GtkWindowClass parent_class;

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};


/* Standard Gtk function */
GType mate_app_get_type (void) G_GNUC_CONST;

/* Create a new (empty) application window.  You must specify the application's name (used
 * internally as an identifier).  The window title can be left as NULL, in which case the window's
 * title will not be set.
 */
GtkWidget *mate_app_new (const gchar *appname, const gchar *title);

/* Constructor for language bindings; you don't normally need this. */
void mate_app_construct (MateApp *app, const gchar *appname, const gchar *title);

/* Sets the menu bar of the application window */
void mate_app_set_menus (MateApp *app, GtkMenuBar *menubar);

/* Sets the main toolbar of the application window */
void mate_app_set_toolbar (MateApp *app, GtkToolbar *toolbar);

/* Sets the status bar of the application window */
void mate_app_set_statusbar (MateApp *app, GtkWidget *statusbar);

/* Sets the status bar of the application window, but uses the given
 * container widget rather than creating a new one. */
void mate_app_set_statusbar_custom (MateApp *app,
				     GtkWidget *container,
				     GtkWidget *statusbar);

/* Sets the content area of the application window */
void mate_app_set_contents (MateApp *app, GtkWidget *contents);

void mate_app_add_toolbar (MateApp *app,
			    GtkToolbar *toolbar,
			    const gchar *name,
			    MateComponentDockItemBehavior behavior,
			    MateComponentDockPlacement placement,
			    gint band_num,
			    gint band_position,
			    gint offset);

GtkWidget *mate_app_add_docked (MateApp *app,
				 GtkWidget *widget,
				 const gchar *name,
				 MateComponentDockItemBehavior behavior,
				 MateComponentDockPlacement placement,
				 gint band_num,
				 gint band_position,
				 gint offset);

void mate_app_add_dock_item (MateApp *app,
			      MateComponentDockItem *item,
			      MateComponentDockPlacement placement,
			      gint band_num,
			      gint band_position,
			      gint offset);

void mate_app_enable_layout_config (MateApp *app, gboolean enable);

MateComponentDock *mate_app_get_dock (MateApp *app);

MateComponentDockItem *mate_app_get_dock_item_by_name (MateApp *app,
						const gchar *name);

G_END_DECLS

#endif /* MATE_APP_H */
