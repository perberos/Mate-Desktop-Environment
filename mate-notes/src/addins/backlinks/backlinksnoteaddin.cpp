/*
 * gnote
 *
 * Copyright (C) 2010-2011 Aurimas Cernius
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

#include <glibmm/i18n.h>

#include <gtkmm/stock.h>

#include "sharp/string.hpp"
#include "backlinksnoteaddin.hpp"
#include "backlinkmenuitem.hpp"
#include "notemanager.hpp"
#include "utils.hpp"

namespace backlinks {

BacklinksModule::BacklinksModule()
{
  ADD_INTERFACE_IMPL(BacklinksNoteAddin);
}
const char * BacklinksModule::id() const
{
  return "BacklinksAddin";
}
const char * BacklinksModule::name() const
{
  return _("Backlinks");
}
const char * BacklinksModule::description() const
{
  return _("See which notes link to the one you're currently viewing.");
}
const char * BacklinksModule::authors() const
{
  return _("Hubert Figuiere and Tomboy Project");
}
int BacklinksModule::category() const
{
  return gnote::ADDIN_CATEGORY_TOOLS;
}
const char * BacklinksModule::version() const
{
  return "0.1";
}



BacklinksNoteAddin::BacklinksNoteAddin()
  : m_menu_item(NULL)
  , m_menu(NULL)
  , m_submenu_built(false)
{
}


void BacklinksNoteAddin::initialize ()
{
}


void BacklinksNoteAddin::shutdown ()
{
}


void BacklinksNoteAddin::on_note_opened ()
{
	m_menu = manage(new Gtk::Menu());
  m_menu->signal_hide().connect(
    sigc::mem_fun(*this, &BacklinksNoteAddin::on_menu_hidden));
  m_menu->show_all ();
  m_menu_item = manage(new Gtk::ImageMenuItem (
                       _("What links here?")));
  m_menu_item->set_image(*manage(new Gtk::Image(Gtk::Stock::JUMP_TO, 
                                                Gtk::ICON_SIZE_MENU)));
  m_menu_item->set_submenu(*m_menu);
  m_menu_item->signal_activate().connect(
    sigc::mem_fun(*this, &BacklinksNoteAddin::on_menu_item_activated));
  m_menu_item->show ();
  add_plugin_menu_item (m_menu_item);
}

void BacklinksNoteAddin::on_menu_item_activated()
{
  if(m_submenu_built) {
    return;
  }
  update_menu();
}


void BacklinksNoteAddin::on_menu_hidden()
{
	// FIXME: Figure out how to have this function be called only when
  // the whole Tools menu is collapsed so that if a user keeps
  // toggling over the "What links here?" menu item, it doesn't
  // keep forcing the submenu to rebuild.

  // Force the submenu to rebuild next time it's supposed to show
  m_submenu_built = false;
}


void BacklinksNoteAddin::update_menu()
{
  //
  // Clear out the old list
  //
  Gtk::MenuShell::MenuList menu_items = m_menu->items();
  for(Gtk::MenuShell::MenuList::reverse_iterator iter = menu_items.rbegin();
      iter != menu_items.rend(); ++iter) {
    m_menu->remove(*iter);
  }

  //
  // Build a new list
  //
  std::list<BacklinkMenuItem*> items;
  get_backlink_menu_items(items);
  for(std::list<BacklinkMenuItem*>::iterator iter = items.begin();
      iter != items.end(); ++iter) {
    BacklinkMenuItem * item(*iter);
    item->show_all();
    m_menu->append (*item);
  }

  // If nothing was found, add in a "dummy" item
  if (m_menu->items().size() == 0) {
    Gtk::MenuItem *blank_item = manage(new Gtk::MenuItem(_("(none)")));
    blank_item->set_sensitive(false);
    blank_item->show_all ();
    m_menu->append (*blank_item);
  }

  m_submenu_built = true;
}


void BacklinksNoteAddin::get_backlink_menu_items(std::list<BacklinkMenuItem*> & items)
{
  std::string search_title = get_note()->get_title();
  std::string encoded_title = sharp::string_trim(
      gnote::utils::XmlEncoder::encode(sharp::string_to_lower(search_title)));

  // Go through each note looking for
  // notes that link to this one.
  const gnote::Note::List & list = get_note()->manager().get_notes();
  for(gnote::Note::List::const_iterator iter = list.begin();
      iter != list.end(); ++iter) {
    const gnote::Note::Ptr & note(*iter);
    if (note != get_note() // don't match ourself
        && check_note_has_match (note, encoded_title)) {
      BacklinkMenuItem *item = manage(new BacklinkMenuItem (note, search_title));

      items.push_back(item);
    }
  }

  items.sort();
}


bool BacklinksNoteAddin::check_note_has_match(const gnote::Note::Ptr & note, 
                                              const std::string & encoded_title)
{
  std::string note_text = sharp::string_to_lower(note->xml_content());
  if (sharp::string_index_of(note_text, encoded_title) < 0)
    return false;
  
  return true;
}



}
