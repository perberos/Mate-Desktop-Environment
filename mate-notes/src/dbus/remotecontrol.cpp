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

#include "config.h"

#include "sharp/map.hpp"
#include "debug.hpp"
#include "notemanager.hpp"
#include "notewindow.hpp"
#include "recentchanges.hpp"
#include "remotecontrolproxy.hpp"
#include "search.hpp"
#include "tag.hpp"
#include "tagmanager.hpp"
#include "dbus/remotecontrol.hpp"

namespace gnote {


  RemoteControl::RemoteControl(DBus::Connection& cnx, NoteManager& manager)
    : DBus::ObjectAdaptor(cnx, RemoteControlProxy::GNOTE_SERVER_PATH)
    , m_manager(manager)
  {
    DBG_OUT("initialized remote control");
    m_manager.signal_note_added.connect(
      sigc::mem_fun(*this, &RemoteControl::on_note_added));
    m_manager.signal_note_deleted.connect(
      sigc::mem_fun(*this, &RemoteControl::on_note_deleted));
    m_manager.signal_note_saved.connect(
      sigc::mem_fun(*this, &RemoteControl::on_note_saved));
  }


  RemoteControl::~RemoteControl()
  {
  }

  bool RemoteControl::AddTagToNote(const std::string& uri, const std::string& tag_name)
  {
    Note::Ptr note = m_manager.find_by_uri (uri);
    if (!note) {
      return false;
    }
    Tag::Ptr tag = TagManager::obj().get_or_create_tag (tag_name);
    note->add_tag (tag);
    return true;
  }


  std::string RemoteControl::CreateNamedNote(const std::string& linked_title)
  {
    Note::Ptr note;

    note = m_manager.find (linked_title);
    if (!note)
      return "";

    try {
      note = m_manager.create (linked_title);
      return note->uri();
    } 
    catch (const std::exception & e) {
      ERR_OUT("create throw: %s", e.what());
    }
    return "";
  }

  std::string RemoteControl::CreateNote()
  {
    try {
      Note::Ptr note = m_manager.create ();
      return note->uri();
    } 
    catch(...)
    {
    }
    return "";
  }

  bool RemoteControl::DeleteNote(const std::string& uri)
  {
    Note::Ptr note;

    note = m_manager.find_by_uri (uri);
    if (!note) {
      return false;
    }

    m_manager.delete_note (note);
    return true;

  }

  bool RemoteControl::DisplayNote(const std::string& uri)
  {
    Note::Ptr note;

    note = m_manager.find_by_uri (uri);
    if (!note) {
      return false;
    }

    note->get_window()->present();
    return true;
  }


  bool RemoteControl::DisplayNoteWithSearch(const std::string& uri, const std::string& search)
  {
    Note::Ptr note;

    note = m_manager.find_by_uri (uri);
    if (!note) {
      return false;
    }

    note->get_window()->present();

    // Pop open the find-bar
    NoteFindBar & findbar = note->get_window()->get_find_bar();
    findbar.show_all ();
    findbar.property_visible() = true;
    findbar.set_search_text(search);

    return true;
  }


  void RemoteControl::DisplaySearch()
  {
    NoteRecentChanges::get_instance(m_manager)->present();
  }


  void RemoteControl::DisplaySearchWithText(const std::string& search_text)
  {
    NoteRecentChanges* recent_changes =
      NoteRecentChanges::get_instance (m_manager);
    if (!recent_changes)
				return;

    recent_changes->set_search_text(search_text);
    recent_changes->present ();
  }


  std::string RemoteControl::FindNote(const std::string& linked_title)
  {
    Note::Ptr note = m_manager.find (linked_title);
    return (!note) ? "" : note->uri();
  }


  std::string RemoteControl::FindStartHereNote()
  {
    Note::Ptr note = m_manager.find_by_uri (m_manager.start_note_uri());
    return (!note) ? "" : note->uri();
  }


  std::vector< std::string > RemoteControl::GetAllNotesWithTag(const std::string& tag_name)
  {
    Tag::Ptr tag = TagManager::obj().get_tag (tag_name);
    if (!tag)
      return std::vector< std::string >();

    std::vector< std::string > tagged_note_uris;
    std::list<Note *> notes;
    tag->get_notes(notes);
    for (std::list<Note *>::const_iterator iter = notes.begin();
         iter != notes.end(); ++iter) {
      tagged_note_uris.push_back((*iter)->uri());
    }
    return tagged_note_uris;
  }


  int32_t RemoteControl::GetNoteChangeDate(const std::string& uri)
  {
    Note::Ptr note;
    note = m_manager.find_by_uri (uri);
    if (!note)
      return -1;
    return note->metadata_change_date().sec();
  }


  std::string RemoteControl::GetNoteCompleteXml(const std::string& uri)
  {
    Note::Ptr note;
    note = m_manager.find_by_uri (uri);
    if (!note)
      return "";
    return note->get_complete_note_xml();

  }


  std::string RemoteControl::GetNoteContents(const std::string& uri)
  {
    Note::Ptr note;
    note = m_manager.find_by_uri (uri);
    if (!note)
      return "";
    return note->text_content();
  }


  std::string RemoteControl::GetNoteContentsXml(const std::string& uri)
  {
    Note::Ptr note;
    note = m_manager.find_by_uri (uri);
    if (!note)
      return "";
    return note->xml_content();
  }


  int32_t RemoteControl::GetNoteCreateDate(const std::string& uri)
  {
    Note::Ptr note;
    note = m_manager.find_by_uri (uri);
    if (!note)
      return -1;
    return note->create_date().sec();
  }


  std::string RemoteControl::GetNoteTitle(const std::string& uri)
  {
    Note::Ptr note;
    note = m_manager.find_by_uri (uri);
    if (!note)
      return "";
    return note->get_title();
  }


  std::vector< std::string > RemoteControl::GetTagsForNote(const std::string& uri)
  {
    Note::Ptr note;
    note = m_manager.find_by_uri (uri);
    if (!note)
      return std::vector< std::string >();

    std::vector< std::string > tags;
    std::list<Tag::Ptr> l;
    note->get_tags(l);
    for (std::list<Tag::Ptr>::const_iterator iter = l.begin();
         iter != l.end(); ++iter) {
      tags.push_back((*iter)->normalized_name());
    }
    return tags;
  }


bool RemoteControl::HideNote(const std::string& uri)
{
  Note::Ptr note;
  note = m_manager.find_by_uri (uri);
  if (!note)
    return false;

  note->get_window()->hide();
  return true;
}


std::vector< std::string > RemoteControl::ListAllNotes()
{
  std::vector< std::string > uris;
  for(Note::List::const_iterator iter = m_manager.get_notes().begin();
      iter != m_manager.get_notes().end(); ++iter) {
    uris.push_back((*iter)->uri());
  }
  return uris;
}


bool RemoteControl::NoteExists(const std::string& uri)
{
  Note::Ptr note = m_manager.find_by_uri (uri);
  return note;
}


bool RemoteControl::RemoveTagFromNote(const std::string& uri, 
                                      const std::string& tag_name)
{
  Note::Ptr note = m_manager.find_by_uri (uri);
  if (!note)
    return false;
  Tag::Ptr tag = TagManager::obj().get_tag (tag_name);
  if (tag) {
    note->remove_tag (tag);
  }
  return true;
}


std::vector< std::string > RemoteControl::SearchNotes(const std::string& query,
                                                      const bool& case_sensitive)
{
  if (query.empty())
    return std::vector< std::string >();

  Search search(m_manager);
  std::vector< std::string > list;
  Search::ResultsPtr results =
    search.search_notes(query, case_sensitive, notebooks::Notebook::Ptr());

  for(Search::Results::const_iterator iter = results->begin();
      iter != results->end(); ++iter) {

    list.push_back(iter->first->uri());
  }

  return list;
}


bool RemoteControl::SetNoteCompleteXml(const std::string& uri, 
                                       const std::string& xml_contents)
{
  Note::Ptr note;
  note = m_manager.find_by_uri(uri);
  if(!note) {
    return false;
  }
    
  note->load_foreign_note_xml(xml_contents, Note::CONTENT_CHANGED);
  return true;
}


bool RemoteControl::SetNoteContents(const std::string& uri, 
                                    const std::string& text_contents)
{
  Note::Ptr note;
  note = m_manager.find_by_uri(uri);
  if(!note) {
    return false;
  }

  note->set_text_content(text_contents);
  return true;
}


bool RemoteControl::SetNoteContentsXml(const std::string& uri, 
                                       const std::string& xml_contents)
{
  Note::Ptr note;
  note = m_manager.find_by_uri(uri);
  if(!note) {
    return false;
  }
  note->set_xml_content(xml_contents);
  return true;
}


std::string RemoteControl::Version()
{
  return VERSION;
}



void RemoteControl::on_note_added(const Note::Ptr & note)
{
  if(note) {
    NoteAdded(note->uri());
  }
}


void RemoteControl::on_note_deleted(const Note::Ptr & note)
{
  if(note) {
    NoteDeleted(note->uri(), note->get_title());
  }
}


void RemoteControl::on_note_saved(const Note::Ptr & note)
{
  if(note) {
    NoteSaved(note->uri());
  }
}


}
