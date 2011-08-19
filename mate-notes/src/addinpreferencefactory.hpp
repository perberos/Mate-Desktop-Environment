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




#ifndef __ADDIN_PREFERENCE_FACTORY_HPP_
#define __ADDIN_PREFERENCE_FACTORY_HPP_

#include <gtkmm/widget.h>

#include "sharp/modulefactory.hpp"


namespace gnote {


/** the base class for the preference dialog factory */
class AddinPreferenceFactoryBase
  : public sharp::IInterface
{
public:
  static const char * IFACE_NAME;
  virtual Gtk::Widget * create_preference_widget() = 0;
};


/** the template version */
template <typename _AddinType>
class AddinPreferenceFactory
  : public AddinPreferenceFactoryBase
{
public:
  static AddinPreferenceFactoryBase * create()
    {
      return new AddinPreferenceFactory<_AddinType>();
    }
  virtual Gtk::Widget * create_preference_widget()
    {
      return Gtk::manage(new _AddinType);
    }
};


};


#endif

