/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * matecomponent-widget.h: MateComponent Widget object.
 *
 * Authors:
 *   Nat Friedman    (nat@nat.org)
 *   Michael Meeks   (michael@ximian.com)
 *
 * Copyright 1999-2001 Ximian, Inc.
 */
#ifndef _MATECOMPONENT_WIDGET_H_
#define _MATECOMPONENT_WIDGET_H_

#include <glib.h>
#include <gtk/gtk.h>
#include <matecomponent/MateComponent.h>
#include <matecomponent/matecomponent-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_WIDGET        (matecomponent_widget_get_type ())
#define MATECOMPONENT_WIDGET(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), MATECOMPONENT_TYPE_WIDGET, MateComponentWidget))
#define MATECOMPONENT_WIDGET_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), MATECOMPONENT_TYPE_WIDGET, MateComponentWidgetClass))
#define MATECOMPONENT_IS_WIDGET(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), MATECOMPONENT_TYPE_WIDGET))
#define MATECOMPONENT_IS_WIDGET_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), MATECOMPONENT_TYPE_WIDGET))

struct _MateComponentWidget;
typedef struct _MateComponentWidget MateComponentWidget;

#include <matecomponent/matecomponent-control-frame.h>

struct _MateComponentWidgetPrivate;
typedef struct _MateComponentWidgetPrivate MateComponentWidgetPrivate;

struct _MateComponentWidget {
	GtkBin		          bin;

	MateComponentWidgetPrivate *priv;
};

typedef struct {
	GtkBinClass	 bin_class;

	gpointer dummy[4];
} MateComponentWidgetClass;

GType             matecomponent_widget_get_type                 (void) G_GNUC_CONST;
MateComponent_Unknown      matecomponent_widget_get_objref               (MateComponentWidget      *bw);

/*
 * MateComponentWidget for Controls.
 */
GtkWidget          *matecomponent_widget_new_control              (const char        *moniker,
							    MateComponent_UIContainer uic);
GtkWidget          *matecomponent_widget_new_control_from_objref  (MateComponent_Control     control,
							    MateComponent_UIContainer uic);
MateComponentControlFrame *matecomponent_widget_get_control_frame        (MateComponentWidget      *bw);

MateComponent_UIContainer  matecomponent_widget_get_ui_container         (MateComponentWidget      *bw);

typedef void      (*MateComponentWidgetAsyncFn)                   (MateComponentWidget       *widget,
							    CORBA_Environment  *ev,
							    gpointer            user_data);

GtkWidget          *matecomponent_widget_new_control_async        (const char         *moniker,
							    MateComponent_UIContainer  uic,
							    MateComponentWidgetAsyncFn fn,
							    gpointer            user_data);

/*
 * Constructors (for derivation and wrapping only)
 */
MateComponentWidget       *matecomponent_widget_construct_control_from_objref (MateComponentWidget      *bw,
								 MateComponent_Control     control,
								 MateComponent_UIContainer uic,
								 CORBA_Environment *ev);
MateComponentWidget       *matecomponent_widget_construct_control             (MateComponentWidget      *bw,
								 const char        *moniker,
								 MateComponent_UIContainer uic,
								 CORBA_Environment *ev);
/*
 * Setting properties on a Control's Property Bag,
 * These take Name, TypeCode, Value triplets.
 */
void                 matecomponent_widget_set_property             (MateComponentWidget       *control,
							     const char         *first_prop,
							     ...) G_GNUC_NULL_TERMINATED;
void                 matecomponent_widget_get_property             (MateComponentWidget       *control,
							     const char         *first_prop,
							     ...) G_GNUC_NULL_TERMINATED;

/* Compat */
#define matecomponent_widget_get_uih(w) matecomponent_widget_get_ui_container (w)

#ifdef __cplusplus
}
#endif

#endif
