/*
 * gnote
 *
 * Copyright (C) 2010 Aurimas Cernius
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
#include <glibmm/miscutils.h>

#include "sharp/directory.hpp"
#include "sharp/files.hpp"
#include "debug.hpp"
#include "note.hpp"
#include "notemanager.hpp"
#include "tomboyimportaddin.hpp"


using gnote::Note;

namespace tomboyimport {

TomboyImportModule::TomboyImportModule()
{
  ADD_INTERFACE_IMPL(TomboyImportAddin);
}
const char * TomboyImportModule::id() const
{
  return "TomboyImportAddin";
}
const char * TomboyImportModule::name() const
{
  return _("Tomboy Importer");
}
const char * TomboyImportModule::description() const
{
  return _("Import your notes from Tomboy.");
}
const char * TomboyImportModule::authors() const
{
  return _("Hubert Figuiere");
}
int TomboyImportModule::category() const
{
  return gnote::ADDIN_CATEGORY_TOOLS;
}
const char * TomboyImportModule::version() const
{
  return "0.1";
}



void TomboyImportAddin::initialize()
{
  m_tomboy_path =
    Glib::build_filename(Glib::get_home_dir(), ".tomboy");

  m_initialized = true;
}


void TomboyImportAddin::shutdown()
{
  m_initialized = false;
}


bool TomboyImportAddin::want_to_run(gnote::NoteManager & )
{
  return sharp::directory_exists(m_tomboy_path);
}


bool TomboyImportAddin::first_run(gnote::NoteManager & manager)
{
  int to_import = 0;
  int imported = 0;
  bool success;

  DBG_OUT("import path is %s", m_tomboy_path.c_str());

  if(sharp::directory_exists(m_tomboy_path)) {
    std::list<std::string> files;
    sharp::directory_get_files_with_ext(m_tomboy_path, ".note", files);

    for(std::list<std::string>::const_iterator iter = files.begin();
        iter != files.end(); ++iter) {
      const std::string & file_path(*iter);

      to_import++;

      Note::Ptr note = manager.import_note(file_path);

      if(note) {
        DBG_OUT("success");
        success = true;
        imported++;
      }
    }
  }

  return success;
}


}
