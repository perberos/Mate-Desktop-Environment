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




#ifndef __TAG_HPP_
#define __TAG_HPP_

#include <string>
#include <tr1/memory>

namespace gnote {

  class Note;

  class Tag 
  {
  public:
    typedef std::tr1::shared_ptr<Tag> Ptr;
    static const char * SYSTEM_TAG_PREFIX;

    Tag(const std::string & name);
    ~Tag();

    // <summary>
    // Associates the specified note with this tag.
    // </summary>
    void add_note(Note & );
    // <summary>
    // Unassociates the specified note with this tag.
    // </summary>
    void remove_note(const Note & );
    // <summary>
    // The name of the tag.  This is what the user types in as the tag and
    // what's used to show the tag to the user. This includes any 'system:' prefixes
    // </summary>
    const std::string & name() const
      {
        return m_name;
      }
    void set_name(const std::string & );
    // <summary>
    // Use the string returned here to reference the tag in Dictionaries.
    // </summary>
    const std::string & normalized_name() const
      { 
        return m_normalized_name; 
      }
     /// <value>
    /// Is Tag a System Value
    /// </value>
    bool is_system() const
      {
        return m_issystem;
      }
    /// <value>
    /// Is Tag a Property?
    /// </value>
    bool is_property() const
      {
        return m_isproperty;
      }
    // <summary>
    // Returns a list of all the notes that this tag is associated with.
    // These pointer are not meant to be freed. They are OWNED.
    // </summary>
    void get_notes(std::list<Note *> &) const;
    // <summary>
    // Returns the number of notes this is currently tagging.
    // </summary>
    int popularity() const;
/////

  private:
    class NoteMap;
    std::string m_name;
    std::string m_normalized_name;
    bool        m_issystem;
    bool        m_isproperty;
    // <summary>
    // Used to track which notes are currently tagged by this tag.  The
    // dictionary key is the Note.Uri.
    // </summary>
    NoteMap *   m_notes;
  };


}

#endif
