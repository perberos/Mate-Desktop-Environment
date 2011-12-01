/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-win.c: The MateComponent Window implementation.
 *
 * Author:
 *	Michael Meeks (michael@ximian.com)
 *
 * Copyright 2000 Ximian, Inc.
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include <matecomponent/matecomponent-dock-item.h>
#include <matecomponent/matecomponent-dock.h>
#include <matecomponent/matecomponent-window.h>
#include <libmatecomponent.h>
#include <glib/gi18n-lib.h>

#include <matecomponent/matecomponent-ui-preferences.h>
#include <matecomponent/matecomponent-ui-engine.h>
#include <matecomponent/matecomponent-ui-private.h>
#include <matecomponent/matecomponent-ui-sync-menu.h>
#include <matecomponent/matecomponent-ui-sync-keys.h>
#include <matecomponent/matecomponent-ui-sync-status.h>
#include <matecomponent/matecomponent-ui-sync-toolbar.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

G_DEFINE_TYPE (MateComponentWindow, matecomponent_window, GTK_TYPE_WINDOW)

struct _MateComponentWindowPrivate {
	MateComponentUIEngine *engine;

	MateComponentUISync   *sync_menu;
	MateComponentUISync   *sync_keys;
	MateComponentUISync   *sync_status;
	MateComponentUISync   *sync_toolbar;

	MateComponentDock     *dock;

	MateComponentDockItem *menu_item;
	GtkMenuBar     *menu;

	GtkAccelGroup  *accel_group;

	char           *name;		/* Win name */
	char           *prefix;		/* Win prefix */

	GtkBox         *status;
};

enum {
	PROP_0,
	PROP_WIN_NAME
};

/**
 * matecomponent_window_remove_popup:
 * @win: the window
 * @path: the path
 *
 * Remove the popup at @path
 **/
void
matecomponent_window_remove_popup (MateComponentWindow     *win,
			    const char    *path)
{
	g_return_if_fail (path != NULL);
	g_return_if_fail (MATECOMPONENT_IS_WINDOW (win));

	matecomponent_ui_sync_menu_remove_popup (
		MATECOMPONENT_UI_SYNC_MENU (win->priv->sync_menu), path);
}

/**
 * matecomponent_window_add_popup:
 * @win: the window
 * @menu: the menu widget
 * @path: the path
 *
 * Add a popup @menu at @path
 **/
void
matecomponent_window_add_popup (MateComponentWindow *win,
			 GtkMenu      *menu,
			 const char   *path)
{
	g_return_if_fail (path != NULL);
	g_return_if_fail (MATECOMPONENT_IS_WINDOW (win));

	matecomponent_ui_sync_menu_add_popup (
		MATECOMPONENT_UI_SYNC_MENU (win->priv->sync_menu), menu, path);
}

/**
 * matecomponent_window_set_contents:
 * @win: the matecomponent window
 * @contents: the new widget for it to contain.
 *
 * Insert a widget into the main window contents.
 **/
void
matecomponent_window_set_contents (MateComponentWindow *win,
			    GtkWidget    *contents)
{
	g_return_if_fail (win != NULL);
	g_return_if_fail (win->priv != NULL);

	matecomponent_dock_set_client_area (win->priv->dock, contents);
}

/**
 * matecomponent_window_get_contents:
 * @win: the matecomponent window
 *
 * Return value: the contained widget
 **/
GtkWidget *
matecomponent_window_get_contents (MateComponentWindow *win)
{
	g_return_val_if_fail (win != NULL, NULL);
	g_return_val_if_fail (win->priv != NULL, NULL);
	g_return_val_if_fail (win->priv->dock != NULL, NULL);

	return matecomponent_dock_get_client_area (win->priv->dock);
}

static void
matecomponent_window_dispose (GObject *object)
{
	MateComponentWindow *win = (MateComponentWindow *)object;

	if (win->priv->engine) {
		matecomponent_ui_engine_dispose (win->priv->engine);
		g_object_unref (win->priv->engine);
		win->priv->engine = NULL;
	}

	G_OBJECT_CLASS (matecomponent_window_parent_class)->dispose (object);
}

static void
matecomponent_window_finalize (GObject *object)
{
	MateComponentWindow *win = (MateComponentWindow *)object;

	g_free (win->priv->name);
	g_free (win->priv->prefix);
	g_free (win->priv);

	win->priv = NULL;

	G_OBJECT_CLASS (matecomponent_window_parent_class)->finalize (object);
}

/**
 * matecomponent_window_get_accel_group:
 * @win: the matecomponent window
 *
 * Return value: the associated accelerator group for this window
 **/
GtkAccelGroup *
matecomponent_window_get_accel_group (MateComponentWindow *win)
{
	g_return_val_if_fail (MATECOMPONENT_IS_WINDOW (win), NULL);

	return win->priv->accel_group;
}

static MateComponentWindowPrivate *
construct_priv (MateComponentWindow *win)
{
	GtkWidget *main_vbox;
	MateComponentWindowPrivate *priv;
	MateComponentDockItemBehavior behavior;

	priv = g_new0 (MateComponentWindowPrivate, 1);

	priv->engine = matecomponent_ui_engine_new (G_OBJECT (win));

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (win), main_vbox);

	priv->dock = MATECOMPONENT_DOCK (matecomponent_dock_new ());
	gtk_box_pack_start (GTK_BOX (main_vbox),
			    GTK_WIDGET (priv->dock),
			    TRUE, TRUE, 0);

	behavior = (MATECOMPONENT_DOCK_ITEM_BEH_EXCLUSIVE
		    | MATECOMPONENT_DOCK_ITEM_BEH_NEVER_VERTICAL);
	if (!matecomponent_ui_preferences_get_menubar_detachable ())
		behavior |= MATECOMPONENT_DOCK_ITEM_BEH_LOCKED;

	priv->menu_item = MATECOMPONENT_DOCK_ITEM (matecomponent_dock_item_new (
		"menu", behavior));
	priv->menu      = GTK_MENU_BAR (gtk_menu_bar_new ());
	gtk_container_add (GTK_CONTAINER (priv->menu_item),
			   GTK_WIDGET    (priv->menu));
	matecomponent_dock_add_item (priv->dock, priv->menu_item,
			     MATECOMPONENT_DOCK_TOP, 0, 0, 0, TRUE);

	priv->status = GTK_BOX (gtk_hbox_new (FALSE, 0));
	gtk_box_pack_end (GTK_BOX (main_vbox),
			  GTK_WIDGET (priv->status),
			  FALSE, FALSE, 0);

	priv->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (win),
				    priv->accel_group);

	gtk_widget_show_all (GTK_WIDGET (main_vbox));
	gtk_widget_hide (GTK_WIDGET (priv->status));

	priv->sync_menu = matecomponent_ui_sync_menu_new (
		priv->engine, priv->menu,
		GTK_WIDGET (priv->menu_item),
		priv->accel_group);

	matecomponent_ui_engine_add_sync (priv->engine, priv->sync_menu);


	priv->sync_toolbar = matecomponent_ui_sync_toolbar_new (
		priv->engine, MATECOMPONENT_DOCK (priv->dock));

	matecomponent_ui_engine_add_sync (priv->engine, priv->sync_toolbar);

	/* Keybindings; the gtk_binding stuff is just too evil */
	priv->sync_keys = matecomponent_ui_sync_keys_new (priv->engine);
	matecomponent_ui_engine_add_sync (priv->engine, priv->sync_keys);

	priv->sync_status = matecomponent_ui_sync_status_new (
		priv->engine, priv->status);
	matecomponent_ui_engine_add_sync (priv->engine, priv->sync_status);

	return priv;
}

/*
 *   To kill bug reports of hiding not working
 * we want to stop show_all showing hidden menus etc.
 */
static void
matecomponent_window_show_all (GtkWidget *widget)
{
	GtkWidget *client;
	MateComponentWindow *win = MATECOMPONENT_WINDOW (widget);

	if (win->priv->dock &&
	    (client = matecomponent_dock_get_client_area (win->priv->dock)))
		gtk_widget_show_all (client);

	gtk_widget_show (widget);
}

static gboolean
matecomponent_window_key_press_event (GtkWidget *widget,
			       GdkEventKey *event)
{
	gboolean handled;
	MateComponentUISyncKeys *sync;
	MateComponentWindow *window = (MateComponentWindow *) widget;

	handled = GTK_WIDGET_CLASS (matecomponent_window_parent_class)->key_press_event (widget, event);
	if (handled)
		return TRUE;

	sync = MATECOMPONENT_UI_SYNC_KEYS (window->priv->sync_keys);
	if (sync)
		return matecomponent_ui_sync_keys_binding_handle (widget, event, sync);

	return FALSE;
}

static gboolean
matecomponent_window_key_release_event (GtkWidget *widget,
				 GdkEventKey *event)
{
	return GTK_WIDGET_CLASS (matecomponent_window_parent_class)->key_release_event (widget, event);
}

static void
matecomponent_window_set_property (GObject         *object,
			guint            prop_id,
			 const GValue    *value,
			GParamSpec      *pspec)
{
	MateComponentWindow *window;

	window = MATECOMPONENT_WINDOW (object);

	switch (prop_id)
	{
	case PROP_WIN_NAME:
		matecomponent_window_set_name (window, g_value_get_string (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
matecomponent_window_get_property (GObject         *object,
			guint            prop_id,
			GValue          *value,
			GParamSpec      *pspec)
{
	MateComponentWindow *window;

	window = MATECOMPONENT_WINDOW (object);

	switch (prop_id)
	{
	case PROP_WIN_NAME:
		g_value_set_string (value, window->priv->name);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
matecomponent_window_class_init (MateComponentWindowClass *klass)
{
	GObjectClass *gobject_class = (GObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

	gobject_class->dispose  = matecomponent_window_dispose;
	gobject_class->finalize = matecomponent_window_finalize;
	gobject_class->set_property = matecomponent_window_set_property;
	gobject_class->get_property = matecomponent_window_get_property;

	widget_class->show_all = matecomponent_window_show_all;
	widget_class->key_press_event = matecomponent_window_key_press_event;
	widget_class->key_release_event = matecomponent_window_key_release_event;

	/* Properties: */
	g_object_class_install_property (gobject_class,
		PROP_WIN_NAME,
		g_param_spec_string ("win_name",
		_("Name"),
		_("Name of the window - used for configuration serialization."),
		NULL,
		G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
matecomponent_window_init (MateComponentWindow *win)
{
	MateComponentUIContainer *ui_container = NULL;

	win->priv = construct_priv (win);

	/* Create UIContainer: */
	ui_container = matecomponent_ui_container_new ();
	matecomponent_ui_container_set_engine (ui_container, win->priv->engine);
	matecomponent_object_unref (MATECOMPONENT_OBJECT (ui_container));
}

/**
 * matecomponent_window_set_name:
 * @win: the matecomponent window
 * @win_name: the window name
 *
 * Set the name of the window - used for configuration
 * serialization.
 **/
void
matecomponent_window_set_name (MateComponentWindow  *win,
			const char *win_name)
{
	MateComponentWindowPrivate *priv;

	g_return_if_fail (MATECOMPONENT_IS_WINDOW (win));

	priv = win->priv;

	g_free (priv->name);
	g_free (priv->prefix);

	if (win_name) {
		priv->name = g_strdup (win_name);
		priv->prefix = g_strconcat ("/", win_name, "/", NULL);
	} else {
		priv->name = NULL;
		priv->prefix = g_strdup ("/");
	}
}

/**
 * matecomponent_window_get_name:
 * @win: the matecomponent window
 *
 * Return value: the name of the window
 **/
char *
matecomponent_window_get_name (MateComponentWindow *win)
{
	g_return_val_if_fail (MATECOMPONENT_IS_WINDOW (win), NULL);
	g_return_val_if_fail (win->priv != NULL, NULL);

	if (win->priv->name)
		return g_strdup (win->priv->name);
	else
		return NULL;
}

/**
 * matecomponent_window_get_ui_engine:
 * @win: the matecomponent window
 *
 * Return value: the #MateComponentUIEngine
 **/
MateComponentUIEngine *
matecomponent_window_get_ui_engine (MateComponentWindow *win)
{
	g_return_val_if_fail (MATECOMPONENT_IS_WINDOW (win), NULL);
	g_return_val_if_fail (win->priv != NULL, NULL);

	return win->priv->engine;
}

/**
 * matecomponent_window_get_ui_container:
 * @win: the matecomponent window
 *
 * Return value: the #MateComponentUIContainer
 **/
MateComponentUIContainer *
matecomponent_window_get_ui_container (MateComponentWindow *win)
{
	g_return_val_if_fail (MATECOMPONENT_IS_WINDOW (win), NULL);
	g_return_val_if_fail (win->priv != NULL, NULL);

	return matecomponent_ui_engine_get_ui_container (win->priv->engine);
}

/**
 * matecomponent_window_construct:
 * @win: the window to construct
 * @iu_container: the UI container
 * @win_name: the window name
 * @title: the window's title for the title bar
 *
 * Don't use this ever - use construct time properties instead.
 * TODO: Remove this when we are allowed API changes.
 *
 * Return value: a constructed window
 **/
GtkWidget *
matecomponent_window_construct (MateComponentWindow      *win,
			 MateComponentUIContainer *ui_container,
			 const char        *win_name,
			 const char        *title)
{
	g_return_val_if_fail (MATECOMPONENT_IS_WINDOW (win), NULL);
	g_return_val_if_fail (MATECOMPONENT_IS_UI_CONTAINER (ui_container), NULL);

	matecomponent_window_set_name (win, win_name);

	matecomponent_ui_container_set_engine (ui_container, win->priv->engine);

	matecomponent_object_unref (MATECOMPONENT_OBJECT (ui_container));

	if (title)
		gtk_window_set_title (GTK_WINDOW (win), title);


	return GTK_WIDGET (win);
}

/**
 * matecomponent_window_new:
 * @win_name: the window name
 * @title: the window's title for the title bar
 *
 * Return value: a new MateComponentWindow
 **/
GtkWidget *
matecomponent_window_new (const char *win_name,
		   const char *title)
{
	MateComponentWindow *win = g_object_new (MATECOMPONENT_TYPE_WINDOW, "win_name", win_name, "title", title, NULL);

	return GTK_WIDGET (win);
}


