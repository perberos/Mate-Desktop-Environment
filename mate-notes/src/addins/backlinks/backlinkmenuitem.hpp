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


#ifndef __BACKLINK_MENU_ITEM_HPP_
#define __BACKLINK_MENU_ITEM_HPP_

#include <string>
#include <gtkmm/imagemenuitem.h>

#include "note.hpp"

namespace backlinks {

class BacklinkMenuItem
  : public Gtk::ImageMenuItem
{
public:
  BacklinkMenuItem(const gnote::Note::Ptr &, const std::string &);
  
  gnote::Note::Ptr get_note()
    { return m_note; }
protected:
  virtual void on_activate();
private:
  gnote::Note::Ptr   m_note;
  std::string m_title_search;
  
  static const Glib::RefPtr<Gdk::Pixbuf> & get_note_icon();
  static Glib::RefPtr<Gdk::Pixbuf> s_note_icon;
};

}

#endif
