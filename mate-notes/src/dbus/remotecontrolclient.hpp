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


#ifndef __GNOTE_REMOTECONTROLCLIENT_HPP_
#define __GNOTE_REMOTECONTROLCLIENT_HPP_

#include "dbus/remotecontrol-client-glue.hpp"

namespace gnote {


class RemoteControlClient
  : public org::mate::Gnote::RemoteControl_proxy
  , public DBus::IntrospectableProxy
  , public DBus::ObjectProxy
{
public:
  RemoteControlClient(DBus::Connection &connection, const char *path, const char *name);

  virtual void NoteAdded(const std::string&) {}
  virtual void NoteDeleted(const std::string&, const std::string&) {}
  virtual void NoteSaved(const std::string&) {}


};

}


#endif
