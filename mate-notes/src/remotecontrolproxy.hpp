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


#ifndef _REMOTECONTROL_PROXY_HPP_
#define _REMOTECONTROL_PROXY_HPP_


namespace gnote {

class RemoteControl;
class RemoteControlClient;
class NoteManager;

class RemoteControlProxy 
{
public:
  static const char *GNOTE_SERVER_PATH;
  static const char *GNOTE_SERVER_NAME;

  /** Get a dbus client */
  static RemoteControlClient *get_instance();

  static RemoteControl *register_remote(NoteManager & manager);
};

}

#endif
