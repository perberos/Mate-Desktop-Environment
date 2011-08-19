/* Procman - callbacks
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

#include <giomm.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <signal.h>

#include "callbacks.h"
#include "interface.h"
#include "proctable.h"
#include "util.h"
#include "procactions.h"
#include "procdialogs.h"
#include "memmaps.h"
#include "openfiles.h"
#include "load-graph.h"
#include "disks.h"
#include "lsof.h"
#include "sysinfo.h"

void
cb_kill_sigstop(GtkAction *action, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	/* no confirmation */
	kill_process (procdata, SIGSTOP);
}




void
cb_kill_sigcont(GtkAction *action, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	/* no confirmation */
	kill_process (procdata, SIGCONT);
}




static void
kill_process_helper(ProcData *procdata, int sig)
{
	if (procdata->config.show_kill_warning)
		procdialog_create_kill_dialog (procdata, sig);
	else
		kill_process (procdata, sig);
}


void
cb_edit_preferences (GtkAction *action, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	procdialog_create_preferences_dialog (procdata);
}


void
cb_renice (GtkAction *action, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	procdialog_create_renice_dialog (procdata);
}


void
cb_end_process (GtkAction *action, gpointer data)
{
	kill_process_helper(static_cast<ProcData*>(data), SIGTERM);
}


void
cb_kill_process (GtkAction *action, gpointer data)
{
	kill_process_helper(static_cast<ProcData*>(data), SIGKILL);
}


void
cb_show_memory_maps (GtkAction *action, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	create_memmaps_dialog (procdata);
}

void
cb_show_open_files (GtkAction *action, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	
	create_openfiles_dialog (procdata);
}

void
cb_show_lsof(GtkAction *action, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	procman_lsof(procdata);
}


void
cb_about (GtkAction *action, gpointer data)
{
	const gchar * const authors[] = {
		"Kevin Vandersloot",
		"Erik Johnsson",
		"Jorgen Scheibengruber",
		"Benoît Dejean",
		"Paolo Borelli",
		"Karl Lattimer",
		NULL
	};

	const gchar * const documenters[] = {
		"Bill Day",
		"Sun Microsystems",
		NULL
	};

	const gchar * const artists[] = {
		"Baptiste Mille-Mathias",
		NULL
	};

	gtk_show_about_dialog (
		NULL,
		"name",			_("System Monitor"),
		"comments",		_("View current processes and monitor "
					  "system state"),
		"version",		VERSION,
		"copyright",		"Copyright \xc2\xa9 2001-2004 Kevin Vandersloot\n"
					"Copyright \xc2\xa9 2005-2007 Benoît Dejean",
		"logo-icon-name",	"utilities-system-monitor",
		"authors",		authors,
		"artists",		artists,
		"documenters",		documenters,
		"translator-credits",	_("translator-credits"),
		"license",		"GPL 2+",
		"wrap-license",		TRUE,
		NULL
		);
}


void
cb_help_contents (GtkAction *action, gpointer data)
{
  GError* error = 0;
  if (!g_app_info_launch_default_for_uri("ghelp:mate-system-monitor", NULL, &error)) {
    g_warning("Could not display help : %s", error->message);
    g_error_free(error);
  }
}


void
cb_app_exit (GtkAction *action, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	cb_app_delete (NULL, NULL, procdata);
}


gboolean
cb_app_delete (GtkWidget *window, GdkEventAny *event, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	procman_save_config (procdata);
	if (procdata->timeout)
		g_source_remove (procdata->timeout);
	if (procdata->disk_timeout)
		g_source_remove (procdata->disk_timeout);

	gtk_main_quit ();

	return TRUE;
}



void
cb_end_process_button_pressed (GtkButton *button, gpointer data)
{
	kill_process_helper(static_cast<ProcData*>(data), SIGTERM);
}


static void change_mateconf_color(MateConfClient *client, const char *key,
			       GSMColorButton *cp)
{
	GdkColor c;
	char color[24]; /* color should be 1 + 3*4 + 1 = 15 chars -> 24 */

	gsm_color_button_get_color(cp, &c);
	g_snprintf(color, sizeof color, "#%04x%04x%04x", c.red, c.green, c.blue);
	mateconf_client_set_string (client, key, color, NULL);
}


void
cb_cpu_color_changed (GSMColorButton *cp, gpointer data)
{
	char key[80];
	gint i = GPOINTER_TO_INT (data);
	MateConfClient *client = mateconf_client_get_default ();

	g_snprintf(key, sizeof key, "/apps/procman/cpu_color%d", i);

	change_mateconf_color(client, key, cp);
}

void
cb_mem_color_changed (GSMColorButton *cp, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);
	change_mateconf_color(procdata->client, "/apps/procman/mem_color", cp);
}


void
cb_swap_color_changed (GSMColorButton *cp, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);
	change_mateconf_color(procdata->client, "/apps/procman/swap_color", cp);
}

void
cb_net_in_color_changed (GSMColorButton *cp, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);
	change_mateconf_color(procdata->client, "/apps/procman/net_in_color", cp);
}

void
cb_net_out_color_changed (GSMColorButton *cp, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);
	change_mateconf_color(procdata->client, "/apps/procman/net_out_color", cp);
}

static void
get_last_selected (GtkTreeModel *model, GtkTreePath *path,
		   GtkTreeIter *iter, gpointer data)
{
	ProcInfo **info = static_cast<ProcInfo**>(data);

	gtk_tree_model_get (model, iter, COL_POINTER, info, -1);
}


void
cb_row_selected (GtkTreeSelection *selection, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	procdata->selection = selection;

	/* get the most recent selected process and determine if there are
	** no selected processes
	*/
	gtk_tree_selection_selected_foreach (procdata->selection, get_last_selected,
					     &procdata->selected_process);

		update_sensitivity(procdata);
}


gboolean
cb_tree_button_pressed (GtkWidget *widget,
			GdkEventButton *event,
			gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
		do_popup_menu (procdata, event);

	return FALSE;
}


gboolean
cb_tree_popup_menu (GtkWidget *widget, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	do_popup_menu (procdata, NULL);

	return TRUE;
}


void
cb_switch_page (GtkNotebook *nb, GtkNotebookPage *page,
		gint num, gpointer data)
{
	cb_change_current_page (nb, num, data);
}

void
cb_change_current_page (GtkNotebook *nb, gint num, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	procdata->config.current_tab = num;


	if (num == PROCMAN_TAB_PROCESSES) {

		cb_timeout (procdata);

		if (!procdata->timeout)
			procdata->timeout = g_timeout_add (
				procdata->config.update_interval,
				cb_timeout, procdata);

		update_sensitivity(procdata);
	}
	else {
		if (procdata->timeout) {
			g_source_remove (procdata->timeout);
			procdata->timeout = 0;
		}

		update_sensitivity(procdata);
	}


	if (num == PROCMAN_TAB_RESOURCES) {
		load_graph_start (procdata->cpu_graph);
		load_graph_start (procdata->mem_graph);
		load_graph_start (procdata->net_graph);
	}
	else {
		load_graph_stop (procdata->cpu_graph);
		load_graph_stop (procdata->mem_graph);
		load_graph_stop (procdata->net_graph);
	}


	if (num == PROCMAN_TAB_DISKS) {

		cb_update_disks (procdata);

		if(!procdata->disk_timeout) {
			procdata->disk_timeout =
				g_timeout_add (procdata->config.disks_update_interval,
					       cb_update_disks,
					       procdata);
		}
	}
	else {
		if(procdata->disk_timeout) {
			g_source_remove (procdata->disk_timeout);
			procdata->disk_timeout = 0;
		}
	}

	if (num == PROCMAN_TAB_SYSINFO) {
		procman::build_sysinfo_ui();
	}
}



gint
cb_user_refresh (GtkAction*, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);
	proctable_update_all(procdata);
	return FALSE;
}


gint
cb_timeout (gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);
	guint new_interval;

	proctable_update_all (procdata);

	if (procdata->smooth_refresh->get(new_interval))
	{
		procdata->timeout = g_timeout_add(new_interval,
						  cb_timeout,
						  procdata);
		return FALSE;
	}

	return TRUE;
}


void
cb_radio_processes(GtkAction *action, GtkRadioAction *current, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);

	procdata->config.whose_process = gtk_radio_action_get_current_value(current);

	mateconf_client_set_int (procdata->client, "/apps/procman/view_as",
			      procdata->config.whose_process, NULL);
}
