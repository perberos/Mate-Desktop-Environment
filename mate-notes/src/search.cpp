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




#include "sharp/string.hpp"
#include "notemanager.hpp"
#include "search.hpp"
#include "tagmanager.hpp"
#include "utils.hpp"

namespace gnote {


  Search::Search(NoteManager & manager)
    : m_manager(manager)
  {
  }


  Search::ResultsPtr Search::search_notes(const std::string & query, bool case_sensitive, 
                                  const notebooks::Notebook::Ptr & selected_notebook)
  {
    std::vector<std::string> words;
    Search::split_watching_quotes(words, query);

    // Used for matching in the raw note XML
    std::vector<std::string> encoded_words; 
    Search::split_watching_quotes(encoded_words, utils::XmlEncoder::encode (query));
    ResultsPtr temp_matches(new Results);
      
      // Skip over notes that are template notes
    Tag::Ptr template_tag = TagManager::obj().get_or_create_system_tag (TagManager::TEMPLATE_NOTE_SYSTEM_TAG);

    for(Note::List::const_iterator iter = m_manager.get_notes().begin();
        iter != m_manager.get_notes().end(); ++iter) {
      const Note::Ptr & note(*iter);

      // Skip template notes
      if (note->contains_tag (template_tag)) {
        continue;
      }
        
      // Skip notes that are not in the
      // selected notebook
      if (selected_notebook && !selected_notebook->contains_note (note))
        continue;
        
      // Check the note's raw XML for at least one
      // match, to avoid deserializing Buffers
      // unnecessarily.
      if (check_note_has_match (note, encoded_words,
                                case_sensitive)) {
        int match_count =
          find_match_count_in_note (note->text_content(),
                                    words, case_sensitive);
        if (match_count > 0) {
          // TODO: Improve note.GetHashCode()
          temp_matches->insert(std::make_pair(note, match_count));
        }
      }
    }
    return temp_matches;
  }

  bool Search::check_note_has_match(const Note::Ptr & note, 
                                    const std::vector<std::string> & encoded_words,
                                    bool match_case)
  {
    std::string note_text = note->xml_content();
    if (!match_case) {
      note_text = sharp::string_to_lower(note_text);
    }

    for(std::vector<std::string>::const_iterator iter = encoded_words.begin();
        iter != encoded_words.end(); ++iter) {
      if (sharp::string_contains(note_text, *iter) ) {
        continue;
      }
      else {
        return false;
      }
    }

    return true;
  }

  int Search::find_match_count_in_note(std::string note_text, 
                                       const std::vector<std::string> & words,
                                       bool match_case)
  {
    int matches = 0;

    if (!match_case) {
      note_text = sharp::string_to_lower(note_text);
    }
    
    for(std::vector<std::string>::const_iterator iter = words.begin();
        iter != words.end(); ++iter) {

      const std::string & word(*iter);

      int idx = 0;
      bool this_word_found = false;

      if (word.empty())
        continue;

      while (true) {
        idx = sharp::string_index_of(note_text, word, idx);

        if (idx == -1) {
          if (this_word_found) {
            break;
          }
          else {
            return 0;
          }
        }

        this_word_found = true;

        matches++;

        idx += word.length();
      }
    }

    return matches;
  }


}
