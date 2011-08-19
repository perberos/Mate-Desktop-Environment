/*
 * gnote
 *
 * Copyright (C) 2009, 2010 Debarshi Ray
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

#include "sharp/datetime.hpp"
#include "debug.hpp"
#include "notemanager.hpp"
#include "noteoftheday.hpp"
#include "tagmanager.hpp"
#include "utils.hpp"

namespace noteoftheday {

const std::string NoteOfTheDay::s_template_title
                                  = _("Today: Template");
const std::string NoteOfTheDay::s_title_prefix
                                  = _("Today: ");

gnote::Note::Ptr NoteOfTheDay::create(gnote::NoteManager & manager,
                                      const Glib::Date & date)
{
  const std::string title = get_title(date);
  const std::string xml = get_content(date, manager);

  gnote::Note::Ptr notd;
  try {
    notd = manager.create(title, xml);
  }
  catch (const sharp::Exception & e) {
    ERR_OUT("NoteOfTheDay could not create %s: %s",
            title.c_str(),
            e.what());
    return gnote::Note::Ptr();
  }

  // Automatically tag all new Note of the Day notes
  notd->add_tag(gnote::TagManager::obj().get_or_create_system_tag(
                                           "NoteOfTheDay"));

  return notd;
}

void NoteOfTheDay::cleanup_old(gnote::NoteManager & manager)
{
  gnote::Note::List kill_list;
  const gnote::Note::List & notes = manager.get_notes();

  Glib::Date date;
  date.set_time_current(); // time set to 00:00:00

  for (gnote::Note::List::const_iterator iter = notes.begin();
       notes.end() != iter; iter++) {
    const std::string & title = (*iter)->get_title();
    const sharp::DateTime & date_time = (*iter)->create_date();

    if (true == Glib::str_has_prefix(title, s_title_prefix)
        && s_template_title != title
        && Glib::Date(
             date_time.day(),
             static_cast<Glib::Date::Month>(date_time.month()),
             date_time.year()) != date
        && !has_changed(*iter)) {
      kill_list.push_back(*iter);
    }
  }

  for (gnote::Note::List::const_iterator iter = kill_list.begin();
       kill_list.end() != iter; iter++) {
    DBG_OUT("NoteOfTheDay: Deleting old unmodified '%s'",
            (*iter)->get_title().c_str());
    manager.delete_note(*iter);
  }
}

std::string NoteOfTheDay::get_content(
                            const Glib::Date & date,
                            const gnote::NoteManager & manager)
{
  const std::string title = get_title(date);

  // Attempt to load content from template
  const gnote::Note::Ptr template_note = manager.find(
                                                   s_template_title);

  if (0 != template_note) {
    std::string xml_content = template_note->xml_content();
    return xml_content.replace(xml_content.find(s_template_title, 0),
                               s_template_title.length(),
                               title);
  }
  else {
    return get_template_content(title);
  }
}

std::string NoteOfTheDay::get_content_without_title(
                              const std::string & content)
{
  const std::string::size_type nl = content.find("\n");

  if (std::string::npos != nl)
    return std::string(content, nl, std::string::npos);
  else
    return std::string();
}

gnote::Note::Ptr NoteOfTheDay::get_note_by_date(
                                 gnote::NoteManager & manager,
                                 const Glib::Date & date)
{
  const gnote::Note::List & notes = manager.get_notes();

  for (gnote::Note::List::const_iterator iter = notes.begin();
       notes.end() != iter; iter++) {
    const std::string & title = (*iter)->get_title();
    const sharp::DateTime & date_time = (*iter)->create_date();

    if (true == Glib::str_has_prefix(title, s_title_prefix)
        && s_template_title != title
        && Glib::Date(
             date_time.day(),
             static_cast<Glib::Date::Month>(date_time.month()),
             date_time.year()) == date) {
      return *iter;
    }
  }

  return gnote::Note::Ptr();
}

std::string NoteOfTheDay::get_template_content(
                            const std::string & title)
{
  return Glib::ustring::compose(
    "<note-content xmlns:size=\"http://beatniksoftware.com/tomboy/size\">"
    "<note-title>%1</note-title>\n\n\n\n"
    "<size:huge>%2</size:huge>\n\n\n"
    "<size:huge>%3</size:huge>\n\n\n"
    "</note-content>",
    title,
    _("Tasks"),
    _("Appointments"));
}

std::string NoteOfTheDay::get_title(const Glib::Date & date)
{
  // Format: "Today: Friday, July 01 2005"
  return s_title_prefix + date.format_string(_("%A, %B %d %Y"));
}

bool NoteOfTheDay::has_changed(const gnote::Note::Ptr & note)
{
  const sharp::DateTime & date_time = note->create_date();
  const std::string original_xml
    = get_content(Glib::Date(
                    date_time.day(),
                    static_cast<Glib::Date::Month>(date_time.month()),
                    date_time.year()),
                  note->manager());

  return get_content_without_title(note->text_content())
           == get_content_without_title(
                gnote::utils::XmlDecoder::decode(original_xml))
         ? false
         : true;
}

}
