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



#ifndef __NOTEBOOK_APPLICATION_ADDIN_HPP__
#define __NOTEBOOK_APPLICATION_ADDIN_HPP__

#include <list>

#include <gdkmm/pixbuf.h>
#include <gtkmm/actiongroup.h>

#include "applicationaddin.hpp"
#include "note.hpp"

namespace gnote {
  namespace notebooks {


    class NotebookApplicationAddin
      : public ApplicationAddin
    {
    public:
      static ApplicationAddin * create();
      virtual void initialize ();
      virtual void shutdown ();
      virtual bool initialized ();

    protected:
      NotebookApplicationAddin();
    private:
      void on_tray_notebook_menu_shown();
      void on_tray_notebook_menu_hidden();
      void on_new_notebook_menu_shown();
      void on_new_notebook_menu_hidden();
      void add_menu_items(Gtk::Menu *, std::list<Gtk::MenuItem*> & menu_items);
      void remove_menu_items(Gtk::Menu *, std::list<Gtk::MenuItem*> & menu_items);
      void on_new_notebook_menu_item();
      void on_tag_added(const Note&, const Tag::Ptr&);
      void on_tag_removed(const Note::Ptr&, const std::string&);
      void on_note_added(const Note::Ptr &);
      void on_note_deleted(const Note::Ptr &);

      bool m_initialized;
      guint m_notebookUi;
      Glib::RefPtr<Gtk::ActionGroup> m_actionGroup;
      Glib::RefPtr<Gdk::Pixbuf>      m_notebookIcon;
      Glib::RefPtr<Gdk::Pixbuf>      m_newNotebookIcon;
      Gtk::Menu                     *m_trayNotebookMenu;
      std::list<Gtk::MenuItem*>      m_trayNotebookMenuItems;
      Gtk::Menu                     *m_mainWindowNotebookMenu;
      std::list<Gtk::MenuItem*>      m_mainWindowNotebookMenuItems;
    };


  }
}


#endif

