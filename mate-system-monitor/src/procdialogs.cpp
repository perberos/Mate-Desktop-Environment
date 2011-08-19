/* Procman - dialogs
 * Copyright (C) 2001 Kevin Vandersloot
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 */


#include <config.h>

#include <glib/gi18n.h>

#include <signal.h>
#include <string.h>
#include "procdialogs.h"
#include "proctable.h"
#include "callbacks.h"
#include "prettytable.h"
#include "procactions.h"
#include "util.h"
#include "load-graph.h"
#include "mateconf-keys.h"
#include "procman_matesu.h"
#include "procman_gksu.h"

static GtkWidget *renice_dialog = NULL;
static GtkWidget *prefs_dialog = NULL;
static gint new_nice_value = 0;


static void
kill_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
	struct KillArgs *kargs = static_cast<KillArgs*>(data);
	
	gtk_widget_destroy (GTK_WIDGET (dialog));

	if (id == GTK_RESPONSE_OK) 
		kill_process (kargs->procdata, kargs->signal);

	g_free (kargs);
}

void
procdialog_create_kill_dialog (ProcData *procdata, int signal)
{
	GtkWidget *kill_alert_dialog;
	gchar *primary, *secondary, *button_text;
	struct KillArgs *kargs;

	kargs = g_new(KillArgs, 1);
	kargs->procdata = procdata;
	kargs->signal = signal;

  	
  	if (signal == SIGKILL) {
                /*xgettext: primary alert message*/
  		primary = _("Kill the selected process?");
		/*xgettext: secondary alert message*/
		secondary = _("Killing a process may destroy data, break the "
			      "session or introduce a security risk. "
			      "Only unresponding processes should be killed.");
  		button_text = _("_Kill Process");
		  	}
  	else {
                /*xgettext: primary alert message*/
                primary = _("End the selected process?");
	        /*xgettext: secondary alert message*/
		secondary = _("Ending a process may destroy data, break the "
			      "session or introduce a security risk. "
			      "Only unresponding processes should be ended.");
                button_text = _("_End Process");
  	}

	kill_alert_dialog = gtk_message_dialog_new (GTK_WINDOW (procdata->app),
						    static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
						    GTK_MESSAGE_WARNING,
						    GTK_BUTTONS_NONE,
						    "%s",
						    primary);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (kill_alert_dialog),
						  "%s",
						  secondary);

	gtk_dialog_add_buttons (GTK_DIALOG (kill_alert_dialog),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				button_text, GTK_RESPONSE_OK,
				NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (kill_alert_dialog),
					 GTK_RESPONSE_CANCEL);

        g_signal_connect (G_OBJECT (kill_alert_dialog), "response",
        		  G_CALLBACK (kill_dialog_button_pressed), kargs);
        
        gtk_widget_show_all (kill_alert_dialog);
}

static gchar *
get_nice_level (gint nice)
{
	if (nice < -7)
		return _("(Very High Priority)");
	else if (nice < -2)
		return _("(High Priority)");
	else if (nice < 3)
		return _("(Normal Priority)");
	else if (nice < 7)
		return _("(Low Priority)");
	else
		return _("(Very Low Priority)");
}

static void
renice_scale_changed (GtkAdjustment *adj, gpointer data)
{
	GtkWidget *label = GTK_WIDGET (data);
	
	new_nice_value = int(gtk_adjustment_get_value (adj));
	gtk_label_set_text (GTK_LABEL (label), get_nice_level (new_nice_value));		
	
}

static void
renice_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	
	if (id == 100) {
		if (new_nice_value == -100)
			return;		
		renice(procdata, new_nice_value);
	}
	
	gtk_widget_destroy (GTK_WIDGET (dialog));
	renice_dialog = NULL;
}

void
procdialog_create_renice_dialog (ProcData *procdata)
{
	ProcInfo *info = procdata->selected_process;
	GtkWidget *dialog = NULL;
	GtkWidget *dialog_vbox;
	GtkWidget *vbox;
	GtkWidget *hbox;
  	GtkWidget *label;
  	GtkWidget *priority_label;
  	GtkWidget *table;
  	GtkObject *renice_adj;
  	GtkWidget *hscale;
	GtkWidget *button;
	GtkWidget *align;
	GtkWidget *icon;
  	gchar *text;

	if (renice_dialog)
		return;
		
	if (!info)
		return;
		
	dialog = gtk_dialog_new_with_buttons (_("Change Priority"), NULL,
				              GTK_DIALOG_DESTROY_WITH_PARENT,
				              GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				              NULL);
  	renice_dialog = dialog;
	gtk_window_set_resizable (GTK_WINDOW (renice_dialog), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (renice_dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (renice_dialog), 5);
  	
	button = gtk_button_new ();
	gtk_widget_set_can_default (button, TRUE);
		
	align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (button), align);
		
	hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (align), hbox);

	icon = gtk_image_new_from_stock (GTK_STOCK_OK, GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX (hbox), icon, FALSE, FALSE, 0);

	label = gtk_label_new_with_mnemonic (_("Change _Priority"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	gtk_dialog_add_action_widget (GTK_DIALOG (renice_dialog), button, 100);
  	gtk_dialog_set_default_response (GTK_DIALOG (renice_dialog), 100);
  	new_nice_value = -100;
  	  
	dialog_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	gtk_box_set_spacing (GTK_BOX (dialog_vbox), 2);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox), 5);
    	    	
    	vbox = gtk_vbox_new (FALSE, 12);
    	gtk_box_pack_start (GTK_BOX (dialog_vbox), vbox, TRUE, TRUE, 0);
    	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
    	
    	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE(table), 12);
	gtk_table_set_row_spacings (GTK_TABLE(table), 6);
	gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);
	
	label = gtk_label_new_with_mnemonic (_("_Nice value:"));
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 2,
			  GTK_FILL, GTK_FILL, 0, 0);
	
	renice_adj = gtk_adjustment_new (info->nice, RENICE_VAL_MIN, RENICE_VAL_MAX, 1, 1, 0);
	new_nice_value = 0;
	hscale = gtk_hscale_new (GTK_ADJUSTMENT (renice_adj));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), hscale);
	gtk_scale_set_digits (GTK_SCALE (hscale), 0);
	gtk_table_attach (GTK_TABLE (table), hscale, 1, 2, 0, 1,
			  static_cast<GtkAttachOptions>(GTK_EXPAND | GTK_FILL), GTK_FILL, 0, 0);
			  
	priority_label = gtk_label_new (get_nice_level (info->nice));
	gtk_table_attach (GTK_TABLE (table), priority_label, 1, 2, 1, 2,
			  GTK_FILL, GTK_FILL, 0, 0);
	
	text = g_strconcat("<small><i><b>", _("Note:"), "</b> ", 
	    _("The priority of a process is given by its nice value. A lower nice value corresponds to a higher priority."),
	    "</i></small>", NULL); 
	label = gtk_label_new (_(text));
    	gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
    	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	g_free (text);
	
	g_signal_connect (G_OBJECT (dialog), "response",
  			  G_CALLBACK (renice_dialog_button_pressed), procdata);
  	g_signal_connect (G_OBJECT (renice_adj), "value_changed",
  			    G_CALLBACK (renice_scale_changed), priority_label);
  	
    	gtk_widget_show_all (dialog);
    	
    	
}

static void
prefs_dialog_button_pressed (GtkDialog *dialog, gint id, gpointer data)
{
	gtk_widget_destroy (GTK_WIDGET (dialog));
	
	prefs_dialog = NULL;
}


static void
show_kill_dialog_toggled (GtkToggleButton *button, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	MateConfClient *client = procdata->client;
	
	gboolean toggled;
	
	toggled = gtk_toggle_button_get_active (button);
	
	mateconf_client_set_bool (client, "/apps/procman/kill_dialog", toggled, NULL);
		
}



static void
solaris_mode_toggled(GtkToggleButton *button, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	MateConfClient *client = procdata->client;
	gboolean toggled;
	toggled = gtk_toggle_button_get_active(button);
	mateconf_client_set_bool(client, procman::mateconf::solaris_mode.c_str(), toggled, NULL);
}


static void
network_in_bits_toggled(GtkToggleButton *button, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	MateConfClient *client = procdata->client;
	gboolean toggled;
	toggled = gtk_toggle_button_get_active(button);
	mateconf_client_set_bool(client, procman::mateconf::network_in_bits.c_str(), toggled, NULL);
}



static void
smooth_refresh_toggled(GtkToggleButton *button, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	MateConfClient *client = procdata->client;

	gboolean toggled;

	toggled = gtk_toggle_button_get_active(button);

	mateconf_client_set_bool(client, SmoothRefresh::KEY.c_str(), toggled, NULL);
}



static void
show_all_fs_toggled (GtkToggleButton *button, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	MateConfClient *client = procdata->client;

	gboolean toggled;

	toggled = gtk_toggle_button_get_active (button);

	mateconf_client_set_bool (client, "/apps/procman/show_all_fs", toggled, NULL);
}


class SpinButtonUpdater
{
public:
  SpinButtonUpdater(const string& mateconf_key)
    : mateconf_key(mateconf_key)
  { }

  static gboolean callback(GtkWidget *widget, GdkEventFocus *event, gpointer data)
  {
    SpinButtonUpdater* updater = static_cast<SpinButtonUpdater*>(data);
    updater->update(GTK_SPIN_BUTTON(widget));
    return FALSE;
  }

private:

  void update(GtkSpinButton* spin)
  {
    int new_value = int(1000 * gtk_spin_button_get_value(spin));
    GError* e = 0;

    if (not mateconf_client_set_int(ProcData::get_instance()->client,
				 this->mateconf_key.c_str(), new_value,
				 &e)) {
      g_warning("Failed to mateconf_client_set_int %s %d : %s\n",
		this->mateconf_key.c_str(), new_value, e->message);
      g_error_free(e);
    }

    procman_debug("set %s to %d", this->mateconf_key.c_str(), new_value);
  }

  const string mateconf_key;
};




static void
field_toggled (GtkCellRendererToggle *cell, gchar *path_str, gpointer data)
{
	GtkTreeModel *model = static_cast<GtkTreeModel*>(data);
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  	GtkTreeIter iter;
  	GtkTreeViewColumn *column;
  	gboolean toggled;
	
	if (!path)
		return;
	
	gtk_tree_model_get_iter (model, &iter, path);
	
	gtk_tree_model_get (model, &iter, 2, &column, -1);
	toggled = gtk_cell_renderer_toggle_get_active (cell);
	
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, !toggled, -1);
	gtk_tree_view_column_set_visible (column, !toggled);
	
	gtk_tree_path_free (path);

}

static GtkWidget *
create_field_page(GtkWidget *tree, const char* text)
{
	GtkWidget *vbox;
	GtkWidget *scrolled;
	GtkWidget *label;
	GtkWidget *treeview;
	GList *it, *columns;
	GtkListStore *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;

	vbox = gtk_vbox_new (FALSE, 6);
	
	label = gtk_label_new_with_mnemonic (text);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 0);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  	GTK_POLICY_AUTOMATIC,
                                  	GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
        gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);
        
        model = gtk_list_store_new (3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_POINTER);	
        
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	gtk_container_add (GTK_CONTAINER (scrolled), treeview);
	g_object_unref (G_OBJECT (model));
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), treeview);
	
	column = gtk_tree_view_column_new ();
	
	cell = gtk_cell_renderer_toggle_new ();
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	gtk_tree_view_column_set_attributes (column, cell,
					                       "active", 0,
					                       NULL);
	g_signal_connect (G_OBJECT (cell), "toggled", G_CALLBACK (field_toggled), model);
	gtk_tree_view_column_set_clickable (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	
	column = gtk_tree_view_column_new ();
	
	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	gtk_tree_view_column_set_attributes (column, cell,
					                       "text", 1,
					                        NULL);
					                        
	gtk_tree_view_column_set_title (column, "Not Shown");
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
	
	columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));
	
	for(it = columns; it; it = it->next)
	{
		GtkTreeViewColumn *column = static_cast<GtkTreeViewColumn*>(it->data);
		GtkTreeIter iter;
		const gchar *title;
		gboolean visible;
		
		title = gtk_tree_view_column_get_title (column);
		if (!title) 
			title = _("Icon");
		
		visible = gtk_tree_view_column_get_visible (column);
		
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter, 0, visible, 1, title, 2, column,-1);
	}

	g_list_free(columns);

	return vbox;
}

void
procdialog_create_preferences_dialog (ProcData *procdata)
{
	static GtkWidget *dialog = NULL;

	typedef SpinButtonUpdater SBU;

	static SBU interval_updater("/apps/procman/update_interval");
	static SBU graph_interval_updater("/apps/procman/graph_update_interval");
	static SBU disks_interval_updater("/apps/procman/disks_interval");

	GtkWidget *notebook;
	GtkWidget *proc_box;
	GtkWidget *sys_box;
	GtkWidget *main_vbox;
	GtkWidget *vbox, *vbox2, *vbox3;
	GtkWidget *hbox, *hbox2, *hbox3;
	GtkWidget *label;
	GtkAdjustment *adjustment;
	GtkWidget *spin_button;
	GtkWidget *check_button;
	GtkWidget *tab_label;
	GtkWidget *smooth_button;
	GtkSizeGroup *size;
	gfloat update;
	gchar *tmp;
	
	if (prefs_dialog)
		return;
		
	size = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	
	dialog = gtk_dialog_new_with_buttons (_("System Monitor Preferences"),
					      GTK_WINDOW (procdata->app),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					      NULL);
	/* FIXME: we should not declare the window size, but let it's   */
	/* driven by window childs. The problem is that the fields list */
	/* have to show at least 4 items to respect HIG. I don't know   */
	/* any function to set list height by contents/items inside it. */
	gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 420);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	prefs_dialog = dialog;
	
	main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	gtk_box_set_spacing (GTK_BOX (main_vbox), 2);
	
	notebook = gtk_notebook_new ();
	gtk_container_set_border_width (GTK_CONTAINER (notebook), 5);
	gtk_box_pack_start (GTK_BOX (main_vbox), notebook, TRUE, TRUE, 0);
	
	proc_box = gtk_vbox_new (FALSE, 18);
	gtk_container_set_border_width (GTK_CONTAINER (proc_box), 12);
	tab_label = gtk_label_new (_("Processes"));
	gtk_widget_show (tab_label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), proc_box, tab_label);
	
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (proc_box), vbox, FALSE, FALSE, 0);
	
	tmp = g_strdup_printf ("<b>%s</b>", _("Behavior"));
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), tmp);
	g_free (tmp);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	vbox2 = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
			  
	hbox2 = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);
	
	label = gtk_label_new_with_mnemonic (_("_Update interval in seconds:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
	
	hbox3 = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox2), hbox3, TRUE, TRUE, 0);
	
	update = (gfloat) procdata->config.update_interval;
	adjustment = (GtkAdjustment *) gtk_adjustment_new(update / 1000.0,
							  MIN_UPDATE_INTERVAL / 1000,
							  MAX_UPDATE_INTERVAL / 1000,
							  0.25,
							  1.0,
							  1.0);

	spin_button = gtk_spin_button_new (adjustment, 1.0, 2);
	gtk_box_pack_start (GTK_BOX (hbox3), spin_button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
				   G_CALLBACK (SBU::callback), &interval_updater);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), spin_button);
	

	hbox2 = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox2, FALSE, FALSE, 0);

	smooth_button = gtk_check_button_new_with_mnemonic(_("Enable _smooth refresh"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(smooth_button),
				     mateconf_client_get_bool(procdata->client,
							   SmoothRefresh::KEY.c_str(),
							   NULL));
	g_signal_connect(G_OBJECT(smooth_button), "toggled",
			 G_CALLBACK(smooth_refresh_toggled), procdata);
	gtk_box_pack_start(GTK_BOX(hbox2), smooth_button, TRUE, TRUE, 0);



	hbox2 = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);
		
	check_button = gtk_check_button_new_with_mnemonic (_("Alert before ending or _killing processes"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button), 
				      procdata->config.show_kill_warning);
	g_signal_connect (G_OBJECT (check_button), "toggled",
			    G_CALLBACK (show_kill_dialog_toggled), procdata);
	gtk_box_pack_start (GTK_BOX (hbox2), check_button, FALSE, FALSE, 0);
	



	hbox2 = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox2, FALSE, FALSE, 0);

	GtkWidget *solaris_button;
	solaris_button = gtk_check_button_new_with_mnemonic(_("Solaris mode"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(solaris_button),
				     mateconf_client_get_bool(procdata->client,
							   procman::mateconf::solaris_mode.c_str(),
							   NULL));
	g_signal_connect(G_OBJECT(solaris_button), "toggled",
			 G_CALLBACK(solaris_mode_toggled), procdata);
	gtk_box_pack_start(GTK_BOX(hbox2), solaris_button, TRUE, TRUE, 0);




	hbox2 = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);
	
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (proc_box), vbox, TRUE, TRUE, 0);
	
	tmp = g_strdup_printf ("<b>%s</b>", _("Information Fields"));
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), tmp);
	g_free (tmp);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	vbox2 = create_field_page (procdata->tree, _("Process i_nformation shown in list:"));
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
	
	sys_box = gtk_vbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (sys_box), 12);
	tab_label = gtk_label_new (_("Resources"));
	gtk_widget_show (tab_label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), sys_box, tab_label);
	
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (sys_box), vbox, FALSE, FALSE, 0);
	
	tmp = g_strdup_printf ("<b>%s</b>", _("Graphs"));
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), tmp);
	g_free (tmp);
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	vbox2 = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
	
	hbox2 = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);
		
	label = gtk_label_new_with_mnemonic (_("_Update interval in seconds:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
	gtk_size_group_add_widget (size, label);
	
	hbox3 = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox2), hbox3, TRUE, TRUE, 0);
			  
	update = (gfloat) procdata->config.graph_update_interval;
	adjustment = (GtkAdjustment *) gtk_adjustment_new(update / 1000.0, 0.25, 
							  100.0, 0.25, 1.0, 1.0);
	spin_button = gtk_spin_button_new (adjustment, 1.0, 2);
	g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
			  G_CALLBACK(SBU::callback),
			  &graph_interval_updater);
	gtk_box_pack_start (GTK_BOX (hbox3), spin_button, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), spin_button);
	

	GtkWidget *bits_button;
	bits_button = gtk_check_button_new_with_mnemonic(_("Show network speed in bits"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bits_button),
				     mateconf_client_get_bool(procdata->client,
							   procman::mateconf::network_in_bits.c_str(),
							   NULL));
	g_signal_connect(G_OBJECT(bits_button), "toggled",
			 G_CALLBACK(network_in_bits_toggled), procdata);
	gtk_box_pack_start(GTK_BOX(vbox2), bits_button, TRUE, TRUE, 0);


	
	hbox2 = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);
	
	/*
	 * Devices
	 */
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
	tab_label = gtk_label_new (_("File Systems"));
	gtk_widget_show (tab_label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, tab_label);
	
	tmp = g_strdup_printf ("<b>%s</b>", _("File Systems"));
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), tmp);
	g_free (tmp);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	vbox2 = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);
	
	hbox2 = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);
	
	label = gtk_label_new_with_mnemonic (_("_Update interval in seconds:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
	
	hbox3 = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox2), hbox3, TRUE, TRUE, 0);
			  
	update = (gfloat) procdata->config.disks_update_interval;
	adjustment = (GtkAdjustment *) gtk_adjustment_new (update / 1000.0, 1.0, 
							   100.0, 1.0, 1.0, 1.0);
	spin_button = gtk_spin_button_new (adjustment, 1.0, 0);
	gtk_box_pack_start (GTK_BOX (hbox3), spin_button, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), spin_button);
	g_signal_connect (G_OBJECT (spin_button), "focus_out_event",
			  G_CALLBACK(SBU::callback),
			  &disks_interval_updater);
		

	hbox2 = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);
	check_button = gtk_check_button_new_with_mnemonic (_("Show _all filesystems"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
				      procdata->config.show_all_fs);
	g_signal_connect (G_OBJECT (check_button), "toggled",
			    G_CALLBACK (show_all_fs_toggled), procdata);
	gtk_box_pack_start (GTK_BOX (hbox2), check_button, FALSE, FALSE, 0);


	vbox2 = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), vbox2, FALSE, FALSE, 0);

	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (vbox2), label, FALSE, FALSE, 0);
	
	tmp = g_strdup_printf ("<b>%s</b>", _("Information Fields"));
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL (label), tmp);
	g_free (tmp);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	
	vbox3 = create_field_page (procdata->disk_list, _("File system i_nformation shown in list:"));
	gtk_box_pack_start (GTK_BOX (hbox), vbox3, TRUE, TRUE, 0);

	gtk_widget_show_all (dialog);
	g_signal_connect (G_OBJECT (dialog), "response",
				  G_CALLBACK (prefs_dialog_button_pressed), procdata);
	
	switch (procdata->config.current_tab) {
	case PROCMAN_TAB_SYSINFO:
	case PROCMAN_TAB_PROCESSES:
		gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);
		break;
	case PROCMAN_TAB_RESOURCES:
		gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 1);
		break;
	case PROCMAN_TAB_DISKS:
		gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 2);
		break;

	}
}



static char *
procman_action_to_command(ProcmanActionType type,
			  gint pid,
			  gint extra_value)
{
	switch (type) {
	case PROCMAN_ACTION_KILL:
	       return g_strdup_printf("kill -s %d %d", extra_value, pid);
	case PROCMAN_ACTION_RENICE:
		return g_strdup_printf("renice %d %d", extra_value, pid);
	default:
		g_assert_not_reached();
	}
}


/*
 * type determines whether if dialog is for killing process or renice.
 * type == PROCMAN_ACTION_KILL,   extra_value -> signal to send
 * type == PROCMAN_ACTION_RENICE, extra_value -> new priority.
 */
gboolean
procdialog_create_root_password_dialog(ProcmanActionType type,
				       ProcData *procdata,
				       gint pid,
				       gint extra_value)
{
	char * command;
	gboolean ret = FALSE;

	command = procman_action_to_command(type, pid, extra_value);

	procman_debug("Trying to run '%s' as root", command);

	if (procman_has_gksu())
		ret = procman_gksu_create_root_password_dialog(command);
	else if (procman_has_matesu())
		ret = procman_matesu_create_root_password_dialog(command);

	g_free(command);
	return ret;
}


