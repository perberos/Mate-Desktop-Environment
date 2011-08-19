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

from gettext import gettext as _
import gobject
import gtk
from gtk import glade

import os
import sys
import threading

import musicapplet.defs
import musicapplet.player


INFO = {
        "name": "Amarok",
        "version": musicapplet.defs.VERSION,
        "icon-name": "amarok",
        "author": "Paul Kuliniewicz <paul@kuliniewicz.org>",
        "copyright": "(C) 2007-2008 Paul Kuliniewicz",
        "website": "http://www.kuliniewicz.org/music-applet/",
}


class AmarokPlugin (musicapplet.player.PluginBase):
    """
    Music Applet plugin to interface with Amarok.
    """


    ##################################################################
    #
    # Creation and connection management
    #
    ##################################################################


    def __init__ (self, conf):
        musicapplet.player.PluginBase.__init__ (self, conf, INFO["name"], INFO["icon-name"])

        import kdecore
        import dcopext

        self.min_rating = 0.0
        self.max_rating = 5.0

        self._app = None
        self._app_lock = threading.Lock ()

        self._should_start = False
        self._should_start_lock = threading.Lock ()

        self.__amarok = None
        self.__poll_source = None
        self.__url = None

    def start (self):
        # KApplication is needed to reliably start the DCOP server and to create the
        # wrapper object for Amarok's DCOP interface.  It's created in a separate
        # thread because it can take a long time to create.  The locking assures only
        # one KApplication ever gets created, even if start() is called repeatedly
        # while the other thread is working.

        if self._app_lock.acquire (False):
            if self._app is None:
                self._should_start = True       # lock unnecessary; no other thread running
                thread = AmarokInitThread (self)
                thread.start()
            else:
                self._start_safely ()
                self._app_lock.release ()

    def _start_safely (self):
        if self.__amarok is None:
            import dcopext
            self.__amarok = dcopext.DCOPApp ("amarok", self._app.dcopClient ())

        if self.__poll_source is None and self.__poll_connect ():
            self.__poll_source = gobject.timeout_add (1000, self.__poll_connect)

    def stop (self):
        if self.__poll_source is not None:
            gobject.source_remove (self.__poll_source)
            self.__poll_source = None

        self._should_start_lock.acquire (True)
        self._should_start = False
        self._should_start_lock.release ()

        # Leave _app alone; KDE doesn't like it when you try creating more than one.

        self._set_no_song ()
        self.__amarok = None
        self.connected = False

    def __poll_connect (self):
        ok, dummy = self.__amarok.player.title ()       # something to actually test for a connection
        if ok:
            self.connected = True
            self.__poll_status ()
            self.__poll_source = gobject.timeout_add (1000, self.__poll_status)
            return False
        else:
            return True

    def __safe_call (self, method_name):
        """
        Call a DCOP method on the player, throwing an exception if it fails.
        """

        ok, result = self.__amarok.player.method (method_name) ()
        if ok:
            return result
        else:
            raise RuntimeError


    ##################################################################
    #
    # Playback controls
    #
    ##################################################################


    def toggle_playback (self):
        self.__amarok.player.playPause ()

    def previous (self):
        self.__amarok.player.prev ()

    def next (self):
        self.__amarok.player.next ()

    def rate_song (self, rating):
        self.__amarok.player.setRating (int (rating * 2))


    ##################################################################
    #
    # Status
    #
    ##################################################################

    def __poll_status (self):
        try:
            # TODO: when dcopext supports async calls, use them instead
            url = self.__safe_call ("encodedURL") or None
            if url != self.__url:
                self.__url = url
                if self.__url is not None:
                    self.title = self.__safe_call ("title") or None
                    self.album = self.__safe_call ("album") or None
                    self.artist = self.__safe_call ("artist") or None
                    self.duration = self.__safe_call ("trackTotalTime")
                    self.rating = self.__safe_call ("rating") / 2.0
                    art_file = self.__safe_call ("coverImage")
                    if not art_file.endswith ("@nocover.png"):
                        self._load_art (art_file)
                    else:
                        self.art = None
                else:
                    self._set_no_song ()
            elif self.__url is not None and self.duration == 0:
                # Radio stations can change some metadata without changing URL
                title = self.__safe_call ("title")
                if title != self.title:
                    self.title = title
                    self.album = self.__safe_call ("album") or None
                    self.artist = self.__safe_call ("artist") or None

            self.playing = self.__safe_call ("isPlaying")
            self.elapsed = self.__safe_call ("trackCurrentTime")

            return True

        except RuntimeError:
            self._set_no_song ()
            self.connected = False
            self.__poll_source = gobject.timeout_add (1000, self.__poll_connect)
            return False


    ##################################################################
    #
    # Configuration
    #
    ##################################################################

    def create_config_dialog (self):
        xml = glade.XML (os.path.join (musicapplet.defs.PKG_DATA_DIR, "amarok.glade"))
        dialog = xml.get_widget ("amarok-dialog")

        full_key = self._conf.resolve_plugin_key (self, "command")
        self._conf.bind_string (full_key, xml.get_widget ("command"), "text")

        xml.get_widget ("browse").connect ("clicked", lambda button: self._browse_for_command (dialog, "Amarok"))
        dialog.set_title (_("%s Plugin") % "Amarok")
        dialog.set_default_response (gtk.RESPONSE_CLOSE)
        dialog.connect ("response", lambda dialog, response: dialog.hide ())
        return dialog


class AmarokInitThread (threading.Thread):
    """
    Separate thread that initializes the plugin's KApplication object.  This
    is done in a thread because the call can take several seconds to complete
    if the KDE services are not already running.

    When thread execution begins, it inherits the _app_lock of the plugin.
    """

    def __init__ (self, plugin):
        threading.Thread.__init__ (self, name="Amarok Init Thread")
        self.__plugin = plugin

    def run (self):
        # The KApplication constructor likes to call exit() if it finds
        # arguments it doesn't understand in argv -- such as the arguments
        # MateComponent uses to launch the applet -- so hide everything but the
        # program name from it.

        import kdecore
        fake_argv = sys.argv[0:1]
        self.__plugin._app = kdecore.KApplication (fake_argv, "music-applet")

        # If a non-Amarok player was already running at applet startup, it's
        # probable that start() and stop() were both called before getting
        # to this point in the thread.

        self.__plugin._should_start_lock.acquire (True)
        if self.__plugin._should_start:
            self.__plugin._start_safely ()
            self.__plugin._should_start = False
        self.__plugin._should_start_lock.release ()

        self.__plugin._app_lock.release ()


def create_instance (conf):
    return AmarokPlugin (conf)
