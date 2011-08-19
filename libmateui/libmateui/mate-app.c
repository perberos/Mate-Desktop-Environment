/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
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
 * Originally by Elliot Lee,
 *
 * improvements and rearrangement by Miguel,
 * and I don't know what you other people did :)
 *
 * Even more changes by Federico Mena.
 *
 * Toolbar separators and configurable relief by Andrew Veliath.
 *
 * Modified by Ettore Perazzoli to support MateComponentDock.
 *
 */

#include "config.h"
#include <libmate/mate-macros.h>

#include <string.h>
#include <gtk/gtk.h>

/* This is for matecomponent_dock_item_set_locked */
#define MATECOMPONENT_UI_INTERNAL

/* Must be before all other mate includes!! */
#include <glib/gi18n-lib.h>

#include <mateconf/mateconf-client.h>
#include <libmate/mate-program.h>
#include <libmate/mate-util.h>
#include <libmate/mate-config.h>
#include <matecomponent/matecomponent-dock.h>
#include "mate-app-helper.h"
#include "mate-uidefs.h"
#include "mate-mateconf-ui.h"
#include "mate-window.h"
#include "mate-window-icon.h"
#include "mate-ui-init.h"

#include "mate-app.h"

#define LAYOUT_CONFIG_PATH      "Placement/Dock"

struct _MateAppPrivate
{
	int dummy;
	/* Nothing right now, needs to get filled with the private things */
	/* XXX: When stuff is added, uncomment the allocation in the
	 * mate_app_instance_init function! */
};


static void mate_app_destroy     (GtkObject     *object);
static void mate_app_finalize    (GObject       *object);
static void mate_app_show        (GtkWidget     *widget);
static void mate_app_get_property   (GObject       *object,
				   guint          param_id,
				   GValue        *value,
				   GParamSpec    *pspec);
static void mate_app_set_property   (GObject       *object,
				   guint          param_id,
				   const GValue  *value,
				   GParamSpec    *pspec);

static gchar *read_layout_config  (MateApp      *app);
static void   write_layout_config (MateApp      *app,
				   MateComponentDockLayout *layout);
static void   layout_changed      (GtkWidget     *widget,
				   gpointer       data);

/* define _get_type and parent_class */
MATE_CLASS_BOILERPLATE (MateApp, mate_app,
			 GtkWindow, GTK_TYPE_WINDOW)

static gchar *
read_layout_config (MateApp *app)
{
	gchar *s;

	mate_config_push_prefix (app->prefix);
	s = mate_config_get_string (LAYOUT_CONFIG_PATH);
	mate_config_pop_prefix ();

	return s;
}

static void
write_layout_config (MateApp *app, MateComponentDockLayout *layout)
{
	gchar *s;

	s = matecomponent_dock_layout_create_string (layout);
	mate_config_push_prefix (app->prefix);
	mate_config_set_string (LAYOUT_CONFIG_PATH, s);
	mate_config_pop_prefix ();
	mate_config_sync ();

	g_free (s);
}

enum {
	PROP_0,
	PROP_APP_ID
};

static void
mate_app_class_init (MateAppClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	gobject_class->finalize = mate_app_finalize;
	gobject_class->set_property = mate_app_set_property;
	gobject_class->get_property = mate_app_get_property;

	object_class->destroy = mate_app_destroy;

	widget_class->show = mate_app_show;

	g_object_class_install_property (gobject_class,
				      PROP_APP_ID,
				      g_param_spec_string ("app_id",
							   _("App ID"),
							   _("The application ID string"),
							   g_get_prgname (),
							   (G_PARAM_READABLE |
							    G_PARAM_WRITABLE |
							    G_PARAM_CONSTRUCT)));
}

static void
mate_app_set_property (GObject       *object,
			guint          param_id,
			const GValue  *value,
			GParamSpec    *pspec)
{
	MateApp *app = MATE_APP (object);

	switch (param_id) {
	case PROP_APP_ID:
		g_free (app->name);
		app->name = g_value_dup_string (value);
		g_free (app->prefix);
		app->prefix = g_strconcat("/", app->name, "/", NULL);
		break;
	}
}

static void
mate_app_get_property (GObject       *object,
			guint          param_id,
			GValue        *value,
			GParamSpec    *pspec)
{
	MateApp *app = MATE_APP (object);

	switch (param_id) {
	case PROP_APP_ID:
		g_value_set_string (value, app->name);
		break;
	}
}

static void
mate_app_instance_init (MateApp *app)
{
	const char *icons = NULL;
	MateProgram * program;
		
	app->_priv = NULL;
	/* XXX: when there is some private stuff enable this
	app->_priv = g_new0(MateAppPrivate, 1);
	*/
	app->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (app), app->accel_group);
	
	app->vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (app), app->vbox);

	app->dock = matecomponent_dock_new ();
	app->layout = matecomponent_dock_layout_new ();

	gtk_box_pack_start (GTK_BOX (app->vbox), app->dock,
			    TRUE, TRUE, 0);

	g_signal_connect (app->dock, "layout_changed", G_CALLBACK (layout_changed), app);
	
	app->enable_layout_config = TRUE;

	program = mate_program_get ();
	if (program == NULL)
		g_error ("You must call mate_program_init() before creating a MateApp");
	g_object_get (G_OBJECT (program),
		      LIBMATEUI_PARAM_DEFAULT_ICON, &icons,
		      NULL);

	if (icons && *icons) {
		char **files = g_strsplit (icons, ";", -1);
		mate_window_icon_set_from_file_list (GTK_WINDOW (app),
						      (const char **)files);
		g_strfreev (files);
	}
}

static void
remove_notification_cb (MateComponentDockItem *item, gpointer data)
{
	MateConfClient *client;
	guint notify_id;

	notify_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "mate-app-mateconf-notify-id"));

	if (notify_id == 0)
		return;
	
	client = mateconf_client_get_default ();
	mateconf_client_notify_remove (client, notify_id);
	g_object_unref (client);
	g_object_set_data (G_OBJECT (item), "mate-app-mateconf-notify-id", NULL);
	
}

static void
dock_item_changed_notify (MateConfClient* client,
			  guint cnxn_id,
			  MateConfEntry *entry,
			  gpointer user_data)
{
	MateComponentDockItem *item;
	guint notify_id;
	
	item = MATECOMPONENT_DOCK_ITEM (user_data);
	notify_id = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "mate-app-mateconf-notify-id"));

	if (notify_id == cnxn_id &&
	    entry->value->type == MATECONF_VALUE_BOOL) {
		gboolean detachable;

		detachable = mateconf_value_get_bool (entry->value);
		
		/* Update */
		GDK_THREADS_ENTER();
		matecomponent_dock_item_set_locked (item, !detachable);
		GDK_THREADS_LEAVE();
	}
}

static void
setup_notification_for_dock_item (MateComponentDockItem *item, const char *key_name)
{
	guint notify_id;
	MateConfClient *client;

	client = mateconf_client_get_default ();
	notify_id = mateconf_client_notify_add (client, key_name,
					     dock_item_changed_notify,
					     item, NULL, NULL);

	g_object_set_data (G_OBJECT (item), "mate-app-mateconf-notify-id", GINT_TO_POINTER (notify_id));
	g_signal_connect (item, "destroy", G_CALLBACK (remove_notification_cb), NULL);
}

static void
mate_app_show (GtkWidget *widget)
{
	MateApp *app;

	app = MATE_APP (widget);

	if (app->layout != NULL) {
		if (app->enable_layout_config) {
			gchar *s;

			/* Override the layout with the user's saved
			   configuration.  */
			s = read_layout_config (app);
			matecomponent_dock_layout_parse_string (app->layout, s);
			g_free (s);
		}

		matecomponent_dock_add_from_layout (MATECOMPONENT_DOCK (app->dock),
					    app->layout);

		if (app->enable_layout_config)
			write_layout_config (app, app->layout);

		g_object_unref (G_OBJECT (app->layout));
		app->layout = NULL;
	}
			
	gtk_widget_show (app->vbox);
	gtk_widget_show (app->dock);

	MATE_CALL_PARENT (GTK_WIDGET_CLASS, show, (widget));
}

static void
layout_changed (GtkWidget *w, gpointer data)
{
	MateApp *app;

	g_return_if_fail (MATE_IS_APP (data));
	g_return_if_fail (MATECOMPONENT_IS_DOCK (w));

	app = MATE_APP (data);

	if (app->enable_layout_config) {
		MateComponentDockLayout *layout;

		layout = matecomponent_dock_get_layout (MATECOMPONENT_DOCK (app->dock));
		write_layout_config (app, layout);
		g_object_unref (G_OBJECT (layout));
	}
}

/**
 * mate_app_new:
 * @appname: Name of program, used in file names and paths.
 * @title: Window title for application.
 *
 * Description:
 * Create a new (empty) application window. You must specify the @appname (used
 * internally as an identifier).  The @title param can be left as NULL, in
 * which case the window's title will not be set.
 *
 * Returns: Pointer to new #MateApp object.
 **/

GtkWidget *
mate_app_new (const gchar *appname, const gchar *title)
{
	GtkObject *app;

	g_return_val_if_fail (appname != NULL, NULL);

	if (title != NULL) {
		app = g_object_new (MATE_TYPE_APP,
				    "app_id", appname,
				    "title", title,
				    NULL);
	} else {
		app = g_object_new (MATE_TYPE_APP,
				    "app_id", appname,
				    NULL);
	}
	return GTK_WIDGET (app);
}

/**
 * mate_app_construct:
 * @app: A newly created #MateApp object.
 * @appname: Name of program, using in file names and paths.
 * @title: Window title for application.
 *
 * Description:
 * Constructor for language bindings; you don't normally need this.
 **/

void 
mate_app_construct (MateApp *app, const gchar *appname, const gchar *title)
{
	g_return_if_fail (appname != NULL);

	if (title != NULL) {
		g_object_set (G_OBJECT (app),
			      "app_id", appname,
			      "title", title,
			      NULL);
	} else {
		g_object_set (G_OBJECT (app),
			      "app_id", appname,
			      NULL);
	}
}

static void
mate_app_destroy (GtkObject *object)
{
	MateApp *app;

	/* remember, destroy can be run multiple times! */
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_APP (object));

	app = MATE_APP (object);

	if (app->accel_group != NULL) {
		g_object_unref (G_OBJECT (app->accel_group));
		app->accel_group = NULL;
	}

	if (app->layout != NULL) {
		g_object_unref (app->layout);
		app->layout = NULL;
	}

	MATE_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
mate_app_finalize (GObject *object)
{
	MateApp *app;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (MATE_IS_APP (object));

	app = MATE_APP (object);

	g_free (app->name);
	app->name = NULL;
	g_free (app->prefix);
	app->prefix = NULL;

	/* Free private data */
	g_free (app->_priv);
	app->_priv = NULL;

	MATE_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

/* Callback for an app's contents' parent_set signal.  We set the app->contents
 * to NULL and detach the signal.
 */
static void
contents_parent_set (GtkWidget *widget, GtkWidget *previous_parent, gpointer data)
{
	MateApp *app;

	app = MATE_APP (data);

	g_assert (previous_parent == app->dock);

	g_signal_handlers_disconnect_by_func (widget, contents_parent_set, data);

	app->contents = NULL;
}

/**
 * mate_app_set_contents:
 * @app: A #MateApp instance.
 * @contents: Widget to be application content area.
 *
 * Description:
 * Sets the content area of the main window of @app.
 **/
void
mate_app_set_contents (MateApp *app, GtkWidget *contents)
{
	GtkWidget *new_contents;

	g_return_if_fail (app != NULL);
	g_return_if_fail (MATE_IS_APP(app));
	g_return_if_fail (app->dock != NULL);
	g_return_if_fail (contents != NULL);
	g_return_if_fail (GTK_IS_WIDGET (contents));

	matecomponent_dock_set_client_area (MATECOMPONENT_DOCK (app->dock), contents);

	/* Re-fetch it in case it did not change */
	new_contents = matecomponent_dock_get_client_area (MATECOMPONENT_DOCK (app->dock));

	if (new_contents == contents && new_contents != app->contents) {
		gtk_widget_show (new_contents);
		g_signal_connect (G_OBJECT (new_contents), "parent_set",
				  G_CALLBACK (contents_parent_set),
				  app);

		app->contents = new_contents;
	}
}

/**
 * mate_app_set_menus:
 * @app: A #MateApp instance.
 * @menubar: Menu bar widget for main application window.
 *
 * Description:
 * Sets the menu bar of the application window.
 **/

void
mate_app_set_menus (MateApp *app, GtkMenuBar *menubar)
{
	GtkWidget *dock_item;
	GtkAccelGroup *ag;
	MateComponentDockItemBehavior behavior;

	g_return_if_fail(app != NULL);
	g_return_if_fail(MATE_IS_APP(app));
	g_return_if_fail(app->menubar == NULL);
	g_return_if_fail(menubar != NULL);
	g_return_if_fail(GTK_IS_MENU_BAR(menubar));

	behavior = (MATECOMPONENT_DOCK_ITEM_BEH_EXCLUSIVE
		    | MATECOMPONENT_DOCK_ITEM_BEH_NEVER_VERTICAL);
	
	if ( ! mate_mateconf_get_bool ("/desktop/mate/interface/menubar_detachable"))
		behavior |= MATECOMPONENT_DOCK_ITEM_BEH_LOCKED;

	dock_item = matecomponent_dock_item_new (MATE_APP_MENUBAR_NAME,
					 behavior);
	setup_notification_for_dock_item (MATECOMPONENT_DOCK_ITEM (dock_item), "/desktop/mate/interface/menubar_detachable");
	gtk_container_add (GTK_CONTAINER (dock_item), GTK_WIDGET (menubar));

	app->menubar = GTK_WIDGET (menubar);
	
	matecomponent_dock_item_set_shadow_type (MATECOMPONENT_DOCK_ITEM (dock_item),
					  GTK_SHADOW_NONE);

	if (app->layout != NULL)
		matecomponent_dock_layout_add_item (app->layout,
					    MATECOMPONENT_DOCK_ITEM (dock_item),
					    MATECOMPONENT_DOCK_TOP,
					    0, 0, 0);
	else
		matecomponent_dock_add_item(MATECOMPONENT_DOCK(app->dock),
				    MATECOMPONENT_DOCK_ITEM (dock_item),
				    MATECOMPONENT_DOCK_TOP,
				    0, 0, 0, TRUE);

	gtk_widget_show (GTK_WIDGET (menubar));
	gtk_widget_show (GTK_WIDGET (dock_item));

	ag = g_object_get_data(G_OBJECT(app), "GtkAccelGroup");
	if (ag && !g_slist_find(gtk_accel_groups_from_object (G_OBJECT (app)), ag))
	        gtk_window_add_accel_group(GTK_WINDOW(app), ag);
}


/**
 * mate_app_set_statusbar:
 * @app: A #MateApp instance
 * @statusbar: Statusbar widget for main app window
 *
 * Description:
 * Sets the status bar of the application window.
 **/

void
mate_app_set_statusbar (MateApp *app,
		         GtkWidget *statusbar)
{
	GtkWidget *hbox;

	g_return_if_fail(app != NULL);
	g_return_if_fail(MATE_IS_APP(app));
	g_return_if_fail(statusbar != NULL);
	g_return_if_fail(app->statusbar == NULL);

	app->statusbar = GTK_WIDGET(statusbar);
	gtk_widget_show(app->statusbar);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 0);
	gtk_box_pack_start(GTK_BOX(hbox), statusbar, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	gtk_box_pack_start (GTK_BOX (app->vbox), hbox, FALSE, FALSE, 0);
}

/**
 * mate_app_set_statusbar_custom:
 * @app: A #MateApp instance
 * @container: container widget containing the statusbar
 * @statusbar: Statusbar widget for main app window
 *
 * Description:
 * Sets the status bar of the application window, but use @container
 * as its container.
 *
 **/

void
mate_app_set_statusbar_custom (MateApp *app,
				GtkWidget *container,
				GtkWidget *statusbar)
{
	g_return_if_fail(app != NULL);
	g_return_if_fail(MATE_IS_APP(app));
	g_return_if_fail(container != NULL);
	g_return_if_fail(GTK_IS_CONTAINER(container));
	g_return_if_fail(statusbar != NULL);
	g_return_if_fail(app->statusbar == NULL);

	app->statusbar = GTK_WIDGET(statusbar);

	gtk_box_pack_start (GTK_BOX (app->vbox), container, FALSE, FALSE, 0);
}

/**
 * mate_app_add_toolbar:
 * @app: A #MateApp widget
 * @toolbar: Toolbar to be added to @app's dock
 * @name: Name for the dock item that will contain @toolbar
 * @behavior: Behavior for the new dock item
 * @placement: Placement for the new dock item
 * @band_num: Number of the band where the dock item should be placed
 * @band_position: Position of the new dock item in band @band_num
 * @offset: Offset from the previous dock item in the band; if there is
 * no previous item, offset from the beginning of the band.
 *
 * Create a new #MateComponentDockItem widget containing @toolbar, and add it
 * to @app's dock with the specified layout information.  Notice that,
 * if automatic layout configuration is enabled, the layout is
 * overridden by the saved configuration, if any.
 **/
void
mate_app_add_toolbar (MateApp *app,
		       GtkToolbar *toolbar,
		       const gchar *name,
		       MateComponentDockItemBehavior behavior,
		       MateComponentDockPlacement placement,
		       gint band_num,
		       gint band_position,
		       gint offset)
{

	GtkWidget *dock_item;
	GtkAccelGroup *ag;

	g_return_if_fail(app != NULL);
	g_return_if_fail(MATE_IS_APP(app));
	g_return_if_fail(toolbar != NULL);

	dock_item = matecomponent_dock_item_new (name, behavior);
	setup_notification_for_dock_item (MATECOMPONENT_DOCK_ITEM (dock_item), "/desktop/mate/interface/toolbar_detachable");
	
	gtk_container_add (GTK_CONTAINER (dock_item), GTK_WIDGET (toolbar));

	if(app->layout)
		matecomponent_dock_layout_add_item (app->layout,
					    MATECOMPONENT_DOCK_ITEM (dock_item),
					    placement,
					    band_num,
					    band_position,
					    offset);
	else
		matecomponent_dock_add_item (MATECOMPONENT_DOCK(app->dock),
				     MATECOMPONENT_DOCK_ITEM (dock_item),
				     placement,
				     band_num,
				     band_position,
				     offset,
				     TRUE);


	mate_app_setup_toolbar(toolbar, MATECOMPONENT_DOCK_ITEM(dock_item));
	
	gtk_widget_show (GTK_WIDGET (toolbar));
	gtk_widget_show (GTK_WIDGET (dock_item));

	ag = g_object_get_data(G_OBJECT(app), "GtkAccelGroup");
	if (ag && !g_slist_find(gtk_accel_groups_from_object (G_OBJECT (app)), ag))
	        gtk_window_add_accel_group(GTK_WINDOW(app), ag);
}

/**
 * mate_app_set_toolbar:
 * @app: A #MateApp instance.
 * @toolbar: Toolbar widget for main app window.
 *
 * Description:
 * Sets the main toolbar of the application window.
 **/

void
mate_app_set_toolbar (MateApp *app,
		       GtkToolbar *toolbar)
{
	MateComponentDockItemBehavior behavior;

	/* Making dock items containing toolbars use
	   `MATECOMPONENT_DOCK_ITEM_BEH_EXCLUSIVE' is not really a
	   requirement.  We only do this for backwards compatibility.  */
	behavior = MATECOMPONENT_DOCK_ITEM_BEH_EXCLUSIVE;
	
	if ( ! mate_mateconf_get_bool ("/desktop/mate/interface/toolbar_detachable"))
		behavior |= MATECOMPONENT_DOCK_ITEM_BEH_LOCKED;
	
	mate_app_add_toolbar (app, toolbar,
			       MATE_APP_TOOLBAR_NAME,
			       behavior,
			       MATECOMPONENT_DOCK_TOP,
			       1, 0, 0);
}


/**
 * mate_app_add_dock_item:
 * @app: A #MateApp widget.
 * @item: Dock item to be added to @app's dock.
 * @placement: Placement for the dock item.
 * @band_num: Number of the band where the dock item should be placed.
 * @band_position: Position of the dock item in band @band_num.
 * @offset: Offset from the previous dock item in the band; if there is
 * no previous item, offset from the beginning of the band.
 * 
 * Add @item according to the specified layout information.  Notice
 * that, if automatic layout configuration is enabled, the layout is
 * overridden by the saved configuration, if any.
 **/
void
mate_app_add_dock_item (MateApp *app,
			 MateComponentDockItem *item,
			 MateComponentDockPlacement placement,
			 gint band_num,
			 gint band_position,
			 gint offset)
{
	if (app->layout)
		matecomponent_dock_layout_add_item (app->layout,
					    MATECOMPONENT_DOCK_ITEM (item),
					    placement,
					    band_num,
					    band_position,
					    offset);
	else
		matecomponent_dock_add_item (MATECOMPONENT_DOCK(app->dock),
				     MATECOMPONENT_DOCK_ITEM( item),
				     placement,
				     band_num,
				     band_position,
				     offset,
				     FALSE);

	g_signal_emit_by_name (app->dock, "layout_changed", app);
}

/**
 * mate_app_add_docked:
 * @app: A #MateApp widget.
 * @widget: Widget to be added to the #MateApp.
 * @name: Name for the new dock item.
 * @behavior: Behavior for the new dock item.
 * @placement: Placement for the new dock item.
 * @band_num: Number of the band where the dock item should be placed.
 * @band_position: Position of the new dock item in band @band_num.
 * @offset: Offset from the previous dock item in the band; if there is
 * no previous item, offset from the beginning of the band.
 * 
 * Add @widget as a dock item according to the specified layout
 * information.  Notice that, if automatic layout configuration is
 * enabled, the layout is overridden by the saved configuration, if
 * any.
 *
 * Returns: The dock item used to contain the widget.
 **/
GtkWidget *
mate_app_add_docked (MateApp *app,
		      GtkWidget *widget,
		      const gchar *name,
		      MateComponentDockItemBehavior behavior,
		      MateComponentDockPlacement placement,
		      gint band_num,
		      gint band_position,
		      gint offset)
{
	GtkWidget *item;

	item = matecomponent_dock_item_new (name, behavior);
	setup_notification_for_dock_item (MATECOMPONENT_DOCK_ITEM (item), "/desktop/mate/interface/toolbar_detachable");
	gtk_container_add (GTK_CONTAINER (item), widget);
	mate_app_add_dock_item (app, MATECOMPONENT_DOCK_ITEM (item),
				 placement, band_num, band_position, offset);

	return item;
}

/**
 * mate_app_enable_layout_config:
 * @app: A #MateApp widget.
 * @enable: Boolean specifying whether automatic configuration saving
 * is enabled.
 * 
 * Specify whether @app should automatically save the dock's
 * layout configuration via mate-config whenever it changes or not.
 **/
void
mate_app_enable_layout_config (MateApp *app, gboolean enable)
{
	app->enable_layout_config = enable;
}

/**
 * mate_app_get_dock_item_by_name:
 * @app: A #MateApp widget.
 * @name: Name of the dock item to retrieve.
 * 
 * Retrieve the dock item whose name matches @name.
 * 
 * Returns: The retrieved dock item.
 **/
MateComponentDockItem *
mate_app_get_dock_item_by_name (MateApp *app,
				 const gchar *name)
{
	MateComponentDockItem *item;

	item = matecomponent_dock_get_item_by_name (MATECOMPONENT_DOCK (app->dock), name,
					    NULL, NULL, NULL, NULL);

	if (item == NULL && app->layout != NULL) {
		MateComponentDockLayoutItem *i;

		i = matecomponent_dock_layout_get_item_by_name (app->layout,
							name);
		if (i == NULL)
			return NULL;

		return i->item;
	} else {
		return item;
	}
}

/**
 * mate_app_get_dock:
 * @app: A #MateApp widget
 * 
 * Retrieves the #MateComponentDock widget contained in @app.
 * 
 * Returns: The relevant #MateComponentDock widget.
 **/
MateComponentDock *
mate_app_get_dock (MateApp *app)
{
	return MATECOMPONENT_DOCK (app->dock);
}
