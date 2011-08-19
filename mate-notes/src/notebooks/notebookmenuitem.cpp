/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
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



#include <glibmm/i18n.h>

#include "notebooks/notebookmenuitem.hpp"
#include "notebooks/notebookmanager.hpp"



namespace gnote {
  namespace notebooks {

    NotebookMenuItem::NotebookMenuItem(Gtk::RadioButtonGroup & group, 
                                       const Note::Ptr & note, const Notebook::Ptr & notebook)
      : Gtk::RadioMenuItem(group, notebook ? notebook->get_name() : _("No notebook"))
      , m_note(note)
      , m_notebook(notebook)
    {
      signal_activate().connect(sigc::mem_fun(*this, &NotebookMenuItem::on_activated));
    }


    void NotebookMenuItem::on_activated()
    {
      if(!m_note) {
        return;
      }

      NotebookManager::instance().move_note_to_notebook(m_note, m_notebook);
    }

    // the menu item is comparable.
    bool NotebookMenuItem::operator==(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() == rhs.m_notebook->get_name();
    }


    bool NotebookMenuItem::operator<(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() < rhs.m_notebook->get_name();
    }


    bool NotebookMenuItem::operator>(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() > rhs.m_notebook->get_name();
    }

  }
}
