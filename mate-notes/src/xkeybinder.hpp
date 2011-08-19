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



#ifndef __XKEYBINDER_HPP_
#define __XKEYBINDER_HPP_

#include <map>

#include <sigc++/slot.h>

#include "keybinder.hpp"

namespace gnote {

class XKeybinder
  : public IKeybinder
{
public:
  XKeybinder();
  virtual void bind(const std::string & keystring, const sigc::slot<void> & handler);
  virtual void unbind(const std::string & keystring);
  virtual void unbind_all();
  virtual bool get_accel_keys(const std::string & prefs_path, guint & keyval, 
                              Gdk::ModifierType & mods);
private:
  static void keybinding_pressed(char *keystring, gpointer user_data);

  typedef sigc::slot<void> Handler;
  typedef std::map<std::string, Handler> BindingMap;
  BindingMap m_bindings;
  
};


}

#endif
