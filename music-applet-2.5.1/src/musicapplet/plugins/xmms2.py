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
        "name": "XMMS2",
        "version": musicapplet.defs.VERSION,
        "icon-name": "music-applet-xmms2",
        "author": "Paul Kuliniewicz <paul@kuliniewicz.org>",
        "copyright": "(C) 2007 Paul Kuliniewicz",
        "website": "http://www.kuliniewicz.org/music-applet/",
}


class XMMS2Plugin (musicapplet.player.PluginBase):
    """
    Music Applet plugin to interface with XMMS2.
    """


    ##################################################################
    #
    # Creation and overall management
    #
    ##################################################################


    def __init__ (self, conf):
        musicapplet.player.PluginBase.__init__ (self, conf, INFO["name"], INFO["icon-name"])

        import xmmsclient

        self.min_rating = 0.0
        self.max_rating = 5.0

        self.__client = xmmsclient.XMMS ("MusicApplet")
        self.__connector = None
        self.__poll_source = None

        self.__current_id = 0
        self.__art_url = None


    def start (self):
        if not self.connected and self.__poll_source is None and self.__poll ():
            self.__poll_source = gobject.timeout_add (1000, self.__poll)


    def stop (self):
        if self.connected:
            self.__disconnect (False)

        if self.__poll_source is not None:
            gobject.source_remove (self.__poll_source)
            self.__poll_source = None


    def create_config_dialog (self):
        xml = glade.XML (os.path.join (musicapplet.defs.PKG_DATA_DIR, "xmms2.glade"))
        dialog = xml.get_widget ("xmms2-dialog")

        full_key = self._conf.resolve_plugin_key (self, "conn_path")
        self._conf.bind_string (full_key, xml.get_widget ("path"), "text")

        full_key = self._conf.resolve_plugin_key (self, "command")
        self._conf.bind_string (full_key, xml.get_widget ("command"), "text")

        xml.get_widget ("browse").connect ("clicked",
                    lambda button: self._browse_for_command (dialog, _("%s Launcher") % "XMMS2"))
        dialog.set_title (_("%s Plugin") % "XMMS2")
        dialog.set_default_response (gtk.RESPONSE_CLOSE)
        dialog.connect ("response", lambda dialog, response: dialog.hide ())
        return dialog


    def __poll (self):
        try:
            full_key = self._conf.resolve_plugin_key (self, "conn_path")
            path = self._conf.client.get_string (full_key)
            if path is None:
                path = os.getenv ("XMMS_PATH")
            self.__client.connect (path, self.__disconnect)
            self.__connector = GLibConnector (self.__client)

            self.__client.broadcast_playback_current_id (self.__set_current_id)
            self.__client.broadcast_medialib_entry_changed (self.__entry_changed)
            self.__client.signal_playback_playtime (
                    lambda result: (self.__set_elapsed (result), result.restart ()))
            self.__client.broadcast_playback_status (self.__set_status)

            self.__client.playback_current_id (self.__set_current_id)
            self.__client.playback_playtime (self.__set_elapsed)
            self.__client.playback_status (self.__set_status)

            self.connected = True
            self.__poll_source = None
            return False

        except IOError:
            return True


    def __disconnect (self, still_enabled=True):
        self.__connector.shutdown ()
        self.__connector = None
        self._set_no_song ()
        self.connected = False

        if still_enabled and self.__poll_source is None:
            self.__poll_source = gobject.timeout_add (1000, self.__poll)


    ##################################################################
    #
    # Playback control
    #
    ##################################################################


    def toggle_playback (self):
        if self.playing:
            self.__client.playback_pause ()
        else:
            self.__client.playback_start ()


    def previous (self):
        self.__client.playlist_set_next_rel (-1, lambda result: self.__client.playback_tickle ())


    def next (self):
        self.__client.playlist_set_next_rel (1, lambda result: self.__client.playback_tickle ())


    def rate_song (self, rating):
        self.__client.medialib_property_set (self.__current_id, "rating", int (rating), "clients/generic")
        self.rating = rating


    ##################################################################
    #
    # Status information
    #
    ##################################################################


    def __set_current_id (self, result):
        self.__current_id = result.value ()
        self.__client.medialib_get_info (self.__current_id, self.__set_song_info)


    def __entry_changed (self, result):
        if result.value () == self.__current_id:
            self.__client.medialib_get_info (self.__current_id, self.__set_song_info)


    def __set_elapsed (self, result):
        self.elapsed = result.value () / 1000


    def __set_status (self, result):
        import xmmsclient
        self.playing = (result.value () == xmmsclient.PLAYBACK_STATUS_PLAY)


    def __set_song_info (self, result):
        info = result.value ()
        if info is None:
            self._set_no_song ()
            return

        if "title" in info:
            if self.title != info["title"]:
                self.title = info["title"]
        else:
            self.title = None

        if "artist" in info:
            if self.artist != info["artist"]:
                self.artist = info["artist"]
        else:
            self.artist = None

        if "album" in info:
            if self.album != info["album"]:
                self.album = info["album"]
        elif "channel" in info:
            if self.album != info["channel"]:
                self.album = info["channel"]
        else:
            self.album = None

        if "duration" in info:
            self.duration = info["duration"] / 1000
        else:
            self.duration = -1

        vote_score = ("clients/generic/voting", "vote_score")
        vote_count = ("clients/generic/voting", "vote_count")
        if ("clients/generic", "rating") in info:
            self.rating = info[("clients/generic", "rating")]
        elif vote_score in info and vote_count in info and info[vote_count] > 0:
            self.rating = info[vote_score] / info[vote_count]
        else:
            self.rating = 0.0

        art_url = self.__find_art_url (info)
        if self.__art_url != art_url:
            self.__art_url = art_url
            self._load_art (art_url)


    def __find_art_url (self, info):
        for source in ["clients/generic/arturl", "clients/generic/arturl_unverified"]:
            for key in ["album_front_thumbnail", "album_front_small", "album_front_large"]:
                if (source, key) in info:
                    return info[(source, key)]
        return None


def create_instance (conf):
    return XMMS2Plugin (conf)


##################################################################
#
# The following was taken from the xmms2-tutorial git repo
#
# http://git.xmms.se/?p=xmms2-tutorial.git;a=summary
#
##################################################################

####
#
# A GLib connector for PyGTK - 
#	to use with your cool PyGTK xmms2 client.
# Tobias Rundstrom <tru@xmms.org)
#       ... and modified by Paul Kuliniewicz <paul@kuliniewicz.org>
#
# Just create the GLibConnector() class with a xmmsclient as argument
#
###

class GLibConnector:
    def __init__ (self, xmms):
        self.xmms = xmms
        xmms.set_need_out_fun (self.need_out)
        self.in_source = gobject.io_add_watch (self.xmms.get_fd (), gobject.IO_IN, self.handle_in)
        self.out_source = None

    def need_out (self, i):
        if self.xmms.want_ioout () and self.out_source is None:
            self.out_source = gobject.io_add_watch (self.xmms.get_fd (), gobject.IO_OUT, self.handle_out)

    def handle_in (self, source, cond):
        if cond == gobject.IO_IN:
            self.xmms.ioin ()

        return True

    def handle_out (self, source, cond):
        if cond == gobject.IO_OUT:
            self.xmms.ioout ()
        if not self.xmms.want_ioout ():
            self.out_source = None
            return False
        else:
            return True

    def shutdown (self):
        if self.in_source is not None:
            gobject.source_remove (self.in_source)
            self.in_source = None
        if self.out_source is not None:
            gobject.source_remove (self.out_source)
            self.out_source = None
        self.xmms.set_need_out_fun (None)
