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



#ifndef __SHARP_STRING_HPP_
#define __SHARP_STRING_HPP_

#include <string>
#include <vector>

#include <glibmm/ustring.h>

namespace sharp {

  /**
   * replace the first instance of %from with %with 
   * in string %source and return the result
   */
  std::string string_replace_first(const std::string & source, const std::string & from,
                             const std::string & with);

  /**
   * replace all instances of %from with %with 
   * in string %source and return the result
   */
  std::string string_replace_all(const std::string & source, const std::string & from,
                                 const std::string & with);
  /** 
   * regex replace in %source with matching %regex with %with
   * and return a copy */
  std::string string_replace_regex(const std::string & source, const std::string & regex,
                                   const std::string & with);
  bool string_match_iregex(const std::string & source, const std::string & regex);

  void string_split(std::vector<std::string> & split, const std::string & source,
                    const char * delimiters);

  /** copy the substring for %source, starting at %start until the end */
  Glib::ustring string_substring(const Glib::ustring & source, int start);
  /** copy the substring for %source, starting at %start and running for %len */
  Glib::ustring string_substring(const Glib::ustring & source, int start, int len);

  std::string string_trim(const std::string & source);
  std::string string_trim(const std::string & source, const char * set_of_char);

  bool string_contains(const std::string & source, const std::string &);
  int string_index_of(const std::string & source, const std::string & with);
  int string_index_of(const std::string & source, const std::string & with, int);
  int string_last_index_of(const std::string & source, const std::string & with);

  Glib::ustring string_to_lower(const Glib::ustring & source);
}



#endif
