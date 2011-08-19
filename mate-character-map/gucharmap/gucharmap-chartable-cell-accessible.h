/*
 * Copyright © 2003  Sun Microsystems Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef GUCHARMAP_CHARTABLE_CELL_ACCESSIBLE_H
#define GUCHARMAP_CHARTABLE_CELL_ACCESSIBLE_H

#include <atk/atk.h>

#include "gucharmap-chartable.h"

G_BEGIN_DECLS

#define GUCHARMAP_TYPE_CHARTABLE_CELL_ACCESSIBLE             (gucharmap_chartable_cell_accessible_get_type ())
#define GUCHARMAP_CHARTABLE_CELL_ACCESSIBLE(o)               (G_TYPE_CHECK_INSTANCE_CAST ((o), GUCHARMAP_TYPE_CHARTABLE_CELL_ACCESSIBLE, GucharmapChartableCellAccessible))
#define GUCHARMAP_CHARTABLE_CELL_ACCESSIBLE_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), GUCHARMAP_TYPE_CHARTABLE_CELL_ACCESSIBLE, GucharmapChartableCellAccessibleClass))
#define GUCHARMAP_IS_CHARTABLE_CELL_ACCESSIBLE(o)            (G_TYPE_CHECK_INSTANCE_TYPE ((o), GUCHARMAP_TYPE_CHARTABLE_CELL_ACCESSIBLE))
#define GUCHARMAP_IS_CHARTABLE_CELL_ACCESSIBLE_CLASS(k)      (G_TYPE_CHECK_CLASS_TYPE ((k), GUCHARMAP_TYPE_CHARTABLE_CELL_ACCESSIBLE))
#define GUCHARMAP_CHARTABLE_CELL_ACCESSIBLE_GET_CLASS(o)     (G_TYPE_INSTANCE_GET_CLASS ((o), GUCHARMAP_TYPE_CHARTABLE_CELL_ACCESSIBLE, GucharmapChartableCellAccessibleClass))

typedef struct _GucharmapChartableCellAccessible      GucharmapChartableCellAccessible;
typedef struct _GucharmapChartableCellAccessibleClass GucharmapChartableCellAccessibleClass;

struct _GucharmapChartableCellAccessible
{
  AtkObject parent;

  GtkWidget    *widget;
  int           index;
  AtkStateSet  *state_set;
  gchar        *activate_description;
  guint         action_idle_handler;
};

struct _GucharmapChartableCellAccessibleClass
{
  AtkObjectClass parent_class;
};

GType gucharmap_chartable_cell_accessible_get_type (void);

AtkObject* gucharmap_chartable_cell_accessible_new (void);

void gucharmap_chartable_cell_accessible_initialise (GucharmapChartableCellAccessible *cell,
                                                     GtkWidget          *widget,
                                                     AtkObject          *parent,
                                                     gint                index);

gboolean gucharmap_chartable_cell_accessible_add_state (GucharmapChartableCellAccessible *cell,
                                                        AtkStateType        state_type,
                                                        gboolean            emit_signal);

gboolean gucharmap_chartable_cell_accessible_remove_state (GucharmapChartableCellAccessible *cell,
                                                           AtkStateType        state_type,
                                                           gboolean            emit_signal);

G_END_DECLS

#endif /* GUCHARMAP_CHARTABLE_CELL_ACCESSIBLE_H */
