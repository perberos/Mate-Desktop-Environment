# Music Applet
# Copyright (C) 2007-2008 Paul Kuliniewicz <paul@kuliniewicz.org>
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

import re


INFO = {
        "name": "Exaile",
        "version": musicapplet.defs.VERSION,
        "icon-name": "exaile",
        "author": "Paul Kuliniewicz <paul@kuliniewicz.org>",
        "copyright": "(C) 2007-2008 Paul Kuliniewicz",
        "website": "http://www.kuliniewicz.org/music-applet/",

}


class ExailePlugin (musicapplet.player.DBusPlugin):

    WEDGED = 3


    def __init__ (self, conf):
        musicapplet.player.DBusPlugin.__init__ (self, conf,
                                                INFO["name"], INFO["icon-name"],
                                                "org.exaile.DBusInterface")

        self.min_rating = 1.0
        self.max_rating = 8.0

        self.__player = None
        self.__poll_source = None

        self.__last_status = None
        self.__last_metadata = None
        self.__wedge_count = 0

        # Since matches are maximal, part 2 is the song metadata blurb
        self.__status_regex = re.compile ("^status: ([^ ]*) (.*) length: ([\d:]+) position: %\d+ \[([\d:]+)\]$")


    def _connect (self):
        if self.__player is not None:
            return

        import dbus
        bus = dbus.SessionBus ()

        proxy = bus.get_object ("org.exaile.DBusInterface", "/DBusInterfaceObject")
        self.__player = dbus.Interface (proxy, "org.exaile.DBusInterface")

        self.__player.get_version (reply_handler=self.__set_exaile_version,
                                   error_handler=self._report_dbus_error)

        self.__poll ()
        self.connected = True
        self.__poll_source = gobject.timeout_add (1000, self.__poll)


    def _disconnect (self):
        self.__player = None

        if self.__poll_source is not None:
            gobject.source_remove (self.__poll_source)
            self.__poll_source = None

        self._set_no_song ()
        self.connected = False


    def toggle_playback (self):
        self.__player.play_pause (reply_handler=lambda: None,
                                  error_handler=self._report_dbus_error)


    def previous (self):
        self.__player.prev_track (reply_handler=lambda: None,
                                  error_handler=self._report_dbus_error)


    def next (self):
        self.__player.next_track (reply_handler=lambda: None,
                                  error_handler=self._report_dbus_error)


    def rate_song (self, rating):
        self.__player.set_rating (rating,
                                  reply_handler=lambda: self.set_property ("rating", rating),
                                  error_handler=self._report_dbus_error)


    def __poll (self):
        self.__player.query (reply_handler=self.__set_status,
                             error_handler=self._report_dbus_error)
        return True


    def __set_exaile_version (self, version_string):
        """
        Starting in version 0.2.14, Exaile switched from an 8-star
        rating scale to a 5-star scale.  Adapt to whichever scale
        is correct for the version we've connected to.
        """

        version = [int (x) for x in version_string.split (".")]
        if version >= [0, 2, 14]:
            self.max_rating = 5.0
        else:
            self.max_rating = 8.0


    def __set_status (self, query_result):
        # Exaile 0.2.8 (and maybe others) gets "stuck" at the end of a playlist, so
        # try to detect this condition and react accordingly.  But watch out for
        # radio streams, where position isn't actually reported!

        if self.__last_status == query_result and query_result.startswith ("status: playing") and self.duration > 0:
            if self.__wedge_count < self.WEDGED:
                self.__wedge_count = self.__wedge_count + 1
        else:
            self.__last_status = query_result
            self.__wedge_count = 0

        match = self.__status_regex.match (query_result)
        if match is None or self.__wedge_count >= self.WEDGED:
            self._set_no_song ()
        else:
            self.playing = (match.group (1) == "playing")
            self.elapsed = self.__convert_time (match.group (4))
            if self.__last_metadata != match.group (2):
                self.__last_metadata = match.group (2)
                self.duration = self.__convert_time (match.group (3))

                # Only update these when they've changed.  Parsing them out of
                # the query result is unreliable in case strings like "album:"
                # actually appear in the metadata.

                self.__player.get_title (reply_handler=self.__set_title,
                                         error_handler=self._report_dbus_error)

                self.__player.get_artist (reply_handler=self.__set_artist,
                                          error_handler=self._report_dbus_error)

                self.__player.get_album (reply_handler=self.__set_album,
                                         error_handler=self._report_dbus_error)

                self.__player.get_cover_path (reply_handler=self.__set_art,
                                              error_handler=self._report_dbus_error)

                self.__player.get_rating (reply_handler=self.__set_rating,
                                          error_handler=self._report_dbus_error)


    def __set_title (self, title):
        if title != "":
            self.title = title
        else:
            self.title = None


    def __set_artist (self, artist):
        if artist != "":
            self.artist = artist
        else:
            self.artist = None


    def __set_album (self, album):
        if album != "":
            self.album = album
        else:
            self.album = None


    def __set_art (self, path):
        if path.endswith ("/images/nocover.png"):
            self.art = None
        else:
            self._load_art (path)


    def __set_rating (self, rating):
        self.rating = rating


    def __convert_time (self, time_str):
        # Convert an H:MM:SS or MM:SS string into a number of seconds.
        result = 0
        for part in time_str.split (":"):
            result = result * 60 + int (part)
        return result


def create_instance (conf):
    return ExailePlugin (conf)
