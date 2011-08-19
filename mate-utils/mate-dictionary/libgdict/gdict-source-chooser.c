/* gdict-source-chooser.h - display widget for dictionary sources
 *
 * Copyright (C) 2007  Emmanuele Bassi <ebassi@mate.org>
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
 */

/**
 * SECTION:gdict-source-chooser
 * @short_description: Display the list of available sources
 *
 * #GdictSourceChooser is a widget that shows the list of available
 * dictionary sources using a #GdictSourceLoader instance as a model.
 * It can be used to allow choosing the current dictionary source.
 *
 * #GdictSourceChooser is available since Gdict 0.12.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include <glib/gi18n-lib.h>

#include <gdk/gdk.h>

#include <gtk/gtk.h>

#include "gdict-source-chooser.h"
#include "gdict-utils.h"
#include "gdict-private.h"
#include "gdict-enum-types.h"
#include "gdict-marshal.h"

#define GDICT_SOURCE_CHOOSER_GET_PRIVATE(obj)   \
        (G_TYPE_INSTANCE_GET_PRIVATE ((obj),    \
         GDICT_TYPE_SOURCE_CHOOSER,             \
         GdictSourceChooserPrivate))

struct _GdictSourceChooserPrivate
{
  GtkListStore *store;

  GtkWidget *treeview;
  GtkWidget *refresh_button;
  GtkWidget *buttons_box;

  GdictSourceLoader *loader;
  gint n_sources;

  GdkCursor *busy_cursor;
  
  gchar *current_source;
};

enum
{
  SOURCE_TRANSPORT,
  SOURCE_NAME,
  SOURCE_DESCRIPTION,
  SOURCE_CURRENT,

  SOURCE_N_COLUMNS
};

enum
{
  PROP_0,

  PROP_LOADER,
  PROP_COUNT
};

enum
{
  SOURCE_ACTIVATED,
  SELECTION_CHANGED,

  LAST_SIGNAL
};

static guint source_chooser_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (GdictSourceChooser, gdict_source_chooser, GTK_TYPE_VBOX);

static void
gdict_source_chooser_finalize (GObject *gobject)
{
  GdictSourceChooser *chooser = GDICT_SOURCE_CHOOSER (gobject);
  GdictSourceChooserPrivate *priv = chooser->priv;

  g_free (priv->current_source);

  G_OBJECT_CLASS (gdict_source_chooser_parent_class)->finalize (gobject);
}

static void
gdict_source_chooser_dispose (GObject *gobject)
{
  GdictSourceChooser *chooser = GDICT_SOURCE_CHOOSER (gobject);
  GdictSourceChooserPrivate *priv = chooser->priv;

  if (priv->store)
    {
      g_object_unref (priv->store);
      priv->store = NULL;
    }

  if (priv->loader)
    {
      g_object_unref (priv->loader);
      priv->loader = NULL;
    }

  if (priv->busy_cursor)
    {
      gdk_cursor_unref (priv->busy_cursor);
      priv->busy_cursor = NULL;
    }

  G_OBJECT_CLASS (gdict_source_chooser_parent_class)->dispose (gobject);
}

static void
gdict_source_chooser_set_property (GObject      *gobject,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_LOADER:
      gdict_source_chooser_set_loader (GDICT_SOURCE_CHOOSER (gobject),
                                       g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
gdict_source_chooser_get_property (GObject    *gobject,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GdictSourceChooserPrivate *priv;

  priv = GDICT_SOURCE_CHOOSER (gobject)->priv;

  switch (prop_id)
    {
    case PROP_LOADER:
      g_value_set_object (value, priv->loader);
      break;
    case PROP_COUNT:
      g_value_set_int (value, priv->n_sources);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
row_activated_cb (GtkTreeView       *treeview,
                  GtkTreePath       *path,
                  GtkTreeViewColumn *column,
                  gpointer           data)
{
  GdictSourceChooser *chooser = GDICT_SOURCE_CHOOSER (data);
  GdictSourceChooserPrivate *priv = chooser->priv;
  GtkTreeIter iter;
  gchar *name;
  GdictSource *source;

  if (!priv->loader)
    return;

  if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->store), &iter, path))
    return;

  gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
                      SOURCE_NAME, &name,
                      -1);
  if (!name)
    return;

  source = gdict_source_loader_get_source (priv->loader, name);
  if (!source)
    {
      g_free (name);
      return;
    }

  g_signal_emit (chooser, source_chooser_signals[SOURCE_ACTIVATED], 0,
                 name, source);

  g_free (name);
  g_object_unref (source);
}

static void
refresh_button_clicked_cb (GtkButton *button,
                           gpointer   data)
{
  gdict_source_chooser_refresh (GDICT_SOURCE_CHOOSER (data));
}

static void
selection_changed_cb (GtkTreeSelection *selection,
                      gpointer          data)
{
  g_signal_emit (data, source_chooser_signals[SELECTION_CHANGED], 0);
}

static GObject *
gdict_source_chooser_constructor (GType                  gtype,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GdictSourceChooser *chooser;
  GdictSourceChooserPrivate *priv;
  GObjectClass *parent_class;
  GObject *retval;
  GtkWidget *sw;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *hbox;

  parent_class = G_OBJECT_CLASS (gdict_source_chooser_parent_class);
  retval = parent_class->constructor (gtype, n_params, params);

  chooser = GDICT_SOURCE_CHOOSER (retval);
  priv = chooser->priv;

  gtk_widget_push_composite_child ();

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_composite_name (sw, "gdict-source-chooser-scrolled-window");
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (chooser), sw, TRUE, TRUE, 0);
  gtk_widget_show (sw);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("sources",
                                                     renderer,
                                                     "text", SOURCE_DESCRIPTION,
                                                     "weight", SOURCE_CURRENT,
                                                     NULL);
  priv->treeview = gtk_tree_view_new ();
  gtk_widget_set_composite_name (priv->treeview, "gdict-source-chooser-treeview");
  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview),
                           GTK_TREE_MODEL (priv->store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->treeview), FALSE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->treeview), column);
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview)),
                    "changed", G_CALLBACK (selection_changed_cb),
                    chooser);
  g_signal_connect (priv->treeview,
                    "row-activated", G_CALLBACK (row_activated_cb),
                    chooser);
  gtk_container_add (GTK_CONTAINER (sw), priv->treeview);
  gtk_widget_show (priv->treeview);

  hbox = gtk_hbox_new (FALSE, 6);
  priv->buttons_box = hbox;

  priv->refresh_button = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (priv->refresh_button),
                        gtk_image_new_from_stock (GTK_STOCK_REFRESH,
                                                  GTK_ICON_SIZE_BUTTON));
  g_signal_connect (priv->refresh_button,
                    "clicked", G_CALLBACK (refresh_button_clicked_cb),
                    chooser);
  gtk_box_pack_start (GTK_BOX (hbox), priv->refresh_button, FALSE, FALSE, 0);
  gtk_widget_show (priv->refresh_button);
  gtk_widget_set_tooltip_text (priv->refresh_button,
                               _("Reload the list of available sources"));

  gtk_box_pack_end (GTK_BOX (chooser), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  gtk_widget_pop_composite_child ();

  return retval;
}

static void
gdict_source_chooser_class_init (GdictSourceChooserClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GdictSourceChooserPrivate));

  gobject_class->finalize = gdict_source_chooser_finalize;
  gobject_class->dispose = gdict_source_chooser_dispose;
  gobject_class->set_property = gdict_source_chooser_set_property;
  gobject_class->get_property = gdict_source_chooser_get_property;
  gobject_class->constructor = gdict_source_chooser_constructor;

  /**
   * GdictSourceChooser:loader:
   *
   * The #GdictSourceLoader used to retrieve the list of available
   * dictionary sources.
   *
   * Since: 0.12
   */
  g_object_class_install_property (gobject_class,
                                   PROP_LOADER,
                                   g_param_spec_object ("loader",
                                                        "Loader",
                                                        "The GdictSourceLoader used to get the list of sources",
                                                        GDICT_TYPE_SOURCE_LOADER,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  /**
   * GdictSourceChooser:count:
   *
   * The number of available dictionary sources, or -1 if no
   * #GdictSourceLoader is set.
   *
   * Since: 0.12
   */
  g_object_class_install_property (gobject_class,
                                   PROP_COUNT,
                                   g_param_spec_int ("count",
                                                     "Count",
                                                     "The number of available dictionary sources",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READABLE));

  /**
   * GdictSourceChooser::source-activated:
   * @chooser: the #GdictSourceChooser that received the signal
   * @source_name: the name of the activated source
   * @source: the activated #GdictSource
   *
   * The ::source-activated signal is emitted each time the user
   * activates a row in the source chooser widget, either by double
   * clicking on it or by a keyboard event.
   *
   * Since: 0.12
   */
  source_chooser_signals[SOURCE_ACTIVATED] =
    g_signal_new ("source-activated",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GdictSourceChooserClass, source_activated),
                  NULL, NULL,
                  gdict_marshal_VOID__STRING_OBJECT,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  GDICT_TYPE_SOURCE);
  /**
   * GdictSourceChooser::selection-changed:
   * @chooser: the #GdictSourceChooser that received the signal
   *
   * The ::selection-changed signal is emitted each time the
   * selection inside the source chooser widget has been changed.
   *
   * Since: 0.12
   */
  source_chooser_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GdictSourceChooserClass, selection_changed),
                  NULL, NULL,
                  gdict_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}

static void
gdict_source_chooser_init (GdictSourceChooser *chooser)
{
  GdictSourceChooserPrivate *priv;

  chooser->priv = priv = GDICT_SOURCE_CHOOSER_GET_PRIVATE (chooser);

  priv->store = gtk_list_store_new (SOURCE_N_COLUMNS,
                                    G_TYPE_INT,    /* TRANSPORT */
                                    G_TYPE_STRING, /* NAME */
                                    G_TYPE_STRING, /* DESCRIPTION */
                                    G_TYPE_INT     /* CURRENT */);

  priv->loader = NULL;
  priv->n_sources = -1;

  priv->busy_cursor = gdk_cursor_new (GDK_WATCH);
}

/**
 * gdict_source_chooser_new:
 *
 * Creates a new #GdictSourceChooser widget. This widget can be used to
 * display the list of available dictionary sources.
 *
 * Return value: the newly created #GdictSourceChooser widget.
 *
 * Since: 0.12
 */
GtkWidget *
gdict_source_chooser_new (void)
{
  return g_object_new (GDICT_TYPE_SOURCE_CHOOSER, NULL);
}

/**
 * gdict_source_chooser_new_with_loader:
 * @loader: a #GdictSourceLoader
 *
 * Creates a new #GdictSourceChooser widget and sets @loader as the
 * #GdictSourceLoader object to be used to retrieve the list of
 * available dictionary sources.
 *
 * Return value: the newly created #GdictSourceChooser widget.
 *
 * Since: 0.12
 */
GtkWidget *
gdict_source_chooser_new_with_loader (GdictSourceLoader *loader)
{
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), NULL);

  return g_object_new (GDICT_TYPE_SOURCE_CHOOSER, "loader", loader, NULL);
}

/**
 * gdict_source_chooser_set_loader:
 * @chooser: a #GdictSourceChooser
 * @loader: a #GdictSourceLoader or %NULL to unset it
 *
 * Sets the #GdictSourceLoader to be used by the source chooser
 * widget.
 *
 * Since: 0.12
 */
void
gdict_source_chooser_set_loader (GdictSourceChooser *chooser,
                                 GdictSourceLoader  *loader)
{
  GdictSourceChooserPrivate *priv;

  g_return_if_fail (GDICT_IS_SOURCE_CHOOSER (chooser));
  g_return_if_fail (loader == NULL || GDICT_IS_SOURCE_LOADER (loader));

  priv = chooser->priv;

  if (priv->loader != loader)
    {
      if (priv->loader)
        g_object_unref (priv->loader);

      if (loader)
        {
          priv->loader = g_object_ref (loader);
          gdict_source_chooser_refresh (chooser);
        }

      g_object_notify (G_OBJECT (chooser), "loader");
    }
}

/**
 * gdict_source_chooser_get_loader:
 * @chooser: a #GdictSourceChooser
 * 
 * Retrieves the #GdictSourceLoader used by @chooser.
 *
 * Return value: a #GdictSourceLoader or %NULL is none is set
 *
 * Since: 0.12
 */
GdictSourceLoader *
gdict_source_chooser_get_loader (GdictSourceChooser *chooser)
{
  g_return_val_if_fail (GDICT_IS_SOURCE_CHOOSER (chooser), NULL);

  return chooser->priv->loader;
}

typedef struct
{
  gchar *source_name;
  GdictSourceChooser *chooser;
  
  guint found       : 1;
  guint do_select   : 1;
  guint do_activate : 1;
} SelectData;

static gboolean
scan_for_source_name (GtkTreeModel *model,
                      GtkTreePath  *path,
                      GtkTreeIter  *iter,
                      gpointer      user_data)
{
  SelectData *select_data = user_data;
  gchar *source_name = NULL;

  if (!select_data)
    return TRUE;

  gtk_tree_model_get (model, iter, SOURCE_NAME, &source_name, -1);
  if (!source_name)
    return FALSE;

  if (strcmp (source_name, select_data->source_name) == 0)
    {
      GtkTreeView *tree_view;
      GtkTreeSelection *selection;

      select_data->found = TRUE;

      tree_view = GTK_TREE_VIEW (select_data->chooser->priv->treeview);

      if (select_data->do_activate)
        {
          GtkTreeViewColumn *column;

          column = gtk_tree_view_get_column (tree_view, 2);

          gtk_list_store_set (GTK_LIST_STORE (model), iter,
                              SOURCE_CURRENT, PANGO_WEIGHT_BOLD,
                              -1);

          gtk_tree_view_row_activated (tree_view, path, column);
        }

      selection = gtk_tree_view_get_selection (tree_view);
      if (select_data->do_select)
        gtk_tree_selection_select_path (selection, path);
      else
        gtk_tree_selection_unselect_path (selection, path);
    }
  else
    {
      gtk_list_store_set (GTK_LIST_STORE (model), iter,
                          SOURCE_CURRENT, PANGO_WEIGHT_NORMAL,
                          -1);
    }

  g_free (source_name);

  return FALSE;
}

/**
 * gdict_source_chooser_select_source:
 * @chooser: a #GdictSourceChooser
 * @source_name: the name of a dictionary source
 *
 * Selects the dictionary source named @source_name inside @chooser.
 * The selection is moved but the row containing the dictionary source
 * is not activated.
 *
 * Return value: %TRUE if the source was found and selected
 *
 * Since: 0.12
 */
gboolean
gdict_source_chooser_select_source (GdictSourceChooser *chooser,
                                    const gchar        *source_name)
{
  GdictSourceChooserPrivate *priv;
  SelectData data;
  gboolean retval;

  g_return_val_if_fail (GDICT_IS_SOURCE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (source_name != NULL, FALSE);

  priv = chooser->priv;

  data.source_name = g_strdup (source_name);
  data.chooser = chooser;
  data.found = FALSE;
  data.do_select = TRUE;
  data.do_activate = FALSE;

  gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                          scan_for_source_name,
                          &data);

  retval = data.found;

  g_free (data.source_name);

  return retval;
}

/**
 * gdict_source_chooser_unselect_source:
 * @chooser: a #GdictSourceChooser
 * @source_name: the name of a dictionary source
 *
 * Unselects @source_name inside @chooser.
 *
 * Return value: %TRUE if the source was found and unselected
 *
 * Since: 0.12
 */
gboolean
gdict_source_chooser_unselect_source (GdictSourceChooser *chooser,
                                      const gchar        *source_name)
{
  GdictSourceChooserPrivate *priv;
  SelectData data;
  gboolean retval;

  g_return_val_if_fail (GDICT_IS_SOURCE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (source_name != NULL, FALSE);

  priv = chooser->priv;

  data.source_name = g_strdup (source_name);
  data.chooser = chooser;
  data.found = FALSE;
  data.do_select = FALSE;
  data.do_activate = FALSE;

  gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                          scan_for_source_name,
                          &data);

  retval = data.found;

  g_free (data.source_name);

  return retval;
}

/**
 * gdict_source_chooser_set_current_source:
 * @chooser: a #GdictSourceChooser
 * @source_name: the name of a dictionary source
 *
 * Sets the current dictionary source named @source_name. The row
 * of the source, if found, will be selected and activated.
 *
 * Return value: %TRUE if the source was found
 *
 * Since: 0.12
 */
gboolean
gdict_source_chooser_set_current_source (GdictSourceChooser *chooser,
                                         const gchar        *source_name)
{
  GdictSourceChooserPrivate *priv;
  SelectData data;
  gboolean retval;

  g_return_val_if_fail (GDICT_IS_SOURCE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (source_name != NULL, FALSE);

  priv = chooser->priv;

  if (priv->current_source && !strcmp (priv->current_source, source_name))
    return TRUE;

  data.source_name = g_strdup (source_name);
  data.chooser = chooser;
  data.found = FALSE;
  data.do_select = TRUE;
  data.do_activate = TRUE;

  gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                          scan_for_source_name,
                          &data);

  retval = data.found;

  GDICT_NOTE (CHOOSER, "%s current source: %s",
              data.found ? "set" : "not set",
              data.source_name);

  if (data.found)
    {
      g_free (priv->current_source);
      priv->current_source = data.source_name;
    }
  else
    g_free (data.source_name);

  return retval;
}

/**
 * gdict_source_chooser_get_current_source:
 * @chooser: a #GdictSourceChooser
 *
 * Retrieves the currently selected source.
 *
 * Return value: a newly allocated string containing the name of
 *   the currently selected source. Use g_free() when done using it
 *
 * Since: 0.12
 */
gchar *
gdict_source_chooser_get_current_source (GdictSourceChooser *chooser)
{
  GdictSourceChooserPrivate *priv;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *retval = NULL;

  g_return_val_if_fail (GDICT_IS_SOURCE_CHOOSER (chooser), NULL);
  
  priv = chooser->priv;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return NULL;

  gtk_tree_model_get (model, &iter, SOURCE_NAME, &retval, -1);
  
  g_free (priv->current_source);
  priv->current_source = g_strdup (retval);

  return retval;
}

/**
 * gdict_source_chooser_get_sources:
 * @chooser: a #GdictSouceChooser
 * @length: return location for the length of the returned vector
 *
 * Retrieves the names of the available dictionary sources.
 *
 * Return value: a newly allocated, %NULL terminated string vector
 *   containing the names of the available sources. Use g_strfreev()
 *   when done using it.
 *
 * Since: 0.12
 */
gchar **
gdict_source_chooser_get_sources (GdictSourceChooser *chooser,
                                  gsize              *length)
{
  GdictSourceChooserPrivate *priv;
  gchar **retval;
  gsize retval_len;

  g_return_val_if_fail (GDICT_IS_SOURCE_CHOOSER (chooser), NULL);

  priv = chooser->priv;

  if (!priv->loader)
    return NULL;

  retval = gdict_source_loader_get_names (priv->loader, &retval_len);
  if (length)
    *length = retval_len;

  return retval;
}

/**
 * gdict_source_chooser_count_sources:
 * @chooser: a #GdictSourceChooser
 *
 * Retrieve the number of available dictionary sources.
 *
 * Return value: the number of available sources, or -1 if no
 *   #GdictSourceLoader has been set
 *
 * Since: 0.12
 */
gint
gdict_source_chooser_count_sources (GdictSourceChooser *chooser)
{
  g_return_val_if_fail (GDICT_IS_SOURCE_CHOOSER (chooser), -1);

  return chooser->priv->n_sources;
}

/**
 * gdict_source_chooser_has_source:
 * @chooser: a #GdictSourceChooser
 * @source_name: the name of a dictionary source
 *
 * Checks whether @chooser has a dictionary source named @source_name.
 *
 * Return value: %TRUE if the dictionary source was found
 *
 * Since: 0.12
 */
gboolean
gdict_source_chooser_has_source (GdictSourceChooser *chooser,
                                 const gchar        *source_name)
{
  GdictSourceChooserPrivate *priv;

  g_return_val_if_fail (GDICT_IS_SOURCE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (source_name != NULL, FALSE);

  priv = chooser->priv;

  if (!priv->loader)
    return FALSE;

  return gdict_source_loader_has_source (priv->loader, source_name);
}

/**
 * gdict_source_chooser_refresh:
 * @chooser: a #GdictSourceChooser
 *
 * Forces a refresh on the contents of the source chooser widget
 *
 * Since: 0.12
 */
void
gdict_source_chooser_refresh (GdictSourceChooser *chooser)
{
  GdictSourceChooserPrivate *priv;

  g_return_if_fail (GDICT_IS_SOURCE_CHOOSER (chooser));

  priv = chooser->priv;

  if (priv->loader)
    {
      const GSList *sources, *l;

      if (priv->treeview)
        gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview), NULL);

      gtk_list_store_clear (priv->store);

      sources = gdict_source_loader_get_sources (priv->loader);
      for (l = sources; l != NULL; l = l->next)
        {
          GdictSource *source = l->data;
          const gchar *name, *description;
          GdictSourceTransport transport;
          gint weight;

          transport = gdict_source_get_transport (source);
          name = gdict_source_get_name (source);
          description = gdict_source_get_description (source);
          weight = PANGO_WEIGHT_NORMAL;

          if (priv->current_source && !strcmp (priv->current_source, name))
            weight = PANGO_WEIGHT_BOLD;

          gtk_list_store_insert_with_values (priv->store, NULL, -1,
                                             SOURCE_TRANSPORT, transport,
                                             SOURCE_NAME, name,
                                             SOURCE_DESCRIPTION, description,
                                             SOURCE_CURRENT, weight,
                                             -1);
        }

      if (priv->treeview)
        gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview),
                                 GTK_TREE_MODEL (priv->store));
    }
}

/**
 * gdict_source_chooser_add_button:
 * @chooser: a #GdictSourceChooser
 * @button_text: text of the button
 *
 * Adds a #GtkButton with @button_text to the button area on
 * the bottom of @chooser. The @button_text can also be a
 * stock ID.
 *
 * Return value: the newly packed button.
 *
 * Since: 0.12
 */
GtkWidget *
gdict_source_chooser_add_button (GdictSourceChooser *chooser,
                                 const gchar        *button_text)
{
  GdictSourceChooserPrivate *priv;
  GtkWidget *button;

  g_return_val_if_fail (GDICT_IS_SOURCE_CHOOSER (chooser), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);

  priv = chooser->priv;

  button = gtk_button_new_from_stock (button_text);

  gtk_widget_set_can_default (button, TRUE);

  gtk_widget_show (button);

  gtk_box_pack_end (GTK_BOX (priv->buttons_box), button, FALSE, TRUE, 0);

  return button;
}

