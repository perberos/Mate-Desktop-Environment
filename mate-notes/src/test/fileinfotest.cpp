/*
 * gnote
 *
 * Copyright (C) 2011 Debarshi Ray
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

#include <boost/test/minimal.hpp>

#include "sharp/fileinfo.hpp"

int test_main(int /*argc*/, char ** /*argv*/)
{
  {
    sharp::FileInfo file_info("/foo/bar/baz.txt");

    BOOST_CHECK(file_info.get_name() == "baz.txt");
    BOOST_CHECK(file_info.get_extension() == ".txt");
  }

  {
    sharp::FileInfo file_info("/foo/bar/baz.");

    BOOST_CHECK(file_info.get_name() == "baz.");
    BOOST_CHECK(file_info.get_extension() == ".");
  }

  {
    sharp::FileInfo file_info("/foo/bar/baz");

    BOOST_CHECK(file_info.get_name() == "baz");
    BOOST_CHECK(file_info.get_extension() == "");
  }

  {
    sharp::FileInfo file_info("/foo/bar/..");

    BOOST_CHECK(file_info.get_name() == "..");
    BOOST_CHECK(file_info.get_extension() == "");
  }

  {
    sharp::FileInfo file_info("/foo/bar/");

    BOOST_CHECK(file_info.get_name() == "bar");
    BOOST_CHECK(file_info.get_extension() == "");
  }

  return 0;
}
