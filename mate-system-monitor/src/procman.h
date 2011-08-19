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
#ifndef _PROCMAN_PROCMAN_H_
#define _PROCMAN_PROCMAN_H_


#include <glibmm/refptr.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <mateconf/mateconf-client.h>
#include <glibtop/cpu.h>

#include <time.h>
#include <sys/types.h>

#include <map>
#include <string>

struct ProcInfo;
struct ProcData;
struct LoadGraph;

#include "smooth_refresh.h"
#include "prettytable.h"

enum
{
	ALL_PROCESSES,
	MY_PROCESSES,
	ACTIVE_PROCESSES
};


static const unsigned MIN_UPDATE_INTERVAL =   1 * 1000;
static const unsigned MAX_UPDATE_INTERVAL = 100 * 1000;


namespace procman
{
	extern const std::string SHOW_SYSTEM_TAB_CMD;
}



enum ProcmanTab
{
	PROCMAN_TAB_SYSINFO,
	PROCMAN_TAB_PROCESSES,
	PROCMAN_TAB_RESOURCES,
	PROCMAN_TAB_DISKS
};


struct ProcConfig
{
	gint		width;
	gint		height;
        gboolean	show_kill_warning;
        gboolean	show_tree;
	gboolean	show_all_fs;
	int		update_interval;
 	int		graph_update_interval;
 	int		disks_update_interval;
	gint		whose_process;
	gint		current_tab;
	GdkColor	cpu_color[GLIBTOP_NCPU];
	GdkColor	mem_color;
	GdkColor	swap_color;
	GdkColor	net_in_color;
	GdkColor	net_out_color;
	GdkColor	bg_color;
	GdkColor	frame_color;
	gint 		num_cpus;
	bool solaris_mode;
	bool network_in_bits;
};



struct MutableProcInfo
{
  MutableProcInfo()
    : status(0)
  { }

  std::string user;

  gchar wchan[40];

  // all these members are filled with libgtop which uses
  // guint64 (to have fixed size data) but we don't need more
  // than an unsigned long (even for 32bit apps on a 64bit
  // kernel) as these data are amounts, not offsets.
  gulong vmsize;
  gulong memres;
  gulong memshared;
  gulong memwritable;
  gulong mem;

  // wnck gives an unsigned long
  gulong memxserver;

  gulong start_time;
  guint64 cpu_time;
  guint status;
  guint pcpu;
  gint nice;
};


class ProcInfo
  : public MutableProcInfo
{
	/* undefined */ ProcInfo& operator=(const ProcInfo&);
	/* undefined */ ProcInfo(const ProcInfo&);

	typedef std::map<guint, std::string> UserMap;
	/* cached username */
	static UserMap users;

 public:

	// TODO: use a set instead
	// sorted by pid. The map has a nice property : it is sorted
	// by pid so this helps a lot when looking for the parent node
	// as ppid is nearly always < pid.
	typedef std::map<pid_t, ProcInfo*> List;
	typedef List::iterator Iterator;

	static List all;

	static ProcInfo* find(pid_t pid);
	static Iterator begin() { return ProcInfo::all.begin(); }
	static Iterator end() { return ProcInfo::all.end(); }


	ProcInfo(pid_t pid);
	~ProcInfo();
	// adds one more ref to icon
	void set_icon(Glib::RefPtr<Gdk::Pixbuf> icon);
	void set_user(guint uid);

	GtkTreeIter	node;
	Glib::RefPtr<Gdk::Pixbuf> pixbuf;
	gchar		*tooltip;
	gchar		*name;
	gchar		*arguments;

	gchar		*security_context;

	const guint	pid;
	guint		ppid;
	guint		uid;

// private:
	// tracks cpu time per process keeps growing because if a
	// ProcInfo is deleted this does not mean that the process is
	// not going to be recreated on the next update.  For example,
	// if dependencies + (My or Active), the proclist is cleared
	// on each update.  This is a workaround
	static std::map<pid_t, guint64> cpu_times;
};

struct ProcData
{
	// lazy initialization
	static ProcData* get_instance();

	GtkUIManager	*uimanager;
	GtkActionGroup	*action_group;
	GtkWidget	*statusbar;
	gint		tip_message_cid;
	GtkWidget	*tree;
	GtkWidget	*loadavg;
	GtkWidget	*endprocessbutton;
	GtkWidget	*popup_menu;
	GtkWidget	*disk_list;
	GtkWidget	*notebook;
	ProcConfig	config;
	LoadGraph	*cpu_graph;
	LoadGraph	*mem_graph;
	LoadGraph	*net_graph;
	gint		cpu_label_fixed_width;
	gint		net_label_fixed_width;
	ProcInfo	*selected_process;
	GtkTreeSelection *selection;
	guint		timeout;
	guint		disk_timeout;

	PrettyTable	pretty_table;

	MateConfClient	*client;
	GtkWidget	*app;
	GtkUIManager	*menu;

	unsigned	frequency;

	SmoothRefresh  *smooth_refresh;

	guint64 cpu_total_time;
	guint64 cpu_total_time_last;

private:
	ProcData();
	/* undefined */ ProcData(const ProcData &);
	/* undefined */ ProcData& operator=(const ProcData &);
};

void		procman_save_config (ProcData *data);
void		procman_save_tree_state (MateConfClient *client, GtkWidget *tree, const gchar *prefix);
gboolean	procman_get_tree_state (MateConfClient *client, GtkWidget *tree, const gchar *prefix);





struct ReniceArgs
{
	ProcData *procdata;
	int nice_value;
};


struct KillArgs
{
	ProcData *procdata;
	int signal;
};

#endif /* _PROCMAN_PROCMAN_H_ */
