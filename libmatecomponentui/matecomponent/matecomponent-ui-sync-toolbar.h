/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-ui-sync-toolbar.h: The MateComponent UI/XML sync engine for toolbars
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_SYNC_TOOLBAR_H_
#define _MATECOMPONENT_UI_SYNC_TOOLBAR_H_

#include <matecomponent/matecomponent-dock.h>
#include <matecomponent/matecomponent-ui-toolbar.h>
#include <matecomponent/matecomponent-ui-sync.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_UI_SYNC_TOOLBAR            (matecomponent_ui_sync_toolbar_get_type ())
#define MATECOMPONENT_UI_SYNC_TOOLBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_SYNC_TOOLBAR, MateComponentUISyncToolbar))
#define MATECOMPONENT_UI_SYNC_TOOLBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_SYNC_TOOLBAR, MateComponentUISyncToolbarClass))
#define MATECOMPONENT_IS_UI_SYNC_TOOLBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_SYNC_TOOLBAR))
#define MATECOMPONENT_IS_UI_SYNC_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_SYNC_TOOLBAR))

typedef struct _MateComponentUISyncToolbarPrivate MateComponentUISyncToolbarPrivate;

typedef struct {
	MateComponentUISync parent;

	MateComponentDock       *dock;

	MateComponentUISyncToolbarPrivate *priv;
} MateComponentUISyncToolbar;

typedef struct {
	MateComponentUISyncClass parent_class;
} MateComponentUISyncToolbarClass;

MateComponentUISync   *matecomponent_ui_sync_toolbar_new              (MateComponentUIEngine *engine,
							 MateComponentDock     *dock);
GtkToolbarStyle matecomponent_ui_sync_toolbar_get_look         (MateComponentUIEngine *engine,
							 MateComponentUINode   *node);

G_END_DECLS

#endif /* _MATECOMPONENT_UI_SYNC_TOOLBAR_H_ */
