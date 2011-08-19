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

#ifndef __BASE_INIFILE_HPP_
#define __BASE_INIFILE_HPP_

#include <string>
#include <glib.h>

namespace base {


extern const bool IniFileLink_;

class IniFile
{
public:
  
  IniFile(const std::string & filename)
    : m_dirty(false)
    , m_filename(filename)
    , m_keyfile(g_key_file_new())
    {
    }
  ~IniFile()
    {
      if(m_dirty) {
        save();
      }
      g_key_file_free(m_keyfile);
    }

  bool load();
  bool save();
  bool get_bool(const char * group, const char * key, bool dflt = false);
  void set_bool(const char * group, const char * key, bool value);

private:
  bool         m_dirty;
  std::string  m_filename;
  GKeyFile    *m_keyfile;
};


}

#endif
