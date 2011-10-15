/* WARNING ____ IMMATURE API ____ liable to change */

/* matecomponent-dock-item.h
 *
 * Copyright (C) 1998 Ettore Perazzoli
 * Copyright (C) 1998 Elliot Lee
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
*/

#ifndef _MATECOMPONENT_DOCK_ITEM_H
#define _MATECOMPONENT_DOCK_ITEM_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_DOCK_ITEM            (matecomponent_dock_item_get_type())
#define MATECOMPONENT_DOCK_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_DOCK_ITEM, MateComponentDockItem))
#define MATECOMPONENT_DOCK_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_DOCK_ITEM, MateComponentDockItemClass))
#define MATECOMPONENT_IS_DOCK_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_DOCK_ITEM))
#define MATECOMPONENT_IS_DOCK_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_DOCK_ITEM))
#define MATECOMPONENT_DOCK_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECOMPONENT_TYPE_DOCK_ITEM, MateComponentDockItemClass))

typedef enum
{
  MATECOMPONENT_DOCK_ITEM_BEH_NORMAL = 0,
  MATECOMPONENT_DOCK_ITEM_BEH_EXCLUSIVE = 1 << 0,
  MATECOMPONENT_DOCK_ITEM_BEH_NEVER_FLOATING = 1 << 1,
  MATECOMPONENT_DOCK_ITEM_BEH_NEVER_VERTICAL = 1 << 2,
  MATECOMPONENT_DOCK_ITEM_BEH_NEVER_HORIZONTAL = 1 << 3,
  MATECOMPONENT_DOCK_ITEM_BEH_LOCKED = 1 << 4
  /* MAINT: Update the size of the bit field in the MateComponentDockItem structure if you add items to this */
} MateComponentDockItemBehavior;

/* obsolete, for compatibility; don't use */
#define MATECOMPONENT_DOCK_ITEM_BEH_NEVER_DETACH MATECOMPONENT_DOCK_ITEM_BEH_NEVER_FLOATING

#define MATECOMPONENT_DOCK_ITEM_NOT_LOCKED(x) (! (MATECOMPONENT_DOCK_ITEM(x)->behavior \
                                          & MATECOMPONENT_DOCK_ITEM_BEH_LOCKED))

typedef struct _MateComponentDockItem        MateComponentDockItem;
typedef struct _MateComponentDockItemPrivate MateComponentDockItemPrivate;
typedef struct _MateComponentDockItemClass   MateComponentDockItemClass;

struct _MateComponentDockItem
{
  GtkBin bin;

  gchar                *name;

  /* <private> */
  GdkWindow            *bin_window; /* parent window for children */
  GdkWindow            *float_window; /* always NULL */
  GtkShadowType         shadow_type;

  /* Start drag position (wrt widget->window).  */
  gint16                  dragoff_x, dragoff_y;

  /* Position of the floating window.  */
  gint16                  float_x, float_y;

  guint                 behavior : 5;
  guint                 orientation : 1;

  guint                 float_window_mapped : 1;
  guint                 is_floating : 1;
  guint                 in_drag : 1;
  /* If TRUE, the pointer must be grabbed on "map_event".  */
  guint                 grab_on_map_event : 1;

  /*< private >*/
  MateComponentDockItemPrivate *_priv;
};

struct _MateComponentDockItemClass
{
  GtkBinClass parent_class;

  void (* dock_drag_begin) (MateComponentDockItem *item);
  void (* dock_drag_motion) (MateComponentDockItem *item, gint x, gint y);
  void (* dock_drag_end) (MateComponentDockItem *item);
  void (* dock_detach) (MateComponentDockItem *item);
  void (* orientation_changed) (MateComponentDockItem *item, GtkOrientation new_orientation);

  gpointer dummy[4];
};

/* Public methods.  */
GType        matecomponent_dock_item_get_type        (void) G_GNUC_CONST;
GtkWidget     *matecomponent_dock_item_new             (const gchar *name,
                                                MateComponentDockItemBehavior behavior);
void           matecomponent_dock_item_construct       (MateComponentDockItem *new_dock_item,
						const gchar *name,
						MateComponentDockItemBehavior behavior);

GtkWidget     *matecomponent_dock_item_get_child       (MateComponentDockItem *dock_item);

char          *matecomponent_dock_item_get_name        (MateComponentDockItem *dock_item);

void           matecomponent_dock_item_set_shadow_type (MateComponentDockItem *dock_item,
                                                GtkShadowType type);

GtkShadowType  matecomponent_dock_item_get_shadow_type (MateComponentDockItem *dock_item);

gboolean       matecomponent_dock_item_set_orientation (MateComponentDockItem *dock_item,
                                                GtkOrientation orientation);

GtkOrientation matecomponent_dock_item_get_orientation (MateComponentDockItem *dock_item);

MateComponentDockItemBehavior
               matecomponent_dock_item_get_behavior    (MateComponentDockItem *dock_item);

void	       matecomponent_dock_item_set_behavior    (MateComponentDockItem         *dock_item,
                                                 MateComponentDockItemBehavior  behavior);

/* Private methods.  */
#ifdef MATECOMPONENT_UI_INTERNAL
void           matecomponent_dock_item_set_locked      (MateComponentDockItem *dock_item,
						 gboolean        locked);
gboolean       matecomponent_dock_item_detach          (MateComponentDockItem *item,
						 gint x, gint y);

void           matecomponent_dock_item_attach          (MateComponentDockItem *item,
						 GtkWidget *parent,
						 gint x, gint y);
void           matecomponent_dock_item_unfloat         (MateComponentDockItem *item);

void           matecomponent_dock_item_grab_pointer    (MateComponentDockItem *item);

void           matecomponent_dock_item_drag_floating   (MateComponentDockItem *item,
						 gint x, gint y);

void           matecomponent_dock_item_handle_size_request
                                               (MateComponentDockItem *item,
                                                GtkRequisition *requisition);

void           matecomponent_dock_item_get_floating_position
                                               (MateComponentDockItem *item,
                                                gint *x, gint *y);
GtkWidget     *matecomponent_dock_item_get_grip       (MateComponentDockItem *item);

#endif /* MATECOMPONENT_UI_INTERNAL */

G_END_DECLS

#endif /* _MATECOMPONENT_DOCK_ITEM_H */
