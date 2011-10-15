/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-ui-toolbar.h
 *
 * Author:
 *    Ettore Perazzoli
 *
 * Copyright (C) 2000 Helix Code, Inc.
 */

#ifndef _MATECOMPONENT_UI_TOOLBAR_H_
#define _MATECOMPONENT_UI_TOOLBAR_H_

#include <glib.h>
#include <gtk/gtk.h>
#include "matecomponent-ui-toolbar-item.h"

#ifndef MATECOMPONENT_UI_DISABLE_DEPRECATED

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_UI_TOOLBAR            (matecomponent_ui_toolbar_get_type ())
#define MATECOMPONENT_UI_TOOLBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR, MateComponentUIToolbar))
#define MATECOMPONENT_UI_TOOLBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR, MateComponentUIToolbarClass))
#define MATECOMPONENT_IS_UI_TOOLBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR))
#define MATECOMPONENT_IS_UI_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR))

typedef enum {
	MATECOMPONENT_UI_TOOLBAR_STYLE_PRIORITY_TEXT,
	MATECOMPONENT_UI_TOOLBAR_STYLE_ICONS_AND_TEXT,
	MATECOMPONENT_UI_TOOLBAR_STYLE_ICONS_ONLY,
	MATECOMPONENT_UI_TOOLBAR_STYLE_TEXT_ONLY
} MateComponentUIToolbarStyle;

typedef struct _MateComponentUIToolbarPrivate MateComponentUIToolbarPrivate;

typedef struct {
	GtkContainer parent;

	MateComponentUIToolbarPrivate *priv;
} MateComponentUIToolbar;

typedef struct {
	GtkContainerClass parent_class;

	void (* set_orientation) (MateComponentUIToolbar *toolbar,
				  GtkOrientation orientation);
	void (* style_changed)   (MateComponentUIToolbar *toolbar);

	gpointer dummy[4];
} MateComponentUIToolbarClass;

GType               matecomponent_ui_toolbar_get_type         (void) G_GNUC_CONST;
void                  matecomponent_ui_toolbar_construct        (MateComponentUIToolbar      *toolbar);
GtkWidget            *matecomponent_ui_toolbar_new              (void);

void                  matecomponent_ui_toolbar_set_orientation  (MateComponentUIToolbar      *toolbar,
							  GtkOrientation        orientation);
GtkOrientation        matecomponent_ui_toolbar_get_orientation  (MateComponentUIToolbar      *toolbar);

MateComponentUIToolbarStyle  matecomponent_ui_toolbar_get_style        (MateComponentUIToolbar      *toolbar);

void                  matecomponent_ui_toolbar_set_hv_styles    (MateComponentUIToolbar      *toolbar,
							  MateComponentUIToolbarStyle  hstyle,
							  MateComponentUIToolbarStyle  vstyle);

void                  matecomponent_ui_toolbar_insert           (MateComponentUIToolbar      *toolbar,
							  MateComponentUIToolbarItem  *item,
							  int                   position);

GtkTooltips          *matecomponent_ui_toolbar_get_tooltips     (MateComponentUIToolbar      *toolbar);
void                  matecomponent_ui_toolbar_show_tooltips    (MateComponentUIToolbar      *toolbar,
							  gboolean              show_tips);


GList                *matecomponent_ui_toolbar_get_children     (MateComponentUIToolbar      *toolbar);

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_UI_DISABLE_DEPRECATED */

#endif /* _MATECOMPONENT_UI_TOOLBAR_H_ */

