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

"""
Base classes for plugins.

All plugins will ultimately subclass PluginBase, which defines the interface
plugins must adhere to and provides a few convenience methods.  Plugins that
will use D-Bus to communicate with their player should subclass DBusPlugin,
which handles finding when the player of interest is running.

Aside from subclassing one of the base classed defined here, there are a few
other things a plugin module must provide.  Take a look at the example
templates to see what these are.
"""

from gettext import gettext as _
import gobject
import gtk
from gtk import gdk, glade

import os.path

import musicapplet.defs
import musicapplet.util


class PluginBase (gobject.GObject):
    """
    Base class for all plugins that implement support for a music player.
    """

    MAX_TIME = 2 ** 31 - 1          # sys.maxint is too big for GObject on 64-bit systems


    ##################################################################
    #
    # Properties
    #
    ##################################################################

    __gproperties__ = {
        "player_name" : (gobject.TYPE_STRING,
                         "player_name",
                         "The (display) name of the player that this plugin connects to",
                         None,
                         gobject.PARAM_READABLE),

        "internal_name" : (gobject.TYPE_STRING,
                           "internal_name",
                           "The internal name of the plugin",
                           None,
                           gobject.PARAM_READABLE),

        "icon_name" : (gobject.TYPE_STRING,
                       "icon_name",
                       "The name of the icon that represents the player",
                       None,
                       gobject.PARAM_READABLE),

        "connected" : (gobject.TYPE_BOOLEAN,
                       "connected",
                       "Whether the plugin is connected",
                       False,
                       gobject.PARAM_READWRITE),

        "playing" : (gobject.TYPE_BOOLEAN,
                     "playing",
                     "Whether the player is currently playing",
                     False,
                     gobject.PARAM_READWRITE),

        "title" : (gobject.TYPE_STRING,
                   "title",
                   "Title of the current song",
                   None,
                   gobject.PARAM_READWRITE),

        "artist" : (gobject.TYPE_STRING,
                    "artist",
                    "Artist who made the current song",
                    None,
                    gobject.PARAM_READWRITE),

        "album" : (gobject.TYPE_STRING,
                   "album",
                   "Album the current song is from",
                   None,
                   gobject.PARAM_READWRITE),

        "duration" : (gobject.TYPE_INT,
                      "duration",
                      "Duration in seconds of the current song",
                      -1, MAX_TIME, -1,
                      gobject.PARAM_READWRITE),

        "rating" : (gobject.TYPE_DOUBLE,
                    "rating",
                    "Rating of the current song",
                    -1.0, 10.0, -1.0,
                    gobject.PARAM_READWRITE),

        "min-rating" : (gobject.TYPE_DOUBLE,
                        "min-rating",
                        "The minimum possible rating",
                        -1.0, 1.0, -1.0,
                        gobject.PARAM_READWRITE),

        "max-rating" : (gobject.TYPE_DOUBLE,
                        "max-rating",
                        "The maximum allowable rating",
                        -1.0, 10.0, -1.0,
                        gobject.PARAM_READWRITE),

        "elapsed" : (gobject.TYPE_INT,
                     "elapsed",
                     "Elapsed time of the current song",
                     -1, MAX_TIME, -1,
                     gobject.PARAM_READWRITE),

        "art": (gobject.TYPE_PYOBJECT,
                "art",
                "Art associated with the song",
                gobject.PARAM_READWRITE),
    }


    player_name = musicapplet.util.make_property ("player_name")
    internal_name = musicapplet.util.make_property ("internal_name")
    icon_name = musicapplet.util.make_property ("icon_name")
    connected = musicapplet.util.make_property ("connected")
    playing = musicapplet.util.make_property ("playing")
    title = musicapplet.util.make_property ("title")
    artist = musicapplet.util.make_property ("artist")
    album = musicapplet.util.make_property ("album")
    duration = musicapplet.util.make_property ("duration")
    rating = musicapplet.util.make_property ("rating")
    min_rating = musicapplet.util.make_property ("min-rating")
    max_rating = musicapplet.util.make_property ("max-rating")
    elapsed = musicapplet.util.make_property ("elapsed")
    art = musicapplet.util.make_property ("art")


    ##################################################################
    #
    # Initialization
    #
    ##################################################################


    def __init__ (self, conf, names, icon_name=None):
        gobject.GObject.__init__ (self)
        self._conf = conf

        if str (type (names)) == "<type 'dict'>":
            player_name = names["display"]
            internal_name = names["internal"]
        else:
            player_name = names
            internal_name = names

        self.__values = {
                "player-name" : player_name,
                "internal-name" : internal_name,
                "icon-name" : icon_name,
                "connected" : False,
                "playing" : False,
                "title" : None,
                "artist" : None,
                "album" : None,
                "duration" : -1,
                "rating" : -1.0,
                "min-rating" : -1.0,
                "max-rating" : -1.0,
                "elapsed" : -1,
                "art": None,
        }


    ##################################################################
    #
    # GObject overrides
    #
    ##################################################################


    def do_get_property (self, property):
        return self.__values[property.name]


    def do_set_property (self, property, value):
        self.__values[property.name] = value


    ##################################################################
    #
    # Public interface
    #
    ##################################################################


    def start (self):
        raise NotImplementedError


    def stop (self):
        raise NotImplementedError


    def toggle_playback (self):
        raise NotImplementedError


    def previous (self):
        raise NotImplementedError


    def next (self):
        raise NotImplementedError


    def launch (self):
        full_key = self._conf.resolve_plugin_key (self, "command")
        command = self._conf.client.get_string (full_key)
        flags = gobject.SPAWN_STDOUT_TO_DEV_NULL | gobject.SPAWN_STDERR_TO_DEV_NULL
        gobject.spawn_async (["/bin/sh", "-c", command], flags=flags)


    # *** OPTIONAL, depending on what the music player or plugin supports ***

    # def rate_song (self, rating):
    #     raise NotImplementedError


    # def create_config_dialog (self):
    #     raise NotImplementedError


    ##################################################################
    #
    # Convenience functions for subclasses
    #
    ##################################################################


    def _browse_for_command (self, parent, program_name=None):
        """
        Run a dialog that prompts for the command to use to launch a program,
        then save it as the plugin's launch command in MateConf.
        """

        if program_name is None:
            program_name = self.player_name
        browser = gtk.FileChooserDialog (_("Choose path to %s") % program_name,
                                         parent,
                                         gtk.FILE_CHOOSER_ACTION_OPEN,
                                         (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL, gtk.STOCK_OK, gtk.RESPONSE_OK))
        browser.set_default_response (gtk.RESPONSE_OK)
        if browser.run () == gtk.RESPONSE_OK:
            full_key = self._conf.resolve_plugin_key (self, "command")
            self._conf.client.set_string (full_key, browser.get_filename ())
        browser.destroy ()


    def _set_no_song (self):
        """
        Update all the properties to reflect that there is no current song.
        """

        self.playing = False
        self.title = None
        self.artist = None
        self.album = None
        self.duration = -1
        self.elapsed = -1
        self.rating = -1.0
        self.art = None


    def _load_art (self, source):
        """
        Load an image from a file or a file:// URI, and set it as self.art;
        set self.art to None if failure.

        It also handles the cases where the "source" is None or an empty
        string.
        """

        try:
            if source is not None:
                if source.startswith ("file://"):
                    source = source[7:]

                if source.startswith ("/"):
                    self.art = gdk.pixbuf_new_from_file (source)
                else:
                    self.art = None
        except gobject.GError:
            # File couldn't be loaded.
            self.art = None


class DBusPlugin (PluginBase):
    """
    Base class for all plugins that rely on D-Bus to communicate with the
    music player.

    This base class handles establishing all the necessary proxies and
    interfaces that are common for any D-Bus-using plugin.
    """


    def __init__ (self, conf, names, icon_name, dbus_name):
        PluginBase.__init__ (self, conf, names, icon_name)

        import dbus
        import dbus.glib

        if dbus.version < (0, 79, 92):
            raise RuntimeError, _("Requires dbus-python 0.80 or later")

        self.__dbus_name = dbus_name
        self.__bus = None
        self.__bus_handlers = []


    def start (self):
        if self.__bus is not None:
            return

        import dbus
        bus = dbus.SessionBus ()

        proxy = bus.get_object ("org.freedesktop.DBus", "/org/freedesktop/DBus")
        self.__bus = dbus.Interface (proxy, "org.freedesktop.DBus")

        self.__bus_handlers = [
            self.__bus.connect_to_signal ("NameOwnerChanged", self.__name_owner_changed_cb,
                                          arg0=self.__dbus_name)
        ]

        self.__bus.NameHasOwner (self.__dbus_name,
                                 reply_handler=self.__name_has_owner_cb,
                                 error_handler=self._report_dbus_error)


    def stop (self):
        self._disconnect ()
        for handler in self.__bus_handlers:
            handler.remove ()
        self.__bus_handlers = []
        self.__bus = None


    def launch (self):
        if not self.connected:
            full_key = self._conf.resolve_plugin_key (self, "launch")
            if self._conf.client.get_string (full_key) == "Command":
                PluginBase.launch (self)
            else:
                self.__bus.StartServiceByName (self.__dbus_name, 0,
                                               reply_handler=lambda result: None,
                                               error_handler=self._report_dbus_error)


    def create_config_dialog (self):
        xml = self._construct_config_dialog ()
        return xml.get_widget ("dbus-dialog")


    def _construct_config_dialog (self):
        """
        Builds the D-Bus plugin basic configuration dialog, and returns the
        glade.XML object for it.

        You'll want to use this in case you override create_config_dialog to
        add your own custom widgets to the dialog.  The "content" widget is a
        VBox you can add you own widgets to.
        """

        xml = glade.XML (os.path.join (musicapplet.defs.PKG_DATA_DIR, "dbus.glade"))
        dialog = xml.get_widget ("dbus-dialog")

        dialog.set_title (_("%s Plugin") % self.player_name)

        full_key = self._conf.resolve_plugin_key (self, "launch")
        self._conf.bind_string_boolean (full_key, "D-Bus", xml.get_widget ("lu-dbus"), "active")
        self._conf.bind_string_boolean (full_key, "Command", xml.get_widget ("lu-command"), "active")
        self._conf.bind_string_boolean (full_key, "Command", xml.get_widget ("command"), "sensitive")
        self._conf.bind_string_boolean (full_key, "Command", xml.get_widget ("browse"), "sensitive")

        full_key = self._conf.resolve_plugin_key (self, "command")
        self._conf.bind_string (full_key, xml.get_widget ("command"), "text")

        xml.get_widget ("browse").connect ("clicked", lambda button: self._browse_for_command (dialog))
        dialog.set_default_response (gtk.RESPONSE_CLOSE)
        dialog.connect ("response", lambda dialog, response: dialog.hide ())

        return xml


    def __name_has_owner_cb (self, has_owner):
        if has_owner:
            self._connect ()


    def __name_owner_changed_cb (self, name, old_owner, new_owner):
        if new_owner != "":
            self._connect ()
        else:
            self._disconnect ()


    def _report_dbus_error (self, error):
        """
        Callback for reporting D-Bus errors.
        """
        
        # TODO: Should this be visible to the user?
        print "D-Bus error: %s" % error


    def _connect (self):
        """
        Called when the player has appeared on the bus.
        """

        raise NotImplementedError


    def _disconnect (self):
        """
        Called when the player has disappeared from the bus.
        """

        raise NotImplementedError



class MPRISPlugin (DBusPlugin):
    """
    Base class for all plugins that communicate with the music player
    using the MPRIS interface via D-Bus.
    """

    CAN_GO_NEXT          = 1 << 0
    CAN_GO_PREV          = 1 << 1
    CAN_PAUSE            = 1 << 2
    CAN_PLAY             = 1 << 3
    CAN_SEEK             = 1 << 4
    CAN_PROVIDE_METADATA = 1 << 5

    STATUS_PLAYING = 0
    STATUS_PAUSED  = 1
    STATUS_STOPPED = 2


    def __init__ (self, conf, names, icon_name, dbus_name):
        DBusPlugin.__init__ (self, conf, names, icon_name, dbus_name)

        self.__dbus_name = dbus_name
        self.__player = None
        self.__handlers = []
        self.__status = self.STATUS_STOPPED
        self.__caps = 0
        self.__poll_source = None


    def _connect (self):
        if self.__player is not None:
            return

        import dbus
        bus = dbus.SessionBus ()

        proxy = bus.get_object (self.__dbus_name, "/Player")
        self.__player = dbus.Interface (proxy, "org.freedesktop.MediaPlayer")

        self.__handlers = [
            self.__player.connect_to_signal ("TrackChange", self.__get_metadata),
            self.__player.connect_to_signal ("StatusChange", self.__get_status),
            self.__player.connect_to_signal ("CapsChange", self.__get_caps),
        ]

        self.__player.GetMetadata (reply_handler=self.__get_metadata,
                                   error_handler=self._report_dbus_error)

        self.__player.GetStatus (reply_handler=self.__get_status,
                                 error_handler=self._report_dbus_error)

        self.__player.GetCaps (reply_handler=self.__get_caps,
                               error_handler=self._report_dbus_error)

        self.__player.PositionGet (reply_handler=self.__position_get,
                                   error_handler=self._report_dbus_error)

        self.__poll_source = gobject.timeout_add (1000, self.__poll)

        self.connected = True


    def _disconnect (self):
        for handler in self.__handlers:
            handler.remove ()
        self.__handlers = []

        if self.__poll_source is not None:
            gobject.source_remove (self.__poll_source)
            self.__poll_source = None

        self.__player = None

        self._set_no_song ()
        self.connected = False


    def toggle_playback (self):
        if self.__status == self.STATUS_PLAYING:
            self.__player.Pause ()
        else:
            self.__player.Play ()


    def previous (self):
        self.__player.Prev ()


    def next (self):
        self.__player.Next ()


    def __get_metadata (self, metadata):
        # VLC hides stream metadata under "nowplaying"
        if metadata.has_key ("nowplaying"):
            self.title = metadata.get ("nowplaying") or None
            self.album = metadata.get ("album", metadata.get ("title", None)) or None
        else:
            self.title = metadata.get ("title", None) or None
            self.album = metadata.get ("album", None) or None
        self.artist = metadata.get ("artist", None) or None

        if metadata.has_key ("time"):
            self.duration = metadata.get ("time")
        elif metadata.has_key ("mtime"):
            self.duration = metadata.get ("mtime") / 1000
        elif metadata.has_key ("length"):
            self.duration = metadata.get ("length") / 1000
        else:
            self.duration = -1

        self.rating = metadata.get ("rating", -1.0)

        if metadata.has_key ("arturl"):
            self._load_art (metadata["arturl"])
        else:
            self.art = None


    def __get_status (self, status):
        # XXX: The spec (http://wiki.xmms2.xmms.se/index.php/MPRIS) claims
        #      this is a list of four ints, but some players only bother
        #      with the first one

        import dbus
        if type (status) == dbus.Struct:
            new_status = status[0]
        else:
            new_status = status
        if self.__status != new_status:
            self.__status = new_status
            self.playing = (self.__status == self.STATUS_PLAYING)
            if self.__status == self.STATUS_STOPPED:
                self._set_no_song ()


    def __get_caps (self, caps):
        if self.__caps != caps:
            self.__caps = caps
            if not (caps & self.CAN_PROVIDE_METADATA):
                self._set_no_song ()


    def __position_get (self, elapsed):
        if elapsed > 0 or self.__status != self.STATUS_STOPPED:
            self.elapsed = elapsed / 1000

            # If playing a radio stream, periodically check for updated
            # metadata, since we can't count on their being a signal sent
            # when this happens.

            if self.duration <= 0 and self.elapsed % 10 == 0:
                self.__player.GetMetadata (reply_handler=self.__get_metadata,
                                           error_handler=self._report_dbus_error)


    def __poll (self):
        self.__player.PositionGet (reply_handler=self.__position_get,
                                   error_handler=self._report_dbus_error)
        return True

    # def rate_song (self, rating): ...


gobject.type_register (PluginBase)
gobject.type_register (DBusPlugin)
gobject.type_register (MPRISPlugin)
