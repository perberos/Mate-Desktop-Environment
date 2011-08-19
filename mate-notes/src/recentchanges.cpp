/*
 * gnote
 *
 * Copyright (C) 2010-2011 Aurimas Cernius
 * Copyright (C) 2010 Debarshi Ray
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

#include <boost/format.hpp>

#include <glibmm/i18n.h>
#include <gtkmm/image.h>
#include <gtkmm/main.h>
#include <gtkmm/stock.h>
#include <gtkmm/table.h>
#include <gtkmm/treestore.h>

#include "sharp/datetime.hpp"
#include "sharp/string.hpp"
#include "actionmanager.hpp"
#include "debug.hpp"
#include "gnote.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "recentchanges.hpp"
#include "recenttreeview.hpp"
#include "search.hpp"
#include "tagmanager.hpp"
#include "utils.hpp"
#include "notebooks/notebookmanager.hpp"
#include "notebooks/notebookstreeview.hpp"


namespace gnote {

  bool        NoteRecentChanges::s_static_inited = false;
  Glib::RefPtr<Gdk::Pixbuf> NoteRecentChanges::s_note_icon;
  Glib::RefPtr<Gdk::Pixbuf> NoteRecentChanges::s_all_notes_icon;
  Glib::RefPtr<Gdk::Pixbuf> NoteRecentChanges::s_unfiled_notes_icon;
  Glib::RefPtr<Gdk::Pixbuf> NoteRecentChanges::s_notebook_icon;
  std::list<std::string>    NoteRecentChanges::s_previous_searches;
  NoteRecentChanges        *NoteRecentChanges::s_instance = NULL;



  NoteRecentChanges *NoteRecentChanges::get_instance(NoteManager& m)
  {
    if(!s_instance) {
      s_instance = new NoteRecentChanges(m);
    }
    return s_instance;
  }


  void NoteRecentChanges::_init_static()
  {
    if(s_static_inited) {
      return;
    }
    s_note_icon = utils::get_icon ("note", 22);
    s_all_notes_icon = utils::get_icon ("filter-note-all", 22);
    s_unfiled_notes_icon = utils::get_icon ("filter-note-unfiled", 22);
    s_notebook_icon = utils::get_icon ("notebook", 22);
    s_static_inited = true;
  }

  NoteRecentChanges::NoteRecentChanges(NoteManager& m)
    : utils::ForcedPresentWindow(_("Search All Notes"))
    , m_manager(m)
    , m_menubar(NULL)
    , m_find_combo(Gtk::ListStore::create(m_find_combo_columns), 0)
    , m_clear_search_button(Gtk::Stock::CLEAR)
    , m_case_sensitive(_("C_ase Sensitive"), true)
    , m_content_vbox(false, 0)
    , m_matches_column(NULL)
    , m_tree(NULL)
    , m_entry_changed_timeout(NULL)
    , m_clickX(0), m_clickY(0)
  {
    _init_static();
//    get_window()->set_icon_name("gnote");
    set_default_size(450,400);
    set_resizable(true);

    add_accel_group(ActionManager::obj().get_ui()->get_accel_group());

    m_menubar = create_menu_bar ();

    Gtk::Image *image = manage(new Gtk::Image (utils::get_icon ("system-search", 48)));

    Gtk::Label *label = manage(new Gtk::Label (_("_Search:"), true));
    label->property_xalign() = 1;

    label->set_mnemonic_widget(m_find_combo);
    m_find_combo.signal_changed()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_changed));
    m_find_combo.get_entry()->set_activates_default(false);
    m_find_combo.get_entry()->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_entry_activated));

    Glib::RefPtr<Gtk::ListStore> model 
      = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(m_find_combo.get_model());
    for(std::list<std::string>::const_iterator liter = s_previous_searches.begin();
        liter != s_previous_searches.end(); ++liter) {
      Gtk::TreeIter iter = model->append();
      iter->set_value(0, *liter);
    }

    m_clear_search_button.set_sensitive(false);
    m_clear_search_button.signal_clicked()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::clear_search_clicked));
    m_clear_search_button.show ();

    m_case_sensitive.signal_toggled()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_case_sensitive_toggled));

    Gtk::Table *table = manage(new Gtk::Table (2, 3, false));
    table->attach (*label, 0, 1, 0, 1, Gtk::SHRINK, (Gtk::AttachOptions)0, 0, 0);
    table->attach (m_find_combo, 1, 2, 0, 1);
    table->attach (m_case_sensitive, 1, 2, 1, 2);
    table->attach (m_clear_search_button,
                  2, 3, 0, 1,
                  Gtk::SHRINK, (Gtk::AttachOptions)0, 0, 0);
    table->property_column_spacing() = 4;
    table->show_all ();

    Gtk::HBox *hbox = manage(new Gtk::HBox (false, 2));
    hbox->set_border_width(8);
    hbox->pack_start (*image, false, false, 4);
    hbox->pack_start (*table, true, true, 0);
    hbox->show_all ();
            
    // Notebooks Pane
    Gtk::Widget *notebooksPane = Gtk::manage(make_notebooks_pane ());
    notebooksPane->show ();
                    
    make_recent_tree ();
    m_tree = manage(m_tree);
    m_tree->show ();

    m_status_bar.set_has_resize_grip(true);
    m_status_bar.show();

    // Update on changes to notes
    m.signal_note_deleted.connect(sigc::mem_fun(*this, &NoteRecentChanges::on_notes_changed));
    m.signal_note_added.connect(sigc::mem_fun(*this, &NoteRecentChanges::on_notes_changed));
    m.signal_note_renamed.connect(sigc::mem_fun(*this, &NoteRecentChanges::on_note_renamed));
    m.signal_note_saved.connect(sigc::mem_fun(*this, &NoteRecentChanges::on_note_saved));

    // List all the current notes
    update_results ();

    m_matches_window.set_shadow_type(Gtk::SHADOW_IN);

    // Reign in the window size if there are notes with long
    // names, or a lot of notes...

    int h, w;
    m_tree->get_size_request (h,w);
    m_matches_window.set_size_request(std::min(h, 420), std::min(w, 480));
                        
    m_matches_window.property_hscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
    m_matches_window.property_vscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
    m_matches_window.add (*m_tree);
    m_matches_window.show ();

    m_hpaned.set_position(150);
    m_hpaned.add1 (*notebooksPane);
    m_hpaned.add2 (m_matches_window);
    m_hpaned.show ();

    restore_position ();

    Gtk::VBox *vbox = manage(new Gtk::VBox (false, 8));
    vbox->set_border_width(6);
    vbox->pack_start (*hbox, false, false, 0);
    vbox->pack_start (m_hpaned, true, true, 0);
    vbox->pack_start (m_status_bar, false, false, 0);
    vbox->show ();

    // Use another VBox to place the MenuBar
    // right at thetop of the window.
    m_content_vbox.pack_start (*manage(m_menubar), false, false, 0);
    m_content_vbox.pack_start (*vbox, true, true, 0);
    m_content_vbox.show ();

    add (m_content_vbox);
    signal_delete_event().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_delete));
    signal_key_press_event()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_key_pressed)); // For Escape
                        
    // Watch when notes are added to notebooks so the search
    // results will be updated immediately instead of waiting
    // until the note's queue_save () kicks in.
    notebooks::NotebookManager::instance().signal_note_added_to_notebook()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_note_added_to_notebook));
    notebooks::NotebookManager::instance().signal_note_added_to_notebook()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_note_removed_to_notebook));
                        
    Gtk::Main::signal_quit()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_exiting_event));

  }

  Gtk::MenuBar *NoteRecentChanges::create_menu_bar ()
  {
    ActionManager &am(ActionManager::obj());
    Gtk::MenuBar *menubar = dynamic_cast<Gtk::MenuBar*>(am.get_widget ("/MainWindowMenubar"));

    am ["OpenNoteAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_note));
    am ["DeleteNoteAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_delete_note));
    // from notebook addin
    am ["NewNotebookNoteAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_new_notebook_note));
    am ["OpenNotebookTemplateNoteAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_open_notebook_template_note));
    am ["NewNotebookAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_new_notebook));
    am ["DeleteNotebookAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_delete_notebook));
    // end notebook addin
    am ["CloseWindowAction"]->signal_activate()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_close_window));
    if (Gnote::tray_icon_showing() == false)
      am ["CloseWindowAction"]->set_visible(false);

    // Allow Escape to close the window as well as <Control>W
    // Should be able to add Escape to the CloseAction.  Can't do that
    // until someone fixes AccelGroup.Connect:
    //     http://bugzilla.ximian.com/show_bug.cgi?id=76988)
    //
    // am.UI.AccelGroup.Connect ((uint) Gdk.Key.Escape,
    //   0,
    //   Gtk::AccelFlags.Mask,
    //   OnCloseWindow);

    return menubar;
  }

  Gtk::Widget *NoteRecentChanges::make_notebooks_pane()
  {
    m_notebooksTree = Gtk::manage(
      new notebooks::NotebooksTreeView (notebooks::NotebookManager::instance()
                                        .get_notebooks_with_special_items()));

    m_notebooksTree->get_selection()->set_mode(Gtk::SELECTION_SINGLE);
    m_notebooksTree->set_headers_visible(true);
    m_notebooksTree->set_rules_hint(false);
      
    Gtk::CellRenderer *renderer;
      
    Gtk::TreeViewColumn *column = manage(new Gtk::TreeViewColumn ());
    column->set_title(_("Notebooks"));
    column->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
    column->set_resizable(false);
      
    renderer = manage(new Gtk::CellRendererPixbuf ());
    column->pack_start (*renderer, false);
    column->set_cell_data_func (*renderer,
                                sigc::mem_fun(*this, &NoteRecentChanges::notebook_pixbuf_cell_data_func));
      
    Gtk::CellRendererText *text_renderer = manage(new Gtk::CellRendererText ());
    text_renderer->property_editable() = true;
    column->pack_start (*text_renderer, true);
    column->set_cell_data_func (*text_renderer,
                                sigc::mem_fun(*this, &NoteRecentChanges::notebook_text_cell_data_func));
    text_renderer->signal_edited().connect(sigc::mem_fun(*this, &NoteRecentChanges::on_notebook_row_edited));

    m_notebooksTree->append_column (*column);

    m_notebooksTree->signal_row_activated()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_notebook_row_activated));
    m_on_notebook_selection_changed_cid = m_notebooksTree->get_selection()->signal_changed()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_notebook_selection_changed));
    m_notebooksTree->signal_button_press_event()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_notebooks_tree_button_pressed), false);
    m_notebooksTree->signal_key_press_event()
      .connect(sigc::mem_fun(*this, &NoteRecentChanges::on_notebooks_key_pressed));
      
    m_notebooksTree->show ();
    Gtk::ScrolledWindow *sw = new Gtk::ScrolledWindow ();
    sw->property_hscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
    sw->property_vscrollbar_policy() = Gtk::POLICY_AUTOMATIC;
    sw->set_shadow_type(Gtk::SHADOW_IN);
    sw->add (*m_notebooksTree);
    sw->show ();
      
    return sw;
  }

  
  void NoteRecentChanges::make_recent_tree ()
  {
    m_targets.push_back(Gtk::TargetEntry ("STRING",
                                          Gtk::TARGET_SAME_APP,
                                          0));
    m_targets.push_back(Gtk::TargetEntry ("text/plain",
                                    Gtk::TARGET_SAME_APP,
                                          0));
    m_targets.push_back(Gtk::TargetEntry ("text/uri-list",
                                    Gtk::TARGET_SAME_APP,
                                          1));

    m_tree = Gtk::manage(new RecentTreeView ());
    m_tree->set_headers_visible(true);
    m_tree->set_rules_hint(true);
    m_tree->signal_row_activated().connect(
      sigc::mem_fun(*this, &NoteRecentChanges::on_row_activated));
    m_tree->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    m_tree->get_selection()->signal_changed().connect(
      sigc::mem_fun(*this, &NoteRecentChanges::on_selection_changed));
    m_tree->signal_button_press_event().connect(
      sigc::mem_fun(*this, &NoteRecentChanges::on_treeview_button_pressed), false);
    m_tree->signal_motion_notify_event().connect(
      sigc::mem_fun(*this, &NoteRecentChanges::on_treeview_motion_notify), false);
    m_tree->signal_button_release_event().connect(
      sigc::mem_fun(*this, &NoteRecentChanges::on_treeview_button_released));
    m_tree->signal_key_press_event().connect(
      sigc::mem_fun(*this,
                    &NoteRecentChanges::on_treeview_key_pressed));
    m_tree->signal_drag_data_get().connect(
      sigc::mem_fun(*this, &NoteRecentChanges::on_treeview_drag_data_get));

    m_tree->enable_model_drag_source(m_targets,
      Gdk::BUTTON1_MASK | Gdk::BUTTON3_MASK, Gdk::ACTION_MOVE);

    Gtk::CellRenderer *renderer;

    Gtk::TreeViewColumn *title = manage(new Gtk::TreeViewColumn ());
    title->set_title(_("Note"));
    title->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
    title->set_expand(true);
    title->set_resizable(true);

    renderer = manage(new Gtk::CellRendererPixbuf ());
    title->pack_start (*renderer, false);
    title->add_attribute (*renderer, "pixbuf", 0 /* icon */);

    renderer = manage(new Gtk::CellRendererText ());
    static_cast<Gtk::CellRendererText*>(renderer)->property_ellipsize() = Pango::ELLIPSIZE_END;
    title->pack_start (*renderer, true);
    title->add_attribute (*renderer, "text", 1 /* title */);
    title->set_sort_column_id(1); /* title */
    title->set_sort_indicator(false);
    title->set_reorderable(false);
    title->set_sort_order(Gtk::SORT_ASCENDING);

    m_tree->append_column (*title);

    Gtk::TreeViewColumn *change = manage(new Gtk::TreeViewColumn ());
    change->set_title(_("Last Changed"));
    change->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
    change->set_resizable(false);

    renderer = manage(new Gtk::CellRendererText ());
    renderer->property_xalign() = 1.0;
    change->pack_start (*renderer, false);
    change->add_attribute (*renderer, "text", 2 /* change date */);
    change->set_sort_column_id(2); /* change date */
    change->set_sort_indicator(false);
    change->set_reorderable(false);
    change->set_sort_order(Gtk::SORT_DESCENDING);

    m_tree->append_column (*change);
  }


  void NoteRecentChanges::update_results()
  {
    // Save the currently selected notes
    Note::List selected_notes = get_selected_notes ();

    int sort_column = 2; /* change date */
    Gtk::SortType sort_type = Gtk::SORT_DESCENDING;
    if (m_store_sort)
      m_store_sort->get_sort_column_id (sort_column, sort_type);

    m_store = Gtk::ListStore::create (m_column_types);

    m_store_filter = Gtk::TreeModelFilter::create (m_store);
    m_store_filter->set_visible_func(sigc::mem_fun(*this, &NoteRecentChanges::filter_notes));
    m_store_sort = Gtk::TreeModelSort::create (m_store_filter);
    m_store_sort->set_sort_func (1 /* title */,
                                sigc::mem_fun(*this, &NoteRecentChanges::compare_titles));
    m_store_sort->set_sort_func (2 /* change date */,
                                sigc::mem_fun(*this, &NoteRecentChanges::compare_dates));

    int cnt = 0;

    for(Note::List::const_iterator note_iter = m_manager.get_notes().begin();
        note_iter != m_manager.get_notes().end(); ++note_iter) {
      const Note::Ptr & note(*note_iter);
      std::string nice_date =
        utils::get_pretty_print_date (note->change_date(), true);

      Gtk::TreeIter iter = m_store->append();
      iter->set_value(0, s_note_icon);  /* icon */
      iter->set_value(1, note->get_title()); /* title */
      iter->set_value(2, nice_date);  /* change date */
      iter->set_value(3, note);      /* note */
      cnt++;
    }

    m_tree->set_model(m_store_sort);

    perform_search ();

    if (sort_column >= 0) {
      // Set the sort column after loading data, since we
      // don't want to resort on every append.
      m_store_sort->set_sort_column_id (sort_column, sort_type);
    }

    // Restore the previous selection
    if (!selected_notes.empty()) {
      select_notes (selected_notes);
    }

  }
  

  void NoteRecentChanges::select_notes(const Note::List & notes)
  {
    Gtk::TreeIter iter = m_store_sort->children().begin();

    if (!iter)
      return;

    do {
      Note::Ptr iter_note = (*iter)[m_column_types.note];
      if (find(notes.begin(), notes.end(), iter_note) != notes.end()) {
        // Found one
        m_tree->get_selection()->select(iter);
        //ScrollToIter (tree, iter);
      }
    } while (++iter);
  }

  void NoteRecentChanges::scroll_to_iter (Gtk::TreeView & tree, const Gtk::TreeIter & iter)
  {
    Gtk::TreePath path = tree.get_model()->get_path(iter);
    if (!path.empty())
      tree.scroll_to_row (path);
  }


  void NoteRecentChanges::perform_search ()
  {
      // For some reason, the matches column must be rebuilt
      // every time because otherwise, it's not sortable.
      remove_matches_column ();
      Search search(m_manager);

      std::string text = get_search_text();
      if (text.empty()) {
        m_current_matches.clear ();
        m_store_filter->refilter ();
        update_total_note_count (m_store_sort->children().size());
        if (m_tree->is_realized()) {
          m_tree->scroll_to_point (0, 0);
        }
        return;
      }
      if (!m_case_sensitive.get_active())
        text = sharp::string_to_lower(text);

      m_current_matches.clear ();
      
      // Search using the currently selected notebook
      notebooks::Notebook::Ptr selected_notebook = get_selected_notebook ();
      if (std::tr1::dynamic_pointer_cast<notebooks::SpecialNotebook>(selected_notebook)) {
        selected_notebook = notebooks::Notebook::Ptr();
      }

      Search::ResultsPtr results =
        search.search_notes(text, m_case_sensitive.get_active(), selected_notebook);
      for(Search::Results::const_iterator iter = results->begin();
          iter != results->end(); ++iter) {
        m_current_matches[iter->first->uri()] = iter->second;
      }
      
      add_matches_column ();
      m_store_filter->refilter ();
      m_tree->scroll_to_point (0, 0);
      update_match_note_count (m_current_matches.size());
  }


  void NoteRecentChanges::add_matches_column ()
  {
    if (!m_matches_column) {
      Gtk::CellRenderer *renderer;

      m_matches_column = manage(new Gtk::TreeViewColumn ());
      m_matches_column->set_title(_("Matches"));
      m_matches_column->set_sizing(Gtk::TREE_VIEW_COLUMN_AUTOSIZE);
      m_matches_column->set_resizable(false);

      renderer = manage(new Gtk::CellRendererText ());
      renderer->property_width() = 75;
      m_matches_column->pack_start (*renderer, false);
      m_matches_column->set_cell_data_func (
        *renderer,
        sigc::mem_fun(*this, &NoteRecentChanges::matches_column_data_func));
      m_matches_column->set_sort_column_id(4);
      m_matches_column->set_sort_indicator(true);
      m_matches_column->set_reorderable(false);
      m_matches_column->set_sort_order(Gtk::SORT_DESCENDING);
      m_matches_column->set_clickable(true);
      m_store_sort->set_sort_func(4 /* matches */,
                                  sigc::mem_fun(*this, &NoteRecentChanges::compare_search_hits));

      m_tree->append_column (*m_matches_column);
      m_store_sort->set_sort_column_id (4, Gtk::SORT_DESCENDING);
    }
  }


  void NoteRecentChanges::remove_matches_column ()
  {
    if (m_matches_column == NULL)
      return;
    
    m_tree->remove_column (*m_matches_column);
    m_matches_column = NULL;

    m_store_sort->set_sort_column_id (2, Gtk::SORT_DESCENDING);
  }


  void NoteRecentChanges::matches_column_data_func(Gtk::CellRenderer * cell, 
                                                   const Gtk::TreeIter & iter)
  {
    Gtk::CellRendererText *crt = dynamic_cast<Gtk::CellRendererText*>(cell);
    if (!crt)
      return;

    std::string match_str = "";

    Note::Ptr note = (*iter)[m_column_types.note];
    if (note) {
      int match_count;
      std::map<std::string, int>::const_iterator miter 
        = m_current_matches.find(note->uri());
      if (miter != m_current_matches.end()) {
        match_count = miter->second;
        if (match_count > 0) {
          const char * fmt;
          fmt = ngettext("%1% match", "%1% matches", match_count);
          match_str = str(boost::format(fmt) % match_count);
        }
      }
    }

    crt->property_text() = match_str;
  }


  void NoteRecentChanges::update_total_note_count (int total)
  {
    try {
      const char * fmt;
      fmt = ngettext("Total: %1% note", "Total: %1% notes", total);
      std::string status = str(boost::format (fmt) % total);
      m_status_bar.pop(0);
      m_status_bar.push(status, 0);
    } 
    catch(const std::exception & e)
    {
      ERR_OUT("exception: %s", e.what());
    }
  }
                
  
  void NoteRecentChanges::update_match_note_count (int matches)
  {
    try {
      const char * fmt;
      fmt = ngettext("Matches: %1% note", "Matches: %1% notes", matches);
      std::string status = str(boost::format (fmt) % matches);
      m_status_bar.pop(0);
      m_status_bar.push(status, 0);
    }
    catch(const std::exception & e)
    {
      ERR_OUT("exception: %s", e.what());
    }
  }
                

  /// <summary>
  /// Filter out notes based on the current search string
  /// and selected tags.  Also prevent template notes from
  /// appearing.
  /// </summary>
  bool NoteRecentChanges::filter_notes(const Gtk::TreeIter & iter)
  {
    Note::Ptr note = (*iter)[m_column_types.note];
    if (!note)
      return false;
                        
    // Don't show the template notes in the list
    Tag::Ptr template_tag = TagManager::obj().get_or_create_system_tag (TagManager::TEMPLATE_NOTE_SYSTEM_TAG);
    if (note->contains_tag (template_tag)) {
      return false;
    }
                        
    notebooks::Notebook::Ptr selected_notebook = get_selected_notebook ();
    if (std::tr1::dynamic_pointer_cast<notebooks::UnfiledNotesNotebook>(selected_notebook)) {
      // If the note belongs to a notebook, return false
      // since the only notes that should be shown in this
      // case are notes that are unfiled (not in a notebook).
      if (notebooks::NotebookManager::instance().get_notebook_from_note (note))
        return false;
    }

    bool passes_search_filter = filter_by_search (note);
    if (passes_search_filter == false)
      return false; // don't waste time checking tags if it's already false

    bool passes_tag_filter = filter_by_tag (note);

    // Must pass both filters to appear in the list
    return passes_tag_filter && passes_search_filter;
    // return true;
  }
    
#if 0 // TODO seems to be unused
  bool NoteRecentChanges::filter_tags(const Gtk::TreeIter & iter)
  {
      Tag t = model.GetValue (iter, 0 /* note */) as Tag;
            if(t.IsProperty || t.IsSystem)
              return false;
            
            return true;

  }
#endif
  // <summary>
  // Return true if the specified note should be shown in the list
  // based on the current selection of tags.  If no tags are selected,
  // all notes should be allowed.
  // </summary>
  bool NoteRecentChanges::filter_by_tag (const Note::Ptr & note)
  {
    if (m_selected_tags.empty())
      return true;

    //   // FIXME: Ugh!  NOT an O(1) operation.  Is there a better way?
    std::list<Tag::Ptr> tags;
    note->get_tags(tags);
    for(std::list<Tag::Ptr>::const_iterator iter = tags.begin();
        iter != tags.end(); ++iter) {
      if(m_selected_tags.find(*iter) != m_selected_tags.end()) {
        return true;
      }
    }

    return false;
  }

  // <summary>
  // Return true if the specified note should be shown in the list
  // based on the search string.  If no search string is specified,
  // all notes should be allowed.
  // </summary>
  bool NoteRecentChanges::filter_by_search(const Note::Ptr & note)
  {
    if (get_search_text().empty())
      return true;

    if (m_current_matches.empty())
      return false;

    return note && (m_current_matches.find(note->uri()) != m_current_matches.end());
  }


  void NoteRecentChanges::on_case_sensitive_toggled()
  {
    perform_search ();
  }

  void NoteRecentChanges::on_notes_changed(const Note::Ptr &)
  {
    update_results ();
  }

  void NoteRecentChanges::on_note_renamed(const Note::Ptr&, const std::string&)
  {
    update_results ();
  }

  void NoteRecentChanges::on_note_saved(const Note::Ptr&)
  {
    update_results ();
  }

  void NoteRecentChanges::on_treeview_drag_data_get(const Glib::RefPtr<Gdk::DragContext> &,
                                                    Gtk::SelectionData &selection_data,
                                                    guint, guint)
  {
    Note::List selected_notes = get_selected_notes ();
    if (selected_notes.empty()) {
      return;
    }
                  
    std::vector<std::string> uris;
    for(Note::List::const_iterator iter = selected_notes.begin();
        iter != selected_notes.end(); ++iter) {

      uris.push_back((*iter)->uri());
    }
    
    selection_data.set_uris(uris);
                  
    if (selected_notes.size() == 1) {
      selection_data.set_text(selected_notes.front()->get_title());
    }
    else {
      selection_data.set_text(_("Notes"));
    }
  }

  void NoteRecentChanges::on_notebook_row_edited(const Glib::ustring& tree_path,
                                                 const Glib::ustring& new_text)
  {
    if (notebooks::NotebookManager::instance().notebook_exists(new_text) ||
        new_text == "") {
      return;
    }
    notebooks::Notebook::Ptr old_notebook = this->get_selected_notebook ();
    if (std::tr1::dynamic_pointer_cast<notebooks::SpecialNotebook>(old_notebook)) {
      return;
    }
    notebooks::Notebook::Ptr new_notebook = notebooks::NotebookManager::instance()
      .get_or_create_notebook (new_text);
    DBG_OUT("Renaming notebook '{%s}' to '{%s}'", old_notebook->get_name().c_str(),
            new_text.c_str());
    std::list<Note *> notes;
    old_notebook->get_tag()->get_notes(notes);
    for(std::list<Note *>::const_iterator note = notes.begin(); note != notes.end(); ++note) {
      notebooks::NotebookManager::instance().move_note_to_notebook (
          (*note)->shared_from_this(), new_notebook);
    }
    notebooks::NotebookManager::instance().delete_notebook (old_notebook);
    Gtk::TreeIter iter;
    if (notebooks::NotebookManager::instance().get_notebook_iter (new_notebook, iter)) {
      m_notebooksTree->get_selection()->select(iter);
    }
  }

  void NoteRecentChanges::on_selection_changed()
  {
    Note::List selected_notes = get_selected_notes ();
    ActionManager &am(ActionManager::obj());

    if (selected_notes.empty()) {
      am ["OpenNoteAction"]->property_sensitive() = false;
      am ["DeleteNoteAction"]->property_sensitive() = false;
    } 
    else if (selected_notes.size() == 1) {
      am ["OpenNoteAction"]->property_sensitive() = true;
      am ["DeleteNoteAction"]->property_sensitive() = true;
    } 
    else {
      // Many notes are selected
      am ["OpenNoteAction"]->property_sensitive() = false;
      am ["DeleteNoteAction"]->property_sensitive() = true;
    }
  }


  bool NoteRecentChanges::on_treeview_button_pressed(GdkEventButton *ev)
  {
    if (ev->window != m_tree->get_bin_window()->gobj()) {
      return false;
    }
                  
    Gtk::TreePath dest_path;
    Gtk::TreeViewColumn *column = NULL;
    int unused;
                  
    m_tree->get_path_at_pos (ev->x, ev->y,
                             dest_path, column, unused, unused);
    // we need to test gobj() isn't NULL.
    // See Gtkmm bug 586437
    // https://bugzilla.mate.org/show_bug.cgi?id=586437
    if (dest_path.empty() || !dest_path.gobj())
      return false;
                  
    m_clickX = ev->x;
    m_clickY = ev->y;
                  
    bool retval = false;

    switch (ev->type) {
    case GDK_2BUTTON_PRESS:
      if (ev->button != 1 || (ev->state &
                             (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK)) != 0) {
        break;
      }
                    
      m_tree->get_selection()->unselect_all ();
      m_tree->get_selection()->select(dest_path);
      // apparently Gtk::TreeView::row_activated() require a dest_path
      // while get_path_at_pos() can return a NULL pointer.
      // See Gtkmm bug 586438
      // https://bugzilla.mate.org/show_bug.cgi?id=586438
      gtk_tree_view_row_activated(m_tree->gobj(), dest_path.gobj(), 
                                  column?column->gobj():NULL);
      break;
    case GDK_BUTTON_PRESS:
      if (ev->button == 3) {
        const Glib::RefPtr<Gtk::TreeSelection> selection
          = m_tree->get_selection();

        if(selection->get_selected_rows().size() <= 1) {
          Gtk::TreeViewColumn * col = 0; // unused
          Gtk::TreePath p;
          int cell_x, cell_y;            // unused
          if (m_tree->get_path_at_pos(ev->x, ev->y, p, col,
                                      cell_x, cell_y)) {
            selection->unselect_all();
            selection->select(p);
          }
        }
        Gtk::Menu *menu = dynamic_cast<Gtk::Menu*>(
          ActionManager::obj().get_widget("/MainWindowContextMenu"));
        popup_context_menu_at_location (menu, ev->x, ev->y);
                      
        // Return true so that the base handler won't
        // run, which causes the selection to change to
        // the row that was right-clicked.
        retval = true;
        break;
      }
                    
      if (m_tree->get_selection()->is_selected(dest_path) 
          && (ev->state & (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK)) == 0) {
        if (column && (ev->button == 1)) {
          Glib::ListHandle<Gtk::CellRenderer *> renderers = column->get_cell_renderers();
          Gtk::CellRenderer *renderer = *renderers.begin();
          Gdk::Rectangle background_area;
          m_tree->get_background_area (dest_path, *column, background_area);
          Gdk::Rectangle cell_area;
          m_tree->get_cell_area (dest_path, *column, cell_area);
                        
          renderer->activate ((GdkEvent*)ev, *m_tree,
                              dest_path.to_string (),
                              background_area, cell_area,
                              Gtk::CELL_RENDERER_SELECTED);
                        
          Gtk::TreeIter iter = m_tree->get_model()->get_iter (dest_path);
          if (iter) {
            m_tree->get_model()->row_changed(dest_path, iter);
          }
        }
                      
        retval = true;
      }
                    
      break;
    default:
      retval = false;
      break;
    }
    return retval;
  }
                
  bool NoteRecentChanges::on_treeview_motion_notify(GdkEventMotion *ev)
  {
    if ((ev->state & Gdk::BUTTON1_MASK) == 0) {
      return false;
    } 
    else if (ev->window != m_tree->get_bin_window()->gobj()) {
      return false;
    }
                  
    bool retval = true;
                  
    if (!m_tree->drag_check_threshold(m_clickX, m_clickY, ev->x, ev->y)) {
      return retval;
    }
                  
    Gtk::TreePath dest_path;
    Gtk::TreeViewColumn * col = NULL; // unused
    int cell_x, cell_y;               // unused
    if (!m_tree->get_path_at_pos (ev->x, ev->y, dest_path, col, cell_x, cell_y)) {
      return retval;
    }
                  
    m_tree->drag_begin (Gtk::TargetList::create (m_targets),
                        Gdk::ACTION_MOVE, 1, (GdkEvent*)ev);
    return retval;
  }

  
  bool NoteRecentChanges::on_treeview_button_released(GdkEventButton *ev)
  {
    if (!m_tree->drag_check_threshold(m_clickX, m_clickY, 
                                      ev->x, ev->y) &&
        ((ev->state & (Gdk::CONTROL_MASK | Gdk::SHIFT_MASK)) == 0) &&
        m_tree->get_selection()->count_selected_rows () > 1) {
                    
      Gtk::TreePath dest_path;
      Gtk::TreeViewColumn * col = NULL; // unused
      int cell_x, cell_y;               // unused
      m_tree->get_path_at_pos (ev->x, ev->y, dest_path, col, cell_x, cell_y);
      m_tree->get_selection()->unselect_all ();
      m_tree->get_selection()->select (dest_path);
    }
    return false;
  }
                
  bool NoteRecentChanges::on_treeview_key_pressed(GdkEventKey * ev)
  {
    switch (ev->keyval) {
    case GDK_Menu:
    {
      // Pop up the context menu if a note is selected
      Note::List selected_notes = get_selected_notes();
      if (!selected_notes.empty()) {
        ActionManager & manager = ActionManager::obj();
        Gtk::Menu * const menu
          = dynamic_cast<Gtk::Menu*>(manager.get_widget(
                                       "/MainWindowContextMenu"));
        popup_context_menu_at_location(menu, 0, 0);
      }
      break;
    }
    default:
      break;
    }

    return false; // Let Escape be handled by the window.
  }

  void NoteRecentChanges::popup_context_menu_at_location(Gtk::Menu *menu, int x, int y)
  {
    menu->show_all ();

    // Set up the funtion to position the context menu
    // if we were called by the keyboard Gdk.Key.Menu.
    if ((x == 0) && (y == 0)) {
      menu->popup (sigc::mem_fun(*this, &NoteRecentChanges::position_context_menu),
                0, gtk_get_current_event_time());
    }
    else {
      menu->popup (0, gtk_get_current_event_time());
    }
  }

  // This is needed for when the user opens
  // the context menu with the keyboard.
  void NoteRecentChanges::position_context_menu(int & x, int & y, bool & push_in)
  {
    // Set default "return" values
    push_in = false; // not used
    x = 0;
    y = 0;

    Gtk::Widget * const focus_widget = this->get_focus();
    if (!focus_widget)
      return;
    focus_widget->get_window()->get_origin(x, y);

    Gtk::TreeView * const tree = dynamic_cast<Gtk::TreeView*>(
                                   focus_widget);
    if (!tree)
      return;
    const Glib::RefPtr<Gdk::Window> tree_area
                                      = tree->get_bin_window();
    if (!tree_area)
      return;
    tree_area->get_origin(x, y);

    const Glib::RefPtr<Gtk::TreeSelection> selection
                                             = tree->get_selection();
    const std::list<Gtk::TreePath> selected_rows
                                     = selection->get_selected_rows();
    if (selected_rows.empty())
      return;

    const Gtk::TreePath & dest_path = selected_rows.front();
    const std::list<Gtk::TreeViewColumn *> columns
                                             = tree->get_columns();
    Gdk::Rectangle cell_rect;
    tree->get_cell_area (dest_path, *columns.front(), cell_rect);

    x += cell_rect.get_x();
    y += cell_rect.get_y();
  }


  Note::List NoteRecentChanges::get_selected_notes()
  {
    Note::List selected_notes;
          
    Glib::ListHandle<Gtk::TreePath, Gtk::TreePath_Traits> selected_rows =
      m_tree->get_selection()->get_selected_rows ();
    for(Glib::ListHandle<Gtk::TreePath, Gtk::TreePath_Traits>::const_iterator iter
          = selected_rows.begin(); iter != selected_rows.end(); ++iter) {
      Note::Ptr note = get_note (*iter);
      if (!note) {
        continue;
      }
            
      selected_notes.push_back(note);
    }
          
    return selected_notes;
  }

  Note::Ptr NoteRecentChanges::get_note(const Gtk::TreePath & p)
  {
    Gtk::TreeIter iter = m_store_sort->get_iter(p); 
    if(iter) {
      return (*iter)[m_column_types.note];
    }
    return Note::Ptr();
  }

  void NoteRecentChanges::on_open_note()
  {
    Note::List selected_notes = get_selected_notes ();
    if (selected_notes.size() != 1)
      return;
                  
    selected_notes.front()->get_window()->present();
  }

  void NoteRecentChanges::on_delete_note()
  {
    Note::List selected_notes = get_selected_notes ();
    if (selected_notes.empty()) {
      return;
    }
                  
    noteutils::show_deletion_dialog(selected_notes, this);
  }



  void NoteRecentChanges::on_close_window()
  {
#if 0
    // Disconnect external signal handlers to prevent bloweup
    manager.NoteDeleted -= OnNotesChanged;
    manager.NoteAdded -= OnNotesChanged;
    manager.NoteRenamed -= OnNoteRenamed;
    manager.NoteSaved -= OnNoteSaved;
                        
    Notebooks.NotebookManager.NoteAddedToNotebook -= OnNoteAddedToNotebook;
    Notebooks.NotebookManager.NoteRemovedFromNotebook -= OnNoteRemovedFromNotebook;
#endif
    // The following code has to be done for the MenuBar to
    // appear properly the next time this window is opened.
    if (m_menubar) {
      m_content_vbox.remove (*m_menubar);
#if 0
      am ["OpenNoteAction"].Activated -= OnOpenNote;
      am ["DeleteNoteAction"].Activated -= OnDeleteNote;
      am ["NewNotebookAction"].Activated -= OnNewNotebook;
      am ["DeleteNotebookAction"].Activated -= OnDeleteNotebook;
      am ["NewNotebookNoteAction"].Activated -= OnNewNotebookNote;
      am ["OpenNotebookTemplateNoteAction"].Activated -= OnOpenNotebookTemplateNote;
      am ["CloseWindowAction"].Activated -= OnCloseWindow;
#endif
    }
                        
    save_position ();
//    Tomboy.ExitingEvent -= OnExitingEvent;

    hide ();
    delete s_instance;
    s_instance = NULL;
    if (Gnote::tray_icon_showing() == false) {
      ActionManager::obj()["QuitGNoteAction"]->activate();
    }
  }


  bool NoteRecentChanges::on_delete(GdkEventAny *)
  {
    on_close_window();
    return true;
  }

  bool NoteRecentChanges::on_key_pressed(GdkEventKey * ev)
  {
    switch (ev->keyval) {
    case GDK_Escape:
      // Allow Escape to close the window
      on_close_window ();
      break;
    default:
      break;
    }
    return true;
  }


  void NoteRecentChanges::on_show()
  {
    // Select "All Notes" in the notebooks list
    select_all_notes_notebook ();
      
    m_find_combo.get_entry()->grab_focus ();
    utils::ForcedPresentWindow::on_show();
  }

  
  int NoteRecentChanges::compare_titles(const Gtk::TreeIter & a, const Gtk::TreeIter & b)
  {
    std::string title_a = (*a)[m_column_types.title];
    std::string title_b = (*b)[m_column_types.title];

    if (title_a.empty() || title_b.empty())
      return -1;

    return title_a.compare(title_b);
  }

  int NoteRecentChanges::compare_dates(const Gtk::TreeIter & a, const Gtk::TreeIter & b)
  {
    Note::Ptr note_a = (*a)[m_column_types.note];
    Note::Ptr note_b = (*b)[m_column_types.note];

    if (!note_a || !note_b) {
      return -1;
    }
    else {
      return sharp::DateTime::compare (note_a->change_date(), note_b->change_date());
    }
  }

  int NoteRecentChanges::compare_search_hits(const Gtk::TreeIter & a, const Gtk::TreeIter & b)
  {
    Note::Ptr note_a = (*a)[m_column_types.note];
    Note::Ptr note_b = (*b)[m_column_types.note];

    if (!note_a || !note_b) {
      return -1;
    }

    int matches_a;
    int matches_b;
    std::map<std::string, int>::iterator iter_a = m_current_matches.find(note_a->uri());
    std::map<std::string, int>::iterator iter_b = m_current_matches.find(note_b->uri());
    bool has_matches_a = (iter_a != m_current_matches.end());
    bool has_matches_b = (iter_b != m_current_matches.end());

    if (!has_matches_a || !has_matches_b) {
      if (has_matches_a) {
        return 1;
      }

      return -1;
    }

    matches_a = iter_a->second;
    matches_b = iter_b->second;
    int result = matches_a - matches_b;
    if (result == 0) {
      // Do a secondary sort by note title in alphabetical order
      result = compare_titles (a, b);

      // Make sure to always sort alphabetically
      if (result != 0) {
        int sort_col_id;
        Gtk::SortType sort_type;
        if (m_store_sort->get_sort_column_id (sort_col_id,
                                              sort_type)) {
          if (sort_type == Gtk::SORT_DESCENDING)
            result = -result; // reverse sign
        }
      }

      return result;
    }

    return result;
  }


  void NoteRecentChanges::on_row_activated(const Gtk::TreePath & p, Gtk::TreeViewColumn*)
  {
    Gtk::TreeIter iter = m_store_sort->get_iter (p);
    if (!iter)
      return;

    Note::Ptr note = (*iter)[m_column_types.note];

    note->get_window()->present ();

    // Tell the new window to highlight the matches and
    // prepopulate the Firefox-style search
    if (!get_search_text().empty()) {
      NoteFindBar & find(note->get_window()->get_find_bar());
      find.show_all ();
      find.property_visible() = true;
      find.set_search_text(get_search_text());
    }
  }

  void NoteRecentChanges::on_entry_activated()
  {
    if (m_entry_changed_timeout) {
      m_entry_changed_timeout->cancel ();
    }

    entry_changed_timeout();
  }

  void NoteRecentChanges::on_entry_changed()
  {
    if (m_entry_changed_timeout == NULL) {
      m_entry_changed_timeout = new utils::InterruptableTimeout ();
      m_entry_changed_timeout->signal_timeout
        .connect(sigc::mem_fun(*this, &NoteRecentChanges::entry_changed_timeout));
    }

    if (get_search_text().empty()) {
      m_clear_search_button.set_sensitive(false);
      perform_search ();
    } 
    else {
      m_entry_changed_timeout->reset (500);
      m_clear_search_button.set_sensitive(true);
    }
  }

  // Called in after .5 seconds of typing inactivity, or on
  // explicit activate.  Redo the search, and update the
  // results...
  void NoteRecentChanges::entry_changed_timeout()
  {
    if (get_search_text().empty())
      return;
    
    perform_search ();
    add_to_previous_searches (get_search_text());
  }


  void NoteRecentChanges::add_to_previous_searches(const std::string & text)
  {
    // Update previous searches, by adding a new term to the
    // list, or shuffling an existing term to the top...
    bool repeat = false;

    if (m_case_sensitive.get_active()) {
      repeat = (find(s_previous_searches.begin(), s_previous_searches.end(), text) != s_previous_searches.end());
    } 
    else {
      std::string lower = sharp::string_to_lower(text);
      for(std::list<std::string>::const_iterator iter = s_previous_searches.begin();
          iter != s_previous_searches.end(); ++iter) {
        if (sharp::string_to_lower(*iter) == lower) {
          repeat = true;
        }
      }
    }

    if (!repeat) {
      s_previous_searches.push_front(text);
      Gtk::TreeIter iter 
        = Glib::RefPtr<Gtk::ListStore>::cast_dynamic(m_find_combo.get_model())->prepend();
      iter->set_value(0, text);
    }
  }

  void NoteRecentChanges::clear_search_clicked()
  {
    m_find_combo.get_entry()->set_text("");
    m_find_combo.get_entry()->grab_focus ();
  }


  void NoteRecentChanges::notebook_pixbuf_cell_data_func(Gtk::CellRenderer * renderer, 
                                                         const Gtk::TreeIter & iter)
  {
    notebooks::Notebook::Ptr notebook;
    iter->get_value(0, notebook);
    if (!notebook)
      return;
      
    Gtk::CellRendererPixbuf *crp = dynamic_cast<Gtk::CellRendererPixbuf*>(renderer);
    if (std::tr1::dynamic_pointer_cast<notebooks::AllNotesNotebook>(notebook)) {
      crp->property_pixbuf() = s_all_notes_icon;
    } 
    else if (std::tr1::dynamic_pointer_cast<notebooks::UnfiledNotesNotebook>(notebook)) {
      crp->property_pixbuf() = s_unfiled_notes_icon;
    } 
    else {
      crp->property_pixbuf() = s_notebook_icon;
    }
  }

  void NoteRecentChanges::notebook_text_cell_data_func(Gtk::CellRenderer * renderer, 
                                                       const Gtk::TreeIter & iter)
  {
    Gtk::CellRendererText *crt = dynamic_cast<Gtk::CellRendererText*>(renderer);
    crt->property_ellipsize() = Pango::ELLIPSIZE_END;
    notebooks::Notebook::Ptr notebook;
    iter->get_value(0, notebook);
    if (!notebook) {
      crt->property_text() = "";
      return;
    }
      
    crt->property_text() = notebook->get_name();

    if (std::tr1::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook)) {
      // Bold the "Special" Notebooks
      crt->property_markup() = str(boost::format("<span weight=\"bold\">%1%</span>") 
                                   % notebook->get_name());
    } 
    else {
      crt->property_text() = notebook->get_name();
    }
  }


  void NoteRecentChanges::on_notebook_selection_changed()
  {
    notebooks::Notebook::Ptr notebook = get_selected_notebook ();
    ActionManager & am(ActionManager::obj());
    if (!notebook) {
      // Clear out the currently selected tags so that no notebook is selected
      m_selected_tags.clear ();

        
      // Select the "All Notes" item without causing
      // this handler to be called again
      m_on_notebook_selection_changed_cid.block();
      select_all_notes_notebook ();
      am["DeleteNotebookAction"]->set_sensitive(false);
      m_on_notebook_selection_changed_cid.unblock();
    } 
    else {
      m_selected_tags.clear ();
      if (notebook->get_tag()) {
        m_selected_tags.insert(notebook->get_tag());
      }
      bool allow_edit = false;
      if (std::tr1::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook)) {
        am["DeleteNotebookAction"]->set_sensitive(false);
      } 
      else {
        am["DeleteNotebookAction"]->set_sensitive(true);
        allow_edit = true;
      }

      Glib::ListHandle<Gtk::CellRenderer*> renderers = m_notebooksTree->get_column(0)->get_cell_renderers();
      for (Glib::ListHandle<Gtk::CellRenderer*>::iterator renderer = renderers.begin();
           renderer != renderers.end(); ++renderer) {
        Gtk::CellRendererText *text_rederer = dynamic_cast<Gtk::CellRendererText*>(*renderer);
        if (text_rederer) {
          text_rederer->property_editable() = allow_edit;
        }
      }
    }
      
    update_results ();
  }
    
  void NoteRecentChanges::on_new_notebook()
  {
    notebooks::NotebookManager::prompt_create_new_notebook (this);
  }
    
  void NoteRecentChanges::on_delete_notebook()
  {
    notebooks::Notebook::Ptr notebook = get_selected_notebook ();
    if (!notebook)
      return;
      
    notebooks::NotebookManager::prompt_delete_notebook (this, notebook);
  }


// Create a new note in the notebook when activated
  void NoteRecentChanges::on_notebook_row_activated(const Gtk::TreePath & , 
                                                    Gtk::TreeViewColumn*)
  {
    on_new_notebook_note();
  }
  
  void NoteRecentChanges::on_new_notebook_note()
  {
    notebooks::Notebook::Ptr notebook = get_selected_notebook ();
    if (!notebook || std::tr1::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook)) {
      // Just create a standard note (not in a notebook)
      ActionManager::obj()["NewNoteAction"]->activate ();
      return;
    }
      
    // Look for the template note and create a new note
    Note::Ptr note = notebook->create_notebook_note ();
    note->get_window()->show ();
  }
    
  void NoteRecentChanges::on_open_notebook_template_note()
  {
    notebooks::Notebook::Ptr notebook = get_selected_notebook ();
    if (!notebook)
      return;
      
    Note::Ptr templateNote = notebook->get_template_note ();
    if (!templateNote)
      return; // something seriously went wrong
      
    templateNote->get_window()->present ();
  }
    
  notebooks::Notebook::Ptr NoteRecentChanges::get_selected_notebook ()
  {
    Gtk::TreeIter iter;
      
    Glib::RefPtr<Gtk::TreeSelection> selection = m_notebooksTree->get_selection();
    if (!selection) {
      return notebooks::Notebook::Ptr();
    }
    iter = selection->get_selected();
    if(!iter) {
      return notebooks::Notebook::Ptr(); // Nothing selected
    }
      
    notebooks::Notebook::Ptr notebook;
    iter->get_value(0, notebook);
    return notebook;
  }
    
  void NoteRecentChanges::select_all_notes_notebook ()
  {
    Glib::RefPtr<Gtk::TreeModel> model = m_notebooksTree->get_model();
    DBG_ASSERT(model, "model is NULL");
    if(!model) {
      return;
    }
    Gtk::TreeIter iter = model->children().begin();
    if (iter) {
      m_notebooksTree->get_selection()->select(iter);
    }
  }


  bool NoteRecentChanges::on_notebooks_tree_button_pressed(GdkEventButton *ev)
  {
    if(ev->button == 3) {
      // third mouse button (right-click)
      Gtk::TreeViewColumn * col = 0; // unused
      Gtk::TreePath p;
      int cell_x, cell_y;            // unused
      const Glib::RefPtr<Gtk::TreeSelection> selection
        = m_notebooksTree->get_selection();

      if (m_notebooksTree->get_path_at_pos(ev->x, ev->y, p, col,
                                           cell_x, cell_y)) {
        selection->select(p);
      }

      notebooks::Notebook::Ptr notebook = get_selected_notebook ();
      if (!notebook)
        return true; // Don't pop open a submenu
          

      bool rowClicked = true;
      if (m_notebooksTree->get_path_at_pos (ev->x, ev->y, p, col, cell_x, cell_y) == false) {
        rowClicked = false;
      }
      if (selection->count_selected_rows () == 0) {
        rowClicked = false;
      }

      Gtk::Menu *menu = NULL;
      if (rowClicked) {
        menu = dynamic_cast<Gtk::Menu*>(ActionManager::obj().get_widget (
                                          "/NotebooksTreeContextMenu"));
      }
      else {
        menu = dynamic_cast<Gtk::Menu*>(ActionManager::obj().get_widget (
                                          "/NotebooksTreeNoRowContextMenu"));
      }
          
      popup_context_menu_at_location (menu,
                                      ev->x, ev->y);
      return true;
    }
    return false;
  }


  bool NoteRecentChanges::on_notebooks_key_pressed(GdkEventKey * ev)
  {
    switch (ev->keyval) {
    case GDK_Menu:
    {
      // Pop up the context menu if a notebook is selected
      notebooks::Notebook::Ptr notebook = get_selected_notebook ();
      if (!notebook || std::tr1::dynamic_pointer_cast<notebooks::SpecialNotebook>(notebook))
        break; // Don't pop open a submenu
          
      Gtk::Menu *menu = dynamic_cast<Gtk::Menu *>(ActionManager::obj().get_widget (
                                                    "/NotebooksTreeContextMenu"));
      popup_context_menu_at_location (menu, 0, 0);

      break;
    }
    default:
      break;
    }

    return false; // Let Escape be handled by the window.
  }
    
  void NoteRecentChanges::on_note_added_to_notebook (const Note & , 
                                                     const notebooks::Notebook::Ptr & )
  {
    update_results ();
  }
    
  void NoteRecentChanges::on_note_removed_to_notebook (const Note & , 
                                                       const notebooks::Notebook::Ptr & )
  {
    update_results ();
  }


  std::string NoteRecentChanges::get_search_text()
  {
    // Entry may be null if search window closes
    // early (bug #544996).
    if (m_find_combo.get_entry() == NULL) {
      return NULL;
    }
    std::string text = m_find_combo.get_entry()->get_text();
    text = sharp::string_trim(text);
    return text;
  }

  void NoteRecentChanges::set_search_text(const std::string & value)
  {
    if (!value.empty()) {
      m_find_combo.get_entry()->set_text(value);
    }
  }
  /// <summary>
  /// Save the position and size of the RecentChanges window
  /// </summary>
  void NoteRecentChanges::save_position ()
  {
    int x;
    int y;
    int width;
    int height;

    get_position(x, y);
    get_size(width, height);
     
    Preferences & prefs(Preferences::obj());
    prefs.set<int> (Preferences::SEARCH_WINDOW_X_POS, x);
    prefs.set<int> (Preferences::SEARCH_WINDOW_Y_POS, y);
    prefs.set<int> (Preferences::SEARCH_WINDOW_WIDTH, width);
    prefs.set<int> (Preferences::SEARCH_WINDOW_HEIGHT, height);
    prefs.set<int> (Preferences::SEARCH_WINDOW_SPLITTER_POS, 
                    m_hpaned.get_position());
  }
        
   
  void NoteRecentChanges::restore_position()
  {
    Preferences & prefs(Preferences::obj());
    int x = prefs.get<int> (Preferences::SEARCH_WINDOW_X_POS);
    int y = prefs.get<int> (Preferences::SEARCH_WINDOW_Y_POS);
    int width = prefs.get<int> (Preferences::SEARCH_WINDOW_WIDTH);
    int height = prefs.get<int> (Preferences::SEARCH_WINDOW_HEIGHT);
    int pos = prefs.get<int> (Preferences::SEARCH_WINDOW_SPLITTER_POS);

    if((width == 0) || (height == 0)) {
      return;
    }
          
    set_default_size(width, height);
    move (x, y);
    if(pos) {
      m_hpaned.set_position(pos);
    }
  }


  bool NoteRecentChanges::on_exiting_event()
  {
    save_position();
    return false;
  }

}

