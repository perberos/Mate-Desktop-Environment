#include <config.h>

#include <gtkmm/messagedialog.h>
#include <glib/gi18n.h>
#include <glibtop/procopenfiles.h>

#include <sys/wait.h>

// #include <libsexy/sexy-icon-entry.h>


#include <set>
#include <string>
#include <sstream>
#include <iterator>

#include "regex.h"

#include "procman.h"
#include "lsof.h"
#include "util.h"


using std::string;


namespace
{

  class Lsof
  {
    Glib::RefPtr<Glib::Regex> re;

    bool matches(const string &filename) const
    {
      return this->re->match(filename);
    }

  public:

    Lsof(const string &pattern, bool caseless)
    {
      Glib::RegexCompileFlags flags = static_cast<Glib::RegexCompileFlags>(0);

      if (caseless)
	flags |= Glib::REGEX_CASELESS;

      this->re = Glib::Regex::create(pattern, flags);
    }


    template<typename OutputIterator>
    void search(const ProcInfo &info, OutputIterator out) const
    {
      glibtop_open_files_entry *entries;
      glibtop_proc_open_files buf;

      entries = glibtop_get_proc_open_files(&buf, info.pid);

      for (unsigned i = 0; i != buf.number; ++i) {
	if (entries[i].type & GLIBTOP_FILE_TYPE_FILE) {
	  const string filename(entries[i].info.file.name);
	  if (this->matches(filename))
	    *out++ = filename;
	}
      }

      g_free(entries);
    }
  };



  // GUI Stuff


  enum ProcmanLsof {
    PROCMAN_LSOF_COL_PIXBUF,
    PROCMAN_LSOF_COL_PROCESS,
    PROCMAN_LSOF_COL_PID,
    PROCMAN_LSOF_COL_FILENAME,
    PROCMAN_LSOF_NCOLS
  };


  struct GUI {

    GtkListStore *model;
    GtkEntry *entry;
    GtkWindow *window;
    GtkLabel *count;
    ProcData *procdata;
    bool case_insensitive;


    GUI()
    {
      procman_debug("New Lsof GUI %p", this);
    }


    ~GUI()
    {
      procman_debug("Destroying Lsof GUI %p", this);
    }


    void clear_results()
    {
      gtk_list_store_clear(this->model);
      gtk_label_set_text(this->count, "");
    }


    void clear()
    {
      this->clear_results();
      gtk_entry_set_text(this->entry, "");
    }


    void display_regex_error(const Glib::RegexError& error)
    {
      const char * msg = _("<b>Error</b>\n"
			   "'%s' is not a valid Perl regular expression.\n"
			   "%s");
      std::string message = make_string(g_strdup_printf(msg, this->pattern().c_str(), error.what().c_str()));

      Gtk::MessageDialog dialog(message,
				true, // use markup
				Gtk::MESSAGE_ERROR,
				Gtk::BUTTONS_OK,
				true); // modal
      dialog.run();
    }


    void update_count(unsigned count)
    {
      string s = static_cast<std::ostringstream&>(std::ostringstream() << count).str();
      gtk_label_set_text(this->count, s.c_str());
    }


    string pattern() const
    {
      return gtk_entry_get_text(this->entry);
    }


    void search()
    {
      typedef std::set<string> MatchSet;
      typedef MatchSet::const_iterator iterator;

      this->clear_results();


      try {
	Lsof lsof(this->pattern(), this->case_insensitive);

	unsigned count = 0;

	for (ProcInfo::Iterator it(ProcInfo::begin()); it != ProcInfo::end(); ++it) {
	  const ProcInfo &info(*it->second);

	  MatchSet matches;
	  lsof.search(info, std::inserter(matches, matches.begin()));
	  count += matches.size();

	  for (iterator it(matches.begin()), end(matches.end()); it != end; ++it) {
	    GtkTreeIter file;
	    gtk_list_store_append(this->model, &file);
	    gtk_list_store_set(this->model, &file,
			       PROCMAN_LSOF_COL_PIXBUF, info.pixbuf->gobj(),
			       PROCMAN_LSOF_COL_PROCESS, info.name,
			       PROCMAN_LSOF_COL_PID, info.pid,
			       PROCMAN_LSOF_COL_FILENAME, it->c_str(),
			       -1);
	  }
	}

	this->update_count(count);
      }
      catch (Glib::RegexError& error) {
	this->display_regex_error(error);
      }
    }


    static void search_button_clicked(GtkButton *, gpointer data)
    {
      static_cast<GUI*>(data)->search();
    }


    static void search_entry_activate(GtkEntry *, gpointer data)
    {
      static_cast<GUI*>(data)->search();
    }


    static void clear_button_clicked(GtkButton *, gpointer data)
    {
      static_cast<GUI*>(data)->clear();
    }


    static void close_button_clicked(GtkButton *, gpointer data)
    {
      GUI *gui = static_cast<GUI*>(data);
      gtk_widget_destroy(GTK_WIDGET(gui->window));
      delete gui;
    }


    static void case_button_toggled(GtkToggleButton *button, gpointer data)
    {
      bool state = gtk_toggle_button_get_active(button);
      static_cast<GUI*>(data)->case_insensitive = state;
    }


    static gboolean window_delete_event(GtkWidget *, GdkEvent *, gpointer data)
    {
      delete static_cast<GUI*>(data);
      return FALSE;
    }

  };
}




void procman_lsof(ProcData *procdata)
{
  GtkListStore *model = \
    gtk_list_store_new(PROCMAN_LSOF_NCOLS,
		       GDK_TYPE_PIXBUF,	// PROCMAN_LSOF_COL_PIXBUF
		       G_TYPE_STRING,	// PROCMAN_LSOF_COL_PROCESS
		       G_TYPE_UINT,	// PROCMAN_LSOF_COL_PID
		       G_TYPE_STRING	// PROCMAN_LSOF_COL_FILENAME
		       );

  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
  g_object_unref(model);
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tree), TRUE);


  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  // PIXBUF / PROCESS

  column = gtk_tree_view_column_new();

  renderer = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(column, renderer, FALSE);
  gtk_tree_view_column_set_attributes(column, renderer,
				      "pixbuf", PROCMAN_LSOF_COL_PIXBUF,
				      NULL);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(column, renderer, FALSE);
  gtk_tree_view_column_set_attributes(column, renderer,
				      "text", PROCMAN_LSOF_COL_PROCESS,
				      NULL);

  gtk_tree_view_column_set_title(column, _("Process"));
  gtk_tree_view_column_set_sort_column_id(column, PROCMAN_LSOF_COL_PROCESS);
  gtk_tree_view_column_set_resizable(column, TRUE);
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_column_set_min_width(column, 10);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), PROCMAN_LSOF_COL_PROCESS,
				       GTK_SORT_ASCENDING);


  // PID
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(_("PID"), renderer,
						    "text", PROCMAN_LSOF_COL_PID,
						    NULL);
  gtk_tree_view_column_set_sort_column_id(column, PROCMAN_LSOF_COL_PID);
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);


  // FILENAME
  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes(_("Filename"), renderer,
						    "text", PROCMAN_LSOF_COL_FILENAME,
						    NULL);
  gtk_tree_view_column_set_sort_column_id(column, PROCMAN_LSOF_COL_FILENAME);
  gtk_tree_view_column_set_resizable(column, TRUE);
  gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);


  GtkWidget *dialog; /* = gtk_dialog_new_with_buttons(_("Search for Open Files"), NULL,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			NULL); */
  dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(procdata->app));
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
  // gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_title(GTK_WINDOW(dialog), _("Search for Open Files"));

  // g_signal_connect(G_OBJECT(dialog), "response",
  //		   G_CALLBACK(close_dialog), NULL);
  gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 575, 400);
  // gtk_dialog_set_has_separator(GTK_DIALOG(dialog), FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(dialog), 12);
  GtkWidget *mainbox = gtk_vbox_new(FALSE, 12);
  gtk_container_add(GTK_CONTAINER(dialog), mainbox);
  gtk_box_set_spacing(GTK_BOX(mainbox), 6);


  // Label, entry and search button

  GtkWidget *hbox1 = gtk_hbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(mainbox), hbox1, FALSE, FALSE, 0);

  GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_DIALOG);
  gtk_box_pack_start(GTK_BOX(hbox1), image, FALSE, FALSE, 0);


  GtkWidget *vbox2 = gtk_vbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(hbox1), vbox2, TRUE, TRUE, 0);


  GtkWidget *hbox = gtk_hbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(vbox2), hbox, TRUE, TRUE, 0);
  GtkWidget *label = gtk_label_new_with_mnemonic(_("_Name contains:"));
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  GtkWidget *entry = gtk_entry_new();

  // entry = sexy_icon_entry_new();
  // sexy_icon_entry_add_clear_button(SEXY_ICON_ENTRY(entry));
  // GtkWidget *icon = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU);
  // sexy_icon_entry_set_icon(SEXY_ICON_ENTRY(entry), SEXY_ICON_ENTRY_PRIMARY, GTK_IMAGE(icon));

  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
  GtkWidget *search_button = gtk_button_new_from_stock(GTK_STOCK_FIND);
  gtk_box_pack_start(GTK_BOX(hbox), search_button, FALSE, FALSE, 0);
  GtkWidget *clear_button = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
  gtk_box_pack_start(GTK_BOX(hbox), clear_button, FALSE, FALSE, 0);


  GtkWidget *case_button = gtk_check_button_new_with_mnemonic(_("Case insensitive matching"));
  GtkWidget *hbox3 = gtk_hbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(hbox3), case_button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox2), hbox3, FALSE, FALSE, 0);


  GtkWidget *results_box = gtk_hbox_new(FALSE, 12);
  gtk_box_pack_start(GTK_BOX(mainbox), results_box, FALSE, FALSE, 0);
  GtkWidget *results_label = gtk_label_new_with_mnemonic(_("S_earch results:"));
  gtk_box_pack_start(GTK_BOX(results_box), results_label, FALSE, FALSE, 0);
  GtkWidget *count_label = gtk_label_new(NULL);
  gtk_box_pack_end(GTK_BOX(results_box), count_label, FALSE, FALSE, 0);




  // Scrolled TreeView
  GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
				 GTK_POLICY_AUTOMATIC,
				 GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
				      GTK_SHADOW_IN);
  gtk_container_add(GTK_CONTAINER(scrolled), tree);
  gtk_box_pack_start(GTK_BOX(mainbox), scrolled, TRUE, TRUE, 0);

  GtkWidget *bottom_box = gtk_hbox_new(FALSE, 12);
  GtkWidget *close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  gtk_box_pack_start(GTK_BOX(mainbox), bottom_box, FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(bottom_box), close_button, FALSE, FALSE, 0);


  GUI *gui = new GUI; // wil be deleted by the close button or delete-event
  gui->procdata = procdata;
  gui->model = model;
  gui->window = GTK_WINDOW(dialog);
  gui->entry = GTK_ENTRY(entry);
  gui->count = GTK_LABEL(count_label);

  g_signal_connect(G_OBJECT(entry), "activate",
		   G_CALLBACK(GUI::search_entry_activate), gui);
  g_signal_connect(G_OBJECT(clear_button), "clicked",
		   G_CALLBACK(GUI::clear_button_clicked), gui);
  g_signal_connect(G_OBJECT(search_button), "clicked",
		   G_CALLBACK(GUI::search_button_clicked), gui);
  g_signal_connect(G_OBJECT(close_button), "clicked",
		   G_CALLBACK(GUI::close_button_clicked), gui);
  g_signal_connect(G_OBJECT(case_button), "toggled",
		   G_CALLBACK(GUI::case_button_toggled), gui);
  g_signal_connect(G_OBJECT(dialog), "delete-event",
		   G_CALLBACK(GUI::window_delete_event), gui);


  gtk_widget_show_all(dialog);
}

