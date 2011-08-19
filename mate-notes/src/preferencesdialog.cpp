/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
 * Copyright (C) 2009 Debarshi Ray
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
/* this file might still contain Tomboy code see the  AUTHORS file for details */


#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include <glibmm/i18n.h>
#include <gtkmm/accelgroup.h>
#include <gtkmm/alignment.h>
#include <gtkmm/button.h>
#include <gtkmm/fontselection.h>
#include <gtkmm/linkbutton.h>
#include <gtkmm/notebook.h>
#include <gtkmm/separator.h>
#include <gtkmm/stock.h>
#include <gtkmm/table.h>

#include "sharp/addinstreemodel.hpp"
#include "sharp/modulemanager.hpp"
#include "sharp/propertyeditor.hpp"
#include "addinmanager.hpp"
#include "addinpreferencefactory.hpp"
#include "debug.hpp"
#include "gnote.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "preferencesdialog.hpp"
#include "preferences.hpp"
#include "preferencetabaddin.hpp"
#include "utils.hpp"
#include "watchers.hpp"

namespace gnote {

  class AddinInfoDialog
    : public Gtk::Dialog
  {
  public:
    AddinInfoDialog(const sharp::DynamicModule * module, Gtk::Dialog &parent);
    void set_addin_id(const std::string & id)
      { m_id = id; }
    const std::string & get_addin_id() const
      { return m_id; }
  private:
    void fill(Gtk::Label &);
    const sharp::DynamicModule *m_module;
    std::string m_id;
  };


  PreferencesDialog::PreferencesDialog(AddinManager & addinmanager)
    : Gtk::Dialog()
    , syncAddinCombo(NULL)
    , syncAddinPrefsContainer(NULL)
    , syncAddinPrefsWidget(NULL)
    , resetSyncAddinButton(NULL)
    , saveSyncAddinButton(NULL)
    , renameBehaviorCombo(NULL)
    , m_addin_manager(addinmanager)
  {
//    set_icon(utils::get_icon("gnote"));
    set_has_separator(false);
    set_border_width(5);
    set_resizable(true);
    set_title(_("Gnote Preferences"));

    get_vbox()->set_spacing(5);
    get_action_area()->set_layout(Gtk::BUTTONBOX_END);

//      addin_prefs_dialogs =
//              new Dictionary<string, Gtk::Dialog> ();
//      addin_info_dialogs =
//              new Dictionary<string, Gtk::Dialog> ();

      // Notebook Tabs (Editing, Hotkeys)...

    Gtk::Notebook *notebook = manage(new Gtk::Notebook());
    notebook->set_tab_pos(Gtk::POS_TOP);
    notebook->set_border_width(5);
    notebook->show();
    
    notebook->append_page (*manage(make_editing_pane()),
                           _("Editing"));
    notebook->append_page (*manage(make_hotkeys_pane()),
                           _("Hotkeys"));
//      }
//    notebook->append_page (make_sync_pane(),
//                           _("Synchronization"));
    notebook->append_page (*manage(make_addins_pane()),
                           _("Add-ins"));

      // TODO: Figure out a way to have these be placed in a specific order
    std::list<PreferenceTabAddin *> tabAddins;
    m_addin_manager.get_preference_tab_addins(tabAddins);
    for(std::list<PreferenceTabAddin *>::const_iterator iter = tabAddins.begin();
          iter != tabAddins.end(); ++iter) {
      PreferenceTabAddin *tabAddin = *iter;
      DBG_OUT("Adding preference tab addin: %s", 
              typeid(*tabAddin).name());
        try {
          std::string tabName;
          Gtk::Widget *tabWidget = NULL;
          if (tabAddin->get_preference_tab_widget (this, tabName, tabWidget)) {
            notebook->append_page (*manage(tabWidget), tabName);
          }
        } 
        catch(...)
        {
          DBG_OUT("Problems adding preferences tab addin: %s", 
                  typeid(*tabAddin).name());
        }
      }

      get_vbox()->pack_start (*notebook, true, true, 0);

// TODO
//      addin_manager.ApplicationAddinListChanged += OnAppAddinListChanged;


      // Ok button...

      Gtk::Button *button = manage(new Gtk::Button(Gtk::Stock::CLOSE));
      button->property_can_default().set_value(true);
      button->show ();

      Glib::RefPtr<Gtk::AccelGroup> accel_group(Gtk::AccelGroup::create());
      add_accel_group (accel_group);

      button->add_accelerator ("activate",
                               accel_group,
                               GDK_Escape,
                               (Gdk::ModifierType)0,
                               (Gtk::AccelFlags)0);

      add_action_widget (*button, Gtk::RESPONSE_CLOSE);
      set_default_response(Gtk::RESPONSE_CLOSE);

    Preferences::obj().signal_setting_changed().connect(
      sigc::mem_fun(
        *this,
        &PreferencesDialog::on_preferences_setting_changed));
  }

  void PreferencesDialog::enable_addin(bool enable)
  {
    sharp::DynamicModule * const module = get_selected_addin();
    if (!module)
      return;

    if (module->has_interface(NoteAddin::IFACE_NAME)) {
      if (enable)
        m_addin_manager.add_note_addin_info(module);
      else
        m_addin_manager.erase_note_addin_info(module);
    }
    else {
      const char * const id = module->id();

      ApplicationAddin * const addin
        = m_addin_manager.get_application_addin(id);
      if (!addin) {
        ERR_OUT("ApplicationAddin %s absent", id);
        return;
      }

      if (enable)
        addin->initialize();
      else
        addin->shutdown();
    }

    module->enabled(enable);
    m_addin_manager.save_addins_prefs();
  }
  
  
  // Page 1
  // List of editing options
  Gtk::Widget *PreferencesDialog::make_editing_pane()
  {
      Gtk::Label *label;
      Gtk::CheckButton *check;
      Gtk::Alignment *align;
      sharp::PropertyEditorBool *peditor, *font_peditor,* bullet_peditor;

      Gtk::VBox *options_list = manage(new Gtk::VBox(false, 12));
      options_list->set_border_width(12);
      options_list->show();


      // Spell checking...

#if FIXED_GTKSPELL
      // TODO I'm not sure there is a proper reason to do that.
      // it is in or NOT. if not, disable the UI.
      if (NoteSpellChecker::gtk_spell_available()) {
        check = manage(make_check_button (
                         _("_Spell check while typing")));
        options_list->pack_start (*check, false, false, 0);
        peditor = new sharp::PropertyEditorBool(Preferences::ENABLE_SPELLCHECKING, *check);
        peditor->setup();

        label = manage(make_tip_label (
                         _("Misspellings will be underlined "
                           "in red, with correct spelling "
                           "suggestions shown in the context "
                          "menu.")));
        options_list->pack_start (*label, false, false, 0);
      }
#endif


      // WikiWords...

      check = manage(make_check_button (_("Highlight _WikiWords")));
      options_list->pack_start (*check, false, false, 0);
      peditor = new sharp::PropertyEditorBool(Preferences::ENABLE_WIKIWORDS, *check);
      peditor->setup();

      label = manage(make_tip_label (
                      _("Enable this option to highlight "
                        "words <b>ThatLookLikeThis</b>. "
                        "Clicking the word will create a "
                        "note with that name.")));
      options_list->pack_start (*label, false, false, 0);

      // Auto bulleted list
      check = manage(make_check_button (_("Enable auto-_bulleted lists")));
      options_list->pack_start (*check, false, false, 0);
      bullet_peditor = new sharp::PropertyEditorBool(Preferences::ENABLE_AUTO_BULLETED_LISTS, 
                                                       *check);
      bullet_peditor->setup();

      // Custom font...

      Gtk::HBox * const font_box = manage(new Gtk::HBox(false, 0));
      check = manage(make_check_button (_("Use custom _font")));
      font_box->pack_start(*check, Gtk::PACK_EXPAND_WIDGET, 0);
      font_peditor = new sharp::PropertyEditorBool(Preferences::ENABLE_CUSTOM_FONT, 
                                                     *check);
      font_peditor->setup();

      font_button = manage(make_font_button());
      font_button->set_sensitive(check->get_active());
      font_box->pack_start(*font_button, Gtk::PACK_EXPAND_WIDGET, 0);
      font_box->show_all();
      options_list->pack_start(*font_box, false, false, 0);

      font_peditor->add_guard (font_button);
      
      // Note renaming behavior
      Gtk::HBox * const rename_behavior_box = manage(new Gtk::HBox(
                                                           false, 0));
      label = manage(make_label(_("When renaming a linked note: ")));
      rename_behavior_box->pack_start(*label,
                                      Gtk::PACK_EXPAND_WIDGET,
                                      0);
      renameBehaviorCombo = manage(new Gtk::ComboBoxText());
      renameBehaviorCombo->append_text(_("Ask me what to do"));
      renameBehaviorCombo->append_text(_("Never rename links"));
      renameBehaviorCombo->append_text(_("Always rename links"));
      Preferences & preferences = Preferences::obj();
      int rename_behavior = preferences.get<int>(
                              Preferences::NOTE_RENAME_BEHAVIOR);
      if (0 > rename_behavior || 2 < rename_behavior) {
        rename_behavior = 0;
        preferences.set<int>(Preferences::NOTE_RENAME_BEHAVIOR,
                             rename_behavior);
      }
      renameBehaviorCombo->set_active(rename_behavior);
      renameBehaviorCombo->signal_changed().connect(
        sigc::mem_fun(
          *this,
          &PreferencesDialog::on_rename_behavior_changed));
      rename_behavior_box->pack_start(*renameBehaviorCombo,
                                      Gtk::PACK_EXPAND_WIDGET,
                                      0);
      rename_behavior_box->show_all();
      options_list->pack_start(*rename_behavior_box, false, false, 0);

      // New Note Template
      // Translators: This is 'New Note' Template, not New 'Note Template'
      label = manage(make_label (_("New Note Template")));
      options_list->pack_start (*label, false, false, 0);

      label = manage(make_tip_label (_("Use the new note template to specify the text "
                                       "that should be used when creating a new note.")));
      options_list->pack_start (*label, false, false, 0);
      
      align = manage(new Gtk::Alignment (0.5f, 0.5f, 0.4f, 1.0f));
      align->show();
      options_list->pack_start (*align, false, false, 0);
      
      Gtk::Button *open_template_button = manage(new Gtk::Button (_("Open New Note Template")));

      open_template_button->signal_clicked().connect(
        sigc::mem_fun(*this, &PreferencesDialog::open_template_button_clicked));
      open_template_button->show ();
      align->add (*open_template_button);

      return options_list;
    }

  Gtk::Button *PreferencesDialog::make_font_button ()
  {
    Gtk::HBox *font_box = manage(new Gtk::HBox (false, 0));
    font_box->show ();

    font_face = manage(new Gtk::Label ());
    font_face->set_use_markup(true);
    font_face->show ();
    font_box->pack_start (*font_face, true, true, 0);

    Gtk::VSeparator *sep = manage(new Gtk::VSeparator());
    sep->show ();
    font_box->pack_start (*sep, false, false, 0);

    font_size = manage(new Gtk::Label());
    font_size->property_xpad().set_value(4);
    font_size->show ();
    font_box->pack_start (*font_size, false, false, 0);

    Gtk::Button *button = new Gtk::Button ();
    button->signal_clicked().connect(sigc::mem_fun(*this, &PreferencesDialog::on_font_button_clicked));
    button->add (*font_box);
    button->show ();

    std::string font_desc = Preferences::obj().get<std::string>(Preferences::CUSTOM_FONT_FACE);
    update_font_button (font_desc);

    return button;
  }

  // Page 2
    // List of Hotkey options
  Gtk::Widget *PreferencesDialog::make_hotkeys_pane()
  {
    Gtk::Label* label;
    Gtk::CheckButton* check;
    Gtk::Alignment* align;
    Gtk::Entry* entry;
    sharp::PropertyEditorBool *keybind_peditor;
    sharp::PropertyEditor *peditor;

    Gtk::VBox* hotkeys_list = new Gtk::VBox (false, 12);
    hotkeys_list->set_border_width(12);
    hotkeys_list->show ();


    // Hotkeys...

    check = manage(make_check_button (_("Listen for _Hotkeys")));
    hotkeys_list->pack_start(*check, false, false, 0);

    keybind_peditor = new sharp::PropertyEditorBool(Preferences::ENABLE_KEYBINDINGS, *check);
    keybind_peditor->setup();

    label = manage(make_tip_label (
      _("Hotkeys allow you to quickly access "
        "your notes from anywhere with a keypress. "
        "Example Hotkeys: "
        "<b>&lt;Control&gt;&lt;Shift&gt;F11</b>, "
        "<b>&lt;Alt&gt;N</b>")));
    hotkeys_list->pack_start(*label, false, false, 0);

    align = manage(new Gtk::Alignment (0.5f, 0.5f, 0.0f, 1.0f));
    align->show ();
    hotkeys_list->pack_start(*align, false, false, 0);

    Gtk::Table *table = manage(new Gtk::Table (4, 2, false));
    table->set_col_spacings(6);
    table->set_row_spacings(6);
    table->show ();
    align->add(*table);


    // Show notes menu keybinding...

    label = manage(make_label (_("Show notes _menu")));
    table->attach (*label, 0, 1, 0, 1);

    entry = manage(new Gtk::Entry ());
    label->set_mnemonic_widget(*entry);
    entry->show ();
    table->attach (*entry, 1, 2, 0, 1);

    peditor = new sharp::PropertyEditor(Preferences::KEYBINDING_SHOW_NOTE_MENU,
                                 *entry);
    peditor->setup();
    keybind_peditor->add_guard (entry);


    // Open Start Here keybinding...

    label = manage(make_label (_("Open \"_Start Here\"")));
    table->attach (*label, 0, 1, 1, 2);

    entry = manage(new Gtk::Entry ());
    label->set_mnemonic_widget(*entry);
    entry->show ();
    table->attach (*entry, 1, 2, 1, 2);

    peditor = new sharp::PropertyEditor(Preferences::KEYBINDING_OPEN_START_HERE,
                                 *entry);
    peditor->setup();
    keybind_peditor->add_guard (entry);

    // Create new note keybinding...

    label = manage(make_label (_("Create _new note")));
    table->attach (*label, 0, 1, 2, 3);

    entry = manage(new Gtk::Entry ());
    label->set_mnemonic_widget(*entry);
    entry->show ();
    table->attach (*entry, 1, 2, 2, 3);
    
    peditor = new sharp::PropertyEditor(Preferences::KEYBINDING_CREATE_NEW_NOTE,
                                 *entry);
    peditor->setup();
    keybind_peditor->add_guard (entry);

    // Open Search All Notes window keybinding...

    label = manage(make_label (_("Open \"Search _All Notes\"")));
    table->attach (*label, 0, 1, 3, 4);

    entry = manage(new Gtk::Entry ());
    label->set_mnemonic_widget(*entry);
    entry->show ();
    table->attach(*entry, 1, 2, 3, 4);

    peditor = new sharp::PropertyEditor(Preferences::KEYBINDING_OPEN_RECENT_CHANGES,
                                        *entry);
    peditor->setup();
    keybind_peditor->add_guard (entry);

    return hotkeys_list;
  }


  Gtk::Widget *PreferencesDialog::make_sync_pane()
  {
#if 0
    Gtk::VBox *vbox = new Gtk::VBox (false, 0);
    vbox->set_spacing(4);
    vbox->set_border_width(8);

    Gtk::HBox *hbox = manage(new Gtk::HBox (false, 4));

    Gtk::Label *label = manage(new Gtk::Label (_("Ser_vice:")));
    label->property_xalign().set_value(0);
    label->show ();
    hbox->pack_start (*label, false, false, 0);

    // Populate the store with all the available SyncServiceAddins
    syncAddinStore = Gtk::ListStore::create(syncAddinStoreRecord);
// TODO
//(SyncServiceAddin));
//    syncAddinIters = new Dictionary<string,Gtk::TreeIter> ();
//    SyncServiceAddin [] addins = Tomboy.DefaultNoteManager.AddinManager.GetSyncServiceAddins ();
//    Array.Sort (addins, CompareSyncAddinsByName);
//    foreach (SyncServiceAddin addin in addins) {
//      Gtk::TreeIter iter = syncAddinStore.Append ();
//      syncAddinStore.SetValue (iter, 0, addin);
//      syncAddinIters [addin.Id] = iter;
//    }

    syncAddinCombo = manage(new Gtk::ComboBox (syncAddinStore));
    label->set_mnemonic_widget(*syncAddinCombo);
// TODO
//    Glib::RefPtr<Gtk::CellRendererText> crt = Gtk::CellRendererText::create();
//    syncAddinCombo->pack_start (crt, true);
//    syncAddinCombo->set_cell_data_func (crt,
//                                      sigc::mem_fun(*this, &PreferencesDialog::ComboBoxTextDataFunc));

    // Read from Preferences which service is configured and select it
    // by default.  Otherwise, just select the first one in the list.
    std::string addin_id = Preferences::obj().get<std::string>(
      Preferences::SYNC_SELECTED_SERVICE_ADDIN);

    Gtk::TreeIter active_iter;
    if (!addin_id.empty() && syncAddinIters.ContainsKey (addin_id)) {
      active_iter = syncAddinIters [addin_id];
      syncAddinCombo.SetActiveIter (active_iter);
    } else {
      if (syncAddinStore.GetIterFirst (out active_iter) == true) {
        syncAddinCombo.SetActiveIter (active_iter);
      }
    }

    syncAddinCombo->signal_changed().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_sync_addin_combo_changed));

    syncAddinCombo->show();
    hbox->pack_start(*syncAddinCombo, true, true, 0);

    hbox->show();
    vbox->pack_start(*hbox, false, false, 0);

    // Get the preferences GUI for the Sync Addin
    if (active_iter.Stamp != Gtk::TreeIter.Zero.Stamp)
      selectedSyncAddin = syncAddinStore.GetValue (active_iter, 0) as SyncServiceAddin;
    
//    if (selectedSyncAddin)
//      syncAddinPrefsWidget = selectedSyncAddin.CreatePreferencesControl ();
    if (syncAddinPrefsWidget == null) {
      Gtk::Label *l = manage(new Gtk::Label (_("Not configurable")));
      l->property_yalign().set_value(0.5f);
      syncAddinPrefsWidget = l;
    }
    if (syncAddinPrefsWidget && !addin_id.empty() &&
        syncAddinIters.ContainsKey (addin_id) && selectedSyncAddin->is_configured()) {
      syncAddinPrefsWidget->set_sensitive = false;
    }
    
    syncAddinPrefsWidget->show ();
    syncAddinPrefsContainer = manage(new Gtk::VBox (false, 0));
    syncAddinPrefsContainer->pack_start(*syncAddinPrefsWidget, true, true, 0);
    syncAddinPrefsContainer->show();
    vbox->pack_start(*syncAddinPrefsContainer, true, true, 0);

    Gtk::HButtonBox *bbox = manage(new Gtk::HButtonBox());
    bbox->set_spacing(4);
    bbox->property_layout_style().set_value(Gtk::BUTTONBOX_END);

    // "Advanced..." button to bring up extra sync config dialog
    Gtk::Button *advancedConfigButton = manage(new Gtk::Button (_("_Advanced...")));
    advancedConfigButton->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_advanced_sync_config_button));
    advancedConfigButton->show ();
    bbox->pack_start(*advancedConfigButton, false, false, 0);
    bbox->set_child_secondary(*advancedConfigButton, true);

    resetSyncAddinButton = manage(new Gtk::Button(Gtk::Stock::CLEAR));
    resetSyncAddinButton->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_reset_sync_addin_button));
    resetSyncAddinButton->set_sensitive(selectedSyncAddin &&
                                        addin_id == selectedSyncAddin->id() &&
                                        selectedSyncAddin->is_configured());
    resetSyncAddinButton->show ();
    bbox->pack_start(*resetSyncAddinButton, false, false, 0);

    // TODO: Tabbing should go directly from sync prefs widget to here
    // TODO: Consider connecting to "Enter" pressed in sync prefs widget
    saveSyncAddinButton = manage(new Gtk::Button (Gtk::Stock::SAVE));
    saveSyncAddinButton->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_save_sync_addin_button));
    saveSyncAddinButton->set_sensitive = (selectedSyncAddin != NULL &&
                                          (addin_id != selectedSyncAddin->id() 
                                           || !selectedSyncAddin->is_configured()));
    saveSyncAddinButton->show();
    bbox->pack_start(*saveSyncAddinButton, false, false, 0);

    syncAddinCombo->set_sensitive(!selectedSyncAddin ||
                                  addin_id != selectedSyncAddin->id() ||
                                  !selectedSyncAddin->is_configured());

    bbox->show();
    vbox->pack_start(*bbox, false, false, 0);

    vbox->show_all();
    return vbox;
#endif
    return NULL;
  }


  // Page 3
  // Extension Preferences
  Gtk::Widget *PreferencesDialog::make_addins_pane()
  {
    Gtk::VBox *vbox = new Gtk::VBox (false, 6);
    vbox->set_border_width(6);
    Gtk::Label *l = manage(new Gtk::Label (_("The following add-ins are installed"), 
                                           true));
    l->property_xalign() = 0;
    l->show ();
    vbox->pack_start(*l, false, false, 0);

    Gtk::HBox *hbox = manage(new Gtk::HBox (false, 6));

    // TreeView of Add-ins
    m_addin_tree = manage(new Gtk::TreeView ());
    m_addin_tree_model = sharp::AddinsTreeModel::create(m_addin_tree);

    m_addin_tree->show ();

    Gtk::ScrolledWindow *sw = manage(new Gtk::ScrolledWindow ());
    sw->property_hscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
    sw->property_vscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
    sw->set_shadow_type(Gtk::SHADOW_IN);
    sw->add (*m_addin_tree);
    sw->show ();
    hbox->pack_start(*sw, true, true, 0);

    // Action Buttons (right of TreeView)
    Gtk::VButtonBox *button_box = manage(new Gtk::VButtonBox ());
    button_box->set_spacing(4);
    button_box->set_layout(Gtk::BUTTONBOX_START);

    // TODO: In a future version, add in an "Install Add-ins..." button

    // TODO: In a future version, add in a "Repositories..." button

    enable_addin_button = manage(new Gtk::Button (_("_Enable"), true));
    enable_addin_button->set_sensitive(false);
    enable_addin_button->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_enable_addin_button));
    enable_addin_button->show ();

    disable_addin_button = manage(new Gtk::Button (_("_Disable"), true));
    disable_addin_button->set_sensitive(false);
    disable_addin_button->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_disable_addin_button));
    disable_addin_button->show ();

    addin_prefs_button = manage(new Gtk::Button (Gtk::Stock::PREFERENCES));
    addin_prefs_button->set_sensitive(false);
    addin_prefs_button->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_addin_prefs_button));
    addin_prefs_button->show ();

    addin_info_button = manage(new Gtk::Button (Gtk::Stock::INFO));
    addin_info_button->set_sensitive(false);
    addin_info_button->signal_clicked().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_addin_info_button));
    addin_info_button->show ();

    button_box->pack_start(*enable_addin_button);
    button_box->pack_start(*disable_addin_button);
    button_box->pack_start(*addin_prefs_button);
    button_box->pack_start(*addin_info_button);

    button_box->show ();
    hbox->pack_start(*button_box, false, false, 0);

    hbox->show ();
    vbox->pack_start(*hbox, true, true, 0);
    vbox->show ();

    m_addin_tree->get_selection()->signal_changed().connect(
      sigc::mem_fun(*this, &PreferencesDialog::on_addin_tree_selection_changed));
    load_addins ();

    return vbox;
  }


  sharp::DynamicModule * PreferencesDialog::get_selected_addin()
  {
    /// TODO really set
    Glib::RefPtr<Gtk::TreeSelection> select = m_addin_tree->get_selection();
    Gtk::TreeIter iter = select->get_selected();
    sharp::DynamicModule * module = NULL;
    if(iter) {
      module = m_addin_tree_model->get_module(iter);
    }
    return module;
  }


  void PreferencesDialog::on_addin_tree_selection_changed()
  {
    update_addin_buttons();
  }


  /// Set the sensitivity of the buttons based on what is selected
  void PreferencesDialog::update_addin_buttons()
  {
    const sharp::DynamicModule * module = get_selected_addin();
    if(module) {
      enable_addin_button->set_sensitive(!module->is_enabled());
      disable_addin_button->set_sensitive(module->is_enabled());
      addin_prefs_button->set_sensitive(
        module->has_interface(AddinPreferenceFactoryBase::IFACE_NAME));
      addin_info_button->set_sensitive(true);
    }
    else {
      enable_addin_button->set_sensitive(false);
      disable_addin_button->set_sensitive(false);
      addin_prefs_button->set_sensitive(false);
      addin_info_button->set_sensitive(false);
    }
  }


  void PreferencesDialog::load_addins()
  {
    ///// TODO populate
    const sharp::ModuleList & list(m_addin_manager.get_modules());
    for(sharp::ModuleList::const_iterator iter = list.begin();
        iter != list.end(); ++iter) {

      Gtk::TreeIter treeiter = m_addin_tree_model->append(*iter);
    }

    update_addin_buttons();
  }


  void PreferencesDialog::on_enable_addin_button()
  {
    enable_addin(true);
    update_addin_buttons();
  }

  void PreferencesDialog::on_disable_addin_button()
  {
    enable_addin(false);
    update_addin_buttons();
  }


  void PreferencesDialog::on_addin_prefs_button()
  {
    const sharp::DynamicModule * module = get_selected_addin();
    Gtk::Dialog *dialog;

    if (!module) {
      return;
    }

    std::map<std::string, Gtk::Dialog* >::iterator iter;
    iter = addin_prefs_dialogs.find(module->id());
    if (iter == addin_prefs_dialogs.end()) {
      // A preference dialog isn't open already so create a new one
      Gtk::Image *icon =
        manage(new Gtk::Image (Gtk::Stock::PREFERENCES, Gtk::ICON_SIZE_DIALOG));
      Gtk::Label *caption = manage(new Gtk::Label());
      caption->set_markup(
        str(boost::format("<span size='large' weight='bold'>%1% %2%</span>") 
            % module->name() % module->version()));
      caption->property_xalign() = 0;
      caption->set_use_markup(true);
      caption->set_use_underline(false);

      Gtk::Widget * pref_widget =
        m_addin_manager.create_addin_preference_widget (module->id());

      if (pref_widget == NULL) {
        pref_widget = manage(new Gtk::Label (_("Not Implemented")));
      }
      
      Gtk::HBox *hbox = manage(new Gtk::HBox (false, 6));
      Gtk::VBox *vbox = manage(new Gtk::VBox (false, 6));
      vbox->set_border_width(6);
      hbox->pack_start (*icon, false, false, 0);
      hbox->pack_start (*caption, true, true, 0);
      vbox->pack_start (*hbox, false, false, 0);

      vbox->pack_start (*pref_widget, true, true, 0);
      vbox->show_all ();

      dialog = new Gtk::Dialog(
        str(boost::format(_("%1% Preferences")) % module->name()),
        *this, false, false);
      dialog->property_destroy_with_parent() = true;
      dialog->add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE);

      dialog->get_vbox()->pack_start (*vbox, true, true, 0);
      dialog->signal_delete_event().connect(
        sigc::bind(
          sigc::mem_fun(*this, &PreferencesDialog::addin_pref_dialog_deleted),
          dialog), false);
      dialog->signal_response().connect(
        sigc::bind(
          sigc::mem_fun(*this, &PreferencesDialog::addin_pref_dialog_response),
          dialog));

      // Store this dialog off in the dictionary so it can be
      // presented again if the user clicks on the preferences button
      // again before closing the preferences dialog.
//      dialog->set_data(Glib::Quark("AddinId"), module->id());
      addin_prefs_dialogs [module->id()] = dialog;
    } 
    else {
      // It's already opened so just present it again
      dialog = iter->second;
    }

    dialog->present ();
  }


  bool PreferencesDialog::addin_pref_dialog_deleted(GdkEventAny*, 
                                                    Gtk::Dialog* dialog)
  {
    // Remove the addin from the addin_prefs_dialogs Dictionary
    dialog->hide ();

//    addin_prefs_dialogs.erase(dialog->get_addin_id());

    return false;
  }

  void PreferencesDialog::addin_pref_dialog_response(int, Gtk::Dialog* dialog)
  {
    addin_pref_dialog_deleted(NULL, dialog);
  }

  void PreferencesDialog::on_addin_info_button()
  {
    const sharp::DynamicModule * addin = get_selected_addin();

    if (addin == NULL) {
      return;
    }

    Gtk::Dialog* dialog;
    std::map<std::string, Gtk::Dialog* >::iterator iter;
    iter = addin_info_dialogs.find(addin->id());
    if (iter == addin_info_dialogs.end()) {
      dialog = new AddinInfoDialog (addin, *this);
      dialog->signal_delete_event().connect(
        sigc::bind(
          sigc::mem_fun(*this, &PreferencesDialog::addin_info_dialog_deleted), 
          dialog), false);
      dialog->signal_response().connect(
        sigc::bind(
          sigc::mem_fun(*this, &PreferencesDialog::addin_info_dialog_response),
          dialog));

      // Store this dialog off in a dictionary so it can be presented
      // again if the user clicks on the Info button before closing
      // the original dialog.
      static_cast<AddinInfoDialog*>(dialog)->set_addin_id(addin->id());
      addin_info_dialogs [addin->id()] = dialog;
    } 
    else {
      // It's already opened so just present it again
      dialog = iter->second;
    }

    dialog->present ();
  }

  bool PreferencesDialog::addin_info_dialog_deleted(GdkEventAny*, 
                                                    Gtk::Dialog* dialog)
  {
    // Remove the addin from the addin_prefs_dialogs Dictionary
    dialog->hide ();

    AddinInfoDialog *addin_dialog = static_cast<AddinInfoDialog*>(dialog);
    addin_info_dialogs.erase(addin_dialog->get_addin_id());
    delete dialog;

    return false;
  }


  void PreferencesDialog::addin_info_dialog_response(int, Gtk::Dialog* dlg)
  {
    addin_info_dialog_deleted(NULL, dlg);
  }



  Gtk::Label *PreferencesDialog::make_label (const std::string & label_text/*, params object[] args*/)
  {
//    if (args.Length > 0)
//      label_text = String.Format (label_text, args);

    Gtk::Label *label = new Gtk::Label (label_text, true);

    label->set_use_markup(true);
    label->set_justify(Gtk::JUSTIFY_LEFT);
    label->set_alignment(0.0f, 0.5f);
    label->show();

    return label;
  }

  Gtk::CheckButton *PreferencesDialog::make_check_button (const std::string & label_text)
  {
    Gtk::Label *label = manage(make_label(label_text));
    
    Gtk::CheckButton *check = new Gtk::CheckButton();
    check->add(*label);
    check->show();
    
    return check;
  }


  Gtk::Label *PreferencesDialog::make_tip_label(std::string label_text)
  {
    Gtk::Label *label = make_label(str(boost::format("<small>%1%</small>") % label_text));
    label->set_line_wrap(true);
    label->property_xpad().set_value(20);
    return label;
  }

  void PreferencesDialog::on_font_button_clicked()
  {
    Gtk::FontSelectionDialog *font_dialog =
      new Gtk::FontSelectionDialog (_("Choose Note Font"));

    std::string font_name = Preferences::obj().get<std::string>(Preferences::CUSTOM_FONT_FACE);
    font_dialog->set_font_name(font_name);

    if (Gtk::RESPONSE_OK == font_dialog->run()) {
      if (font_dialog->get_font_name() != font_name) {
        Preferences::obj().set<std::string>(Preferences::CUSTOM_FONT_FACE,
                                            font_dialog->get_font_name());

        update_font_button (font_dialog->get_font_name());
      }
    }

    delete font_dialog;
  }

  void PreferencesDialog::update_font_button (const std::string & font_desc)
  {
    PangoFontDescription *desc = pango_font_description_from_string(font_desc.c_str());

    // Set the size label
    font_size->set_text(boost::lexical_cast<std::string>(pango_font_description_get_size(desc) / PANGO_SCALE));

    pango_font_description_unset_fields(desc, PANGO_FONT_MASK_SIZE);

    // Set the font name label
    char * descstr = pango_font_description_to_string(desc);
    font_face->set_markup(str(boost::format("<span font_desc='%1%'>%2%</span>")
                              % font_desc % std::string(descstr)));
    g_free(descstr);
    pango_font_description_free(desc);
  }



  void  PreferencesDialog::open_template_button_clicked()
  {
    NoteManager &manager = Gnote::obj().default_note_manager();
    Note::Ptr template_note = manager.get_or_create_template_note ();

    // Open the template note
    template_note->get_window()->show ();
  }



  void  PreferencesDialog::on_preferences_setting_changed(
                             Preferences * preferences,
                             MateConfEntry * entry)
  {
    const char * const key = mateconf_entry_get_key(entry);

    if (strcmp(key, Preferences::NOTE_RENAME_BEHAVIOR) == 0) {
      const MateConfValue * const value = mateconf_entry_get_value(entry);
      int rename_behavior = mateconf_value_get_int(value);
      if (0 > rename_behavior || 2 < rename_behavior) {
        rename_behavior = 0;
        preferences->set<int>(Preferences::NOTE_RENAME_BEHAVIOR,
                              rename_behavior);
      }
      if (renameBehaviorCombo->get_active_row_number()
            != rename_behavior) {
        renameBehaviorCombo->set_active(rename_behavior);
      }
    }
  }



  void  PreferencesDialog::on_rename_behavior_changed()
  {
    Preferences::obj().set<int>(
      Preferences::NOTE_RENAME_BEHAVIOR,
      renameBehaviorCombo->get_active_row_number());
  }



  AddinInfoDialog::AddinInfoDialog(const sharp::DynamicModule * module,
                                   Gtk::Dialog &parent)
    : Gtk::Dialog(module->name(), parent, false, true)
    , m_module(module)
  {
    property_destroy_with_parent() = true;
    add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE);
    
    // TODO: Change this icon to be an addin/package icon
    Gtk::Image *icon = manage(new Gtk::Image(Gtk::Stock::DIALOG_INFO, 
                                             Gtk::ICON_SIZE_DIALOG));
    icon->property_yalign() = 0;

    Gtk::Label *info_label = manage(new Gtk::Label ());
    info_label->property_xalign() = 0;
    info_label->property_yalign() = 0;
    info_label->set_use_markup(true);
    info_label->set_use_underline(false);
    info_label->property_wrap() = true;

    Gtk::HBox *hbox = manage(new Gtk::HBox (false, 6));
    Gtk::VBox *vbox = manage(new Gtk::VBox (false, 12));
    hbox->set_border_width(12);
    vbox->set_border_width(6);

    hbox->pack_start (*icon, false, false, 0);
    hbox->pack_start (*vbox, true, true, 0);

    vbox->pack_start (*info_label, true, true, 0);

    hbox->show_all ();

    get_vbox()->pack_start(*hbox, true, true, 0);

    fill (*info_label);
  }

	void AddinInfoDialog::fill(Gtk::Label & info_label)
  {
    std::string sb("<b><big>");
    sb += std::string(m_module->name()) + "</big></b>\n\n";

    const char * s = m_module->description();
    if (s && *s) {
      sb += std::string(s) + "\n\n";
    }

    sb += str(boost::format("<small><b>%1%</b>\n%2%\n\n")
              % _("Version:") % m_module->version());

    s = m_module->authors();
    if (s && *s) {
      sb += str(boost::format("<b>%1%</b>\n%2%\n\n")
                % _("Author:") % s);
    }
    
    s = m_module->copyright();
    if (s && *s) {
      sb += str(boost::format("<b>%1%</b>\n%2%\n\n") 
                % _("Copyright:") % s);
    }

#if 0 // TODO handle module dependencies
    if (info.Dependencies.Count > 0) {
      sb.Append (string.Format (
                   "<b>{0}</b>\n",
                   Catalog.GetString ("Add-in Dependencies:")));
      foreach (Mono.Addins.Description.Dependency dep in info.Dependencies) {
        sb.Append (dep.Name + "\n");
      }
    }
#endif

    sb += "</small>";

    info_label.set_markup(sb);
  }

}

