/*
 * gnote
 *
 * Copyright (C) 2011 Aurimas Cernius
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

#include <glib/gstdio.h>
#include <glibmm.h>
#include <giomm/file.h>

#include "files.hpp"


namespace sharp {


  bool file_exists(const std::string & file)
  {
    return Glib::file_test(file, Glib::FILE_TEST_EXISTS)
           && Glib::file_test(file, Glib::FILE_TEST_IS_REGULAR);
  }


  std::string file_basename(const std::string & p)
  {
    const std::string & filename = Glib::path_get_basename(p);
    const std::string::size_type pos = filename.find_last_of('.');

    return std::string(filename, 0, pos);
  }

  std::string file_dirname(const std::string & p)
  {
    return Glib::path_get_dirname(p);
  }


  std::string file_filename(const std::string & p)
  {
    return Glib::path_get_basename(p);
  }

  void file_delete(const std::string & p)
  {
    g_unlink(p.c_str());
  }


  void file_copy(const std::string & source, const std::string & dest)
  {
    Gio::File::create_for_path(source)->copy(Gio::File::create_for_path(dest), Gio::FILE_COPY_OVERWRITE);
  }

  void file_move(const std::string & from, const std::string & to)
  {
    g_rename(from.c_str(), to.c_str());
  }
}

