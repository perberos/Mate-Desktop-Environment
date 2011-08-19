/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
 * Original C# file
 * (C) 2006 Ryan Lortie <desrt@desrt.ca>
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


// Translated from FixedWidthNoteAddin.cs:
// Add a 'fixed width' item to the font styles menu.

#include <glibmm/i18n.h>

#include "sharp/modulefactory.hpp"
#include "fixedwidthmenuitem.hpp"
#include "fixedwidthnoteaddin.hpp"
#include "fixedwidthtag.hpp"


namespace fixedwidth {

  FixedWidthModule::FixedWidthModule()
  {
    ADD_INTERFACE_IMPL(FixedWidthNoteAddin);
  }

  const char * FixedWidthModule::id() const
  {
    return "FixedWidthAddin";
  }
  const char * FixedWidthModule::name() const
  {
    // this is the name of the plugin.
    return _("Fixed Width");
  }
  const char * FixedWidthModule::description() const
  {
    return _("Adds fixed-width font style.");
  }
  const char * FixedWidthModule::authors() const
  {
    return "";
  }
  int FixedWidthModule::category() const
  {
    return gnote::ADDIN_CATEGORY_FORMATTING;
  }
  const char * FixedWidthModule::version() const
  {
    return "0.2";
  }



  void FixedWidthNoteAddin::initialize ()
  {
    // If a tag of this name already exists, don't install.
    if (!get_note()->get_tag_table()->lookup ("monospace")) {
      m_tag = Glib::RefPtr<Gtk::TextTag>(new FixedWidthTag ());
				get_note()->get_tag_table()->add (m_tag);
			}

  }


  void FixedWidthNoteAddin::shutdown ()
  {
	// Remove the tag only if we installed it.
    if (m_tag) {
      get_note()->get_tag_table()->remove (m_tag);
    }
  }


  void FixedWidthNoteAddin::on_note_opened ()
  {
    // Add here instead of in Initialize to avoid creating unopened
    // notes' windows/buffers.
    add_text_menu_item (new FixedWidthMenuItem (this));
  }



}

