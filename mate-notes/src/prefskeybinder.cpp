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



#include "debug.hpp"
#include "gnote.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferences.hpp"
#include "prefskeybinder.hpp"
#include "recentchanges.hpp"

namespace gnote {


  class PrefsKeybinder::Binding
  {
  public:
    Binding(const std::string & pref_path, const std::string & default_binding,
            const sigc::slot<void> & handler, IKeybinder & native_keybinder);
    ~Binding();
    void remove_notify();
    void set_binding();
    void unset_binding();
  private:

    static void on_binding_changed_mateconf(MateConfClient *, guint , 
                                         MateConfEntry* entry, gpointer data);

    std::string m_pref_path;
    std::string m_key_sequence;
    sigc::slot<void> m_handler;
    IKeybinder  & m_native_keybinder;
    guint         m_notify_cnx;
  };

  PrefsKeybinder::Binding::Binding(const std::string & pref_path, 
                                   const std::string & default_binding,
                                   const sigc::slot<void> & handler, 
                                   IKeybinder & native_keybinder)
    : m_pref_path(pref_path)
    , m_key_sequence(default_binding)
    , m_handler(handler)
    , m_native_keybinder(native_keybinder)
    , m_notify_cnx(0)
  {
    m_key_sequence = Preferences::obj().get<std::string>(pref_path);
    set_binding();
    m_notify_cnx = Preferences::obj()
      .add_notify(pref_path.c_str(), &PrefsKeybinder::Binding::on_binding_changed_mateconf, 
                  this);
  }

  PrefsKeybinder::Binding::~Binding()
  {
    remove_notify();
  }


  void PrefsKeybinder::Binding::remove_notify()
  {
    if(m_notify_cnx) {
      Preferences::obj().remove_notify(m_notify_cnx);
      m_notify_cnx = 0;
    }
  }

  void PrefsKeybinder::Binding::on_binding_changed_mateconf(MateConfClient *, guint , 
                                                         MateConfEntry* entry, gpointer data)
  {
    Binding* self = static_cast<Binding*>(data);
    if(self == NULL) {
      return;
    }
    const char * key = mateconf_entry_get_key(entry);
    if (key == self->m_pref_path) {
      MateConfValue *value = mateconf_entry_get_value(entry);
      const char *string_val = mateconf_value_get_string(value);
      DBG_OUT("Binding for '%s' changed to '%s'!",
              self->m_pref_path.c_str(), string_val);

      self->unset_binding ();

      self->m_key_sequence = string_val;
      self->set_binding ();
    }
  }


  void PrefsKeybinder::Binding::set_binding()
  {
    if(m_key_sequence.empty() || (m_key_sequence == "disabled")) {
      return;
    }
    DBG_OUT("Binding key '%s' for '%s'",
            m_key_sequence.c_str(), m_pref_path.c_str());

    m_native_keybinder.bind (m_key_sequence, m_handler);
  }

  void PrefsKeybinder::Binding::unset_binding()
  {
    if(m_key_sequence.empty()) {
      return;
    }
    DBG_OUT("Unbinding key  '%s' for '%s'",
            m_key_sequence.c_str(), m_pref_path.c_str());

    m_native_keybinder.unbind (m_key_sequence);
  }
  

  PrefsKeybinder::PrefsKeybinder()
    : m_native_keybinder(Gnote::obj().keybinder())
  {
  }


  PrefsKeybinder::~PrefsKeybinder()
  {

  }


  void PrefsKeybinder::bind(const std::string & pref_path, const std::string & default_binding, 
                            const sigc::slot<void> & handler)
  {
    m_bindings.push_back(new Binding(pref_path, default_binding,
                                     handler, m_native_keybinder));
  }

  void PrefsKeybinder::unbind_all()
  {
    for(std::list<Binding*>::const_iterator iter = m_bindings.begin();
        iter != m_bindings.end(); ++iter) {
      delete *iter;
    }
    m_bindings.clear ();
    m_native_keybinder.unbind_all ();
  }


  GnotePrefsKeybinder::GnotePrefsKeybinder(NoteManager & manager, IGnoteTray & trayicon)
    : m_manager(manager)
    , m_trayicon(trayicon)
  {
    enable_disable(Preferences::obj().get<bool>(Preferences::ENABLE_KEYBINDINGS));
    m_prefs_cid = Preferences::obj().signal_setting_changed()
      .connect(sigc::mem_fun(*this, &GnotePrefsKeybinder::enable_keybindings_changed));
  }


  GnotePrefsKeybinder::~GnotePrefsKeybinder()
  {
    m_prefs_cid.disconnect();
  }


  void GnotePrefsKeybinder::enable_keybindings_changed(Preferences*, MateConfEntry* entry)
  {
    if(mateconf_entry_get_key(entry) == Preferences::ENABLE_KEYBINDINGS) {
      MateConfValue *value = mateconf_entry_get_value(entry);
      
      bool enabled = mateconf_value_get_bool(value);
      enable_disable(enabled);
    }
  }


  void GnotePrefsKeybinder::enable_disable(bool enable)
  {
    DBG_OUT("EnableDisable Called: enabling... %s", enable ? "true" : "false");
    if(enable) {
      bind_preference (Preferences::KEYBINDING_SHOW_NOTE_MENU,
                       sigc::mem_fun(*this, &GnotePrefsKeybinder::key_show_menu));

      bind_preference (Preferences::KEYBINDING_OPEN_START_HERE,
                       sigc::mem_fun(*this, &GnotePrefsKeybinder::key_openstart_here));

      bind_preference (Preferences::KEYBINDING_CREATE_NEW_NOTE,
                       sigc::mem_fun(*this, &GnotePrefsKeybinder::key_create_new_note));

      bind_preference (Preferences::KEYBINDING_OPEN_SEARCH,
                       sigc::mem_fun(*this, &GnotePrefsKeybinder::key_open_search));

      bind_preference (Preferences::KEYBINDING_OPEN_RECENT_CHANGES,
                       sigc::mem_fun(*this, &GnotePrefsKeybinder::key_open_recent_changes));
    }
    else {
      unbind_all();
    }
  }


  void GnotePrefsKeybinder::bind_preference(const std::string & pref_path, const sigc::slot<void> & handler)
  {
    bind(pref_path, Preferences::obj().get_default<std::string>(pref_path), handler);
  }


  void GnotePrefsKeybinder::key_show_menu()
  {
    // Show the notes menu, highlighting the first item.
    // This matches the behavior of GTK for
    // accelerator-shown menus.
    m_trayicon.show_menu (true);
  }


  void GnotePrefsKeybinder::key_openstart_here()
  {
    Note::Ptr note = m_manager.find_by_uri (m_manager.start_note_uri());
    if (note) {
      note->get_window()->present ();
    }
  }


  void GnotePrefsKeybinder::key_create_new_note()
  {
    try {
      Note::Ptr new_note = m_manager.create ();
      new_note->get_window()->show ();
    } 
    catch (...) {
      // Fail silently.
    }

  }


  void GnotePrefsKeybinder::key_open_search()
  {
    /* Find dialog is deprecated in favor of searcable ToC */
    /*
      NoteFindDialog find_dialog = NoteFindDialog.GetInstance (manager);
      find_dialog.Present ();
    */
    key_open_recent_changes ();
  }


  void GnotePrefsKeybinder::key_open_recent_changes()
  {
    NoteRecentChanges *recent = NoteRecentChanges::get_instance (m_manager);
    recent->present ();
  }

}

