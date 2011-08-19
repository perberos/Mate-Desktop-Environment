/*
 * gnote
 *
 * Copyright (C) 2009 Debarshi Ray
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

#include "debug.hpp"
#include "gnote.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "noteoftheday.hpp"
#include "noteofthedaypreferences.hpp"
#include "notewindow.hpp"

namespace noteoftheday {

NoteOfTheDayPreferences::NoteOfTheDayPreferences()
  : Gtk::VBox(false, 12)
  , m_open_template_button(_("_Open Today: Template"), true)
  , m_label(_("Change the <span weight=\"bold\">Today: Template</span> "
              "note to customize the text that new Today notes have."))
{
  m_label.set_line_wrap(true);
  m_label.set_use_markup(true);
  pack_start(m_label, true, true, 0);

  m_open_template_button.set_use_underline(true);
  m_open_template_button.signal_clicked().connect(
      sigc::mem_fun(*this,
                    &NoteOfTheDayPreferences::open_template_button_clicked));
  pack_start(m_open_template_button, false, false, 0);

  show_all();
}

NoteOfTheDayPreferences::~NoteOfTheDayPreferences()
{
}

void NoteOfTheDayPreferences::open_template_button_clicked() const
{
  gnote::NoteManager & manager = gnote::Gnote::obj().default_note_manager();
  gnote::Note::Ptr template_note = manager.find(
                                       NoteOfTheDay::s_template_title);

  if (0 == template_note) {
    try {
      template_note = manager.create(
                                NoteOfTheDay::s_template_title,
                                NoteOfTheDay::get_template_content(
                                  NoteOfTheDay::s_template_title));
      template_note->queue_save(gnote::Note::CONTENT_CHANGED);
    }
    catch (const sharp::Exception & e) {
      ERR_OUT("NoteOfTheDay could not create %s: %s",
              NoteOfTheDay::s_template_title.c_str(),
              e.what());
    }
  }

  if (0 != template_note)
    template_note->get_window()->show();
}

}
