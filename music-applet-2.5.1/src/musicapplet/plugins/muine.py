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

import musicapplet.player
import musicapplet.widgets

import array

import gobject
from gtk import gdk


INFO = {
        "name": "Muine",
        "version": musicapplet.defs.VERSION,
        "icon-name": "muine",
        "author": "Paul Kuliniewicz <paul@kuliniewicz.org>",
        "copyright": "(C) 2007 Paul Kuliniewicz",
        "website": "http://www.kuliniewicz.org/music-applet/",
}


class MuinePlugin (musicapplet.player.DBusPlugin):
    """
    Music Applet plugin to interface with Muine.
    """


    ##################################################################
    #
    # Creation and overall management
    #
    ##################################################################


    def __init__ (self, conf):
        musicapplet.player.DBusPlugin.__init__ (self, conf, "Muine", "muine",
                                                "org.mate.Muine")

        self.__player = None
        self.__handlers = []
        self.__poll_source = None


    ##################################################################
    #
    # Playback control
    #
    ##################################################################


    def toggle_playback (self):
        self.__player.SetPlaying (not self.playing,
                                  reply_handler=lambda: None,
                                  error_handler=self._report_dbus_error)


    def previous (self):
        self.__player.Previous (reply_handler=lambda: None,
                                error_handler=self._report_dbus_error)


    def next (self):
        self.__player.Next (reply_handler=lambda: None,
                            error_handler=self._report_dbus_error)


    ##################################################################
    #
    # Playing
    #
    ##################################################################


    def __set_playing (self, playing):
        self.playing = playing


    ##################################################################
    #
    # Song Data
    #
    ##################################################################


    def __set_song (self, info):
        if len (info) > 0:
            artists = []
            self.album = None

            for line in info.splitlines ():
                [name, value] = line.split (": ", 1)
                if name == "title":
                    self.title = value
                elif name == "artist" or name == "performer":
                    artists.append (value)
                elif name == "album":
                    self.album = value
                elif name == "duration":
                    self.duration = int (value)

            if len (artists) > 0:
                self.artist = ", ".join (artists)
            else:
                self.artist = None

            # Muine crashes if you call GetAlbumCover and there isn't any
            # art available, but the result of saving it to /dev/null will
            # tell us whether it exists.
            #
            # See http://bugzilla.mate.org/show_bug.cgi?id=396624

            self.__player.WriteAlbumCoverToFile ("/dev/null",
                                                 reply_handler=self.__does_art_exist,
                                                 error_handler=self._report_dbus_error)

        else:
            self._set_no_song ()


    ##################################################################
    #
    # Art
    #
    ##################################################################


    def __does_art_exist (self, exists):
        if exists:
            self.__player.GetAlbumCover (reply_handler=self.__set_art,
                                         error_handler=self._report_dbus_error)
        else:
            self.art = None


    def __set_art (self, bytes):
        raw = array.array ('B', [int(b) for b in bytes])
        if len (raw) > 0:
            self.art = musicapplet.widgets.ma_deserialize_pixdata (raw.tostring ())
        else:
            self.art = None


    ##################################################################
    #
    # Elapsed
    #
    ##################################################################


    def __set_elapsed (self, elapsed):
        if self.title is not None:
            self.elapsed = elapsed


    ##################################################################
    #
    # Connection management
    #
    ##################################################################


    def _connect (self):
        if self.__player is not None:
            return

        import dbus
        bus = dbus.SessionBus ()

        proxy = bus.get_object ("org.mate.Muine", "/org/mate/Muine/Player")
        self.__player = dbus.Interface (proxy, "org.mate.Muine.Player")

        self.__handlers = [
            self.__player.connect_to_signal ("StateChanged", self.__set_playing),
            self.__player.connect_to_signal ("SongChanged", self.__set_song),
        ]

        self.__player.GetPlaying (reply_handler=self.__set_playing,
                                  error_handler=self._report_dbus_error)

        self.__player.GetCurrentSong (reply_handler=self.__set_song,
                                      error_handler=self._report_dbus_error)

        self.__poll ()
        self.connected = True
        self.__poll_source = gobject.timeout_add (1000, self.__poll)


    def _disconnect (self):
        if self.__poll_source is not None:
            gobject.source_remove (self.__poll_source)
            self.__poll_source = None

        for handler in self.__handlers:
            handler.remove ()
        self.__handlers = []

        self.__player = None

        self._set_no_song ()
        self.connected = False


    def __poll (self):
        self.__player.GetPosition (reply_handler=self.__set_elapsed,
                                   error_handler=self._report_dbus_error)

        return True


def create_instance (conf):
    return MuinePlugin (conf)
