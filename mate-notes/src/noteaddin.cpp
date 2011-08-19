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




#include "noteaddin.hpp"
#include "notewindow.hpp"


namespace gnote {

  const char * NoteAddin::IFACE_NAME = "gnote::NoteAddin";

  void NoteAddin::initialize(const Note::Ptr & note)
  {
    m_note = note;
    m_note_opened_cid = m_note->signal_opened().connect(
      sigc::mem_fun(*this, &NoteAddin::on_note_opened_event));
    initialize();
    if(m_note->is_opened()) {
      on_note_opened();
    }
  }


  void NoteAddin::dispose(bool disposing)
  {
    if (disposing) {
      for(std::list<Gtk::MenuItem*>::const_iterator iter = m_tools_menu_items.begin();
          iter != m_tools_menu_items.end(); ++iter) {
        delete *iter;
      }
      for(std::list<Gtk::MenuItem*>::const_iterator iter = m_text_menu_items.begin();
          iter != m_text_menu_items.end(); ++iter) {
        delete *iter;
      }
        
      for(ToolItemMap::const_iterator iter = m_toolbar_items.begin();
               iter != m_toolbar_items.end(); ++iter) {
        delete iter->first;
      }

      shutdown ();
    }
    
    m_note_opened_cid.disconnect();
    m_note = Note::Ptr();
  }

  void NoteAddin::on_note_opened_event(Note & )
  {
    on_note_opened();
    NoteWindow * window = get_window();

    for(std::list<Gtk::MenuItem*>::const_iterator iter = m_tools_menu_items.begin();
        iter != m_tools_menu_items.end(); ++iter) {
      Gtk::Widget *item= *iter;
      if ((item->get_parent() == NULL) ||
          (item->get_parent() != window->plugin_menu()))
        window->plugin_menu()->add (*item);
    }

    for(std::list<Gtk::MenuItem*>::const_iterator iter = m_text_menu_items.begin();
        iter != m_text_menu_items.end(); ++iter) {
      Gtk::Widget *item = *iter;
      if ((item->get_parent() == NULL) ||
          (item->get_parent() != window->text_menu())) {
        window->text_menu()->add (*item);
        window->text_menu()->reorder_child(*(Gtk::MenuItem*)item, 7);
      }
    }
      
    for(ToolItemMap::const_iterator iter = m_toolbar_items.begin();
        iter != m_toolbar_items.end(); ++iter) {
      if ((iter->first->get_parent() == NULL) ||
          (iter->first->get_parent() != window->toolbar())) {
        window->toolbar()->insert (*(iter->first), iter->second);
      }
    }
  }


  void NoteAddin::add_plugin_menu_item (Gtk::MenuItem *item)
  {
    if (is_disposing())
      throw sharp::Exception ("Plugin is disposing already");

    m_tools_menu_items.push_back (item);

    if (m_note->is_opened()) {
      get_window()->plugin_menu()->add (*item);
    }
  }
    
  void NoteAddin::add_tool_item (Gtk::ToolItem *item, int position)
  {
    if (is_disposing())
      throw sharp::Exception ("Add-in is disposing already");
        
    m_toolbar_items [item] = position;
      
    if (m_note->is_opened()) {
      get_window()->toolbar()->insert (*item, position);
    }
  }

  void NoteAddin::add_text_menu_item (Gtk::MenuItem * item)
  {
    if (is_disposing())
      throw sharp::Exception ("Plugin is disposing already");

    m_text_menu_items.push_back(item);

    if (m_note->is_opened()) {
      get_window()->text_menu()->add (*item);
      get_window()->text_menu()->reorder_child (*item, 7);
    }
  }

  
}
