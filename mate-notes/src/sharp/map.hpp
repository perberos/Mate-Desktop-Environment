/*
 * gnote
 *
 * Copyright (C) 2009 Hubert Figuiere
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */



#ifndef __SHARP_MAP_HPP_
#define __SHARP_MAP_HPP_

#include <list>
#include <vector>
#include <map>

namespace sharp {

  /** get all the keys from the map. */
  template <typename _Map>
  void map_get_keys(const _Map & m, std::vector<typename _Map::mapped_type> & l) 
  {
    l.clear();
    for(typename _Map::const_iterator iter = m.begin();
        iter != m.end(); ++iter) {
      l.push_back(iter->first);
    }
  }


  /** get all the mapped elements from the map. */
  template <typename _Map>
  void map_get_values(const _Map & m, std::list<typename _Map::mapped_type> & l) 
  {
    l.clear();
    for(typename _Map::const_iterator iter = m.begin();
        iter != m.end(); ++iter) {
      l.push_back(iter->second);
    }
  }


  /** call operator delete on all the data element. */
  template <typename _Map>
  void map_delete_all_second(const _Map & m)
  {
    for(typename _Map::const_iterator iter = m.begin();
        iter != m.end(); ++iter) {
      delete iter->second;
    }    
  }

}


#endif
