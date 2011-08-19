/* -*- Mode: C; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/* logview-loglist.c - displays a list of the opened logs
 *
 * Copyright (C) 2005 Vincent Noel <vnoel@cox.net>
 * Copyright (C) 2008 Cosimo Cecchi <cosimoc@mate.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "logview-manager.h"
#include "logview-log.h"
#include "logview-utils.h"

#include "logview-loglist.h"

struct _LogviewLoglistPrivate {
  GtkTreeStore *model;
  LogviewManager *manager;
  GtkTreePath *selection;
  gboolean has_day_selection;
};

G_DEFINE_TYPE (LogviewLoglist, logview_loglist, GTK_TYPE_TREE_VIEW);

#define GET_PRIVATE(o)  \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), LOGVIEW_TYPE_LOGLIST, LogviewLoglistPrivate))

enum {
  LOG_OBJECT = 0,
  LOG_NAME,
  LOG_WEIGHT,
  LOG_WEIGHT_SET,
  LOG_DAY
};

enum {
  DAY_SELECTED,
  DAY_CLEARED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
save_day_selection (LogviewLoglist *loglist, GtkTreeIter *iter)
{
  if (loglist->priv->selection) {
    gtk_tree_path_free (loglist->priv->selection);
  }

  loglist->priv->selection = gtk_tree_model_get_path
    (GTK_TREE_MODEL (loglist->priv->model), iter);
}

static void
update_days_and_lines_for_log (LogviewLoglist *loglist,
                               GtkTreeIter *log, GSList *days)
{
  gboolean res;
  GtkTreeIter iter, dummy;
  GSList *l;
  int i;
  char date[200];
  Day *day;

  /* if we have some days, we can't remove all the items immediately, otherwise,
   * if the row is expanded, it will be collapsed because there are no items,
   * so we create a dummy entry, remove all the others and then remove the
   * dummy one.
   */
  res = gtk_tree_model_iter_children (GTK_TREE_MODEL (loglist->priv->model),
                                      &iter, log);
  if (res) {
    gtk_tree_store_insert_before (loglist->priv->model, &dummy, log, &iter);
    gtk_tree_store_set (loglist->priv->model, &dummy,
                        LOG_NAME, "", -1);
    do {
      gtk_tree_store_remove (loglist->priv->model, &iter);
    } while (gtk_tree_store_iter_is_valid (loglist->priv->model, &iter));
  }

  for (i = 1, l = days; l; l = l->next) {
    /* now insert all the days */
    day = l->data;

    g_date_strftime (date, 200, "%A, %e %b", day->date);

    gtk_tree_store_insert (GTK_TREE_STORE (loglist->priv->model),
                           &iter, log, i);
    gtk_tree_store_set (GTK_TREE_STORE (loglist->priv->model),
                        &iter, LOG_NAME, date, LOG_DAY, day, -1);
    i++;
  }

  if (res) {
    gtk_tree_store_remove (loglist->priv->model, &dummy);
  }
}

static GtkTreeIter *
logview_loglist_find_log (LogviewLoglist *list, LogviewLog *log)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeIter *retval = NULL;
  LogviewLog *current;

  model = GTK_TREE_MODEL (list->priv->model);

  if (!gtk_tree_model_get_iter_first (model, &iter)) {
    return NULL;
  }

  do {
    gtk_tree_model_get (model, &iter, LOG_OBJECT, &current, -1);
    if (current == log) {
      retval = gtk_tree_iter_copy (&iter);
    }
    g_object_unref (current);
  } while (gtk_tree_model_iter_next (model, &iter) != FALSE && retval == NULL);

  return retval;
}

static void
log_changed_cb (LogviewLog *log,
                gpointer user_data)
{
  LogviewLoglist *list = user_data;
  LogviewLog *active;
  GtkTreeIter *iter;

  active = logview_manager_get_active_log (list->priv->manager);

  if (log == active) {
    g_object_unref (active);
    return;
  }

  iter = logview_loglist_find_log (list, log);

  if (!iter) {
    return;
  }

  /* make the log bold in the list */
  gtk_tree_store_set (list->priv->model, iter,
                      LOG_WEIGHT, PANGO_WEIGHT_BOLD,
                      LOG_WEIGHT_SET, TRUE, -1);

  gtk_tree_iter_free (iter);
}


static void
tree_selection_changed_cb (GtkTreeSelection *selection,
                           gpointer user_data)
{
  LogviewLoglist *list = user_data;
  GtkTreeModel *model;
  GtkTreeIter iter, parent;
  LogviewLog *log;
  gboolean is_bold, is_active;
  Day *day;

  if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
      return;
  }

  gtk_tree_model_get (model, &iter, LOG_OBJECT, &log,
                      LOG_WEIGHT_SET, &is_bold,
                      LOG_DAY, &day, -1);
  if (log) {
    is_active = logview_manager_log_is_active (list->priv->manager, log);

    if (is_active && list->priv->has_day_selection) {
      list->priv->has_day_selection = FALSE;
      g_signal_emit (list, signals[DAY_CLEARED], 0, NULL);
    } else if (!is_active) {
      logview_manager_set_active_log (list->priv->manager, log);
    }
  } else if (day) {
    list->priv->has_day_selection = TRUE;
    gtk_tree_model_iter_parent (model, &parent, &iter);
    gtk_tree_model_get (model, &parent, LOG_OBJECT, &log, -1);

    if (!logview_manager_log_is_active (list->priv->manager, log)) {
      save_day_selection (list, &iter);      
      logview_manager_set_active_log (list->priv->manager, log);
    } else {
      g_signal_emit (list, signals[DAY_SELECTED], 0, day, NULL);
    }
  }

  if (is_bold) {
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                        LOG_WEIGHT_SET, FALSE, -1);
  }

  if (log) {
    g_object_unref (log);
  }
}

static void
manager_active_changed_cb (LogviewManager *manager,
                           LogviewLog *log,
                           LogviewLog *old_log,
                           gpointer user_data)
{
  LogviewLoglist *list = user_data;
  GtkTreeIter * iter, sel_iter;
  GtkTreeSelection * selection;

  if (list->priv->selection && 
      gtk_tree_model_get_iter (GTK_TREE_MODEL (list->priv->model),
                               &sel_iter, list->priv->selection))
  {
    Day *day;

    iter = gtk_tree_iter_copy (&sel_iter);

    gtk_tree_model_get (GTK_TREE_MODEL (list->priv->model), iter,
                        LOG_DAY, &day, -1);

    if (day) {
      g_signal_emit (list, signals[DAY_SELECTED], 0, day, NULL);
    }

    gtk_tree_path_free (list->priv->selection);
    list->priv->selection = NULL;
  } else {
    iter = logview_loglist_find_log (list, log);
  }

  if (!iter) {
    return;
  }

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
  g_signal_handlers_block_by_func (selection, tree_selection_changed_cb, list);

  gtk_tree_selection_select_iter (selection, iter);

  g_signal_handlers_unblock_by_func (selection, tree_selection_changed_cb, list);
  gtk_tree_iter_free (iter);
}

static void
manager_log_closed_cb (LogviewManager *manager,
                       LogviewLog *log,
                       gpointer user_data)
{
  LogviewLoglist *list = user_data;
  GtkTreeIter *iter;
  gboolean res;

  iter = logview_loglist_find_log (list, log);

  if (!iter) {
    return;
  }

  g_signal_handlers_disconnect_by_func (log, log_changed_cb, list);

  res = gtk_tree_store_remove (list->priv->model, iter);
  if (res) {
    GtkTreeSelection *selection;

    /* iter now points to the next valid row */
    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
    gtk_tree_selection_select_iter (selection, iter);
  } else {
    /* FIXME: what shall we do here? */
  }

  gtk_tree_iter_free (iter);
}

static void
manager_log_added_cb (LogviewManager *manager,
                      LogviewLog *log,
                      gpointer user_data)
{
  LogviewLoglist *list = user_data;
  GtkTreeIter iter, child;

  gtk_tree_store_append (list->priv->model, &iter, NULL);
  gtk_tree_store_set (list->priv->model, &iter,
                      LOG_OBJECT, g_object_ref (log),
                      LOG_NAME, logview_log_get_display_name (log), -1);
  if (logview_log_get_has_days (log)) {
    gtk_tree_store_insert (list->priv->model,
                           &child, &iter, 0);
    gtk_tree_store_set (list->priv->model, &child,
                        LOG_NAME, _("Loading..."), -1);
  }

  g_signal_connect (log, "log-changed",
                    G_CALLBACK (log_changed_cb), list);
}

static void
row_expanded_cb (GtkTreeView *view,
                 GtkTreeIter *iter,
                 GtkTreePath *path,
                 gpointer user_data)
{
  LogviewLoglist *list = user_data;
  LogviewLog *log;

  gtk_tree_model_get (GTK_TREE_MODEL (list->priv->model), iter,
                      LOG_OBJECT, &log, -1);
  if (!logview_manager_log_is_active (list->priv->manager, log)) {
    logview_manager_set_active_log (list->priv->manager, log);
  }

  g_object_unref (log);
}

static int
loglist_sort_func (GtkTreeModel *model,
                   GtkTreeIter *a,
                   GtkTreeIter *b,
                   gpointer user_data)
{
  char *name_a, *name_b;
  Day *day_a, *day_b;
  int retval = 0;

  switch (gtk_tree_store_iter_depth (GTK_TREE_STORE (model), a)) {
    case 0:
      gtk_tree_model_get (model, a, LOG_NAME, &name_a, -1);
      gtk_tree_model_get (model, b, LOG_NAME, &name_b, -1);
      retval = g_strcmp0 (name_a, name_b);
      g_free (name_a);
      g_free (name_b);

      break;
    case 1:
      gtk_tree_model_get (model, a, LOG_DAY, &day_a, -1);
      gtk_tree_model_get (model, b, LOG_DAY, &day_b, -1);
      if (day_a && day_b) {
        retval = days_compare (day_a, day_b);
      } else {
        retval = 0;
      }

      break;
    default:
      g_assert_not_reached ();

      break;
  }

  return retval;
}

static void
do_finalize (GObject *obj)
{
  LogviewLoglist *list = LOGVIEW_LOGLIST (obj);

  g_object_unref (list->priv->model);
  list->priv->model = NULL;

  if (list->priv->selection) {
    gtk_tree_path_free (list->priv->selection);
    list->priv->selection = NULL;
  }

  G_OBJECT_CLASS (logview_loglist_parent_class)->finalize (obj);
}

static void 
logview_loglist_init (LogviewLoglist *list)
{
  GtkTreeStore *model;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;
  GtkCellRenderer *cell;

  list->priv = GET_PRIVATE (list);
  list->priv->has_day_selection = FALSE;
  list->priv->selection = NULL;

  model = gtk_tree_store_new (5, LOGVIEW_TYPE_LOG, G_TYPE_STRING, G_TYPE_INT,
                              G_TYPE_BOOLEAN, G_TYPE_POINTER);
  gtk_tree_view_set_model (GTK_TREE_VIEW (list), GTK_TREE_MODEL (model));
  list->priv->model = model;
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (list), FALSE);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (tree_selection_changed_cb), list);

  cell = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text", LOG_NAME,
                                       "weight-set", LOG_WEIGHT_SET,
                                       "weight", LOG_WEIGHT,
                                       NULL);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (list->priv->model), LOG_NAME, GTK_SORT_ASCENDING);
  gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (list->priv->model),
                                   LOG_NAME,
                                   (GtkTreeIterCompareFunc) loglist_sort_func,
                                   list, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (list), column);
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (list), -1);

  list->priv->manager = logview_manager_get ();

  g_signal_connect (list->priv->manager, "log-added",
                    G_CALLBACK (manager_log_added_cb), list);
  g_signal_connect (list->priv->manager, "log-closed",
                    G_CALLBACK (manager_log_closed_cb), list);
  g_signal_connect_after (list->priv->manager, "active-changed",
                          G_CALLBACK (manager_active_changed_cb), list);
  g_signal_connect (list, "row-expanded",
                    G_CALLBACK (row_expanded_cb), list);
}

static void
logview_loglist_class_init (LogviewLoglistClass *klass)
{
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  oclass->finalize = do_finalize;

  signals[DAY_SELECTED] = g_signal_new ("day-selected",
                                        G_OBJECT_CLASS_TYPE (oclass),
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET (LogviewLoglistClass, day_selected),
                                        NULL, NULL,
                                        g_cclosure_marshal_VOID__POINTER,
                                        G_TYPE_NONE, 1,
                                        G_TYPE_POINTER);

  signals[DAY_CLEARED] = g_signal_new ("day-cleared",
                                       G_OBJECT_CLASS_TYPE (oclass),
                                       G_SIGNAL_RUN_LAST,
                                       G_STRUCT_OFFSET (LogviewLoglistClass, day_cleared),
                                       NULL, NULL,
                                       g_cclosure_marshal_VOID__VOID,
                                       G_TYPE_NONE, 0);

  g_type_class_add_private (klass, sizeof (LogviewLoglistPrivate));
}

/* public methods */

GtkWidget *
logview_loglist_new (void)
{
  GtkWidget *widget;
  widget = g_object_new (LOGVIEW_TYPE_LOGLIST, NULL);
  return widget;
}

void
logview_loglist_update_lines (LogviewLoglist *loglist, LogviewLog *log)
{
  GSList *days;
  GtkTreeIter *parent;

  g_assert (LOGVIEW_IS_LOGLIST (loglist));
  g_assert (LOGVIEW_IS_LOG (log));

  days = logview_log_get_days_for_cached_lines (log);
  parent = logview_loglist_find_log (loglist, log);
  update_days_and_lines_for_log (loglist, parent, days);

  gtk_tree_iter_free (parent);
}

