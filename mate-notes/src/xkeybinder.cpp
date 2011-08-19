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



#include "libtomboy/eggaccelerators.h"
#include "libtomboy/tomboykeybinder.h"

#include "preferences.hpp"
#include "xkeybinder.hpp"

namespace gnote {


  XKeybinder::XKeybinder()
  {
    tomboy_keybinder_init ();
  }


  void XKeybinder::keybinding_pressed(char *keystring, gpointer user_data)
  {
    XKeybinder * self = static_cast<XKeybinder*>(user_data);
    BindingMap::iterator iter = self->m_bindings.find(keystring);
    if(iter != self->m_bindings.end()) {
      iter->second();
    }
  }

  void XKeybinder::bind(const std::string & keystring, const sigc::slot<void> & handler)
  {
    m_bindings[keystring] = handler;
    tomboy_keybinder_bind(keystring.c_str(), &XKeybinder::keybinding_pressed, this);
  }


  void XKeybinder::unbind(const std::string & keystring)
  {
    BindingMap::iterator iter = m_bindings.find(keystring);
    if(iter != m_bindings.end()) {
      tomboy_keybinder_unbind(keystring.c_str(), &XKeybinder::keybinding_pressed);
      m_bindings.erase(iter);
    }
  }


  void XKeybinder::unbind_all()
  {
    for(BindingMap::const_iterator iter = m_bindings.begin();
        iter != m_bindings.end(); ++iter) {
      tomboy_keybinder_unbind (iter->first.c_str(), &XKeybinder::keybinding_pressed);
    }
    m_bindings.clear();
  }


  bool XKeybinder::get_accel_keys(const std::string & mateconf_path, guint & keyval, 
                              Gdk::ModifierType & mods)
  {
    keyval = 0;
    mods = (Gdk::ModifierType)0;

    try {
      std::string binding = Preferences::obj().get<std::string> (mateconf_path);
      if (binding.empty() || binding == "disabled") {
        return false;
      }

      EggVirtualModifierType virtual_mods = (EggVirtualModifierType)0;
      if (!egg_accelerator_parse_virtual (binding.c_str(),
                                          &keyval,
                                          &virtual_mods)) {
        return false;
      }

      GdkKeymap *keymap = gdk_keymap_get_default();
      GdkModifierType pmods = (GdkModifierType)0;
      egg_keymap_resolve_virtual_modifiers (keymap,
                                            virtual_mods,
                                            &pmods);
      mods = (Gdk::ModifierType)pmods;
    } 
    catch  (...) {
      return false;
    }
    return true;
  }

}
