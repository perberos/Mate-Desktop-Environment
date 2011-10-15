/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-ui-toolbar-toggle-button-item.h
 *
 * Author:
 *     Ettore Perazzoli
 *
 * Copyright (C) 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_TOOLBAR_TOGGLE_BUTTON_ITEM_H_
#define _MATECOMPONENT_UI_TOOLBAR_TOGGLE_BUTTON_ITEM_H_

#include <glib.h>
#include "matecomponent-ui-toolbar-button-item.h"

#ifndef MATECOMPONENT_UI_DISABLE_DEPRECATED

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_UI_TOOLBAR_TOGGLE_BUTTON_ITEM             (matecomponent_ui_toolbar_toggle_button_item_get_type ())
#define MATECOMPONENT_UI_TOOLBAR_TOGGLE_BUTTON_ITEM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR_TOGGLE_BUTTON_ITEM, MateComponentUIToolbarToggleButtonItem))
#define MATECOMPONENT_UI_TOOLBAR_TOGGLE_BUTTON_ITEM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR_TOGGLE_BUTTON_ITEM, MateComponentUIToolbarToggleButtonItemClass))
#define MATECOMPONENT_IS_UI_TOOLBAR_TOGGLE_BUTTON_ITEM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR_TOGGLE_BUTTON_ITEM))
#define MATECOMPONENT_IS_UI_TOOLBAR_TOGGLE_BUTTON_ITEM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR_TOGGLE_BUTTON_ITEM))


typedef struct _MateComponentUIToolbarToggleButtonItemPrivate MateComponentUIToolbarToggleButtonItemPrivate;

typedef struct {
	MateComponentUIToolbarButtonItem parent;
} MateComponentUIToolbarToggleButtonItem;

typedef struct {
	MateComponentUIToolbarButtonItemClass parent_class;

	void (* toggled) (MateComponentUIToolbarToggleButtonItem *toggle_button_item);

	gpointer dummy[2];
} MateComponentUIToolbarToggleButtonItemClass;


GType    matecomponent_ui_toolbar_toggle_button_item_get_type   (void) G_GNUC_CONST;
void       matecomponent_ui_toolbar_toggle_button_item_construct  (MateComponentUIToolbarToggleButtonItem *toggle_button_item,
							 GdkPixbuf                     *icon,
							 const char                    *label);
GtkWidget *matecomponent_ui_toolbar_toggle_button_item_new        (GdkPixbuf                     *icon,
							 const char                    *label);

void      matecomponent_ui_toolbar_toggle_button_item_set_active  (MateComponentUIToolbarToggleButtonItem *item,
							 gboolean                       active);
gboolean  matecomponent_ui_toolbar_toggle_button_item_get_active  (MateComponentUIToolbarToggleButtonItem *item);

G_END_DECLS

#endif /* MATECOMPONENT_UI_DISABLE_DEPRECATED */

#endif /* _MATECOMPONENT_UI_TOOLBAR_TOGGLE_BUTTON_ITEM_H_ */
