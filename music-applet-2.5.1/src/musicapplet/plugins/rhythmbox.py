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

import gobject

import musicapplet.defs
import musicapplet.player


INFO = {
        "name": "Rhythmbox",
        "version": musicapplet.defs.VERSION,
        "icon-name": "rhythmbox",
        "author": "Paul Kuliniewicz <paul@kuliniewicz.org>",
        "copyright": "(C) 2007-2008 Paul Kuliniewicz",
        "website": "http://www.kuliniewicz.org/music-applet/",
}


class RhythmboxPlugin (musicapplet.player.DBusPlugin):
    """
    Music Applet plugin to interface with Rhythmbox.
    """


    ##################################################################
    #
    # Creation and overall management
    #
    ##################################################################


    def __init__ (self, conf):
        musicapplet.player.DBusPlugin.__init__ (self, conf, "Rhythmbox", "rhythmbox",
                                                "org.mate.Rhythmbox")

        self.min_rating = 0.0
        self.max_rating = 5.0

        self.__handlers = []
        self.__uri = None

        self.__player = None
        self.__shell = None

        self.__art_source = None


    ##################################################################
    #
    # Playback control
    #
    ##################################################################


    def toggle_playback (self):
        self.__player.playPause (False,
                                 reply_handler=lambda: None,
                                 error_handler=self._report_dbus_error)


    def previous (self):
        self.__player.previous (reply_handler=lambda: None,
                                error_handler=self._report_dbus_error)


    def next (self):
        self.__player.next (reply_handler=lambda: None,
                            error_handler=self._report_dbus_error)


    ##################################################################
    #
    # Elapsed time
    #
    ##################################################################


    def __set_elapsed (self, elapsed):
        if elapsed > self.MAX_TIME:
            self.elapsed = -1
        else:
            self.elapsed = elapsed

        if not self.playing and self.elapsed < 0:
            self._set_no_song ()


    ##################################################################
    #
    # Playing
    #
    ##################################################################


    def __set_playing (self, playing):
        self.playing = playing

        if not self.playing and self.elapsed < 0:
            self._set_no_song ()


    ##################################################################
    #
    # URI
    #
    ##################################################################


    def __set_uri (self, uri):
        if self.__art_source is not None:
            gobject.source_remove (self.__art_source)
            self.__art_source = None

        if uri is not None and uri != "":
            self.__uri = uri
            self.__shell.getSongProperties (uri,
                                            reply_handler=self.__set_song_properties,
                                            error_handler=self._report_dbus_error)
        else:
            self._set_no_song ()


    def __set_song_properties (self, properties):
        self.title = properties.get ("title") or None
        self.artist = properties.get ("artist") or None
        self.album = properties.get ("album") or None
        self.duration = properties.get ("duration", -1)
        self.rating = properties.get ("rating", -1.0)
        if properties.has_key ("rb:coverArt"):
            self._load_art (properties.get ("rb:coverArt"))
        elif properties.has_key ("rb:coverArt-uri"):
            self._load_art (properties.get ("rb:coverArt-uri"))
        else:
            self.art = None
            # Rhythmbox sometimes takes a few seconds before actually loading
            # the art, so maybe try again a little later.
            if self.elapsed < 3 and self.__art_source is None:
                self.__art_source = gobject.timeout_add (3000, self.__refetch_art)


    def __set_song_properties_art_only (self, properties):
        """
        See if art is now available for the album, and if so, show it.
        """

        if properties.has_key ("rb:coverArt"):
            self._load_art (properties.get ("rb:coverArt"))
        elif properties.has_key ("rb:coverArt-uri"):
            self._load_art (properties.get ("rb:coverArt-uri"))
        self.__art_source = None


    def __refetch_art (self):
        """
        Look up the song properties again in case art is now available.
        """

        self.__shell.getSongProperties (self.__uri,
                                        reply_handler=self.__set_song_properties_art_only,
                                        error_handler=self._report_dbus_error)
        return False


    def __property_changed (self, uri, property, old_value, new_value):
        # Rhythmbox uses 'title' for the radio station name, but displays
        # it as though it were 'album'.  Let's do likewise.  Of course,
        # override if -artist or -album gets updated.

        if property == "title":
            self.title = new_value
        elif property == "rb:stream-song-title":
            if self.album is None:
                self.album = self.title
            self.title = new_value or None
        elif property == "artist" or property == "rb:stream-song-artist":
            self.artist = new_value or None
        elif property == "album" or property == "rb:stream-song-album":
            self.album = new_value or None
        elif property == "duration":
            self.duration = new_value
        elif property == "rating":
            self.rating = new_value
        elif property == "rb:coverArt" or property == "rb:coverArt-uri":
            self._load_art (new_value)


    ##################################################################
    #
    # Rating
    #
    ##################################################################


    def __set_rating (self, rating):
        self.rating = rating


    def rate_song (self, rating):
        self.__shell.setSongProperty (self.__uri, "rating", float (rating),
                                      reply_handler=lambda: self.set_property ("rating", rating),
                                      error_handler=self._report_dbus_error)


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

        proxy = bus.get_object ("org.mate.Rhythmbox", "/org/mate/Rhythmbox/Player")
        self.__player = dbus.Interface (proxy, "org.mate.Rhythmbox.Player")

        proxy = bus.get_object ("org.mate.Rhythmbox", "/org/mate/Rhythmbox/Shell")
        self.__shell = dbus.Interface (proxy, "org.mate.Rhythmbox.Shell")

        self.__handlers = [
            self.__player.connect_to_signal ("playingChanged", self.__set_playing),
            self.__player.connect_to_signal ("elapsedChanged", self.__set_elapsed),
            self.__player.connect_to_signal ("playingUriChanged", self.__set_uri),
            self.__player.connect_to_signal ("playingSongPropertyChanged", self.__property_changed),
        ]

        self.__player.getPlaying (reply_handler=self.__set_playing,
                                  error_handler=self._report_dbus_error)

        self.__player.getElapsed (reply_handler=self.__set_elapsed,
                                  error_handler=self._report_dbus_error)

        self.__player.getPlayingUri (reply_handler=self.__set_uri,
                                     error_handler=self._report_dbus_error)

        self.connected = True


    def _disconnect (self):
        for handler in self.__handlers:
            handler.remove ()
        self.__handlers = []

        self.__player = None
        self.__shell = None

        self._set_no_song ()
        self.connected = False


def create_instance (conf):
    return RhythmboxPlugin (conf)
