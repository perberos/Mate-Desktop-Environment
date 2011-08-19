/*
 * Mini-Commander Applet
 * Copyright (C) 1998, 1999 Oliver Maruhn <oliver@maruhn.com>
 *               2002 Sun Microsystems Inc.
 *
 * Authors: Oliver Maruhn <oliver@maruhn.com>
 *          Mark McLoughlin <mark@skynet.ie>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <config.h>

#include "preferences.h"

#include <string.h>

#include <gtk/gtk.h>

#include <mate-panel-applet.h>
#include <mate-panel-applet-mateconf.h>
#include <mateconf/mateconf-client.h>

#include "mini-commander_applet.h"
#include "command_line.h"
#include "history.h"
#include "mc-default-macros.h"

enum {
	COLUMN_PATTERN,
	COLUMN_COMMAND
};

#define NEVER_SENSITIVE		"never_sensitive"

static GSList *mc_load_macros (MCData *mc);

/* set sensitive and setup NEVER_SENSITIVE appropriately */
static void
hard_set_sensitive (GtkWidget *w, gboolean sensitivity)
{
	gtk_widget_set_sensitive (w, sensitivity);
	g_object_set_data (G_OBJECT (w), NEVER_SENSITIVE,
			   GINT_TO_POINTER ( ! sensitivity));
}


/* set sensitive, but always insensitive if NEVER_SENSITIVE is set */
static void
soft_set_sensitive (GtkWidget *w, gboolean sensitivity)
{
	if (g_object_get_data (G_OBJECT (w), NEVER_SENSITIVE))
		gtk_widget_set_sensitive (w, FALSE);
	else
		gtk_widget_set_sensitive (w, sensitivity);
}

gboolean
mc_key_writable (MCData *mc, const char *key)
{
	gboolean writable;
	char *fullkey;
	static MateConfClient *client = NULL;
	if (client == NULL)
		client = mateconf_client_get_default ();

	fullkey = mate_panel_applet_mateconf_get_full_key (mc->applet, key);

	writable = mateconf_client_key_is_writable (client, fullkey, NULL);

	g_free (fullkey);

	return writable;
}


/* MateConf notification handlers
 */
static void
show_default_theme_changed (MateConfClient  *client,
			    guint         cnxn_id,
			    MateConfEntry   *entry,
			    MCData       *mc)
{
    if (!entry->value || entry->value->type != MATECONF_VALUE_BOOL)
	return;

    mc->preferences.show_default_theme = mateconf_value_get_bool (entry->value);

    mc_applet_draw (mc); /* FIXME: we shouldn't have to redraw the whole applet */
}

static void
auto_complete_history_changed (MateConfClient  *client,
			       guint         cnxn_id,
			       MateConfEntry   *entry,
			       MCData       *mc)
{
    if (!entry->value || entry->value->type != MATECONF_VALUE_BOOL)
	return;

    mc->preferences.auto_complete_history = mateconf_value_get_bool (entry->value);
}

static void
normal_size_x_changed (MateConfClient  *client,
		       guint         cnxn_id,
		       MateConfEntry   *entry,
		       MCData       *mc)
{
    if (!entry->value || entry->value->type != MATECONF_VALUE_INT)
	return;

    mc->preferences.normal_size_x = mateconf_value_get_int (entry->value);

    mc_command_update_entry_size (mc);
}

static void
normal_size_y_changed (MateConfClient  *client,
		       guint         cnxn_id,
		       MateConfEntry   *entry,
		       MCData       *mc)
{
    if (!entry->value || entry->value->type != MATECONF_VALUE_INT)
	return;

    mc->preferences.normal_size_y = mateconf_value_get_int (entry->value);

    mc_applet_draw (mc); /* FIXME: we shouldn't have to redraw the whole applet */
}

static void
cmd_line_color_fg_r_changed (MateConfClient  *client,
			     guint         cnxn_id,
			     MateConfEntry   *entry,
			     MCData       *mc)
{
    if (!entry->value || entry->value->type != MATECONF_VALUE_INT)
	return;

    mc->preferences.cmd_line_color_fg_r = mateconf_value_get_int (entry->value);

    mc_command_update_entry_color (mc);
}

static void
cmd_line_color_fg_g_changed (MateConfClient  *client,
			     guint         cnxn_id,
			     MateConfEntry   *entry,
			     MCData       *mc)
{
    if (!entry->value || entry->value->type != MATECONF_VALUE_INT)
	return;

    mc->preferences.cmd_line_color_fg_g = mateconf_value_get_int (entry->value);

    mc_command_update_entry_color (mc);
}

static void
cmd_line_color_fg_b_changed (MateConfClient  *client,
			     guint         cnxn_id,
			     MateConfEntry   *entry,
			     MCData       *mc)
{
    if (!entry->value || entry->value->type != MATECONF_VALUE_INT)
	return;

    mc->preferences.cmd_line_color_fg_b = mateconf_value_get_int (entry->value);

    mc_command_update_entry_color (mc);
}

static void
cmd_line_color_bg_r_changed (MateConfClient  *client,
			     guint         cnxn_id,
			     MateConfEntry   *entry,
			     MCData       *mc)
{
    if (!entry->value || entry->value->type != MATECONF_VALUE_INT)
	return;

    mc->preferences.cmd_line_color_bg_r = mateconf_value_get_int (entry->value);

    mc_command_update_entry_color (mc);
}

static void
cmd_line_color_bg_g_changed (MateConfClient  *client,
			     guint         cnxn_id,
			     MateConfEntry   *entry,
			     MCData       *mc)
{
    if (!entry->value || entry->value->type != MATECONF_VALUE_INT)
	return;

    mc->preferences.cmd_line_color_bg_g = mateconf_value_get_int (entry->value);

    mc_command_update_entry_color (mc);
}

static void
cmd_line_color_bg_b_changed (MateConfClient  *client,
			     guint         cnxn_id,
			     MateConfEntry   *entry,
			     MCData       *mc)
{
    if (!entry->value || entry->value->type != MATECONF_VALUE_INT)
	return;

    mc->preferences.cmd_line_color_bg_b = mateconf_value_get_int (entry->value);

    mc_command_update_entry_color (mc);
}

static gboolean
load_macros_in_idle (MCData *mc)
{
    mc->preferences.idle_macros_loader_id = 0;

    if (mc->preferences.macros)
	mc_macros_free (mc->preferences.macros);

    mc->preferences.macros = mc_load_macros (mc);

    return FALSE;
}

static void
macros_changed (MateConfClient  *client,
		guint         cnxn_id,
		MateConfEntry   *entry,
		MCData       *mc)
{
    if (!entry->value || entry->value->type != MATECONF_VALUE_LIST)
	return;

    if (mc->preferences.idle_macros_loader_id == 0)
	mc->preferences.idle_macros_loader_id =
		g_idle_add ((GSourceFunc) load_macros_in_idle, mc);
}

/* Properties dialog
 */
static void
save_macros_to_mateconf (MCData *mc)
{
    MCPrefsDialog *dialog;
    GtkTreeIter    iter;
    MateConfValue    *patterns;
    MateConfValue    *commands;
    GSList        *pattern_list = NULL;
    GSList        *command_list = NULL;
    MateConfClient   *client;

    dialog = &mc->prefs_dialog;

    if (!gtk_tree_model_get_iter_first  (GTK_TREE_MODEL (dialog->macros_store), &iter))
	return;

    patterns = mateconf_value_new (MATECONF_VALUE_LIST);
    mateconf_value_set_list_type (patterns, MATECONF_VALUE_STRING);

    commands = mateconf_value_new (MATECONF_VALUE_LIST);
    mateconf_value_set_list_type (commands, MATECONF_VALUE_STRING);

    do {
	char *pattern = NULL;
	char *command = NULL;

	gtk_tree_model_get (
		GTK_TREE_MODEL (dialog->macros_store), &iter,
		0, &pattern,
		1, &command,
		-1);

	pattern_list = g_slist_prepend (pattern_list,
					mateconf_value_new_from_string (MATECONF_VALUE_STRING, pattern, NULL));
	command_list = g_slist_prepend (command_list,
					mateconf_value_new_from_string (MATECONF_VALUE_STRING, command, NULL));
    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->macros_store), &iter));

    pattern_list = g_slist_reverse (pattern_list);
    command_list = g_slist_reverse (command_list);

    mateconf_value_set_list_nocopy (patterns, pattern_list); pattern_list = NULL;
    mateconf_value_set_list_nocopy (commands, command_list); command_list = NULL;
    
    client = mateconf_client_get_default ();
    mateconf_client_set (client, "/apps/mini-commander/macro_patterns",
		    patterns, NULL);
    mateconf_client_set (client, "/apps/mini-commander/macro_commands",
		    commands, NULL);

    mateconf_value_free (patterns);
    mateconf_value_free (commands);
}

static gboolean
duplicate_pattern (MCData     *mc,
		   const char *new_pattern)
{
    MCPrefsDialog *dialog;
    GtkTreeIter    iter;

    dialog = &mc->prefs_dialog;

    if (!gtk_tree_model_get_iter_first  (GTK_TREE_MODEL (dialog->macros_store), &iter))
	return FALSE;

    do {
	char *pattern = NULL;

	gtk_tree_model_get (
		GTK_TREE_MODEL (dialog->macros_store), &iter,
		0, &pattern, -1);

	if (!strcmp (pattern, new_pattern))
		return TRUE;

    } while (gtk_tree_model_iter_next (GTK_TREE_MODEL (dialog->macros_store), &iter));


    return FALSE;
}

static void
show_help_section (GtkWindow *dialog, gchar *section)
{
	GError *error = NULL;
	char *uri;

	if (section)
		uri = g_strdup_printf ("ghelp:command-line?%s", section);
	else
		uri = g_strdup ("ghelp:command-line");

	gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (dialog)),
			uri,
			gtk_get_current_event_time (),
			&error);

	g_free (uri);

	if (error) {
		GtkWidget *error_dialog;

		error_dialog = gtk_message_dialog_new (
				NULL,
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				_("There was an error displaying help: %s"),
				error->message);

		g_signal_connect (error_dialog, "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);

		gtk_window_set_resizable (GTK_WINDOW (error_dialog), FALSE);
		gtk_window_set_screen (GTK_WINDOW (error_dialog),
				       gtk_widget_get_screen (GTK_WIDGET (dialog)));
		gtk_widget_show (error_dialog);
		g_error_free (error);
	}
}

static void
add_response (GtkWidget *window,
	      int        id,
	      MCData    *mc)
{
    MCPrefsDialog *dialog;

    dialog = &mc->prefs_dialog;

    switch (id) {
    case GTK_RESPONSE_OK: {
	const char  *pattern;
	const char  *command;
	GtkTreeIter  iter;
	const char  *error_message = NULL;

	pattern = gtk_entry_get_text (GTK_ENTRY (dialog->pattern_entry));
	command = gtk_entry_get_text (GTK_ENTRY (dialog->command_entry));

	if (!pattern || !pattern [0])
		error_message = _("You must specify a pattern");

	if (!command || !command [0]) 
		error_message = error_message != NULL ?
					_("You must specify a pattern and a command") :
					_("You must specify a command");

	if (!error_message && duplicate_pattern (mc, pattern))
		error_message = _("You may not specify duplicate patterns");

	if (error_message) {
	    GtkWidget *error_dialog;

	    error_dialog = gtk_message_dialog_new (GTK_WINDOW (window),
					           GTK_DIALOG_DESTROY_WITH_PARENT,
					           GTK_MESSAGE_ERROR,
					           GTK_BUTTONS_OK,
					           error_message);

	    g_signal_connect (error_dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);
	    gtk_window_set_resizable (GTK_WINDOW (error_dialog), FALSE);
	    gtk_widget_show_all (error_dialog);
	    return;
	}

	gtk_widget_hide (window);

	gtk_list_store_append (dialog->macros_store, &iter);
	gtk_list_store_set (dialog->macros_store, &iter,
			    COLUMN_PATTERN, pattern, 
			    COLUMN_COMMAND, command,
			    -1);

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW (dialog->macros_tree));

	gtk_editable_delete_text (GTK_EDITABLE (dialog->pattern_entry), 0, -1);
	gtk_editable_delete_text (GTK_EDITABLE (dialog->command_entry), 0, -1);

	save_macros_to_mateconf (mc);
    }
	break;
    case GTK_RESPONSE_HELP:
    	show_help_section (GTK_WINDOW (window), "command-line-prefs-2");
	break;
    case GTK_RESPONSE_CLOSE:
    default:
	gtk_editable_delete_text (GTK_EDITABLE (dialog->pattern_entry), 0, -1);
	gtk_editable_delete_text (GTK_EDITABLE (dialog->command_entry), 0, -1);
	gtk_widget_hide (window);
	break;
    }
}

static void
setup_add_dialog (GtkBuilder *builder,
		  MCData     *mc)
{
    MCPrefsDialog *dialog;

    dialog = &mc->prefs_dialog;

    g_signal_connect (dialog->macro_add_dialog, "response",
		      G_CALLBACK (add_response), mc);

    dialog->pattern_entry = GTK_WIDGET (gtk_builder_get_object (builder, "pattern_entry"));
    dialog->command_entry = GTK_WIDGET (gtk_builder_get_object (builder, "command_entry"));

    gtk_dialog_set_default_response (GTK_DIALOG (dialog->macro_add_dialog), GTK_RESPONSE_OK);
}

static void
macro_add (GtkWidget *button,
           MCData    *mc)
{
    if (!mc->prefs_dialog.macro_add_dialog) {
	GtkBuilder *builder;

	builder = gtk_builder_new ();
	gtk_builder_add_from_file (builder, GTK_BUILDERDIR "/mini-commander.ui", NULL);

	mc->prefs_dialog.macro_add_dialog = GTK_WIDGET (gtk_builder_get_object (builder, "mc_macro_add_dialog"));

	g_object_add_weak_pointer (G_OBJECT (mc->prefs_dialog.macro_add_dialog),
				   (gpointer *) &mc->prefs_dialog.macro_add_dialog);

	setup_add_dialog (builder, mc);

	g_object_unref (builder);
    }

    gtk_window_set_screen (GTK_WINDOW (mc->prefs_dialog.macro_add_dialog),
			   gtk_widget_get_screen (GTK_WIDGET (mc->applet)));
    gtk_widget_grab_focus (mc->prefs_dialog.pattern_entry);
    gtk_window_present (GTK_WINDOW (mc->prefs_dialog.macro_add_dialog));
}

static void
macro_delete (GtkWidget *button,
	      MCData    *mc)
{
    MCPrefsDialog    *dialog;
    GtkTreeModel     *model = NULL;
    GtkTreeSelection *selection;
    GtkTreeIter       iter;
  
    dialog = &mc->prefs_dialog;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->macros_tree));

    if (!gtk_tree_selection_get_selected (selection, &model, &iter))
	return;

    gtk_list_store_remove (dialog->macros_store, &iter);

    save_macros_to_mateconf (mc);
}

static void
show_macros_list (MCData *mc)
{
    MCPrefsDialog *dialog;
    GtkTreeIter    iter;
    GSList        *l;

    dialog = &mc->prefs_dialog;

    gtk_list_store_clear (dialog->macros_store);

    for (l = mc->preferences.macros; l; l = l->next) {
	MCMacro *macro = l->data;

	gtk_list_store_append (dialog->macros_store, &iter);
	gtk_list_store_set (dialog->macros_store, &iter,
			    COLUMN_PATTERN, macro->pattern,
			    COLUMN_COMMAND, macro->command,
			    -1);
    }

    gtk_tree_view_columns_autosize (GTK_TREE_VIEW (dialog->macros_tree));
}

static void
macro_edited (GtkCellRendererText *renderer,
	      const char          *path,
	      const char          *new_text,
	      MCData              *mc)
{
    MCPrefsDialog *dialog;
    GtkTreeIter    iter;
    int            col;

    dialog = &mc->prefs_dialog;

    col = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (renderer), "column"));

    if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (dialog->macros_store), &iter, path))
	gtk_list_store_set (dialog->macros_store, &iter, col, new_text, -1);

    save_macros_to_mateconf (mc);
}

static void
foreground_color_set (GtkColorButton *color_button,
		      MCData    *mc)
{
    GdkColor color;
    
    gtk_color_button_get_color (color_button, &color);
    
    mate_panel_applet_mateconf_set_int (mc->applet, "cmd_line_color_fg_r", (int) color.red, NULL);
    mate_panel_applet_mateconf_set_int (mc->applet, "cmd_line_color_fg_g", (int) color.green, NULL);
    mate_panel_applet_mateconf_set_int (mc->applet, "cmd_line_color_fg_b", (int) color.blue, NULL);
}

static void
background_color_set (GtkColorButton *color_button,
		      MCData    *mc)
{
    GdkColor color;
    
    gtk_color_button_get_color (color_button, &color);
    
    mate_panel_applet_mateconf_set_int (mc->applet, "cmd_line_color_bg_r", (int) color.red, NULL);
    mate_panel_applet_mateconf_set_int (mc->applet, "cmd_line_color_bg_g", (int) color.green, NULL);
    mate_panel_applet_mateconf_set_int (mc->applet, "cmd_line_color_bg_b", (int) color.blue, NULL);
}

static void
auto_complete_history_toggled (GtkToggleButton *toggle,
			       MCData          *mc)
{
    gboolean auto_complete_history;
    
    auto_complete_history = gtk_toggle_button_get_active (toggle);
    if (auto_complete_history == mc->preferences.auto_complete_history) 
        return;
        
    mate_panel_applet_mateconf_set_bool (mc->applet, "autocomplete_history",
				 auto_complete_history, NULL);
}

static void
size_value_changed (GtkSpinButton *spinner,
		    MCData        *mc)
{
    int size;

    size = gtk_spin_button_get_value (spinner);
    if (size == mc->preferences.normal_size_x)
	return;

    mate_panel_applet_mateconf_set_int (mc->applet, "normal_size_x", size, NULL);
}

static void
use_default_theme_toggled (GtkToggleButton *toggle,
			   MCData          *mc)
{
    gboolean use_default_theme;
    
    use_default_theme = gtk_toggle_button_get_active (toggle);
    if (use_default_theme == mc->preferences.show_default_theme) 
        return;
        
    soft_set_sensitive (mc->prefs_dialog.fg_color_picker, !use_default_theme);
    soft_set_sensitive (mc->prefs_dialog.bg_color_picker, !use_default_theme);

    mate_panel_applet_mateconf_set_bool (mc->applet, "show_default_theme", use_default_theme, NULL);
}

static void
preferences_response (MCPrefsDialog *dialog,
		      int        id,
		      MCData    *mc)
{
    switch (id) {
    case GTK_RESPONSE_HELP:
    	show_help_section (GTK_WINDOW (dialog), "command-line-prefs-0");
	break;
    case GTK_RESPONSE_CLOSE:
    default: {
        GtkTreeViewColumn *col;

	dialog = &mc->prefs_dialog;

	/* A hack to make sure 'edited' on the renderer if we
	 * close the dialog while editing.
	 */
	col = gtk_tree_view_get_column (GTK_TREE_VIEW (dialog->macros_tree), 0);
	if (col->editable_widget && GTK_IS_CELL_EDITABLE (col->editable_widget))
	    gtk_cell_editable_editing_done (col->editable_widget);

	col = gtk_tree_view_get_column (GTK_TREE_VIEW (dialog->macros_tree), 1);
	if (col->editable_widget && GTK_IS_CELL_EDITABLE (col->editable_widget))
	    gtk_cell_editable_editing_done (col->editable_widget);

	gtk_widget_hide (dialog->dialog);
    }
	break;
    }
}

static void
mc_preferences_setup_dialog (GtkBuilder *builder,
			     MCData     *mc)
{
    MCPrefsDialog   *dialog;
    GtkCellRenderer *renderer;
    MateConfClient     *client;
    GdkColor         color;

    dialog = &mc->prefs_dialog;

    g_signal_connect (dialog->dialog, "response",
		      G_CALLBACK (preferences_response), mc);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog), GTK_RESPONSE_CLOSE);
    gtk_window_set_default_size (GTK_WINDOW (dialog->dialog), 400, -1);

    dialog->auto_complete_history_toggle = GTK_WIDGET (gtk_builder_get_object (builder, "auto_complete_history_toggle"));
    dialog->size_spinner                 = GTK_WIDGET (gtk_builder_get_object (builder, "size_spinner"));
    dialog->use_default_theme_toggle     = GTK_WIDGET (gtk_builder_get_object (builder, "default_theme_toggle"));
    dialog->fg_color_picker              = GTK_WIDGET (gtk_builder_get_object (builder, "fg_color_picker"));
    dialog->bg_color_picker              = GTK_WIDGET (gtk_builder_get_object (builder, "bg_color_picker"));
    dialog->macros_tree                  = GTK_WIDGET (gtk_builder_get_object (builder, "macros_tree"));
    dialog->delete_button                = GTK_WIDGET (gtk_builder_get_object (builder, "delete_button"));
    dialog->add_button                   = GTK_WIDGET (gtk_builder_get_object (builder, "add_button"));

    /* History based autocompletion */
    g_signal_connect (dialog->auto_complete_history_toggle, "toggled",
		      G_CALLBACK (auto_complete_history_toggled), mc);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->auto_complete_history_toggle),
				  mc->preferences.auto_complete_history);
    if ( ! mc_key_writable (mc, "autocomplete_history"))
	    hard_set_sensitive (dialog->auto_complete_history_toggle, FALSE);

    /* Width */
    gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->size_spinner), mc->preferences.normal_size_x);
    g_signal_connect (dialog->size_spinner, "value_changed",
		      G_CALLBACK (size_value_changed), mc); 
    if ( ! mc_key_writable (mc, "normal_size_x")) {
	    hard_set_sensitive (dialog->size_spinner, FALSE);
	    hard_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "size_label")), FALSE);
	    hard_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "size_post_label")), FALSE);
    }

    /* Use default theme */
    g_signal_connect (dialog->use_default_theme_toggle, "toggled",
		      G_CALLBACK (use_default_theme_toggled), mc);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->use_default_theme_toggle),
				  mc->preferences.show_default_theme);
    if ( ! mc_key_writable (mc, "show_default_theme"))
	    hard_set_sensitive (dialog->use_default_theme_toggle, FALSE);

    /* Foreground color */
    g_signal_connect (dialog->fg_color_picker, "color_set",
		      G_CALLBACK (foreground_color_set), mc);
    color.red = mc->preferences.cmd_line_color_fg_r;
    color.green = mc->preferences.cmd_line_color_fg_g;
    color.blue = mc->preferences.cmd_line_color_fg_b;
    gtk_color_button_set_color (GTK_COLOR_BUTTON (dialog->fg_color_picker), &color);
    soft_set_sensitive (dialog->fg_color_picker, !mc->preferences.show_default_theme);

    if ( ! mc_key_writable (mc, "cmd_line_color_fg_r") ||
	 ! mc_key_writable (mc, "cmd_line_color_fg_g") ||
	 ! mc_key_writable (mc, "cmd_line_color_fg_b")) {
	    hard_set_sensitive (dialog->fg_color_picker, FALSE);
	    hard_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "fg_color_label")), FALSE);
    }

    /* Background color */
    g_signal_connect (dialog->bg_color_picker, "color_set",
		      G_CALLBACK (background_color_set), mc);
    color.red = mc->preferences.cmd_line_color_bg_r;
    color.green = mc->preferences.cmd_line_color_bg_g;
    color.blue = mc->preferences.cmd_line_color_bg_b;
    gtk_color_button_set_color (GTK_COLOR_BUTTON (dialog->bg_color_picker), &color);
    soft_set_sensitive (dialog->bg_color_picker, !mc->preferences.show_default_theme);

    if ( ! mc_key_writable (mc, "cmd_line_color_bg_r") ||
	 ! mc_key_writable (mc, "cmd_line_color_bg_g") ||
	 ! mc_key_writable (mc, "cmd_line_color_bg_b")) {
	    hard_set_sensitive (dialog->bg_color_picker, FALSE);
	    hard_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "bg_color_label")), FALSE);
    }


    /* Macros Delete and Add buttons */
    g_signal_connect (dialog->delete_button, "clicked", G_CALLBACK (macro_delete), mc);
    g_signal_connect (dialog->add_button, "clicked", G_CALLBACK (macro_add), mc);

    client = mateconf_client_get_default ();
    if ( ! mateconf_client_key_is_writable (client,
		 "/apps/mini-commander/macro_patterns", NULL) ||
	 ! mateconf_client_key_is_writable (client,
		 "/apps/mini-commander/macro_commands", NULL)) {
	    hard_set_sensitive (dialog->add_button, FALSE);
	    hard_set_sensitive (dialog->delete_button, FALSE);
	    hard_set_sensitive (dialog->macros_tree, FALSE);
    }

    /* Macros tree view */
    dialog->macros_store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING, NULL);
    gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->macros_tree),
			     GTK_TREE_MODEL (dialog->macros_store));

    renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "editable", TRUE, NULL);
    g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_PATTERN));
    g_signal_connect (renderer, "edited", G_CALLBACK (macro_edited), mc);

    gtk_tree_view_insert_column_with_attributes (
			GTK_TREE_VIEW (dialog->macros_tree), -1,
			_("Pattern"), renderer,
			"text", COLUMN_PATTERN,
			NULL);

    renderer = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT, "editable", TRUE, NULL);
    g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_COMMAND));
    g_signal_connect (renderer, "edited", G_CALLBACK (macro_edited), mc);

    gtk_tree_view_insert_column_with_attributes (
			GTK_TREE_VIEW (dialog->macros_tree), -1,
			_("Command"), renderer,
			"text", COLUMN_COMMAND,
			NULL);

    show_macros_list (mc);
}

void
mc_show_preferences (GtkAction *action,
		     MCData    *mc)
{
    if (!mc->prefs_dialog.dialog) {
	GtkBuilder *builder;

	builder = gtk_builder_new ();
	gtk_builder_add_from_file (builder, GTK_BUILDERDIR "/mini-commander.ui", NULL);

	mc->prefs_dialog.dialog = GTK_WIDGET (gtk_builder_get_object (builder,
			"mc_preferences_dialog"));

	g_object_add_weak_pointer (G_OBJECT (mc->prefs_dialog.dialog),
				   (gpointer *) &mc->prefs_dialog.dialog);

	mc_preferences_setup_dialog (builder, mc);

	g_object_unref (builder);
    }

    gtk_window_set_screen (GTK_WINDOW (mc->prefs_dialog.dialog),
			   gtk_widget_get_screen (GTK_WIDGET (mc->applet)));
    gtk_window_present (GTK_WINDOW (mc->prefs_dialog.dialog));
}

static MCMacro *
mc_macro_new (const char *pattern,
	      const char *command)
{
    MCMacro *macro;

    g_return_val_if_fail (pattern != NULL, NULL);
    g_return_val_if_fail (command != NULL, NULL);

    macro = g_new0 (MCMacro, 1);
            
    macro->pattern = g_strdup (pattern);
    macro->command = g_strdup (command); 
	    
    if (macro->pattern [0] != '\0')
	regcomp (&macro->regex, macro->pattern, REG_EXTENDED);

    return macro;
}

void
mc_macros_free (GSList *macros)
{
    GSList *l;

    for (l = macros; l; l = l->next) {
	MCMacro *macro = l->data;

	regfree(&macro->regex);
	g_free (macro->pattern);
	g_free (macro->command);
	g_free (macro);
    }

    g_slist_free (macros);
}
	      
static GSList *
mc_load_macros (MCData *mc)
{
    MateConfValue *macro_patterns;
    MateConfValue *macro_commands;
    GSList     *macros_list = NULL;
    MateConfClient *client;
    
    client = mateconf_client_get_default ();
    macro_patterns = mateconf_client_get (client,
		    "/apps/mini-commander/macro_patterns", NULL);
    macro_commands = mateconf_client_get (client,
		    "/apps/mini-commander/macro_commands", NULL);
    
    if (macro_patterns && macro_commands) {
    	GSList *patterns;
	GSList *commands;

        patterns = mateconf_value_get_list (macro_patterns);
        commands = mateconf_value_get_list (macro_commands);

	for (; patterns && commands; patterns = patterns->next, commands = commands->next) {
            MateConfValue *v1 = patterns->data;
            MateConfValue *v2 = commands->data;
	    MCMacro    *macro;
            const char *pattern, *command;
            
            pattern = mateconf_value_get_string (v1);
            command = mateconf_value_get_string (v2);

	    if (!(macro = mc_macro_new (pattern, command)))
		continue;

	    macros_list = g_slist_prepend (macros_list, macro);
        }
    } else {    
	int i;

	for (i = 0; i < G_N_ELEMENTS (mc_default_macros); i++)
	    macros_list = g_slist_prepend (macros_list,
					   mc_macro_new (mc_default_macros [i].pattern,
							 mc_default_macros [i].command));
    }

    macros_list = g_slist_reverse (macros_list);

    if (macro_commands)
	mateconf_value_free (macro_commands);

    if (macro_patterns)
	mateconf_value_free (macro_patterns);

    return macros_list;
}

static void
mc_setup_listeners (MCData *mc)
{
    MateConfClient *client;
    char        *key;
    int          i = 0;

    client = mateconf_client_get_default ();
    mateconf_client_add_dir (client, "/apps/mini-commander",
			  MATECONF_CLIENT_PRELOAD_ONELEVEL, NULL);

    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (mc->applet), "show_default_theme");
    mc->listeners [i++] = mateconf_client_notify_add (
				client, key,
				(MateConfClientNotifyFunc) show_default_theme_changed,
                                mc,
                                NULL, NULL);
    g_free (key);

    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (mc->applet), "autocomplete_history");
    mc->listeners [i++] = mateconf_client_notify_add (
				client, key,
				(MateConfClientNotifyFunc) auto_complete_history_changed,
                                mc,
                                NULL, NULL);
    g_free (key);

    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (mc->applet), "normal_size_x");
    mc->listeners [i++] = mateconf_client_notify_add (
				client, key,
				(MateConfClientNotifyFunc) normal_size_x_changed,
                                mc,
                                NULL, NULL);
    g_free (key);

    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (mc->applet), "normal_size_y");
    mc->listeners [i++] = mateconf_client_notify_add (
				client, key,
				(MateConfClientNotifyFunc) normal_size_y_changed,
                                mc,
                                NULL, NULL);
    g_free (key);

    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (mc->applet), "cmd_line_color_fg_r");
    mc->listeners [i++] = mateconf_client_notify_add (
				client, key,
				(MateConfClientNotifyFunc) cmd_line_color_fg_r_changed,
                                mc,
                                NULL, NULL);
    g_free (key);


    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (mc->applet), "cmd_line_color_fg_g");
    mc->listeners [i++] = mateconf_client_notify_add (
				client, key,
				(MateConfClientNotifyFunc) cmd_line_color_fg_g_changed,
                                mc,
                                NULL, NULL);
    g_free (key);


    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (mc->applet), "cmd_line_color_fg_b");
    mc->listeners [i++] = mateconf_client_notify_add (
				client, key,
				(MateConfClientNotifyFunc) cmd_line_color_fg_b_changed,
                                mc,
                                NULL, NULL);
    g_free (key);

    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (mc->applet), "cmd_line_color_bg_r");
    mc->listeners [i++] = mateconf_client_notify_add (
				client, key,
				(MateConfClientNotifyFunc) cmd_line_color_bg_r_changed,
                                mc,
                                NULL, NULL);
    g_free (key);

    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (mc->applet), "cmd_line_color_bg_g");
    mc->listeners [i++] = mateconf_client_notify_add (
				client, key,
				(MateConfClientNotifyFunc) cmd_line_color_bg_g_changed,
                                mc,
                                NULL, NULL);
    g_free (key);

    key = mate_panel_applet_mateconf_get_full_key (MATE_PANEL_APPLET (mc->applet), "cmd_line_color_bg_b");
    mc->listeners [i++] = mateconf_client_notify_add (
				client, key,
				(MateConfClientNotifyFunc) cmd_line_color_bg_b_changed,
                                mc,
                                NULL, NULL);
    g_free (key);

    mc->listeners [i++] = mateconf_client_notify_add (
				client, "/apps/mini-commander/macro_patterns",
				(MateConfClientNotifyFunc) macros_changed,
                                mc,
                                NULL, NULL);

    mc->listeners [i++] = mateconf_client_notify_add (
				client, "/apps/mini-commander/macro_commands",
				(MateConfClientNotifyFunc) macros_changed,
                                mc,
                                NULL, NULL);

    g_assert (i == MC_NUM_LISTENERS);

    g_object_unref (client);
}

void
mc_load_preferences (MCData *mc)
{
    MateConfValue *history;
    GError     *error = NULL;

    g_return_if_fail (mc != NULL);
    g_return_if_fail (PANEL_IS_APPLET (mc->applet));

    mc->preferences.show_default_theme =
		mate_panel_applet_mateconf_get_bool (mc->applet, "show_default_theme", &error);
    if (error) {
	g_error_free (error);
	error = NULL;
	mc->preferences.show_default_theme = MC_DEFAULT_SHOW_DEFAULT_THEME;
    }

    mc->preferences.auto_complete_history =
		mate_panel_applet_mateconf_get_bool (mc->applet, "autocomplete_history", &error);
    if (error) {
	g_error_free (error);
	error = NULL;
	mc->preferences.auto_complete_history = MC_DEFAULT_AUTO_COMPLETE_HISTORY;
    }

    mc->preferences.normal_size_x =
		mate_panel_applet_mateconf_get_int (mc->applet, "normal_size_x", &error);
    if (error) {
	g_error_free (error);
	error = NULL;
	mc->preferences.normal_size_x = MC_DEFAULT_NORMAL_SIZE_X;
    }
    mc->preferences.normal_size_x = MAX (mc->preferences.normal_size_x, 50);

    mc->preferences.normal_size_y =
		mate_panel_applet_mateconf_get_int (mc->applet, "normal_size_y", &error);
    if (error) {
	g_error_free (error);
	error = NULL;
	mc->preferences.normal_size_y = MC_DEFAULT_NORMAL_SIZE_Y;
    }
    mc->preferences.normal_size_y = CLAMP (mc->preferences.normal_size_y, 5, 200);

    mc->preferences.cmd_line_color_fg_r =
		mate_panel_applet_mateconf_get_int (mc->applet, "cmd_line_color_fg_r", &error);
    if (error) {
	g_error_free (error);
	error = NULL;
	mc->preferences.cmd_line_color_fg_r = MC_DEFAULT_CMD_LINE_COLOR_FG_R;
    }

    mc->preferences.cmd_line_color_fg_g =
		mate_panel_applet_mateconf_get_int (mc->applet, "cmd_line_color_fg_g", &error);
    if (error) {
	g_error_free (error);
	error = NULL;
	mc->preferences.cmd_line_color_fg_g = MC_DEFAULT_CMD_LINE_COLOR_FG_G;
    }

    mc->preferences.cmd_line_color_fg_b =
		mate_panel_applet_mateconf_get_int (mc->applet, "cmd_line_color_fg_b", &error);
    if (error) {
	g_error_free (error);
	error = NULL;
	mc->preferences.cmd_line_color_fg_b = MC_DEFAULT_CMD_LINE_COLOR_FG_B;
    }

    mc->preferences.cmd_line_color_bg_r =
		mate_panel_applet_mateconf_get_int (mc->applet, "cmd_line_color_bg_r", &error);
    if (error) {
	g_error_free (error);
	error = NULL;
	mc->preferences.cmd_line_color_bg_r = MC_DEFAULT_CMD_LINE_COLOR_BG_R;
    }

    mc->preferences.cmd_line_color_bg_g =
		mate_panel_applet_mateconf_get_int (mc->applet, "cmd_line_color_bg_g", &error);
    if (error) {
	g_error_free (error);
	error = NULL;
	mc->preferences.cmd_line_color_bg_g = MC_DEFAULT_CMD_LINE_COLOR_BG_G;
    }

    mc->preferences.cmd_line_color_bg_b =
		mate_panel_applet_mateconf_get_int (mc->applet, "cmd_line_color_bg_b", &error);
    if (error) {
	g_error_free (error);
	error = NULL;
	mc->preferences.cmd_line_color_bg_b = MC_DEFAULT_CMD_LINE_COLOR_BG_B;
    }

    mc->preferences.macros = mc_load_macros (mc);

    history = mate_panel_applet_mateconf_get_value (mc->applet, "history", NULL);
    if (history) {
        GSList *l;

	for (l = mateconf_value_get_list (history); l; l = l->next) {
            const char *entry = NULL;
            
            if ((entry = mateconf_value_get_string (l->data)))
                append_history_entry (mc, entry, TRUE);
        }
	
	mateconf_value_free (history);
    }

    mc_setup_listeners (mc);

    mc->preferences.idle_macros_loader_id = 0;
}
