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




#ifndef __PREFERENCES_HPP_
#define __PREFERENCES_HPP_

#include <string>
#include <mateconf/mateconf-client.h>
#include <sigc++/signal.h>

#include "base/singleton.hpp"

namespace gnote {

  class Preferences 
    : public base::Singleton<Preferences>
  {
  public:
    typedef sigc::signal<void, Preferences*, MateConfEntry*> NotifyChangeSignal;
    static const char *ENABLE_SPELLCHECKING;
    static const char *ENABLE_WIKIWORDS;
    static const char *ENABLE_CUSTOM_FONT;
    static const char *ENABLE_KEYBINDINGS;
    static const char *ENABLE_STARTUP_NOTES;
    static const char *ENABLE_AUTO_BULLETED_LISTS;
    static const char *ENABLE_ICON_PASTE;
    static const char *ENABLE_CLOSE_NOTE_ON_ESCAPE;

    static const char *START_NOTE_URI;
    static const char *CUSTOM_FONT_FACE;
    static const char *MENU_NOTE_COUNT;
    static const char *MENU_PINNED_NOTES;

    static const char *KEYBINDING_SHOW_NOTE_MENU;
    static const char *KEYBINDING_OPEN_START_HERE;
    static const char *KEYBINDING_CREATE_NEW_NOTE;
    static const char *KEYBINDING_OPEN_SEARCH;
    static const char *KEYBINDING_OPEN_RECENT_CHANGES;

    static const char *EXPORTHTML_LAST_DIRECTORY;
    static const char *EXPORTHTML_EXPORT_LINKED;
    static const char *EXPORTHTML_EXPORT_LINKED_ALL;

    static const char *SYNC_CLIENT_ID;
    static const char *SYNC_LOCAL_PATH;
    static const char *SYNC_SELECTED_SERVICE_ADDIN;
    static const char *SYNC_CONFIGURED_CONFLICT_BEHAVIOR;

    static const char *NOTE_RENAME_BEHAVIOR;

    static const char *INSERT_TIMESTAMP_FORMAT;
    
    static const char *SEARCH_WINDOW_X_POS;
    static const char *SEARCH_WINDOW_Y_POS;
    static const char *SEARCH_WINDOW_WIDTH;
    static const char *SEARCH_WINDOW_HEIGHT;
    static const char *SEARCH_WINDOW_SPLITTER_POS;

    Preferences();
    ~Preferences();

    template<typename T>
    void set(const char * p, const T&);

    template<typename T>
    void set(const std::string & p, const T&v)
      {
        set<T>(p.c_str(),v);
      }


    template<typename T>
    T get(const char * p);

    template<typename T>
    T get(const std::string & p)
      {
        return get<T>(p.c_str());
      }

    template<typename T>
    T get_default(const char * p);

    template<typename T>
    T get_default(const std::string & p)
      {
        return get_default<T>(p.c_str());
      }
    
    sigc::signal<void, Preferences*, MateConfEntry*> & signal_setting_changed()
      {
        return  m_signal_setting_changed;
      }

    // this is very hackish. maybe I should just use mateconfmm
    guint add_notify(const char *ns, MateConfClientNotifyFunc func, gpointer data);
    void remove_notify(guint);
    MateConfClient * get_client() const
      {
        return m_client;
      }
  private:
    Preferences(const Preferences &); // non implemented
    MateConfClient        *m_client;
    guint               m_cnx;

    static void mateconf_notify_glue(MateConfClient *client, guint cid, MateConfEntry *entry,
                                  Preferences * self);
    NotifyChangeSignal m_signal_setting_changed;
  };


}


#endif
