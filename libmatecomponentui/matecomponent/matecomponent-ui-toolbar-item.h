/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-ui-toolbar-item.h
 *
 * Author: Ettore Perazzoli
 *
 * Copyright (C) 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_TOOLBAR_ITEM_H_
#define _MATECOMPONENT_UI_TOOLBAR_ITEM_H_

#include <glib.h>
#include <gtk/gtk.h>

#undef GTK_DISABLE_DEPRECATED
#ifndef MATECOMPONENT_UI_DISABLE_DEPRECATED

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_UI_TOOLBAR_ITEM			(matecomponent_ui_toolbar_item_get_type ())
#define MATECOMPONENT_UI_TOOLBAR_ITEM(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR_ITEM, MateComponentUIToolbarItem))
#define MATECOMPONENT_UI_TOOLBAR_ITEM_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR_ITEM, MateComponentUIToolbarItemClass))
#define MATECOMPONENT_IS_UI_TOOLBAR_ITEM(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR_ITEM))
#define MATECOMPONENT_IS_UI_TOOLBAR_ITEM_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR_ITEM))


typedef enum {
	MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_VERTICAL,
	MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_AND_TEXT_HORIZONTAL,
	MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_ICON_ONLY,
	MATECOMPONENT_UI_TOOLBAR_ITEM_STYLE_TEXT_ONLY
} MateComponentUIToolbarItemStyle;


typedef struct _MateComponentUIToolbarItemPrivate MateComponentUIToolbarItemPrivate;

typedef struct {
	GtkBin parent;

	MateComponentUIToolbarItemPrivate *priv;
} MateComponentUIToolbarItem;

typedef struct {
	GtkBinClass parent_class;

	/* Virtual method */
	void (* set_state)       (MateComponentUIToolbarItem     *item,
				  const char              *new_state);
	void (* set_tooltip)     (MateComponentUIToolbarItem     *item,
				  GtkTooltips             *tooltips,
				  const char              *tooltip);

	/* Signals */
	void (* set_orientation) (MateComponentUIToolbarItem     *item,
				  GtkOrientation           orientation);
	void (* set_style)       (MateComponentUIToolbarItem     *item,
				  MateComponentUIToolbarItemStyle style);
	void (* set_want_label)  (MateComponentUIToolbarItem     *item,
				  gboolean                 want_label);
	void (* activate)        (MateComponentUIToolbarItem     *item);

	gpointer dummy[4];
} MateComponentUIToolbarItemClass;


GType                   matecomponent_ui_toolbar_item_get_type         (void) G_GNUC_CONST;
GtkWidget                *matecomponent_ui_toolbar_item_new              (void);
void                      matecomponent_ui_toolbar_item_set_tooltip      (MateComponentUIToolbarItem      *item,
								   GtkTooltips              *tooltips,
								   const char               *tooltip);
void                      matecomponent_ui_toolbar_item_set_state        (MateComponentUIToolbarItem      *item,
								   const char               *new_state);
void                      matecomponent_ui_toolbar_item_set_orientation  (MateComponentUIToolbarItem      *item,
								   GtkOrientation            orientation);
GtkOrientation            matecomponent_ui_toolbar_item_get_orientation  (MateComponentUIToolbarItem      *item);
void                      matecomponent_ui_toolbar_item_set_style        (MateComponentUIToolbarItem      *item,
								   MateComponentUIToolbarItemStyle  style);
MateComponentUIToolbarItemStyle  matecomponent_ui_toolbar_item_get_style        (MateComponentUIToolbarItem      *item);

void			 matecomponent_ui_toolbar_item_set_minimum_width(MateComponentUIToolbarItem *item,
								  int minimum_width);

/* FIXME ugly names.  */
void                      matecomponent_ui_toolbar_item_set_want_label   (MateComponentUIToolbarItem      *button_item,
								   gboolean                  prefer_text);
gboolean                  matecomponent_ui_toolbar_item_get_want_label   (MateComponentUIToolbarItem      *button_item);

void                      matecomponent_ui_toolbar_item_set_expandable   (MateComponentUIToolbarItem      *button_item,
								   gboolean                  expandable);
gboolean                  matecomponent_ui_toolbar_item_get_expandable   (MateComponentUIToolbarItem      *button_item);

void                      matecomponent_ui_toolbar_item_set_pack_end     (MateComponentUIToolbarItem      *button_item,
								   gboolean                  expandable);
gboolean                  matecomponent_ui_toolbar_item_get_pack_end     (MateComponentUIToolbarItem      *button_item);

void                      matecomponent_ui_toolbar_item_activate         (MateComponentUIToolbarItem     *item);

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_UI_DISABLE_DEPRECATED */

#endif /* __MATECOMPONENT_UI_TOOLBAR_ITEM_H__ */
