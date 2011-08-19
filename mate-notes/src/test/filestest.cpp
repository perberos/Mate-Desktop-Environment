/*
 * gnote
 *
 * Copyright (C) 2011 Debarshi Ray
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

#include <iostream>

#include <boost/test/minimal.hpp>
#include <glibmm.h>

#include "sharp/files.hpp"

using namespace sharp;

int test_main(int /*argc*/, char ** /*argv*/)
{
  {
    std::string path = "/foo/bar/baz.txt";

    BOOST_CHECK(file_basename(path) == "baz");
    BOOST_CHECK(file_dirname(path) == "/foo/bar");
    BOOST_CHECK(file_filename(path) == "baz.txt");
  }

  {
    std::string path = "/foo/bar/baz";

    BOOST_CHECK(file_basename(path) == "baz");
    BOOST_CHECK(file_dirname(path) == "/foo/bar");
    BOOST_CHECK(file_filename(path) == "baz");
  }

  {
    std::string path = "/foo/bar/..";

    BOOST_CHECK(file_basename(path) == ".");
    BOOST_CHECK(file_dirname(path) == "/foo/bar");
    BOOST_CHECK(file_filename(path) == "..");
  }

  {
    std::string path = "/foo/bar/";

    BOOST_CHECK(file_basename(path) == "bar");
    BOOST_CHECK(file_dirname(path) == "/foo/bar");
    BOOST_CHECK(file_filename(path) == "bar");
  }

  {
    std::string dir = Glib::get_current_dir();

    BOOST_CHECK(file_exists(dir) == false);
    // Very unlikely to exist.
    BOOST_CHECK(file_exists(__FILE__ __FILE__) == false);
    BOOST_CHECK(file_exists(__FILE__) == true);
  }

  return 0;
}
