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



#ifndef __BUGZILLA_PREFERENCES_HPP_
#define __BUGZILLA_PREFERENCES_HPP_

#include <gdkmm/pixbuf.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/treeview.h>

namespace sharp {

  class FileInfo;

}

namespace bugzilla {


class BugzillaPreferences
  : public Gtk::VBox
{
public:
  BugzillaPreferences();

protected:
  virtual void on_realize();

private:
  void update_icon_store();
  std::string parse_host(const sharp::FileInfo &);
  void selection_changed();
  void add_clicked();
	bool copy_to_bugzilla_icons_dir(const std::string & file_path,
                                  const std::string & host,
                                  std::string & err_msg);
  void resize_if_needed(const std::string & path);
  void remove_clicked();

  class Columns
    : public Gtk::TreeModelColumnRecord
  {
  public:
    Columns()
      { add(icon); add(host); add(file_path); }
    Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
    Gtk::TreeModelColumn<std::string>                host;
    Gtk::TreeModelColumn<std::string>                file_path;
  };

  Columns        m_columns;
  Gtk::TreeView *icon_tree;
  Glib::RefPtr<Gtk::ListStore> icon_store;

  Gtk::Button *add_button;
  Gtk::Button *remove_button;

  std::string last_opened_dir;

  void _init_static();
  static bool        s_static_inited;
  static std::string s_image_dir;
};

}

#endif
