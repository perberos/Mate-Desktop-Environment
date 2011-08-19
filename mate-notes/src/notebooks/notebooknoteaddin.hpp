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




#ifndef __NOTEBOOKS_NOTEBOOK_NOTE_ADDIN_HPP__
#define __NOTEBOOKS_NOTEBOOK_NOTE_ADDIN_HPP__

#include <gtkmm/menu.h>
#include <gtkmm/menutoolbutton.h>

#include "noteaddin.hpp"
#include "notebooks/notebook.hpp"
#include "notebooks/notebookmenuitem.hpp"
#include "note.hpp"
#include "utils.hpp"

namespace gnote {
namespace notebooks {

  class NotebookNoteAddin
    : public NoteAddin
  {
  public:
    static NoteAddin * create();    
    virtual void initialize ();
    virtual void shutdown ();
    virtual void on_note_opened ();

  protected:
    NotebookNoteAddin();

  private:
    void initialize_tool_button();
    void on_menu_shown();
    void on_note_added_to_notebook(const Note &, const Notebook::Ptr &);
    void on_note_removed_from_notebook(const Note &, const Notebook::Ptr &);
    void on_new_notebook_menu_item();
    void update_notebook_button_label();
    void update_notebook_button_label(const Notebook::Ptr &);
    void update_menu();
    void get_notebook_menu_items(std::list<NotebookMenuItem*> &);
    gnote::utils::ToolMenuButton  *m_toolButton;
    Gtk::Menu                *m_menu;
    std::list<Gtk::MenuItem *> m_menu_items;
    Gtk::RadioButtonGroup     m_radio_group;
    sigc::connection          m_show_menu_cid;
    sigc::connection          m_note_added_cid;
    sigc::connection          m_note_removed_cid;

    static void _init_static();
    static bool               s_static_inited;
    static Glib::RefPtr<Gdk::Pixbuf> s_notebookIcon;
    static Glib::RefPtr<Gdk::Pixbuf> s_newNotebookIcon;
  };

}
}


#endif
