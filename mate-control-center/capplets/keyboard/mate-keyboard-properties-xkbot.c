/* -*- mode: c; style: linux -*- */

/* mate-keyboard-properties-xkbot.c
 * Copyright (C) 2003-2007 Sergey V. Udaltsov
 *
 * Written by: Sergey V. Udaltsov <svu@gnome.org>
 *             John Spray <spray_john@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib/gi18n.h>
#include <string.h>
#include <mateconf/mateconf-client.h>

#include "capplet-util.h"

#include "mate-keyboard-properties-xkb.h"

static GtkBuilder *chooser_dialog = NULL;
static const char *current1st_level_id = NULL;
static GSList *option_checks_list = NULL;
static GtkWidget *current_none_radio = NULL;
static GtkWidget *current_expander = NULL;
static gboolean current_multi_select = FALSE;
static GSList *current_radio_group = NULL;

#define OPTION_ID_PROP "optionID"
#define SELCOUNTER_PROP "selectionCounter"
#define MATECONFSTATE_PROP "mateconfState"
#define EXPANDERS_PROP "expandersList"

GSList *
xkb_options_get_selected_list (void)
{
	GSList *retval;

	retval = mateconf_client_get_list (xkb_mateconf_client,
					MATEKBD_KEYBOARD_CONFIG_KEY_OPTIONS,
					MATECONF_VALUE_STRING, NULL);
	if (retval == NULL) {
		GSList *cur_option;

		for (cur_option = initial_config.options;
		     cur_option != NULL; cur_option = cur_option->next)
			retval =
			    g_slist_prepend (retval,
					     g_strdup (cur_option->data));

		retval = g_slist_reverse (retval);
	}

	return retval;
}

/* Returns the selection counter of the expander (static current_expander) */
static int
xkb_options_expander_selcounter_get (void)
{
	return
	    GPOINTER_TO_INT (g_object_get_data
			     (G_OBJECT (current_expander),
			      SELCOUNTER_PROP));
}

/* Increments the selection counter in the expander (static current_expander)
   using the value (can be 0)*/
static void
xkb_options_expander_selcounter_add (int value)
{
	g_object_set_data (G_OBJECT (current_expander), SELCOUNTER_PROP,
			   GINT_TO_POINTER
			   (xkb_options_expander_selcounter_get ()
			    + value));
}

/* Resets the seletion counter in the expander (static current_expander) */
static void
xkb_options_expander_selcounter_reset (void)
{
	g_object_set_data (G_OBJECT (current_expander), SELCOUNTER_PROP,
			   GINT_TO_POINTER (0));
}

/* Formats the expander (static current_expander), based on the selection counter */
static void
xkb_options_expander_highlight (void)
{
	char *utf_group_name =
	    g_object_get_data (G_OBJECT (current_expander),
			       "utfGroupName");
	int counter = xkb_options_expander_selcounter_get ();
	if (utf_group_name != NULL) {
		gchar *titlemarkup =
		    g_strconcat (counter >
				 0 ? "<span weight=\"bold\">" : "<span>",
				 utf_group_name, "</span>", NULL);
		gtk_expander_set_label (GTK_EXPANDER (current_expander),
					titlemarkup);
		g_free (titlemarkup);
	}
}

/* Add optionname from the backend's selection list if it's not
   already in there. */
static void
xkb_options_select (gchar * optionname)
{
	gboolean already_selected = FALSE;
	GSList *options_list = xkb_options_get_selected_list ();
	GSList *option;
	for (option = options_list; option != NULL; option = option->next)
		if (!strcmp ((gchar *) option->data, optionname))
			already_selected = TRUE;

	if (!already_selected)
		options_list =
		    g_slist_append (options_list, g_strdup (optionname));
	xkb_options_set_selected_list (options_list);

	clear_xkb_elements_list (options_list);
}

/* Remove all occurences of optionname from the backend's selection list */
static void
xkb_options_deselect (gchar * optionname)
{
	GSList *options_list = xkb_options_get_selected_list ();
	GSList *nodetmp;
	GSList *option = options_list;
	while (option != NULL) {
		gchar *id = (char *) option->data;
		if (!strcmp (id, optionname)) {
			nodetmp = option->next;
			g_free (id);
			options_list =
			    g_slist_remove_link (options_list, option);
			g_slist_free_1 (option);
			option = nodetmp;
		} else
			option = option->next;
	}
	xkb_options_set_selected_list (options_list);
	clear_xkb_elements_list (options_list);
}

/* Return true if optionname describes a string already in the backend's
   list of selected options */
static gboolean
xkb_options_is_selected (gchar * optionname)
{
	gboolean retval = FALSE;
	GSList *options_list = xkb_options_get_selected_list ();
	GSList *option;
	for (option = options_list; option != NULL; option = option->next) {
		if (!strcmp ((gchar *) option->data, optionname))
			retval = TRUE;
	}
	clear_xkb_elements_list (options_list);
	return retval;
}

/* Make sure selected options stay visible when navigating with the keyboard */
static gboolean
option_focused_cb (GtkWidget * widget, GdkEventFocus * event,
		   gpointer data)
{
	GtkScrolledWindow *win = GTK_SCROLLED_WINDOW (data);
	GtkAllocation alloc;
	GtkAdjustment *adj;

	gtk_widget_get_allocation (widget, &alloc);
	adj = gtk_scrolled_window_get_vadjustment (win);
	gtk_adjustment_clamp_page (adj, alloc.y,
				   alloc.y + alloc.height);

	return FALSE;
}

/* Update xkb backend to reflect the new UI state */
static void
option_toggled_cb (GtkWidget * checkbutton, gpointer data)
{
	gpointer optionID =
	    g_object_get_data (G_OBJECT (checkbutton), OPTION_ID_PROP);
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbutton)))
		xkb_options_select (optionID);
	else
		xkb_options_deselect (optionID);
}

/* Add a check_button or radio_button to control a particular option
   This function makes particular use of the current... variables at
   the top of this file. */
static void
xkb_options_add_option (XklConfigRegistry * config_registry,
			XklConfigItem * config_item, GtkBuilder * dialog)
{
	GtkWidget *option_check;
	gchar *utf_option_name = xci_desc_to_utf8 (config_item);
	/* Copy this out because we'll load it into the widget with set_data */
	gchar *full_option_name =
	    g_strdup (matekbd_keyboard_config_merge_items
		      (current1st_level_id, config_item->name));
	gboolean initial_state;

	if (current_multi_select)
		option_check =
		    gtk_check_button_new_with_label (utf_option_name);
	else {
		if (current_radio_group == NULL) {
			/* The first radio in a group is to be "Default", meaning none of
			   the below options are to be included in the selected list.
			   This is a HIG-compliant alternative to allowing no
			   selection in the group. */
			option_check =
			    gtk_radio_button_new_with_label
			    (current_radio_group, _("Default"));
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON
						      (option_check),
						      TRUE);
			/* Make option name underscore -
			   to enforce its first position in the list */
			g_object_set_data_full (G_OBJECT (option_check),
						"utfOptionName",
						g_strdup (" "), g_free);
			option_checks_list =
			    g_slist_append (option_checks_list,
					    option_check);
			current_radio_group =
			    gtk_radio_button_get_group (GTK_RADIO_BUTTON
							(option_check));
			current_none_radio = option_check;

			g_signal_connect (option_check, "focus-in-event",
					  G_CALLBACK (option_focused_cb),
					  WID ("options_scroll"));
		}
		option_check =
		    gtk_radio_button_new_with_label (current_radio_group,
						     utf_option_name);
		current_radio_group =
		    gtk_radio_button_get_group (GTK_RADIO_BUTTON
						(option_check));
		g_object_set_data (G_OBJECT (option_check), "NoneRadio",
				   current_none_radio);
	}

	initial_state = xkb_options_is_selected (full_option_name);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (option_check),
				      initial_state);

	g_object_set_data_full (G_OBJECT (option_check), OPTION_ID_PROP,
				full_option_name, g_free);
	g_object_set_data_full (G_OBJECT (option_check), "utfOptionName",
				utf_option_name, g_free);

	g_signal_connect (option_check, "toggled",
			  G_CALLBACK (option_toggled_cb), NULL);

	option_checks_list =
	    g_slist_append (option_checks_list, option_check);

	g_signal_connect (option_check, "focus-in-event",
			  G_CALLBACK (option_focused_cb),
			  WID ("options_scroll"));

	xkb_options_expander_selcounter_add (initial_state);
	g_object_set_data (G_OBJECT (option_check), MATECONFSTATE_PROP,
			   GINT_TO_POINTER (initial_state));
}

static gint
xkb_option_checks_compare (GtkWidget * chk1, GtkWidget * chk2)
{
	const gchar *t1 =
	    g_object_get_data (G_OBJECT (chk1), "utfOptionName");
	const gchar *t2 =
	    g_object_get_data (G_OBJECT (chk2), "utfOptionName");
	return g_utf8_collate (t1, t2);
}

/* Add a group of options: create title and layout widgets and then
   add widgets for all the options in the group. */
static void
xkb_options_add_group (XklConfigRegistry * config_registry,
		       XklConfigItem * config_item, GtkBuilder * dialog)
{
	GtkWidget *align, *vbox, *option_check;
	gboolean allow_multiple_selection =
	    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (config_item),
						XCI_PROP_ALLOW_MULTIPLE_SELECTION));

	GSList *expanders_list =
	    g_object_get_data (G_OBJECT (dialog), EXPANDERS_PROP);

	gchar *utf_group_name = xci_desc_to_utf8 (config_item);
	gchar *titlemarkup =
	    g_strconcat ("<span>", utf_group_name, "</span>", NULL);

	current_expander = gtk_expander_new (titlemarkup);
	gtk_expander_set_use_markup (GTK_EXPANDER (current_expander),
				     TRUE);
	g_object_set_data_full (G_OBJECT (current_expander),
				"utfGroupName", utf_group_name, g_free);
	g_object_set_data_full (G_OBJECT (current_expander), "groupId",
				g_strdup (config_item->name), g_free);

	g_free (titlemarkup);
	align = gtk_alignment_new (0, 0, 1, 1);
	gtk_alignment_set_padding (GTK_ALIGNMENT (align), 6, 12, 12, 0);
	vbox = gtk_vbox_new (TRUE, 6);
	gtk_container_add (GTK_CONTAINER (align), vbox);
	gtk_container_add (GTK_CONTAINER (current_expander), align);

	current_multi_select = (gboolean) allow_multiple_selection;
	current_radio_group = NULL;
	current1st_level_id = config_item->name;

	option_checks_list = NULL;

	xkl_config_registry_foreach_option (config_registry,
					    config_item->name,
					    (ConfigItemProcessFunc)
					    xkb_options_add_option,
					    dialog);
	/* sort it */
	option_checks_list =
	    g_slist_sort (option_checks_list,
			  (GCompareFunc) xkb_option_checks_compare);
	while (option_checks_list) {
		option_check = GTK_WIDGET (option_checks_list->data);
		gtk_box_pack_start (GTK_BOX (vbox), option_check, TRUE, TRUE, 0);
		option_checks_list = option_checks_list->next;
	}
	/* free it */
	g_slist_free (option_checks_list);
	option_checks_list = NULL;

	xkb_options_expander_highlight ();

	expanders_list = g_slist_append (expanders_list, current_expander);
	g_object_set_data (G_OBJECT (dialog), EXPANDERS_PROP,
			   expanders_list);

	g_signal_connect (current_expander, "focus-in-event",
			  G_CALLBACK (option_focused_cb),
			  WID ("options_scroll"));
}

static gint
xkb_options_expanders_compare (GtkWidget * expander1,
			       GtkWidget * expander2)
{
	const gchar *t1 =
	    g_object_get_data (G_OBJECT (expander1), "utfGroupName");
	const gchar *t2 =
	    g_object_get_data (G_OBJECT (expander2), "utfGroupName");
	return g_utf8_collate (t1, t2);
}

/* Create widgets to represent the options made available by the backend */
void
xkb_options_load_options (GtkBuilder * dialog)
{
	GtkWidget *opts_vbox = WID ("options_vbox");
	GSList *expanders_list;
	GtkWidget *expander;

	current1st_level_id = NULL;
	current_none_radio = NULL;
	current_multi_select = FALSE;
	current_radio_group = NULL;

	/* fill the list */
	xkl_config_registry_foreach_option_group (config_registry,
						  (ConfigItemProcessFunc)
						  xkb_options_add_group,
						  dialog);
	/* sort it */
	expanders_list =
	    g_object_get_data (G_OBJECT (dialog), EXPANDERS_PROP);
	expanders_list =
	    g_slist_sort (expanders_list,
			  (GCompareFunc) xkb_options_expanders_compare);
	g_object_set_data (G_OBJECT (dialog), EXPANDERS_PROP,
			   expanders_list);
	while (expanders_list) {
		expander = GTK_WIDGET (expanders_list->data);
		gtk_box_pack_start (GTK_BOX (opts_vbox), expander, FALSE,
				    FALSE, 0);
		expanders_list = expanders_list->next;
	}

	gtk_widget_show_all (opts_vbox);
}

static void
chooser_response_cb (GtkDialog * dialog, gint response, gpointer data)
{
	switch (response) {
	case GTK_RESPONSE_HELP:
		capplet_help (GTK_WINDOW (dialog),
			      "prefs-keyboard-layoutoptions");
		break;
	case GTK_RESPONSE_CLOSE:{
			/* just cleanup */
			GSList *expanders_list =
			    g_object_get_data (G_OBJECT (dialog),
					       EXPANDERS_PROP);
			g_object_set_data (G_OBJECT (dialog),
					   EXPANDERS_PROP, NULL);
			g_slist_free (expanders_list);

			gtk_widget_destroy (GTK_WIDGET (dialog));
			chooser_dialog = NULL;
		}
		break;
	}
}

/* Create popup dialog */
void
xkb_options_popup_dialog (GtkBuilder * dialog)
{
	GtkWidget *chooser;

	chooser_dialog = gtk_builder_new ();
    gtk_builder_add_from_file (chooser_dialog, MATECC_UI_DIR
                               "/mate-keyboard-properties-options-dialog.ui",
                               NULL);

	chooser = CWID ("xkb_options_dialog");
	gtk_window_set_transient_for (GTK_WINDOW (chooser),
				      GTK_WINDOW (WID
						  ("keyboard_dialog")));
	xkb_options_load_options (chooser_dialog);

	g_signal_connect (chooser, "response",
			  G_CALLBACK (chooser_response_cb), dialog);

	gtk_dialog_run (GTK_DIALOG (chooser));
}

/* Update selected option counters for a group-bound expander */
static void
xkb_options_update_option_counters (XklConfigRegistry * config_registry,
				    XklConfigItem * config_item)
{
	gchar *full_option_name =
	    g_strdup (matekbd_keyboard_config_merge_items
		      (current1st_level_id, config_item->name));
	gboolean current_state =
	    xkb_options_is_selected (full_option_name);
	xkb_options_expander_selcounter_add (current_state);
}

/* Respond to a change in the xkb mateconf settings */
static void
xkb_options_update (MateConfClient * client,
		    guint cnxn_id, MateConfEntry * entry, GtkBuilder * dialog)
{
	/* Updating options is handled by mateconf notifies for each widget
	   This is here to avoid calling it N_OPTIONS times for each mateconf
	   change. */
	enable_disable_restoring (dialog);

	if (chooser_dialog != NULL) {
		GSList *expanders_list =
		    g_object_get_data (G_OBJECT (chooser_dialog),
				       EXPANDERS_PROP);
		while (expanders_list) {
			current_expander =
			    GTK_WIDGET (expanders_list->data);
			gchar *group_id =
			    g_object_get_data (G_OBJECT (current_expander),
					       "groupId");
			current1st_level_id = group_id;
			xkb_options_expander_selcounter_reset ();
			xkl_config_registry_foreach_option
			    (config_registry, group_id,
			     (ConfigItemProcessFunc)
			     xkb_options_update_option_counters,
			     current_expander);
			xkb_options_expander_highlight ();
			expanders_list = expanders_list->next;
		}
	}
}

void
xkb_options_register_mateconf_listener (GtkBuilder * dialog)
{
	mateconf_client_notify_add (xkb_mateconf_client,
				 MATEKBD_KEYBOARD_CONFIG_KEY_OPTIONS,
				 (MateConfClientNotifyFunc)
				 xkb_options_update, dialog, NULL, NULL);
}
