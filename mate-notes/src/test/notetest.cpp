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



#include <string>
#include <list>

#include <boost/test/minimal.hpp>

#include <libxml/tree.h>

#include "note.hpp"

int test_main(int /*argc*/, char ** /*argv*/)
{
//  std::string markup = "<tags><tag>system:notebook:ggoiiii</tag><tag>system:template</tag></tags>";
  std::string markup = "<tags xmlns=\"http://beatniksoftware.com/tomboy\"><tag>system:notebook:ggoiiii</tag><tag>system:template</tag></tags>";

  xmlDocPtr doc = xmlParseDoc((const xmlChar*)markup.c_str());
  BOOST_CHECK(doc);

  if(doc) {
    std::list<std::string> tags;
    gnote::Note::parse_tags(xmlDocGetRootElement(doc), tags);
    BOOST_CHECK(!tags.empty());

    xmlFreeDoc(doc);
  }

  return 0;
}

