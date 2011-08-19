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



#include <boost/format.hpp>
#include <glibmm/i18n.h>
#include <gtkmm/image.h>

#include "sharp/string.hpp"
#include "gnote.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "notebooks/notebookmenuitem.hpp"
#include "notebooks/notebooknewnotemenuitem.hpp"

namespace gnote {
  namespace notebooks {

    NotebookNewNoteMenuItem::NotebookNewNoteMenuItem(const Notebook::Ptr & notebook)
      : Gtk::ImageMenuItem(str(boost::format(_("New \"%1%\" Note")) % notebook->get_name()))
      , m_notebook(notebook)
    {
      set_image(*manage(new Gtk::Image(utils::get_icon("note-new", 16))));
      signal_activate().connect(sigc::mem_fun(*this, &NotebookNewNoteMenuItem::on_activated));
    }


    
    void NotebookNewNoteMenuItem::on_activated()
    {
      if (!m_notebook) {
        return;
      }
      
      // Look for the template note and create a new note
      Note::Ptr note = m_notebook->create_notebook_note ();
      note->get_window()->show ();
    }


    bool NotebookNewNoteMenuItem::operator==(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() == rhs.get_notebook()->get_name();
    }


    bool NotebookNewNoteMenuItem::operator<(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() < rhs.get_notebook()->get_name();
    }


    bool NotebookNewNoteMenuItem::operator>(const NotebookMenuItem & rhs)
    {
      return m_notebook->get_name() > rhs.get_notebook()->get_name();
    }

  }
}
