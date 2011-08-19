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



#ifndef __IMPORT_ADDIN_HPP__
#define __IMPORT_ADDIN_HPP__

#include "applicationaddin.hpp"

namespace gnote {

class NoteManager;

class ImportAddin 
  : public ApplicationAddin
{
public:
  static const char * IFACE_NAME;

  ImportAddin();

  virtual bool initialized();

  /** Return whether the importer want to run at startup or not 
   */
  virtual bool want_to_run(NoteManager & manager) = 0;
  /** Run import during first run.
   *  @param manager the NoteManager to import into.
   *  @return true in case of success.
   */
  virtual bool first_run(NoteManager & manager) = 0;

protected:
  bool m_initialized;
};


}


#endif
