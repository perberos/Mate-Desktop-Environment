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





#include "sharp/string.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

#include <pcrecpp.h>

#include "debug.hpp"

namespace sharp {


  std::string string_replace_first(const std::string & source, const std::string & from,
                             const std::string & with)
  {
    return boost::replace_first_copy(source, from, with);
  }

  std::string string_replace_all(const std::string & source, const std::string & from,
                                 const std::string & with)
  {
    return boost::replace_all_copy(source, from, with);
  }

  std::string string_replace_regex(const std::string & source,
                                   const std::string & regex,
                                   const std::string & with)
  {
    pcrecpp::RE re(regex);
    std::string result = source;
    re.Replace(with, &result);
    return result;
  }
  
  bool string_match_iregex(const std::string & source, const std::string & regex)  
  {
    pcrecpp::RE re(regex, pcrecpp::RE_Options(PCRE_CASELESS|PCRE_UTF8));
    return re.FullMatch(source);
  }

  void string_split(std::vector<std::string> & split, const std::string & source,
                    const char * delimiters)
  {
    boost::split(split, source, boost::is_any_of(delimiters));
  }

  Glib::ustring string_substring(const Glib::ustring & source, int start)
  {
    DBG_ASSERT(start >= 0, "start can't be negative");
    if(source.size() <= (unsigned int)start) {
      return "";
    }
    return Glib::ustring(source, start, std::string::npos);
  }

  Glib::ustring string_substring(const Glib::ustring & source, int start, int len)
  {
    return Glib::ustring(source, start, len);
  }

  std::string string_trim(const std::string & source)
  {
    return boost::trim_copy(source);
  }  

  std::string string_trim(const std::string & source, const char * set_of_char)
  {
    return boost::trim_copy_if(source, boost::is_any_of(set_of_char));
  }

  bool string_contains(const std::string & source, const std::string &search)
  {
    return string_index_of(source, search) != -1;
  }

  int string_last_index_of(const std::string & source, const std::string & search)
  {
    if(search.empty()) {
        // Return last index, unless the source is the empty string, return 0
        return source.empty() ? 0 : source.size() - 1;
    }
    boost::iterator_range<std::string::const_iterator> iter
      = boost::find_last(source, search);
    if(iter.begin() == source.end()) {
      // NOT FOUND
      return -1;
    }
    return iter.begin() - source.begin();
  }


  int string_index_of(const std::string & source, const std::string & search)
  {
    // C# returns index 0 if looking for the empty string
    if(search.empty()) {
      return 0;
    }
    boost::iterator_range<std::string::const_iterator> iter
      = boost::find_first(source, search);
    if(iter.begin() == source.end()) {
      // NOT FOUND
      return -1;
    }
    return iter.begin() - source.begin();
  }


  int string_index_of(const std::string & source, const std::string & search, int start_at)
  {
    std::string source2(source.begin() + start_at, source.end());
    boost::iterator_range<std::string::const_iterator> iter
      = boost::find_first(source2, search);
    // C# returns index 0 if looking for the empty string
    if(search.empty()) {
      // Note: check this after 'find_first' so boost will throw an exception if start_at > source.size()
      return start_at;
    }
    if(iter.begin() == source2.end()) {
      // NOT FOUND
      return -1;
    }
    return iter.begin() - source2.begin() + start_at;
  }


  Glib::ustring string_to_lower(const Glib::ustring & source)
  {
    return source.lowercase();
  }

}
