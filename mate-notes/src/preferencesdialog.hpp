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




#ifndef __PREFERENCES_DIALOG_HPP_
#define __PREFERENCES_DIALOG_HPP_

#include <string>
#include <map>

#include <gtkmm/dialog.h>
#include <gtkmm/treeiter.h>
#include <gtkmm/liststore.h>
#include <gtkmm/combobox.h>
#include <gtkmm/comboboxtext.h>

#include "sharp/addinstreemodel.hpp"

namespace gnote {

class SyncServiceAddin;
class AddinManager;

class PreferencesDialog
  : public Gtk::Dialog
{
public:
  PreferencesDialog(AddinManager & addinmanager);
  

  Gtk::Widget *make_editing_pane();
  Gtk::Widget *make_hotkeys_pane();
  Gtk::Widget *make_sync_pane();
  Gtk::Widget *make_addins_pane();

private:
  Gtk::Label *make_tip_label(std::string label_text);
  Gtk::Button *make_font_button();
  Gtk::Label *make_label (const std::string & label_text/*, params object[] args*/);
  Gtk::CheckButton *make_check_button (const std::string & label_text);

  void enable_addin(bool enable);

  void open_template_button_clicked();
  void on_font_button_clicked();
  void update_font_button(const std::string & font_desc);
  void on_sync_addin_combo_changed();
  void on_advanced_sync_config_button();
  void on_reset_sync_addin_button();
  void on_save_sync_addin_button();

  void on_preferences_setting_changed(Preferences * preferences,
                                      MateConfEntry * entry);
  void on_rename_behavior_changed();

  sharp::DynamicModule * get_selected_addin();
  void on_addin_tree_selection_changed();
  void update_addin_buttons();
  void load_addins();
  void on_enable_addin_button();
  void on_disable_addin_button();
  void on_addin_prefs_button();
  bool addin_pref_dialog_deleted(GdkEventAny*, Gtk::Dialog*);
  void addin_pref_dialog_response(int, Gtk::Dialog*);
  void on_addin_info_button();
  bool addin_info_dialog_deleted(GdkEventAny*, Gtk::Dialog*);
  void addin_info_dialog_response(int, Gtk::Dialog*);
////

  class SyncStoreModel
    : public Gtk::TreeModelColumnRecord
  {
  public:
    SyncStoreModel()
      {
        add(m_col1);
      }

    Gtk::TreeModelColumn<std::string> m_col1;
  };

  SyncStoreModel syncAddinStoreRecord;
  Glib::RefPtr<Gtk::ListStore> syncAddinStore;
  std::map<std::string, Gtk::TreeIter> syncAddinIters;
  Gtk::ComboBox *syncAddinCombo;
  SyncServiceAddin *selectedSyncAddin;
  Gtk::VBox   *syncAddinPrefsContainer;
  Gtk::Widget *syncAddinPrefsWidget;
  Gtk::Button *resetSyncAddinButton;
  Gtk::Button *saveSyncAddinButton;
  Gtk::ComboBoxText *renameBehaviorCombo;
  AddinManager &m_addin_manager;
    
  Gtk::Button *font_button;
  Gtk::Label  *font_face;
  Gtk::Label  *font_size;

  Gtk::TreeView              *m_addin_tree;
  sharp::AddinsTreeModel::Ptr m_addin_tree_model;
  
  Gtk::Button *enable_addin_button;
  Gtk::Button *disable_addin_button;
  Gtk::Button *addin_prefs_button;
  Gtk::Button *addin_info_button;

  Gtk::RadioButton promptOnConflictRadio;
  Gtk::RadioButton renameOnConflictRadio;
  Gtk::RadioButton overwriteOnConflictRadio;


  /// Keep track of the opened addin prefs dialogs so other windows
  /// can be interacted with (as opposed to opening these as modal
  /// dialogs).
  ///
  /// Key = Mono.Addins.Addin.Id
  std::map<std::string, Gtk::Dialog* > addin_prefs_dialogs;

  /// Used to keep track of open AddinInfoDialogs.
  /// Key = Mono.Addins.Addin.Id
  std::map<std::string, Gtk::Dialog* > addin_info_dialogs;

};


}

#endif
