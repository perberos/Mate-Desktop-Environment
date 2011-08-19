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





#ifndef __KEYBINDER_HPP_
#define __KEYBINDER_HPP_

#include <string>

#include <sigc++/slot.h>
#include <gdkmm/types.h>


namespace gnote {


class IKeybinder
{
public:
  virtual void bind(const std::string & keystring, const sigc::slot<void> & handler) = 0;
  virtual void unbind(const std::string & keystring) = 0;
  virtual void unbind_all() = 0;
  virtual bool get_accel_keys(const std::string & prefs_path, guint & keyval, 
                              Gdk::ModifierType & mods) = 0;
};


}


#endif
