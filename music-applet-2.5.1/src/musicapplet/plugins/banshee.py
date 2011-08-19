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


INFO = {
        "display-name": "Banshee (pre-1.0)",
        "internal-name": "Banshee",
        "version": musicapplet.defs.VERSION,
        "icon-name": "music-player-banshee",
        "author": "Paul Kuliniewicz <paul@kuliniewicz.org>",
        "copyright": "(C) 2007-2008 Paul Kuliniewicz",
        "website": "http://www.kuliniewicz.org/music-applet/",
}


class BansheePlugin (musicapplet.player.DBusPlugin):
    """
    Music Applet plugin to interface with Banshee (pre-1.0 versions).
    """


    ##################################################################
    #
    # Creation and overall management
    #
    ##################################################################


    def __init__ (self, conf):
        musicapplet.player.DBusPlugin.__init__ (self, conf, "Banshee", "music-player-banshee",
                                                "org.mate.Banshee")

        self.min_rating = 0.0
        self.max_rating = 5.0

        self.__uri = None
        self.__poll_source = None

        self.__player = None


    ##################################################################
    #
    # Playback control
    #
    ##################################################################


    def toggle_playback (self):
        self.__player.TogglePlaying (reply_handler=lambda: None,
                                     error_handler=self._report_dbus_error)


    def previous (self):
        self.__player.Previous (reply_handler=lambda: None,
                                error_handler=self._report_dbus_error)


    def next (self):
        self.__player.Next (reply_handler=lambda: None,
                            error_handler=self._report_dbus_error)


    ##################################################################
    #
    # URI
    #
    ##################################################################


    def __set_uri (self, uri):
        if uri != self.__uri:
            if uri is not None and uri != "":
                self.__uri = uri
                self.__player.GetPlayingTitle (reply_handler=self.__set_title,
                                               error_handler=self._report_dbus_error)

                self.__player.GetPlayingArtist (reply_handler=self.__set_artist,
                                                error_handler=self._report_dbus_error)

                self.__player.GetPlayingAlbum (reply_handler=self.__set_album,
                                               error_handler=self._report_dbus_error)

                self.__player.GetPlayingDuration (reply_handler=self.__set_duration,
                                                  error_handler=self._report_dbus_error)

                self.__player.GetPlayingRating (reply_handler=self.__set_rating,
                                                error_handler=self._report_dbus_error)

                self.__player.GetPlayingCoverUri (reply_handler=self.__set_art,
                                                  error_handler=self._report_dbus_error)
            else:
                self._set_no_song ()


    ##################################################################
    #
    # Title
    #
    ##################################################################


    def __set_title (self, title):
        self.title = title


    ##################################################################
    #
    # Artist
    #
    ##################################################################


    def __set_artist (self, artist):
        self.artist = artist


    ##################################################################
    #
    # Album
    #
    ##################################################################


    def __set_album (self, album):
        self.album = album


    ##################################################################
    #
    # Duration
    #
    ##################################################################


    def __set_duration (self, duration):
        # XXX: Does Banshee still sometimes return a bogus duration?
        self.duration = duration


    ##################################################################
    #
    # Rating
    #
    ##################################################################


    def __set_rating (self, rating):
        self.rating = rating


    def rate_song (self, rating):
        self.__player.SetPlayingRating (rating,
                                        reply_handler=lambda old: self.set_property ("rating", rating),
                                        error_handler=self._report_dbus_error)


    ##################################################################
    #
    # Playing
    #
    ##################################################################


    def __set_playing (self, playing):
        self.playing = (playing == 1)


    ##################################################################
    #
    # Elapsed
    #
    ##################################################################


    def __set_elapsed (self, elapsed):
        self.elapsed = elapsed


    ##################################################################
    #
    # Art
    #
    ##################################################################


    def __set_art (self, art_uri):
        self._load_art (art_uri)


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

        proxy = bus.get_object ("org.mate.Banshee", "/org/mate/Banshee/Player")
        self.__player = dbus.Interface (proxy, "org.mate.Banshee.Core")

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


    def __poll (self):
        self.__player.GetPlayingUri (reply_handler=self.__set_uri,
                                     error_handler=self._report_dbus_error)

        self.__player.GetPlayingStatus (reply_handler=self.__set_playing,
                                        error_handler=self._report_dbus_error)

        self.__player.GetPlayingPosition (reply_handler=self.__set_elapsed,
                                          error_handler=self._report_dbus_error)

        # XXX: Need to watch for http://bugzilla.mate.org/show_bug.cgi?id=344774 ?

        return True


def create_instance (conf):
    return BansheePlugin (conf)
