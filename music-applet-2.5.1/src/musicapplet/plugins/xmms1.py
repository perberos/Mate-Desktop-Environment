# Music Applet
# Copyright (C) 2007 Paul Kuliniewicz <paul@kuliniewicz.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02111-1301, USA.

from gettext import gettext as _
import gobject
import gtk
from gtk import glade

import os

import musicapplet.defs
import musicapplet.player


INFO = {
        "name": "XMMS",
        "version": musicapplet.defs.VERSION,
        "icon-name": "xmms",
        "author": "Paul Kuliniewicz <paul@kuliniewicz.org>",
        "copyright": "(C) 2007 Paul Kuliniewicz",
        "website": "http://www.kuliniewicz.org/music-applet/",
}


class XMMSPlugin (musicapplet.player.PluginBase):
    """
    Music Applet plugin to interface with XMMS 1.x.
    """

    def __init__ (self, conf):
        musicapplet.player.PluginBase.__init__ (self, conf, INFO["name"], INFO["icon-name"])

        # Note that this file is named xmms1.py instead of xmms.py, to
        # avoid confusion with the system's xmms module.

        import xmms.control

        self.__poll_source = None

        # Since the session ID is used so frequently, and changes so rarely,
        # save a copy of its value in the class instead of querying MateConf every
        # single time.

        full_key = conf.resolve_plugin_key (self, "session")
        self.__session = conf.client.get_int (full_key)
        conf.client.notify_add (full_key, self.__session_changed_cb)


    def __session_changed_cb (self, conf, id, entry, unused):
        if entry.value is not None:
            self.__session = entry.value.get_int ()


    def start (self):
        if self.__poll_source is None:
            self.__poll_source = gobject.timeout_add (1000, self.__poll)


    def stop (self):
        if self.__poll_source is not None:
            gobject.source_remove (self.__poll_source)
            self.__poll_source = None
            self._set_no_song ()
            self.connected = False


    def toggle_playback (self):
        import xmms.control
        xmms.control.play_pause (self.__session)


    def previous (self):
        import xmms.control
        xmms.control.playlist_prev (self.__session)


    def next (self):
        import xmms.control
        xmms.control.playlist_next (self.__session)


    def create_config_dialog (self):
        xml = glade.XML (os.path.join (musicapplet.defs.PKG_DATA_DIR, "xmms1.glade"))
        dialog = xml.get_widget ("xmms-dialog")

        full_key = self._conf.resolve_plugin_key (self, "command")
        self._conf.bind_string (full_key, xml.get_widget ("command"), "text")

        full_key = self._conf.resolve_plugin_key (self, "session")
        self._conf.bind_int (full_key, xml.get_widget ("session"), "value")

        xml.get_widget ("browse").connect ("clicked", lambda button: self._browse_for_command (dialog, "XMMS"))
        dialog.set_title (_("%s Plugin") % "XMMS")
        dialog.set_default_response (gtk.RESPONSE_CLOSE)
        dialog.connect ("response", lambda dialog, response: dialog.hide ())
        return dialog


    def __poll (self):
        import xmms.control

        # XMMS is pretty spartan in terms of its feature set....

        self.connected = xmms.control.is_running (self.__session)
        if self.connected:
            if xmms.control.is_playing (self.__session):
                self.playing = not xmms.control.is_paused (self.__session)
                index = xmms.control.get_playlist_pos (self.__session)
                title = xmms.control.get_playlist_title (index, self.__session)
                if self.title != title:
                    self.title = title
                self.duration = xmms.control.get_playlist_time (index, self.__session) / 1000
                self.elapsed = xmms.control.get_output_time (self.__session) / 1000
            else:
                self.playing = False
                self.title = None
                self.duration = -1
                self.elapsed = -1
        else:
            self._set_no_song ()

        return True


def create_instance (conf):
    return XMMSPlugin (conf)
