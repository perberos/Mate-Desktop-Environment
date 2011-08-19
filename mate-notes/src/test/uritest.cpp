/*
 * gnote
 *
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




#include <boost/test/minimal.hpp>

#include "sharp/uri.hpp"

using namespace sharp;

int test_main(int /*argc*/, char ** /*argv*/)
{

  Uri uri1("http://bugzilla.mate.org/");
  
  BOOST_CHECK(!uri1.is_file());
  BOOST_CHECK(uri1.local_path() == "http://bugzilla.mate.org/");
  BOOST_CHECK(uri1.get_host() == "bugzilla.mate.org");

  Uri uri2("https://bugzilla.mate.org/");
  
  BOOST_CHECK(!uri2.is_file());
  BOOST_CHECK(uri2.local_path() == "https://bugzilla.mate.org/");
  BOOST_CHECK(uri2.get_host() == "bugzilla.mate.org");


  Uri uri3("file:///tmp/foo.txt");
  BOOST_CHECK(uri3.is_file());
  BOOST_CHECK(uri3.local_path() == "/tmp/foo.txt");
  BOOST_CHECK(uri3.get_host() == "");  

  return 0;
}

