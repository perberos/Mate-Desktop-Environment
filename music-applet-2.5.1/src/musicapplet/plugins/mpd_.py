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

from gettext import gettext as _
import gobject
import gtk
from gtk import glade

import os
import Queue
import random
import socket
import threading
import time


# Naming this file mpd_ instead of mpd avoids confusion with the mpd
# module that provides the MPD client library.


INFO = {
        "name": "MPD",
        "version": musicapplet.defs.VERSION,
        "icon-name": "music-applet-mpd",
        "author": "Paul Kuliniewicz <paul@kuliniewicz.org>",
        "copyright": "(C) 2007-2008 Paul Kuliniewicz",
        "website": "http://www.kuliniewicz.org/music-applet/",
}


class MPDPlugin (musicapplet.player.PluginBase):


    def __init__ (self, conf):
        musicapplet.player.PluginBase.__init__ (self, conf, INFO["name"], INFO["icon-name"])

        import mpd

        self.__thread = None


    def start (self):
        if self.__thread is None:
            self.__thread = MPDThread (self)
            self.__thread.start ()


    def stop (self):
        if self.__thread is not None:
            self.__thread.cancel = True
            self.__thread = None

        self._set_no_song ()
        self.connected = False


    def toggle_playback (self):
        self.__thread.queue.put (("toggle_playback",))


    def previous (self):
        self.__thread.queue.put (("previous",))


    def next (self):
        self.__thread.queue.put (("next",))


    # def rate_song (self, rating):
    #     pass


    def create_config_dialog (self):
        xml = glade.XML (os.path.join (musicapplet.defs.PKG_DATA_DIR, "mpd.glade"))
        dialog = xml.get_widget ("mpd-dialog")

        full_key = self._conf.resolve_plugin_key (self, "host")
        self._conf.bind_string (full_key, xml.get_widget ("host"), "text")

        full_key = self._conf.resolve_plugin_key (self, "port")
        self._conf.bind_int (full_key, xml.get_widget ("port"), "value")

        full_key = self._conf.resolve_plugin_key (self, "command")
        self._conf.bind_string (full_key, xml.get_widget ("command"), "text")

        xml.get_widget ("browse").connect ("clicked",
                    lambda button: self._browse_for_command (dialog, _("%s Launcher") % "MPD"))
        dialog.set_title (_("%s Plugin") % "MPD")
        dialog.set_default_response (gtk.RESPONSE_CLOSE)
        dialog.connect ("response", lambda dialog, response: dialog.hide ())
        return dialog


    def launch (self):
        self.__thread.queue.put (("launch",))


    def _show_password_dialog (self):
        xml = glade.XML (os.path.join (musicapplet.defs.PKG_DATA_DIR, "password.glade"))
        dialog = xml.get_widget ("password-dialog")

        xml.get_widget ("message").set_text (_("A password is needed to connect to the MPD server at %s:%d.") %
                                             (self.__thread.host, self.__thread.port))

        dialog.set_default_response (gtk.RESPONSE_OK)
        dialog.connect ("response", self.__password_response_cb, xml)
        dialog.show ()

        return False


    def __password_response_cb (self, dialog, response, xml):
        if response == gtk.RESPONSE_OK:
            self.__thread.queue.put (("password",
                                      xml.get_widget ("password").get_text (),
                                      xml.get_widget ("remember").get_active ()))
        else:
            self.__thread.queue.put (("cancel",))
        dialog.destroy ()


class MPDThread (threading.Thread):
    """
    Separate thread that does all the actual interaction with MPD, since the
    MPD client library uses blocking I/O, which can cause unpredictable and
    large delays when running MPD over a network.
    """


    MIN_INITIAL_DELAY = 8       # seconds
    MAX_INITIAL_DELAY = 60      # seconds
    MAX_DELAY = 600             # seconds


    def __init__ (self, plugin):
        threading.Thread.__init__ (self, name="MPD Thread")
        self.setDaemon (True)
        self.queue = Queue.Queue ()
        self.cancel = False

        # These tell the password dialog where the server is; they aren't
        # actually used for connecting; mpdclient2 handles the defaults there.
        self.host = None
        self.port = None

        self.__plugin = plugin
        self.__mpd = None
        self.__delay = 0
        self.__autoconnect = True
        self.__authenticated = False


    def run (self):
        while not self.cancel:
            if self.__mpd is None and self.__autoconnect:
                self.__try_connect ()

            if self.__authenticated:
                self.__poll ()
            elif self.__mpd is None and self.__autoconnect:
                if self.__delay <= 0:
                    self.__delay = random.randint (self.MIN_INITIAL_DELAY, self.MAX_INITIAL_DELAY)
                else:
                    self.__delay = min (self.__delay * 2, self.MAX_DELAY)
            else:
                self.__delay = None

            if not self.cancel:
                self.__process_queue (self.__delay)


    def __try_connect (self):
        import mpd
        try:
            full_key = self.__plugin._conf.resolve_plugin_key (self.__plugin, "host")
            host = self.__plugin._conf.client.get_string (full_key)
            if host == "":
                host = "localhost"
                self.host = os.environ.get ("MPD_HOST", "localhost")
                if "@" in self.host:
                    self.host = (self.host.split ("@", 1))[1]
            else:
                self.host = host

            full_key = self.__plugin._conf.resolve_plugin_key (self.__plugin, "port")
            port = self.__plugin._conf.client.get_int (full_key)
            if port <= 0:
                port = 6600
                self.port = int (os.environ.get ("MPD_PORT", 6600))
            else:
                self.port = port

            self.__mpd = mpd.MPDClient ()
            self.__mpd.connect (host, port)

            if not self.__authenticate ():
                try:
                    import matekeyring
                    keyring = matekeyring.get_default_keyring_sync ()
                    found = matekeyring.find_items_sync (matekeyring.ITEM_NETWORK_PASSWORD,
                                                          { "server":self.host, "port":self.port, "protocol":"mpd" })
                    for item in found:
                        if self.__authenticate (item.secret, False):
                            break
                except ImportError:
                    print "MPD plugin: unable to check for saved passwords"
                except matekeyring.DeniedError:
                    print "MPD plugin: no passwords found"
                except AttributeError:
                    print "MPD plugin: password lookup requires matekeyring >= 2.18.0"

            if not self.__authenticated:
                gobject.idle_add (self.__plugin._show_password_dialog)

        except socket.error:
            # Thrown if connect fails.
            self.__mpd = None


    def __authenticate (self, password=None, save=None):
        if password is not None:
            import mpd
            try:
                self.__mpd.password (password)
            except mpd.CommandError:
                return False

        # Just because the password worked doesn't mean we have all the
        # permissions we need.
        needed_commands = ("status", "currentsong", "play", "previous", "next")
        if len ([c for c in self.__mpd.notcommands () if c in needed_commands]) > 0:
            return False

        if save:
            try:
                import matekeyring
                keyring = matekeyring.get_default_keyring_sync ()
                result = matekeyring.item_create_sync (keyring,
                                                        matekeyring.ITEM_NETWORK_PASSWORD,
                                                        "Password for MPD server at %s:%d" % (self.host, self.port),
                                                        { "server":self.host, "port":self.port, "protocol":"mpd" },
                                                        password,
                                                        True)
            except ImportError:
                print "MPD plugin: unable to save password; matekeyring not found"

        self.__delay = 1
        self.__authenticated = True
        gobject.idle_add (self.__set_connected, True)
        return True


    def __set_connected (self, connected):
        if not connected:
            self.__plugin._set_no_song ()
        self.__plugin.connected = connected
        return False


    def __poll (self):
        import mpd
        try:
            status = self.__mpd.status ()
            if status["state"] != "stop":
                song = self.__mpd.currentsong ()
            else:
                song = None
            gobject.idle_add (self.__set_status, status, song)
        except mpd.ConnectionError:
            self.__mpd = None
            self.__authenticated = False
            gobject.idle_add (self.__set_connected, False)


    def __set_status (self, status, song):
        self.__plugin.playing = (status["state"] == "play")
        if song is not None:
            if self.__plugin.title != song["title"]:
                self.__plugin.title = song["title"]

            if "artist" in song:
                if self.__plugin.artist != song["artist"]:
                    self.__plugin.artist = song["artist"]
            else:
                self.__plugin.artist = None

            if "album" in song:
                if self.__plugin.album != song["album"]:
                    self.__plugin.album = song["album"]
            else:
                self.__plugin.album = None

            self.__plugin.duration = int (song["time"])
            self.__plugin.elapsed = int (status["time"].split (":")[0])
        else:
            self.__plugin._set_no_song ()

        return False


    def __process_queue (self, delay=None):
        try:
            # Wait up to 'delay' seconds for a command.  Once a command arrives,
            # process the entire queue and finish, since the command probably
            # changed MPD's status somehow, and we want to fetch it ASAP.
            command = self.queue.get (True, delay)
            while True:
                if command[0] == "toggle_playback":
                    if self.__mpd.currentsong () == {}:
                        self.__mpd.play ()
                    else:
                        self.__mpd.pause ()
                elif command[0] == "previous":
                    self.__mpd.previous ()
                elif command[0] == "next":
                    self.__mpd.next ()
                elif command[0] == "password":
                    if not self.__authenticate (command[1], command[2]):
                        gobject.idle_add (self.__plugin._show_password_dialog)
                elif command[0] == "cancel":
                    self.__mpd.close ()
                    self.__mpd.disconnect ()
                    self.__mpd = None
                    self.__authenticated = False
                    self.__autoconnect = False
                elif command[0] == "launch":
                    if self.__mpd is None and not self.__autoconnect:
                        # We stopped trying to connect, so maybe it's actually running...
                        self.__try_connect ()
                    if self.__mpd is None:
                        # Nope, need to launch it after all.
                        gobject.idle_add (lambda: musicapplet.player.PluginBase.launch (self.__plugin) and False)
                        self.__autoconnect = True
                        self.__delay = 1
                else:
                    print "MPD plugin: (internal) unknown command '%s'" % command[0]
                command = self.queue.get (False)
        except Queue.Empty:
            pass


    def __sleep (self, delay):
        # Since the timeout handlers in other plugins might make sleep() wake
        # up much earlier than we like....
        start = time.time ()
        while start + delay > time.time ():
            time.sleep (start + delay - time.time ())


def create_instance (conf):
    return MPDPlugin (conf)
