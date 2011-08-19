# Music Applet
# Copyright (C) 2008 Paul Kuliniewicz <paul@kuliniewicz.org>
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

import musicapplet.defs
import musicapplet.player

import gobject

import os


INFO = {
        "display-name": "Banshee",
        "internal-name": "Banshee 1.0",
        "version": musicapplet.defs.VERSION,
        "icon-name": "media-player-banshee",
        "author": "Paul Kuliniewicz <paul@kuliniewicz.org>",
        "copyright": "(C) 2008 Paul Kuliniewicz",
        "website": "http://www.kuliniewicz.org/music-applet/",
}


class Banshee1Plugin (musicapplet.player.DBusPlugin):
    """
    Music Applet plugin to interface with Banshee (version 1.0 and later).
    """

    def __init__ (self, conf):
        musicapplet.player.DBusPlugin.__init__ (self,
                                                conf,
                                                { "display": INFO["display-name"], "internal": INFO["internal-name"] },
                                                INFO["icon-name"],
                                                "org.bansheeproject.Banshee")

        self.__handlers = []
        self.__poll_source = None
        self.__engine = None
        self.__controller = None

    def _connect (self):
        if self.__engine is not None and self.__controller is not None:
            return

        import dbus
        bus = dbus.SessionBus ()

        proxy = bus.get_object ("org.bansheeproject.Banshee", "/org/bansheeproject/Banshee/PlayerEngine")
        self.__engine = dbus.Interface (proxy, "org.bansheeproject.Banshee.PlayerEngine")

        proxy = bus.get_object ("org.bansheeproject.Banshee", "/org/bansheeproject/Banshee/PlaybackController")
        self.__controller = dbus.Interface (proxy, "org.bansheeproject.Banshee.PlaybackController")

        self.__handlers = [
            self.__engine.connect_to_signal ("EventChanged", self.__event_changed),
            self.__engine.connect_to_signal ("StateChanged", self.__state_changed),
        ]

        self.__engine.GetCurrentState (reply_handler=self.__get_current_state,
                                       error_handler=self._report_dbus_error)
        self.__engine.GetCurrentTrack (reply_handler=self.__get_current_track,
                                       error_handler=self._report_dbus_error)

        self.__poll ()
        self.connected = True
        self.__poll_source = gobject.timeout_add (1000, self.__poll)

    def _disconnect (self):
        for handler in self.__handlers:
            handler.remove ()
        self.__handlers = []

        if self.__poll_source is not None:
            gobject.source_remove (self.__poll_source)
            self.__poll_source = None

        self.__engine = None
        self.__controller = None

        self._set_no_song ()
        self.connected = False

    def toggle_playback (self):
        self.__engine.TogglePlaying (reply_handler=lambda: None,
                                     error_handler=self._report_dbus_error)

    def previous (self):
        self.__controller.Previous (True, reply_handler=lambda: None,
                                        error_handler=self._report_dbus_error)

    def next (self):
        self.__controller.Next (True, reply_handler=lambda: None,
                                    error_handler=self._report_dbus_error)

    # TODO: implement this, once Banshee supports it
    #def rate_song (self, rating):
    #    pass

    def __event_changed (self, event, message, bufferingPercent):
        if event == "startofstream" or event == "trackinfoupdated":
            self.__engine.GetCurrentTrack (reply_handler=self.__get_current_track,
                                           error_handler=self._report_dbus_error)

    def __state_changed (self, state):
        self.playing = (state == "playing")
        if state == "idle":
            self._set_no_song ()

    def __get_current_state (self, state):
        self.playing = (state == "playing")
        if state == "idle":
            self._set_no_song ()

    def __get_current_track (self, track):
        self.title = track.get ("name")
        self.artist = track.get ("artist")
        self.album = track.get ("album")
        self.duration = track.get ("length", -1)
        self.rating = track.get ("rating", -1.0)

        # A hack, but Banshee provides no cleaner interface for artwork.
        if self.artist is not None and self.album is not None:
            art_file = os.path.expanduser ("~/.cache/album-art/%s-%s.jpg" % (self.__cache_name (self.artist),
                                                                             self.__cache_name (self.album)))
            self._load_art (art_file)
        else:
            self.art = None

    def __get_position (self, position):
        self.elapsed = position / 1000

    def __poll (self):
        self.__engine.GetPosition (reply_handler=self.__get_position,
                                   error_handler=self._report_dbus_error)
        return True

    def __cache_name (self, name):
        """
        Filter out all non-alphanumeric characters in the name and convert
        it to lowercase.  This is how artists and albums get named in
        Banshee's album art cache directory.
        """

        return "".join ([c for c in name.lower() if c.isalnum()])


def create_instance (conf):
    return Banshee1Plugin (conf)
