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


#include "sharp/uri.hpp"
#include "debug.hpp"
#include "utils.hpp"
#include "bugzillalink.hpp"
#include "bugzillanoteaddin.hpp"

namespace bugzilla {

#define URI_ATTRIBUTE_NAME "uri"

  Glib::RefPtr<Gdk::Pixbuf> BugzillaLink::s_bug_icon;

  void BugzillaLink::_static_init()
  {
    s_bug_icon = gnote::utils::get_icon("bug", 16);
  }


  BugzillaLink::BugzillaLink()
  {
    _static_init();
  }


  void BugzillaLink::initialize(const std::string & element_name)
  {
    gnote::DynamicNoteTag::initialize(element_name);

    property_underline() = Pango::UNDERLINE_SINGLE;
    property_foreground() = "blue";
    set_can_activate(true);
    set_can_grow(true);
    set_can_spell_check(false);
    set_can_split(false);
  }


  std::string BugzillaLink::get_bug_url() const
  {
    std::string url;
    AttributeMap::const_iterator iter = get_attributes().find(URI_ATTRIBUTE_NAME);
    if(iter != get_attributes().end()) {
      url = iter->second;
    }
    return url;
  }


  void BugzillaLink::set_bug_url(const std::string & value)
  {
    get_attributes()[URI_ATTRIBUTE_NAME] = value;
    make_image();
  }


  void BugzillaLink::make_image()
  {
    sharp::Uri uri(get_bug_url());

    std::string host = uri.get_host();

    std::string imageDir = BugzillaNoteAddin::images_dir();
    std::string imagePath = imageDir + host + ".png";
    Glib::RefPtr<Gdk::Pixbuf> image;
    try {
      image = Gdk::Pixbuf::create_from_file(imagePath);
    }
    catch(...) {
      image = s_bug_icon;
    }
    set_image(image);
  }


  bool BugzillaLink::on_activate(const gnote::NoteEditor & , 
                                 const Gtk::TextIter & , 
                                 const Gtk::TextIter & )
  {
    if(!get_bug_url().empty()) {
      DBG_OUT("Opening url '%s'...", get_bug_url().c_str());
				
      try {
        gnote::utils::open_url(get_bug_url());
      } 
      catch (const Glib::Error & e) {
        gnote::utils::show_opening_location_error(NULL, 
                                                  get_bug_url(), e.what());
      }
    }
    return true;
  }


  void BugzillaLink::on_attribute_read(const std::string & attributeName)
  {
    gnote::DynamicNoteTag::on_attribute_read(attributeName);
    if (attributeName == URI_ATTRIBUTE_NAME) {
      make_image();
    }
  }

}
