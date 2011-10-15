/* matecomponent-dock-band.h

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

#ifndef _MATECOMPONENT_DOCK_BAND_H
#define _MATECOMPONENT_DOCK_BAND_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MATECOMPONENT_TYPE_DOCK_BAND            (matecomponent_dock_band_get_type ())
#define MATECOMPONENT_DOCK_BAND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MATECOMPONENT_TYPE_DOCK_BAND, MateComponentDockBand))
#define MATECOMPONENT_DOCK_BAND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MATECOMPONENT_TYPE_DOCK_BAND, MateComponentDockBandClass))
#define MATECOMPONENT_IS_DOCK_BAND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MATECOMPONENT_TYPE_DOCK_BAND))
#define MATECOMPONENT_IS_DOCK_BAND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MATECOMPONENT_TYPE_DOCK_BAND))
#define MATECOMPONENT_DOCK_BAND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MATECOMPONENT_TYPE_DOCK_BAND, MateComponentDockBandClass))

typedef struct _MateComponentDockBand MateComponentDockBand;
typedef struct _MateComponentDockBandPrivate MateComponentDockBandPrivate;
typedef struct _MateComponentDockBandClass MateComponentDockBandClass;
typedef struct _MateComponentDockBandChild MateComponentDockBandChild;

#include <matecomponent/matecomponent-dock.h>
#include <matecomponent/matecomponent-dock-item.h>
#include <matecomponent/matecomponent-dock-layout.h>

struct _MateComponentDockBand
{
  GtkContainer container;

  GList *children;              /* MateComponentDockBandChild */

  GList *floating_child;        /* MateComponentDockBandChild */

  /* This used to remember the allocation before the drag begin: it is
     necessary to do so because we actually decide what docking action
     happens depending on it, instead of using the current allocation
     (which might be constantly changing while the user drags things
     around).  */
  GtkAllocation drag_allocation;

  guint tot_offsets;

  guint max_space_requisition : 16;
  guint num_children : 8;
  guint new_for_drag : 1;
  guint doing_drag : 1;
  guint orientation : 1;

  /*< private >*/
  MateComponentDockBandPrivate *_priv;
};

struct _MateComponentDockBandClass
{
  GtkContainerClass parent_class;

  gpointer dummy[2];
};

struct _MateComponentDockBandChild
{
  GtkWidget *widget;

  GtkAllocation drag_allocation;

  /* Maximum (requested) offset from the previous child.  */
  guint16 offset;

  /* Actual offset.  */
  guint16 real_offset;

  guint16 drag_offset;

  guint16 prev_space, foll_space;
  guint16 drag_prev_space, drag_foll_space;

  guint16 max_space_requisition;
};

GtkWidget     *matecomponent_dock_band_new              (void);
GType        matecomponent_dock_band_get_type         (void) G_GNUC_CONST;

void           matecomponent_dock_band_set_orientation  (MateComponentDockBand *band,
                                                 GtkOrientation orientation);
GtkOrientation matecomponent_dock_band_get_orientation  (MateComponentDockBand *band);

gboolean       matecomponent_dock_band_insert           (MateComponentDockBand *band,
                                                 GtkWidget *child,
                                                 guint offset,
                                                 gint position);
gboolean       matecomponent_dock_band_prepend          (MateComponentDockBand *band,
                                                 GtkWidget *child,
                                                 guint offset);
gboolean       matecomponent_dock_band_append           (MateComponentDockBand *band,
                                                 GtkWidget *child,
                                                 guint offset);

void           matecomponent_dock_band_set_child_offset (MateComponentDockBand *band,
                                                 GtkWidget *child,
                                                 guint offset);
guint          matecomponent_dock_band_get_child_offset (MateComponentDockBand *band,
                                                 GtkWidget *child);
void           matecomponent_dock_band_move_child       (MateComponentDockBand *band,
                                                 GList *old_child,
                                                 guint new_num);

guint          matecomponent_dock_band_get_num_children (MateComponentDockBand *band);

void           matecomponent_dock_band_drag_begin       (MateComponentDockBand *band,
                                                 MateComponentDockItem *item);
gboolean       matecomponent_dock_band_drag_to          (MateComponentDockBand *band,
                                                 MateComponentDockItem *item,
                                                 gint x, gint y);
void           matecomponent_dock_band_drag_end         (MateComponentDockBand *band,
                                                 MateComponentDockItem *item);

MateComponentDockItem *matecomponent_dock_band_get_item_by_name (MateComponentDockBand *band,
                                                 const char *name,
                                                 guint *position_return,
                                                 guint *offset_return);

void           matecomponent_dock_band_layout_add       (MateComponentDockBand *band,
                                                 MateComponentDockLayout *layout,
                                                 MateComponentDockPlacement placement,
                                                 guint band_num);

#ifdef MATECOMPONENT_UI_INTERNAL
gint _matecomponent_dock_band_handle_key_nav (MateComponentDockBand *band,
				      MateComponentDockItem *item,
				      GdkEventKey    *event);
#endif /* MATECOMPONENT_UI_INTERNAL */

G_END_DECLS

#endif
