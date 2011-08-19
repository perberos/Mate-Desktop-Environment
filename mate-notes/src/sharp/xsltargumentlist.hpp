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




#ifndef __SHARP_XSLTARGUMENTLIST_HPP_
#define __SHARP_XSLTARGUMENTLIST_HPP_

#include <list>
#include <string>
#include <utility>

namespace sharp {

  extern bool XsltArgumentListLink_;

/** argument list for %XslTransform */
class XsltArgumentList
{
public:
  typedef std::list<std::pair<std::string,std::string> > container_t;
  typedef container_t::const_iterator const_iterator;

  /** add a string parameter */
  void add_param(const char* name, const char *uri, const std::string &);
  /** add a bool parameter */
  void add_param(const char* name, const char *uri, bool);

  /** returns a param array suitable for libxslt. Caller 
   *  must call free().
   */
  const char ** get_xlst_params() const;

  /** */
  size_t size() const
    { return m_args.size(); }
  const_iterator begin() const
    { return m_args.begin(); }
  const_iterator end() const
    { return m_args.end(); }
private:

  container_t m_args;
};


}


#endif
