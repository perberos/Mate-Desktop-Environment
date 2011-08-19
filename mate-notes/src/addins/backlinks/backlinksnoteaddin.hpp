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

#ifndef __BACKLINKS_NOTEADDIN_HPP_
#define __BACKLINKS_NOTEADDIN_HPP_

#include <list>

#include <gtkmm/imagemenuitem.h>
#include <gtkmm/menu.h>

#include "sharp/dynamicmodule.hpp"
#include "note.hpp"
#include "noteaddin.hpp"

namespace backlinks {

class BacklinksModule
  : public sharp::DynamicModule
{
public:
  BacklinksModule();
  virtual const char * id() const;
  virtual const char * name() const;
  virtual const char * description() const;
  virtual const char * authors() const;
  virtual int          category() const;
  virtual const char * version() const;
};

DECLARE_MODULE(BacklinksModule);

class BacklinkMenuItem;

class BacklinksNoteAddin
  : public gnote::NoteAddin
{
public:
  static BacklinksNoteAddin *create()
    {
      return new BacklinksNoteAddin;
    }
  BacklinksNoteAddin();

  virtual void initialize ();
  virtual void shutdown ();
  virtual void on_note_opened ();
private:
  void on_menu_item_activated();
  void on_menu_hidden();
  void update_menu();
  void get_backlink_menu_items(std::list<BacklinkMenuItem*> & items);
  bool check_note_has_match(const gnote::Note::Ptr &, const std::string &);
  Gtk::ImageMenuItem *m_menu_item;
  Gtk::Menu          *m_menu;
  bool                m_submenu_built;
};

}

#endif

