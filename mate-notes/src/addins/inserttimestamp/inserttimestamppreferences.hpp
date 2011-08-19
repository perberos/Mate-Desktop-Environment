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


#ifndef __INSERTTIMESTAMP_PREFERENCES_HPP_
#define __INSERTTIMESTAMP_PREFERENCES_HPP_


#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/liststore.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>


namespace inserttimestamp {

class InsertTimestampPreferences
  : public Gtk::VBox
{
public:
  InsertTimestampPreferences();
private:
  static void _init_static();
  class FormatColumns
    : public Gtk::TreeModelColumnRecord
  {
  public:
    FormatColumns()
      { add(formatted); add(format); }

    Gtk::TreeModelColumn<std::string> formatted;
    Gtk::TreeModelColumn<std::string> format;
  };
  void on_selected_radio_toggled();
  void on_selection_changed();

  static bool       s_static_inited;
  static std::vector<std::string> s_formats;
  FormatColumns     m_columns;
  Gtk::RadioButton *selected_radio;
  Gtk::RadioButton *custom_radio;

  Gtk::ScrolledWindow         *scroll;
  Gtk::TreeView               *tv;
  Glib::RefPtr<Gtk::ListStore> store;
  Gtk::Entry                  *custom_entry;
};


}

#endif
