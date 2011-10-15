/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-ui-toolbar-button-item.h: a toolbar button
 *
 * Author: Ettore Perazzoli
 *
 * Copyright (C) 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM_H_
#define _MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM_H_

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "matecomponent-ui-toolbar-item.h"

#ifndef MATECOMPONENT_UI_DISABLE_DEPRECATED

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_UI_TOOLBAR_BUTTON_ITEM		(matecomponent_ui_toolbar_button_item_get_type ())
#define MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR_BUTTON_ITEM, MateComponentUIToolbarButtonItem))
#define MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR_BUTTON_ITEM, MateComponentUIToolbarButtonItemClass))
#define MATECOMPONENT_IS_UI_TOOLBAR_BUTTON_ITEM(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR_BUTTON_ITEM))
#define MATECOMPONENT_IS_UI_TOOLBAR_BUTTON_ITEM_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR_BUTTON_ITEM))

typedef struct _MateComponentUIToolbarButtonItemPrivate MateComponentUIToolbarButtonItemPrivate;

typedef struct {
	MateComponentUIToolbarItem parent;

	MateComponentUIToolbarButtonItemPrivate *priv;
} MateComponentUIToolbarButtonItem;

typedef struct {
	MateComponentUIToolbarItemClass parent_class;

	/* Virtual methods */
	void (* set_icon)       (MateComponentUIToolbarButtonItem *button_item,
				 gpointer                   image);
	void (* set_label)      (MateComponentUIToolbarButtonItem *button_item,
				 const char                *label);

	/* Signals.  */
	void (* clicked)	(MateComponentUIToolbarButtonItem *toolbar_button_item);
	void (* set_want_label) (MateComponentUIToolbarButtonItem *toolbar_button_item);

	gpointer dummy[2];
} MateComponentUIToolbarButtonItemClass;

GType    matecomponent_ui_toolbar_button_item_get_type           (void) G_GNUC_CONST;
void       matecomponent_ui_toolbar_button_item_construct          (MateComponentUIToolbarButtonItem *item,
							     GtkButton                 *button_widget,
							     GdkPixbuf                 *icon,
							     const char                *label);
GtkWidget *matecomponent_ui_toolbar_button_item_new                (GdkPixbuf                 *icon,
							     const char                *label);

void       matecomponent_ui_toolbar_button_item_set_image          (MateComponentUIToolbarButtonItem *button_item,
							     gpointer                   image);
void       matecomponent_ui_toolbar_button_item_set_label          (MateComponentUIToolbarButtonItem *button_item,
							     const char                *label);

GtkButton *matecomponent_ui_toolbar_button_item_get_button_widget  (MateComponentUIToolbarButtonItem *button_item);

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_UI_DISABLE_DEPRECATED */

#endif /* _MATECOMPONENT_UI_TOOLBAR_BUTTON_ITEM_H_ */
