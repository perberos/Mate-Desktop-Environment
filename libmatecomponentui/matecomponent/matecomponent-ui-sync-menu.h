/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-ui-sync-menu.h: The MateComponent UI/XML sync engine for menus
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_SYNC_MENU_H_
#define _MATECOMPONENT_UI_SYNC_MENU_H_

#include <gtk/gtk.h>

#include <matecomponent/matecomponent-ui-sync.h>

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_UI_SYNC_MENU            (matecomponent_ui_sync_menu_get_type ())
#define MATECOMPONENT_UI_SYNC_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_SYNC_MENU, MateComponentUISyncMenu))
#define MATECOMPONENT_UI_SYNC_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_SYNC_MENU, MateComponentUISyncMenuClass))
#define MATECOMPONENT_IS_UI_SYNC_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_SYNC_MENU))
#define MATECOMPONENT_IS_UI_SYNC_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_SYNC_MENU))

typedef struct _MateComponentUISyncMenuPrivate MateComponentUISyncMenuPrivate;

typedef struct {
	MateComponentUISync parent;

	GtkMenuBar    *menu;
	GtkWidget     *menu_dock_item;
	GtkAccelGroup *accel_group;
	GHashTable    *radio_groups;
	GSList        *popups;

	MateComponentUISyncMenuPrivate *priv;
} MateComponentUISyncMenu;

typedef struct {
	MateComponentUISyncClass parent_class;
} MateComponentUISyncMenuClass;

MateComponentUISync *matecomponent_ui_sync_menu_new          (MateComponentUIEngine *engine,
						GtkMenuBar     *menu,
						GtkWidget      *menu_dock_item,
						GtkAccelGroup  *group);

void          matecomponent_ui_sync_menu_remove_popup (MateComponentUISyncMenu *sync,
						const char       *path);

void          matecomponent_ui_sync_menu_add_popup    (MateComponentUISyncMenu *sync,
						GtkMenu          *menu,
						const char       *path);

G_END_DECLS

#endif /* _MATECOMPONENT_UI_SYNC_MENU_H_ */
