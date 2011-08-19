/* gdict-strategy-chooser.c - display widget for strategy names
 *
 * Copyright (C) 2006  Emmanuele Bassi <ebassi@gmail.com>
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
 * SECTION:gdict-strategy-chooser
 * @short_description: Display a list of matching strategies
 *
 * Each #GdictContext allows matching a word using a specific "matching
 * strategy". The #GdictStrategyChooser widget queries a #GdictContext and
 * displays the list of available matching strategies.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "gdict-strategy-chooser.h"
#include "gdict-utils.h"
#include "gdict-debug.h"
#include "gdict-private.h"
#include "gdict-enum-types.h"
#include "gdict-marshal.h"

#define GDICT_STRATEGY_CHOOSER_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_STRATEGY_CHOOSER, GdictStrategyChooserPrivate))

struct _GdictStrategyChooserPrivate
{
  GtkListStore *store;

  GtkWidget *treeview;
  GtkWidget *clear_button;
  GtkWidget *refresh_button;
  GtkWidget *buttons_box;
  
  GdictContext *context;
  gint results;

  guint start_id;
  guint match_id;
  guint end_id;
  guint error_id;

  GdkCursor *busy_cursor;

  gchar *current_strat;

  guint is_searching : 1;
};

enum
{
  STRATEGY_NAME,
  STRATEGY_ERROR
} StratType;

enum
{
  STRAT_COLUMN_TYPE,
  STRAT_COLUMN_NAME,
  STRAT_COLUMN_DESCRIPTION,
  STRAT_COLUMN_CURRENT,

  STRAT_N_COLUMNS
};

enum
{
  PROP_0,
  
  PROP_CONTEXT,
  PROP_COUNT
};

enum
{
  STRATEGY_ACTIVATED,
  CLOSED,

  LAST_SIGNAL
};

static guint db_chooser_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GdictStrategyChooser,
               gdict_strategy_chooser,
               GTK_TYPE_VBOX);


static void
set_gdict_context (GdictStrategyChooser *chooser,
		   GdictContext         *context)
{
  GdictStrategyChooserPrivate *priv;
  
  g_assert (GDICT_IS_STRATEGY_CHOOSER (chooser));
  priv = chooser->priv;
  
  if (priv->context)
    {
      if (priv->start_id)
        {
          GDICT_NOTE (CHOOSER, "Removing old context handlers");
          
          g_signal_handler_disconnect (priv->context, priv->start_id);
          g_signal_handler_disconnect (priv->context, priv->match_id);
          g_signal_handler_disconnect (priv->context, priv->end_id);
          
          priv->start_id = 0;
          priv->end_id = 0;
          priv->match_id = 0;
        }
      
      if (priv->error_id)
        {
          g_signal_handler_disconnect (priv->context, priv->error_id);

          priv->error_id = 0;
        }

      GDICT_NOTE (CHOOSER, "Removing old context");
      
      g_object_unref (G_OBJECT (priv->context));
      
      priv->context = NULL;
      priv->results = -1;
    }

  if (!context)
    return;

  if (!GDICT_IS_CONTEXT (context))
    {
      g_warning ("Object of type '%s' instead of a GdictContext\n",
      		 g_type_name (G_OBJECT_TYPE (context)));
      return;
    }

  GDICT_NOTE (CHOOSER, "Setting new context");
    
  priv->context = g_object_ref (context);
  priv->results = 0;
}

static void
gdict_strategy_chooser_dispose (GObject *gobject)
{
  GdictStrategyChooser *chooser = GDICT_STRATEGY_CHOOSER (gobject);
  GdictStrategyChooserPrivate *priv = chooser->priv;

  set_gdict_context (chooser, NULL);

  if (priv->busy_cursor)
    {
      gdk_cursor_unref (priv->busy_cursor);
      priv->busy_cursor = NULL;
    }

  if (priv->store)
    {
      g_object_unref (priv->store);
      priv->store = NULL;
    }

  G_OBJECT_CLASS (gdict_strategy_chooser_parent_class)->dispose (gobject);
}

static void
gdict_strategy_chooser_finalize (GObject *gobject)
{
  GdictStrategyChooser *chooser = GDICT_STRATEGY_CHOOSER (gobject);
  GdictStrategyChooserPrivate *priv = chooser->priv;

  g_free (priv->current_strat);

  G_OBJECT_CLASS (gdict_strategy_chooser_parent_class)->finalize (gobject);
}

static void
gdict_strategy_chooser_set_property (GObject      *gobject,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
  GdictStrategyChooser *chooser = GDICT_STRATEGY_CHOOSER (gobject);
  
  switch (prop_id)
    {
    case PROP_CONTEXT:
      set_gdict_context (chooser, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
gdict_strategy_chooser_get_property (GObject    *gobject,
				     guint       prop_id,
				     GValue     *value,
				     GParamSpec *pspec)
{
  GdictStrategyChooser *chooser = GDICT_STRATEGY_CHOOSER (gobject);
  
  switch (prop_id)
    {
    case PROP_CONTEXT:
      g_value_set_object (value, chooser->priv->context);
      break;
    case PROP_COUNT:
      g_value_set_int (value, chooser->priv->results);
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
		  gpointer           user_data)
{
  GdictStrategyChooser *chooser = GDICT_STRATEGY_CHOOSER (user_data);
  GdictStrategyChooserPrivate *priv = chooser->priv;
  GtkTreeIter iter;
  gchar *db_name, *db_desc;
  gboolean valid;

  valid = gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->store),
		  		   &iter,
				   path);
  if (!valid)
    {
      g_warning ("Invalid iterator found");
      return;
    }

  gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
		      STRAT_COLUMN_NAME, &db_name,
		      STRAT_COLUMN_DESCRIPTION, &db_desc,
		      -1);
  if (db_name && db_desc)
    {
      g_signal_emit (chooser, db_chooser_signals[STRATEGY_ACTIVATED], 0,
		     db_name, db_desc);
    }
  else
    {
      gchar *row = gtk_tree_path_to_string (path);

      g_warning ("Row %s activated, but no strategy attached", row);
      g_free (row);
    }

  g_free (db_name);
  g_free (db_desc);
}

static void
refresh_button_clicked_cb (GtkWidget *widget,
			   gpointer   user_data)
{
  GdictStrategyChooser *chooser = GDICT_STRATEGY_CHOOSER (user_data);

  gdict_strategy_chooser_refresh (chooser);
}

static void
clear_button_clicked_cb (GtkWidget *widget,
			 gpointer   user_data)
{
  GdictStrategyChooser *chooser = GDICT_STRATEGY_CHOOSER (user_data);

  gdict_strategy_chooser_clear (chooser);
}

static GObject *
gdict_strategy_chooser_constructor (GType                  type,
				    guint                  n_params,
				    GObjectConstructParam *params)
{
  GObject *object;
  GdictStrategyChooser *chooser;
  GdictStrategyChooserPrivate *priv;
  GtkWidget *sw;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *hbox;

  object = G_OBJECT_CLASS (gdict_strategy_chooser_parent_class)->constructor (type,
		  							      n_params,
									      params);

  chooser = GDICT_STRATEGY_CHOOSER (object);
  priv = chooser->priv;

  gtk_widget_push_composite_child ();

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_composite_name (sw, "gdict-strategy-chooser-scrolled-window");
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
		  		  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
		  		       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (chooser), sw, TRUE, TRUE, 0);
  gtk_widget_show (sw);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("strategies",
		  				     renderer,
						     "text", STRAT_COLUMN_DESCRIPTION,
                                                     "weight", STRAT_COLUMN_CURRENT,
						     NULL);
  priv->treeview = gtk_tree_view_new ();
  gtk_widget_set_composite_name (priv->treeview, "gdict-strategy-chooser-treeview");
  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview),
		  	   GTK_TREE_MODEL (priv->store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->treeview), FALSE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->treeview), column);
  g_signal_connect (priv->treeview, "row-activated",
		    G_CALLBACK (row_activated_cb), chooser);
  gtk_container_add (GTK_CONTAINER (sw), priv->treeview);
  gtk_widget_show (priv->treeview);

  hbox = gtk_hbox_new (FALSE, 0);

  priv->refresh_button = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (priv->refresh_button),
		        gtk_image_new_from_stock (GTK_STOCK_REFRESH,
						  GTK_ICON_SIZE_SMALL_TOOLBAR));
  g_signal_connect (priv->refresh_button, "clicked",
		    G_CALLBACK (refresh_button_clicked_cb),
		    chooser);
  gtk_box_pack_start (GTK_BOX (hbox), priv->refresh_button, FALSE, FALSE, 0);
  gtk_widget_show (priv->refresh_button);
  gtk_widget_set_tooltip_text (priv->refresh_button,
                               _("Reload the list of available strategies"));

  priv->clear_button = gtk_button_new ();
  gtk_button_set_image (GTK_BUTTON (priv->clear_button),
		        gtk_image_new_from_stock (GTK_STOCK_CLEAR,
						  GTK_ICON_SIZE_SMALL_TOOLBAR));
  g_signal_connect (priv->clear_button, "clicked",
		    G_CALLBACK (clear_button_clicked_cb),
		    chooser);
  gtk_box_pack_start (GTK_BOX (hbox), priv->clear_button, FALSE, FALSE, 0);
  gtk_widget_show (priv->clear_button);
  gtk_widget_set_tooltip_text (priv->clear_button,
                               _("Clear the list of available strategies"));

  gtk_box_pack_end (GTK_BOX (chooser), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  
  gtk_widget_pop_composite_child ();

  return object;
}

static void
gdict_strategy_chooser_class_init (GdictStrategyChooserClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->finalize = gdict_strategy_chooser_finalize;
  gobject_class->dispose = gdict_strategy_chooser_dispose;
  gobject_class->set_property = gdict_strategy_chooser_set_property;
  gobject_class->get_property = gdict_strategy_chooser_get_property;
  gobject_class->constructor = gdict_strategy_chooser_constructor;
  
  /**
   * GdictStrategyChooser:context:
   *
   * The #GdictContext object used to retrieve the list of strategies.
   */
  g_object_class_install_property (gobject_class,
  				   PROP_CONTEXT,
  				   g_param_spec_object ("context",
  				   			"Context",
  				   			"The GdictContext object used to get the list of strategies",
  				   			GDICT_TYPE_CONTEXT,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT)));

  /**
   * GdictStrategyChooser::strategy-activated:
   * @chooser: the widget that received the signal
   * @name: the name of the activated strategy
   * @description: the description of the activate strategy
   *
   * The ::strategy-activated signal is emitted each time the user
   * activates a strategy in the @chooser, either by double click or
   * using the keyboard.
   */
  db_chooser_signals[STRATEGY_ACTIVATED] =
    g_signal_new ("strategy-activated",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GdictStrategyChooserClass, strategy_activated),
		  NULL, NULL,
		  gdict_marshal_VOID__STRING_STRING,
		  G_TYPE_NONE, 2,
		  G_TYPE_STRING,
		  G_TYPE_STRING);
  
  g_type_class_add_private (gobject_class, sizeof (GdictStrategyChooserPrivate));
}

static void
gdict_strategy_chooser_init (GdictStrategyChooser *chooser)
{
  GdictStrategyChooserPrivate *priv;

  chooser->priv = priv = GDICT_STRATEGY_CHOOSER_GET_PRIVATE (chooser);

  priv->results = -1;
  priv->context = NULL;

  priv->store = gtk_list_store_new (STRAT_N_COLUMNS,
		                    G_TYPE_INT,    /* strat_type */
		                    G_TYPE_STRING, /* strat_name */
				    G_TYPE_STRING, /* strat_desc */
                                    G_TYPE_INT     /* strat_current */);

  priv->start_id = 0;
  priv->end_id = 0;
  priv->match_id = 0;
  priv->error_id = 0;
}

/**
 * gdict_strategy_chooser_new:
 *
 * Creates a new #GdictStrategyChooser. Use this widget to show a list
 * of matching strategies available on a dictionary source represented
 * by a #GdictContext, set with gdict_strategy_chooser_set_context().
 *
 * Return value: the newly created #GdictStrategyChooser widget
 *
 * Since: 0.9
 */
GtkWidget *
gdict_strategy_chooser_new (void)
{
  return g_object_new (GDICT_TYPE_STRATEGY_CHOOSER, NULL);
}

/**
 * gdict_strategy_chooser_new_with_context:
 * @context: a #GdictContext
 *
 * Creates a new #GdictStrategyChooser widget, using @context as the
 * representation of a dictionary source.
 *
 * Return value: the newly created #GdictStrategyChooser widget
 *
 * Since: 0.9
 */
GtkWidget *
gdict_strategy_chooser_new_with_context (GdictContext *context)
{
  g_return_val_if_fail (GDICT_IS_CONTEXT (context), NULL);
  
  return g_object_new (GDICT_TYPE_STRATEGY_CHOOSER,
                       "context", context,
                       NULL);
}

/**
 * gdict_strategy_chooser_get_context:
 * @chooser: a #GdictStrategyChooser
 *
 * Retrieves the #GdictContext used by @chooser.
 *
 * Return value: a #GdictContext
 *
 * Since:
 */
GdictContext *
gdict_strategy_chooser_get_context (GdictStrategyChooser *chooser)
{
  g_return_val_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser), NULL);
  
  return chooser->priv->context;
}

/**
 * gdict_strategy_chooser_set_context:
 * @chooser: a #GdictStrategyChooser
 * @context: a #GdictContext, or %NULL to unset the context
 *
 * Sets the #GdictContext to be used by @chooser to retrieve the
 * list of matching strategies.
 *
 * Since: 0.9
 */
void
gdict_strategy_chooser_set_context (GdictStrategyChooser *chooser,
				    GdictContext         *context)
{
  g_return_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser));
  g_return_if_fail (context == NULL || GDICT_IS_CONTEXT (context));

  set_gdict_context (chooser, context);

  g_object_notify (G_OBJECT (chooser), "context");
}

/**
 * gdict_strategy_chooser_get_strategies:
 * @chooser: a #GdictStrategyChooser
 * @length: return location for the length of the returned string list
 *
 * Retrieves the list of matching strategies available.
 *
 * Return value: a string vector containing the names of the matching
 *   strategies. Use g_strfreev() to deallocate the memory when done
 *
 * Since:0.9
 */
gchar **
gdict_strategy_chooser_get_strategies (GdictStrategyChooser  *chooser,
                                       gsize                 *length)
{
  GdictStrategyChooserPrivate *priv;
  GtkTreeIter iter;
  gchar **retval;
  gsize i;

  g_return_val_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser), NULL);

  priv = chooser->priv;

  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter))
    return NULL;

  i = 0;
  retval = g_new (gchar*, priv->results);

  do
    {
      gchar *strat_name;

      gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
                          STRAT_COLUMN_NAME, &strat_name,
                          -1);

      retval[i++] = strat_name;
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter));

  retval[i] = NULL;

  if (length)
    *length = i;

  return retval;
}

/**
 * gdict_strategy_chooser_has_strategy:
 * @chooser: a #GdictStrategyChooser
 * @strategy: a strategy name
 *
 * Checks whether @strategy is available in the list of matching
 * strategies displayed by @chooser.
 *
 * Return value: %TRUE if the strategy was found, %FALSE otherwise
 *
 * Since: 0.9
 */
gboolean
gdict_strategy_chooser_has_strategy (GdictStrategyChooser *chooser,
				     const gchar          *strategy)
{
  GdictStrategyChooserPrivate *priv;
  GtkTreeIter iter;
  gboolean retval;

  g_return_val_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (strategy != NULL, FALSE);

  priv = chooser->priv;

  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter))
    return FALSE;

  retval = FALSE;

  do
    {
      gchar *strat_name;

      gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
                          STRAT_COLUMN_NAME, &strat_name,
                          -1);
      
      if (strcmp (strat_name, strategy) == 0)
        {
          retval = TRUE;
          g_free (strat_name);
          break;
        }

      g_free (strat_name);
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter));

  return retval;
}

/**
 * gdict_strategy_chooser_count_strategies:
 * @chooser: a #GdictStrategyChooser
 *
 * Returns the number of strategies found.
 *
 * Return value: the number of strategies or -1 if case of error
 *
 * Since:
 */
gint
gdict_strategy_chooser_count_strategies (GdictStrategyChooser *chooser)
{
  g_return_val_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser), -1);

  return chooser->priv->results;
}

static void
lookup_start_cb (GdictContext *context,
		 gpointer      user_data)
{
  GdictStrategyChooser *chooser = GDICT_STRATEGY_CHOOSER (user_data);
  GdictStrategyChooserPrivate *priv = chooser->priv;

  if (!priv->busy_cursor)
    priv->busy_cursor = gdk_cursor_new (GDK_WATCH);

  if (gtk_widget_get_window (GTK_WIDGET (chooser)))
    gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (chooser)), priv->busy_cursor);

  priv->is_searching = TRUE;
}

static void
lookup_end_cb (GdictContext *context,
	       gpointer      user_data)
{
  GdictStrategyChooser *chooser = GDICT_STRATEGY_CHOOSER (user_data);
  GdictStrategyChooserPrivate *priv = chooser->priv;

  if (gtk_widget_get_window (GTK_WIDGET (chooser)))
    gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (chooser)), NULL);

  priv->is_searching = FALSE;
}

static void
strategy_found_cb (GdictContext  *context,
		   GdictStrategy *strategy,
		   gpointer       user_data)
{
  GdictStrategyChooser *chooser = GDICT_STRATEGY_CHOOSER (user_data);
  GdictStrategyChooserPrivate *priv = chooser->priv;
  const gchar *name, *description;
  GtkTreeIter iter;
  gint weight = PANGO_WEIGHT_NORMAL;

  name = gdict_strategy_get_name (strategy);
  description = gdict_strategy_get_description (strategy);

  GDICT_NOTE (CHOOSER, "STRATEGY: `%s' (`%s')",
              name,
              description);

  if (priv->current_strat && !strcmp (priv->current_strat, name))
    weight = PANGO_WEIGHT_BOLD;

  gtk_list_store_append (priv->store, &iter);
  gtk_list_store_set (priv->store, &iter,
		      STRAT_COLUMN_TYPE, STRATEGY_NAME,
		      STRAT_COLUMN_NAME, name,
		      STRAT_COLUMN_DESCRIPTION, description,
                      STRAT_COLUMN_CURRENT, weight,
		      -1);

  priv->results += 1;
}

static void
error_cb (GdictContext *context,
          const GError *error,
	  gpointer      user_data)
{
  GdictStrategyChooser *chooser = GDICT_STRATEGY_CHOOSER (user_data);

  if (gtk_widget_get_window (GTK_WIDGET (chooser)))
    gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (chooser)), NULL);

  chooser->priv->is_searching = FALSE;
  chooser->priv->results = 0;
}

/**
 * gdict_strategy_chooser_refresh:
 * @chooser: a #GdictStrategyChooser
 *
 * Reloads the list of available strategies.
 *
 * Since: 0.10
 */
void
gdict_strategy_chooser_refresh (GdictStrategyChooser *chooser)
{
  GdictStrategyChooserPrivate *priv;
  GError *db_error;
  
  g_return_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser));

  priv = chooser->priv;

  if (!priv->context)
    {
      g_warning ("Attempting to retrieve the available strategies, but "
		 "no GdictContext has been set.  Use gdict_strategy_chooser_set_context() "
		 "before invoking gdict_strategy_chooser_refresh().");
      return;
    }

  if (priv->is_searching)
    return;

  gdict_strategy_chooser_clear (chooser);

  if (!priv->start_id)
    {
      priv->start_id = g_signal_connect (priv->context, "lookup-start",
		      			 G_CALLBACK (lookup_start_cb),
					 chooser);
      priv->match_id = g_signal_connect (priv->context, "strategy-found",
		      			 G_CALLBACK (strategy_found_cb),
					 chooser);
      priv->end_id = g_signal_connect (priv->context, "lookup-end",
		      		       G_CALLBACK (lookup_end_cb),
				       chooser);
    }

  if (!priv->error_id)
    priv->error_id = g_signal_connect (priv->context, "error",
		    		       G_CALLBACK (error_cb),
				       chooser);

  db_error = NULL;
  gdict_context_lookup_strategies (priv->context, &db_error);
  if (db_error)
    {
      GtkTreeIter iter;

      gtk_list_store_append (priv->store, &iter);
      gtk_list_store_set (priv->store, &iter,
		      	  STRAT_COLUMN_TYPE, STRATEGY_ERROR,
			  STRAT_COLUMN_NAME, _("Error while matching"),
			  STRAT_COLUMN_DESCRIPTION, NULL,
			  -1);

      g_warning ("Error while retrieving strategies: %s",
                 db_error->message);

      g_error_free (db_error);
    }
}

/**
 * gdict_strategy_chooser_clear:
 * @chooser: a #GdictStrategyChooser
 *
 * Clears @chooser.
 *
 * Since: 0.10
 */
void
gdict_strategy_chooser_clear (GdictStrategyChooser *chooser)
{
  GdictStrategyChooserPrivate *priv;

  g_return_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser));

  priv = chooser->priv;

  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview), NULL);

  gtk_list_store_clear (priv->store);
  priv->results = 0;

  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview),
		  	   GTK_TREE_MODEL (priv->store));
}

typedef struct
{
  gchar *strat_name;
  GdictStrategyChooser *chooser;
  
  guint found       : 1;
  guint do_select   : 1;
  guint do_activate : 1;
} SelectData;

static gboolean
scan_for_strat_name (GtkTreeModel *model,
                  GtkTreePath  *path,
                  GtkTreeIter  *iter,
                  gpointer      user_data)
{
  SelectData *select_data = user_data;
  gchar *strat_name = NULL;

  if (!select_data)
    return TRUE;

  gtk_tree_model_get (model, iter, STRAT_COLUMN_NAME, &strat_name, -1);
  if (!strat_name)
    return FALSE;

  if (strcmp (strat_name, select_data->strat_name) == 0)
    {
      GtkTreeView *tree_view;
      GtkTreeSelection *selection;

      select_data->found = TRUE;

      tree_view = GTK_TREE_VIEW (select_data->chooser->priv->treeview);
      if (select_data->do_activate)
        {
          GtkTreeViewColumn *column;

          gtk_list_store_set (GTK_LIST_STORE (model), iter,
                              STRAT_COLUMN_CURRENT, PANGO_WEIGHT_BOLD,
                              -1);

          column = gtk_tree_view_get_column (tree_view, 2);
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
                          STRAT_COLUMN_CURRENT, PANGO_WEIGHT_NORMAL,
                          -1);
    }

  g_free (strat_name);

  return FALSE;
}

/**
 * gdict_strategy_chooser_select_strategy:
 * @chooser: a #GdictStrategyChooser
 * @strat_name: the name of the strategy to select
 *
 * Selects @strat_name, if available.
 *
 * Return value: %TRUE if the matching strategy was found and selected
 *
 * Since: 0.10
 */
gboolean
gdict_strategy_chooser_select_strategy (GdictStrategyChooser *chooser,
                                        const gchar          *strat_name)
{
  GdictStrategyChooserPrivate *priv;
  SelectData data;
  gboolean retval;

  g_return_val_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (strat_name != NULL, FALSE);

  priv = chooser->priv;

  data.strat_name = g_strdup (strat_name);
  data.chooser = chooser;
  data.found = FALSE;
  data.do_select = TRUE;
  data.do_activate = FALSE;

  gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                          scan_for_strat_name,
                          &data);

  retval = data.found;

  g_free (data.strat_name);

  return retval;
}

/**
 * gdict_strategy_chooser_unselect_strategy:
 * @chooser: a #GdictStrategyChooser
 * @strat_name: the name of the strategy to unselect
 *
 * Unselects @strat_name from the list.
 *
 * Return value: %TRUE if the matching strategy was found and successfully
 *   unselected
 *
 * Since: 0.10
 */
gboolean
gdict_strategy_chooser_unselect_strategy (GdictStrategyChooser *chooser,
                                          const gchar          *strat_name)
{
  GdictStrategyChooserPrivate *priv;
  SelectData data;
  gboolean retval;

  g_return_val_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (strat_name != NULL, FALSE);

  priv = chooser->priv;

  data.strat_name = g_strdup (strat_name);
  data.chooser = chooser;
  data.found = FALSE;
  data.do_select = FALSE;
  data.do_activate = FALSE;

  gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                          scan_for_strat_name,
                          &data);

  retval = data.found;

  g_free (data.strat_name);

  return retval;
}

/**
 * gdict_strategy_chooser_set_current_strategy:
 * @chooser: a #GdictStrategyChooser
 * @strat_name: the name of the matching strategy
 *
 * Sets @strat_name as the current matching strategy.
 *
 * Return value: %TRUE if the matching strategy was found
 *
 * Since: 0.10
 */
gboolean
gdict_strategy_chooser_set_current_strategy (GdictStrategyChooser *chooser,
                                             const gchar          *strat_name)
{
  GdictStrategyChooserPrivate *priv;
  SelectData data;
  gboolean retval;

  g_return_val_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (strat_name != NULL, FALSE);

  priv = chooser->priv;

  if (priv->current_strat && !strcmp (priv->current_strat, strat_name))
    return TRUE;

  data.strat_name = g_strdup (strat_name);
  data.chooser = chooser;
  data.found = FALSE;
  data.do_select = TRUE;
  data.do_activate = TRUE;

  gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                          scan_for_strat_name,
                          &data);

  retval = data.found;

  if (data.found)
    {
      g_free (priv->current_strat);
      priv->current_strat = data.strat_name;
    }
  else
    g_free (data.strat_name);

  return retval;
}

/**
 * gdict_strategy_chooser_get_current_strategy:
 * @chooser: a #GdictStrategyChooser
 *
 * Retrieves the current matching strategy.
 *
 * Return value: a newly allocated string containing the name of
 *   the current matching strategy
 *
 * Since: 0.10
 */
gchar *
gdict_strategy_chooser_get_current_strategy (GdictStrategyChooser *chooser)
{
  GdictStrategyChooserPrivate *priv;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *retval = NULL;

  g_return_val_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser), NULL);
  
  priv = chooser->priv;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return NULL;

  gtk_tree_model_get (model, &iter, STRAT_COLUMN_NAME, &retval, -1);
  
  g_free (priv->current_strat);
  priv->current_strat = g_strdup (retval);

  return retval;
}

/**
 * gdict_strategy_chooser_add_button:
 * @chooser: a #GdictStrategyChooser
 * @button_text: text of the button (can be a stock id)
 *
 * Creates a new button and packs it into the #GdictStrategyChooser
 * "action area".
 *
 * Return value: the packed #GtkButton
 *
 * Since: 0.10
 */
GtkWidget *
gdict_strategy_chooser_add_button (GdictStrategyChooser *chooser,
                                   const gchar          *button_text)
{
  GdictStrategyChooserPrivate *priv;
  GtkWidget *button;

  g_return_val_if_fail (GDICT_IS_STRATEGY_CHOOSER (chooser), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);

  priv = chooser->priv;

  button = gtk_button_new_from_stock (button_text);

  gtk_widget_set_can_default (button, TRUE);

  gtk_widget_show (button);

  gtk_box_pack_end (GTK_BOX (priv->buttons_box), button, FALSE, TRUE, 0);

  return button;
}
