/* gdict-source-dialog.c - source dialog
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
#include <time.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gi18n.h>
#include <mateconf/mateconf-client.h>

#include "gdict-source-dialog.h"
#include "gdict-common.h"

#define GDICT_SOURCE_UI 	PKGDATADIR "/mate-dictionary-source.ui"

/*********************
 * GdictSourceDialog *
 *********************/

struct _GdictSourceDialog
{
  GtkDialog parent_instance;

  GtkBuilder *builder; 

  MateConfClient *mateconf_client;
  guint notify_id;
  
  GdictSourceLoader *loader;
  GdictSource *source;
  gchar *source_name;
  GdictContext *context;
  
  GdictSourceDialogAction action;
  
  GdictSourceTransport transport;

  GtkWidget *add_button;
  GtkWidget *close_button;
  GtkWidget *cancel_button;
  GtkWidget *help_button;

  GtkWidget *db_chooser;
  GtkWidget *strat_chooser;
  
  GtkWidget *transport_combo;
};

struct _GdictSourceDialogClass
{
  GtkDialogClass parent_class;
};

enum
{
  PROP_0,
  
  PROP_SOURCE_LOADER,
  PROP_SOURCE_NAME,
  PROP_ACTION
};

G_DEFINE_TYPE (GdictSourceDialog, gdict_source_dialog, GTK_TYPE_DIALOG);

static void
set_source_loader (GdictSourceDialog *dialog,
		   GdictSourceLoader *loader)
{
  if (dialog->loader)
    g_object_unref (dialog->loader);
  
  dialog->loader = g_object_ref (loader);
}

static void
transport_combo_changed_cb (GtkWidget *widget,
			    gpointer   user_data)
{
  GdictSourceDialog *dialog = GDICT_SOURCE_DIALOG (user_data);
  gint transport;

  transport = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
  if (transport == dialog->transport)
    return;

  if (transport == GDICT_SOURCE_TRANSPORT_DICTD)
    {
      gtk_widget_show (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "hostname_label")));
      gtk_widget_show (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "hostname_entry")));
      gtk_widget_show (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "port_label")));
      gtk_widget_show (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "port_entry")));
      
      if (dialog->action == GDICT_SOURCE_DIALOG_CREATE)
        {
          gtk_widget_set_sensitive (dialog->add_button, TRUE);
          
          dialog->transport = GDICT_SOURCE_TRANSPORT_DICTD;
        }
    }
  else
    {
      gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "hostname_label")));
      gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "hostname_entry")));
      gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "port_label")));
      gtk_widget_hide (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "port_entry")));

      if (dialog->action == GDICT_SOURCE_DIALOG_CREATE)
        {
          gtk_widget_set_sensitive (dialog->add_button, FALSE);
          
          dialog->transport = GDICT_SOURCE_TRANSPORT_INVALID;
        }
    }
}

static gchar *
get_text_from_entry (GdictSourceDialog *dialog,
		     const gchar       *entry_name)
{
  GtkWidget *entry;
  gchar *retval;

  entry = GTK_WIDGET (gtk_builder_get_object (dialog->builder, entry_name));
  if (!entry)
    return NULL;
  
  retval = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);
  
  return retval;
}

static void
set_text_to_entry (GdictSourceDialog *dialog,
		   const gchar       *entry_name,
		   const gchar       *text)
{
  GtkWidget *entry;

  entry = GTK_WIDGET (gtk_builder_get_object (dialog->builder, entry_name));
  if (!entry)
    return;

  gtk_entry_set_text (GTK_ENTRY (entry), text);
}

static void
set_transport_settings (GdictSourceDialog *dialog)
{
  switch (dialog->transport)
    {
    case GDICT_SOURCE_TRANSPORT_DICTD:
      {
        GdictClientContext *context;
        const gchar *hostname;
        gchar *port_str;
        guint port;

        context = GDICT_CLIENT_CONTEXT (dialog->context);
        hostname = gdict_client_context_get_hostname (context);
        port = gdict_client_context_get_port (context);
        port_str = g_strdup_printf ("%d", port);

        set_text_to_entry (dialog, "hostname_entry", hostname);
        set_text_to_entry (dialog, "port_entry", port_str);

        gtk_widget_show (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "hostname_label")));
        gtk_widget_show (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "hostname_entry")));
        gtk_widget_show (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "port_label")));
        gtk_widget_show (GTK_WIDGET (gtk_builder_get_object (dialog->builder, "port_entry")));

        g_free (port_str);
      }
      break;
    case GDICT_SOURCE_TRANSPORT_INVALID:
    default:
      break;
    }
}

static void
update_dialog_ui (GdictSourceDialog *dialog)
{
  GdictSource *source;
  
  /* TODO - add code to update the contents of the dialog depending
   * on the action; if we are in _CREATE, no action is needed
   */
  switch (dialog->action)
    {
    case GDICT_SOURCE_DIALOG_VIEW:
    case GDICT_SOURCE_DIALOG_EDIT:
      if (!dialog->source_name)
	{
          g_warning ("Attempting to retrieve source, but no "
		     "source name has been defined.  Aborting...");
	  return;
	}
      
      source = gdict_source_loader_get_source (dialog->loader,
		      			       dialog->source_name);
      if (!source)
	{
          g_warning ("Attempting to retrieve source, but no "
		     "source named `%s' was found.  Aborting...",
		     dialog->source_name);
	  return;
	}
      
      g_object_ref (source);
      
      dialog->source = source;
      set_text_to_entry (dialog, "description_entry",
		         gdict_source_get_description (source));

      dialog->transport = gdict_source_get_transport (source);
      gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->transport_combo),
                                (gint) dialog->transport);

      /* set the context for the database and strategy choosers */
      dialog->context = gdict_source_get_context (source);
      if (!dialog->context)
        {
          g_warning ("Attempting to retrieve the context, but "
                     "none was found for source `%s'.",
                     dialog->source_name);
          return;
        }
      
      set_transport_settings (dialog);

      gdict_database_chooser_set_context (GDICT_DATABASE_CHOOSER (dialog->db_chooser),
                                          dialog->context);
      gdict_database_chooser_refresh (GDICT_DATABASE_CHOOSER (dialog->db_chooser));
      gdict_strategy_chooser_set_context (GDICT_STRATEGY_CHOOSER (dialog->strat_chooser),
                                          dialog->context);
      gdict_strategy_chooser_refresh (GDICT_STRATEGY_CHOOSER (dialog->strat_chooser));
      break;
    case GDICT_SOURCE_DIALOG_CREATE:
      /* DICTD transport is default */
      gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->transport_combo), 0);
      g_signal_emit_by_name (dialog->transport_combo, "changed");
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

static void
build_new_source (GdictSourceDialog *dialog)
{
  GdictSource *source;
  gchar *name, *text;
  GdictSourceTransport transport;
  gchar *host, *port;
  gchar *data;
  gsize length;
  GError *error;
  gchar *filename;
  GdictDatabaseChooser *db_chooser;
  GdictStrategyChooser *strat_chooser;
  
  source = gdict_source_new ();
      
  /* use the timestamp and the pid to get a unique name */
  name = g_strdup_printf ("source-%lu-%u",
                          (gulong) time (NULL),
                          (guint) getpid ());
  gdict_source_set_name (source, name);
  g_free (name);
      
  text = get_text_from_entry (dialog, "description_entry");
  gdict_source_set_description (source, text);
  g_free (text);

  db_chooser = GDICT_DATABASE_CHOOSER (dialog->db_chooser);
  text = gdict_database_chooser_get_current_database (db_chooser);
  gdict_source_set_database (source, text);
  g_free (text);

  strat_chooser = GDICT_STRATEGY_CHOOSER (dialog->strat_chooser);
  text = gdict_strategy_chooser_get_current_strategy (strat_chooser);
  gdict_source_set_strategy (source, text);
  g_free (text);

  /* get the selected transport id */
  transport = dialog->transport;
  switch (transport)
    {
    case GDICT_SOURCE_TRANSPORT_DICTD:
      host = get_text_from_entry (dialog, "hostname_entry");
      port = get_text_from_entry (dialog, "port_entry");
       
      gdict_source_set_transport (source, GDICT_SOURCE_TRANSPORT_DICTD,
          			  "hostname", host,
          			  "port", atoi (port),
          			  NULL);
          
      g_free (host);
      g_free (port);
      break;
    case GDICT_SOURCE_TRANSPORT_INVALID:
    default:
      g_warning ("Invalid transport");
      return;
    }
      
  error = NULL;
  data = gdict_source_to_data (source, &length, &error);
  if (error)
    {
      gdict_show_gerror_dialog (GTK_WINDOW (dialog),
				_("Unable to create a source file"),
				error);
       
      g_object_unref (source);
      return;
    }
      
  name = g_strdup_printf ("%s.desktop", gdict_source_get_name (source));
  filename = g_build_filename (g_get_home_dir (),
  			       ".mate2",
      			       "mate-dictionary",
      			       name,
      			       NULL);
  g_free (name);
      
  g_file_set_contents (filename, data, length, &error);
  if (error)
    gdict_show_gerror_dialog (GTK_WINDOW (dialog),
       			      _("Unable to save source file"),
       			      error);

  g_free (filename);
  g_free (data);
  g_object_unref (source);
}

static void
save_source (GdictSourceDialog *dialog)
{
  GdictSource *source;
  GdictDatabaseChooser *db_chooser;
  GdictStrategyChooser *strat_chooser;
  gchar *name, *text;
  GdictSourceTransport transport;
  gchar *host, *port;
  gchar *data;
  gsize length;
  GError *error;
  gchar *filename;
  
  source = gdict_source_loader_get_source (dialog->loader,
		  			   dialog->source_name);
  if (!source)
    {
      g_warning ("Attempting to save source `%s', but no "
		 "source for that name was found.",
		 dialog->source_name);

      return;
    }
      
  text = get_text_from_entry (dialog, "description_entry");
  gdict_source_set_description (source, text);
  g_free (text);

  db_chooser = GDICT_DATABASE_CHOOSER (dialog->db_chooser);
  text = gdict_database_chooser_get_current_database (db_chooser);
  gdict_source_set_database (source, text);
  g_free (text);

  strat_chooser = GDICT_STRATEGY_CHOOSER (dialog->strat_chooser);
  text = gdict_strategy_chooser_get_current_strategy (strat_chooser);
  gdict_source_set_strategy (source, text);
  g_free (text);


  /* get the selected transport id */
  transport = dialog->transport;
  switch (transport)
    {
    case GDICT_SOURCE_TRANSPORT_DICTD:
      host = get_text_from_entry (dialog, "hostname_entry");
      port = get_text_from_entry (dialog, "port_entry");
       
      gdict_source_set_transport (source, GDICT_SOURCE_TRANSPORT_DICTD,
          			  "hostname", host,
          			  "port", atoi (port),
          			  NULL);
          
      g_free (host);
      g_free (port);
      break;
    case GDICT_SOURCE_TRANSPORT_INVALID:
    default:
      g_warning ("Invalid transport");
      return;
    }
      
  error = NULL;
  data = gdict_source_to_data (source, &length, &error);
  if (error)
    {
      gdict_show_gerror_dialog (GTK_WINDOW (dialog),
			 	_("Unable to create a source file"),
			 	error);
      
      g_object_unref (source);
      return;
    }
      
  name = g_strdup_printf ("%s.desktop", gdict_source_get_name (source));
  filename = g_build_filename (g_get_home_dir (),
      			       ".mate2",
			       "mate-dictionary",
			       name,
			       NULL);
  g_free (name);
      
  g_file_set_contents (filename, data, length, &error);
  if (error)
    gdict_show_gerror_dialog (GTK_WINDOW (dialog),
       			      _("Unable to save source file"),
       			      error);

  g_free (filename);
  g_free (data);
  g_object_unref (source);
}

static void
gdict_source_dialog_response_cb (GtkDialog *dialog,
				 gint       response_id,
				 gpointer   user_data)
{
  GError *err = NULL;
  
  switch (response_id)
    {
    case GTK_RESPONSE_ACCEPT:
      build_new_source (GDICT_SOURCE_DIALOG (dialog));
      break;
    case GTK_RESPONSE_HELP:
      gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)),
                    "ghelp:mate-dictionary#mate-dictionary-add-source",
                    gtk_get_current_event_time (), &err);
      if (err)
        {
          gdict_show_gerror_dialog (GTK_WINDOW (dialog),
          			    _("There was an error while displaying help"),
          		 	    err);
          g_error_free (err);
        }

      /* we don't want the dialog to close itself */
      g_signal_stop_emission_by_name (dialog, "response");
      break;
    case GTK_RESPONSE_CLOSE:
      save_source (GDICT_SOURCE_DIALOG (dialog));
      break;
    case GTK_RESPONSE_CANCEL:
      break;
    default:
      break;
    }
}

static void
gdict_source_dialog_finalize (GObject *object)
{
  GdictSourceDialog *dialog = GDICT_SOURCE_DIALOG (object);

  if (dialog->mateconf_client)
    g_object_unref (dialog->mateconf_client);
  
  if (dialog->builder)
    g_object_unref (dialog->builder);

  if (dialog->source_name)
    g_free (dialog->source_name);

  if (dialog->source)
    g_object_unref (dialog->source);
  
  if (dialog->loader)
    g_object_unref (dialog->loader);
  
  G_OBJECT_CLASS (gdict_source_dialog_parent_class)->finalize (object);
}

static void
gdict_source_dialog_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
  GdictSourceDialog *dialog = GDICT_SOURCE_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_SOURCE_LOADER:
      set_source_loader (dialog, g_value_get_object (value));
      break;
    case PROP_SOURCE_NAME:
      g_free (dialog->source_name);
      dialog->source_name = g_strdup (g_value_get_string (value));
      break;
    case PROP_ACTION:
      dialog->action = (GdictSourceDialogAction) g_value_get_int (value);
      break;
    default:
      break;
    }
}

static void
gdict_source_dialog_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
  GdictSourceDialog *dialog = GDICT_SOURCE_DIALOG (object);
  
  switch (prop_id)
    {
    case PROP_SOURCE_LOADER:
      g_value_set_object (value, dialog->loader);
      break;
    case PROP_SOURCE_NAME:
      g_value_set_string (value, dialog->source_name);
      break;
    case PROP_ACTION:
      g_value_set_int (value, dialog->action);
      break;
    default:
      break;
    }
}

static GObject *
gdict_source_dialog_constructor (GType                  type,
				 guint                  n_construct_properties,
				 GObjectConstructParam *construct_params)
{
  GObject *object;
  GdictSourceDialog *dialog;
  GtkWidget *vbox;
  GError *error = NULL;

  object = G_OBJECT_CLASS (gdict_source_dialog_parent_class)->constructor (type,
									   n_construct_properties,
									   construct_params);
  dialog = GDICT_SOURCE_DIALOG (object);

  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), 2);
  
  gtk_widget_push_composite_child ();

  /* get the UI from the GtkBuilder file */
  dialog->builder = gtk_builder_new ();
  gtk_builder_add_from_file (dialog->builder, GDICT_SOURCE_UI, &error);

  if (error) {
    g_critical ("Unable to load the user interface definition file: %s",
                error->message);
    g_error_free (error);
    g_assert_not_reached ();
  }
  
  /* the main widget */
  gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                     GTK_WIDGET (gtk_builder_get_object (dialog->builder, "source_root")));

  /* the transport combo changes the UI by changing the visible widgets
   * bound to the transport's own options.
   */
  dialog->transport_combo = GTK_WIDGET (gtk_builder_get_object (dialog->builder, "transport_combo"));
  g_signal_connect (dialog->transport_combo, "changed",
  		    G_CALLBACK (transport_combo_changed_cb),
  		    dialog);

  /* the help button is always visible */
  dialog->help_button = gtk_dialog_add_button (GTK_DIALOG (dialog),
  					       GTK_STOCK_HELP,
					       GTK_RESPONSE_HELP);
  
  vbox = GTK_WIDGET (gtk_builder_get_object (dialog->builder, "db-vbox"));
  dialog->db_chooser = gdict_database_chooser_new ();
  gtk_box_pack_start (GTK_BOX (vbox), dialog->db_chooser, TRUE, TRUE, 0);
  gtk_widget_show (dialog->db_chooser);

  vbox = GTK_WIDGET (gtk_builder_get_object (dialog->builder, "strat-vbox"));
  dialog->strat_chooser = gdict_strategy_chooser_new ();
  gtk_box_pack_start (GTK_BOX (vbox), dialog->strat_chooser, TRUE, TRUE, 0);
  gtk_widget_show (dialog->strat_chooser);

  /* the UI changes depending on the action that the source dialog
   * should perform
   */
  switch (dialog->action)
    {
    case GDICT_SOURCE_DIALOG_VIEW:
      /* disable every editable widget */
      gtk_editable_set_editable (GTK_EDITABLE (gtk_builder_get_object (dialog->builder, "name_entry")), FALSE);
      gtk_editable_set_editable (GTK_EDITABLE (gtk_builder_get_object (dialog->builder, "description_entry")), FALSE);
      gtk_editable_set_editable (GTK_EDITABLE (gtk_builder_get_object (dialog->builder, "hostname_entry")), FALSE);
      gtk_editable_set_editable (GTK_EDITABLE (gtk_builder_get_object (dialog->builder, "port_entry")), FALSE);
      
      gtk_widget_set_sensitive (dialog->transport_combo, FALSE);

      /* we just allow closing the dialog */
      dialog->close_button  = gtk_dialog_add_button (GTK_DIALOG (dialog),
      						     GTK_STOCK_CLOSE,
      						     GTK_RESPONSE_CLOSE);
      break;
    case GDICT_SOURCE_DIALOG_CREATE:
      dialog->cancel_button = gtk_dialog_add_button (GTK_DIALOG (dialog),
      						     GTK_STOCK_CANCEL,
      						     GTK_RESPONSE_CANCEL);
      dialog->add_button    = gtk_dialog_add_button (GTK_DIALOG (dialog),
      						     GTK_STOCK_ADD,
      						     GTK_RESPONSE_ACCEPT);
      /* the "add" button sensitivity is controlled by the transport_combo
       * since it's the only setting that makes a source usable.
       */
      gtk_widget_set_sensitive (dialog->add_button, FALSE);
      break;
    case GDICT_SOURCE_DIALOG_EDIT:
      dialog->cancel_button = gtk_dialog_add_button (GTK_DIALOG (dialog),
      						     GTK_STOCK_CANCEL,
      						     GTK_RESPONSE_CANCEL);
      dialog->close_button  = gtk_dialog_add_button (GTK_DIALOG (dialog),
		      	 			     GTK_STOCK_CLOSE,
						     GTK_RESPONSE_CLOSE);
      break;
    default:
      g_assert_not_reached ();
      break;
    }
  
  /* this will take care of updating the contents of the dialog
   * based on the action
   */
  update_dialog_ui (dialog);
  
  gtk_widget_pop_composite_child ();
  
  return object;
}

static void
gdict_source_dialog_class_init (GdictSourceDialogClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  
  gobject_class->constructor = gdict_source_dialog_constructor;
  gobject_class->set_property = gdict_source_dialog_set_property;
  gobject_class->get_property = gdict_source_dialog_get_property;
  gobject_class->finalize = gdict_source_dialog_finalize;
  
  g_object_class_install_property (gobject_class,
  				   PROP_SOURCE_LOADER,
  				   g_param_spec_object ("source-loader",
  				   			"Source Loader",
  				   			"The GdictSourceLoader used by the application",
  				   			GDICT_TYPE_SOURCE_LOADER,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
  g_object_class_install_property (gobject_class,
  				   PROP_SOURCE_NAME,
  				   g_param_spec_string ("source-name",
  				   			"Source Name",
  				   			"The source name",
  				   			NULL,
  				   			(G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT)));
  g_object_class_install_property (gobject_class,
  				   PROP_ACTION,
  				   g_param_spec_int ("action",
  				   		     "Action",
  				   		     "The action the source dialog should perform",
  				   		     -1,
  				   		     GDICT_SOURCE_DIALOG_EDIT,
  				   		     GDICT_SOURCE_DIALOG_VIEW,
  				   		     (G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY)));
}

static void
gdict_source_dialog_init (GdictSourceDialog *dialog)
{
  gtk_widget_set_size_request (GTK_WIDGET (dialog), 400, 300);
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  
  gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

  dialog->transport = GDICT_SOURCE_TRANSPORT_INVALID;

  g_signal_connect (dialog, "response",
  		    G_CALLBACK (gdict_source_dialog_response_cb),
  		    NULL);
}

GtkWidget *
gdict_source_dialog_new (GtkWindow               *parent,
			 const gchar             *title,
			 GdictSourceDialogAction  action,
			 GdictSourceLoader       *loader,
			 const gchar             *source_name)
{
  GtkWidget *retval;
  
  g_return_val_if_fail ((parent == NULL || GTK_IS_WINDOW (parent)), NULL);
  g_return_val_if_fail (GDICT_IS_SOURCE_LOADER (loader), NULL);
  
  retval = g_object_new (GDICT_TYPE_SOURCE_DIALOG,
  			 "source-loader", loader,
  			 "source-name", source_name,
  			 "action", action,
  			 "title", title,
  			 NULL);

  if (parent)
    {
      gtk_window_set_transient_for (GTK_WINDOW (retval), parent);
      gtk_window_set_destroy_with_parent (GTK_WINDOW (retval), TRUE);
      gtk_window_set_screen (GTK_WINDOW (retval),
                             gtk_widget_get_screen (GTK_WIDGET (parent)));
    }
  
  return retval;
}
