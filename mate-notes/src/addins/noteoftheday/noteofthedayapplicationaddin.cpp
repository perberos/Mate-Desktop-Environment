/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
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

#include <glibmm.h>
#include <glibmm/i18n.h>

#include "gnote.hpp"
#include "noteoftheday.hpp"
#include "noteofthedayapplicationaddin.hpp"
#include "noteofthedaypreferencesfactory.hpp"

namespace noteoftheday {

DECLARE_MODULE(NoteOfTheDayModule);

NoteOfTheDayModule::NoteOfTheDayModule()
{
  ADD_INTERFACE_IMPL(NoteOfTheDayApplicationAddin);
  ADD_INTERFACE_IMPL(NoteOfTheDayPreferencesFactory);
  enabled(false);
}
const char * NoteOfTheDayModule::id() const
{
  return "NoteOfTheDayAddin";
}
const char * NoteOfTheDayModule::name() const
{
  return _("Note of the Day");
}
const char * NoteOfTheDayModule::description() const
{
  return _("Automatically creates a \"Today\" note for easily "
           "jotting down daily thoughts");
}
const char * NoteOfTheDayModule::authors() const
{
  return _("Debarshi Ray and the Tomboy Project");
}
int NoteOfTheDayModule::category() const
{
  return gnote::ADDIN_CATEGORY_TOOLS;
}
const char * NoteOfTheDayModule::version() const
{
  return "0.1";
}

const char * NoteOfTheDayApplicationAddin::IFACE_NAME
               = "gnote::ApplicationAddin";

NoteOfTheDayApplicationAddin::NoteOfTheDayApplicationAddin()
  : ApplicationAddin()
  , m_initialized(false)
  , m_manager(0)
  , m_timeout()
{
}

NoteOfTheDayApplicationAddin::~NoteOfTheDayApplicationAddin()
{
}

void NoteOfTheDayApplicationAddin::check_new_day() const
{
  Glib::Date date;
  date.set_time_current();

  if (0 == NoteOfTheDay::get_note_by_date(*m_manager, date)) {
    NoteOfTheDay::cleanup_old(*m_manager);

    // Create a new NotD if the day has changed
    NoteOfTheDay::create(*m_manager, date);
  }
}

void NoteOfTheDayApplicationAddin::initialize()
{
  if (!m_timeout) {
    m_timeout
      = Glib::signal_timeout().connect_seconds(
          sigc::bind_return(
            sigc::mem_fun(
              *this,
              &NoteOfTheDayApplicationAddin::check_new_day),
            true),
          60,
          Glib::PRIORITY_DEFAULT);
  }

  Glib::signal_idle().connect_once(
    sigc::mem_fun(*this,
                  &NoteOfTheDayApplicationAddin::check_new_day),
    Glib::PRIORITY_DEFAULT);

  m_initialized = true;
  m_manager = &gnote::Gnote::obj().default_note_manager();
}

void NoteOfTheDayApplicationAddin::shutdown()
{
  if (m_timeout)
    m_timeout.disconnect();

  m_initialized = false;
  m_manager = 0;
}

bool NoteOfTheDayApplicationAddin::initialized()
{
  return m_initialized;
}

}
