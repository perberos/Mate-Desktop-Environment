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



#ifndef __SEARCH_HPP_
#define __SEARCH_HPP_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#include "note.hpp"
#include "notebooks/notebook.hpp"

namespace gnote {

  class NoteManager;

class Search 
{
public:
  typedef std::map<Note::Ptr,int> Results;
  typedef std::tr1::shared_ptr<Results> ResultsPtr;

  template<typename T>
  static void split_watching_quotes(std::vector<T> & split,
                                    const T & source);

  Search(NoteManager &);

    
  /// <summary>
  /// Search the notes!
  /// </summary>
  /// <param name="query">
  /// A <see cref="System.String"/>
  /// </param>
  /// <param name="case_sensitive">
  /// A <see cref="System.Boolean"/>
  /// </param>
  /// <param name="selected_notebook">
  /// A <see cref="Notebooks.Notebook"/>.  If this is not
  /// null, only the notes of the specified notebook will
  /// be searched.
  /// </param>
  /// <returns>
  /// A <see cref="IDictionary`2"/>
  /// </returns>  
  ResultsPtr search_notes(const std::string &, bool, 
                          const notebooks::Notebook::Ptr & );
  bool check_note_has_match(const Note::Ptr & note, const std::vector<std::string> & ,
                            bool match_case);
  int find_match_count_in_note(std::string note_text, const std::vector<std::string> &,
                               bool match_case);
private:

  NoteManager &m_manager;
};

template<typename T>
void Search::split_watching_quotes(std::vector<T> & split,
                                   const T & source)
{
  boost::split(split, source, boost::is_any_of("\""));

  std::vector<T> tmp;

  for (typename std::vector<T>::iterator i = split.begin();
       split.end() != i;
       i++) {
    const T & part = *i;
    std::vector<T> parts;
    boost::split(parts, part, boost::is_any_of(" \t\n"));

    for (typename std::vector<T>::const_iterator j = parts.begin();
         parts.end() != j;
         j++) {
      const T & s = *j;
      if (!s.empty())
        tmp.push_back(s);
    }

    i = split.erase(i);
    if (split.end() == i)
      break;
  }

  split.insert(split.end(), tmp.begin(), tmp.end());
}

}

#endif

