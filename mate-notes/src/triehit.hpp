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



#ifndef __TRIEHIT_HPP_
#define __TRIEHIT_HPP_

#include <list>
#include <string>
#include <tr1/memory>

namespace gnote {

  template<class value_t>
  class TrieHitList
    : public std::list<value_t*>
  {
  public:
    ~TrieHitList()
      {
        typename TrieHitList::iterator iter;
        for(iter = this->begin(); iter != this->end(); ++iter) {
          delete *iter;
        }
      }
  };


  template<class value_t>
  class TrieHit
  {
  public:
    typedef TrieHitList<TrieHit<value_t> > List;
    typedef std::tr1::shared_ptr<List>      ListPtr;
    
    int         start;
    int         end;
    std::string key;
    value_t     value;

    TrieHit(int s, int e, const std::string & k, const value_t & v)
      : start(s), end(e), key(k), value(v)
      {
      }
  };


}

#endif
