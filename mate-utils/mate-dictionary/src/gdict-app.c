/* gdict-app.c - main application class
 *
 * This file is part of MATE Dictionary
 *
 * Copyright (C) 2005 Emmanuele Bassi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>

#include "gdict-common.h"
#include "gdict-pref-dialog.h"
#include "gdict-app.h"

static GdictApp *singleton = NULL;


struct _GdictAppClass
{
  GObjectClass parent_class;
};



G_DEFINE_TYPE (GdictApp, gdict_app, G_TYPE_OBJECT);


static void
gdict_app_finalize (GObject *object)
{
  GdictApp *app = GDICT_APP (object);
  
  if (app->loader)
    g_object_unref (app->loader);
  
  app->current_window = NULL;
  
  g_slist_foreach (app->windows,
  		   (GFunc) gtk_widget_destroy,
  		   NULL);
  g_slist_free (app->windows);

  g_slist_foreach (app->lookup_words, (GFunc) g_free, NULL);
  g_slist_free (app->lookup_words);

  g_slist_foreach (app->match_words, (GFunc) g_free, NULL);
  g_slist_free (app->match_words);

  g_free (app->database);
  g_free (app->source_name);

  G_OBJECT_CLASS (gdict_app_parent_class)->finalize (object);
}

static void
gdict_app_class_init (GdictAppClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->finalize = gdict_app_finalize;
}

static void
gdict_app_init (GdictApp *app)
{
  app->windows = NULL;
  app->current_window = NULL;
}

static void
gdict_window_destroy_cb (GtkWidget *widget,
		         gpointer   user_data)
{
  GdictWindow *window = GDICT_WINDOW (widget);
  GdictApp *app = GDICT_APP (user_data);
  
  g_assert (GDICT_IS_APP (app));

  app->windows = g_slist_remove (app->windows, window);
  
  if (window == app->current_window)
    app->current_window = app->windows ? app->windows->data : NULL;
  
  if (app->windows == NULL)
    gtk_main_quit ();  
}

static void
gdict_window_created_cb (GdictWindow *parent,
			 GdictWindow *new_window,
			 gpointer     user_data)
{
  GdictApp *app = GDICT_APP (user_data);
  
  /* this might seem convoluted - but it's necessary, since I don't want
   * GdictWindow to know about the GdictApp singleton.  every time a new
   * window is created by a GdictWindow, it will register its "child window"
   * here; the lifetime handlers will check every child window created and
   * destroyed, and will add/remove it to the windows list accordingly
   */
  g_signal_connect (new_window, "created",
  		    G_CALLBACK (gdict_window_created_cb), app);
  g_signal_connect (new_window, "destroy",
  		    G_CALLBACK (gdict_window_destroy_cb), app);

  if (gtk_window_get_group (GTK_WINDOW (parent)))
    gtk_window_group_add_window (gtk_window_get_group (GTK_WINDOW (parent)),
		    		 GTK_WINDOW (new_window));
  
  app->windows = g_slist_prepend (app->windows, new_window);
  app->current_window = new_window;
}

static void
gdict_create_window (GdictApp *app)
{
  GSList *l;

  if (!singleton->lookup_words && !singleton->match_words)
    {
      GtkWidget *window;

      window = gdict_window_new (GDICT_WINDOW_ACTION_CLEAR,
      				 singleton->loader,
				 singleton->source_name,
				 NULL);
      g_signal_connect (window, "created",
		        G_CALLBACK (gdict_window_created_cb), app);
      g_signal_connect (window, "destroy",
		        G_CALLBACK (gdict_window_destroy_cb), app);
  
      app->windows = g_slist_prepend (app->windows, window);
      app->current_window = GDICT_WINDOW (window);
  
      gtk_widget_show (window);

      return;
    }
      
  for (l = singleton->lookup_words; l != NULL; l = l->next)
    {
      gchar *word = l->data;
      GtkWidget *window;

      window = gdict_window_new (GDICT_WINDOW_ACTION_LOOKUP,
      				 singleton->loader,
		      		 singleton->source_name,
				 word);
      
      g_signal_connect (window, "created",
		        G_CALLBACK (gdict_window_created_cb), app);
      g_signal_connect (window, "destroy",
		        G_CALLBACK (gdict_window_destroy_cb), app);
  
      app->windows = g_slist_prepend (app->windows, window);
      app->current_window = GDICT_WINDOW (window);
  
      gtk_widget_show (window);
    }

  for (l = singleton->match_words; l != NULL; l = l->next)
    {
      gchar *word = l->data;
      GtkWidget *window;

      window = gdict_window_new (GDICT_WINDOW_ACTION_MATCH,
      				 singleton->loader,
				 singleton->source_name,
				 word);
      
      g_signal_connect (window, "created",
      			G_CALLBACK (gdict_window_created_cb), app);
      g_signal_connect (window, "destroy",
      			G_CALLBACK (gdict_window_destroy_cb), app);
      
      app->windows = g_slist_prepend (app->windows, window);
      app->current_window = GDICT_WINDOW (window);

      gtk_widget_show (window);
    }
}

static void
definition_found_cb (GdictContext    *context,
		     GdictDefinition *definition,
		     gpointer         user_data)
{
  /* Translators: the first is the word found, the second is the
   * database name and the last is the definition's text; please
   * keep the new lines. */
  g_print (_("Definition for '%s'\n"
	     "  From '%s':\n"
	     "\n"
	     "%s\n"),
           gdict_definition_get_word (definition),
	   gdict_definition_get_database (definition),
	   gdict_definition_get_text (definition));
}

static void
error_cb (GdictContext *context,
          const GError *error,
	  gpointer      user_data)
{
  g_print (_("Error: %s\n"), error->message);

  gtk_main_quit ();
}

static void
lookup_end_cb (GdictContext *context,
	       gpointer      user_data)
{
  GdictApp *app = GDICT_APP (user_data);

  app->remaining_words -= 1;

  if (app->remaining_words == 0)
    gtk_main_quit ();
}

static void
gdict_look_up_word_and_quit (GdictApp *app)
{
  GdictSource *source;
  GdictContext *context;
  GSList *l;
  
  if ((!app->lookup_words) || (!app->match_words))
    {
      g_print (_("See mate-dictionary --help for usage\n"));

      gdict_cleanup ();
      exit (1);
    }

  if (app->source_name)
    source = gdict_source_loader_get_source (app->loader, app->source_name);
  else
    source = gdict_source_loader_get_source (app->loader, GDICT_DEFAULT_SOURCE_NAME);

  if (!source)
    {
      g_warning (_("Unable to find a suitable dictionary source"));

      gdict_cleanup ();
      exit (1);
    }

  /* we'll just use this one context, so we can destroy it along with
   * the source that contains it
   */
  context = gdict_source_peek_context (source);
  g_assert (GDICT_IS_CONTEXT (context));

  g_signal_connect (context, "definition-found",
		    G_CALLBACK (definition_found_cb), app);
  g_signal_connect (context, "error",
		    G_CALLBACK (error_cb), app);
  g_signal_connect (context, "lookup-end",
		    G_CALLBACK (lookup_end_cb), app);

  app->remaining_words = 0;
  for (l = app->lookup_words; l != NULL; l = l->next)
    {
      gchar *word = l->data;
      GError *err = NULL;

      app->remaining_words += 1;

      gdict_context_define_word (context,
		      		 app->database,
				 word,
				 &err);

      if (err)
	{
          g_warning (_("Error while looking up the definition of \"%s\":\n%s"),
		     word,
		     err->message);

	  g_error_free (err);
	}
    }

  gtk_main ();

  g_object_unref (source);

  gdict_cleanup ();
  exit (0);
}

void
gdict_init (int *argc, char ***argv)
{
  GError *mateconf_error, *err = NULL;
  GOptionContext *context;
  gchar *loader_path;
  gchar **lookup_words = NULL;
  gchar **match_words = NULL;
  gchar *database = NULL;
  gchar *strategy = NULL;
  gchar *source_name = NULL;
  gboolean no_window = FALSE;
  gboolean list_sources = FALSE;

  const GOptionEntry gdict_app_goptions[] =
  {
    { "look-up", 0, 0, G_OPTION_ARG_STRING_ARRAY, &lookup_words,
       N_("Words to look up"), N_("word") },
    { "match", 0, 0, G_OPTION_ARG_STRING_ARRAY, &match_words,
       N_("Words to match"), N_("word") },
    { "source", 's', 0, G_OPTION_ARG_STRING, &source_name,
       N_("Dictionary source to use"), N_("source") },
    { "list-sources", 'l', 0, G_OPTION_ARG_NONE, &list_sources,
       N_("Show available dictionary sources"), NULL },
    { "no-window", 'n', 0, G_OPTION_ARG_NONE, &no_window,
       N_("Print result to the console"), NULL },
    { "database", 'D', 0, G_OPTION_ARG_STRING, &database,
       N_("Database to use"), N_("db") },
    { "strategy", 'S', 0, G_OPTION_ARG_STRING, &strategy,
       N_("Strategy to use"), N_("strat") },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &lookup_words,
       N_("Words to look up"), N_("word") },
    { NULL },
  };
  
  /* we must have GLib's type system up and running in order to create the
   * singleton object for mate-dictionary; thus, we can't rely on
   * mate_program_init() calling g_type_init() for us.
   */
  g_type_init ();

  g_assert (singleton == NULL);  
  
  singleton = GDICT_APP (g_object_new (GDICT_TYPE_APP, NULL));
  g_assert (GDICT_IS_APP (singleton));
  
  /* create the new option context */
  context = g_option_context_new (_(" - Look up words in dictionaries"));
  
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_add_main_entries (context, gdict_app_goptions, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gdict_get_option_group ());
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  g_option_context_parse (context, argc, argv, &err);
  if (err)
    {
      g_critical ("Failed to parse argument: %s", err->message);
      g_error_free (err);
      g_option_context_free (context);
      gdict_cleanup ();

      exit (1);
    }
  
  g_set_application_name (_("Dictionary"));
  gtk_window_set_default_icon_name ("accessories-dictionary");
  
  if (!gdict_create_data_dir ())
    {
      gdict_cleanup ();

      exit (1);
    }
  
  mateconf_error = NULL;
  singleton->mateconf_client = mateconf_client_get_default ();
  mateconf_client_add_dir (singleton->mateconf_client,
  			GDICT_MATECONF_DIR,
  			MATECONF_CLIENT_PRELOAD_ONELEVEL,
  			&mateconf_error);
  if (mateconf_error)
    {
      g_warning ("Unable to access MateConf: %s\n", mateconf_error->message);
      
      g_error_free (mateconf_error);
      g_object_unref (singleton->mateconf_client);
    }

  /* add user's path for fetching dictionary sources */  
  singleton->loader = gdict_source_loader_new ();
  loader_path = gdict_get_data_dir (); 
  gdict_source_loader_add_search_path (singleton->loader, loader_path);
  g_free (loader_path);

  if (lookup_words)
    {
      gsize i;
      gsize length = g_strv_length (lookup_words);

      for (i = 0; i < length; i++)
	singleton->lookup_words = g_slist_prepend (singleton->lookup_words,
			                           g_strdup (lookup_words[i]));
    }

  if (match_words)
    {
      gsize i;
      gsize length = g_strv_length (match_words);

      for (i = 0; i < length; i++)
        singleton->match_words = g_slist_prepend (singleton->match_words,
			                          g_strdup (match_words[i]));
    }

  if (database)
    singleton->database = g_strdup (database);
  
  if (source_name)
    singleton->source_name = g_strdup (source_name);

  if (no_window)
    singleton->no_window = TRUE;

  if (list_sources)
    singleton->list_sources = TRUE;
}

void
gdict_main (void)
{
  if (!singleton)
    {
      g_warning ("You must initialize GdictApp using gdict_init()\n");
      return;
    }
  
  if (!singleton->no_window)
    gdict_create_window (singleton);
  else
    gdict_look_up_word_and_quit (singleton);
  
  gtk_main ();
}

void
gdict_cleanup (void)
{
  if (!singleton)
    {
      g_warning ("You must initialize GdictApp using gdict_init()\n");
      return;
    }

  g_object_unref (singleton);
}
