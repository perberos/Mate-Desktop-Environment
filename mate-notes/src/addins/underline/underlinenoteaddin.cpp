/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
 * Copyright (C) 2009 Hubert Figuiere
 * Original C# file
 * (C) 2009 Mark Wakim <markwakim@gmail.com>
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


// Translated from UnderlineNoteAddin.cs:

#include <glibmm/i18n.h>

#include "sharp/modulefactory.hpp"
#include "underlinemenuitem.hpp"
#include "underlinenoteaddin.hpp"
#include "underlinetag.hpp"


namespace underline {

  UnderlineModule::UnderlineModule()
  {
    ADD_INTERFACE_IMPL(UnderlineNoteAddin);
    enabled(false);
  }

  const char * UnderlineModule::id() const
  {
    return "UnderlineAddin";
  }
  const char * UnderlineModule::name() const
  {
    // this is the name of the plugin.
    return _("Underline");
  }
  const char * UnderlineModule::description() const
  {
    return _("Adds ability to underline text.");
  }
  const char * UnderlineModule::authors() const
  {
    return _("Hubert Figuière and the Tomboy Project");
  }
  int UnderlineModule::category() const
  {
    return gnote::ADDIN_CATEGORY_FORMATTING;
  }
  const char * UnderlineModule::version() const
  {
    return "0.1";
  }



  void UnderlineNoteAddin::initialize ()
  {
    // If a tag of this name already exists, don't install.
    if (!get_note()->get_tag_table()->lookup ("underline")) {
      m_tag = Glib::RefPtr<Gtk::TextTag>(new UnderlineTag ());
				get_note()->get_tag_table()->add (m_tag);
			}

  }


  void UnderlineNoteAddin::shutdown ()
  {
	// Remove the tag only if we installed it.
    if (m_tag) {
      get_note()->get_tag_table()->remove (m_tag);
    }
  }


  void UnderlineNoteAddin::on_note_opened ()
  {
    // Add here instead of in Initialize to avoid creating unopened
    // notes' windows/buffers.
    add_text_menu_item (new UnderlineMenuItem (this));
  }



}

