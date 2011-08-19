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


#include "sharp/xmlreader.hpp"

using namespace sharp;

int test_main(int /*argc*/, char ** /*argv*/)
{
  XmlReader xml;

  std::string content = "<note-content xmlns:link=\"http://beatniksoftware.com/tomboy/link\">"
                  "Start Here\n\n"
                  "<bold>Welcome to Gnote!</bold>\n\n"
                  "Use this \"Start Here\" note to begin organizing "
                  "your ideas and thoughts.\n\n"
                  "You can create new notes to hold your ideas by "
                  "selecting the \"Create New Note\" item from the "
                  "Gnote menu in your MATE Panel. "
                  "Your note will be saved automatically.\n\n"
                  "Then organize the notes you create by linking "
                  "related notes and ideas together!\n\n"
                  "We've created a note called "
                  "<link:internal>Using Links in Gnote</link:internal>. "
                  "Notice how each time we type <link:internal>Using "
                  "Links in Gnote</link:internal> it automatically "
                  "gets underlined?  Click on the link to open the note."
    "</note-content>";

  xml.load_buffer(content);

  BOOST_CHECK(xml.read());
  BOOST_CHECK(xml.get_node_type() == XML_READER_TYPE_ELEMENT);
  BOOST_CHECK(xml.get_name() == "note-content");
  BOOST_CHECK(xml.get_attribute("xmlns:link") == "http://beatniksoftware.com/tomboy/link");

#if 0
  BOOST_CHECK(xml.read_inner_xml() == "Start Here\n\n"
                  "<bold>Welcome to Gnote!</bold>\n\n"
                  "Use this \"Start Here\" note to begin organizing "
                  "your ideas and thoughts.\n\n"
                  "You can create new notes to hold your ideas by "
                  "selecting the \"Create New Note\" item from the "
                  "Gnote Notes menu in your MATE Panel. "
                  "Your note will be saved automatically.\n\n"
                  "Then organize the notes you create by linking "
                  "related notes and ideas together!\n\n"
                  "We've created a note called "
                  "<link:internal>Using Links in Gnote</link:internal>. "
                  "Notice how each time we type <link:internal>Using "
                  "Links in Gnote</link:internal> it automatically "
              "gets underlined?  Click on the link to open the note.");
#endif
  BOOST_CHECK(xml.read());
  BOOST_CHECK(xml.get_node_type()  == XML_READER_TYPE_TEXT);
  
  return 0;
}

