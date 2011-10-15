/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-ui-config-widget.h: MateComponent Component UIConfig
 *
 * Author:
 *   Michael Meeks (michael@helixcode.com)
 *
 * Copyright  2000 Helix Code, Inc.
 */
#ifndef MATECOMPONENT_UI_CONFIG_WIDGET_H
#define MATECOMPONENT_UI_CONFIG_WIDGET_H

/* Should be purely internal */
#ifndef MATECOMPONENT_UI_DISABLE_DEPRECATED

#include <gtk/gtk.h>
#include <matecomponent/matecomponent-ui-engine.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_UI_CONFIG_WIDGET            (matecomponent_ui_config_widget_get_type ())
#define MATECOMPONENT_UI_CONFIG_WIDGET(obj)		G_TYPE_CHECK_INSTANCE_CAST(obj,  matecomponent_ui_config_widget_get_type (), MateComponentUIConfigWidget)
#define MATECOMPONENT_UI_CONFIG_WIDGET_CLASS(klass)	G_TYPE_CHECK_CLASS_CAST (klass, matecomponent_ui_config_widget_get_type (), MateComponentUIConfigWidgetClass)
#define MATECOMPONENT_IS_UI_CONFIG_WIDGET(obj)		G_TYPE_CHECK_INSTANCE_TYPE (obj, matecomponent_ui_config_widget_get_type ())

typedef struct _MateComponentUIConfigWidgetPrivate MateComponentUIConfigWidgetPrivate;

typedef struct {
	GtkVBox parent;

	MateComponentUIEngine *engine;

	MateComponentUIConfigWidgetPrivate *priv;
} MateComponentUIConfigWidget;

typedef struct {
	GtkVBoxClass parent_class;

	gpointer dummy[2];
} MateComponentUIConfigWidgetClass;

GType	   matecomponent_ui_config_widget_get_type  (void) G_GNUC_CONST;

GtkWidget *matecomponent_ui_config_widget_construct (MateComponentUIConfigWidget *config,
					      MateComponentUIEngine       *engine,
					      GtkAccelGroup        *accel_group);

GtkWidget *matecomponent_ui_config_widget_new       (MateComponentUIEngine       *engine,
					      GtkAccelGroup        *accel_group);

#ifdef __cplusplus
}
#endif

#endif /* MATECOMPONENT_UI_DISABLE_DEPRECATED */

#endif /* MATECOMPONENT_UI_CONFIG_WIDGET_H */
