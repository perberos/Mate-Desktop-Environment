/* gdict-database-chooser.c - display widget for database names
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
 * SECTION:gdict-database-chooser
 * @short_description: Display the list of available databases
 *
 * Each #GdictContext has a list of databases, that is dictionaries that
 * can be queried. #GdictDatabaseChooser is a widget that queries a given
 * #GdictContext and displays the list of available databases.
 *
 * #GdictDatabaseChooser is available since Gdict 0.10
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include "gdict-database-chooser.h"
#include "gdict-utils.h"
#include "gdict-debug.h"
#include "gdict-private.h"
#include "gdict-enum-types.h"
#include "gdict-marshal.h"

#define GDICT_DATABASE_CHOOSER_GET_PRIVATE(obj) \
(G_TYPE_INSTANCE_GET_PRIVATE ((obj), GDICT_TYPE_DATABASE_CHOOSER, GdictDatabaseChooserPrivate))

struct _GdictDatabaseChooserPrivate
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

  gchar *current_db;

  guint is_searching : 1;
};

enum
{
  DATABASE_NAME,
  DATABASE_ERROR
} DBType;

enum
{
  DB_COLUMN_TYPE,
  DB_COLUMN_NAME,
  DB_COLUMN_DESCRIPTION,
  DB_COLUMN_CURRENT,

  DB_N_COLUMNS
};

enum
{
  PROP_0,
  
  PROP_CONTEXT,
  PROP_COUNT
};

enum
{
  DATABASE_ACTIVATED,
  SELECTION_CHANGED,

  LAST_SIGNAL
};

static guint db_chooser_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GdictDatabaseChooser,
               gdict_database_chooser,
               GTK_TYPE_VBOX);


static void
set_gdict_context (GdictDatabaseChooser *chooser,
		   GdictContext         *context)
{
  GdictDatabaseChooserPrivate *priv;
  
  g_assert (GDICT_IS_DATABASE_CHOOSER (chooser));
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
gdict_database_chooser_finalize (GObject *gobject)
{
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (gobject);
  GdictDatabaseChooserPrivate *priv = chooser->priv;

  g_free (priv->current_db);
  
  G_OBJECT_CLASS (gdict_database_chooser_parent_class)->finalize (gobject);
}

static void
gdict_database_chooser_dispose (GObject *gobject)
{
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (gobject);
  GdictDatabaseChooserPrivate *priv = chooser->priv;

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

  G_OBJECT_CLASS (gdict_database_chooser_parent_class)->dispose (gobject);
}

static void
gdict_database_chooser_set_property (GObject      *gobject,
				     guint         prop_id,
				     const GValue *value,
				     GParamSpec   *pspec)
{
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (gobject);
  
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
gdict_database_chooser_get_property (GObject    *gobject,
				     guint       prop_id,
				     GValue     *value,
				     GParamSpec *pspec)
{
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (gobject);
  
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
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (user_data);
  GdictDatabaseChooserPrivate *priv = chooser->priv;
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
		      DB_COLUMN_NAME, &db_name,
		      DB_COLUMN_DESCRIPTION, &db_desc,
		      -1);
  if (db_name && db_desc)
    {
      g_free (priv->current_db);
      priv->current_db = g_strdup (db_name);

      g_signal_emit (chooser, db_chooser_signals[DATABASE_ACTIVATED], 0,
		     db_name, db_desc);
    }
  else
    {
      gchar *row = gtk_tree_path_to_string (path);

      g_warning ("Row %s activated, but no database attached", row);
      g_free (row);
    }

  g_free (db_name);
  g_free (db_desc);
}

static void
refresh_button_clicked_cb (GtkWidget *widget,
			   gpointer   user_data)
{
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (user_data);

  gdict_database_chooser_refresh (chooser);
}

static void
clear_button_clicked_cb (GtkWidget *widget,
			 gpointer   user_data)
{
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (user_data);

  gdict_database_chooser_clear (chooser);
}

static void
selection_changed_cb (GtkTreeSelection *selection,
                      gpointer          user_data)
{
  g_signal_emit (user_data, db_chooser_signals[SELECTION_CHANGED], 0);
}

static GObject *
gdict_database_chooser_constructor (GType                  type,
				    guint                  n_params,
				    GObjectConstructParam *params)
{
  GObjectClass *parent_class;
  GObject *object;
  GdictDatabaseChooser *chooser;
  GdictDatabaseChooserPrivate *priv;
  GtkWidget *sw;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkWidget *hbox;

  parent_class = G_OBJECT_CLASS (gdict_database_chooser_parent_class);
  object = parent_class->constructor (type, n_params, params);

  chooser = GDICT_DATABASE_CHOOSER (object);
  priv = chooser->priv;

  gtk_widget_push_composite_child ();

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_composite_name (sw, "gdict-database-chooser-scrolled-window");
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
		  		  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
		  		       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (chooser), sw, TRUE, TRUE, 0);
  gtk_widget_show (sw);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("databases",
		  				     renderer,
						     "text", DB_COLUMN_DESCRIPTION,
                                                     "weight", DB_COLUMN_CURRENT,
						     NULL);
  priv->treeview = gtk_tree_view_new ();
  gtk_widget_set_composite_name (priv->treeview, "gdict-database-chooser-treeview");
  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview),
		  	   GTK_TREE_MODEL (priv->store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->treeview), FALSE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->treeview), column);
  g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview)),
                    "changed", G_CALLBACK (selection_changed_cb),
                    chooser);
  g_signal_connect (priv->treeview, "row-activated",
		    G_CALLBACK (row_activated_cb), chooser);
  gtk_container_add (GTK_CONTAINER (sw), priv->treeview);
  gtk_widget_show (priv->treeview);

  hbox = gtk_hbox_new (FALSE, 0);
  priv->buttons_box = hbox;

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
                               _("Reload the list of available databases"));

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
                               _("Clear the list of available databases"));

  gtk_box_pack_end (GTK_BOX (chooser), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  
  gtk_widget_pop_composite_child ();

  return object;
}

static void
gdict_database_chooser_class_init (GdictDatabaseChooserClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->finalize = gdict_database_chooser_finalize;
  gobject_class->dispose = gdict_database_chooser_dispose;
  gobject_class->set_property = gdict_database_chooser_set_property;
  gobject_class->get_property = gdict_database_chooser_get_property;
  gobject_class->constructor = gdict_database_chooser_constructor;

  /**
   * GdictDatabaseChooser:context:
   *
   * The #GdictContext used to retrieve the list of available databases.
   *
   * Since: 0.10
   */
  g_object_class_install_property (gobject_class,
  				   PROP_CONTEXT,
  				   g_param_spec_object ("context",
  				   			"Context",
  				   			"The GdictContext object used to get the list of databases",
  				   			GDICT_TYPE_CONTEXT,
  				   			(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));
  /**
   * GdictDatabaseChooser:count:
   *
   * The number of displayed databases or, if no #GdictContext is set, -1.
   *
   * Since: 0.12
   */
  g_object_class_install_property (gobject_class,
                                   PROP_COUNT,
                                   g_param_spec_int ("count",
                                                     "Count",
                                                     "The number of available databases",
                                                     -1, G_MAXINT, -1,
                                                     G_PARAM_READABLE));

  /**
   * GdictDatabaseChooser::database-activated:
   * @chooser: the database chooser that received the signal
   * @name: the name of the activated database
   * @description: the description of the activated database
   *
   * The ::database-activated signal is emitted each time the user
   * activated a row in the database chooser widget, either by double
   * clicking on it or by a keyboard event.
   *
   * Since: 0.10
   */
  db_chooser_signals[DATABASE_ACTIVATED] =
    g_signal_new ("database-activated",
		  G_OBJECT_CLASS_TYPE (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GdictDatabaseChooserClass, database_activated),
		  NULL, NULL,
		  gdict_marshal_VOID__STRING_STRING,
		  G_TYPE_NONE, 2,
		  G_TYPE_STRING,
		  G_TYPE_STRING);
  /**
   * GdictDatabaseChooser::selection-changed:
   * @chooser: the database chooser that received the signal
   *
   * The ::selection-changed signal is emitted each time the selection
   * inside the database chooser has been changed.
   *
   * Since: 0.12
   */
  db_chooser_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GdictDatabaseChooserClass, selection_changed),
                  NULL, NULL,
                  gdict_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  g_type_class_add_private (gobject_class, sizeof (GdictDatabaseChooserPrivate));
}

static void
gdict_database_chooser_init (GdictDatabaseChooser *chooser)
{
  GdictDatabaseChooserPrivate *priv;

  chooser->priv = priv = GDICT_DATABASE_CHOOSER_GET_PRIVATE (chooser);

  priv->results = -1;
  priv->context = NULL;

  priv->store = gtk_list_store_new (DB_N_COLUMNS,
		                    G_TYPE_INT,    /* db_type */
		                    G_TYPE_STRING, /* db_name */
				    G_TYPE_STRING, /* db_desc */
                                    G_TYPE_INT     /* db_current */);

  priv->start_id = 0;
  priv->end_id = 0;
  priv->match_id = 0;
  priv->error_id = 0;
}

/**
 * gdict_database_chooser_new:
 *
 * Creates a new #GdictDatabaseChooser widget. A Database chooser widget
 * can be used to display the list of available databases on a dictionary
 * source using the #GdictContext representing it. After creation, the
 * #GdictContext can be set using gdict_database_chooser_set_context().
 *
 * Return value: the newly created #GdictDatabaseChooser widget.
 *
 * Since: 0.10
 */
GtkWidget *
gdict_database_chooser_new (void)
{
  return g_object_new (GDICT_TYPE_DATABASE_CHOOSER, NULL);
}

/**
 * gdict_database_chooser_new_with_context:
 * @context: a #GdictContext
 *
 * Creates a new #GdictDatabaseChooser, using @context as the representation
 * of the dictionary source to query for the list of available databases.
 *
 * Return value: the newly created #GdictDatabaseChooser widget.
 *
 * Since: 0.10
 */
GtkWidget *
gdict_database_chooser_new_with_context (GdictContext *context)
{
  g_return_val_if_fail (GDICT_IS_CONTEXT (context), NULL);
  
  return g_object_new (GDICT_TYPE_DATABASE_CHOOSER,
                       "context", context,
                       NULL);
}

/**
 * gdict_database_chooser_get_context:
 * @chooser: a #GdictDatabaseChooser
 *
 * Retrieves the #GdictContext used by @chooser.
 *
 * Return value: a #GdictContext or %NULL
 *
 * Since: 0.10
 */
GdictContext *
gdict_database_chooser_get_context (GdictDatabaseChooser *chooser)
{
  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser), NULL);
  
  return chooser->priv->context;
}

/**
 * gdict_database_chooser_set_context:
 * @chooser: a #GdictDatabaseChooser
 * @context: a #GdictContext
 *
 * Sets the #GdictContext to be used to query a dictionary source
 * for the list of available databases.
 *
 * Since: 0.10
 */
void
gdict_database_chooser_set_context (GdictDatabaseChooser *chooser,
				    GdictContext         *context)
{
  g_return_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser));
  g_return_if_fail (context == NULL || GDICT_IS_CONTEXT (context));

  set_gdict_context (chooser, context);

  g_object_notify (G_OBJECT (chooser), "context");
}

/**
 * gdict_database_chooser_get_databases:
 * @chooser: a #GdictDatabaseChooser
 * @length: return location for the length of the returned vector
 *
 * Gets the list of available database names.
 *
 * Return value: a newly allocated, %NULL terminated string vector
 *   containing database names. Use g_strfreev() to deallocate it.
 *
 * Since: 0.10
 */
gchar **
gdict_database_chooser_get_databases (GdictDatabaseChooser  *chooser,
				      gsize                 *length)
{
  GdictDatabaseChooserPrivate *priv;
  GtkTreeIter iter;
  gchar **retval;
  gsize i;

  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser), NULL);

  priv = chooser->priv;

  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter))
    return NULL;

  i = 0;
  retval = g_new (gchar*, priv->results);

  do
    {
      gchar *db_name;

      gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
                          DB_COLUMN_NAME, &db_name,
                          -1);

      retval[i++] = db_name;
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter));

  retval[i] = NULL;

  if (length)
    *length = i;

  return retval;
}

/**
 * gdict_database_chooser_has_database:
 * @chooser: a #GdictDatabaseChooser
 * @database: the name of a database
 *
 * Checks whether the @chooser displays @database
 *
 * Return value: %TRUE if the search database name is present
 *
 * Since: 0.10
 */
gboolean
gdict_database_chooser_has_database (GdictDatabaseChooser *chooser,
				     const gchar          *database)
{
  GdictDatabaseChooserPrivate *priv;
  GtkTreeIter iter;
  gboolean retval;

  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (database != NULL, FALSE);

  priv = chooser->priv;

  if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter))
    return FALSE;

  retval = FALSE;

  do
    {
      gchar *db_name;

      gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
                          DB_COLUMN_NAME, &db_name,
                          -1);
      
      if (strcmp (db_name, database) == 0)
        {
          g_free (db_name);
          retval = TRUE;
          break;
        }
      
      g_free (db_name);
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter));

  return retval;
}

/**
 * gdict_database_chooser_count_databases:
 * @chooser: a #GdictDatabaseChooser
 *
 * Returns the number of databases found.
 *
 * Return value: the number of databases or -1 if no context is set
 *
 * Since: 0.10
 */
gint
gdict_database_chooser_count_databases (GdictDatabaseChooser *chooser)
{
  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser), -1);

  return chooser->priv->results;
}

static void
lookup_start_cb (GdictContext *context,
		 gpointer      user_data)
{
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (user_data);
  GdictDatabaseChooserPrivate *priv = chooser->priv;

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
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (user_data);
  GdictDatabaseChooserPrivate *priv = chooser->priv;

  if (gtk_widget_get_window (GTK_WIDGET (chooser)))
    gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (chooser)), NULL);

  priv->is_searching = FALSE;
}

static void
database_found_cb (GdictContext  *context,
		   GdictDatabase *database,
		   gpointer       user_data)
{
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (user_data);
  GdictDatabaseChooserPrivate *priv = chooser->priv;
  GtkTreeIter iter;
  const gchar *name, *full_name;
  gint weight = PANGO_WEIGHT_NORMAL;

  name = gdict_database_get_name (database);
  full_name = gdict_database_get_full_name (database);

  if (priv->current_db && !strcmp (priv->current_db, name))
    weight = PANGO_WEIGHT_BOLD;

  GDICT_NOTE (CHOOSER, "DATABASE: `%s' (`%s')",
              name,
              full_name);
  
  gtk_list_store_append (priv->store, &iter);
  gtk_list_store_set (priv->store, &iter,
		      DB_COLUMN_TYPE, DATABASE_NAME,
		      DB_COLUMN_NAME, name,
		      DB_COLUMN_DESCRIPTION, full_name,
                      DB_COLUMN_CURRENT, weight,
		      -1);

  priv->results += 1;
}

static void
error_cb (GdictContext *context,
          const GError *error,
	  gpointer      user_data)
{
  GdictDatabaseChooser *chooser = GDICT_DATABASE_CHOOSER (user_data);

  if (gtk_widget_get_window (GTK_WIDGET (chooser)))
    gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (chooser)), NULL);

  chooser->priv->is_searching = FALSE;
  chooser->priv->results = 0;
}

/**
 * gdict_database_chooser_refresh:
 * @chooser: a #GdictDatabaseChooser
 *
 * Reloads the list of available databases.
 *
 * Since: 0.10
 */
void
gdict_database_chooser_refresh (GdictDatabaseChooser *chooser)
{
  GdictDatabaseChooserPrivate *priv;
  GError *db_error;
  
  g_return_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser));

  priv = chooser->priv;

  if (!priv->context)
    {
      g_warning ("Attempting to retrieve the available databases, but "
		 "no GdictContext has been set.  Use gdict_database_chooser_set_context() "
		 "before invoking gdict_database_chooser_refresh().");
      return;
    }

  if (priv->is_searching)
    return;

  gdict_database_chooser_clear (chooser);

  if (!priv->start_id)
    {
      priv->start_id = g_signal_connect (priv->context, "lookup-start",
		      			 G_CALLBACK (lookup_start_cb),
					 chooser);
      priv->match_id = g_signal_connect (priv->context, "database-found",
		      			 G_CALLBACK (database_found_cb),
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
  gdict_context_lookup_databases (priv->context, &db_error);
  if (db_error)
    {
      GtkTreeIter iter;

      gtk_list_store_append (priv->store, &iter);
      gtk_list_store_set (priv->store, &iter,
		      	  DB_COLUMN_TYPE, DATABASE_ERROR,
			  DB_COLUMN_NAME, _("Error while matching"),
			  DB_COLUMN_DESCRIPTION, NULL,
			  -1);

      g_warning ("Error while looking for databases: %s",
                 db_error->message);

      g_error_free (db_error);
    }
}

/**
 * gdict_database_chooser_clear:
 * @chooser: a #GdictDatabaseChooser
 *
 * Clears @chooser.
 *
 * Since: 0.10
 */
void
gdict_database_chooser_clear (GdictDatabaseChooser *chooser)
{
  GdictDatabaseChooserPrivate *priv;

  g_return_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser));

  priv = chooser->priv;

  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview), NULL);

  gtk_list_store_clear (priv->store);
  priv->results = 0;

  gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview),
		  	   GTK_TREE_MODEL (priv->store));
}

typedef struct
{
  gchar *db_name;
  GdictDatabaseChooser *chooser;
  
  guint found       : 1;
  guint do_select   : 1;
  guint do_activate : 1;
} SelectData;

static gboolean
scan_for_db_name (GtkTreeModel *model,
                  GtkTreePath  *path,
                  GtkTreeIter  *iter,
                  gpointer      user_data)
{
  SelectData *select_data = user_data;
  gchar *db_name = NULL;

  if (!select_data)
    return TRUE;

  gtk_tree_model_get (model, iter, DB_COLUMN_NAME, &db_name, -1);
  if (!db_name)
    return FALSE;

  if (strcmp (db_name, select_data->db_name) == 0)
    {
      GtkTreeView *tree_view;
      GtkTreeSelection *selection;

      select_data->found = TRUE;

      tree_view = GTK_TREE_VIEW (select_data->chooser->priv->treeview);
      if (select_data->do_activate)
        {
          GtkTreeViewColumn *column;

          gtk_list_store_set (GTK_LIST_STORE (model), iter,
                              DB_COLUMN_CURRENT, PANGO_WEIGHT_BOLD,
                              -1);

          column = gtk_tree_view_get_column (tree_view, 0);
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
                          DB_COLUMN_CURRENT, PANGO_WEIGHT_NORMAL,
                          -1);
    }

  g_free (db_name);

  return FALSE;
}

/**
 * gdict_database_chooser_select_database:
 * @chooser: a #GdictDatabaseChooser
 * @db_name: name of the database to select
 *
 * Selects the database with @db_name inside the @chooser widget.
 *
 * Return value: %TRUE if the database was found and selected
 *
 * Since: 0.10
 */
gboolean
gdict_database_chooser_select_database (GdictDatabaseChooser *chooser,
                                        const gchar          *db_name)
{
  GdictDatabaseChooserPrivate *priv;
  SelectData data;
  gboolean retval;

  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (db_name != NULL, FALSE);

  priv = chooser->priv;

  data.db_name = g_strdup (db_name);
  data.chooser = chooser;
  data.found = FALSE;
  data.do_select = TRUE;
  data.do_activate = FALSE;

  gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                          scan_for_db_name,
                          &data);

  retval = data.found;

  g_free (data.db_name);

  return retval;
}

/**
 * gdict_database_chooser_unselect_database:
 * @chooser: a #GdictDatabaseChooser
 * @db_name: name of the database to unselect
 *
 * Unselects the database @db_name inside the @chooser widget
 *
 * Return value: %TRUE if the database was found and unselected
 *
 * Since: 0.10
 */
gboolean
gdict_database_chooser_unselect_database (GdictDatabaseChooser *chooser,
                                          const gchar          *db_name)
{
  GdictDatabaseChooserPrivate *priv;
  SelectData data;
  gboolean retval;

  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (db_name != NULL, FALSE);

  priv = chooser->priv;

  data.db_name = g_strdup (db_name);
  data.chooser = chooser;
  data.found = FALSE;
  data.do_select = FALSE;
  data.do_activate = FALSE;

  gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                          scan_for_db_name,
                          &data);

  retval = data.found;

  g_free (data.db_name);

  return retval;
}

/**
 * gdict_database_chooser_set_current_database:
 * @chooser: a #GdictDatabaseChooser
 * @db_name: the name of the database
 *
 * Sets @db_name as the current database. This function will select
 * and activate the corresponding row, if the database is found.
 *
 * Return value: %TRUE if the database was found and set
 *
 * Since: 0.10
 */
gboolean
gdict_database_chooser_set_current_database (GdictDatabaseChooser *chooser,
                                             const gchar          *db_name)
{
  GdictDatabaseChooserPrivate *priv;
  SelectData data;
  gboolean retval;

  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser), FALSE);
  g_return_val_if_fail (db_name != NULL, FALSE);

  priv = chooser->priv;

  data.db_name = g_strdup (db_name);
  data.chooser = chooser;
  data.found = FALSE;
  data.do_select = TRUE;
  data.do_activate = TRUE;

  gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                          scan_for_db_name,
                          &data);

  retval = data.found;

  if (data.found)
    {
      g_free (priv->current_db);
      priv->current_db = data.db_name;
    }
  else
    g_free (data.db_name);

  return retval;
}

/**
 * gdict_database_chooser_get_current_database:
 * @chooser: a #GdictDatabaseChooser
 *
 * Retrieves the name of the currently selected database inside @chooser
 *
 * Return value: the name of the selected database. Use g_free() on the
 *   returned string when done using it
 *
 * Since: 0.10
 */
gchar *
gdict_database_chooser_get_current_database (GdictDatabaseChooser *chooser)
{
  GdictDatabaseChooserPrivate *priv;
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gchar *retval = NULL;

  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser), NULL);
  
  priv = chooser->priv;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return NULL;

  gtk_tree_model_get (model, &iter, DB_COLUMN_NAME, &retval, -1);
  
  g_free (priv->current_db);
  priv->current_db = g_strdup (retval);

  return retval;
}

/**
 * gdict_database_chooser_add_button:
 * @chooser: a #GdictDatabase
 * @button_text: text of the button
 *
 * Adds a #GtkButton with @button_text to the button area on
 * the bottom of @chooser. The @button_text can also be a
 * stock ID.
 *
 * Return value: the newly packed button.
 *
 * Since: 0.10
 */
GtkWidget *
gdict_database_chooser_add_button (GdictDatabaseChooser *chooser,
                                   const gchar          *button_text)
{
  GdictDatabaseChooserPrivate *priv;
  GtkWidget *button;

  g_return_val_if_fail (GDICT_IS_DATABASE_CHOOSER (chooser), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);

  priv = chooser->priv;

  button = gtk_button_new_from_stock (button_text);

  gtk_widget_set_can_default (button, TRUE);

  gtk_widget_show (button);

  gtk_box_pack_end (GTK_BOX (priv->buttons_box), button, FALSE, TRUE, 0);

  return button;
}
