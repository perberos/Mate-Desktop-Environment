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



#include <iostream>
#include <boost/test/minimal.hpp>

#include "sharp/xmlconvert.cpp"



int test_main(int /*argc*/, char ** /*argv*/)
{
  std::string format = "%Y-%m-%dT%T.@7f@%z";
  sharp::DateTime d(678901234, 67890);

  std::string date_string = sharp::XmlConvert::to_string(d);
  BOOST_CHECK(date_string == "1991-07-07T15:40:34.067890Z");

  sharp::DateTime d2 = sharp::DateTime::from_iso8601(date_string);
  
  BOOST_CHECK(d == d2);

  sharp::DateTime d3 = sharp::DateTime::from_iso8601("2009-03-24T03:34:35.2914680-04:00");
  BOOST_CHECK(d3.is_valid());

  // check when usec is 0.
  // see http://bugzilla.mate.org/show_bug.cgi?id=581844
  d3.set_usec(0);

  date_string = sharp::XmlConvert::to_string(d3);
  BOOST_CHECK(date_string == "2009-03-24T07:34:35.000000Z");

  return 0;
}

