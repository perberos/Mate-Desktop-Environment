#include <config.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <glibmm.h>
#include <glib/gi18n.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <glibtop/fsusage.h>
#include <glibtop/mountlist.h>
#include <glibtop/mem.h>
#include <glibtop/sysinfo.h>

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <math.h>
#include <errno.h>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/utsname.h>

#include "sysinfo.h"
#include "procman.h"
#include "util.h"


using std::string;
using std::vector;


namespace {


  class SysInfo
  {
  public:
    string hostname;
    string distro_name;
    string distro_release;
    string kernel;
    string mate_version;
    guint64 memory_bytes;
    guint64 free_space_bytes;

    guint n_processors;
    vector<string> processors;


    SysInfo()
    {
      this->load_processors_info();
      this->load_memory_info();
      this->load_disk_info();
      this->load_uname_info();
      this->load_mate_version();
    }

    virtual ~SysInfo()
    { }

    virtual void set_distro_labels(GtkWidget* name, GtkWidget* release)
    {
      g_object_set(G_OBJECT(name),
		   "label",
		   ("<big><big><b>" + this->distro_name + "</b></big></big>").c_str(),
		   NULL);


      char* markup = g_strdup_printf(_("Release %s"), this->distro_release.c_str());

      g_object_set(G_OBJECT(release),
		   "label",
		   markup,
		   NULL);

      g_free(markup);
    }

    static string system()
    {
      return uname().sysname;
    }

  private:

    void load_memory_info()
    {
      glibtop_mem mem;

      glibtop_get_mem(&mem);
      this->memory_bytes = mem.total;
    }


    void load_processors_info()
    {
      const glibtop_sysinfo *info = glibtop_get_sysinfo();

      for (guint i = 0; i != info->ncpu; ++i) {
	const char * const keys[] = { "model name", "cpu" };
	gchar *model = 0;

	for (guint j = 0; !model && j != G_N_ELEMENTS(keys); ++j)
	  model = static_cast<char*>(g_hash_table_lookup(info->cpuinfo[i].values,
							 keys[j]));

	if (!model)
	  model = _("Unknown CPU model");

	this->processors.push_back(model);
      }
    }



    void load_disk_info()
    {
      glibtop_mountentry *entries;
      glibtop_mountlist mountlist;

      entries = glibtop_get_mountlist(&mountlist, 0);

      this->free_space_bytes = 0;

      for (guint i = 0; i != mountlist.number; ++i) {

	if (string(entries[i].devname).find("/dev/") != 0)
	  continue;

	if (string(entries[i].mountdir).find("/media/") == 0)
	  continue;

	glibtop_fsusage usage;
	glibtop_get_fsusage(&usage, entries[i].mountdir);
	this->free_space_bytes += usage.bavail * usage.block_size;
      }

      g_free(entries);
    }

    static const struct utsname & uname()
    {
      static struct utsname name;

      if (!name.sysname[0]) {
	::uname(&name);
      }

      return name;
    }

    void load_uname_info()
    {
      this->hostname = uname().nodename;
      this->kernel = string(uname().sysname) + ' ' + uname().release;
    }


    void load_mate_version()
    {
      xmlDocPtr document;
      xmlXPathContextPtr context;
      const string nodes[3] = { "string(/mate-version/platform)",
				"string(/mate-version/minor)",
				"string(/mate-version/micro)" };
      string values[3];

      if (not (document = xmlParseFile(DATADIR "/mate-about/mate-version.xml")))
	return;

      if (not (context = xmlXPathNewContext(document)))
	return;

      for (size_t i = 0; i != 3; ++i)
	{
	  xmlXPathObjectPtr xpath;
	  xpath = xmlXPathEvalExpression(BAD_CAST nodes[i].c_str(), context);
	  
	  if (xpath and xpath->type == XPATH_STRING)
	    values[i] = reinterpret_cast<const char*>(xpath->stringval);

	  xmlXPathFreeObject(xpath);
	}

      xmlXPathFreeContext(context);
      xmlFreeDoc(document);

      this->mate_version = values[0] + '.' + values[1] + '.' + values[2];
    }
  };



  class SolarisSysInfo
    : public SysInfo
  {
  public:
    SolarisSysInfo()
    {
      this->load_solaris_info();
    }

  private:
    void load_solaris_info()
    {
      this->distro_name = "Solaris";

      std::ifstream input("/etc/release");

      if (input)
	std::getline(input, this->distro_release);
    }
  };


  class LSBSysInfo
    : public SysInfo
  {
  public:
    LSBSysInfo()
      : re(Glib::Regex::create("^.+?:\\s*(.+)\\s*$"))
    {
      // start();
    }

    virtual void set_distro_labels(GtkWidget* name, GtkWidget* release)
    {
      this->name = name;
      this->release = release;

      this->start();
    }


  private:

    sigc::connection child_watch;
    int lsb_fd;
    GtkWidget* name;
    GtkWidget* release;

    void strip_description(string &s) const
    {
      const GRegexMatchFlags flags = static_cast<GRegexMatchFlags>(0);
      GMatchInfo* info = 0;

      if (g_regex_match(this->re->gobj(), s.c_str(), flags, &info)) {
	s = make_string(g_match_info_fetch(info, 1));
	g_match_info_free(info);
      }
    }

    std::istream& get_value(std::istream &is, string &s) const
    {
      if (std::getline(is, s))
	this->strip_description(s);
      return is;
    }


    void read_lsb(Glib::Pid pid, int status)
    {
      this->child_watch.disconnect();

      if (!WIFEXITED(status) or WEXITSTATUS(status) != 0) {
	g_error("Child %d failed with status %d", int(pid), status);
	return;
      }

      Glib::RefPtr<Glib::IOChannel> channel = Glib::IOChannel::create_from_fd(this->lsb_fd);
      Glib::ustring content;

      while (channel->read_to_end(content) == Glib::IO_STATUS_AGAIN)
	;

      channel->close();
      Glib::spawn_close_pid(pid);

      procman_debug("lsb_release output = '%s'", content.c_str());

      string release, codename;
      std::istringstream input(content);

      this->get_value(input, this->distro_name)
	and this->get_value(input, release)
	and this->get_value(input, codename);

      this->distro_release = release;
      if (codename != "" && codename != "n/a")
	this->distro_release += " (" + codename + ')';

      this->SysInfo::set_distro_labels(this->name, this->release);
    }


    void start()
    {
      std::vector<string> argv(2);
      argv[0] = "lsb_release";
      argv[1] = "-irc";

      Glib::SpawnFlags flags = Glib::SPAWN_DO_NOT_REAP_CHILD
	| Glib::SPAWN_SEARCH_PATH
	| Glib::SPAWN_STDERR_TO_DEV_NULL;

      Glib::Pid child;

      try {
	Glib::spawn_async_with_pipes("/", // wd
				     argv,
				     flags,
				     sigc::slot<void>(), // child setup
				     &child,
				     0, // stdin
				     &this->lsb_fd); // stdout
      } catch (Glib::SpawnError &e) {
	g_error("g_spawn_async_with_pipes error: %s", e.what().c_str());
	return;
      }

      sigc::slot<void,GPid, int> slot = sigc::mem_fun(this, &LSBSysInfo::read_lsb);
      this->child_watch = Glib::signal_child_watch().connect(slot, child);
    }


    void sync_lsb_release()
    {
      char *out= 0;
      GError *error = 0;
      int status;

      if (g_spawn_command_line_sync("lsb_release -irc",
				    &out,
				    0,
				    &status,
				    &error)) {
	string release, codename;
	if (!error and WIFEXITED(status) and WEXITSTATUS(status) == 0) {
	  std::istringstream input(out);
	  this->get_value(input, this->distro_name)
	    and this->get_value(input, release)
	    and this->get_value(input, codename);
	  this->distro_release = release;
	  if (codename != "" && codename != "n/a")
	    this->distro_release += " (" + codename + ')';
	}
      }

      if (error)
	g_error_free(error);

      g_free(out);
    }

  private:
    Glib::RefPtr<Glib::Regex> re;
  };


  class NetBSDSysInfo
    : public SysInfo
  {
  public:
    NetBSDSysInfo()
    {
      this->load_netbsd_info();
    }

  private:
    void load_netbsd_info()
    {
      this->distro_name = "NetBSD";

      std::ifstream input("/etc/release");

      if (input)
	std::getline(input, this->distro_release);
    }
  };


  SysInfo* get_sysinfo()
  {
    if (char *p = g_find_program_in_path("lsb_release")) {
      g_free(p);
      return new LSBSysInfo;
    }
    else if (SysInfo::system() == "SunOS") {
      return new SolarisSysInfo;
    }
    else if (SysInfo::system() == "NetBSD") {
      return new NetBSDSysInfo;
    }

    return new SysInfo;
  }
}


#define X_PAD  5
#define Y_PAD  12
#define LOGO_W 92
#define LOGO_H 351
#define RADIUS 5

static gboolean
sysinfo_logo_expose (GtkWidget *widget,
		     GdkEventExpose *event,
		     gpointer data_ptr)
{
  GtkAllocation allocation;
  GtkStyle *style;
  cairo_t *cr;
  cairo_pattern_t *cp;

  cr = gdk_cairo_create(gtk_widget_get_window(widget));

  gtk_widget_get_allocation (widget, &allocation);
  cairo_translate(cr, allocation.x, allocation.y);

  cairo_move_to(cr, X_PAD + RADIUS, Y_PAD);
  cairo_line_to(cr, X_PAD + LOGO_W - RADIUS, Y_PAD);
  cairo_arc(cr, X_PAD + LOGO_W - RADIUS, Y_PAD + RADIUS, RADIUS, -0.5 * M_PI, 0);
  cairo_line_to(cr, X_PAD + LOGO_W, Y_PAD + LOGO_H - RADIUS);
  cairo_arc(cr, X_PAD + LOGO_W - RADIUS, Y_PAD + LOGO_H - RADIUS, RADIUS, 0, 0.5 * M_PI);
  cairo_line_to(cr, X_PAD + RADIUS, Y_PAD + LOGO_H);
  cairo_arc(cr, X_PAD + RADIUS, Y_PAD + LOGO_H - RADIUS, RADIUS, 0.5 * M_PI, -1.0 * M_PI);
  cairo_line_to(cr, X_PAD, Y_PAD + RADIUS);
  cairo_arc(cr,  X_PAD + RADIUS, Y_PAD + RADIUS, RADIUS, -1.0 * M_PI, -0.5 * M_PI);

  cp = cairo_pattern_create_linear(0, Y_PAD, 0, Y_PAD + LOGO_H);
  style = gtk_widget_get_style (widget);
  cairo_pattern_add_color_stop_rgba(cp, 0.0,
				    style->base[GTK_STATE_SELECTED].red / 65535.0,
				    style->base[GTK_STATE_SELECTED].green / 65535.0,
				    style->base[GTK_STATE_SELECTED].blue / 65535.0,
				    1.0);
  cairo_pattern_add_color_stop_rgba(cp, 1.0,
				    style->base[GTK_STATE_SELECTED].red / 65535.0,
				    style->base[GTK_STATE_SELECTED].green / 65535.0,
				    style->base[GTK_STATE_SELECTED].blue / 65535.0,
				    0.0);
  cairo_set_source(cr, cp);
  cairo_fill(cr);

  cairo_pattern_destroy(cp);
  cairo_destroy(cr);

  return FALSE;
}

static GtkWidget*
add_section(GtkBox *vbox , const char * title, int num_row, int num_col, GtkWidget **out_frame)
{
  GtkWidget *table;

  GtkWidget *frame = gtk_frame_new(title);
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.5);
  gtk_label_set_use_markup(
			   GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(frame))),
			   TRUE
			   );
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

  GtkWidget *alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0, 12, 0);
  gtk_container_add(GTK_CONTAINER(frame), alignment);

  table = gtk_table_new(num_row, num_col, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 6);
  gtk_table_set_col_spacings(GTK_TABLE(table), 6);
  gtk_container_set_border_width(GTK_CONTAINER(table), 6);
  gtk_container_add(GTK_CONTAINER(alignment), table);

  if(out_frame)
    *out_frame = frame;

  return table;
}


static GtkWidget* 
add_row(GtkTable * table, const char * label, const char * value, int row)
{
  GtkWidget *header = gtk_label_new(label);
  gtk_misc_set_alignment(GTK_MISC(header), 0.0, 0.5);
  gtk_table_attach(
		   table, header,
		   0, 1, row, row + 1,
		   GTK_FILL, GTK_FILL, 0, 0
		   );

  GtkWidget *label_widget = gtk_label_new(value);
  gtk_misc_set_alignment(GTK_MISC(label_widget), 0.0, 0.5);
  gtk_table_attach(
		   table, label_widget,
		   1, 2, row, row + 1,
		   GTK_FILL, GTK_FILL, 0, 0
		   );
  return label_widget;
}


static GtkWidget *
procman_create_sysinfo_view(void)
{
  GtkWidget *hbox;
  GtkWidget *vbox;

  SysInfo *data = get_sysinfo();;

  GtkWidget * logo;

  GtkWidget *distro_frame;
  GtkWidget *distro_release_label;
  GtkWidget *distro_table;

  GtkWidget *hardware_table;
  GtkWidget *memory_label;
  GtkWidget *processor_label;

  GtkWidget *disk_space_table;
  GtkWidget *disk_space_label;

  GtkWidget *header;

  gchar *markup;


  hbox = gtk_hbox_new(FALSE, 12);
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

  /* left-side logo */

  logo = gtk_image_new_from_file(DATADIR "/pixmaps/mate-system-monitor/side.png");
  gtk_misc_set_alignment(GTK_MISC(logo), 0.5, 0.0);
  gtk_misc_set_padding(GTK_MISC(logo), 5, 12);
  gtk_box_pack_start(GTK_BOX(hbox), logo, FALSE, FALSE, 0);

  g_signal_connect(G_OBJECT(logo), "expose-event",
		   G_CALLBACK(sysinfo_logo_expose), NULL);

  vbox = gtk_vbox_new(FALSE, 12);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
  gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);

  // hostname

  markup = g_strdup_printf("<big><big><b><u>%s</u></b></big></big>",
    data->hostname.c_str());
  GtkWidget *hostname_frame = gtk_frame_new(markup);
  g_free(markup);
  gtk_frame_set_label_align(GTK_FRAME(hostname_frame), 0.0, 0.5);
  gtk_label_set_use_markup(
			   GTK_LABEL(gtk_frame_get_label_widget(GTK_FRAME(hostname_frame))),
			   TRUE
			   );
  gtk_frame_set_shadow_type(GTK_FRAME(hostname_frame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vbox), hostname_frame, FALSE, FALSE, 0);


  /* distro section */

  unsigned table_size = 2;
  if (data->mate_version != "")
     table_size++;
  distro_table = add_section(GTK_BOX(vbox), "???", table_size, 1, &distro_frame);

  unsigned table_count = 0;

  distro_release_label = gtk_label_new("???");
  gtk_misc_set_alignment(GTK_MISC(distro_release_label), 0.0, 0.5);
  gtk_table_attach(
		   GTK_TABLE(distro_table), distro_release_label,
		   0, 1, table_count, table_count+1,
		   GTK_FILL, GTK_FILL, 0, 0
		   );
  table_count++;
  data->set_distro_labels(gtk_frame_get_label_widget(GTK_FRAME(distro_frame)), distro_release_label);

  markup = g_strdup_printf(_("Kernel %s"), data->kernel.c_str());
  header = gtk_label_new(markup);
  g_free(markup);
  gtk_misc_set_alignment(GTK_MISC(header), 0.0, 0.5);
  gtk_table_attach(
		   GTK_TABLE(distro_table), header,
		   0, 1, table_count, table_count + 1,
		   GTK_FILL, GTK_FILL, 0, 0
		   );
  table_count++;

  if (data->mate_version != "")
    {
      markup = g_strdup_printf(_("MATE %s"), data->mate_version.c_str());
      header = gtk_label_new(markup);
      g_free(markup);
      gtk_misc_set_alignment(GTK_MISC(header), 0.0, 0.5);
      gtk_table_attach(
		       GTK_TABLE(distro_table), header,
		       0, 1, table_count, table_count + 1,
		       GTK_FILL, GTK_FILL, 0, 0
		       );
      table_count++;
    }

  /* hardware section */

  markup = g_strdup_printf(_("<b>Hardware</b>"));
  hardware_table = add_section(GTK_BOX(vbox), markup, data->processors.size(), 2, NULL);
  g_free(markup);

  markup = procman::format_size(data->memory_bytes);
  memory_label = add_row(GTK_TABLE(hardware_table), _("Memory:"),
                         markup, 0);
  g_free(markup);

  for (guint i = 0; i < data->processors.size(); ++i) {
    const gchar * t;
    if (data->processors.size() > 1) {
      markup = g_strdup_printf(_("Processor %d:"), i);
      t = markup;
    }
    else {
      markup = NULL;
      t = _("Processor:");
    }

    processor_label = add_row(GTK_TABLE(hardware_table), t,
                              data->processors[i].c_str(), 1 + i);

    if(markup)
      g_free(markup);
  }

  /* disk space section */

  markup = g_strdup_printf(_("<b>System Status</b>"));
  disk_space_table = add_section(GTK_BOX(vbox), markup, 1, 2, NULL);
  g_free(markup);

  markup = procman::format_size(data->free_space_bytes);
  disk_space_label = add_row(GTK_TABLE(disk_space_table), 
                             _("Available disk space:"), markup,
                             0);
  g_free(markup);

  return hbox;
}



namespace procman
{
  void build_sysinfo_ui()
  {
    static GtkWidget* ui;

    if (!ui) {
      ProcData* procdata = ProcData::get_instance();
      ui = procman_create_sysinfo_view();
      GtkBox* box = GTK_BOX(gtk_notebook_get_nth_page(GTK_NOTEBOOK(procdata->notebook),
						      PROCMAN_TAB_SYSINFO));
      gtk_box_pack_start(box, ui, TRUE, TRUE, 0);
      gtk_widget_show_all(ui);
    }
  }
}
