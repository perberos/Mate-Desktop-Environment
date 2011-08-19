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




#ifndef __NOTEBOOK_NEW_NOTEMNUITEM_HPP__
#define __NOTEBOOK_NEW_NOTEMNUITEM_HPP__


#include <gtkmm/imagemenuitem.h>

#include "notebooks/notebook.hpp"

namespace gnote {
  namespace notebooks {

    class NotebookMenuItem;

class NotebookNewNoteMenuItem
  : public Gtk::ImageMenuItem
{
public:
  NotebookNewNoteMenuItem(const Notebook::Ptr &);
  void on_activated();
  Notebook::Ptr get_notebook() const
    {
      return m_notebook;
    }
  // the menu item is comparable.
  bool operator==(const NotebookMenuItem &);
  bool operator<(const NotebookMenuItem &);
  bool operator>(const NotebookMenuItem &);
private:
  Notebook::Ptr m_notebook;
};


  }
}

#endif
