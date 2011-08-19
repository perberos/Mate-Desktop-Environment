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



#ifndef _PREFSKEYBINDER_HPP_
#define _PREFSKEYBINDER_HPP_

#include <string>
#include <sigc++/signal.h>
#include <sigc++/slot.h>

#include "preferences.hpp"

namespace gnote {

class TrayIcon;
class NoteManager;
class IGnoteTray;
class IKeybinder;


class PrefsKeybinder
{
public:
  PrefsKeybinder();
  virtual ~PrefsKeybinder();
  void bind(const std::string & pref_path, const std::string & default_binding, 
            const sigc::slot<void> & handler);
  void unbind_all();

private:
  class Binding;

  IKeybinder & m_native_keybinder;
  std::list<Binding*> m_bindings;
};

class GnotePrefsKeybinder
  : public PrefsKeybinder
{
public:
  GnotePrefsKeybinder(NoteManager & manager, IGnoteTray & trayicon);
  ~GnotePrefsKeybinder();
  void enable_keybindings_changed(Preferences*, MateConfEntry* entry);
  void enable_disable(bool enable);
  void bind_preference(const std::string & pref_path, const sigc::slot<void> & handler);
private:
  void key_show_menu();
  void key_openstart_here();
  void key_create_new_note();
  void key_open_search();
  void key_open_recent_changes();
  NoteManager & m_manager;
  IGnoteTray & m_trayicon;
  sigc::connection m_prefs_cid;
};


}

#endif
