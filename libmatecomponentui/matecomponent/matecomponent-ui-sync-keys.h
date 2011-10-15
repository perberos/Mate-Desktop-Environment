/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * matecomponent-ui-sync-keys.h: The MateComponent UI/XML sync engine for keys bindings
 *
 * Author:
 *	Michael Meeks (michael@helixcode.com)
 *
 * Copyright 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_SYNC_KEYS_H_
#define _MATECOMPONENT_UI_SYNC_KEYS_H_

#include <gtk/gtk.h>

#include <matecomponent/matecomponent-ui-sync.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_UI_SYNC_KEYS            (matecomponent_ui_sync_keys_get_type ())
#define MATECOMPONENT_UI_SYNC_KEYS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_SYNC_KEYS, MateComponentUISyncKeys))
#define MATECOMPONENT_UI_SYNC_KEYS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_SYNC_KEYS, MateComponentUISyncKeysClass))
#define MATECOMPONENT_IS_UI_SYNC_KEYS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_SYNC_KEYS))
#define MATECOMPONENT_IS_UI_SYNC_KEYS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_SYNC_KEYS))

typedef struct _MateComponentUISyncKeysPrivate MateComponentUISyncKeysPrivate;

typedef struct {
	MateComponentUISync parent;

	GHashTable   *keybindings;

	MateComponentUISyncKeysPrivate *priv;
} MateComponentUISyncKeys;

typedef struct {
	MateComponentUISyncClass parent_class;
} MateComponentUISyncKeysClass;

MateComponentUISync *matecomponent_ui_sync_keys_new            (MateComponentUIEngine   *engine);

gint          matecomponent_ui_sync_keys_binding_handle (GtkWidget        *widget,
						  GdkEventKey      *event,
						  MateComponentUISyncKeys *msync);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_UI_SYNC_KEYS_H_ */
