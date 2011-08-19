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




#ifndef _BUGZILLA_NOTE_ADDIN_HPP__
#define _BUGZILLA_NOTE_ADDIN_HPP__


#include "sharp/dynamicmodule.hpp"
#include "noteaddin.hpp"

namespace bugzilla {


class BugzillaModule
  : public sharp::DynamicModule
{
public:
  BugzillaModule();
  virtual const char * id() const;
  virtual const char * name() const;
  virtual const char * description() const;
  virtual const char * authors() const;
  virtual int category() const;
  virtual const char * version() const;
};

class BugzillaNoteAddin
  : public gnote::NoteAddin
{
public:
  static BugzillaNoteAddin* create()
    {
      return new BugzillaNoteAddin;
    }
  static std::string images_dir();
  virtual void initialize();
  virtual void shutdown();
  virtual void on_note_opened();
private:
  BugzillaNoteAddin();
  void migrate_images(const std::string & old_images_dir);

  static const char * TAG_NAME;

  void on_drag_data_received(const Glib::RefPtr<Gdk::DragContext>&, int, int, const Gtk::SelectionData &,
                             guint, guint);
  void drop_uri_list(const Glib::RefPtr<Gdk::DragContext>& context, int x, int y, 
                     const Gtk::SelectionData & selection_data, guint time);

  bool insert_bug (int x, int y, const std::string & uri, int id);

};


}


#endif
