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
Classes that provide access to the applet's configuration.

An instance of the Conf class is provided to each plugin, giving it
access to the applet's preferences as well as the preferences specific
to that plugin.  All preferences are stored in MateConf internally.
"""

import mateconf
import gtk
from gtk import glade

import os

import musicapplet.defs
import musicapplet.util


class Conf:
    """
    Helper object for managing the applet's preferences.

    The "client" object is a convenience instance of a MateConf client, which
    plugin writers will use directly most of the time.  The Conf object
    itself is primarily useful in mapping keys into where they're actually
    stored in MateConf, since each instance of the applet has its preferences
    stored in a different location.
    
    There are also methods that bind a MateConf entry to some other object, so
    that changes in one automatically cause a change in the other.
    """


    # For convenience
    client = mateconf.client_get_default ()


    def __init__ (self, applet):
        self.__applet = applet
        print "DEBUG: MateConf root is %s" % applet.get_preferences_key ()


    def bind_boolean (self, full_key, object, property):
        """
        Bind a MateConf boolean value to a GObject's property.
        """

        def mateconf_cb (client, id, entry, unused):
            if entry.value is not None and entry.value.get_bool () != object.get_property (property):
                object.set_property (property, entry.value.get_bool ())

        def object_cb (object, pspec):
            if object.get_property (property) != self.client.get_bool (full_key):
                self.client.set_bool (full_key, object.get_property (property))

        def destroy_cb (object, id):
            self.client.notify_remove (id)

        object.set_property (property, self.client.get_bool (full_key))
        id = self.client.notify_add (full_key, mateconf_cb)
        object.connect ("notify::%s" % property, object_cb)
        object.connect ("destroy", destroy_cb, id)


    def bind_int (self, full_key, object, property):
        """
        Bind a MateConf int value to a GObject's property.
        """

        def mateconf_cb (client, id, entry, unused):
            if entry.value is not None and entry.value.get_int () != object.get_property (property):
                object.set_property (property, entry.value.get_int ())

        def object_cb (object, pspec):
            if object.get_property (property) != self.client.get_int (full_key):
                self.client.set_int (full_key, int (object.get_property (property)))

        def destroy_cb (object, id):
            self.client.notify_remove (id)

        object.set_property (property, self.client.get_int (full_key))
        id = self.client.notify_add (full_key, mateconf_cb)
        object.connect ("notify::%s" % property, object_cb)
        object.connect ("destroy", destroy_cb, id)


    def bind_string (self, full_key, object, property):
        """
        Bind a MateConf string value to a GObject's property.
        """

        def mateconf_cb (client, id, entry, unused):
            if entry.value is not None and entry.value.get_string () != object.get_property (property):
                object.set_property (property, entry.value.get_string ())

        def object_cb (object, pspec):
            if object.get_property (property) != self.client.get_string (full_key):
                self.client.set_string (full_key, object.get_property (property))

        def destroy_cb (object, id):
            self.client.notify_remove (id)

        object.set_property (property, self.client.get_string (full_key))
        id = self.client.notify_add (full_key, mateconf_cb)
        object.connect ("notify::%s" % property, object_cb)
        object.connect ("destroy", destroy_cb, id)


    def bind_string_boolean (self, full_key, value, object, property):
        """
        Bind a MateConf string value to a GObject's boolean property, by
        setting the property to true iff the string matches a particular
        value, and vice versa.
        """

        def mateconf_cb (client, id, entry, unused):
            if entry.value is not None:
                bval = (entry.value.get_string () == value)
                if bval != object.get_property (property):
                    object.set_property (property, bval)

        def object_cb (object, pspec):
            if object.get_property (property) and self.client.get_string (full_key) != value:
                self.client.set_string (full_key, value)

        def destroy_cb (object, id):
            self.client.notify_remove (id)

        object.set_property (property, self.client.get_string (full_key) == value)
        id = self.client.notify_add (full_key, mateconf_cb)
        object.connect ("notify::%s" % property, object_cb)
        object.connect ("destroy", destroy_cb, id)


    def bind_combo_string (self, full_key, combo, column):
        """
        Bind a MateConf string value to a ComboBox column.
        """

        def mateconf_cb (client, id, entry, unused):
            if entry.value is not None:
                iter = musicapplet.util.search_model (combo.get_model (), column, entry.value.get_string ())
                if iter is not None and iter != combo.get_active_iter ():
                    combo.set_active_iter (iter)

        def combo_cb (combo):
            model = combo.get_model ()
            value = model.get_value (combo.get_active_iter (), column)
            if value != self.client.get_string (full_key):
                self.client.set_string (full_key, value)

        def destroy_cb (combo, id):
            self.client.notify_remove (id)

        iter = musicapplet.util.search_model (combo.get_model (), column, self.client.get_string (full_key))
        if iter is not None:
            combo.set_active_iter (iter)
        id = self.client.notify_add (full_key, mateconf_cb)
        combo.connect ("changed", combo_cb)
        combo.connect ("destroy", destroy_cb, id)


    def resolve_key (self, key):
        """
        Resolve the full name of a key for general applet preferences.
        """

        return "%s/%s" % (self.__applet.get_preferences_key (), key)


    def resolve_plugin_key (self, plugin, key):
        """
        Resolve the full name of a key for a particular plugin.

        This function will try the preferred key style "Player-Name/key",
        falling back on the legacy, deprecated style "player-name_key".
        Ultimately, the schema determines which will be used.
        """

        escaped = plugin.internal_name.replace (" ", "-")
        legacy_key = self.resolve_key ("%s_%s" % (escaped.lower (), key))
        if self.client.get (legacy_key) is not None:
            return legacy_key
        else:
            return self.resolve_key ("%s/%s" % (escaped, key))


def create_preferences_dialog (conf):
    """
    Create a dialog for configuring general applet preferences.
    """

    xml = glade.XML (os.path.join (musicapplet.defs.PKG_DATA_DIR, "preferences.glade"))

    full_key = conf.resolve_key ("show_song_information")
    conf.bind_boolean (full_key, xml.get_widget ("show-song-information"), "active")

    full_key = conf.resolve_key ("show_rating")
    conf.bind_boolean (full_key, xml.get_widget ("show-rating"), "active")

    full_key = conf.resolve_key ("show_time")
    conf.bind_boolean (full_key, xml.get_widget ("show-time"), "active")

    full_key = conf.resolve_key ("show_controls")
    conf.bind_boolean (full_key, xml.get_widget ("show-controls"), "active")

    full_key = conf.resolve_key ("show_notification")
    conf.bind_boolean (full_key, xml.get_widget ("show-notification"), "active")
    try:
        import pynotify
    except ImportError:
        xml.get_widget ("show-notification").set_sensitive (False)

    xml.get_widget ("preferences-dialog").set_default_response (gtk.RESPONSE_CLOSE)
    xml.get_widget ("preferences-dialog").connect ("response", lambda dialog, response: dialog.hide ())
    return xml.get_widget ("preferences-dialog")
