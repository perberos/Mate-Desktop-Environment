/* MateComponent component browser
 *
 * AUTHORS:
 *      Dan Siemon <dan@coverfire.com>
 *      Rodrigo Moya <rodrigo@mate-db.org>
 *      Patanjali Somayaji <patanjali@morelinux.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <matecomponent/matecomponent-main.h>
#include <matecomponent/matecomponent-ui-component.h>
#include <matecomponent/matecomponent-ui-container.h>
#include <matecomponent/matecomponent-window.h>
#include <matecomponent/matecomponent-ui-util.h>
#include "matecomponent-browser.h"
#include "component-list.h"
#include "component-details.h"

static void verb_FileNewWindow (MateComponentUIComponent *uic, void *data, const char *path);
static void verb_FileClose (MateComponentUIComponent *uic, void *data, const char *path);
static void verb_HelpAbout (MateComponentUIComponent *uic, void *data, const char *path);

static GList *open_windows = NULL;

static MateComponentUIVerb window_verbs[] = {
	MATECOMPONENT_UI_VERB ("FileNewWindow", verb_FileNewWindow),
	MATECOMPONENT_UI_VERB ("FileClose", verb_FileClose),
	MATECOMPONENT_UI_VERB ("HelpAbout", verb_HelpAbout),
	MATECOMPONENT_UI_VERB_END
};

struct window_info {
	GtkWidget *comp_list;
	GtkWidget *entry;
};

/*********************************
 * Callbacks
 *********************************/
static void
window_closed_cb (GObject *object, gpointer user_data)
{
	MateComponentWindow *window = (MateComponentWindow *) object;

	g_return_if_fail (MATECOMPONENT_IS_WINDOW (window));

	open_windows = g_list_remove (open_windows, window);

	gtk_widget_destroy (GTK_WIDGET (window));
	if (g_list_length (open_windows) <= 0) {
		matecomponent_main_quit ();
	}
}

/*
 * Called when the Close button is clicked on a details window.
 */
static void
close_details_window_cb (GObject *object, gint response_id, gpointer user_data)
{
	GtkWidget *window = (GtkWidget *) user_data;

	gtk_widget_destroy (GTK_WIDGET (window));
}

/*
 * Callbacks for the query buttons.
 */
static void
all_query_cb (GObject *object, gpointer data)
{
	struct window_info *info;

	info = (struct window_info *) data;

	component_list_show (COMPONENT_LIST (info->comp_list),
			     "_active || _active == FALSE");

	gtk_entry_set_text (GTK_ENTRY (info->entry),
			    "_active || _active == FALSE");
}

static void
active_query_cb (GObject *object, gpointer data)
{
	struct window_info *info;

	info = (struct window_info *) data;

	component_list_show (COMPONENT_LIST (info->comp_list),
			     "_active");

	gtk_entry_set_text (GTK_ENTRY (info->entry),
			    "_active");
}

static void
inactive_query_cb (GObject *object, gpointer data)
{
	struct window_info *info;

	info = (struct window_info *) data;

	component_list_show (COMPONENT_LIST (info->comp_list),
			     "_active == FALSE");

	gtk_entry_set_text (GTK_ENTRY (info->entry),
			    "_active == FALSE");
}

static void
execute_query_cb (GObject *object, gpointer data)
{
	struct window_info *info;
	char *query;

	info = (struct window_info *) data;

	query = g_strdup_printf ("%s",
				 gtk_entry_get_text (GTK_ENTRY (info->entry)));

	component_list_show (COMPONENT_LIST (info->comp_list), query);
}

/*
 * Creates and shows the details window.
 */
static void
component_details_cb (GObject *object, gpointer data)
{
	GtkWidget *comp_details;
	GtkWidget *window;
	ComponentList *list;
	gchar *iid = NULL;

	list = (ComponentList *) data;

	iid = component_list_get_selected_iid (list);
	if (iid == NULL) {
		/* We do not handle this situation */
		g_assert_not_reached();
	}
	
	window = gtk_dialog_new_with_buttons (_("Component Details"),
					      NULL, 0,
					      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					      NULL);
	g_signal_connect (G_OBJECT (window), "response",
			  G_CALLBACK (close_details_window_cb), window);

	comp_details = component_details_new (iid);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), comp_details, TRUE, TRUE, 0);

	gtk_widget_show_all (window);

	g_free (iid);
}

/*
 * Verbs' commands
 */
static void
verb_FileClose (MateComponentUIComponent *uic, void *data, const char *path)
{
	MateComponentWindow *window = (MateComponentWindow *) data;

	g_return_if_fail (MATECOMPONENT_IS_WINDOW (window));
	gtk_widget_destroy (GTK_WIDGET (window));
}

static void
verb_FileNewWindow (MateComponentUIComponent *uic, void *data, const char *path)
{
	matecomponent_browser_create_window ();
}

static void
verb_HelpAbout (MateComponentUIComponent *uic, void *data, const char *path)
{
	MateComponentWindow *window = (MateComponentWindow *) data;

	static const gchar *authors[] = {
		"Dan Siemon <dan@coverfire.com>",
		"Rodrigo Moya <rodrigo@mate-db.org>",
		"Patanjali Somayaji <patanjali@morelinux.com>",
		NULL
	};

	gtk_show_about_dialog (GTK_WINDOW (window),
		      "name", _("MateComponent Browser"),
		      "version", VERSION,
		      "copyright", _("Copyright 2001, The MATE Foundation"),
		      "comments", _("MateComponent component browser"),
		      "authors", authors,
		      "documenters", NULL,
		      "translator-credits", _("translator-credits"),
		      "logo-icon-name", "gtk-about",
		      NULL);
}

/*
 * Public functions
 */
void
matecomponent_browser_create_window (void)
{
	GtkWidget *window, *status_bar;
	GtkWidget *all_button, *active_button, *inactive_button;
	GtkWidget *query_label, *execute_button;
	GtkWidget *main_vbox, *hbox;
	struct window_info *info;
	MateComponentUIContainer *ui_container;
	MateComponentUIComponent *ui_component;
	MateComponent_UIContainer corba_container;
	CORBA_Environment ev;

	CORBA_exception_init (&ev);

	info = g_malloc (sizeof (struct window_info));

	/* create the window */
	window = matecomponent_window_new ("matecomponent-browser", _("Component Browser"));
	gtk_window_set_role (GTK_WINDOW (window), "Main window");
	gtk_window_set_type_hint (GTK_WINDOW (window),
				  GDK_WINDOW_TYPE_HINT_NORMAL);
	g_signal_connect (G_OBJECT (window), "delete_event",
			  G_CALLBACK (window_closed_cb), NULL);
	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (window_closed_cb), NULL);

	ui_container = matecomponent_window_get_ui_container (MATECOMPONENT_WINDOW(window));
	corba_container = MATECOMPONENT_OBJREF (ui_container);

	ui_component = matecomponent_ui_component_new ("matecomponent-browser");
	matecomponent_ui_component_set_container (ui_component, corba_container,
					   NULL);

	/* set UI for the window */
	matecomponent_ui_component_freeze (ui_component, NULL);
	matecomponent_ui_util_set_ui (ui_component, MATECOMPONENT_BROWSER_DATADIR,
			       "matecomponent-browser.xml",
			       "matecomponent-browser", &ev);
	matecomponent_ui_component_add_verb_list_with_data (ui_component,
						     window_verbs, window); 
	matecomponent_ui_component_thaw (ui_component, NULL);

	/* Create the main window */
	main_vbox = gtk_vbox_new (FALSE, 0);
	matecomponent_window_set_contents (MATECOMPONENT_WINDOW (window), main_vbox);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 3);

	info->comp_list = component_list_new ();
	gtk_box_pack_start (GTK_BOX (main_vbox), info->comp_list,
			    TRUE, TRUE, 3);

	status_bar = gtk_statusbar_new ();
	gtk_box_pack_start (GTK_BOX (main_vbox), status_bar, FALSE, TRUE, 3);

	/* Fill out the tool bar */
	all_button = gtk_radio_button_new_with_label (NULL, _("All"));
	g_signal_connect (G_OBJECT (all_button), "clicked",
			  G_CALLBACK (all_query_cb), info);
	active_button = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (all_button)), _("Active"));
	g_signal_connect (G_OBJECT (active_button), "clicked",
			  G_CALLBACK (active_query_cb), info);
	inactive_button = gtk_radio_button_new_with_label (gtk_radio_button_get_group (GTK_RADIO_BUTTON (all_button)), _("Inactive"));
	g_signal_connect (G_OBJECT (inactive_button), "clicked",
			  G_CALLBACK (inactive_query_cb), info);
	query_label = gtk_label_new ("Query:");
	info->entry = gtk_entry_new ();
	g_signal_connect (GTK_ENTRY (info->entry), "activate",
			  G_CALLBACK (execute_query_cb), info);
	execute_button = gtk_button_new_from_stock (GTK_STOCK_EXECUTE);
	g_signal_connect (G_OBJECT (execute_button), "clicked",
			  G_CALLBACK (execute_query_cb), info);

	gtk_box_pack_start (GTK_BOX (hbox), all_button, FALSE, FALSE, 3);
	gtk_box_pack_start (GTK_BOX (hbox), active_button, FALSE, FALSE, 3);
	gtk_box_pack_start (GTK_BOX (hbox), inactive_button, FALSE, FALSE, 3);
	gtk_box_pack_start (GTK_BOX (hbox), query_label, FALSE, FALSE, 1);
	gtk_box_pack_start (GTK_BOX (hbox), info->entry, TRUE, TRUE, 3);
	gtk_box_pack_start (GTK_BOX (hbox), execute_button, FALSE, FALSE, 3);

	/* Attach to the component-details signal */
	g_signal_connect (G_OBJECT (info->comp_list), "component-details",
			  G_CALLBACK (component_details_cb), info->comp_list);

	component_list_show (COMPONENT_LIST (info->comp_list),
			     "_active || _active == FALSE");
	gtk_entry_set_text (GTK_ENTRY (info->entry),
			    "_active || _active == FALSE");

	/* add this window to our list of open windows */
	open_windows = g_list_append (open_windows, window);

	gtk_widget_set_size_request (window, 600, 500);
	gtk_widget_show_all (window);
}
