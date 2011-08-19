#!/usr/bin/python
# -*- coding: UTF-8 -*-

   #########################################################################
 ##                                                                       ##
##              ┏┓╻┏━┓╻ ╻╺┳╸╻╻  ╻ ╻┏━┓   ╺┳╸┏━╸┏━┓┏┳┓╻┏┓╻┏━┓╻             ##
##              ┃┗┫┣━┫┃ ┃ ┃ ┃┃  ┃ ┃┗━┓    ┃ ┣╸ ┣┳┛┃┃┃┃┃┗┫┣━┫┃             ##
##              ╹ ╹╹ ╹┗━┛ ╹ ╹┗━╸┗━┛┗━┛    ╹ ┗━╸╹┗╸╹ ╹╹╹ ╹╹ ╹┗━╸           ##
##                  — An integrated terminal for caja —               ##
##                                                                        ##
############################################################################
##                                                                        ##
## Caja Terminal - An integrated terminal for caja                ##
##                                                                        ##
## Copyright (C) 2010  Fabien Loison (flo@flogisoft.com)                  ##
##                                                                        ##
## This program is free software: you can redistribute it and/or modify   ##
## it under the terms of the GNU General Public License as published by   ##
## the Free Software Foundation, either version 3 of the License, or      ##
## (at your option) any later version.                                    ##
##                                                                        ##
## This program is distributed in the hope that it will be useful,        ##
## but WITHOUT ANY WARRANTY; without even the implied warranty of         ##
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          ##
## GNU General Public License for more details.                           ##
##                                                                        ##
## You should have received a copy of the GNU General Public License      ##
## along with this program.  If not, see <http://www.gnu.org/licenses/>.  ##
##                                                                        ##
############################################################################
##                                                                        ##
## WEB SITE : http://software.flogisoft.com/caja-terminal/            ##
##                                                                       ##
#########################################################################


"""An integrated terminal for caja"""

__author__ = "Fabien LOISON <flo@flogisoft.com>"
__version__ = "0.7"
__appname__ = "caja-terminal"


import os
import signal
import re
import urllib
import gettext
gettext.install(__appname__)
from xdg import BaseDirectory

import gtk
import pygtk
pygtk.require("2.0")
import pango
import vte

import caja


#Paths
BASE_PATH = "/usr/share/caja-terminal"
TERMINAL_GUI_FILE = os.path.join(
        BASE_PATH,
        "caja-terminal.glade",
        )
PREF_GUI_FILE = os.path.join(
        BASE_PATH,
        "caja-terminal-preferences.glade",
        )
#Config file
CONF_FILE = os.path.join(
        os.environ.get("HOME"),
        ".caja-terminal.conf",
        )
if not os.path.isfile(CONF_FILE): #keeping compatibility with previous versions
    CONF_FILE = os.path.join(
            BaseDirectory.save_config_path(__appname__),
            "caja-terminal.conf",
            )


#Terminal
CURSOR_SHAPE = [
        _("Block"),
        _("Vertical bar"),
        _("Underlined"),
        ]
PREDEF_PALETTE = {
        'Tango': [
            "#000000", "#cc0000", "#4e9a06", "#c4a000",
            "#3465a4", "#75507b", "#06989a", "#d3d7cf",
            "#555753", "#ef2929", "#8ae234", "#fce94f",
            "#729fcf", "#ad7fa8", "#34e2e2", "#ffffff",
            ],
        'Linux console': [
            "#000000", "#aa0000", "#00aa00", "#aa5500",
            "#0000aa", "#aa00aa", "#00aaaa", "#aaaaaa",
            "#555555", "#ff5555", "#55ff55", "#ffff55",
            "#5555ff", "#ff55ff", "#55ffff", "#ffffff",
            ],
        'XTerm': [
            "#000000", "#cd0000", "#00cd00", "#cdcd00",
            "#1e90ff", "#cd00cd", "#00cdcd", "#e5e5e5",
            "#4c4c4c", "#ff0000", "#00ff00", "#ffff00",
            "#4682b4", "#ff00ff", "#00ffff", "#ffffff",
            ],
        'Rxvt': [
            "#000000", "#cd0000", "#00cd00", "#cdcd00",
            "#0000cd", "#cd00cd", "#00cdcd", "#faebd7",
            "#404040", "#ff0000", "#00ff00", "#ffff00",
            "#0000ff", "#ff00ff", "#00ffff", "#ffffff",
            ],
        'Evening': [
            "#333333", "#c75959", "#57af57", "#f5ce54",
            "#4b7bba", "#ca68ca", "#8bcfcf", "#a1aeaf",
            "#757575", "#ffa0a0", "#90ee90", "#ffff60",
            "#add8e6", "#ffbbff", "#e0ffff", "#ffffff",
            ],
        'Fruity': [
            "#111111", "#d1070d", "#4e9a06", "#fb660a",
            "#1066a9", "#ea037c", "#16afc6", "#8598a3",
            "#373737", "#d1070d", "#4e9a06", "#fb660a",
            "#1066a9", "#ea037c", "#16afc6", "#ffffff",
            ],
        }


class Conf(dict):
    """Read/Write and store configuration."""

    def __init__(self, conf_file=None):
        """The constructor.
        
        Argument:
            * conf_file -- the path of the configuration file
        """
        self._conf_file = conf_file
        #Initialize the dictionary
        dict.__init__(self)
        #General
        self['general_starthidden'] = False
        self['general_showscrollbar'] = False
        self['general_defheight'] = 7
        self['general_command'] = "/bin/bash"
        self['general_cursor'] = 0
        #Color
        self['color_text'] = "#ffffff"
        self['color_background'] = "#000000"
        self['color_palettename'] = "Tango"
        self['color_palette'] = []
        #Font
        self['font_name'] = "monospace 10"
        self['font_allowbold'] = True
        #Folders
        self['folders_showinall'] = True
        self['folders_list'] = []
        #Read configuration
        if self._conf_file != None:
            self.import_user_conf()
            self._check_conf()

    def import_user_conf(self):
        """Import user configuration."""
        if os.path.isfile(self._conf_file):
            current_section = "unknown"
            user_conf_file = open(self._conf_file, "r")
            for line in user_conf_file:
                #Comments
                if re.match(r"\s*#.*", line):
                    continue
                #Section
                elif re.match(r"\s*\[([a-z]+)\]\s*", line.lower()):
                    match = re.match(r'\s*\[([a-z]+)\]\s*', line.lower())
                    current_section = match.group(1)
                #Boolean key
                elif re.match(r"\s*([a-z]+)\s*=\s*(yes|no|true|false)\s*", line.lower()):
                    match = re.match(r"\s*([a-z]+)\s*=\s*(yes|no|true|false)\s*", line.lower())
                    key = match.group(1)
                    value = match.group(2)
                    if value in ("yes", "true"):
                        value = True
                    else:
                        value = False
                    self[current_section + "_" + key] = value
                #String key : path
                elif re.match(r"\s*(path|PATH|Path)\s*=\s*\"(.+)\"\s*", line):
                    match = re.match(r"\s*(path|PATH|Path)\s*=\s*\"(.+)\"\s*", line)
                    key = "list"
                    value = match.group(2)
                    self[current_section + "_" + key].append(value)
                #String key : palette
                elif re.match(r"\s*(palette)\s*=\s*\"([0-9a-f#;]+)\"\s*", line.lower()):
                    match = re.match(r"\s*(palette)\s*=\s*\"([0-9a-f#;]+)\"\s*", line.lower())
                    key = "palette"
                    value = match.group(2)
                    self[current_section + "_" + key] = value.split(";")
                #String key
                elif re.match(r"\s*([a-z]+)\s*=\s*\"(.+)\"\s*", line):
                    match = re.match(r"\s*([a-z]+)\s*=\s*\"(.+)\"\s*", line)
                    key = match.group(1)
                    value = match.group(2)
                    self[current_section + "_" + key] = value
                #Integer key
                elif re.match(r"\s*([a-z]+)\s*=\s*([0-9]+)\s*", line.lower()):
                    match = re.match(r"\s*([a-z]+)\s*=\s*([0-9]+)\s*", line.lower())
                    key = match.group(1)
                    value = match.group(2)
                    self[current_section + "_" + key] = int(value)
            user_conf_file.close()

    def save_user_conf(self):
        """Save configuration."""
        try:
            user_conf_file = open(self._conf_file, "w")
            #Warning
            user_conf_file.write("#" + _("Configuration written by Caja Terminal") + "\n")
            user_conf_file.write("#" + _("Please edit with caution") + "\n")
            #General
            user_conf_file.write("\n[GENERAL]\n")
            user_conf_file.write(self._write_bool("general_starthidden"))
            user_conf_file.write(self._write_bool("general_showscrollbar"))
            user_conf_file.write(self._write_int("general_defheight"))
            user_conf_file.write(self._write_str("general_command"))
            user_conf_file.write(self._write_int("general_cursor"))
            #Color
            user_conf_file.write("\n[COLOR]\n")
            user_conf_file.write(self._write_str("color_text"))
            user_conf_file.write(self._write_str("color_background"))
            user_conf_file.write(self._write_str("color_palettename"))
            user_conf_file.write(self._write_palette("color_palette"))
            #Font
            user_conf_file.write("\n[FONT]\n")
            user_conf_file.write(self._write_str("font_name"))
            user_conf_file.write(self._write_bool("font_allowbold"))
            #Folders
            user_conf_file.write("\n[FOLDERS]\n")
            user_conf_file.write(self._write_bool("folders_showinall"))
            user_conf_file.write(self._write_list("folders_list"))
            #End
            user_conf_file.write("\n\n") #I like blank lines at the end :)
        except OSError: #Permission denied
            print("E: [%s:Conf.save_user_conf] Can't write configuration (permission denied)" % __file__)
            return
        except IOError: #Input/Output error
            print("E: [%s:Conf.save_user_conf] Can't write configuration (IO error)" % __file__)
            return
        else:
            user_conf_file.close()

    def _check_conf(self):
        """Check configuration."""
        #General
        if not os.path.isfile(str(self['general_command'])):
            self['general_command'] = "/bin/bash"
        if int(self['general_defheight']) < 3:
            self['general_defheight'] = 3
        elif int(self['general_defheight']) > 20:
            self['general_defheight'] = 20
        if not int(self['general_cursor']) in (0, 1, 2):
            self['general_cursor'] = 0
        #Color
        if not self['color_palettename'] in PREDEF_PALETTE and \
        self['color_palettename'] != "Custom":
            self['color_palettename'] = "Tango"
        if len(self['color_palette']) != 16:
            self['color_palette'] = list(PREDEF_PALETTE['Tango'])
        #Folders
        if len(self['folders_list']) == 0:
            self['folders_showinall'] = True

    def _write_bool(self, key):
        """Return the string to write in config file for boolean key.

        Argument:
          * key -- the name of the CONF key
        """
        wkey = key.split("_")[1]
        if self[key]:
            value = "Yes"
        else:
            value = "No"
        return "\t%s = %s\n" % (wkey, value)

    def _write_list(self, key):
        """Return the string to write in config file for list key (path).

        Argument:
          * key -- the name of the CONF key
        """
        result = ""
        for value in self[key]:
            result += "\tpath = \"%s\"\n" % value
        return result

    def _write_palette(self, key):
        """Return the string to write in config file for palette.

        Argument:
          * key -- the name of the CONF key
        """
        wkey = key.split("_")[1]
        value = ""
        for color in self[key]:
            value += ";%s" % color
        return "\t%s = \"%s\"\n" % (wkey, value[1:])

    def _write_str(self, key):
        """Return the string to write in config file for string key.

        Argument:
          * key -- the name of the CONF key
        """
        wkey = key.split("_")[1]
        value = self[key]
        return "\t%s = \"%s\"\n" % (wkey, value)

    def _write_int(self, key):
        """Return the string to write in config file for integer key.

        Argument:
          * key -- the name of the CONF key
        """
        wkey = key.split("_")[1]
        value = self[key]
        return "\t%s = %i\n" % (wkey, value)


class CajaTerminalPref(object):
    """Preferences window for Caja Terminal."""

    def __init__(self, callback, terminal):
        """The constructor."""
        self._callback = callback
        self._terminal = terminal
        self._conf = Conf(CONF_FILE)
        #Change current path to home dir
        os.chdir(os.environ.get("HOME"))
        #GUI
        self.gui = gtk.Builder()
        self.gui.set_translation_domain(__appname__)
        self.gui.add_from_file(PREF_GUI_FILE)
        self.gui.connect_signals(self)
        #### winMain ####
        self.winMain = self.gui.get_object("winMain")
        #demoTerm
        self.demoTerm = vte.Terminal()
        self.demoTerm.set_size(20, 3)
        #Display something in the terminal
        self.demoTerm.feed("       _\|/_   zZ    ❭       ")
        self.demoTerm.feed("        ❭         _\|/_\n\033[1G")
        self.demoTerm.feed("       (- -) zZ      ❭  \_°< ")
        self.demoTerm.feed("Coin !  ❭         (O O)\n\033[1G")
        self.demoTerm.feed("---oOO--(_)--OOo---  ❭               ❭  ")
        self.demoTerm.feed("---oOO--(_)--OOo---\n\033[2J\033[1;1f")
        self.demoTerm.feed("[\033[40m \033[41m \033[42m \033[43m \033[44m ")
        self.demoTerm.feed("\033[45m \033[46m \033[47m \033[0m] ")
        self.demoTerm.feed("[\033[30m#\033[31m#\033[32m#\033[33m#\033[34m#")
        self.demoTerm.feed("\033[35m#\033[36m#\033[37m#\033[0m] ")
        self.demoTerm.feed("[\033[1;30m#\033[1;31m#\033[1;32m#\033[1;33m#")
        self.demoTerm.feed("\033[1;34m#\033[1;35m#\033[1;36m#\033[1;37m#")
        self.demoTerm.feed("\033[0m] [\033[0m#\033[0m\033[1m#\033[0m\033[4m#")
        self.demoTerm.feed("\033[0m\033[5m#\033[0m\033[7m#\033[0m]\n\033[1G")
        self.demoTerm.feed("CajaTerminal@Preferences:~# ")
        self.demoTerm.show()
        #General
        self.cbShowScrollbar = self.gui.get_object("cbShowScrollbar")
        self.cbStartHidden = self.gui.get_object("cbStartHidden")
        self.spbtnDefHeight = self.gui.get_object("spbtnDefHeight")
        self.entryCmd = self.gui.get_object("entryCmd")
        self.comboboxCursor = self.gui.get_object("comboboxCursor")
        self.lsstCursor = gtk.ListStore(str)
        self.comboboxCursor.set_model(self.lsstCursor)
        self.cellCursor = gtk.CellRendererText()
        self.comboboxCursor.pack_start(self.cellCursor, True)
        self.comboboxCursor.add_attribute(self.cellCursor, "text", 0)
        for shape in CURSOR_SHAPE:
            self.lsstCursor.append([shape])
        #Color
        self.comboboxPalette = self.gui.get_object("comboboxPalette")
        self.lsstPalette = gtk.ListStore(str)
        self.comboboxPalette.set_model(self.lsstPalette)
        self.cellPallette = gtk.CellRendererText()
        self.comboboxPalette.pack_start(self.cellPallette, True)
        self.comboboxPalette.add_attribute(self.cellPallette, "text", 0)
        self._palette_list = []
        for palette in PREDEF_PALETTE:
            self._palette_list.append(palette)
            self.lsstPalette.append([palette])
        self._palette_list.append("Custom")
        gettext.install(__appname__) #Why it does not works whereas it was
                                     #already defined at the beginning :/ ?
        self.lsstPalette.append([_("Custom")])
        self.clbtnFg = self.gui.get_object("clbtnFg")
        self.clbtnBg = self.gui.get_object("clbtnBg")
        self.clbtnPalette = []
        for i in xrange(16):
            self.clbtnPalette.append(self.gui.get_object("clbtnPalette%i" % i))
            self.clbtnPalette[i].connect(
                    "color-set",
                    self.on_clbtnPalette_color_set,
                    i
                    )
        #Font
        self.fontbtn = self.gui.get_object("fontbtn")
        self.cbAllowBold = self.gui.get_object("cbAllowBold")
        #Folders
        self.rbAllFolders = self.gui.get_object("rbAllFolders")
        self.rbFolderList = self.gui.get_object("rbFolderList")
        self.btnFolderRemove = self.gui.get_object("btnRemoveFolder")
        self.trvFolderList = self.gui.get_object("trvFolderList")
        self.lsstFolderList = gtk.ListStore(str)
        self.trvFolderList.set_model(self.lsstFolderList)
        self.columnFolderList = gtk.TreeViewColumn(
                "Path",
                gtk.CellRendererText(),
                text=0,
                )
        self.trvFolderList.append_column(self.columnFolderList)
        #vboxTerm
        self.alTerm = self.gui.get_object("alTerm")
        self.alTerm.add(self.demoTerm)
        #### winAbout ####
        self.winAbout = self.gui.get_object("winAbout")
        self.winAbout.connect("response", self.on_winAbout_response)
        self.winAbout.set_version(__version__)
        #### filechooser ####
        self.filechooser = self.gui.get_object("filechooser")
        ##
        self._put_opt()

    #~~~~ winMAin ~~~~

    def on_btnCancel_clicked(self, widget):
        self.winMain.destroy()

    def on_btnOk_clicked(self, widget):
        self._conf._check_conf()
        CONF['folders_list'] = []
        for key in self._conf:
            if key != "folders_list":
                CONF[key] = self._conf[key]
            else:
                for path in self._conf['folders_list']:
                    CONF['folders_list'].append(path)
        CONF.save_user_conf()
        self._callback(self._terminal)
        self.winMain.destroy()

    def on_btnAbout_clicked(self, widget):
        self.winAbout.show()

    #~~~~ winMAin:General ~~~~

    def on_cbStartHidden_toggled(self, widget):
        self._conf['general_starthidden'] = widget.get_active()

    def on_cbShowScrollbar_toggled(self, widget):
        self._conf['general_showscrollbar'] = widget.get_active()

    def on_spbtnDefHeight_value_changed(self, widget):
        self._conf['general_defheight'] = widget.get_value()

    def on_entryCmd_changed(self, widget):
        self._conf['general_command'] = widget.get_text()

    def on_comboboxCursor_changed(self, widget):
        self._conf['general_cursor'] = widget.get_active()
        self.demoTerm.set_cursor_shape(widget.get_active())

    #~~~~ winLMain:Color ~~~~

    def on_clbtnFg_color_set(self, widget):
        self._conf['color_text'] = str(widget.get_color())
        self._set_palette(self._conf['color_palettename'])

    def on_clbtnBg_color_set(self, widget):
        self._conf['color_background'] = str(widget.get_color())
        self._set_palette(self._conf['color_palettename'])

    def on_comboboxPalette_changed(self, widget):
        index = widget.get_active()
        self._conf['color_palettename'] = self._palette_list[index]
        self._set_palette(self._conf['color_palettename'])

    def on_clbtnPalette_color_set(self, widget, index=0):
        """General method for on_color_set"""
        changed_color = str(widget.get_color())
        if self._conf['color_palettename'] != "Custom":
            self._conf['color_palette'] = list(
                    PREDEF_PALETTE[self._conf['color_palettename']]
                    )
            self._conf['color_palettename'] = "Custom"
            self.comboboxPalette.set_active(self._palette_list.index("Custom"))
        self._conf['color_palette'][index] = changed_color
        self._set_palette(self._conf['color_palettename'])

    #~~~~ winMain:Font ~~~~

    def on_fontbtn_font_set(self, widget):
        self._conf['font_name'] = str(self.fontbtn.get_font_name())
        font = pango.FontDescription(self.fontbtn.get_font_name())
        self.demoTerm.set_font(font)

    def on_cbAllowBold_toggled(self, widget):
        self._conf['font_allowbold'] = widget.get_active()
        self.demoTerm.set_allow_bold(widget.get_active())

    #~~~~ winMain:Folders ~~~~

    def on_rbAllFolders_toggled(self, widget):
        self._conf['folders_showinall'] = widget.get_active()

    def on_trvFolderList_cursor_changed(self, widget):
        model, iter_ = widget.get_selection().get_selected()
        if iter_ != None:
            self.btnFolderRemove.set_sensitive(True)

    def on_btnAddFolder_clicked(self, widget):
        self.filechooser.show()

    def on_btnRemoveFolder_clicked(self, widget):
        self._remove_path_from_list(
                self.trvFolderList,
                self.lsstFolderList,
                self._conf['folders_list'],
                )
        self.btnFolderRemove.set_sensitive(False)

    #~~~~ winAbout ~~~~

    def on_winAbout_delete_event(self, widget, event):
        self.winAbout.hide()
        return True #Don't delete

    def on_winAbout_response(self, widget, response):
        if response < 0:
            self.winAbout.hide()

    #~~~~ filechooser ~~~~

    def on_filechooser_delete_event(self, widget, response):
        self.filechooser.hide()
        return True #Don't delete

    def on_btnFcOpen_clicked(self, widget):
        self._add_path_to_list(
                self.lsstFolderList,
                self.filechooser.get_filename(),
                self._conf['folders_list'],
                )
        self.filechooser.hide()

    def on_btnFcCancel_clicked(self, widget):
        self.filechooser.hide()

    #~~~~

    def _put_opt(self):
        """Put options on the GUI"""
        #General
        self.cbStartHidden.set_active(self._conf['general_starthidden'])
        self.cbShowScrollbar.set_active(self._conf['general_showscrollbar'])
        self.spbtnDefHeight.set_value(self._conf['general_defheight'])
        self.entryCmd.set_text(self._conf['general_command'])
        self.comboboxCursor.set_active(self._conf['general_cursor'])
        #Color
        self.clbtnFg.set_color(gtk.gdk.Color(self._conf['color_text']))
        self.clbtnBg.set_color(gtk.gdk.Color(self._conf['color_background']))
        index = self._palette_list.index(self._conf['color_palettename'])
        self.comboboxPalette.set_active(index)
        self._set_palette(self._conf['color_palettename'])
        #Font
        self.fontbtn.set_font_name(self._conf['font_name'])
        font = pango.FontDescription(self._conf['font_name'])
        self.demoTerm.set_font(font)
        self.cbAllowBold.set_active(self._conf['font_allowbold'])
        self.demoTerm.set_allow_bold(self._conf['font_allowbold'])
        #Folders
        if self._conf['folders_showinall']:
            self.rbAllFolders.set_active(True)
        else:
            self.rbFolderList.set_active(True)
        self._conf['folders_list'] = []
        for path in CONF['folders_list']:
            self._add_path_to_list(
                    self.lsstFolderList,
                    path,
                    self._conf['folders_list'],
                    )

    def _set_palette(self, palette_name):
        if palette_name in PREDEF_PALETTE:
            colors = PREDEF_PALETTE[palette_name]
        elif palette_name == "Custom":
            colors = self._conf['color_palette']
        else:
            colors = PREDEF_PALETTE['Tango']
        fg = self.clbtnFg.get_color()
        bg = self.clbtnBg.get_color()
        palette = []
        for i in xrange(16):
            palette.append(gtk.gdk.Color(colors[i]))
            self.clbtnPalette[i].set_color(palette[i])
        self.demoTerm.set_colors(fg, bg, palette)

    def _remove_path_from_list(self, gtktree, gtklist, conflist):
        """ Remove the path from the list

        Arguments:
          * gtktree -- the gtkTreeView
          * gtklist -- the gtkListStore where we will remove the path
          * confilst -- the "Conf" path list
        """
        model, iter_ = gtktree.get_selection().get_selected()
        conflist.remove(gtklist.get_value(iter_, 0))
        gtklist.remove(iter_)

    def _add_path_to_list(self, gtklist, path, conflist):
        """ Adds the path in the list

        Arguments:
          * path -- the path to add
          * gtklist -- the gtkListStore where we will add the path
          * confilst -- the "Conf" path list
        """
        if not path in conflist:
            gtklist.append([path])
            conflist.append(path)


class CajaTerminal(caja.LocationWidgetProvider):
    """An integrated terminal for Caja."""

    def __init__(self):
        """The constructor."""

    def get_widget(self, uri, window):
        """Returns the widgets to display to Caja."""
        #Kill old child process
        try:
            window.nt_lastpid
        except AttributeError: #First terminal of the window
            window.nt_lastpid = -1
        else: #Kill old child
            if window.nt_lastpid > 0:
                try:
                    os.kill(window.nt_lastpid, signal.SIGKILL)
                except:
                    pass
                finally:
                    window.nt_lastpid = -1
        #Set some variables for new windows
        try:
            window.nt_termheight
        except AttributeError:
            window.nt_termheight = -1
        try:
            window.nt_termhidden
        except AttributeError:
            window.nt_termhidden = CONF['general_starthidden']
        #If it's not a local folder, directory = $HOME
        if uri[:7] == "file://":
            path = urllib.url2pathname(uri[7:])
        else:
            path = os.environ.get("HOME")
        #Disable for desktop folder
        if uri.startswith("x-caja-desktop://"):
            return
        #White list
        if not CONF['folders_showinall'] and \
        not match_path(path, CONF['folders_list']):
            return
        #GUI
        gui = gtk.Builder()
        gui.set_translation_domain(__appname__)
        gui.add_from_file(TERMINAL_GUI_FILE)
        vboxMain = gui.get_object("vboxMain")
        #### hboxHidden ####
        hboxHidden = gui.get_object("hboxHidden")
        if window.nt_termhidden:
            hboxHidden.show()
        else:
            hboxHidden.hide()
        btnPref2 = gui.get_object("btnPref2")
        btnHide = gui.get_object("btnHide")
        btnShow = gui.get_object("btnShow")
        #### hboxTerm ####
        hboxTerm = gui.get_object("hboxTerm")
        if window.nt_termhidden:
            hboxTerm.hide()
        else:
            hboxTerm.show()
        btnPref = gui.get_object("btnPref")
        sclwinTerm = gui.get_object("sclwinTerm")
        #terminal
        terminal = vte.Terminal()
        terminal.last_size = window.nt_termheight
        self.fork_cmd(window, terminal, sclwinTerm, path)
        terminal.show()
        sclwinTerm.add(terminal)
        #scrollbar
        terminal.set_scroll_adjustments(
                gui.get_object("adjH"),
                gui.get_object("adjV"),
                )
        if CONF['general_showscrollbar']:
            vpolicy = gtk.POLICY_ALWAYS
        else:
            vpolicy = gtk.POLICY_NEVER
        sclwinTerm.set_policy(
                gtk.POLICY_NEVER, #Horizontal
                vpolicy, #Vertical
                )
        if window.nt_termheight == -1:
            window.nt_termheight = terminal.get_char_height() * CONF['general_defheight']
        sclwin_width, sclwin_height = sclwinTerm.size_request()
        sclwinTerm.set_size_request(sclwin_width, window.nt_termheight)
        #Apply config on the terminal
        self._set_terminal(terminal)
        #### evResize ####
        evResize = gui.get_object("evResize")
        if window.nt_termhidden:
            evResize.hide()
        else:
            evResize.show()
        #### Signals ####
        #evResize
        evResize.connect("enter-notify-event", self.on_evResize_enter_notify_event,
                sclwinTerm)
        evResize.connect("leave-notify-event", self.on_evResize_leave_notify_event)
        evResize.connect("motion-notify-event", self.on_evResize_motion_notify_event,
                sclwinTerm, terminal, window)
        #Button preferences
        btnPref.connect("clicked", self.preferences, terminal)
        #Button preferences 2
        btnPref2.connect("clicked", self.preferences, terminal)
        #Button Hide
        btnHide.connect("clicked", self.hide,
                window, hboxHidden, hboxTerm, evResize,
                )
        #Button Show
        btnShow.connect("clicked", self.show,
                window, hboxHidden, hboxTerm, terminal, evResize,
                )
        #Terminal
        terminal.connect("destroy", self.on_terminal_destroy, window)
        terminal.connect("child-exited", self.on_terminal_child_exited,
                terminal,
                )
        terminal.connect("commit", self.on_terminal_commit,
                window, sclwinTerm, path,
                )
        terminal.connect("focus", self.on_terminal_need_child,
                window, terminal, sclwinTerm, path,
                )
        terminal.connect("expose-event", self.on_terminal_need_child,
                window, terminal, sclwinTerm, path,
                )
        terminal.connect("button-press-event", self.on_terminal_need_child,
                window, terminal, sclwinTerm, path,
                )
        terminal.connect("key-press-event", self.on_terminal_key_press_event,
                window, sclwinTerm, path,
                )
        terminal.connect("key-release-event", self.on_terminal_key_release_event)
        #DnD
        terminal.drag_dest_set(
                gtk.DEST_DEFAULT_MOTION |
                gtk.DEST_DEFAULT_HIGHLIGHT |
                gtk.DEST_DEFAULT_DROP,
                [('text/uri-list', 0, 80)],
                gtk.gdk.ACTION_COPY,
                )
        terminal.connect("drag_motion", self.on_terminal_drag_motion)
        terminal.connect("drag_drop", self.on_terminal_drag_drop)
        terminal.connect("drag_data_received", self.on_terminal_drag_data_received)
        #### Accel ####
        accel_group = gtk.AccelGroup()
        window.add_accel_group(accel_group)
        terminal.add_accelerator(
                "paste-clipboard",
                accel_group,
                ord('V'),
                gtk.gdk.CONTROL_MASK | gtk.gdk.SHIFT_MASK,
                gtk.ACCEL_VISIBLE,
                )
        terminal.add_accelerator(
                "copy-clipboard",
                accel_group,
                ord('C'),
                gtk.gdk.CONTROL_MASK | gtk.gdk.SHIFT_MASK,
                gtk.ACCEL_VISIBLE,
                )
        btnShow.add_accelerator(
                "clicked",
                accel_group,
                ord('T'),
                gtk.gdk.CONTROL_MASK | gtk.gdk.SHIFT_MASK,
                gtk.ACCEL_VISIBLE,
                )
        btnHide.add_accelerator(
                "clicked",
                accel_group,
                ord('T'),
                gtk.gdk.CONTROL_MASK | gtk.gdk.SHIFT_MASK,
                gtk.ACCEL_VISIBLE,
                )
        #### Return the widgets ####
        vboxMain.unparent()
        return vboxMain

    def fork_cmd(self, window, terminal, rwidget, path):
        #Change to current dir
        os.chdir(path)
        #Size
        width, height = rwidget.size_request()
        rwidget.set_size_request(width, window.nt_termheight)
        #fork
        window.nt_lastpid = terminal.fork_command(CONF['general_command'])
        terminal.feed("\033[2K\033[1G")
        terminal.has_child = True
        terminal.ctrl_pressed = False
        terminal.current_command = ""
        #Go back to home dir
        os.chdir(os.environ.get("HOME"))

    def preferences(self, widget, terminal):
        """Display the preferences dialog.

        Arguments:
            * widget -- the widget that call this function
            * terminal -- the vte (for applying the changes)
        """
        CajaTerminalPref(self._set_terminal, terminal)

    def show(self, widget, window, hboxhidden, hboxterm, terminal, evresize):
        hboxhidden.hide()
        hboxterm.show()
        evresize.show()
        window.set_focus(terminal)
        window.nt_termhidden = False

    def hide(self, widget, window, hboxhidden, hboxterm, evresize):
        hboxhidden.show()
        hboxterm.hide()
        evresize.hide()
        window.nt_termhidden = True

    def on_terminal_destroy(self, widget, window):
        if window.nt_lastpid > 0:
            try:
                os.kill(window.nt_lastpid, signal.SIGKILL)
            except:
                pass
            finally:
                window.nt_lastpid = -1

    def on_terminal_child_exited(self, widget, terminal):
        terminal.has_child = False

    def on_terminal_commit(self, widget, char, size, window, rwidget, path):
        if size != 1:
            return
        if ord(char) == 0x0D: #0x0D = Enter
            if re.match(r"^\s*exit\s*[0-9]*\s*", widget.current_command.lower()):
                #Restart the shell on "exit" command
                if not widget.has_child:
                    window.nt_lastpid = -1
                    self.fork_cmd(window, widget, rwidget, path)
            widget.current_command = ""
        elif ord(char) == 0x7F: #0x7F = Backspace
            widget.current_command = widget.current_command[:-1]
        else:
            widget.current_command += char

    def on_terminal_key_press_event(self, widget, event, window, rwidget, path):
        if widget.ctrl_pressed:
            if event.keyval == 99: #99 = c
                widget.current_command = ""
            elif event.keyval == 100 or event.keyval == 68: #100 = d, 68 = D
                widget.has_child = False
                window.nt_lastpid = -1
                self.fork_cmd(window, widget, rwidget, path)
        else:
            if event.keyval == 65507 or event.keyval == 65508: #65507 = CTRL_L, 65508 = CTRL_R
                widget.ctrl_pressed = True

    def on_terminal_key_release_event(self, widget, event):
        if event.keyval == 65507 or event.keyval == 65508: #65507 = CTRL_L, 65508 = CTRL_R
            widget.ctrl_pressed = False

    def on_terminal_need_child(self, widget, event, window, terminal, rwidget, path):
        if not terminal.has_child:
            if window.nt_lastpid > 0:
                try:
                    os.kill(window.nt_lastpid, signal.SIGKILL)
                except:
                    pass
                window.nt_lastpid = -1
            self.fork_cmd(window, terminal, rwidget, path)

    def on_terminal_drag_motion(self, widget, event, x, y, time):
        event.drag_status(gtk.gdk.ACTION_COPY, time)
        return True

    def on_terminal_drag_drop(self, widget, event, x, y, time):
        event.finish(True, False, time)
        return True

    def on_terminal_drag_data_received(self, widget, context, x, y, selection, target_type, time):
       uri_list = selection.data.strip('\r\n\x00').split()
       for uri in uri_list:
           if uri[:7] == "file://": #local file
               path = urllib.url2pathname(uri[7:]).replace("'", "'\"'\"'")
               widget.feed_child(" '%s' " % path)

    def on_evResize_enter_notify_event(self, widget, event, rwidget):
        width, height = rwidget.get_size_request()
        rwidget.set_size_request(width, height)
        cursor = gtk.gdk.Cursor(gtk.gdk.SB_V_DOUBLE_ARROW)
        widget.window.set_cursor(cursor)

    def on_evResize_leave_notify_event(self, widget, event):
        cursor = gtk.gdk.Cursor(gtk.gdk.ARROW)
        widget.window.set_cursor(cursor)

    def on_evResize_motion_notify_event(self, widget, event, rwidget, term, window):
        width, height = rwidget.get_size_request()
        char_height = term.get_char_height()
        if event.y <= (0 - char_height):
            rwidget.set_size_request(width, int(height - char_height))
            window.nt_termheight = int(height - char_height)
        elif event.y >= char_height and height < 400:
            rwidget.set_size_request(width, int(height + char_height))
            window.nt_termheight = int(height + char_height)

    def _set_terminal(self, terminal):
        """Set terminal font, colors, palette,...
        Arguments:
            * terminal -- the vte
        """
        #General
        terminal.set_cursor_shape(CONF['general_cursor'])
        #Colors
        if CONF['color_palettename'] in PREDEF_PALETTE:
            colors = PREDEF_PALETTE[CONF['color_palettename']]
        elif CONF['color_palettename'] == "Custom":
            colors = CONF['color_palette']
        else:
            colors = PREDEF_PALETTE['Tango']
        fg = gtk.gdk.Color(CONF['color_text'])
        bg = gtk.gdk.Color(CONF['color_background'])
        palette = []
        for color in colors:
            palette.append(gtk.gdk.Color(color))
        terminal.set_colors(fg, bg, palette)
        #Font
        font = pango.FontDescription(CONF['font_name'])
        terminal.set_font(font)
        terminal.set_allow_bold(CONF['font_allowbold'])


def match_path(path, path_list):
    """Test if a folder is a sub-folder of another one in the list.

    Arguments
      * path -- path to check
      * path_list -- list of path
    """
    match = False
    #We add a slash at the end.
    if path[-1:] != "/":
        path += "/"
    for entry in path_list:
        #We add a slash at the end.
        if entry[-1:] != "/":
            entry += "/"
        if re.match(r"^" + entry + ".*", path):
            match = True
            break
    return match


CONF = Conf(CONF_FILE)


