#include "config.h"
#include <matecomponent/matecomponent-ui-component.h>
#include <matecomponent/matecomponent-selector.h>
#include <matecomponent/matecomponent-window.h>
#include <gtk/gtk.h>

#include "container-menu.h"
#include "container.h"
#include "container-filesel.h"
#include "document.h"

static void
verb_AddComponent_cb (MateComponentUIComponent *uic, gpointer user_data, const char *cname)
{
	SampleApp *inst = user_data;
	char *required_interfaces [2] =
	    { "IDL:MateComponent/CanvasComponentFactory:1.0", NULL };
	char *obj_id;

	/* Ask the user to select a component. */
	obj_id = matecomponent_selector_select_id (
		"Select an embeddable MateComponent component to add",
		(const gchar **) required_interfaces);

	if (!obj_id)
		return;

	sample_doc_add_component (inst->doc, obj_id);

	g_free (obj_id);
}

static void
load_response_cb (GtkWidget *caller, gint response, SampleApp *app)
{
	GtkWidget *fs = app->fileselection;
	gchar *filename;

 	if (response == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fs));

		if (filename)
			sample_doc_load (app->doc, filename);

		g_free (filename);
	}

	gtk_widget_destroy (fs);
}

static void
save_response_cb (GtkWidget *caller, gint response, SampleApp *app)
{
	GtkWidget *fs = app->fileselection;
	gchar *filename;

 	if (response == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fs));

		if (filename)
			sample_doc_save (app->doc, filename);

		g_free (filename);
	}

	gtk_widget_destroy (fs);
}

static void
verb_FileSaveAs_cb (MateComponentUIComponent *uic, gpointer user_data, const char *cname)
{
	SampleApp *app = user_data;

	container_request_file (app, TRUE, G_CALLBACK (save_response_cb), app);
}

static void
verb_FileLoad_cb (MateComponentUIComponent *uic, gpointer user_data, const char *cname)
{
	SampleApp *app = user_data;

	container_request_file (app, FALSE, G_CALLBACK (load_response_cb), app);
}

static void
verb_PrintPreview_cb (MateComponentUIComponent *uic, gpointer user_data, const char *cname)
{
#if 0
	SampleApp *app = user_data;

	sample_app_print_preview (app);
#else
	g_warning ("Print Preview not implemented yet.");
#endif
}

static void
verb_HelpAbout_cb (MateComponentUIComponent *uic, gpointer user_data, const char *cname)
{
#if 0
	/* mate_about would require a libmateui dependency */

	static const gchar *authors[] = {
		"ÉRDI Gergõ <cactus@cactus.rulez.org>",
		"Mike Kestner <mkestner@speakeasy.net",
		"Michael Meeks <michael@ximian.com>",
		NULL
	};

	GtkWidget *about = mate_about_new ("sample-container-2", VERSION,
					    "(C) 2000-2001 ÉRDI Gergõ, Mike Kestner, and Ximian, Inc",
					    authors,
					    "MateComponent sample container", NULL);
	gtk_widget_show (about);
#endif
}

static void
verb_FileExit_cb (MateComponentUIComponent *uic, gpointer user_data, const char *cname)
{
	SampleApp *app = user_data;

	sample_app_exit (app);
}

/*
 * The menus.
 */
static char ui_commands [] =
"<commands>\n"
"	<cmd name=\"AddEmbeddable\" _label=\"A_dd Embeddable\"/>\n"
"	<cmd name=\"FileOpen\" _label=\"_Open\"\n"
"		pixtype=\"stock\" pixname=\"Open\" _tip=\"Open a file\"/>\n"
"	<cmd name=\"FileSaveAs\" _label=\"Save _As...\"\n"
"		pixtype=\"stock\" pixname=\"Save\"\n"
"		_tip=\"Save the current file with a different name\"/>\n"
"	<cmd name=\"PrintPreview\" _label=\"Print Pre_view\"/>\n"
"	<cmd name=\"FileExit\" _label=\"E_xit\" _tip=\"Exit the program\"\n"
"		pixtype=\"stock\" pixname=\"Quit\" accel=\"*Control*q\"/>\n"
"	<cmd name=\"HelpAbout\" _label=\"_About...\" _tip=\"About this application\"\n"
"		pixtype=\"stock\" pixname=\"About\"/>\n"
"</commands>";

static char ui_data [] =
"<menu>\n"
"	<submenu name=\"File\" _label=\"_File\">\n"
"		<menuitem name=\"AddEmbeddable\" verb=\"\"/>\n"
"		<separator/>"
"		<menuitem name=\"FileOpen\" verb=\"\"/>\n"
"\n"
"		<menuitem name=\"FileSaveAs\" verb=\"\"/>\n"
"\n"
"		<placeholder name=\"Placeholder\"/>\n"
"\n"
"		<separator/>\n"
"		<menuitem name=\"PrintPreview\" verb=\"\"/>\n"
"		<separator/>\n"
"		<menuitem name=\"FileExit\" verb=\"\"/>\n"
"	</submenu>\n"
"\n"
"	<submenu name=\"Help\" _label=\"_Help\">\n"
"		<menuitem name=\"HelpAbout\" verb=\"\"/>\n"
"	</submenu>\n"
"</menu>";

static MateComponentUIVerb sample_app_verbs[] = {
	MATECOMPONENT_UI_VERB ("AddEmbeddable", verb_AddComponent_cb),
	MATECOMPONENT_UI_VERB ("FileOpen", verb_FileLoad_cb),
	MATECOMPONENT_UI_VERB ("FileSaveAs", verb_FileSaveAs_cb),
	MATECOMPONENT_UI_VERB ("PrintPreview", verb_PrintPreview_cb),
	MATECOMPONENT_UI_VERB ("FileExit", verb_FileExit_cb),
	MATECOMPONENT_UI_VERB ("HelpAbout", verb_HelpAbout_cb),
	MATECOMPONENT_UI_VERB_END
};

void
sample_app_fill_menu (SampleApp *app)
{
	MateComponent_UIContainer corba_uic;
	MateComponentUIComponent *uic;

	uic = matecomponent_ui_component_new ("sample");
	corba_uic = MATECOMPONENT_OBJREF (matecomponent_window_get_ui_container (
						MATECOMPONENT_WINDOW (app->win)));
	matecomponent_ui_component_set_container (uic, corba_uic, NULL);

	matecomponent_ui_component_set_translate (uic, "/", ui_commands, NULL);
	matecomponent_ui_component_set_translate (uic, "/", ui_data, NULL);

	matecomponent_ui_component_add_verb_list_with_data (uic, sample_app_verbs, app);
}
