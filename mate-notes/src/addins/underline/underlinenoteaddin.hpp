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


#ifndef __ADDIN_UNDERLINENOTEADDIN_H_
#define __ADDIN_UNDERLINENOTEADDIN_H_

#include <gtkmm/texttag.h>

#include "sharp/dynamicmodule.hpp"
#include "noteaddin.hpp"

namespace underline {

  class UnderlineModule
    : public sharp::DynamicModule
  {
  public:
    UnderlineModule();
    virtual const char * id() const;
    virtual const char * name() const;
    virtual const char * description() const;
    virtual const char * authors() const;
    virtual int          category() const;
    virtual const char * version() const;
  };


  DECLARE_MODULE(UnderlineModule);

  class UnderlineNoteAddin
    : public gnote::NoteAddin
  {
  public:
    static UnderlineNoteAddin* create()
      { 
        return new UnderlineNoteAddin();
      }
    virtual void initialize ();
    virtual void shutdown ();
    virtual void on_note_opened ();
  private:
    Glib::RefPtr<Gtk::TextTag> m_tag;
  };


}


#endif

