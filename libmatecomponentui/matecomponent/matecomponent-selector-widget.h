/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-selector-widget.h: MateComponent Component Selector
 *
 * Author:
 *   Michael Meeks (michael@helixcode.com)
 *
 * Copyright  2000 Helix Code, Inc.
 */
#ifndef MATECOMPONENT_SELECTOR_WIDGET_H
#define MATECOMPONENT_SELECTOR_WIDGET_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_SELECTOR_WIDGET             (matecomponent_selector_widget_get_type ())
#define MATECOMPONENT_SELECTOR_WIDGET(obj)		G_TYPE_CHECK_INSTANCE_CAST(obj,  matecomponent_selector_widget_get_type (), MateComponentSelectorWidget)
#define MATECOMPONENT_SELECTOR_WIDGET_CLASS(klass)	G_TYPE_CHECK_CLASS_CAST (klass, matecomponent_selector_widget_get_type (), MateComponentSelectorWidgetClass)
#define MATECOMPONENT_IS_SELECTOR_WIDGET(obj)		G_TYPE_CHECK_INSTANCE_TYPE (obj, matecomponent_selector_widget_get_type ())

typedef struct _MateComponentSelectorWidgetPrivate MateComponentSelectorWidgetPrivate;

typedef struct {
	GtkVBox parent;

	MateComponentSelectorWidgetPrivate *priv;
} MateComponentSelectorWidget;

typedef struct {
	GtkVBoxClass parent_class;

	/* Virtual methods */
	gchar *(* get_id)          (MateComponentSelectorWidget *sel);
	gchar *(* get_name)        (MateComponentSelectorWidget *sel);
	gchar *(* get_description) (MateComponentSelectorWidget *sel);
	void   (* set_interfaces)  (MateComponentSelectorWidget *sel,
				    const gchar         **interfaces);

	/* User select */
	void   (* final_select)    (MateComponentSelectorWidget *sel);

	gpointer dummy[2];
} MateComponentSelectorWidgetClass;

GType	   matecomponent_selector_widget_get_type (void) G_GNUC_CONST;

GtkWidget *matecomponent_selector_widget_new      (void);

void	   matecomponent_selector_widget_set_interfaces  (MateComponentSelectorWidget *sel,
						   const gchar **interfaces_required);

gchar	  *matecomponent_selector_widget_get_id          (MateComponentSelectorWidget *sel);
gchar     *matecomponent_selector_widget_get_name        (MateComponentSelectorWidget *sel);
gchar     *matecomponent_selector_widget_get_description (MateComponentSelectorWidget *sel);


G_END_DECLS

#endif /* MATECOMPONENT_SELECTOR_H */

