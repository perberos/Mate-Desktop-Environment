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


#include "notewindow.hpp"

#include "backlinkmenuitem.hpp"

namespace backlinks {

Glib::RefPtr<Gdk::Pixbuf> BacklinkMenuItem::s_note_icon;


const Glib::RefPtr<Gdk::Pixbuf> & BacklinkMenuItem::get_note_icon()
{
  if(!s_note_icon) {
    s_note_icon = gnote::utils::get_icon("note", 16);
  }
  return s_note_icon;
}


BacklinkMenuItem::BacklinkMenuItem(const gnote::Note::Ptr & note,
                                   const std::string & title_search)
  : Gtk::ImageMenuItem(note->get_title())
  , m_note(note)
  , m_title_search(title_search)
{
  set_image(*manage(new Gtk::Image(get_note_icon())));
}


void BacklinkMenuItem::on_activate()
{
  if (!m_note) {
    return;
  }

  // Show the title of the note
  // where the user just came from.
  gnote::NoteFindBar & find = m_note->get_window()->get_find_bar();
  find.show_all ();
  find.property_visible() = true;
  find.set_search_text(m_title_search);
  
  m_note->get_window()->present ();
}


}
