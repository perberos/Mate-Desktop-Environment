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





#ifndef __INSERTIMESTAMP_NOTEADDIN_HPP_
#define __INSERTIMESTAMP_NOTEADDIN_HPP_

#include <gtkmm/menuitem.h>

#include "sharp/dynamicmodule.hpp"
#include "noteaddin.hpp"
#include "preferences.hpp"

namespace inserttimestamp {

class InsertTimeStampModule
  : public sharp::DynamicModule
{
public:
  InsertTimeStampModule();
  virtual const char * id() const;
  virtual const char * name() const;
  virtual const char * description() const;
  virtual const char * authors() const;
  virtual int          category() const;
  virtual const char * version() const;
};


DECLARE_MODULE(InsertTimeStampModule);


class InsertTimestampNoteAddin
  : public gnote::NoteAddin 
{
public:
  static InsertTimestampNoteAddin* create()
    {
      return new InsertTimestampNoteAddin;
    }
  virtual void initialize();
  virtual void shutdown();
  virtual void on_note_opened();
private:
  void on_menu_item_activated();
  void on_format_setting_changed(gnote::Preferences*, MateConfEntry*);

  std::string    m_date_format;
  Gtk::MenuItem *m_item;
};

}

#endif
