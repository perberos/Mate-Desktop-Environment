/*
 * gnote
 *
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




#ifndef _NOTEBOOK_MANAGER_HPP__
#define _NOTEBOOK_MANAGER_HPP__

#include <sigc++/signal.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treemodelsort.h>
#include <gtkmm/treemodelfilter.h>

#include "notebooks/notebook.hpp"
#include "note.hpp"
#include "tag.hpp"

namespace gnote {
  namespace notebooks {


class NotebookManager
{
public:

  static NotebookManager & instance()
    {
      static NotebookManager *s_instance = new NotebookManager();
      return *s_instance;
    }

  typedef sigc::signal<void, const Note &, const Notebook::Ptr &> NotebookEventHandler;
  bool is_adding_notebook() const
    {
      return m_adding_notebook;
    }

  Glib::RefPtr<Gtk::TreeModel> get_notebooks()
    { return m_filteredNotebooks; }
  /// <summary>
  /// A Gtk.TreeModel that contains all of the items in the
  /// NotebookManager TreeStore including SpecialNotebooks
  /// which are used in the "Search All Notes" window.
  /// </summary>
  /// <param name="notebookName">
  /// A <see cref="System.String"/>
  /// </param>
  /// <returns>
  /// A <see cref="Notebook"/>
  /// </returns>
  Glib::RefPtr<Gtk::TreeModel> get_notebooks_with_special_items()
    { return m_sortedNotebooks; }

  Notebook::Ptr get_notebook(const std::string & notebookName) const;
  bool notebook_exists(const std::string & notebookName) const;
  Notebook::Ptr get_or_create_notebook(const std::string &);
  void delete_notebook(const Notebook::Ptr &);
  bool get_notebook_iter(const Notebook::Ptr &, Gtk::TreeIter & );
  Notebook::Ptr get_notebook_from_note(const Note::Ptr &);
  Notebook::Ptr get_notebook_from_tag(const Tag::Ptr &);
  static bool is_notebook_tag(const Tag::Ptr &);
  static Notebook::Ptr prompt_create_new_notebook(Gtk::Window *);
  static Notebook::Ptr prompt_create_new_notebook(Gtk::Window *,
                                                  const Note::List & notesToAdd);
  static void prompt_delete_notebook(Gtk::Window *, const Notebook::Ptr &);
  bool move_note_to_notebook (const Note::Ptr &, const Notebook::Ptr &);

  NotebookEventHandler & signal_note_added_to_notebook()
    { return m_note_added_to_notebook; }

  NotebookEventHandler & signal_note_removed_from_notebook()
    { return m_note_removed_from_notebook; }

private:
  NotebookManager();

  static int compare_notebooks_sort_func(const Gtk::TreeIter &, const Gtk::TreeIter &);
  void load_notebooks();
  static bool filter_notebooks(const Gtk::TreeIter &);

  class ColumnRecord
    : public Gtk::TreeModelColumnRecord
  {
  public:
    ColumnRecord()
      { add(m_col1); }
    Gtk::TreeModelColumn<Notebook::Ptr> m_col1;
  };

  ColumnRecord                         m_column_types;
  Glib::RefPtr<Gtk::ListStore>         m_notebooks;
  Glib::RefPtr<Gtk::TreeModelSort>     m_sortedNotebooks;
  Glib::RefPtr<Gtk::TreeModelFilter>   m_filteredNotebooks;
  // <summary>
  // The key for this dictionary is Notebook.Name.ToLower ().
  // </summary>
  std::map<std::string, Gtk::TreeIter> m_notebookMap;
  //object locker = new object ();    
  bool                                 m_adding_notebook;
  NotebookEventHandler                 m_note_added_to_notebook;
  NotebookEventHandler                 m_note_removed_from_notebook;
};

  }
}

#endif
