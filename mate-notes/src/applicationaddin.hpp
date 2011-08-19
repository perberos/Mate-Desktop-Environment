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



#ifndef __APPLICATION_ADDIN_HPP_
#define __APPLICATION_ADDIN_HPP_


#include "abstractaddin.hpp"


namespace gnote {

class ApplicationAddin
  : public AbstractAddin
{
public:
  static const char * IFACE_NAME;

  /// <summary>
  /// Called when Gnote has started up and is nearly 100% initialized.
  /// </summary>
  virtual void initialize () = 0;

  /// <summary>
  /// Called just before Gnote shuts down for good.
  /// </summary>
  virtual void shutdown () = 0;

  /// <summary>
  /// Return true if the addin is initialized
  /// </summary>
  virtual bool initialized () = 0;

};


}

#endif

