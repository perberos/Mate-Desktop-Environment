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



#ifndef __TOMBOY_IMPORT_ADDIN_HPP_
#define __TOMBOY_IMPORT_ADDIN_HPP_

#include <string>

#include "sharp/dynamicmodule.hpp"
#include "importaddin.hpp"


namespace tomboyimport {


class TomboyImportModule
  : public sharp::DynamicModule
{
public:
  TomboyImportModule();
  virtual const char * id() const;
  virtual const char * name() const;
  virtual const char * description() const;
  virtual const char * authors() const;
  virtual int          category() const;
  virtual const char * version() const;
};


DECLARE_MODULE(TomboyImportModule);

class TomboyImportAddin
  : public gnote::ImportAddin
{
public:

  static TomboyImportAddin * create()
    {
      return new TomboyImportAddin;
    }
  virtual void initialize();
  virtual void shutdown();
  virtual bool want_to_run(gnote::NoteManager & manager);
  virtual bool first_run(gnote::NoteManager & manager);

private:

  std::string m_tomboy_path;
};


}


#endif

