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
Classes that manage the collection of available plugins.

Plugins shouldn't need to worry about anything here.
"""

from gettext import gettext as _
import mateconf
import gobject
import gtk
from gtk import glade

import os
import pydoc
from xml.sax import saxutils

import musicapplet.defs
import musicapplet.util


class Manager (gtk.ListStore):
    """
    Manages the collection of available plugins.
    """

    COL_PATH = 0
    COL_INTERNAL_NAME = 1
    COL_ICON_NAME = 2
    COL_MODULE = 3
    COL_OBJECT = 4
    COL_ENABLED = 5
    COL_HANDLER = 6
    COL_ERROR = 7
    COL_DIALOG = 8
    COL_DISPLAY_NAME = 9


    __gproperties__ = {
        "active": (gobject.TYPE_PYOBJECT,
                   "active",
                   "The currently active plugin instance",
                   gobject.PARAM_READWRITE),

        "preferred": (gobject.TYPE_PYOBJECT,
                      "preferred",
                      "The preferred plugin instance",
                      gobject.PARAM_READWRITE),
    }

    active = musicapplet.util.make_property ("active")
    preferred = musicapplet.util.make_property ("preferred")


    def __init__ (self, conf, dirs):
        gtk.ListStore.__init__ (self, str,                      # COL_PATH
                                      str,                      # COL_INTERNAL_NAME
                                      str,                      # COL_ICON_NAME
                                      gobject.TYPE_PYOBJECT,    # COL_MODULE
                                      gobject.TYPE_PYOBJECT,    # COL_OBJECT
                                      bool,                     # COL_ENABLED
                                      int,                      # COL_HANDLER
                                      str,                      # COL_ERROR
                                      gobject.TYPE_PYOBJECT,    # COL_DIALOG
                                      str)                      # COL_DISPLAY_NAME
        self.__active = None
        self.__preferred = None
        self.__conf = conf

        self.__dirs = dirs
        avail = {}
        for dir in dirs:
            try:
                for file in os.listdir (dir):
                    try:
                        path = os.path.join (dir, file)
                        module = pydoc.importfile (path)
                        module.INFO["path"] = path
                        name = module.INFO.get ("internal-name") or module.INFO["name"]
                        if name not in avail.keys () or self.__better_plugin (module, avail[name]):
                            avail[name] = module
                    except Exception, detail:
                        if not file.startswith ("__init__.py"):
                            print "Failed to open module at %s: %s" % (path, detail)
            except OSError, detail:
                if dir.find ("/.mate2/music-applet/plugins") == -1:
                    print "Failed to scan %s: %s" % (dir, detail)

        names = avail.keys ()
        names.sort (key=str.lower)
        for name in names:
            module = avail[name]
            self.append ((module.INFO["path"],
                          module.INFO.get ("internal-name") or module.INFO["name"],
                          module.INFO["icon-name"],
                          module,
                          None,
                          False,
                          0,
                          "",
                          None,
                          module.INFO.get ("display-name") or module.INFO["name"]))

        full_key = conf.resolve_key ("enabled_plugins")
        self.foreach (self.__set_enabled, (conf.client.get_list (full_key, mateconf.VALUE_STRING), False))
        conf.client.notify_add (full_key,
            lambda client, id, entry, unused:
                   entry.value is not None and
                           self.foreach (self.__set_enabled,
                                         ([v.get_string () for v in entry.value.get_list ()], False)))

        full_key = conf.resolve_key ("preferred_player")
        self.__update_preferred (conf.client.get_string (full_key))
        conf.client.notify_add (full_key,
            lambda client, id, entry, unused:
                   entry.value is not None and self.__update_preferred (entry.value.get_string ()))

        self.foreach (self.__update_status, None)
        self.foreach (self.__find_connected)


    def do_get_property (self, property):
        if property.name == "active":
            return self.__active
        elif property.name == "preferred":
            return self.__preferred
        else:
            raise AttributeError, "unknown property '%s'" % property.name


    def do_set_property (self, property, value):
        if property.name == "active":
            self.foreach (self.__update_status, value)
            self.__active = value
        elif property.name == "preferred":
            self.__preferred = value
        else:
            raise AttributeError, "unknown property '%s'" % property.name


    def toggle_plugin (self, path):
        """
        Toggle the enabled status of the plugin specified by the given path.
        """

        iter = self.get_iter (path)
        if self.get_value (iter, self.COL_ENABLED):
            self.__disable_plugin (iter)
        else:
            self.__enable_plugin (iter, True)


    def __enable_plugin (self, iter, activate):
        """
        Attempt to enable a plugin.  If the operation fails, the plugin will
        remain unenabled.
        """

        (internal_name, module, object, enabled) = self.get (
                iter, self.COL_INTERNAL_NAME, self.COL_MODULE, self.COL_OBJECT, self.COL_ENABLED)
        if enabled:
            return

        try:
            if object is None:
                object = module.create_instance (self.__conf)
                handler = object.connect ("notify::connected", self.__plugin_connected_cb)
                self.set (iter, self.COL_OBJECT, object,
                                self.COL_HANDLER, handler,
                                self.COL_ICON_NAME, module.INFO["icon-name"],
                                self.COL_ERROR, "")
        
            self.set_value (iter, self.COL_ENABLED, True)
            if self.__active is None:
                object.start ()
                if activate and object.connected:
                    self.active = object

            full_key = self.__conf.resolve_key ("enabled_plugins")
            enabled_names = self.__conf.client.get_list (full_key, mateconf.VALUE_STRING)
            if internal_name not in enabled_names:
                enabled_names.append (internal_name)
                self.__conf.client.set_list (full_key, mateconf.VALUE_STRING, enabled_names)

        except Exception, error:
            # XXX: Can we rely on the underlying icon name of the stock icon?
            self.set (iter, self.COL_ICON_NAME, "stock_dialog-error", self.COL_ERROR, error)


    def __disable_plugin (self, iter):
        """
        Disable a plugin.
        """

        (internal_name, object, enabled) = self.get (iter, self.COL_INTERNAL_NAME, self.COL_OBJECT, self.COL_ENABLED)
        if not enabled:
            return

        self.set (iter, self.COL_ENABLED, False)
        if object is not None:
            object.stop ()
            if self.__active is object:
                self.active = None
                self.foreach (self.__find_connected)
            if self.__preferred is object:
                self.preferred = None
                full_key = self.__conf.resolve_key ("preferred_player")
                self.__conf.client.set_string (full_key, "")

        full_key = self.__conf.resolve_key ("enabled_plugins")
        enabled_names = self.__conf.client.get_list (full_key, mateconf.VALUE_STRING)
        if internal_name in enabled_names:
            enabled_names.remove (internal_name)
            self.__conf.client.set_list (full_key, mateconf.VALUE_STRING, enabled_names)


    def __set_enabled (self, model, path, iter, data):
        """
        TreeModel foreach callback that changes plugins' enabled status to
        match the given list of plugin names.
        """

        (enabled_names, activate) = data
        internal_name = model.get_value (iter, model.COL_INTERNAL_NAME)
        if internal_name in enabled_names:
            self.__enable_plugin (iter, activate)
        else:
            self.__disable_plugin (iter)

        return False


    def __better_plugin (self, new, old):
        """
        Check if one module is better than another one.

        Plugins are compared by the following rules, in order:
          * Later versions are better than older versions
          * More optimized copies are better than less optimized copies
          * The new one wins
        """

        new_ver = new.INFO["version"].split (".")
        old_ver = old.INFO["version"].split (".")
        if new_ver > old_ver:
            return True
        if new_ver < old_ver:
            return False

        new_opt = self.__optimization (new)
        old_opt = self.__optimization (old)
        if new_opt > old_opt:
            return True
        if new_opt < old_opt:
            return False

        return True


    def __optimization (self, module):
        """
        Get an estimate of the optimization level of a module.

        The higher the result, the more optimized the module is; unknown
        extensions are assumed to be native code.
        """

        levels = {"pyo": 3, "pyc": 2, "py": 1}
        name = module.INFO["path"]
        ext = name[name.rindex (".") + 1 : -1]
        if ext in levels.keys ():
            return levels[ext]
        else:
            return 4


    def __update_status (self, model, path, iter, active):
        """
        A gtk.TreeModel foreach callback for updating rows when the active
        plugin changes.
        """

        (object, enabled) = model.get (iter, model.COL_OBJECT, model.COL_ENABLED)
        if active is not None:
            if object is not None and object is not active:
                object.stop ()
        else:
            if enabled and object is not None:
                object.start ()

        return False


    def __find_connected (self, model, path, iter):
        """
        A gtk.TreeModel foreach callback that looks for a row with a connected
        plugin.
        """

        (object, enabled) = model.get (iter, model.COL_OBJECT, model.COL_ENABLED)
        if enabled and object is not None and object.connected:
            self.__plugin_connected_cb (object, None)
            return True
        else:
            return False


    def __update_preferred (self, internal_name):
        """
        Update the preferred property based on the name of the preferred
        player.
        """

        iter = musicapplet.util.search_model (self, self.COL_INTERNAL_NAME, internal_name)
        if iter is not None:
            self.preferred = self.get_value (iter, self.COL_OBJECT)


    def __plugin_connected_cb (self, object, pspec):
        if object.connected and self.__active is None:
            self.active = object
        elif not object.connected and object is self.__active:
            self.active = None


def create_plugins_dialog (manager, conf):
    """
    Create a dialog that lets the user configure plugins.
    """

    def selection_changed_cb (selection):
        keys = ["version", "author", "copyright", "website"]
        (model, iter) = selection.get_selected ()
        if iter is not None:
            (module, object, enabled, error) = model.get (
                    iter, model.COL_MODULE, model.COL_OBJECT, model.COL_ENABLED, model.COL_ERROR)
            xml.get_widget ("name").set_markup ("<big><b>" +
                                                _("%s Plugin") % saxutils.escape (module.INFO.get ("display-name") or module.INFO["name"]) +
                                                "</b></big>")
            for key in keys:
                xml.get_widget (key).set_text (module.INFO[key])
            xml.get_widget ("configure").set_sensitive (enabled and object is not None and "create_config_dialog" in dir (object))
            if error != "":
                xml.get_widget ("error").set_text (error)
                xml.get_widget ("error-box").show ()
            else:
                xml.get_widget ("error").set_text ("")
                xml.get_widget ("error-box").hide ()
        else:
            xml.get_widget ("name").set_markup ("<big><b> </b></big>")
            for key in keys:
                xml.get_widget (key).set_text ("")
            xml.get_widget ("configure").set_sensitive (False)
            xml.get_widget ("error").set_text ("")
            xml.get_widget ("error-box").hide ()

    def row_changed_cb (model, path, iter):
        selection = xml.get_widget ("plugins").get_selection ()
        if selection.iter_is_selected (iter):
            selection_changed_cb (selection)

    def configure_cb (button):
        selection = xml.get_widget ("plugins").get_selection ()
        (model, iter) = selection.get_selected ()
        if iter is not None:
            (object, dialog) = model.get (iter, model.COL_OBJECT, model.COL_DIALOG)
            if dialog is None:
                dialog = object.create_config_dialog ()
                model.set_value (iter, model.COL_DIALOG, dialog)
                dialog.connect ("destroy", lambda dialog: model.set_value (iter, model.COL_DIALOG, None))
                dialog.show ()
            else:
                dialog.present ()

    xml = glade.XML (os.path.join (musicapplet.defs.PKG_DATA_DIR, "plugins.glade"))

    # Plugins list

    plugins = xml.get_widget ("plugins")
    plugins.set_model (manager)
    manager.connect ("row-changed", row_changed_cb)

    renderer = gtk.CellRendererToggle ()
    renderer.set_property ("activatable", True)
    renderer.connect ("toggled", lambda renderer, path: manager.toggle_plugin (path))
    column = gtk.TreeViewColumn ("Enabled", renderer, active=manager.COL_ENABLED)
    plugins.append_column (column)

    renderer = gtk.CellRendererPixbuf ()
    column = gtk.TreeViewColumn ("Icon", renderer, icon_name=manager.COL_ICON_NAME)
    plugins.append_column (column)

    renderer = gtk.CellRendererText ()
    column = gtk.TreeViewColumn ("Name", renderer, text=manager.COL_DISPLAY_NAME)
    plugins.append_column (column)

    selection = plugins.get_selection ()
    selection.connect ("changed", selection_changed_cb)
    selection.select_path ("0")

    # Preferred plugin combo box

    preferred = xml.get_widget ("preferred")
    model = manager.filter_new ()
    model.set_visible_column (manager.COL_ENABLED)
    preferred.set_model (model)

    renderer = gtk.CellRendererPixbuf ()
    preferred.pack_start (renderer, False)
    preferred.set_attributes (renderer, icon_name=manager.COL_ICON_NAME)

    renderer = gtk.CellRendererText ()
    preferred.pack_start (renderer, True)
    preferred.set_attributes (renderer, text=manager.COL_DISPLAY_NAME)

    full_key = conf.resolve_key ("preferred_player")
    conf.bind_combo_string (full_key, preferred, manager.COL_INTERNAL_NAME)

    # Everything else

    xml.get_widget ("configure").connect ("clicked", configure_cb)

    xml.get_widget ("plugins-dialog").set_default_response (gtk.RESPONSE_CLOSE)
    xml.get_widget ("plugins-dialog").connect ("response", lambda dialog, response: dialog.hide ())
    return xml.get_widget ("plugins-dialog")


gobject.type_register (Manager)
