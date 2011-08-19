/*
 * gnote
 *
 * Copyright (C) 2011 Aurimas Cernius
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>

#include <glibmm/i18n.h>
#include <gtkmm/arrow.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/separatormenuitem.h>

#include "libtomboy/tomboyutil.h"
#include "debug.hpp"
#include "note.hpp"
#include "notewindow.hpp"
#include "notemanager.hpp"
#include "noteeditor.hpp"
#include "preferences.hpp"
#include "utils.hpp"
#include "undo.hpp"
#include "recentchanges.hpp"
#include "search.hpp"
#include "actionmanager.hpp"
#include "sharp/exception.hpp"
#include "sharp/string.hpp"


namespace gnote {

  NoteWindow::NoteWindow(Note & note)
    : ForcedPresentWindow(note.get_title())
    , m_note(note)
    , m_global_keys(NULL)
  {
//    get_window()->set_icon_name("gnote");
    set_default_size(450, 360);
    set_resizable(true);

    m_accel_group = Gtk::AccelGroup::create();
    add_accel_group(m_accel_group);

    m_text_menu 
      = Gtk::manage(new NoteTextMenu(m_accel_group, 
                                     note.get_buffer(), note.get_buffer()->undoer()));

    // Add the Find menu item to the toolbar Text menu.  It
    // should only show up in the toplevel Text menu, since
    // the context menu already has a Find submenu.

    Gtk::SeparatorMenuItem *spacer = manage(new Gtk::SeparatorMenuItem ());
    spacer->show ();
    m_text_menu->append(*spacer);

    Gtk::ImageMenuItem *find_item =
      manage(new Gtk::ImageMenuItem (_("Find in This Note")));
    find_item->set_image(*manage(new Gtk::Image (Gtk::Stock::FIND, Gtk::ICON_SIZE_MENU)));
    find_item->signal_activate().connect(sigc::mem_fun(*this, &NoteWindow::find_button_clicked));
    find_item->add_accelerator ("activate",
                                m_accel_group,
                                GDK_F,
                                Gdk::CONTROL_MASK,
                                Gtk::ACCEL_VISIBLE);
    find_item->show ();
    m_text_menu->append(*find_item);

    m_plugin_menu = manage(make_plugin_menu());

    m_toolbar = manage(make_toolbar());
    m_toolbar->show();

    // The main editor widget
    m_editor = manage(new NoteEditor(note.get_buffer()));
    m_editor->signal_populate_popup().connect(sigc::mem_fun(*this, &NoteWindow::on_populate_popup));
    m_editor->show();

    // Sensitize the Link toolbar button on text selection
    m_mark_set_timeout = new utils::InterruptableTimeout();
    m_mark_set_timeout->signal_timeout.connect(
      sigc::mem_fun(*this, &NoteWindow::update_link_button_sensitivity));
    note.get_buffer()->signal_mark_set().connect(
      sigc::mem_fun(*this, &NoteWindow::on_selection_mark_set));

    // FIXME: I think it would be really nice to let the
    //        window get bigger up till it grows more than
    //        60% of the screen, and then show scrollbars.
    m_editor_window = manage(new Gtk::ScrolledWindow());
    m_editor_window->property_hscrollbar_policy().set_value(Gtk::POLICY_AUTOMATIC);
    m_editor_window->property_vscrollbar_policy().set_value(Gtk::POLICY_AUTOMATIC);
    m_editor_window->add(*m_editor);
    m_editor_window->show();

    set_focus_child(*m_editor);

    m_find_bar = manage(new NoteFindBar(note));
    m_find_bar->property_visible() = false;
    m_find_bar->set_no_show_all(true);
    m_find_bar->signal_hide().connect(sigc::mem_fun(*this, &NoteWindow::find_bar_hidden));

    Gtk::VBox *box = manage(new Gtk::VBox (false, 2));
    box->pack_start(*m_toolbar, false, false, 0);
    box->pack_start(*m_editor_window, true, true, 0);
    box->pack_start(*m_find_bar, false, false, 0);

    box->show();

    // Don't set up Ctrl-W or Ctrl-N if Emacs is in use
    bool using_emacs = false;
    std::string gtk_key_theme = 
      Preferences::obj().get<std::string>("/desktop/mate/interface/gtk_key_theme");
    if (!gtk_key_theme.empty() && (gtk_key_theme == "Emacs"))
      using_emacs = true;

    // NOTE: Since some of our keybindings are only
    // available in the context menu, and the context menu
    // is created on demand, register them with the
    // global keybinder
    m_global_keys = new utils::GlobalKeybinder (m_accel_group);

    // Close window (Ctrl-W)
    if (!using_emacs)
      m_global_keys->add_accelerator (sigc::mem_fun(*this, &NoteWindow::close_window_handler),
                                      GDK_W,
                                      Gdk::CONTROL_MASK,
                                      Gtk::ACCEL_VISIBLE);

    // Escape has been moved to be handled by a KeyPress Handler so that
    // Escape can be used to close the FindBar.

    // Close all windows on current Desktop (Ctrl-Q)
    m_global_keys->add_accelerator (sigc::mem_fun(*this, &NoteWindow::close_all_windows_handler),
                                   GDK_Q,
                                   Gdk::CONTROL_MASK,
                                   Gtk::ACCEL_VISIBLE);

    // Find Next (Ctrl-G)
    m_global_keys->add_accelerator (sigc::mem_fun(*this, &NoteWindow::find_next_activate),
                                 GDK_G,
                                 Gdk::CONTROL_MASK,
                                 Gtk::ACCEL_VISIBLE);

    // Find Previous (Ctrl-Shift-G)
    m_global_keys->add_accelerator (sigc::mem_fun(*this, &NoteWindow::find_previous_activate),
                                 GDK_G, (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK),
                                 Gtk::ACCEL_VISIBLE);

    // Open Help (F1)
    m_global_keys->add_accelerator (sigc::mem_fun(*this, &NoteWindow::open_help_activate),
                                    GDK_F1, (Gdk::ModifierType)0, (Gtk::AccelFlags)0);

    // Create a new note
    if (!using_emacs)
      m_global_keys->add_accelerator (sigc::mem_fun(*this, &NoteWindow::create_new_note),
                                     GDK_N, Gdk::CONTROL_MASK,
                                     Gtk::ACCEL_VISIBLE);

    signal_key_press_event().connect(sigc::mem_fun(*this, &NoteWindow::on_key_pressed));


    // Increase Indent
    m_global_keys->add_accelerator (sigc::mem_fun(*this, &NoteWindow::change_depth_right_handler),
                                   GDK_Right, Gdk::MOD1_MASK,
                                   Gtk::ACCEL_VISIBLE);

    // Decrease Indent
    m_global_keys->add_accelerator (sigc::mem_fun(*this, &NoteWindow::change_depth_left_handler),
                                   GDK_Left, Gdk::MOD1_MASK,
                                   Gtk::ACCEL_VISIBLE);
    add(*box);
  }


  NoteWindow::~NoteWindow()
  {
    delete m_global_keys;
    m_global_keys = NULL;
    delete m_mark_set_timeout;
    m_mark_set_timeout = NULL;
    // make sure editor is NULL. See bug 586084
    m_editor = NULL;
    Preferences::obj().remove_notify(m_mateconf_notify);
  }


  bool NoteWindow::on_delete_event(GdkEventAny * /*ev*/)
  {
    close_window_handler();
    return true;
  }

  void NoteWindow::on_hide()
  {
    utils::ForcedPresentWindow::on_hide();

    // Workaround Gtk bug, where adding or changing Widgets
    // while the Window is hidden causes it to be reshown at
    // 0,0...
    int x, y;
    get_position(x, y);
    move(x, y);
  }


  bool NoteWindow::on_key_pressed(GdkEventKey *ev)
  {
    if(ev->keyval == GDK_Escape) {
      if (m_find_bar && m_find_bar->is_visible()) {
        m_find_bar->hide();
      }
      else if (Preferences::obj().get<bool>(
                 Preferences::ENABLE_CLOSE_NOTE_ON_ESCAPE)) {
        close_window_handler();
      }
      return true;
    }
    return false;
  }

  // FIXME: Need to just emit a delete event, and do this work in
  // the default delete handler, so that plugins can attach to
  // delete event and have it always work.
  void NoteWindow::close_window_handler()
  {
    // Unmaximize before hiding to avoid reopening
    // pseudo-maximized
    if ((get_window()->get_state() & Gdk::WINDOW_STATE_MAXIMIZED) != 0) {
      unmaximize();
    }
    hide();
  }


  void NoteWindow::close_all_windows_handler()
  {
    int workspace = tomboy_window_get_workspace(gobj());
    
    for(Note::List::const_iterator iter = m_note.manager().get_notes().begin();
        iter != m_note.manager().get_notes().end(); ++iter) {
      const Note::Ptr & note(*iter);
      if (!note->is_opened())
        continue;

      // Close windows on the same workspace, or all
      // open windows if no workspace.
      if ((workspace < 0) ||
          (tomboy_window_get_workspace (note->get_window()->gobj()) == workspace)) {
        note->get_window()->close_window_handler();
      }
    }
  }
  

    // Delete this Note.
    //

  void NoteWindow::on_delete_button_clicked()
  {
    // Prompt for note deletion
    std::list<Note::Ptr> single_note_list;
    single_note_list.push_back(m_note.shared_from_this());
    noteutils::show_deletion_dialog(single_note_list, this);
  }

  void NoteWindow::on_selection_mark_set(const Gtk::TextIter&, const Glib::RefPtr<Gtk::TextMark>&)
  {
    // FIXME: Process in a timeout due to GTK+ bug #172050.
    if(m_mark_set_timeout) {
      m_mark_set_timeout->reset(0);
    }
  }

  void NoteWindow::update_link_button_sensitivity()
  {
    m_link_button->set_sensitive(!m_note.get_buffer()->get_selection().empty());
  }

  void NoteWindow::on_populate_popup(Gtk::Menu* menu)
  {
    menu->set_accel_group(m_accel_group);

    DBG_OUT("Populating context menu...");

    // Remove the lame-o gigantic Insert Unicode Control
    // Characters menu item.
    Gtk::Widget *lame_unicode;
    std::vector<Gtk::Widget*> children(menu->get_children());
      
    lame_unicode = *children.rbegin();
    menu->remove(*lame_unicode);

    Gtk::MenuItem *spacer1 = manage(new Gtk::SeparatorMenuItem());
    spacer1->show ();

    Gtk::ImageMenuItem *search = manage(new Gtk::ImageMenuItem(
                                          _("_Search All Notes"), true));
    search->set_image(*manage(new Gtk::Image (Gtk::Stock::FIND, Gtk::ICON_SIZE_MENU)));
    search->signal_activate().connect(sigc::mem_fun(*this, &NoteWindow::search_button_clicked));
    search->add_accelerator ("activate", m_accel_group, GDK_F,
                             (Gdk::CONTROL_MASK |  Gdk::SHIFT_MASK),
                             Gtk::ACCEL_VISIBLE);
    search->show();

    Gtk::ImageMenuItem *link = manage(new Gtk::ImageMenuItem(_("_Link to New Note"), true));
    link->set_image(*manage(new Gtk::Image (Gtk::Stock::JUMP_TO, Gtk::ICON_SIZE_MENU)));
    link->set_sensitive(!m_note.get_buffer()->get_selection().empty());
    link->signal_activate().connect(sigc::mem_fun(*this, &NoteWindow::link_button_clicked));
    link->add_accelerator("activate", m_accel_group, GDK_L,
                          Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    link->show();
      
    Gtk::ImageMenuItem *text_item = manage(new Gtk::ImageMenuItem(_("Te_xt"), true));
    text_item->set_image(*manage(new Gtk::Image(
                                   Gtk::Stock::SELECT_FONT, 
                                   Gtk::ICON_SIZE_MENU)));
    text_item->set_submenu(*manage(new NoteTextMenu(m_accel_group,
                                                    m_note.get_buffer(),
                                                    m_note.get_buffer()->undoer())));
    text_item->show();

    Gtk::ImageMenuItem *find_item = manage(new Gtk::ImageMenuItem(_("_Find in This Note"), true));
    find_item->set_image(*manage(new Gtk::Image (Gtk::Stock::FIND, Gtk::ICON_SIZE_MENU)));
    find_item->set_submenu(*manage(make_find_menu()));
    find_item->show();

    Gtk::MenuItem *spacer2 = manage(new Gtk::SeparatorMenuItem());
    spacer2->show();

    menu->prepend(*spacer1);
    menu->prepend(*text_item);
    menu->prepend(*find_item);
    menu->prepend(*link);
    menu->prepend(*search);

    Gtk::MenuItem *close_all =
      manage(new Gtk::MenuItem(_("Clos_e All Notes"), true));
    close_all->signal_activate().connect(
      sigc::mem_fun(*this, &NoteWindow::close_all_windows_handler));
    close_all->add_accelerator("activate", m_accel_group,
                               GDK_Q, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    close_all->show();

    Gtk::ImageMenuItem *close_window = manage(
      new Gtk::ImageMenuItem(_("_Close"), true));
    close_window->set_image(*manage(new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU)));
    close_window->signal_activate().connect(
      sigc::mem_fun(*this, &NoteWindow::close_window_handler));
    close_window->add_accelerator("activate", m_accel_group,
                                  GDK_W,
                                  Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);
    close_window->show();

    menu->append(*close_all);
    menu->append(*close_window);
  }
  
  //
  // Toolbar
  //
  // Add Link button, Font menu, Delete button to the window's
  // toolbar.
  //
  Gtk::Toolbar *NoteWindow::make_toolbar()
  {
    Gtk::Toolbar *tb = new Gtk::Toolbar();

    Gtk::ToolButton *search = manage(new Gtk::ToolButton (
                                       *manage(new Gtk::Image(
                                                 Gtk::Stock::FIND, 
                                                 tb->get_icon_size())
                                         ), _("Search")));
    search->set_use_underline(true);
    search->set_is_important(true);
    search->signal_clicked().connect(sigc::mem_fun(*this, &NoteWindow::search_button_clicked));
    search->set_tooltip_text(_("Search your notes (Ctrl-Shift-F)"));
    search->add_accelerator("clicked", m_accel_group,
                             GDK_F,
                             (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK),
                             Gtk::ACCEL_VISIBLE);
    search->show_all();
    tb->insert(*search, -1);

    m_link_button = manage(new Gtk::ToolButton(
                             *manage(new Gtk::Image (Gtk::Stock::JUMP_TO, tb->get_icon_size())),
                             _("Link")));
    m_link_button->set_use_underline(true);
    m_link_button->set_is_important(true);
    m_link_button->set_sensitive(!m_note.get_buffer()->get_selection().empty());
    m_link_button->signal_clicked().connect(
      sigc::mem_fun(*this, &NoteWindow::link_button_clicked));
    m_link_button->set_tooltip_text(_("Link selected text to a new note (Ctrl-L)"));
    m_link_button->add_accelerator("clicked", m_accel_group,
                                   GDK_L, Gdk::CONTROL_MASK,
                                   Gtk::ACCEL_VISIBLE);
    m_link_button->show_all();
    tb->insert(*m_link_button, -1);

    utils::ToolMenuButton *text_button = manage(new utils::ToolMenuButton(*tb,
                                                  Gtk::Stock::SELECT_FONT,
                                                  _("_Text"),
                                                  m_text_menu));
    text_button->set_use_underline(true);
    text_button->set_is_important(true);
    text_button->show_all();
    tb->insert(*text_button, -1);
    text_button->set_tooltip_text(_("Set properties of text"));

    utils::ToolMenuButton *plugin_button = Gtk::manage(
      new utils::ToolMenuButton (*tb, Gtk::Stock::EXECUTE,
                                 _("T_ools"),
                                 m_plugin_menu));
    plugin_button->set_use_underline(true);
    plugin_button->show_all();
    tb->insert(*plugin_button, -1);
    plugin_button->set_tooltip_text(_("Use tools on this note"));

    tb->insert(*manage(new Gtk::SeparatorToolItem()), -1);

    m_delete_button = manage(new Gtk::ToolButton(Gtk::Stock::DELETE));
    m_delete_button->set_use_underline(true);
    m_delete_button->signal_clicked().connect(
      sigc::mem_fun(*this, &NoteWindow::on_delete_button_clicked));
    m_delete_button->show_all();
    tb->insert(*m_delete_button, -1);
    m_delete_button->set_tooltip_text(_("Delete this note"));

      // Don't allow deleting the "Start Here" note...
    if (m_note.is_special()) {
      m_delete_button->set_sensitive(false);
    }
    tb->insert(*manage(new Gtk::SeparatorToolItem()), -1);

#if 0    // Disable because of bug 585610
    Gtk::ImageMenuItem *item =
      manage(new Gtk::ImageMenuItem (_("Synchronize Notes")));
    item->set_image(*manage(new Gtk::Image(Gtk::Stock::CONVERT, Gtk::ICON_SIZE_MENU)));
    item->signal_activate().connect(
      sigc::mem_fun(*this, &NoteWindow::sync_item_selected));
    item->show();
    m_plugin_menu->add(*item);
#endif

    tb->show_all();
    return tb;
  }


  void NoteWindow::sync_item_selected ()
  {
    ActionManager::obj()["NoteSynchronizationAction"]->activate();
  }

  
  //
  // This menu can be
  // populated by individual plugins using
  // NotePlugin.AddPluginMenuItem().
  //
  Gtk::Menu *NoteWindow::make_plugin_menu()
  {
    Gtk::Menu *menu = new Gtk::Menu();
    return menu;
  }


  //
  // Find context menu
  //
  // Find, Find Next, Find Previous menu items.  Next nd previous
  // are only sensitized when there are search results for this
  // buffer to iterate.
  //
  Gtk::Menu *NoteWindow::make_find_menu()
  {
    Gtk::Menu *menu = manage(new Gtk::Menu());
    menu->set_accel_group(m_accel_group);

    Gtk::ImageMenuItem *find = manage(new Gtk::ImageMenuItem(_("_Find..."), true));
    find->set_image(*manage(new Gtk::Image (Gtk::Stock::FIND, Gtk::ICON_SIZE_MENU)));
    find->signal_activate().connect(sigc::mem_fun(*this, &NoteWindow::find_button_clicked));
    find->add_accelerator("activate", m_accel_group,
                          GDK_F, Gdk::CONTROL_MASK,
                          Gtk::ACCEL_VISIBLE);
    find->show();

    Gtk::ImageMenuItem *find_next =  manage(new Gtk::ImageMenuItem (_("Find _Next"), true));
    find_next->set_image(*manage(new Gtk::Image(Gtk::Stock::GO_FORWARD, Gtk::ICON_SIZE_MENU)));
    find_next->set_sensitive(get_find_bar().find_next_button().is_sensitive());

    find_next->signal_activate().connect(sigc::mem_fun(*this, &NoteWindow::find_next_activate));
    find_next->add_accelerator("activate", m_accel_group,
                              GDK_G, Gdk::CONTROL_MASK,
                              Gtk::ACCEL_VISIBLE);
    find_next->show();

    Gtk::ImageMenuItem *find_previous = manage(new Gtk::ImageMenuItem (_("Find _Previous"), true));
    find_previous->set_image(*manage(new Gtk::Image(Gtk::Stock::GO_BACK, Gtk::ICON_SIZE_MENU)));
    find_previous->set_sensitive(get_find_bar().find_previous_button().is_sensitive());

    find_previous->signal_activate().connect(sigc::mem_fun(*this, &NoteWindow::find_previous_activate));
    find_previous->add_accelerator("activate", m_accel_group,
                                  GDK_G, (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK),
                                  Gtk::ACCEL_VISIBLE);
    find_previous->show();

    menu->append(*find);
    menu->append(*find_next);
    menu->append(*find_previous);

    return menu;
  }


  void NoteWindow::find_button_clicked()
  {
    get_find_bar().show_all();
    get_find_bar().property_visible() = true;
    get_find_bar().set_search_text(m_note.get_buffer()->get_selection());
  }

  void NoteWindow::find_next_activate()
  {
    get_find_bar().find_next_button().clicked();
  }

  void NoteWindow::find_previous_activate()
  {
    get_find_bar().find_previous_button().clicked();
  }

  void NoteWindow::find_bar_hidden()
  {
    // Reposition the current focus back to the editor so the
    // cursor will be ready for typing.
    if(m_editor) {
      m_editor->grab_focus();
    }
  }

  //
  // Link menu item activate
  //
  // Create a new note, names according to the buffer's selected
  // text.  Does nothing if there is no active selection.
  //
  void NoteWindow::link_button_clicked()
  {
    std::string select = m_note.get_buffer()->get_selection();
    if (select.empty())
      return;
    
    std::string body_unused;
    std::string title = NoteManager::split_title_from_content(select, body_unused);
    if (title.empty())
      return;

    Note::Ptr match = m_note.manager().find(title);
    if (!match) {
      try {
        match = m_note.manager().create(select);
      } 
      catch (const sharp::Exception & e) {
        utils::HIGMessageDialog dialog(this,
          GTK_DIALOG_DESTROY_WITH_PARENT,
          Gtk::MESSAGE_ERROR,  Gtk::BUTTONS_OK,
          _("Cannot create note"), e.what());
        dialog.run ();
        return;
      }
    }

    match->get_window()->present();
  }

  void NoteWindow::open_help_activate()
  {
    utils::show_help("gnote", "editing-notes", get_screen()->gobj(), this);
  }

  void NoteWindow::create_new_note ()
  {
    ActionManager::obj()["NewNoteAction"]->activate();
  }

  void NoteWindow::search_button_clicked()
  {
    NoteRecentChanges *search = NoteRecentChanges::get_instance(m_note.manager());
    if (!m_note.get_buffer()->get_selection().empty()) {
      search->set_search_text(m_note.get_buffer()->get_selection());
    }
    search->present();
  }

  void NoteWindow::change_depth_right_handler()
  {
    Glib::RefPtr<NoteBuffer>::cast_static(m_editor->get_buffer())->change_cursor_depth_directional(true);
  }

  void NoteWindow::change_depth_left_handler()
  {
    Glib::RefPtr<NoteBuffer>::cast_static(m_editor->get_buffer())->change_cursor_depth_directional(false);
  }

  
  NoteFindBar::NoteFindBar(Note & note)
    : Gtk::HBox(false, 0)
    , m_note(note)
    , m_next_button(_("_Next"), true)
    , m_prev_button(_("_Previous"), true)
    , m_entry_changed_timeout(NULL)
    , m_note_changed_timeout(NULL)
    , m_shift_key_pressed(false)
  {
    set_border_width(2);
    Gtk::Button *button = manage(new Gtk::Button());
    button->set_image(*manage(new Gtk::Image(Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU)));
    button->set_relief(Gtk::RELIEF_NONE);
    button->signal_clicked().connect(sigc::mem_fun(*this, &NoteFindBar::hide_find_bar));
    button->show ();
    pack_start(*button, false, false, 4);

    Gtk::Label *label = manage(new Gtk::Label(_("_Find:"), true));
    label->show();
    pack_start(*label, false, false, 0);

    label->set_mnemonic_widget(m_entry);
    m_entry.signal_changed().connect(sigc::mem_fun(*this, &NoteFindBar::on_find_entry_changed));
    m_entry.signal_activate().connect(sigc::mem_fun(*this, &NoteFindBar::on_find_entry_activated));
    m_entry.show();
    pack_start(m_entry, true, true, 0);

    m_prev_button.set_image(*manage(new Gtk::Arrow (Gtk::ARROW_LEFT, 
                                                    Gtk::SHADOW_NONE)));
    m_prev_button.set_relief(Gtk::RELIEF_NONE);
    m_prev_button.set_sensitive(false);
    m_prev_button.set_focus_on_click(false);
    m_prev_button.signal_clicked().connect(sigc::mem_fun(*this, &NoteFindBar::on_prev_clicked));
    m_prev_button.show();
    pack_start(m_prev_button, false, false, 0);

    m_next_button.set_image(*manage(new Gtk::Arrow (Gtk::ARROW_RIGHT, 
                                                    Gtk::SHADOW_NONE)));
    m_next_button.set_relief(Gtk::RELIEF_NONE);
    m_next_button.set_sensitive(false);
    m_next_button.set_focus_on_click(false);
    m_next_button.signal_clicked().connect(sigc::mem_fun(*this, &NoteFindBar::on_next_clicked));
    m_next_button.show();
    pack_start(m_next_button, false, false, 0);

    // Bind ESC to close the FindBar if it's open and has
    // focus or the window otherwise.  Also bind Return and
    // Shift+Return to advance the search if the search
    // entry has focus.
    m_entry.signal_key_press_event().connect(sigc::mem_fun(*this, &NoteFindBar::on_key_pressed));
    m_entry.signal_key_release_event().connect(sigc::mem_fun(*this, &NoteFindBar::on_key_released));
  }

  NoteFindBar::~NoteFindBar()
  {
    delete m_entry_changed_timeout;
    delete m_note_changed_timeout;
  }


  void NoteFindBar::on_show()
  {
    m_entry.grab_focus();

    // Highlight words from a previous existing search
    highlight_matches(true);

    // Call PerformSearch on newly inserted text when
    // the FindBar is visible
    m_insert_cid = m_note.get_buffer()->signal_insert()
      .connect(sigc::mem_fun(*this, &NoteFindBar::on_insert_text));
    m_delete_cid = m_note.get_buffer()->signal_erase()
      .connect(sigc::mem_fun(*this, &NoteFindBar::on_delete_range));

    Gtk::HBox::on_show();
  }
  
  void NoteFindBar::on_hide()
  {
    highlight_matches(false);
    
    // Prevent searching when the FindBar is not visible
    m_insert_cid.disconnect();
    m_delete_cid.disconnect();

    Gtk::HBox::on_hide();
  }

  void NoteFindBar::hide_find_bar()
  {
    hide();
  }

  void NoteFindBar::on_prev_clicked()
  {
    if (m_current_matches.empty() || m_current_matches.size() == 0)
      return;

    std::list<Match>::reverse_iterator iter(m_current_matches.rbegin());
    for ( ; iter != m_current_matches.rend() ; ++iter) {
      Match & match(*iter);
      
      Glib::RefPtr<NoteBuffer> buffer = match.buffer;
      Gtk::TextIter selection_start, selection_end;
      buffer->get_selection_bounds(selection_start, selection_end);
      Gtk::TextIter end = buffer->get_iter_at_mark(match.start_mark);

      if (end.get_offset() < selection_start.get_offset()) {
        jump_to_match(match);
        return;
      }
    }

    // Wrap to first match
    jump_to_match (*m_current_matches.rbegin());
  }

  void NoteFindBar::on_next_clicked()
  {
    if (m_current_matches.empty() || m_current_matches.size() == 0)
      return;

    std::list<Match>::iterator iter(m_current_matches.begin());
    for ( ; iter != m_current_matches.end() ; ++iter) {
      Match & match(*iter);

      Glib::RefPtr<NoteBuffer> buffer = match.buffer;
      Gtk::TextIter selection_start, selection_end;
      buffer->get_selection_bounds(selection_start, selection_end);
      Gtk::TextIter start = buffer->get_iter_at_mark(match.start_mark);

      if (start.get_offset() >= selection_end.get_offset()) {
        jump_to_match(match);
        return;
      }
    }

    // Else wrap to first match
    jump_to_match(*m_current_matches.begin());
  }

  void NoteFindBar::jump_to_match(const Match & match)
  {
    Glib::RefPtr<NoteBuffer> buffer(match.buffer);

    Gtk::TextIter start = buffer->get_iter_at_mark(match.start_mark);
    Gtk::TextIter end = buffer->get_iter_at_mark(match.end_mark);

    // Move cursor to end of match, and select match text
    buffer->place_cursor(end);
    buffer->move_mark(buffer->get_selection_bound(), start);

    Gtk::TextView *editor = m_note.get_window()->editor();
    editor->scroll_mark_onscreen(buffer->get_insert());
  }


  void NoteFindBar::on_find_entry_activated()
  {
    if (m_entry_changed_timeout) {
      m_entry_changed_timeout->cancel();
      delete m_entry_changed_timeout;
      m_entry_changed_timeout = NULL;
    }

    if (m_prev_search_text.empty() &&  !search_text().empty() &&
        (m_prev_search_text == search_text())) {
      m_next_button.clicked();
    }
    else {
      perform_search(true);
    }
  }

  void NoteFindBar::on_find_entry_changed()
  {
    if (!m_entry_changed_timeout) {
      m_entry_changed_timeout = new utils::InterruptableTimeout();
      m_entry_changed_timeout->signal_timeout.connect(
        sigc::mem_fun(*this, &NoteFindBar::entry_changed_timeout));
    }

    if (search_text().empty()) {
      perform_search(false);
    } 
    else {
      m_entry_changed_timeout->reset(500);
    }
  }


  // Called after .5 seconds of typing inactivity, or on explicit
  // activate.  Redo the search and update the results...
  void NoteFindBar::entry_changed_timeout()
  {
    delete m_entry_changed_timeout;
    m_entry_changed_timeout = NULL;

    if (search_text().empty())
      return;

    perform_search(true);
  }


  void NoteFindBar::perform_search (bool scroll_to_hit)
  {
    cleanup_matches();

    Glib::ustring text = search_text();
    if (text.empty())
      return;

    text = text.lowercase();

    std::vector<Glib::ustring> words;
    Search::split_watching_quotes(words, text);

    find_matches_in_buffer(m_note.get_buffer(), words, m_current_matches);

    m_prev_search_text = search_text();

    if (!m_current_matches.empty()) {
      highlight_matches(true);

      // Select/scroll to the first match
      if (scroll_to_hit)
        on_next_clicked();
    }

    update_sensitivity ();
  }


  void NoteFindBar::update_sensitivity()
  {
    if (search_text().empty()) {
      m_next_button.set_sensitive(false);
      m_prev_button.set_sensitive(false);
    }

    if (!m_current_matches.empty() && m_current_matches.size() > 0) {
      m_next_button.set_sensitive(true);
      m_prev_button.set_sensitive(true);
    } 
    else {
      m_next_button.set_sensitive(false);
      m_prev_button.set_sensitive(false);
    }
  }

  void NoteFindBar::update_search()
  {
    if (!m_note_changed_timeout) {
      m_note_changed_timeout = new utils::InterruptableTimeout();
      m_note_changed_timeout->signal_timeout.connect(
        sigc::mem_fun(*this, &NoteFindBar::note_changed_timeout));
    }

    if (search_text().empty()) {
      perform_search(false);
    } 
    else {
      m_note_changed_timeout->reset(500);
    }
  }

  // Called after .5 seconds of typing inactivity to update
  // the search when the text of a note changes.  This prevents
  // the search from running on every single change made in a
  // note.
  void NoteFindBar::note_changed_timeout()
  {
    delete m_note_changed_timeout;
    m_note_changed_timeout = NULL;

    if (search_text().empty())
      return;

    perform_search(false);
  }

  void NoteFindBar::on_insert_text(const Gtk::TextBuffer::iterator &, 
                                   const Glib::ustring &, int)
  {
    update_search();
  }

  void NoteFindBar::on_delete_range(const Gtk::TextBuffer::iterator &, 
                                    const Gtk::TextBuffer::iterator &)
  {
    update_search();
  }

  bool NoteFindBar::on_key_pressed(GdkEventKey *ev)
  {
    switch (ev->keyval)
    {
    case GDK_Escape:
      hide();
      break;
    case GDK_Shift_L:
    case GDK_Shift_R:
      m_shift_key_pressed = true;
      return false;
      break;
    case GDK_Return:
      if (m_shift_key_pressed)
        m_prev_button.clicked();
      break;
    default:
      return false;
      break;
    }
    return true;
  }


  bool NoteFindBar::on_key_released(GdkEventKey *ev)
  {
    switch (ev->keyval)
    {
    case GDK_Shift_L:
    case GDK_Shift_R:
      m_shift_key_pressed = false;
      break;
    }
    return false;
  }

  Glib::ustring NoteFindBar::search_text()
  {
    return sharp::string_trim(m_entry.get_text());
  }


  void NoteFindBar::set_search_text(const Glib::ustring &value)
  {
    if(!value.empty()) {
      m_entry.set_text(value);
    }
    m_entry.grab_focus();
  }


  void NoteFindBar::highlight_matches(bool highlight)
  {
    if(m_current_matches.empty()) {
      return;
    }

    for(std::list<Match>::iterator iter = m_current_matches.begin();
        iter != m_current_matches.end(); ++iter) {
      Match &match(*iter);
      Glib::RefPtr<NoteBuffer> buffer = match.buffer;

      if (match.highlighting != highlight) {
        Gtk::TextIter start = buffer->get_iter_at_mark(match.start_mark);
        Gtk::TextIter end = buffer->get_iter_at_mark(match.end_mark);

        match.highlighting = highlight;

        if (match.highlighting) {
          buffer->apply_tag_by_name("find-match", start, end);
        }
        else {
          buffer->remove_tag_by_name("find-match", start, end);
        }
      }
    }
  }


  void NoteFindBar::cleanup_matches()
  {
    if (!m_current_matches.empty()) {
      highlight_matches (false /* unhighlight */);

      for(std::list<Match>::const_iterator iter = m_current_matches.begin();
          iter != m_current_matches.end(); ++iter) {
        const Match &match(*iter);
        match.buffer->delete_mark(match.start_mark);
        match.buffer->delete_mark(match.end_mark);
      }

      m_current_matches.clear();
    }
    update_sensitivity ();
  }



  void NoteFindBar::find_matches_in_buffer(const Glib::RefPtr<NoteBuffer> & buffer, 
                                           const std::vector<Glib::ustring> & words,
                                           std::list<NoteFindBar::Match> & matches)
  {
    matches.clear();
    Glib::ustring note_text = buffer->get_slice (buffer->begin(),
                                               buffer->end(),
                                               false /* hidden_chars */);
    note_text = note_text.lowercase();

    for(std::vector<Glib::ustring>::const_iterator iter = words.begin();
        iter != words.end(); ++iter) {
      const Glib::ustring & word(*iter);
      Glib::ustring::size_type idx = 0;
      bool this_word_found = false;

      if (word.empty())
        continue;

      while(true) {
        idx = note_text.find(word, idx);
        if (idx == Glib::ustring::npos) {
          if (this_word_found) {
            break;
          }
          else {
            matches.clear();
            return;
          }
        }

        this_word_found = true;

        Gtk::TextIter start = buffer->get_iter_at_offset(idx);
        Gtk::TextIter end = start;
        end.forward_chars(word.length());

        Match match;
        match.buffer = buffer;
        match.start_mark = buffer->create_mark(start, false);
        match.end_mark = buffer->create_mark(end, true);
        match.highlighting = false;

        matches.push_back(match);

        idx += word.length();
      }
    }
  }


  // FIXME: Tags applied to a word should hold over the space
  // between the next word, as thats where you'll start typeing.
  // Tags are only active -after- a character with that tag.  This
  // is different from the way gtk-textbuffer applies tags.

  //
  // Text menu
  //
  // Menu for font style and size, and set the active radio
  // menuitem depending on the cursor poition.
  //
  NoteTextMenu::NoteTextMenu(const Glib::RefPtr<Gtk::AccelGroup>& accel_group,
                             const Glib::RefPtr<NoteBuffer> & buffer, UndoManager & undo_manager)
    : Gtk::Menu()
    , m_buffer(buffer)
    , m_undo_manager(undo_manager)
    , m_bold(_("<b>_Bold</b>"), true)
    , m_italic(_("<i>_Italic</i>"), true)
    , m_strikeout(_("<s>_Strikeout</s>"), true)
    , m_highlight(Glib::ustring("<span background=\"yellow\">")
                  + _("_Highlight") + "</span>", true)
    , m_fontsize_group()
    , m_normal(m_fontsize_group, _("_Normal"), true)
    , m_huge(m_fontsize_group, Glib::ustring("<span size=\"x-large\">")
             + _("Hu_ge") + "</span>", true)
    , m_large(m_fontsize_group, Glib::ustring("<span size=\"large\">")
            + _("_Large") + "</span>", true)
       ,  m_small(m_fontsize_group, Glib::ustring("<span size=\"small\">")
                  + _("S_mall") + "</span>", true)
    , m_hidden_no_size(m_fontsize_group, "", true)
    , m_bullets(_("Bullets"))
    , m_increase_indent(Gtk::Stock::INDENT)
    , m_decrease_indent(Gtk::Stock::UNINDENT)
    , m_increase_font(_("Increase Font Size"), true)
    , m_decrease_font(_("Decrease Font Size"), true)
    {
      m_undo = manage(new Gtk::ImageMenuItem (Gtk::Stock::UNDO));
//      m_undo->set_accel_group(accel_group);
      m_undo->signal_activate().connect(sigc::mem_fun(*this, &NoteTextMenu::undo_clicked));
      m_undo->add_accelerator ("activate", accel_group,
                               GDK_Z,
                               Gdk::CONTROL_MASK,
                               Gtk::ACCEL_VISIBLE);
      m_undo->show();
      append(*m_undo);

      m_redo = manage(new Gtk::ImageMenuItem (Gtk::Stock::REDO));
//      m_redo->set_accel_group(accel_group);
      m_redo->signal_activate().connect(sigc::mem_fun(*this, &NoteTextMenu::redo_clicked));
      m_redo->add_accelerator ("activate", accel_group,
                               GDK_Z, (Gdk::CONTROL_MASK |
                                       Gdk::SHIFT_MASK),
                               Gtk::ACCEL_VISIBLE);
      m_redo->show();
      append(*m_redo);

      Gtk::SeparatorMenuItem *undo_spacer = manage(new Gtk::SeparatorMenuItem());
      append(*undo_spacer);

      // Listen to events so we can sensitize and
      // enable keybinding
      undo_manager.signal_undo_changed().connect(sigc::mem_fun(*this, &NoteTextMenu::undo_changed));

      Glib::Quark tag_quark("Tag");
      markup_label(m_bold);
      m_bold.set_data(tag_quark, (void *)"bold");
      m_bold.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_style_clicked), &m_bold));
      m_bold.add_accelerator ("activate", accel_group,
                           GDK_B,
                           Gdk::CONTROL_MASK,
                           Gtk::ACCEL_VISIBLE);

      markup_label (m_italic);
      m_italic.set_data(tag_quark, (void *)"italic");
      m_italic.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_style_clicked), &m_italic));
      m_italic.add_accelerator ("activate",
                             accel_group,
                             GDK_I,
                             Gdk::CONTROL_MASK,
                             Gtk::ACCEL_VISIBLE);

      markup_label (m_strikeout);
      m_strikeout.set_data(tag_quark, (void *)"strikethrough");
      m_strikeout.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_style_clicked), &m_strikeout));
      m_strikeout.add_accelerator ("activate",
                                accel_group,
                                GDK_S,
                                Gdk::CONTROL_MASK,
                                Gtk::ACCEL_VISIBLE);

      markup_label (m_highlight);
      m_highlight.set_data(tag_quark, (void *)"highlight");
      m_highlight.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_style_clicked), &m_highlight));
      m_highlight.add_accelerator ("activate",
                                accel_group,
                                GDK_H,
                                Gdk::CONTROL_MASK,
                                Gtk::ACCEL_VISIBLE);

      Gtk::SeparatorMenuItem *spacer1 = manage(new Gtk::SeparatorMenuItem());

      Gtk::MenuItem *font_size = manage(new Gtk::MenuItem(_("Font Size")));
      font_size->set_sensitive(false);

      markup_label(m_normal);
      m_normal.set_active(true);
      m_normal.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_size_activated), &m_normal));

      markup_label(m_huge);
      m_huge.set_data(tag_quark, (void*)"size:huge");
      m_huge.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_size_activated), &m_huge));

      markup_label(m_large);
      m_large.set_data(tag_quark, (void*)"size:large");
      m_large.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_size_activated), &m_large));

      markup_label(m_small);
      m_small.set_data(tag_quark, (void*)"size:small");
      m_small.signal_activate()
        .connect(sigc::bind(sigc::mem_fun(*this, &NoteTextMenu::font_size_activated), &m_small));

      m_hidden_no_size.hide();

      m_increase_font.add_accelerator ("activate",
                                       accel_group,
                                       GDK_plus,
                                       Gdk::CONTROL_MASK,
                                       Gtk::ACCEL_VISIBLE);
      m_increase_font.signal_activate()
        .connect(sigc::mem_fun(*this, &NoteTextMenu::increase_font_clicked));

      m_decrease_font.add_accelerator ("activate",
                                       accel_group,
                                       GDK_minus,
                                       Gdk::CONTROL_MASK,
                                       Gtk::ACCEL_VISIBLE);
      m_decrease_font.signal_activate()
        .connect(sigc::mem_fun(*this, &NoteTextMenu::decrease_font_clicked));

      Gtk::SeparatorMenuItem *spacer2 = manage(new Gtk::SeparatorMenuItem());

      m_bullets_clicked_cid = m_bullets.signal_activate()
        .connect(sigc::mem_fun(*this, &NoteTextMenu::toggle_bullets_clicked));

//      m_increase_indent.set_accel_group(accel_group);
      m_increase_indent.add_accelerator ("activate", accel_group,
                                       GDK_Right,
                                       Gdk::MOD1_MASK,
                                       Gtk::ACCEL_VISIBLE);
      m_increase_indent.signal_activate()
        .connect(sigc::mem_fun(*this, &NoteTextMenu::increase_indent_clicked));
      m_increase_indent.show();

//      m_decrease_indent.set_accel_group(accel_group);
      m_decrease_indent.add_accelerator ("activate", accel_group,
                                         GDK_Left,
                                         Gdk::MOD1_MASK,
                                         Gtk::ACCEL_VISIBLE);
      m_decrease_indent.signal_activate()
        .connect(sigc::mem_fun(*this, &NoteTextMenu::decrease_indent_clicked));
      m_decrease_indent.show();

      refresh_state();

      append(m_bold);
      append(m_italic);
      append(m_strikeout);
      append(m_highlight);
      append(*spacer1);
      append(*font_size);
      append(m_small);
      append(m_normal);
      append(m_large);
      append(m_huge);
      append(m_increase_font);
      append(m_decrease_font);
      append(*spacer2);
      append(m_bullets);
      append(m_increase_indent);
      append(m_decrease_indent);
      show_all ();
    }

  void NoteTextMenu::on_show()
  {
    refresh_state();
    Gtk::Menu::on_show();
  }

  void NoteTextMenu::markup_label (Gtk::MenuItem & item)
  {
    Gtk::Label *label = dynamic_cast<Gtk::Label*>(item.get_child());
    if(label) {
      label->set_use_markup(true);
      label->set_use_underline(true);
    }
  }


  void NoteTextMenu::refresh_sizing_state()
  {
    Gtk::TextIter cursor = m_buffer->get_iter_at_mark(m_buffer->get_insert());
    Gtk::TextIter selection = m_buffer->get_iter_at_mark(m_buffer->get_selection_bound());

    // When on title line, activate the hidden menu item
    if ((cursor.get_line() == 0) || (selection.get_line() == 0)) {
      m_hidden_no_size.set_active(true);
      return;
    }

    bool has_size = false;
    bool active;
    active = m_buffer->is_active_tag ("size:huge");
    has_size |= active;
    m_huge.set_active(active);
    active = m_buffer->is_active_tag ("size:large");
    has_size |= active;
    m_large.set_active(active);
    active = m_buffer->is_active_tag ("size:small");
    has_size |= active;
    m_small.set_active(active);

    m_normal.set_active(!has_size);

  }

  void NoteTextMenu::refresh_state ()
  {
    m_event_freeze = true;

    m_bold.set_active(m_buffer->is_active_tag("bold"));
    m_italic.set_active(m_buffer->is_active_tag("italic"));
    m_strikeout.set_active(m_buffer->is_active_tag("strikethrough"));
    m_highlight.set_active(m_buffer->is_active_tag("highlight"));

    bool inside_bullets = m_buffer->is_bulleted_list_active();
    bool can_make_bulleted_list = m_buffer->can_make_bulleted_list();
    m_bullets_clicked_cid.block();
    m_bullets.set_active(inside_bullets);
    m_bullets_clicked_cid.unblock();
    m_bullets.set_sensitive(can_make_bulleted_list);
    m_increase_indent.set_sensitive(inside_bullets);
    m_decrease_indent.set_sensitive(inside_bullets);

    refresh_sizing_state ();

    m_undo->set_sensitive(m_undo_manager.get_can_undo());
    m_redo->set_sensitive(m_undo_manager.get_can_redo());

    m_event_freeze = false;
  }

  //
  // Font-style menu item activate
  //
  // Toggle the style tag for the current text.  Style tags are
  // stored in a "Tag" member of the menuitem's Data.
  //
  void NoteTextMenu::font_style_clicked(Gtk::CheckMenuItem * item)
  {
    if (m_event_freeze)
      return;

    const char * tag = (const char *)item->get_data(Glib::Quark("Tag"));

    if (tag)
      m_buffer->toggle_active_tag(tag);
  }

  //
  // Font-style menu item activate
  //
  // Set the font size tag for the current text.  Style tags are
  // stored in a "Tag" member of the menuitem's Data.
  //

  // FIXME: Change this back to use FontSizeToggled instead of using the
  // Activated signal.  Fix the Text menu so it doesn't show a specific
  // font size already selected if multiple sizes are highlighted. The
  // Activated event is used here to fix
  // http://bugzilla.mate.org/show_bug.cgi?id=412404.
  void NoteTextMenu::font_size_activated(Gtk::RadioMenuItem *item)
  {
    if (m_event_freeze)
      return;

    if (!item->get_active())
      return;

    m_buffer->remove_active_tag ("size:huge");
    m_buffer->remove_active_tag ("size:large");
    m_buffer->remove_active_tag ("size:small");

    const char * tag = (const char *)item->get_data(Glib::Quark("Tag"));
    if (tag)
      m_buffer->set_active_tag(tag);
  }

  void NoteTextMenu::increase_font_clicked ()
  {
    if (m_event_freeze)
      return;

    if (m_buffer->is_active_tag ("size:small")) {
      m_buffer->remove_active_tag ("size:small");
    } 
    else if (m_buffer->is_active_tag ("size:large")) {
      m_buffer->remove_active_tag ("size:large");
      m_buffer->set_active_tag ("size:huge");
    } 
    else if (m_buffer->is_active_tag ("size:huge")) {
      // Maximum font size, do nothing
    } 
    else {
      // Current font size is normal
      m_buffer->set_active_tag ("size:large");
    }
 }

  void NoteTextMenu::decrease_font_clicked ()
  {
    if (m_event_freeze)
      return;

    if (m_buffer->is_active_tag ("size:small")) {
// Minimum font size, do nothing
    } 
    else if (m_buffer->is_active_tag ("size:large")) {
      m_buffer->remove_active_tag ("size:large");
    } 
    else if (m_buffer->is_active_tag ("size:huge")) {
      m_buffer->remove_active_tag ("size:huge");
      m_buffer->set_active_tag ("size:large");
    } 
    else {
// Current font size is normal
      m_buffer->set_active_tag ("size:small");
    }
  }

  void NoteTextMenu::undo_clicked ()
  {
    if (m_undo_manager.get_can_undo()) {
      DBG_OUT("Running undo...");
      m_undo_manager.undo();
    }
  }

  void NoteTextMenu::redo_clicked ()
  {
    if (m_undo_manager.get_can_redo()) {
      DBG_OUT("Running redo...");
      m_undo_manager.redo();
    }
  }

  void NoteTextMenu::undo_changed ()
  {
    m_undo->set_sensitive(m_undo_manager.get_can_undo());
    m_redo->set_sensitive(m_undo_manager.get_can_redo());
  }


    //
    // Bulleted list handlers
    //
  void  NoteTextMenu::toggle_bullets_clicked()
  {
    m_buffer->toggle_selection_bullets();
  }

  void  NoteTextMenu::increase_indent_clicked()
  {
    m_buffer->increase_cursor_depth();
  }

  void  NoteTextMenu::decrease_indent_clicked()
  {
    m_buffer->decrease_cursor_depth();
  }

}
