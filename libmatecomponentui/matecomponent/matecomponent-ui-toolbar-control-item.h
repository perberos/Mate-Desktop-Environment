/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-ui-toolbar-control-item.c: A special toolbar item for controls.
 *
 * Author:
 *	Jon K Hellan (hellan@acm.org)
 *
 * Copyright 2000 Jon K Hellan.
 */
#ifndef _MATECOMPONENT_UI_TOOLBAR_CONTROL_ITEM_H_
#define _MATECOMPONENT_UI_TOOLBAR_CONTROL_ITEM_H_

#include <glib.h>
#include "matecomponent-ui-toolbar-button-item.h"
#include "matecomponent-widget.h"

/* All private / non-installed header */

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_UI_TOOLBAR_CONTROL_ITEM            (matecomponent_ui_toolbar_control_item_get_type ())
#define MATECOMPONENT_UI_TOOLBAR_CONTROL_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR_CONTROL_ITEM, MateComponentUIToolbarControlItem))
#define MATECOMPONENT_UI_TOOLBAR_CONTROL_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR_CONTROL_ITEM, MateComponentUIToolbarControlItemClass))
#define MATECOMPONENT_IS_UI_TOOLBAR_CONTROL_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_UI_TOOLBAR_CONTROL_ITEM))
#define MATECOMPONENT_IS_UI_TOOLBAR_CONTROL_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_UI_TOOLBAR_CONTROL_ITEM))

typedef enum {
	MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_CONTROL,
	MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_BUTTON,
	MATECOMPONENT_UI_TOOLBAR_CONTROL_DISPLAY_NONE
} MateComponentUIToolbarControlDisplay;

typedef struct _MateComponentUIToolbarControlItemPrivate MateComponentUIToolbarControlItemPrivate;

typedef struct {
	GtkToolButton parent;

	GtkWidget    *widget;   /* The widget (of a control, or custom */
        MateComponentWidget *control;	/* The wrapped control - if a control, or NULL */
	GtkWidget    *box;      /* Box to pack innards inside to avoid reparenting */
	GtkWidget    *button;	/* GtkToolItem's internal button */

	MateComponentUIToolbarControlDisplay hdisplay;
	MateComponentUIToolbarControlDisplay vdisplay;
} MateComponentUIToolbarControlItem;

typedef struct {
	GtkToolButtonClass parent_class;
} MateComponentUIToolbarControlItemClass;

GType       matecomponent_ui_toolbar_control_item_get_type    (void) G_GNUC_CONST;
GtkWidget    *matecomponent_ui_toolbar_control_item_new         (MateComponent_Control control_ref);
GtkWidget    *matecomponent_ui_toolbar_control_item_new_widget  (GtkWidget *custom_in_proc_widget);
GtkWidget    *matecomponent_ui_toolbar_control_item_construct   (MateComponentUIToolbarControlItem *control_item,
							  GtkWidget                  *widget,
							  MateComponent_Control              control_ref);
void          matecomponent_ui_toolbar_control_item_set_display (MateComponentUIToolbarControlItem    *item,
							  MateComponentUIToolbarControlDisplay  hdisplay,
							  MateComponentUIToolbarControlDisplay  vdisplay);

G_END_DECLS

#endif /* _MATECOMPONENT_UI_TOOLBAR_CONTROL_ITEM_H_ */
