/* matecomponent-dock.h

   Copyright (C) 1998 Free Software Foundation

   All rights reserved.

   The Mate Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Mate Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Mate Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/
/*
  @NOTATION@
*/

#ifndef _MATECOMPONENT_DOCK_H
#define _MATECOMPONENT_DOCK_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_DOCK            (matecomponent_dock_get_type ())
#define MATECOMPONENT_DOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_DOCK, MateComponentDock))
#define MATECOMPONENT_DOCK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_DOCK, MateComponentDockClass))
#define MATECOMPONENT_IS_DOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_DOCK))
#define MATECOMPONENT_IS_DOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_DOCK))
#define MATECOMPONENT_DOCK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECOMPONENT_TYPE_DOCK, MateComponentDockClass))

typedef enum
{
  MATECOMPONENT_DOCK_TOP,
  MATECOMPONENT_DOCK_RIGHT,
  MATECOMPONENT_DOCK_BOTTOM,
  MATECOMPONENT_DOCK_LEFT,
  MATECOMPONENT_DOCK_FLOATING
} MateComponentDockPlacement;

typedef struct _MateComponentDock MateComponentDock;
typedef struct _MateComponentDockPrivate MateComponentDockPrivate;
typedef struct _MateComponentDockClass MateComponentDockClass;

#include <matecomponent/matecomponent-dock-band.h>
#include <matecomponent/matecomponent-dock-layout.h>

struct _MateComponentDock
{
  GtkContainer container;

  GtkWidget *client_area;

  /* MateComponentDockBands associated with this dock.  */
  GList *top_bands;
  GList *bottom_bands;
  GList *right_bands;
  GList *left_bands;

  /* Children that are currently not docked.  */
  GList *floating_children;     /* GtkWidget */

  /* Client rectangle before drag.  */
  GtkAllocation client_rect;

  guint floating_items_allowed : 1;

  /*< private >*/
  MateComponentDockPrivate *_priv;
};

struct _MateComponentDockClass
{
  GtkContainerClass parent_class;

  void (* layout_changed) (MateComponentDock *dock);

  gpointer dummy[4];
};

GtkWidget     *matecomponent_dock_new               (void);
GType        matecomponent_dock_get_type          (void) G_GNUC_CONST;

void           matecomponent_dock_allow_floating_items
                                            (MateComponentDock *dock,
                                             gboolean enable);

void           matecomponent_dock_add_item          (MateComponentDock             *dock,
                                             MateComponentDockItem         *item,
                                             MateComponentDockPlacement  placement,
                                             guint                  band_num,
                                             gint                   position,
                                             guint                  offset,
                                             gboolean               in_new_band);

void           matecomponent_dock_add_floating_item (MateComponentDock *dock,
                                             MateComponentDockItem *widget,
                                             gint x, gint y,
                                             GtkOrientation orientation);

void             matecomponent_dock_set_client_area   (MateComponentDock             *dock,
						GtkWidget             *widget);

GtkWidget       *matecomponent_dock_get_client_area   (MateComponentDock             *dock);

MateComponentDockItem  *matecomponent_dock_get_item_by_name  (MateComponentDock *dock,
						const gchar *name,
						MateComponentDockPlacement *placement_return,
						guint *num_band_return,
						guint *band_position_return,
						guint *offset_return);

MateComponentDockLayout *matecomponent_dock_get_layout      (MateComponentDock *dock);

gboolean          matecomponent_dock_add_from_layout (MateComponentDock *dock,
					       MateComponentDockLayout *layout);

/* protected */
#ifdef MATECOMPONENT_UI_INTERNAL
gint _matecomponent_dock_handle_key_nav (MateComponentDock     *dock,
				  MateComponentDockBand *band,
				  MateComponentDockItem *item,
				  GdkEventKey    *event);
#endif /* MATECOMPONENT_UI_INTERNAL */

G_END_DECLS

#endif
