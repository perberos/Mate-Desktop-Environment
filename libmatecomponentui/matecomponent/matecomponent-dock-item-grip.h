/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/**
 * matecomponent-dock-item-grip.h
 *
 * Author:
 *    Michael Meeks
 *
 * Copyright (C) 2002 Sun Microsystems, Inc.
 */

#ifndef _MATECOMPONENT_DOCK_ITEM_GRIP_H_
#define _MATECOMPONENT_DOCK_ITEM_GRIP_H_

#include <gtk/gtk.h>
#include <matecomponent/matecomponent-dock-item.h>

G_BEGIN_DECLS

#define MATECOMPONENT_TYPE_DOCK_ITEM_GRIP            (matecomponent_dock_item_grip_get_type())
#define MATECOMPONENT_DOCK_ITEM_GRIP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_DOCK_ITEM_GRIP, MateComponentDockItemGrip))
#define MATECOMPONENT_DOCK_ITEM_GRIP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_DOCK_ITEM_GRIP, MateComponentDockItemGripClass))
#define MATECOMPONENT_IS_DOCK_ITEM_GRIP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_DOCK_ITEM_GRIP))
#define MATECOMPONENT_IS_DOCK_ITEM_GRIP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_DOCK_ITEM_GRIP))
#define MATECOMPONENT_DOCK_ITEM_GRIP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECOMPONENT_TYPE_DOCK_ITEM_GRIP, MateComponentDockItemGripClass))

typedef struct {
	GtkWidget parent;

	MateComponentDockItem *item;
} MateComponentDockItemGrip;

typedef struct {
	GtkWidgetClass parent_class;

	void (*activate) (MateComponentDockItemGrip *grip);
} MateComponentDockItemGripClass;

GType      matecomponent_dock_item_grip_get_type (void);
GtkWidget *matecomponent_dock_item_grip_new      (MateComponentDockItem *item);

G_END_DECLS

#endif /* _MATECOMPONENT_DOCK_ITEM_GRIP_H_ */
