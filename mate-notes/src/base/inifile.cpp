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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "base/inifile.hpp"
#include "debug.hpp"

namespace base {

const bool IniFileLink_ = true;


bool IniFile::load()
{
  GError *error = NULL;
  if(!g_key_file_load_from_file(m_keyfile, m_filename.c_str(), 
                                G_KEY_FILE_NONE, &error)) {
    DBG_OUT("can't load keyfile '%s': %s", m_filename.c_str(),
            error->message);
    g_error_free(error);
    return false;
  }
  return true;
}


bool IniFile::save()
{
  bool success = true;
  GError *error = NULL;
  gsize length;
  size_t written;
  gchar * data;

  data = g_key_file_to_data(m_keyfile, &length, &error);
  if(!data) {
    ERR_OUT("couldn't get keyfile data: %s", error->message);
    g_error_free(error);
    return false;
  }

  FILE * f = fopen(m_filename.c_str(), "w");
  if(f) {
    written = fwrite(data, 1, length, f);
    if(written != length) {
      ERR_OUT("short write: %zd of %zd", written, length);
      success = false;
    }
    fclose(f);
  }
  else {
    ERR_OUT("couldn't open file '%s': %s", m_filename.c_str(), strerror(errno));
    success = false;
  }
  g_free(data);

  return success;
}


bool IniFile::get_bool(const char * group, const char * key, bool dflt)
{
  GError *error = NULL;
  gboolean value = g_key_file_get_boolean(m_keyfile, group, key, &error);
  if(error) {
    if(error->code == G_KEY_FILE_ERROR_INVALID_VALUE) {
      DBG_OUT("key %s:%s is not bool", group, key);
    }
    value = dflt;
    g_error_free(error);
  }

  return value;
}


void IniFile::set_bool(const char * group, const char * key, bool value)
{
  g_key_file_set_boolean(m_keyfile, group, key, value);
  m_dirty = true;
}


}
