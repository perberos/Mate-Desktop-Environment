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



#ifndef __UNDERLINE_MENUITEM_HPP_
#define __UNDERLINE_MENUITEM_HPP_

#include <gtkmm/checkmenuitem.h>

namespace gnote {
  class NoteAddin;
}


namespace underline {


class UnderlineMenuItem
  : public Gtk::CheckMenuItem
{
public:
  UnderlineMenuItem(gnote::NoteAddin *);
  
protected:
  virtual void on_activate();

private:
  void menu_shown();

  gnote::NoteAddin * m_note_addin;
  bool m_event_freeze;
};



}


#endif
