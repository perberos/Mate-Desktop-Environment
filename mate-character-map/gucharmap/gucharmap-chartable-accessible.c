/*
 * Copyright © 2003  Sun Microsystems Inc.
 * Copyright © 2007  Christian Persch
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <config.h>

#include <string.h>

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "gucharmap.h"
#include "gucharmap-chartable.h"
#include "gucharmap-chartable-accessible.h"
#include "gucharmap-chartable-cell-accessible.h"
#include "gucharmap-private.h"

typedef struct
{
  GtkAdjustment *vadjustment;
  GPtrArray *cells;
  AtkObject *focus_obj;
} GucharmapChartableAccessiblePrivate;

#define GET_PRIVATE(inst) (G_TYPE_INSTANCE_GET_PRIVATE ((inst), gucharmap_chartable_accessible_get_type (), GucharmapChartableAccessiblePrivate))

static gpointer gucharmap_chartable_accessible_parent_class;

static void
set_cell_visibility (GucharmapChartable  *chartable,
                     GucharmapChartableCellAccessible  *cell,
                     gboolean emit_signal)
{
  GucharmapChartablePrivate *chartable_priv = chartable->priv;
  gucharmap_chartable_cell_accessible_add_state (cell, ATK_STATE_VISIBLE, emit_signal);

  if (cell->index >= chartable_priv->page_first_cell &&
      cell->index < chartable_priv->page_first_cell
                    + chartable_priv->rows * chartable_priv->cols)
    {
      gucharmap_chartable_cell_accessible_add_state (cell, ATK_STATE_SHOWING, emit_signal);
    }
  else 
    {
      gucharmap_chartable_cell_accessible_remove_state (cell, ATK_STATE_SHOWING, emit_signal);
    }
}

static void
cell_destroyed (GucharmapChartableCellAccessible *cell)
{
  GucharmapChartableAccessiblePrivate *priv;
  AtkObject *parent;
  GPtrArray *cells;
  guint n_cells, n;
  GucharmapChartableAccessible *accessible;

  parent = atk_object_get_parent (ATK_OBJECT (cell));
  accessible = GUCHARMAP_CHARTABLE_ACCESSIBLE (parent);
  priv = GET_PRIVATE (accessible);

  cells = priv->cells;
  n_cells = cells->len;
  for (n = 0; n < n_cells; ++n)
    {
      GucharmapChartableCellAccessible *another_cell = g_ptr_array_index (cells, n);

      if (another_cell == cell)
        {
          g_ptr_array_remove_index_fast (priv->cells, n);
          return;
        }
    }

  g_warning ("Cell destroyed but was not in the cells cache?\n");
}

static AtkObject*
gucharmap_chartable_accessible_ref_child (AtkObject *obj,
                                          int index)
{
  GucharmapChartableAccessible *table;
  GucharmapChartableAccessiblePrivate *priv;
  GtkWidget *widget;
  GucharmapChartable *chartable;
  AtkObject *child;
  gchar* name;
  GPtrArray *cells;
  guint n_cells, n;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (obj));
#else
  widget = GTK_ACCESSIBLE (obj)->widget;
#endif
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  if (index > UNICHAR_MAX)
    return NULL;

  table = GUCHARMAP_CHARTABLE_ACCESSIBLE (obj);
  priv = GET_PRIVATE (table);

  /* Check whether the child is cached */
  cells = priv->cells;
  n_cells = cells->len;
  for (n = 0; n < n_cells; ++n)
    {
      GucharmapChartableCellAccessible *cell = g_ptr_array_index (cells, n);

      if (index == cell->index)
        return g_object_ref (cell);
    }

  /* Not cached, create a new cell accessible */
  chartable = GUCHARMAP_CHARTABLE (widget);

  child = gucharmap_chartable_cell_accessible_new ();
  gucharmap_chartable_cell_accessible_initialise (GUCHARMAP_CHARTABLE_CELL_ACCESSIBLE (child),
                            GTK_WIDGET (chartable), obj, index);
  /* Set the name of the cell */
  name = g_strdup_printf("U+%4.4X %s", index, gucharmap_get_unicode_name (index));
  atk_object_set_name (child, name);
  g_free (name);
  set_cell_visibility (chartable, GUCHARMAP_CHARTABLE_CELL_ACCESSIBLE (child), FALSE);

  /* Store the cell in our cache */
  g_ptr_array_add (priv->cells, child);
  g_object_weak_ref (G_OBJECT (child),
                     (GWeakNotify) cell_destroyed,
                     child);

  return child; 
}

static AtkObject* 
gucharmap_chartable_accessible_ref_at (AtkTable *table,
                             gint    row,
                             gint    column)
{
  GucharmapChartable *chartable;
  GucharmapChartablePrivate *chartable_priv;
  GtkWidget *widget;
  AtkObject *child;
  gint index;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (table));
#else
  widget = GTK_ACCESSIBLE (table)->widget;
#endif
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  chartable = GUCHARMAP_CHARTABLE (widget);
  chartable_priv = chartable->priv;

  index =  row * chartable_priv->cols + column;

  child = gucharmap_chartable_accessible_ref_child (ATK_OBJECT (table), index);

  return child;
}

static AtkObject*
gucharmap_chartable_accessible_ref_accessible_at_point (AtkComponent *component,
                                              gint x, 
                                              gint y,
                                              AtkCoordType coord_type)
{
  GtkWidget *widget;
  GucharmapChartable *chartable;
  GucharmapChartablePrivate *chartable_priv;
  gint x_pos, y_pos;
  gint row, col;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (component));
#else
  widget = GTK_ACCESSIBLE (component)->widget;
#endif
  if (widget == NULL)
    /* State is defunct */
    return NULL;

  chartable = GUCHARMAP_CHARTABLE (widget);
  chartable_priv = chartable->priv;

  atk_component_get_extents (component, &x_pos, &y_pos,
                             NULL, NULL, coord_type);

  /* Find cell at offset x - x_pos, y - y_pos */

  x_pos = x - x_pos;
  y_pos = y - y_pos;

  for (col = 0; col < chartable_priv->cols; col++)
    {
      if (x_pos < _gucharmap_chartable_x_offset (chartable, col))
        {
          col--;
          break;
        }
    }
  if (col == chartable_priv->cols || col < 0)
    return NULL;

  for (row = 0; row < chartable_priv->rows; row++)
    {
      if (y_pos < _gucharmap_chartable_y_offset (chartable, row))
        {
          row--;
          break;
        }
    }
  if (row == chartable_priv->rows || row < 0)
    return NULL;

  row += chartable_priv->page_first_cell / chartable_priv->cols;

  return gucharmap_chartable_accessible_ref_at (ATK_TABLE (component), row, col);
}

static void
gucharmap_chartable_accessible_component_interface_init (AtkComponentIface *iface)
{
  g_return_if_fail (iface != NULL);

  iface->ref_accessible_at_point = gucharmap_chartable_accessible_ref_accessible_at_point;
}

static AtkStateSet*
gucharmap_chartable_accessible_ref_state_set (AtkObject *obj)
{
  AtkStateSet *state_set;
  GtkWidget *widget;

  state_set = ATK_OBJECT_CLASS (gucharmap_chartable_accessible_parent_class)->ref_state_set (obj);
#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (obj));
#else
  widget = GTK_ACCESSIBLE (obj)->widget;
#endif

  if (widget != NULL)
    atk_state_set_add_state (state_set, ATK_STATE_MANAGES_DESCENDANTS);

  /* FIXME: reference count ? */
  return state_set;
}

/* FIXMEchpe: shouldn't this get the number from the chartable's codepoint list instead?? */
static gint
gucharmap_chartable_accessible_get_n_children (AtkObject *obj)
{
  GtkWidget *widget;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (obj));
#else
  widget = GTK_ACCESSIBLE (obj)->widget;
#endif
  if (widget == NULL)
    /* State is defunct */
    return 0;

  return UNICHAR_MAX + 1;
}

static void
gucharmap_chartable_accessible_init (GucharmapChartableAccessible *accessible)
{
  GucharmapChartableAccessiblePrivate *priv = GET_PRIVATE (accessible);

  priv->cells = g_ptr_array_sized_new (512);
}

static void
gucharmap_chartable_accessible_finalize (GObject *obj)
{
  GucharmapChartableAccessible *accessible = GUCHARMAP_CHARTABLE_ACCESSIBLE (obj);
  GucharmapChartableAccessiblePrivate *priv = GET_PRIVATE (accessible);
  GPtrArray *cells;
  guint n_cells, n;

  if (priv->focus_obj)
    g_object_unref (priv->focus_obj);

  /* Remove the weak ref notifications */
  cells = priv->cells;
  n_cells = cells->len;
  for (n = 0; n < n_cells; ++n)
    {
      GucharmapChartableCellAccessible *cell = g_ptr_array_index (cells, n);

      g_object_weak_unref (G_OBJECT (cell),
                           (GWeakNotify) cell_destroyed,
                           cell);
    }

  g_ptr_array_free (priv->cells, TRUE);

  G_OBJECT_CLASS (gucharmap_chartable_accessible_parent_class)->finalize (obj);
}

static void
gucharmap_chartable_accessible_update_all_cells (AtkObject *obj)
{
  GucharmapChartableAccessible *accessible;
  GucharmapChartableAccessiblePrivate *priv;
  GtkWidget *widget;
  GucharmapChartable *chartable;
  GPtrArray *cells;
  guint n_cells, n;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (obj));
#else
  widget = GTK_ACCESSIBLE (obj)->widget;
#endif
  if (!widget)
    /* Widget is being deleted */
    return;
  
  accessible = GUCHARMAP_CHARTABLE_ACCESSIBLE (obj);
  priv = GET_PRIVATE (accessible);

  chartable = GUCHARMAP_CHARTABLE (widget);

  cells = priv->cells;
  n_cells = cells->len;
  for (n = 0; n < n_cells; ++n)
    {
      GucharmapChartableCellAccessible *cell = g_ptr_array_index (cells, n);

      set_cell_visibility (chartable, GUCHARMAP_CHARTABLE_CELL_ACCESSIBLE (cell), TRUE);
    }

  g_signal_emit_by_name (obj, "visible-data-changed");
}

static void
adjustment_changed (GtkAdjustment *adj,
                    AtkObject     *obj)
{
  gucharmap_chartable_accessible_update_all_cells (obj);
}

static void
size_allocated (GtkWidget     *widget,
                GtkAllocation *alloc,
                gpointer      data)
{
  gucharmap_chartable_accessible_update_all_cells (ATK_OBJECT (data));
}

static AtkObject* /* reference */
find_object (GucharmapChartable   *chartable,
             gunichar  uc,
             AtkObject *obj)
{
  GucharmapChartablePrivate *chartable_priv = chartable->priv;
  gint row, column;

  row = uc / chartable_priv->cols;
  column = _gucharmap_chartable_cell_column (chartable, uc);

  return atk_table_ref_at (ATK_TABLE (obj), row, column);
}

static void
sync_active_char (GucharmapChartable  *chartable,
                  GParamSpec *pspec,
                  GucharmapChartableAccessible *accessible)
{
  GucharmapChartableAccessiblePrivate *priv = GET_PRIVATE (accessible);
  gunichar uc;
  AtkObject *child;
  AtkObject *focus_obj;

  uc = gucharmap_chartable_get_active_character (chartable);

  child = find_object (chartable, uc, ATK_OBJECT (accessible));
  focus_obj = priv->focus_obj;
  if (focus_obj != child)
    {
      if (focus_obj)
        gucharmap_chartable_cell_accessible_remove_state (GUCHARMAP_CHARTABLE_CELL_ACCESSIBLE (focus_obj), ATK_STATE_FOCUSED, FALSE);

      gucharmap_chartable_cell_accessible_add_state (GUCHARMAP_CHARTABLE_CELL_ACCESSIBLE (child), ATK_STATE_FOCUSED, FALSE);
    }

  if (focus_obj)
    g_object_unref (focus_obj);

  priv->focus_obj = child; /* adopts the reference from find_object */

  g_signal_emit_by_name (accessible, "active-descendant-changed", child);
}

static void
gucharmap_chartable_accessible_set_scroll_adjustments (GucharmapChartable *chartable,
                                                       GtkAdjustment *hadjustment,
                                                       GtkAdjustment *vadjustment,
                                                       AtkObject *obj)
{
  GucharmapChartableAccessible *accessible = GUCHARMAP_CHARTABLE_ACCESSIBLE (obj);
  GucharmapChartableAccessiblePrivate *priv = GET_PRIVATE (accessible);

  if (priv->vadjustment != vadjustment)
    {
      GtkAdjustment **adjustment_ptr = &priv->vadjustment;
      g_object_remove_weak_pointer (G_OBJECT (priv->vadjustment),
                                    (gpointer *) adjustment_ptr);
      g_signal_handlers_disconnect_by_func (priv->vadjustment,
                                            G_CALLBACK (adjustment_changed),
                                            obj);
      priv->vadjustment = vadjustment;
      g_object_add_weak_pointer (G_OBJECT (priv->vadjustment),
                                 (gpointer *) adjustment_ptr);
      g_signal_connect (vadjustment, "value-changed",
                        G_CALLBACK (adjustment_changed), obj);
    }
}

static void
gucharmap_chartable_accessible_initialize (AtkObject *obj,
                                           gpointer  data)
{
  GtkWidget *widget;
  AtkObject *focus_obj, *old_focus_obj;
  GucharmapChartableAccessible *accessible;
  GucharmapChartableAccessiblePrivate *priv;
  GucharmapChartable *chartable;
  GucharmapChartablePrivate *chartable_priv;

  ATK_OBJECT_CLASS (gucharmap_chartable_accessible_parent_class)->initialize (obj, data);

  widget = GTK_WIDGET (data);
  accessible = GUCHARMAP_CHARTABLE_ACCESSIBLE (obj);
  priv = GET_PRIVATE (accessible);
  chartable = GUCHARMAP_CHARTABLE (widget);
  chartable_priv = chartable->priv;

  if (chartable_priv->vadjustment)
    {
      GtkAdjustment **adjustment_ptr = &priv->vadjustment;

      priv->vadjustment = chartable_priv->vadjustment;
      g_object_add_weak_pointer (G_OBJECT (priv->vadjustment),
                                 (gpointer *) adjustment_ptr);
      g_signal_connect (chartable_priv->vadjustment, "value-changed",
                        G_CALLBACK (adjustment_changed), obj);
    }

  g_signal_connect_after (chartable, "set-scroll-adjustments",
                          G_CALLBACK (gucharmap_chartable_accessible_set_scroll_adjustments),
                          obj);
  g_signal_connect (widget, "size-allocate",
                    G_CALLBACK (size_allocated), obj);
  g_signal_connect (chartable, "notify::active-character",
                    G_CALLBACK (sync_active_char), obj);

  focus_obj = find_object (chartable, chartable_priv->active_cell, obj);
  old_focus_obj = priv->focus_obj;
  priv->focus_obj = focus_obj; /* adopts the reference from find_object */
  if (old_focus_obj)
    g_object_unref (old_focus_obj);
}

static void
gucharmap_chartable_accessible_destroyed (GtkWidget *widget,
                                          AtkObject *obj)
{
  GucharmapChartableAccessible *accessible = GUCHARMAP_CHARTABLE_ACCESSIBLE (obj);
  GucharmapChartableAccessiblePrivate *priv = GET_PRIVATE (accessible);

  if (priv->vadjustment)
    {
      GtkAdjustment **adjustment_ptr = &priv->vadjustment;

      g_object_remove_weak_pointer (G_OBJECT (priv->vadjustment),
                                    (gpointer *) adjustment_ptr);
          
      g_signal_handlers_disconnect_by_func (priv->vadjustment,
                                            G_CALLBACK (adjustment_changed),
                                            obj);
      priv->vadjustment = NULL;
    }

  g_signal_handlers_disconnect_by_func (widget,
                                        G_CALLBACK (gucharmap_chartable_accessible_set_scroll_adjustments),
                                        obj);
  g_signal_handlers_disconnect_by_func (widget,
                                        G_CALLBACK (size_allocated),
                                        obj);
  g_signal_handlers_disconnect_by_func (widget,
                                        G_CALLBACK (sync_active_char),
                                        obj);
}

static void
gucharmap_chartable_accessible_connect_widget_destroyed (GtkAccessible *accessible)
{
#if GTK_CHECK_VERSION (2, 21, 0)
  if (gtk_accessible_get_widget (accessible))
    {
      g_signal_connect_after (gtk_accessible_get_widget (accessible),
#else
  if (accessible->widget)
    {
      g_signal_connect_after (accessible->widget,
#endif
                              "destroy",
                              G_CALLBACK (gucharmap_chartable_accessible_destroyed),
                              accessible);
    }

  GTK_ACCESSIBLE_CLASS (gucharmap_chartable_accessible_parent_class)->connect_widget_destroyed (accessible);
}

static void
gucharmap_chartable_accessible_class_init (GucharmapChartableAccessibleClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  AtkObjectClass *atk_object_class = ATK_OBJECT_CLASS (klass);
  GtkAccessibleClass *accessible_class = GTK_ACCESSIBLE_CLASS (klass);

  gucharmap_chartable_accessible_parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gucharmap_chartable_accessible_finalize;

  accessible_class->connect_widget_destroyed = gucharmap_chartable_accessible_connect_widget_destroyed;

  atk_object_class->get_n_children = gucharmap_chartable_accessible_get_n_children;
  atk_object_class->ref_child = gucharmap_chartable_accessible_ref_child;
  atk_object_class->ref_state_set = gucharmap_chartable_accessible_ref_state_set;
  atk_object_class->initialize = gucharmap_chartable_accessible_initialize;

  g_type_class_add_private (gobject_class, sizeof (GucharmapChartableAccessiblePrivate));
}

static gint
gucharmap_chartable_accessible_get_n_columns (AtkTable *table)
{
  GtkWidget *widget;
  GucharmapChartable *chartable;
  GucharmapChartablePrivate *chartable_priv;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (table));
#else
  widget = GTK_ACCESSIBLE (table)->widget;
#endif
  if (widget == NULL)
    /* State is defunct */
    return 0;

  chartable = GUCHARMAP_CHARTABLE (widget);
  chartable_priv = chartable->priv;

  return chartable_priv->cols;
}


static gint
gucharmap_chartable_accessible_get_column_extent_at (AtkTable       *table,
                                           gint           row,
                                           gint           column)
{
  GtkWidget *widget;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (table));
#else
  widget = GTK_ACCESSIBLE (table)->widget;
#endif
  if (widget == NULL)
    /* State is defunct */
    return 0;

  return 1;
}


static gint
gucharmap_chartable_accessible_get_n_rows (AtkTable *table)
{
  GtkWidget *widget;
  GucharmapChartable *chartable;
  GucharmapChartablePrivate *chartable_priv;
  gint n_rows;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (table));
#else
  widget = GTK_ACCESSIBLE (table)->widget;
#endif
  if (widget == NULL)
    /* State is defunct */
    return 0;

  chartable = GUCHARMAP_CHARTABLE (widget);
  chartable_priv = chartable->priv;

  n_rows = UNICHAR_MAX / chartable_priv->cols + 1;

  return n_rows;
}

 
static gint
gucharmap_chartable_accessible_get_row_extent_at (AtkTable *table,
                                        gint    row,
                                        gint    column)
{
  GtkWidget *widget;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (table));
#else
  widget = GTK_ACCESSIBLE (table)->widget;
#endif
  if (widget == NULL)
    /* State is defunct */
    return 0;

  return 1;
}

 
static gint
gucharmap_chartable_accessible_get_index_at (AtkTable *table,
                                   gint     row,
                                   gint     column)
{
  GtkWidget *widget;
  GucharmapChartable *chartable;
  GucharmapChartablePrivate *chartable_priv;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (table));
#else
  widget = GTK_ACCESSIBLE (table)->widget;
#endif
  if (widget == NULL)
    /* State is defunct */
    return -1;

  chartable = GUCHARMAP_CHARTABLE (widget);
  chartable_priv = chartable->priv;

  return row * chartable_priv->cols + column;
}


static gint
gucharmap_chartable_accessible_get_column_at_index (AtkTable *table,
                                          gint     index)
{
  GtkWidget *widget;
  GucharmapChartable *chartable;
  GucharmapChartablePrivate *chartable_priv;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (table));
#else
  widget = GTK_ACCESSIBLE (table)->widget;
#endif
  if (widget == NULL)
    /* State is defunct */
    return -1;

  chartable = GUCHARMAP_CHARTABLE (widget);
  chartable_priv = chartable->priv;

  return index % chartable_priv->cols;
}


static gint
gucharmap_chartable_accessible_get_row_at_index (AtkTable *table,
                                                 gint     index)
{
  GtkWidget *widget;
  GucharmapChartable *chartable;
  GucharmapChartablePrivate *chartable_priv;

#if GTK_CHECK_VERSION (2, 21, 0)
  widget = gtk_accessible_get_widget (GTK_ACCESSIBLE (table));
#else
  widget = GTK_ACCESSIBLE (table)->widget;
#endif
  if (widget == NULL)
    /* State is defunct */
    return -1;

  chartable = GUCHARMAP_CHARTABLE (widget);
  chartable_priv = chartable->priv;

  return index / chartable_priv->cols;
}


static void
gucharmap_chartable_accessible_table_interface_init (AtkTableIface *iface)
{
  iface->ref_at = gucharmap_chartable_accessible_ref_at;
  iface->get_n_columns = gucharmap_chartable_accessible_get_n_columns;
  iface->get_column_extent_at = gucharmap_chartable_accessible_get_column_extent_at;
  iface->get_n_rows = gucharmap_chartable_accessible_get_n_rows;
  iface->get_row_extent_at = gucharmap_chartable_accessible_get_row_extent_at;
  iface->get_index_at = gucharmap_chartable_accessible_get_index_at;
  iface->get_column_at_index = gucharmap_chartable_accessible_get_column_at_index;
  iface->get_row_at_index = gucharmap_chartable_accessible_get_row_at_index;
}

#if 0
/* See http://bugzilla.mate.org/show_bug.cgi?id=502840 */
#include <atk/atktype.h>

ATK_DEFINE_TYPE_WITH_CODE (GucharmapChartableAccessible,
                           gucharmap_chartable_accessible,
                           GUCHARMAP_TYPE_CHARTABLE,
                           G_IMPLEMENT_INTERFACE (ATK_TYPE_TABLE,
                                                  gucharmap_chartable_accessible_table_interface_init)
                           G_IMPLEMENT_INTERFACE (ATK_TYPE_COMPONENT,
                                                  gucharmap_chartable_accessible_component_interface_init))

#else

GType
gucharmap_chartable_accessible_get_type (void)
{
  static volatile gsize type__volatile = 0;

  if (g_once_init_enter (&type__volatile))
    {
      GTypeInfo typeinfo =
      {
        0 /* filled in by the code below */,
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) gucharmap_chartable_accessible_class_init, /* class init */
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        0 /* filled in by the code below */,
        0, /* nb preallocs */
        (GInstanceInitFunc) gucharmap_chartable_accessible_init, /* instance init */
        NULL /* value table */
      };
      const GInterfaceInfo atk_table_info =
      {
          (GInterfaceInitFunc) gucharmap_chartable_accessible_table_interface_init,
          (GInterfaceFinalizeFunc) NULL,
          NULL
      };
      const GInterfaceInfo atk_component_info =
      {
          (GInterfaceInitFunc) gucharmap_chartable_accessible_component_interface_init,
          (GInterfaceFinalizeFunc) NULL,
          NULL
      };
      AtkObjectFactory *factory;
      GType derived_type;
      GTypeQuery query;
      GType derived_atk_type;
      GType type;

      /* Figure out the size of the class and instance we are deriving from */
      derived_type = g_type_parent (GUCHARMAP_TYPE_CHARTABLE);
      factory = atk_registry_get_factory (atk_get_default_registry (), 
                                          derived_type);
      derived_atk_type = atk_object_factory_get_accessible_type (factory);
      g_type_query (derived_atk_type, &query);
      typeinfo.class_size = query.class_size;
      typeinfo.instance_size = query.instance_size;

      type = g_type_register_static (derived_atk_type,
                                     "GucharmapChartableAccessible",
                                     &typeinfo, 0);

      g_type_add_interface_static (type, ATK_TYPE_TABLE,
                                   &atk_table_info);
      g_type_add_interface_static (type, ATK_TYPE_COMPONENT,
                                   &atk_component_info);

      g_once_init_leave (&type__volatile, type);
    }

  return type__volatile;
}
#endif

/* API */

AtkObject* 
gucharmap_chartable_accessible_new (GucharmapChartable *chartable)
{
  GObject *object;
  AtkObject *accessible;

  object = g_object_new (gucharmap_chartable_accessible_get_type (), NULL);
  accessible = ATK_OBJECT (object);

  /* atk is fucked up... */
  atk_object_initialize (accessible, GTK_WIDGET (chartable));
  accessible->role = ATK_ROLE_TABLE;
  atk_object_set_name (accessible, _("Character Table"));

  return accessible;
}
