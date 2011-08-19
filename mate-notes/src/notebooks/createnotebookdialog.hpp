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




#ifndef __NOTEBOOK_CREATENOTEBOOKDIALOG_HPP_
#define __NOTEBOOK_CREATENOTEBOOKDIALOG_HPP_

#include <gdkmm/pixbuf.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>

#include "utils.hpp"

namespace gnote {
  namespace notebooks {


class CreateNotebookDialog
  : public utils::HIGMessageDialog
{
public:
  CreateNotebookDialog(Gtk::Window *parent, GtkDialogFlags f);

  std::string get_notebook_name();
  void set_notebook_name(const std::string &);

private:
  void on_name_entry_changed();
  Gtk::Entry                m_nameEntry;
  Gtk::Label                m_errorLabel;
  Glib::RefPtr<Gdk::Pixbuf> m_newNotebookIcon;
  Glib::RefPtr<Gdk::Pixbuf> m_newNotebookIconDialog;
};

  }
}


#endif
