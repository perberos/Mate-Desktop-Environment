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

#ifndef __STICKYNOTE_IMPORT_NOTE_ADDIN_HPP_
#define __STICKYNOTE_IMPORT_NOTE_ADDIN_HPP_

#include <string>

#include <libxml/tree.h>

#include <gtkmm/imagemenuitem.h>
#include <gtkmm/messagedialog.h>

#include "sharp/dynamicmodule.hpp"
#include "importaddin.hpp"

namespace stickynote {


class StickyNoteImportModule
  : public sharp::DynamicModule
{
public:
  StickyNoteImportModule();
  virtual const char * id() const;
  virtual const char * name() const;
  virtual const char * description() const;
  virtual const char * authors() const;
  virtual int          category() const;
  virtual const char * version() const;
};


DECLARE_MODULE(StickyNoteImportModule);

class StickyNoteImportNoteAddin
  : public gnote::ImportAddin
{
public:

  static StickyNoteImportNoteAddin * create()
    {
      return new StickyNoteImportNoteAddin;
    }

  StickyNoteImportNoteAddin()
    {
      _init_static();
    }
  virtual void initialize();
  virtual void shutdown();

  virtual bool want_to_run(gnote::NoteManager & manager);
  virtual bool first_run(gnote::NoteManager & manager);

private:
  void check_for_first_run(gnote::NoteManager & manager);
  xmlDocPtr get_sticky_xml_doc();
  void import_button_clicked(gnote::NoteManager & manager);
  void show_no_sticky_xml_dialog(const std::string & xml_path);
  void show_results_dialog(int numNotesImported, int numNotesTotal);
  void import_notes(xmlDocPtr xml_doc, bool showResultsDialog, gnote::NoteManager & manager);
  bool create_note_from_sticky(const char * stickyTitle, const char* content,
                               gnote::NoteManager & manager);
  void show_message_dialog(const std::string & title, const std::string & message, 
                           Gtk::MessageType messageType);

  Gtk::ImageMenuItem *m_item;

  static void _init_static();
  static bool s_static_inited;
  static bool s_sticky_file_might_exist;
  static bool s_sticky_file_existence_confirmed;
  static std::string s_sticky_xml_path;
};


}


#endif

