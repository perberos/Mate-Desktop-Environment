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

#ifndef __GNOTE_REMOTECONTROL_HPP_
#define __GNOTE_REMOTECONTROL_HPP_

#include <string>
#include <vector>

#include <dbus-c++/dbus.h>

#include "dbus/iremotecontrol.hpp"
#include "note.hpp"


namespace gnote {

class NoteManager;

class RemoteControl
  : public IRemoteControl
  , public DBus::IntrospectableAdaptor
  , public DBus::ObjectAdaptor
{
public:
  RemoteControl(DBus::Connection&, gnote::NoteManager&);
  virtual ~RemoteControl();

  virtual bool AddTagToNote(const std::string& uri, const std::string& tag_name);
  virtual std::string CreateNamedNote(const std::string& linked_title);
  virtual std::string CreateNote();
  virtual bool DeleteNote(const std::string& uri);
  virtual bool DisplayNote(const std::string& uri);
  virtual bool DisplayNoteWithSearch(const std::string& uri, const std::string& search);
  virtual void DisplaySearch();
  virtual void DisplaySearchWithText(const std::string& search_text);
  virtual std::string FindNote(const std::string& linked_title);
  virtual std::string FindStartHereNote();
  virtual std::vector< std::string > GetAllNotesWithTag(const std::string& tag_name);
  virtual int32_t GetNoteChangeDate(const std::string& uri);
  virtual std::string GetNoteCompleteXml(const std::string& uri);
  virtual std::string GetNoteContents(const std::string& uri);
  virtual std::string GetNoteContentsXml(const std::string& uri);
  virtual int32_t GetNoteCreateDate(const std::string& uri);
  virtual std::string GetNoteTitle(const std::string& uri);
  virtual std::vector< std::string > GetTagsForNote(const std::string& uri);
  virtual bool HideNote(const std::string& uri);
  virtual std::vector< std::string > ListAllNotes();
  virtual bool NoteExists(const std::string& uri);
  virtual bool RemoveTagFromNote(const std::string& uri, const std::string& tag_name);
  virtual std::vector< std::string > SearchNotes(const std::string& query, const bool& case_sensitive);
  virtual bool SetNoteCompleteXml(const std::string& uri, const std::string& xml_contents);
  virtual bool SetNoteContents(const std::string& uri, const std::string& text_contents);
  virtual bool SetNoteContentsXml(const std::string& uri, const std::string& xml_contents);
  virtual std::string Version();

private:
  void on_note_added(const Note::Ptr &);
  void on_note_deleted(const Note::Ptr &);
  void on_note_saved(const Note::Ptr &);

  NoteManager & m_manager;
};


}

#endif

