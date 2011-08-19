/*
 * hello.c
 *
 * A hello world application using the MateComponent UI handler
 *
 * Authors:
 *	Michael Meeks    <michael@ximian.com>
 *	Murray Cumming   <murrayc@usa.net>
 *      Havoc Pennington <hp@redhat.com>
 *
 * Copyright (C) 1999 Red Hat, Inc.
 *               2001 Murray Cumming,
 *               2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#undef GTK_DISABLE_DEPRECATED

#include <config.h>
#include <string.h>
#include <stdlib.h>

#include <libmatecomponentui.h>

/* Keep a list of all open application windows */
static GSList *app_list = NULL;

#define HELLO_UI_XML "MateComponent_Sample_Hello.xml"

/*   A single forward prototype - try and keep these
 * to a minumum by ordering your code nicely */
static GtkWidget *hello_new (void);

static void
hello_on_menu_file_new (MateComponentUIComponent *uic,
			gpointer           user_data,
			const gchar       *verbname)
{
	gtk_widget_show_all (hello_new ());
}

static void
show_nothing_dialog(GtkWidget* parent)
{
	GtkWidget* dialog;

	dialog = gtk_message_dialog_new (
		GTK_WINDOW (parent),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		_("This does nothing; it is only a demonstration."));

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
hello_on_menu_file_open (MateComponentUIComponent *uic,
			    gpointer           user_data,
			    const gchar       *verbname)
{
	show_nothing_dialog (GTK_WIDGET (user_data));
}

static void
hello_on_menu_file_save (MateComponentUIComponent *uic,
			    gpointer           user_data,
			    const gchar       *verbname)
{
	show_nothing_dialog (GTK_WIDGET (user_data));
}

static void
hello_on_menu_file_saveas (MateComponentUIComponent *uic,
			      gpointer           user_data,
			      const gchar       *verbname)
{
	show_nothing_dialog (GTK_WIDGET (user_data));
}

static void
hello_on_menu_file_exit (MateComponentUIComponent *uic,
			    gpointer           user_data,
			    const gchar       *verbname)
{
	/* FIXME: quit the mainloop nicely */
	exit (0);
}	

static void
hello_on_menu_file_close (MateComponentUIComponent *uic,
			     gpointer           user_data,
			     const gchar       *verbname)
{
	GtkWidget *app = user_data;

	/* Remove instance: */
	app_list = g_slist_remove (app_list, app);

	gtk_widget_destroy (app);

	if (!app_list)
		hello_on_menu_file_exit(uic, user_data, verbname);
}

static void
hello_on_menu_edit_undo (MateComponentUIComponent *uic,
			    gpointer           user_data,
			    const gchar       *verbname)
{
	show_nothing_dialog (GTK_WIDGET (user_data));
}	

static void
hello_on_menu_edit_redo (MateComponentUIComponent *uic,
			    gpointer           user_data,
			    const gchar       *verbname)
{
	show_nothing_dialog (GTK_WIDGET (user_data));
}	

static void
hello_on_menu_help_about (MateComponentUIComponent *uic,
			     gpointer           user_data,
			     const gchar       *verbname)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (
		GTK_WINDOW (user_data),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
		_("MateComponentUI-Hello."));

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static gchar *
utf8_reverse (const char *string)
{
	int len;
	gchar *result;
	const gchar *p;
	gchar *m, *r, skip;

	len = strlen (string);

	result = g_new (gchar, len + 1);
	r = result + len;
	p = string;
	while (*p)  {
		skip = g_utf8_skip[*(guchar*)p];
		r -= skip;
		for (m = r; skip; skip--)
			*m++ = *p++;
	}
	result[len] = 0;

	return result;
}

static void
hello_on_button_click (GtkWidget* w, gpointer user_data)
{
	gchar    *text;
	GtkLabel *label = GTK_LABEL (user_data);

	text = utf8_reverse (gtk_label_get_text (label));

	gtk_label_set_text (label, text);

	g_free (text);
}

/*
 *   These verb names are standard, see limatecomponentbui/doc/std-ui.xml
 * to find a list of standard verb names.
 *   The menu items are specified in MateComponent_Sample_Hello.xml and
 * given names which map to these verbs here.
 */
static const MateComponentUIVerb hello_verbs [] = {
	MATECOMPONENT_UI_VERB ("FileNew",    hello_on_menu_file_new),
	MATECOMPONENT_UI_VERB ("FileOpen",   hello_on_menu_file_open),
	MATECOMPONENT_UI_VERB ("FileSave",   hello_on_menu_file_save),
	MATECOMPONENT_UI_VERB ("FileSaveAs", hello_on_menu_file_saveas),
	MATECOMPONENT_UI_VERB ("FileClose",  hello_on_menu_file_close),
	MATECOMPONENT_UI_VERB ("FileExit",   hello_on_menu_file_exit),

	MATECOMPONENT_UI_VERB ("EditUndo",   hello_on_menu_edit_undo),
	MATECOMPONENT_UI_VERB ("EditRedo",   hello_on_menu_edit_redo),

	MATECOMPONENT_UI_VERB ("HelpAbout",  hello_on_menu_help_about),

	MATECOMPONENT_UI_VERB_END
};

static MateComponentWindow *
hello_create_main_window (void)
{
	MateComponentWindow      *win;
	MateComponentUIContainer *ui_container;
	MateComponentUIComponent *ui_component;

	win = MATECOMPONENT_WINDOW (matecomponent_window_new (GETTEXT_PACKAGE, _("Mate Hello")));

	/* Create Container: */
	ui_container = matecomponent_window_get_ui_container (win);

	/* This determines where the UI configuration info. will be stored */
	matecomponent_ui_engine_config_set_path (matecomponent_window_get_ui_engine (win),
					  "/hello-app/UIConfig/kvps");

	/* Create a UI component with which to communicate with the window */
	ui_component = matecomponent_ui_component_new_default ();

	/* Associate the MateComponentUIComponent with the container */
	matecomponent_ui_component_set_container (
		ui_component, MATECOMPONENT_OBJREF (ui_container), NULL);

	/* NB. this creates a relative file name from the current dir,
	 * in production you want to pass the application's datadir
	 * see Makefile.am to see how HELLO_SRCDIR gets set. */
	matecomponent_ui_util_set_ui (ui_component, "", /* data dir */
			       HELLO_SRCDIR HELLO_UI_XML,
			       "matecomponent-hello", NULL);

	/* Associate our verb -> callback mapping with the MateComponentWindow */
	/* All the callback's user_data pointers will be set to 'win' */
	matecomponent_ui_component_add_verb_list_with_data (ui_component, hello_verbs, win);

	return win;
}

static gint 
delete_event_cb (GtkWidget *window, GdkEventAny *e, gpointer user_data)
{
	hello_on_menu_file_close (NULL, window, NULL);

	/* Prevent the window's destruction, since we destroyed it 
	 * ourselves with hello_app_close() */
	return TRUE;
}

static GtkWidget *
hello_new (void)
{
	GtkWidget    *label;
	GtkWidget    *frame;
	GtkWidget    *button;
	MateComponentWindow *win;

	win = hello_create_main_window();

	/* Create Button: */
	button = gtk_button_new ();
	gtk_container_set_border_width (GTK_CONTAINER (button), 10);

	/* Create Label and put it in the Button: */
	label = gtk_label_new (_("Hello, World!"));
	gtk_container_add (GTK_CONTAINER (button), label);

	/* Connect the Button's 'clicked' signal to the signal handler:
	 * pass label as the data, so that the signal handler can use it. */
	g_signal_connect (
		GTK_OBJECT (button), "clicked",
		G_CALLBACK(hello_on_button_click), label);

	gtk_window_set_resizable (GTK_WINDOW (win), TRUE);
	gtk_window_set_default_size (GTK_WINDOW (win), 250, 350);

	/* Create Frame and add it to the main MateComponentWindow: */
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), button);
	matecomponent_window_set_contents (win, frame);

	/* Connect to the delete_event: a close from the window manager */
	g_signal_connect (GTK_OBJECT (win),
			    "delete_event",
			    G_CALLBACK (delete_event_cb),
			    NULL);

	/* Append ourself to the list of windows */
	app_list = g_slist_prepend (app_list, win);

	return GTK_WIDGET(win);
}

int 
main (int argc, char* argv[])
{
	GtkWidget *app;

	/* Setup translaton domain */
	bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

	if (!matecomponent_ui_init ("matecomponent-hello", VERSION, &argc, argv))
		g_error (_("Cannot init libmatecomponentui code"));

	app = hello_new ();

	gtk_widget_show_all (GTK_WIDGET (app));

	matecomponent_main ();

	return 0;
}
