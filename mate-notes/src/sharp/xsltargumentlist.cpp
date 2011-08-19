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


#include <stdlib.h>

#include <boost/format.hpp>

#include "sharp/map.hpp"
#include "sharp/xsltargumentlist.hpp"

namespace sharp {

  bool XsltArgumentListLink_ = true;

void XsltArgumentList::add_param(const char* name, const char * /*uri*/, const std::string & value)
{
  std::string pv = str(boost::format("\"%1%\"") % value);
  m_args.push_back(std::make_pair(name, pv));
}


void XsltArgumentList::add_param(const char* name, const char * /*uri*/, bool value)
{
  m_args.push_back(std::make_pair(name, value?"1":"0"));
}


const char ** XsltArgumentList::get_xlst_params() const
{
  const char **params = NULL;

  params = (const char**)calloc(m_args.size() * 2 + 1, sizeof(char*));
  const_iterator iter(m_args.begin());
  const_iterator e(m_args.end());

  const char **cur = params;
  for( ; iter != e; ++iter) {
    *cur = iter->first.c_str();
    ++cur;
    *cur = iter->second.c_str();
    ++cur;
  }

  return params;
}


}
