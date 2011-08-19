# Music Applet
# Copyright (C) 2007-2008 Paul Kuliniewicz <paul@kuliniewicz.org>
#           (C) 2007 Mickael Royer <mickael.royer@gmail.com>
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


INFO = {
        "name": "Quod Libet",
        "version": musicapplet.defs.VERSION,
        "icon-name": "quodlibet",
        "author": "Mickael Royer <mickael.royer@gmail.com>,\n" +
                  "Paul Kuliniewicz <paul@kuliniewicz.org>",
        "copyright": "(C) 2007-2008 Paul Kuliniewicz\n" +
                     "(C) 2007 Mickael Royer",
        "website": "http://www.kuliniewicz.org/music-applet/",
}


class QuodLibetPlugin (musicapplet.player.DBusPlugin):

    def __init__ (self, conf):
        musicapplet.player.DBusPlugin.__init__ (self, conf,
                                                INFO["name"], INFO["icon-name"],
                                                "net.sacredchao.QuodLibet")

        self.min_rating = 0.0
        self.max_rating = 4.0

        self.__player = None
        self.__handlers = []
        self.__poll_source = None


    def _connect (self):
        if self.__player is not None:
            return

        import dbus
        bus = dbus.SessionBus ()

        proxy = bus.get_object ("net.sacredchao.QuodLibet",
                                "/net/sacredchao/QuodLibet")
        self.__player = dbus.Interface (proxy, "net.sacredchao.QuodLibet")

        self.__handlers = [
            self.__player.connect_to_signal ("SongStarted", self.__song_started),
            self.__player.connect_to_signal ("SongEnded", self.__song_ended),
            self.__player.connect_to_signal ("Paused", self.__set_stop),
            self.__player.connect_to_signal ("Unpaused", self.__set_start),
        ]

        self.__player.CurrentSong (reply_handler=self.__set_song,
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
        self.__player.IsPlaying (reply_handler=self.__set_playing,
                                 error_handler=self._report_dbus_error)
        self.__player.GetPosition (reply_handler=self.__set_elapsed,
                                   error_handler=self._report_dbus_error)

        return True


    def toggle_playback (self):
        self.__player.PlayPause (reply_handler=lambda paused: None,
                                 error_handler=self._report_dbus_error)


    def previous (self):
        self.__player.Previous (reply_handler=lambda: None,
                                error_handler=self._report_dbus_error)


    def next (self):
        self.__player.Next (reply_handler=lambda: None,
                            error_handler=self._report_dbus_error)


    def __set_start (self):
        self.playing = True


    def __set_stop (self):
        self.playing = False


    def __set_playing (self, playing):
        self.playing = playing


    def __song_started (self, song):
        self.__set_song (song)


    def __song_ended (self, song, skipped):
        self._set_no_song ()


    def __set_song (self, song):
        if len (song) == 0:
            self._set_no_song ()
        else:
            if "title" in song.keys ():
                self.title = song["title"]

            if "artist" in song.keys ():
                self.artist = song["artist"]

            if "album" in song.keys ():
                self.album = song["album"]
            elif "organization" in song.keys ():
                self.album = song["organization"]   # radio stream name

            if "~#length" in song.keys ():
                self.duration = int (song["~#length"])

            if "~#rating" in song.keys ():
                self.rating = int (float (song["~#rating"]) * 4)


    def __set_elapsed (self, elapsed):
        if self.title is not None:
            self.elapsed = int (elapsed / 1000)
        else:
            self.elapsed = -1


def create_instance (conf):
    return QuodLibetPlugin (conf)
