/*
 * gnote
 *
 * Copyright (C) 2010-2011 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
 * Copyright (C) 2009 Hubert Figuiere
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */



#ifndef __NOTE_RECENT_CHANGES_HPP_
#define __NOTE_RECENT_CHANGES_HPP_

#include <list>
#include <map>
#include <set>
#include <string>

#include <gdkmm/dragcontext.h>
#include <gdkmm/pixbuf.h>
#include <gtkmm/cellrenderer.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/comboboxentry.h>
#include <gtkmm/liststore.h>
#include <gtkmm/menubar.h>
#include <gtkmm/paned.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/selectiondata.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/treemodelfilter.h>
#include <gtkmm/treemodelsort.h>
#include <gtkmm/treepath.h>
#include <gtkmm/treeview.h>

#include "note.hpp"
#include "tag.hpp"
#include "utils.hpp"
#include "notebooks/notebook.hpp"

namespace gnote {
  namespace notebooks {
    class NotebooksTreeView;
  }
  class NoteManager;

class NoteRecentChanges
  : public utils::ForcedPresentWindow
{
public:
  static NoteRecentChanges *get_instance(NoteManager& m);

  void set_search_text(const std::string & value);

//////

protected:
  NoteRecentChanges(NoteManager& m);
  virtual void on_show();
  ///
private:

private:
  static void _init_static();
  Gtk::MenuBar *create_menu_bar ();
  Gtk::Widget  *make_notebooks_pane();
  void make_recent_tree ();
  void update_results();
  void select_notes(const Note::List &);
  static void scroll_to_iter (Gtk::TreeView & tree, const Gtk::TreeIter & iter);
  void perform_search ();
  void add_matches_column ();
  void remove_matches_column ();
  void matches_column_data_func(Gtk::CellRenderer *, const Gtk::TreeIter &);
  void update_total_note_count (int);
  void update_match_note_count (int);
  bool filter_notes(const Gtk::TreeIter & );
  bool filter_tags(const Gtk::TreeIter & );
  bool filter_by_tag (const Note::Ptr &);
  bool filter_by_search(const Note::Ptr &);
  void on_case_sensitive_toggled();
  void on_notes_changed(const Note::Ptr &);
  void on_note_renamed(const Note::Ptr&, const std::string&);
  void on_note_saved(const Note::Ptr&);
  void on_treeview_drag_data_get(const Glib::RefPtr<Gdk::DragContext> &,
                                 Gtk::SelectionData &, guint, guint);
  void on_selection_changed();
  bool on_treeview_button_pressed(GdkEventButton *);
  bool on_treeview_motion_notify(GdkEventMotion *);
  bool on_treeview_button_released(GdkEventButton *);  
  bool on_treeview_key_pressed(GdkEventKey *);
  void popup_context_menu_at_location(Gtk::Menu *, int, int);
  void position_context_menu(int & x, int & y, bool & push_in);
  Note::List get_selected_notes();
  Note::Ptr get_note(const Gtk::TreePath & p);
  void on_open_note();
  void on_delete_note();
  void on_close_window();
  bool on_delete(GdkEventAny *);
  bool on_key_pressed(GdkEventKey *);
  int compare_titles(const Gtk::TreeIter & , const Gtk::TreeIter &);
  int compare_dates(const Gtk::TreeIter & , const Gtk::TreeIter &);
  int compare_search_hits(const Gtk::TreeIter & , const Gtk::TreeIter &);
  void on_row_activated(const Gtk::TreePath & , Gtk::TreeViewColumn*);
  void on_entry_activated();
  void on_entry_changed();
  void entry_changed_timeout();
  void add_to_previous_searches(const std::string &);
  void clear_search_clicked();
  void notebook_pixbuf_cell_data_func(Gtk::CellRenderer *, const Gtk::TreeIter &);
  void notebook_text_cell_data_func(Gtk::CellRenderer *, const Gtk::TreeIter &);
  void on_notebook_row_edited(const Glib::ustring& path, const Glib::ustring& new_text);
  void on_notebook_selection_changed();
  void on_new_notebook();
  void on_delete_notebook();
  void on_notebook_row_activated(const Gtk::TreePath & , Gtk::TreeViewColumn*);
  void on_new_notebook_note();
  void on_open_notebook_template_note();
  notebooks::Notebook::Ptr get_selected_notebook ();
  void select_all_notes_notebook ();
  bool on_notebooks_tree_button_pressed(GdkEventButton *);
  bool on_notebooks_key_pressed(GdkEventKey *);
  void on_note_added_to_notebook (const Note & note, const notebooks::Notebook::Ptr & notebook);
  void on_note_removed_to_notebook (const Note & note, const notebooks::Notebook::Ptr & notebook);
  std::string get_search_text();
  void save_position ();
  void restore_position();
  bool on_exiting_event();

  class RecentNotesColumnTypes
    : public Gtk::TreeModelColumnRecord
  {
  public:
    RecentNotesColumnTypes()
      {
        add(icon); add(title); add(change_date); add(note);
      }

    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
    Gtk::TreeModelColumn<std::string>                title;
    Gtk::TreeModelColumn<std::string>                change_date;
    Gtk::TreeModelColumn<Note::Ptr>                  note;
  };
  class RecentSearchColumnTypes
    : public Gtk::TreeModelColumnRecord
  {
  public:
    RecentSearchColumnTypes()
      {
        add(text);
      }
    Gtk::TreeModelColumn<std::string> text;
  };

  NoteManager &       m_manager;
  Gtk::MenuBar       *m_menubar;
  RecentSearchColumnTypes m_find_combo_columns;
  Gtk::ComboBoxEntry  m_find_combo;
  Gtk::Button         m_clear_search_button;
  Gtk::CheckButton    m_case_sensitive;
  Gtk::Statusbar      m_status_bar;
  Gtk::ScrolledWindow m_matches_window;
  Gtk::HPaned         m_hpaned;
  Gtk::VBox           m_content_vbox;
  Gtk::TreeViewColumn *m_matches_column;
        
  notebooks::NotebooksTreeView  *m_notebooksTree;
        
  // Use the following like a Set
  std::set<Tag::Ptr>  m_selected_tags;

  Gtk::TreeView      *m_tree;
  Glib::RefPtr<Gtk::ListStore>       m_store;
  Glib::RefPtr<Gtk::TreeModelFilter> m_store_filter;
  Glib::RefPtr<Gtk::TreeModelSort>   m_store_sort;

  /// <summary>
  /// Stores search results as integers hashed by note uri.
  /// </summary>
  std::map<std::string, int> m_current_matches;

  utils::InterruptableTimeout *m_entry_changed_timeout;
                
  std::vector<Gtk::TargetEntry>     m_targets;
  int                  m_clickX, m_clickY;
  RecentNotesColumnTypes m_column_types;
  sigc::connection                  m_on_notebook_selection_changed_cid;

  static bool        s_static_inited;
  static Glib::RefPtr<Gdk::Pixbuf> s_note_icon;
  static Glib::RefPtr<Gdk::Pixbuf> s_all_notes_icon;
  static Glib::RefPtr<Gdk::Pixbuf> s_unfiled_notes_icon;
  static Glib::RefPtr<Gdk::Pixbuf> s_notebook_icon;
  static std::list<std::string>    s_previous_searches;
  static NoteRecentChanges        *s_instance;
};


}

#endif
