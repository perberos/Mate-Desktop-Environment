/*
 * matecomponent-file-selector-util.c - functions for getting files from a
 * selector
 *
 * Authors:
 *    Jacob Berkman  <jacob@ximian.com>
 *    Paolo Maggi  <maggi@athena.polito.it>
 *
 * Copyright 2001-2002 Ximian, Inc.
 *
 */

#include <config.h>

#include "matecomponent-file-selector-util.h"

#include <string.h>

#include <matecomponent/matecomponent-event-source.h>
#include <matecomponent/matecomponent-exception.h>
#include <matecomponent/matecomponent-widget.h>

#include <gtk/gtk.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#define GET_MODE(w) (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "MateFileSelectorMode")))
#define SET_MODE(w, m) (g_object_set_data (G_OBJECT (w), "MateFileSelectorMode", GINT_TO_POINTER (m)))

typedef enum {
	FILESEL_OPEN,
	FILESEL_OPEN_MULTI,
	FILESEL_SAVE
} FileselMode;

static GQuark user_data_id = 0;

static gint
delete_file_selector (GtkWidget *d, GdkEventAny *e, gpointer data)
{
	gtk_widget_hide (d);
	gtk_main_quit ();
	return TRUE;
}

static void
listener_cb (MateComponentListener *listener, 
	     const gchar *event_name,
	     const CORBA_any *any,
	     CORBA_Environment *ev,
	     gpointer data)
{
	GtkWidget *dialog;
	CORBA_sequence_CORBA_string *seq;
	char *subtype;

	dialog = data;
	gtk_widget_hide (dialog);

	subtype = matecomponent_event_subtype (event_name);
	if (!strcmp (subtype, "Cancel"))
		goto cancel_clicked;

	seq = any->_value;
	if (seq->_length < 1)
		goto cancel_clicked;

	if (GET_MODE (dialog) == FILESEL_OPEN_MULTI) {
		char **strv;
		int i;

		if (seq->_length == 0)
			goto cancel_clicked;

		strv = g_new (char *, seq->_length + 1);
		for (i = 0; i < seq->_length; i++)
			strv[i] = g_strdup (seq->_buffer[i]);
		strv[i] = NULL;
		g_object_set_qdata (G_OBJECT (dialog),
				    user_data_id, strv);
	} else 
		g_object_set_qdata (G_OBJECT (dialog),
				    user_data_id,
				    g_strdup (seq->_buffer[0]));

 cancel_clicked:
	g_free (subtype);
	gtk_main_quit ();
}

static MateComponentWidget *
create_control (gboolean enable_vfs, FileselMode mode)
{
	CORBA_Environment ev;
	MateComponentWidget *bw;
	char *moniker;

	moniker = g_strdup_printf (
		"OAFIID:MATE_FileSelector_Control!"
		"Application=%s;"
		"EnableVFS=%d;"
		"MultipleSelection=%d;"
		"SaveMode=%d",
		g_get_prgname (),
		enable_vfs,
		mode == FILESEL_OPEN_MULTI, 
		mode == FILESEL_SAVE);

	bw = g_object_new (MATECOMPONENT_TYPE_WIDGET, NULL);
	CORBA_exception_init (&ev);

	bw = matecomponent_widget_construct_control (
		bw, moniker, CORBA_OBJECT_NIL, &ev);

	CORBA_exception_free (&ev);
	g_free (moniker);

	return bw;
}

static GtkWindow *
create_matecomponent_selector (gboolean    enable_vfs,
			FileselMode mode,
			const char *mime_types,
			const char *default_path, 
			const char *default_filename)

{
	GtkWidget    *dialog;
	MateComponentWidget *control;

	control = create_control (enable_vfs, mode);
	if (!control)
		return NULL;

	dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_add (GTK_CONTAINER (dialog), GTK_WIDGET (control));
	gtk_window_set_default_size (GTK_WINDOW (dialog), 560, 450);

	matecomponent_event_source_client_add_listener (
		matecomponent_widget_get_objref (control), 
		listener_cb, 
		"MATE/FileSelector/Control:ButtonClicked",
		NULL, dialog);

	if (mime_types)
		matecomponent_widget_set_property (
			control, "MimeTypes", TC_CORBA_string, mime_types, NULL);

	if (default_path)
		matecomponent_widget_set_property (
			control, "DefaultLocation", TC_CORBA_string, default_path, NULL);

	if (default_filename)
		matecomponent_widget_set_property (
			control, "DefaultFileName", TC_CORBA_string, default_filename, NULL);
	
	return GTK_WINDOW (dialog);
}

static void
response_cb (GtkFileChooser *chooser, gint response, gpointer data)
{
	gchar *file_name;

	if (response != GTK_RESPONSE_OK) {
		gtk_widget_hide (GTK_WIDGET (chooser));
		gtk_main_quit ();

		g_object_set_qdata (G_OBJECT (chooser),
				    user_data_id,
				    NULL);
		return;
	}

	file_name = gtk_file_chooser_get_filename (chooser);

	if (file_name == NULL || !strlen (file_name)) {
		g_free (file_name);
		return;
	}
	
	if (GET_MODE (chooser) == FILESEL_OPEN_MULTI) {
		gchar **strv;
		GSList *files = gtk_file_chooser_get_filenames (chooser);
		GSList *iter;
		int i;

		strv = g_new (gchar *, (g_slist_length (files) + 1));

		for (iter = files, i = 0; iter != NULL; iter = iter->next, i++)
			strv[i] = iter->data;
		strv[i] = NULL;
		g_slist_free (files);

		g_object_set_qdata (G_OBJECT (chooser),
				    user_data_id, strv);
	} else
		g_object_set_qdata (G_OBJECT (chooser),
				    user_data_id,
				    g_strdup (file_name));

	gtk_widget_hide (GTK_WIDGET (chooser));
	gtk_main_quit ();

	g_free (file_name);
}

static GtkWindow *
create_gtk_selector (FileselMode mode,
		     const char *default_path,
		     const char *default_filename)
{
	GtkWidget *chooser;
	
	chooser = gtk_file_chooser_dialog_new (NULL,
					       NULL,
					       mode == FILESEL_SAVE ? GTK_FILE_CHOOSER_ACTION_SAVE
					       			     : GTK_FILE_CHOOSER_ACTION_OPEN,
					       GTK_STOCK_CANCEL,
					       GTK_RESPONSE_CANCEL,
					       mode == FILESEL_SAVE ? GTK_STOCK_SAVE
								    : GTK_STOCK_OPEN,
					       GTK_RESPONSE_OK,
					       NULL);
	gtk_window_set_default_size (GTK_WINDOW (chooser), 600, 400);
	gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);

	g_signal_connect (G_OBJECT (chooser),
			  "response", G_CALLBACK (response_cb),
			  NULL);

	if (default_path)
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
						     default_path);

	if (default_filename)
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser),
						   default_filename);

	if (mode == FILESEL_OPEN_MULTI) 
		gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (chooser), TRUE);

	return GTK_WINDOW (chooser);
}

static gpointer
run_file_selector (GtkWindow  *parent,
		   gboolean    enable_vfs,
		   FileselMode mode, 
		   const char *title,
		   const char *mime_types,
		   const char *default_path, 
		   const char *default_filename)

{
	GtkWindow *dialog = NULL;
	gpointer   retval;
	gpointer   data;
	gboolean   using_matecomponent_filesel=FALSE;
	
	if (!user_data_id)
		user_data_id = g_quark_from_static_string ("UserData");

	if (!g_getenv ("MATE_FILESEL_DISABLE_MATECOMPONENT")) {
		using_matecomponent_filesel = TRUE;
		dialog = create_matecomponent_selector (enable_vfs, mode, mime_types, 
						 default_path, default_filename);
	}

	if (!dialog) {
		dialog = create_gtk_selector (mode, default_path, default_filename);
		using_matecomponent_filesel = FALSE;
	}

	SET_MODE (dialog, mode);

	gtk_window_set_title (dialog, title);
	gtk_window_set_modal (dialog, TRUE);

	if (parent)
		gtk_window_set_transient_for (dialog, parent);
	
	g_signal_connect (GTK_OBJECT (dialog), "delete_event",
			    G_CALLBACK (delete_file_selector),
			    dialog);

	gtk_widget_show_all (GTK_WIDGET (dialog));

	gtk_main ();

	data = g_object_get_qdata (G_OBJECT (dialog), user_data_id);

	if (data) {
#ifndef G_OS_WIN32
		if (enable_vfs && !using_matecomponent_filesel &&
				(mode != FILESEL_OPEN_MULTI)) {
	 		retval = g_filename_to_uri (data, NULL, NULL);
 			g_free (data);

		} else if (enable_vfs && !using_matecomponent_filesel &&
				(mode == FILESEL_OPEN_MULTI)) {
			gint i;
			gchar **files = data;

			for (i = 0; files[i]; ++i) {
				gchar *tmp = files[i];
				files[i] = g_filename_to_uri (tmp, NULL, NULL);
				g_free (tmp);
			}

			retval = files;
	 	} else
 			retval = data;
#else
		retval = data;
#endif
	} else
		retval = NULL;

	gtk_widget_destroy (GTK_WIDGET (dialog));

	return retval;
}

/**
 * matecomponent_file_selector_open:
 * @parent: optional window the dialog should be a transient for.
 * @enable_vfs: if FALSE, restrict files to local paths.
 * @title: optional window title to use
 * @mime_types: optional list of mime types to provide filters for.
 *   These are of the form: "HTML Files:text/html|Text Files:text/html,text/plain"
 * @default_path: optional directory to start in
 *
 * Creates and shows a modal open file dialog, waiting for the user to
 * select a file or cancel before returning.
 *
 * Return value: the URI (or plain file path if @enable_vfs is FALSE)
 * of the file selected, or NULL if cancel was pressed.
 **/
char *
matecomponent_file_selector_open (GtkWindow  *parent,
			   gboolean    enable_vfs,
			   const char *title,
			   const char *mime_types,
			   const char *default_path)
{
	return run_file_selector (parent, enable_vfs, FILESEL_OPEN, 
				  title ? title : _("Select a file to open"),
				  mime_types, default_path, NULL);
}

/**
 * matecomponent_file_selector_open_multi:
 * @parent: optional window the dialog should be a transient for
 * @enable_vfs: if FALSE, restrict files to local paths.
 * @title: optional window title to use
 * @mime_types: optional list of mime types to provide filters for.
 *   These are of the form: "HTML Files:text/html|Text Files:text/html,text/plain"
 * @default_path: optional directory to start in
 *
 * Creates and shows a modal open file dialog, waiting for the user to
 * select a file or cancel before returning.
 *
 * Return value: a NULL terminated string array of the selected URIs
 * (or local file paths if @enable_vfs is FALSE), or NULL if cancel
 * was pressed.
 **/
char **
matecomponent_file_selector_open_multi (GtkWindow  *parent,
				gboolean    enable_vfs,
				const char *title,
				const char *mime_types,
				const char *default_path)
{
	return run_file_selector (parent, enable_vfs, FILESEL_OPEN_MULTI,
				  title ? title : _("Select files to open"),
				  mime_types, default_path, NULL);
}

/**
 * matecomponent_file_selector_save:
 * @parent: optional window the dialog should be a transient for
 * @enable_vfs: if FALSE, restrict files to local paths.
 * @title: optional window title to use
 * @mime_types: optional list of mime types to provide filters for.
 *   These are of the form: "HTML Files:text/html|Text Files:text/html,text/plain"
 * @default_path: optional directory to start in
 * @default_filename: optional file name to default to
 *
 * Creates and shows a modal save file dialog, waiting for the user to
 * select a file or cancel before returning.
 *
 * Return value: the URI (or plain file path if @enable_vfs is FALSE)
 * of the file selected, or NULL if cancel was pressed.
 **/
char *
matecomponent_file_selector_save (GtkWindow  *parent,
			   gboolean    enable_vfs,
			   const char *title,
			   const char *mime_types,
			   const char *default_path, 
			   const char *default_filename)
{
	return run_file_selector (parent, enable_vfs, FILESEL_SAVE,
				  title ? title : _("Select a filename to save"),
				  mime_types, default_path, default_filename);
}
