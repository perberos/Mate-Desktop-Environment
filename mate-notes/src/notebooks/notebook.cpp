/*
 * gnote
 *
 * Copyright (C) 2010-2011 Aurimas Cernius
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



#include <boost/format.hpp>
#include <glibmm/i18n.h>

#include "sharp/string.hpp"
#include "gnote.hpp"
#include "notemanager.hpp"
#include "notebooks/notebook.hpp"
#include "tagmanager.hpp"

namespace gnote {
namespace notebooks {


  const char * Notebook::NOTEBOOK_TAG_PREFIX = "notebook:";

  /// <summary>
  /// Construct a new Notebook with a given name
  /// </summary>
  /// <param name="name">
  /// A <see cref="System.String"/>.  This is the name that will be used
  /// to identify the notebook.
  /// </param>
  Notebook::Notebook(const std::string & name, bool is_special)
  {
    // is special assume the name as is, and we don't want a tag.
    if(is_special) {
      m_name = name;
    }
    else {
      set_name(name);
      m_tag = TagManager::obj().get_or_create_system_tag (
        std::string(NOTEBOOK_TAG_PREFIX) + name);
    }
  }

  /// <summary>
  /// Construct a new Notebook with the specified notebook system tag.
  /// </summary>
  /// <param name="notebookTag">
  /// A <see cref="Tag"/>.  This must be a system notebook tag.
  /// </param>
  Notebook::Notebook(const Tag::Ptr & notebookTag)
  {
  // Parse the notebook name from the tag name
    std::string systemNotebookPrefix = std::string(Tag::SYSTEM_TAG_PREFIX)
      + NOTEBOOK_TAG_PREFIX;
    std::string notebookName = sharp::string_substring(notebookTag->name(), 
                                                       systemNotebookPrefix.length());
    set_name(notebookName);
    m_tag = notebookTag;
  }

  void Notebook::set_name(const std::string & value)
  {
    std::string trimmedName = value;
    if(!trimmedName.empty()) {
      m_name = trimmedName;
      m_normalized_name = sharp::string_to_lower(trimmedName);

      // The templateNoteTite should show the name of the
      // notebook.  For example, if the name of the notebooks
      // "Meetings", the templateNoteTitle should be "Meetings
      // Notebook Template".  Translators should place the
      // name of the notebook accordingly using "%1%".
      std::string format = _("%1% Notebook Template");
      m_default_template_note_title = str(boost::format(format) % m_name);
    }
  }


  std::string Notebook::get_normalized_name() const
  {
    return m_normalized_name;
  }


  Tag::Ptr Notebook::get_tag() const
  { 
    return m_tag; 
  }


  Note::Ptr Notebook::find_template_note() const
  {
    Note::Ptr note;
    Tag::Ptr template_tag = TagManager::obj().get_system_tag (TagManager::TEMPLATE_NOTE_SYSTEM_TAG);
    Tag::Ptr notebook_tag = TagManager::obj().get_system_tag (NOTEBOOK_TAG_PREFIX + get_name());
    if(!template_tag || !notebook_tag) {
      return note;
    }
    std::list<Note*> notes;
    template_tag->get_notes(notes);
    for (std::list<Note*>::iterator iter = notes.begin(); iter != notes.end(); ++iter) {
      if ((*iter)->contains_tag (notebook_tag)) {
        note = (*iter)->shared_from_this();
        break;
      }
    }

    return note;
  }

  Note::Ptr Notebook::get_template_note() const
  {
    NoteManager & noteManager = Gnote::obj().default_note_manager();
    Note::Ptr note = find_template_note();

    if (!note) {
      std::string title = m_default_template_note_title;
      if (noteManager.find(title)) {
        std::list<Note*> tag_notes;
        m_tag->get_notes(tag_notes);
        title = noteManager.get_unique_name (title, tag_notes.size());
      }
      note =
        noteManager.create (title,
                            NoteManager::get_note_template_content (title));
          
      // Select the initial text
      NoteBuffer::Ptr buffer = note->get_buffer();
      buffer->select_note_body();

      // Flag this as a template note
      Tag::Ptr template_tag = TagManager::obj().get_or_create_system_tag (TagManager::TEMPLATE_NOTE_SYSTEM_TAG);
      note->add_tag (template_tag);

      // Add on the notebook system tag so Tomboy
      // will persist the tag/notebook across sessions
      // if no other notes are added to the notebook.
      Tag::Ptr notebook_tag = TagManager::obj().get_or_create_system_tag (NOTEBOOK_TAG_PREFIX + get_name());
      note->add_tag (notebook_tag);
        
      note->queue_save (Note::CONTENT_CHANGED);
    }

    return note;
  }

  Note::Ptr Notebook::create_notebook_note()
  {
    Note::Ptr template_note = find_template_note();
    NoteManager & note_manager = Gnote::obj().default_note_manager();

    Note::Ptr note = note_manager.create();
    if(template_note)
      NoteManager::replace_body_if_differ(note, template_note);

    // Add the notebook tag
    note->add_tag (m_tag);

    return note;
  }

  /// <summary>
  /// Returns true when the specified note exists in the notebook
  /// </summary>
  /// <param name="note">
  /// A <see cref="Note"/>
  /// </param>
  /// <returns>
  /// A <see cref="System.Boolean"/>
  /// </returns>
  bool Notebook::contains_note(const Note::Ptr & note)
  {
    return note->contains_tag (m_tag);
  }

  std::string Notebook::normalize(const std::string & s)
  {
    return sharp::string_to_lower(sharp::string_trim(s));
  }

  Tag::Ptr SpecialNotebook::get_tag() const
  {
    return Tag::Ptr();
  }

  Note::Ptr SpecialNotebook::get_template_note() const
  {
    return Gnote::obj().default_note_manager().get_or_create_template_note();
  }


  AllNotesNotebook::AllNotesNotebook()
    : SpecialNotebook(_("All Notes"))
  {
  }


  std::string AllNotesNotebook::get_normalized_name() const
  {
    return "___NotebookManager___AllNotes__Notebook___";
  }


  UnfiledNotesNotebook::UnfiledNotesNotebook()
    : SpecialNotebook(_("Unfiled Notes"))
  {
  }

  std::string UnfiledNotesNotebook::get_normalized_name() const
  {
    return "___NotebookManager___UnfiledNotes__Notebook___";
  }

}
}
