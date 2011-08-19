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




#ifndef __NOTEBOOK_MENUITEM_HPP__
#define __NOTEBOOK_MENUITEM_HPP__

#include <gtkmm/radiomenuitem.h>

#include "note.hpp"
#include "notebooks/notebook.hpp"


namespace gnote {
  namespace notebooks {

    class NotebookMenuItem
      : public Gtk::RadioMenuItem
    {
    public:
      NotebookMenuItem(Gtk::RadioButtonGroup & group, const Note::Ptr &, const Notebook::Ptr &);

      const Note::Ptr & get_note() const
        {
          return m_note;
        }
      const Notebook::Ptr & get_notebook() const
        {
          return m_notebook;
        }
      // the menu item is comparable.
      bool operator==(const NotebookMenuItem &);
      bool operator<(const NotebookMenuItem &);
      bool operator>(const NotebookMenuItem &);
    private:
      void on_activated();

      Note::Ptr m_note;
      Notebook::Ptr m_notebook;
    };

  }
}

#endif
