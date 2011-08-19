/*
 * gnote
 *
 * Copyright (C) 2010 Debarshi Ray
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



#include <map>

#include <glibmm.h>

#include "sharp/map.hpp"
#include "sharp/string.hpp"
#include "note.hpp"
#include "tag.hpp"

namespace gnote {

  const char * Tag::SYSTEM_TAG_PREFIX = "system:";

  class Tag::NoteMap
    : public std::map<std::string, Note *>
  {
  };

  Tag::Tag(const std::string & _name)
    : m_issystem(false)
    , m_isproperty(false)
    , m_notes(new NoteMap)
  {
    set_name(_name);
  }

  Tag::~Tag()
  {
    delete m_notes;
  }

  void Tag::add_note(Note & note)
  {
    if(m_notes->find(note.uri()) == m_notes->end()) {
      (*m_notes)[note.uri()] = &note;
    }
  }


  void Tag::remove_note(const Note & note)
  {
    NoteMap::iterator iter = m_notes->find(note.uri());
    if(iter != m_notes->end()) {
      m_notes->erase(iter);
    }
  }


  void Tag::set_name(const std::string & value)
  {
    if (!value.empty()) {
      std::string trimmed_name = sharp::string_trim(value);
      if (!trimmed_name.empty()) {
        m_name = trimmed_name;
        m_normalized_name = sharp::string_to_lower(trimmed_name);
        if(Glib::str_has_prefix(m_normalized_name, SYSTEM_TAG_PREFIX)) {
          m_issystem = true;
        }
        std::vector<std::string> splits;
        sharp::string_split(splits, value, ":");
        m_isproperty  = (splits.size() >= 3);
      }
    }
  }


  void Tag::get_notes(std::list<Note *> & l) const
  {
    sharp::map_get_values(*m_notes, l);
  }


  int Tag::popularity() const
  {
    return m_notes->size();
  }

}

