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


#include <glibmm.h>
#include <giomm/file.h>
#include <giomm/fileinfo.h>
#include "sharp/fileinfo.hpp"


namespace sharp {


  bool FileInfoLink_ = true;


  FileInfo::FileInfo(const std::string & s)
    : m_path(s)
  {
  }


  std::string FileInfo::get_name() const
  {
    return Glib::path_get_basename(m_path);
  }


  std::string FileInfo::get_extension() const
  {
    const std::string & name = get_name();

    if ("." == name || ".." == name)
      return "";

    const std::string::size_type pos = name.find_last_of('.');
    return (std::string::npos == pos) ? "" : std::string(name, pos);
  }


  DateTime file_modification_time(const std::string &path)
  {
    Glib::RefPtr<Gio::FileInfo> file_info = Gio::File::create_for_path(path)->query_info(
        G_FILE_ATTRIBUTE_TIME_MODIFIED + std::string(",") + G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC);
    if(file_info)
      return DateTime(file_info->modification_time());
    return DateTime();
  }
}
