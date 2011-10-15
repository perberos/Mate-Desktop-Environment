/* matecomponent-dock-layout.c

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

#ifndef _MATECOMPONENT_DOCK_LAYOUT_H
#define _MATECOMPONENT_DOCK_LAYOUT_H



#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_DOCK_LAYOUT            (matecomponent_dock_layout_get_type ())
#define MATECOMPONENT_DOCK_LAYOUT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_DOCK_LAYOUT, MateComponentDockLayout))
#define MATECOMPONENT_DOCK_LAYOUT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_DOCK_LAYOUT, MateComponentDockLayoutClass))
#define MATECOMPONENT_IS_DOCK_LAYOUT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_DOCK_LAYOUT))
#define MATECOMPONENT_IS_DOCK_LAYOUT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_DOCK_LAYOUT))
#define MATECOMPONENT_DOCK_LAYOUT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECOMPONENT_TYPE_DOCK_LAYOUT, MateComponentDockLayoutClass))

typedef struct _MateComponentDockLayoutItem    MateComponentDockLayoutItem;
typedef struct _MateComponentDockLayoutClass   MateComponentDockLayoutClass;
typedef struct _MateComponentDockLayout        MateComponentDockLayout;
typedef struct _MateComponentDockLayoutPrivate MateComponentDockLayoutPrivate;

#include <matecomponent/matecomponent-dock.h>
#include <matecomponent/matecomponent-dock-item.h>

struct _MateComponentDockLayoutItem
{
  MateComponentDockItem *item;

  MateComponentDockPlacement placement;

  union
  {
    struct
    {
      gint x;
      gint y;
      GtkOrientation orientation;
    } floating;

    struct
    {
      gint band_num;
      gint band_position;
      gint offset;
    } docked;

  } position;
};

struct _MateComponentDockLayout
{
  GObject object;

  GList *items;                 /* MateComponentDockLayoutItem */

  /*< private >*/
  MateComponentDockLayoutPrivate *_priv;
};

struct _MateComponentDockLayoutClass
{
  GObjectClass parent_class;

  gpointer dummy[4];
};

MateComponentDockLayout     *matecomponent_dock_layout_new      (void);
GType                 matecomponent_dock_layout_get_type (void) G_GNUC_CONST;

gboolean             matecomponent_dock_layout_add_item (MateComponentDockLayout *layout,
                                                 MateComponentDockItem *item,
                                                 MateComponentDockPlacement placement,
                                                 gint band_num,
                                                 gint band_position,
                                                 gint offset);

gboolean             matecomponent_dock_layout_add_floating_item
                                                (MateComponentDockLayout *layout,
                                                 MateComponentDockItem *item,
                                                 gint x, gint y,
                                                 GtkOrientation orientation);

MateComponentDockLayoutItem *matecomponent_dock_layout_get_item (MateComponentDockLayout *layout,
                                                 MateComponentDockItem *item);
MateComponentDockLayoutItem *matecomponent_dock_layout_get_item_by_name
                                                (MateComponentDockLayout *layout,
                                                 const gchar *name);

gboolean             matecomponent_dock_layout_remove_item
                                                (MateComponentDockLayout *layout,
                                                 MateComponentDockItem *item);
gboolean             matecomponent_dock_layout_remove_item_by_name
                                                (MateComponentDockLayout *layout,
                                                 const gchar *name);

gchar               *matecomponent_dock_layout_create_string
                                                (MateComponentDockLayout *layout);
gboolean             matecomponent_dock_layout_parse_string
                                                (MateComponentDockLayout *layout,
                                                 const gchar *string);

gboolean             matecomponent_dock_layout_add_to_dock
                                                (MateComponentDockLayout *layout,
                                                 MateComponentDock *dock);

G_END_DECLS

#endif
