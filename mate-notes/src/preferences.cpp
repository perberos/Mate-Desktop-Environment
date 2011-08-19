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



#include "preferences.hpp"



namespace gnote {


  const char * Preferences::ENABLE_SPELLCHECKING = "/apps/gnote/enable_spellchecking";
  const char * Preferences::ENABLE_WIKIWORDS = "/apps/gnote/enable_wikiwords";
  const char * Preferences::ENABLE_CUSTOM_FONT = "/apps/gnote/enable_custom_font";
  const char * Preferences::ENABLE_KEYBINDINGS = "/apps/gnote/enable_keybindings";
  const char * Preferences::ENABLE_STARTUP_NOTES = "/apps/gnote/enable_startup_notes";
  const char * Preferences::ENABLE_AUTO_BULLETED_LISTS = "/apps/gnote/enable_bulleted_lists";
  const char * Preferences::ENABLE_ICON_PASTE = "/apps/gnote/enable_icon_paste";
  const char * Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE = "/apps/gnote/enable_close_note_on_escape";

  const char * Preferences::START_NOTE_URI = "/apps/gnote/start_note";
  const char * Preferences::CUSTOM_FONT_FACE = "/apps/gnote/custom_font_face";
  const char * Preferences::MENU_NOTE_COUNT = "/apps/gnote/menu_note_count";
  const char * Preferences::MENU_PINNED_NOTES = "/apps/gnote/menu_pinned_notes";

  const char * Preferences::KEYBINDING_SHOW_NOTE_MENU = "/apps/gnote/global_keybindings/show_note_menu";
  const char * Preferences::KEYBINDING_OPEN_START_HERE = "/apps/gnote/global_keybindings/open_start_here";
  const char * Preferences::KEYBINDING_CREATE_NEW_NOTE = "/apps/gnote/global_keybindings/create_new_note";
  const char * Preferences::KEYBINDING_OPEN_SEARCH = "/apps/gnote/global_keybindings/open_search";
  const char * Preferences::KEYBINDING_OPEN_RECENT_CHANGES = "/apps/gnote/global_keybindings/open_recent_changes";

  const char * Preferences::EXPORTHTML_LAST_DIRECTORY = "/apps/gnote/export_html/last_directory";
  const char * Preferences::EXPORTHTML_EXPORT_LINKED = "/apps/gnote/export_html/export_linked";
  const char * Preferences::EXPORTHTML_EXPORT_LINKED_ALL = "/apps/gnote/export_html/export_linked_all";

  const char * Preferences::SYNC_CLIENT_ID = "/apps/gnote/sync/sync_guid";
  const char * Preferences::SYNC_LOCAL_PATH = "/apps/gnote/sync/sync_local_path";
  const char * Preferences::SYNC_SELECTED_SERVICE_ADDIN = "/apps/gnote/sync/sync_selected_service_addin";
  const char * Preferences::SYNC_CONFIGURED_CONFLICT_BEHAVIOR = "/apps/gnote/sync/sync_conflict_behavior";

  const char * Preferences::NOTE_RENAME_BEHAVIOR = "/apps/gnote/note_rename_behavior";

  const char * Preferences::INSERT_TIMESTAMP_FORMAT = "/apps/gnote/insert_timestamp/format";
    
  const char * Preferences::SEARCH_WINDOW_X_POS = "/apps/gnote/search_window_x_pos";
  const char * Preferences::SEARCH_WINDOW_Y_POS = "/apps/gnote/search_window_y_pos";
  const char * Preferences::SEARCH_WINDOW_WIDTH = "/apps/gnote/search_window_width";
  const char * Preferences::SEARCH_WINDOW_HEIGHT = "/apps/gnote/search_window_height";
  const char * Preferences::SEARCH_WINDOW_SPLITTER_POS = "/apps/gnote/search_window_splitter_pos";


  Preferences::Preferences()
    : m_client(mateconf_client_get_default())
  {
    mateconf_client_add_dir(m_client, "/apps/gnote", MATECONF_CLIENT_PRELOAD_NONE, NULL);
    m_cnx = mateconf_client_notify_add(m_client, "/apps/gnote" , 
                            MateConfClientNotifyFunc(&Preferences::mateconf_notify_glue),
                            this, NULL, NULL);
  }
  
  Preferences::~Preferences()
  {
    mateconf_client_remove_dir(m_client, "/apps/gnote", NULL);
    mateconf_client_notify_remove(m_client, m_cnx);
  }

  void Preferences::mateconf_notify_glue(MateConfClient */*client*/, guint /*cid*/, MateConfEntry *entry,
                                             Preferences * self)
  {
    self->m_signal_setting_changed(self, entry);
  }

  template<>
  void Preferences::set<bool>(const char *p, const bool & v) 
  {
    mateconf_client_set_bool(m_client, p, v, NULL);
  }

  template<>
  void Preferences::set<std::string>(const char *p, const std::string & v) 
  {
    mateconf_client_set_string(m_client, p, v.c_str(), NULL);
  }

  template<>
  void Preferences::set<int>(const char *p, const int & v) 
  {
    mateconf_client_set_int(m_client, p, v, NULL);
  }

  template<>
  bool Preferences::get<bool>(const char *p) 
  {
    return mateconf_client_get_bool(m_client, p, NULL);
  }

  template<>
  std::string Preferences::get<std::string>(const char *p) 
  {
    gchar * s = mateconf_client_get_string(m_client, p, NULL);
    std::string s2 = (s ? s : "");
    g_free(s);
    return s2;
  }

  template<>
  int Preferences::get<int>(const char *p) 
  {
    return mateconf_client_get_int(m_client, p, NULL);
  }

  template<>
  bool Preferences::get_default<bool>(const char *p) 
  {
    MateConfValue * v = mateconf_client_get_default_from_schema(m_client, p, NULL);
    bool b = mateconf_value_get_bool(v);
    mateconf_value_free(v);
    return b;
  }

  template<>
  std::string Preferences::get_default<std::string>(const char *p) 
  {
    MateConfValue * v = mateconf_client_get_default_from_schema(m_client, p, NULL);
    const gchar* s = mateconf_value_get_string(v);
    std::string s2 = (s ? s : "");
    mateconf_value_free(v);
    return s2;
  }

  template<>
  int Preferences::get_default<int>(const char *p) 
  {
    MateConfValue * v = mateconf_client_get_default_from_schema(m_client, p, NULL);
    int i = mateconf_value_get_int(v);
    mateconf_value_free(v);
    return i;
  }


  guint Preferences::add_notify(const char *ns, MateConfClientNotifyFunc func, gpointer data)
  {
    return mateconf_client_notify_add(m_client, ns, func, data, NULL, NULL);
  }


  void Preferences::remove_notify(guint cnxid)
  {
    mateconf_client_notify_remove(m_client, cnxid);
  }

}
