/* Procman
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

#include <stdlib.h>

#include <locale.h>

#include <gtkmm/main.h>
#include <giomm/volumemonitor.h>
#include <giomm/init.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <bacon-message-connection.h>
#include <mateconf/mateconf-client.h>
#include <glibtop.h>
#include <glibtop/close.h>
#include <glibtop/loadavg.h>

#include "load-graph.h"
#include "procman.h"
#include "interface.h"
#include "proctable.h"
#include "prettytable.h"
#include "callbacks.h"
#include "smooth_refresh.h"
#include "util.h"
#include "mateconf-keys.h"
#include "argv.h"


ProcData::ProcData()
  : tree(NULL),
    cpu_graph(NULL),
    mem_graph(NULL),
    net_graph(NULL),
    selected_process(NULL),
    timeout(0),
    disk_timeout(0),
    cpu_total_time(1),
    cpu_total_time_last(1)
{ }


ProcData* ProcData::get_instance()
{
  static ProcData instance;
  return &instance;
}


static void
tree_changed_cb (MateConfClient *client, guint id, MateConfEntry *entry, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	MateConfValue *value = mateconf_entry_get_value (entry);
	
	procdata->config.show_tree = mateconf_value_get_bool (value);

	g_object_set(G_OBJECT(procdata->tree),
		     "show-expanders", procdata->config.show_tree,
		     NULL);

	proctable_clear_tree (procdata);

	proctable_update_all (procdata);
}

static void
solaris_mode_changed_cb(MateConfClient *client, guint id, MateConfEntry *entry, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	MateConfValue *value = mateconf_entry_get_value (entry);

	procdata->config.solaris_mode = mateconf_value_get_bool(value);
	proctable_update_all (procdata);
}


static void
network_in_bits_changed_cb(MateConfClient *client, guint id, MateConfEntry *entry, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	MateConfValue *value = mateconf_entry_get_value (entry);

	procdata->config.network_in_bits = mateconf_value_get_bool(value);
	// force scale to be redrawn
	procdata->net_graph->clear_background();
}



static void
view_as_changed_cb (MateConfClient *client, guint id, MateConfEntry *entry, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	MateConfValue *value = mateconf_entry_get_value (entry);
	
	procdata->config.whose_process = mateconf_value_get_int (value);
	procdata->config.whose_process = CLAMP (procdata->config.whose_process, 0, 2);
	proctable_clear_tree (procdata);
	proctable_update_all (procdata);
	
}

static void
warning_changed_cb (MateConfClient *client, guint id, MateConfEntry *entry, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	const gchar *key = mateconf_entry_get_key (entry);
	MateConfValue *value = mateconf_entry_get_value (entry);
	
	if (g_str_equal (key, "/apps/procman/kill_dialog")) {
		procdata->config.show_kill_warning = mateconf_value_get_bool (value);
	}
}

static void
timeouts_changed_cb (MateConfClient *client, guint id, MateConfEntry *entry, gpointer data)
{
	ProcData *procdata = static_cast<ProcData*>(data);
	const gchar *key = mateconf_entry_get_key (entry);
	MateConfValue *value = mateconf_entry_get_value (entry);

	if (g_str_equal (key, "/apps/procman/update_interval")) {
		procdata->config.update_interval = mateconf_value_get_int (value);
		procdata->config.update_interval = 
			MAX (procdata->config.update_interval, 1000);

		procdata->smooth_refresh->reset();

		if(procdata->timeout) {
			g_source_remove (procdata->timeout);
			procdata->timeout = g_timeout_add (procdata->config.update_interval,
							   cb_timeout,
							   procdata);
		}
	}
	else if (g_str_equal (key, "/apps/procman/graph_update_interval")){
		procdata->config.graph_update_interval = mateconf_value_get_int (value);
		procdata->config.graph_update_interval = 
			MAX (procdata->config.graph_update_interval, 
			     250);
		load_graph_change_speed(procdata->cpu_graph,
					procdata->config.graph_update_interval);
		load_graph_change_speed(procdata->mem_graph,
					procdata->config.graph_update_interval);
		load_graph_change_speed(procdata->net_graph,
					procdata->config.graph_update_interval);
	}
	else if (g_str_equal(key, "/apps/procman/disks_interval")) {
		
		procdata->config.disks_update_interval = mateconf_value_get_int (value);
		procdata->config.disks_update_interval = 
			MAX (procdata->config.disks_update_interval, 1000);	

		if(procdata->disk_timeout) {
			g_source_remove (procdata->disk_timeout);
			procdata->disk_timeout = \
				g_timeout_add (procdata->config.disks_update_interval,
					       cb_update_disks,
					       procdata);
		}
	}
	else {
		g_assert_not_reached();
	}
}

static void
color_changed_cb (MateConfClient *client, guint id, MateConfEntry *entry, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);
	const gchar *key = mateconf_entry_get_key (entry);
	MateConfValue *value = mateconf_entry_get_value (entry);
	const gchar *color = mateconf_value_get_string (value);

	if (g_str_has_prefix (key, "/apps/procman/cpu_color")) {
		for (int i = 0; i < GLIBTOP_NCPU; i++) {
			string cpu_key = make_string(g_strdup_printf("/apps/procman/cpu_color%d", i));
			if (cpu_key == key) {
				gdk_color_parse (color, &procdata->config.cpu_color[i]);
				procdata->cpu_graph->colors.at(i) = procdata->config.cpu_color[i];
				break;
			}
		}
	}
	else if (g_str_equal (key, "/apps/procman/mem_color")) {
		gdk_color_parse (color, &procdata->config.mem_color);
		procdata->mem_graph->colors.at(0) = procdata->config.mem_color;
	}
	else if (g_str_equal (key, "/apps/procman/swap_color")) {
		gdk_color_parse (color, &procdata->config.swap_color);
		procdata->mem_graph->colors.at(1) = procdata->config.swap_color;
	}
	else if (g_str_equal (key, "/apps/procman/net_in_color")) {
		gdk_color_parse (color, &procdata->config.net_in_color);
		procdata->net_graph->colors.at(0) = procdata->config.net_in_color;
	}
	else if (g_str_equal (key, "/apps/procman/net_out_color")) {
		gdk_color_parse (color, &procdata->config.net_out_color);
		procdata->net_graph->colors.at(1) = procdata->config.net_out_color;
	}
	else {
		g_assert_not_reached();
	}
}



static void
show_all_fs_changed_cb (MateConfClient *client, guint id, MateConfEntry *entry, gpointer data)
{
	ProcData * const procdata = static_cast<ProcData*>(data);
	MateConfValue *value = mateconf_entry_get_value (entry);

	procdata->config.show_all_fs = mateconf_value_get_bool (value);

	cb_update_disks (data);
}


static ProcData *
procman_data_new (MateConfClient *client)
{

	ProcData *pd;
	gchar *color;
	gint swidth, sheight;
	gint i;
	glibtop_cpu cpu;

	pd = ProcData::get_instance();

	pd->config.width = mateconf_client_get_int (client, "/apps/procman/width", NULL);
	pd->config.height = mateconf_client_get_int (client, "/apps/procman/height", NULL);
	pd->config.show_tree = mateconf_client_get_bool (client, "/apps/procman/show_tree", NULL);
	mateconf_client_notify_add (client, "/apps/procman/show_tree", tree_changed_cb,
				 pd, NULL, NULL);

	pd->config.solaris_mode = mateconf_client_get_bool(client, procman::mateconf::solaris_mode.c_str(), NULL);
	mateconf_client_notify_add(client, procman::mateconf::solaris_mode.c_str(), solaris_mode_changed_cb, pd, NULL, NULL);

	pd->config.network_in_bits = mateconf_client_get_bool(client, procman::mateconf::network_in_bits.c_str(), NULL);
	mateconf_client_notify_add(client, procman::mateconf::network_in_bits.c_str(), network_in_bits_changed_cb, pd, NULL, NULL);


	pd->config.show_kill_warning = mateconf_client_get_bool (client, "/apps/procman/kill_dialog", 
							      NULL);
	mateconf_client_notify_add (client, "/apps/procman/kill_dialog", warning_changed_cb,
				 pd, NULL, NULL);
	pd->config.update_interval = mateconf_client_get_int (client, "/apps/procman/update_interval", 
							   NULL);
	mateconf_client_notify_add (client, "/apps/procman/update_interval", timeouts_changed_cb,
				 pd, NULL, NULL);
	pd->config.graph_update_interval = mateconf_client_get_int (client,
						"/apps/procman/graph_update_interval",
						NULL);
	mateconf_client_notify_add (client, "/apps/procman/graph_update_interval", timeouts_changed_cb,
				 pd, NULL, NULL);
	pd->config.disks_update_interval = mateconf_client_get_int (client,
								 "/apps/procman/disks_interval",
								 NULL);
	mateconf_client_notify_add (client, "/apps/procman/disks_interval", timeouts_changed_cb,
				 pd, NULL, NULL);


	/* /apps/procman/show_all_fs */
	pd->config.show_all_fs = mateconf_client_get_bool (
		client, "/apps/procman/show_all_fs",
		NULL);
	mateconf_client_notify_add
		(client, "/apps/procman/show_all_fs",
		 show_all_fs_changed_cb, pd, NULL, NULL);


	pd->config.whose_process = mateconf_client_get_int (client, "/apps/procman/view_as", NULL);
	mateconf_client_notify_add (client, "/apps/procman/view_as", view_as_changed_cb,
				 pd, NULL, NULL);
	pd->config.current_tab = mateconf_client_get_int (client, "/apps/procman/current_tab", NULL);

	for (int i = 0; i < GLIBTOP_NCPU; i++) {
		gchar *key;
		key = g_strdup_printf ("/apps/procman/cpu_color%d", i);
		
		color = mateconf_client_get_string (client, key, NULL);
		if (!color)
			color = g_strdup ("#f25915e815e8");
		mateconf_client_notify_add (client, key, 
			  	 color_changed_cb, pd, NULL, NULL);
		gdk_color_parse(color, &pd->config.cpu_color[i]);
		g_free (color);
		g_free (key);
	}
	color = mateconf_client_get_string (client, "/apps/procman/mem_color", NULL);
	if (!color)
		color = g_strdup ("#000000ff0082");
	mateconf_client_notify_add (client, "/apps/procman/mem_color", 
			  	 color_changed_cb, pd, NULL, NULL);
	gdk_color_parse(color, &pd->config.mem_color);
	
	g_free (color);
	
	color = mateconf_client_get_string (client, "/apps/procman/swap_color", NULL);
	if (!color)
		color = g_strdup ("#00b6000000ff");
	mateconf_client_notify_add (client, "/apps/procman/swap_color", 
			  	 color_changed_cb, pd, NULL, NULL);
	gdk_color_parse(color, &pd->config.swap_color);
	g_free (color);

	color = mateconf_client_get_string (client, "/apps/procman/net_in_color", NULL);
	if (!color)
		color = g_strdup ("#000000f200f2");
	mateconf_client_notify_add (client, "/apps/procman/net_in_color",
			  	 color_changed_cb, pd, NULL, NULL);
	gdk_color_parse(color, &pd->config.net_in_color);
	g_free (color);

	color = mateconf_client_get_string (client, "/apps/procman/net_out_color", NULL);
	if (!color)
		color = g_strdup ("#00f2000000c1");
	mateconf_client_notify_add (client, "/apps/procman/net_out_color",
			  	 color_changed_cb, pd, NULL, NULL);
	gdk_color_parse(color, &pd->config.net_out_color);
	g_free (color);
	
	/* Sanity checks */
	swidth = gdk_screen_width ();
	sheight = gdk_screen_height ();
	pd->config.width = CLAMP (pd->config.width, 50, swidth);
	pd->config.height = CLAMP (pd->config.height, 50, sheight);
	pd->config.update_interval = MAX (pd->config.update_interval, 1000);
	pd->config.graph_update_interval = MAX (pd->config.graph_update_interval, 250);
	pd->config.disks_update_interval = MAX (pd->config.disks_update_interval, 1000);
	pd->config.whose_process = CLAMP (pd->config.whose_process, 0, 2);
	pd->config.current_tab = CLAMP(pd->config.current_tab,
				       PROCMAN_TAB_SYSINFO,
				       PROCMAN_TAB_DISKS);
	
	/* Determinie number of cpus since libgtop doesn't really tell you*/
	pd->config.num_cpus = 0;
	glibtop_get_cpu (&cpu);
	pd->frequency = cpu.frequency;
	i=0;
    	while (i < GLIBTOP_NCPU && cpu.xcpu_total[i] != 0) {
    	    pd->config.num_cpus ++;
    	    i++;
    	}
    	if (pd->config.num_cpus == 0)
    		pd->config.num_cpus = 1;

	// delayed initialization as SmoothRefresh() needs ProcData
	// i.e. we can't call ProcData::get_instance
	pd->smooth_refresh = new SmoothRefresh();

	return pd;

}

static void
procman_free_data (ProcData *procdata)
{

	proctable_free_table (procdata);
	delete procdata->smooth_refresh;
}


gboolean
procman_get_tree_state (MateConfClient *client, GtkWidget *tree, const gchar *prefix)
{
	GtkTreeModel *model;
	GList *columns, *it;
	gint sort_col;
	GtkSortType order;
	gchar *key;
	

	g_assert(tree);
	g_assert(prefix);
	
	if (!mateconf_client_dir_exists (client, prefix, NULL)) 
		return FALSE;
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
	
	key = g_strdup_printf ("%s/sort_col", prefix);
	sort_col = mateconf_client_get_int (client, key, NULL);
	g_free (key);
	
	key = g_strdup_printf ("%s/sort_order", prefix);
	order = static_cast<GtkSortType>(mateconf_client_get_int (client, key, NULL));
	g_free (key);
	
	if (sort_col != -1)
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
					      	      sort_col,
					              order);

	columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));

	for(it = columns; it; it = it->next)
	{
		GtkTreeViewColumn *column;
		MateConfValue *value = NULL;
		gint width;
		gboolean visible;
		int id;

		column = static_cast<GtkTreeViewColumn*>(it->data);
		id = gtk_tree_view_column_get_sort_column_id (column);

		key = g_strdup_printf ("%s/col_%d_width", prefix, id);
		value = mateconf_client_get (client, key, NULL);
		g_free (key);

		if (value != NULL) {
			width = mateconf_value_get_int(value);
			mateconf_value_free (value);

			key = g_strdup_printf ("%s/col_%d_visible", prefix, id);
			visible = mateconf_client_get_bool (client, key, NULL);
			g_free (key);

			column = gtk_tree_view_get_column (GTK_TREE_VIEW (tree), id);
			if(!column) continue;
			gtk_tree_view_column_set_visible (column, visible);
			if (visible) {
				/* ensure column is really visible */
				width = MAX(width, 10);
				gtk_tree_view_column_set_fixed_width(column, width);
			}
		}
	}

	if(g_str_has_suffix(prefix, "proctree") || g_str_has_suffix(prefix, "disktreenew"))
	{
		GSList *order;
		char *key;

		key = g_strdup_printf("%s/columns_order", prefix);
		order = mateconf_client_get_list(client, key, MATECONF_VALUE_INT, NULL);
		proctable_set_columns_order(GTK_TREE_VIEW(tree), order);

		g_slist_free(order);
		g_free(key);
	}

	g_list_free(columns);
	
	return TRUE;
}

void
procman_save_tree_state (MateConfClient *client, GtkWidget *tree, const gchar *prefix)
{
	GtkTreeModel *model;
	GList *it, *columns;
	gint sort_col;
	GtkSortType order;
	
	g_assert(tree);
	g_assert(prefix);
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (tree));
	if (gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model), &sort_col,
					          &order)) {
		gchar *key;
		
		key = g_strdup_printf ("%s/sort_col", prefix);
		mateconf_client_set_int (client, key, sort_col, NULL);
		g_free (key);
		
		key = g_strdup_printf ("%s/sort_order", prefix);
		mateconf_client_set_int (client, key, order, NULL);
		g_free (key);
	}			       
	
	columns = gtk_tree_view_get_columns (GTK_TREE_VIEW (tree));

	for(it = columns; it; it = it->next)
	{
		GtkTreeViewColumn *column;
		gboolean visible;
		gint width;
		gchar *key;
		int id;

		column = static_cast<GtkTreeViewColumn*>(it->data);
		id = gtk_tree_view_column_get_sort_column_id (column);
		visible = gtk_tree_view_column_get_visible (column);
		width = gtk_tree_view_column_get_width (column);

		key = g_strdup_printf ("%s/col_%d_width", prefix, id);
		mateconf_client_set_int (client, key, width, NULL);
		g_free (key);

		key = g_strdup_printf ("%s/col_%d_visible", prefix, id);
		mateconf_client_set_bool (client, key, visible, NULL);
		g_free (key);
	}

	if(g_str_has_suffix(prefix, "proctree") || g_str_has_suffix(prefix, "disktreenew"))
	{
		GSList *order;
		char *key;
		GError *error = NULL;

		key = g_strdup_printf("%s/columns_order", prefix);
		order = proctable_get_columns_order(GTK_TREE_VIEW(tree));

		if(!mateconf_client_set_list(client, key, MATECONF_VALUE_INT, order, &error))
		{
			g_critical("Could not save MateConf key '%s' : %s",
				   key,
				   error->message);
			g_error_free(error);
		}

		g_slist_free(order);
		g_free(key);
	}

	g_list_free(columns);
}

void
procman_save_config (ProcData *data)
{
	MateConfClient *client = data->client;
	gint width, height;

	g_assert(data);
		
	procman_save_tree_state (data->client, data->tree, "/apps/procman/proctree");
	procman_save_tree_state (data->client, data->disk_list, "/apps/procman/disktreenew");
		
	gdk_drawable_get_size (gtk_widget_get_window (data->app), &width, &height);
	data->config.width = width;
	data->config.height = height;
	
	mateconf_client_set_int (client, "/apps/procman/width", data->config.width, NULL);
	mateconf_client_set_int (client, "/apps/procman/height", data->config.height, NULL);	
	mateconf_client_set_int (client, "/apps/procman/current_tab", data->config.current_tab, NULL);

	mateconf_client_suggest_sync (client, NULL);
	

	
}

static guint32
get_startup_timestamp ()
{
	const gchar *startup_id_env;
	gchar *startup_id = NULL;
	gchar *time_str;
	gulong retval = 0;

	/* we don't unset the env, since startup-notification
	 * may still need it */
	startup_id_env = g_getenv ("DESKTOP_STARTUP_ID");
	if (startup_id_env == NULL)
		goto out;

	startup_id = g_strdup (startup_id_env);

	time_str = g_strrstr (startup_id, "_TIME");
	if (time_str == NULL)
		goto out;

	/* Skip past the "_TIME" part */
	time_str += 5;

	retval = strtoul (time_str, NULL, 0);

 out:
	g_free (startup_id);

	return retval;
}


static void
cb_server (const gchar *msg, gpointer user_data)
{
	GdkWindow *window;
	ProcData *procdata;
	guint32 timestamp = 0;

	window = gdk_get_default_root_window ();

	procdata = *(ProcData**)user_data;
	g_assert (procdata != NULL);

	procman_debug("cb_server(%s)", msg);
	if (msg != NULL && procman::SHOW_SYSTEM_TAB_CMD == msg) {
		procman_debug("Changing to PROCMAN_TAB_SYSINFO via bacon message");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_SYSINFO);
		cb_change_current_page(GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_SYSINFO, procdata);
	} else
		timestamp = strtoul(msg, NULL, 0);

	if (timestamp == 0)
	{
		/* fall back to rountripping to X */
		timestamp = gdk_x11_get_server_time (window);
	}

	gdk_x11_window_set_user_time (window, timestamp);

	gtk_window_present (GTK_WINDOW(procdata->app));
}




static void
mount_changed(const Glib::RefPtr<Gio::Mount>&)
{
  cb_update_disks(ProcData::get_instance());
}


static void
init_volume_monitor(ProcData *procdata)
{
  using namespace Gio;
  using namespace Glib;

  RefPtr<VolumeMonitor> monitor = VolumeMonitor::get();

  monitor->signal_mount_added().connect(sigc::ptr_fun(&mount_changed));
  monitor->signal_mount_changed().connect(sigc::ptr_fun(&mount_changed));
  monitor->signal_mount_removed().connect(sigc::ptr_fun(&mount_changed));
}


namespace procman
{
	const std::string SHOW_SYSTEM_TAB_CMD("SHOWSYSTAB");
}



int
main (int argc, char *argv[])
{
	guint32 startup_timestamp;
	MateConfClient *client;
	ProcData *procdata;
	BaconMessageConnection *conn;

	bindtextdomain (GETTEXT_PACKAGE, MATELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	setlocale (LC_ALL, "");
	
	startup_timestamp = get_startup_timestamp();

	Glib::OptionContext context;
	context.set_summary(_("A simple process and system monitor."));
	context.set_ignore_unknown_options(true);
	procman::OptionGroup option_group;
	context.set_main_group(option_group);

	try {
		context.parse(argc, argv);
	} catch (const Glib::Error& ex) {
		g_error("Arguments parse error : %s", ex.what().c_str());
	}

	Gio::init();
	Gtk::Main kit(&argc, &argv);
	procman_debug("post gtk_init");

	conn = bacon_message_connection_new ("mate-system-monitor");
	if (!conn) g_error("Couldn't connect to mate-system-monitor");

	if (bacon_message_connection_get_is_server (conn))
	{
		bacon_message_connection_set_callback (conn, cb_server, &procdata);
	}
	else /* client */
	{
		char *timestamp;

		timestamp = g_strdup_printf ("%" G_GUINT32_FORMAT, startup_timestamp);

		if (option_group.show_system_tab)
			bacon_message_connection_send(conn, procman::SHOW_SYSTEM_TAB_CMD.c_str());

		bacon_message_connection_send (conn, timestamp);

		gdk_notify_startup_complete ();

		g_free (timestamp);
		bacon_message_connection_free (conn);

		exit (0);
	}

	gtk_window_set_default_icon_name ("utilities-system-monitor");
	g_set_application_name(_("System Monitor"));
		    
	mateconf_init (argc, argv, NULL);
			    
	client = mateconf_client_get_default ();
	mateconf_client_add_dir(client, "/apps/procman", MATECONF_CLIENT_PRELOAD_NONE, NULL);

	glibtop_init ();

	procman_debug("end init");
	
	procdata = procman_data_new (client);
	procdata->client = client;

	procman_debug("begin create_main_window");
	create_main_window (procdata);
	procman_debug("end create_main_window");
	
	// proctable_update_all (procdata);

	init_volume_monitor (procdata);

	g_assert(procdata->app);
			
	if (option_group.show_system_tab) {
		procman_debug("Starting with PROCMAN_TAB_SYSINFO by commandline request");
		gtk_notebook_set_current_page(GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_SYSINFO);
		cb_change_current_page (GTK_NOTEBOOK(procdata->notebook), PROCMAN_TAB_SYSINFO, procdata);
	}

 	gtk_widget_show(procdata->app);
       
	procman_debug("begin gtk_main");
	kit.run();
	
	procman_free_data (procdata);

	glibtop_close ();

	return 0;
}

