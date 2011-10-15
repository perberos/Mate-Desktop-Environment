/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-ui-toolbar-popup-item.h
 *
 * Author:
 *    Ettore Perazzoli
 *
 * Copyright (C) 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_TOOLBAR_POPUP_ITEM_H_
#define _MATECOMPONENT_UI_TOOLBAR_POPUP_ITEM_H_

#include <glib.h>
#include "matecomponent-ui-toolbar-toggle-button-item.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_UI_TOOLBAR_POPUP_ITEM            (matecomponent_ui_toolbar_popup_item_get_type ())
#define MATECOMPONENT_UI_TOOLBAR_POPUP_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR_POPUP_ITEM, MateComponentUIToolbarPopupItem))
#define MATECOMPONENT_UI_TOOLBAR_POPUP_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR_POPUP_ITEM, MateComponentUIToolbarPopupItemClass))
#define MATECOMPONENT_IS_UI_TOOLBAR_POPUP_ITEM(obj)	     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR_POPUP_ITEM))
#define MATECOMPONENT_IS_UI_TOOLBAR_POPUP_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR_POPUP_ITEM))

typedef struct _MateComponentUIToolbarPopupItemPrivate MateComponentUIToolbarPopupItemPrivate;

typedef struct {
	MateComponentUIToolbarToggleButtonItem parent;
} MateComponentUIToolbarPopupItem;

typedef struct {
	MateComponentUIToolbarToggleButtonItemClass parent_class;
} MateComponentUIToolbarPopupItemClass;

GType    matecomponent_ui_toolbar_popup_item_get_type  (void) G_GNUC_CONST;
GtkWidget *matecomponent_ui_toolbar_popup_item_new       (void);
void       matecomponent_ui_toolbar_popup_item_construct (MateComponentUIToolbarPopupItem *);

#ifdef __cplusplus
}
#endif

#endif /* _MATECOMPONENT_UI_TOOLBAR_POPUP_ITEM_H_ */
