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

import musicapplet.defs
import musicapplet.player


INFO = {
        "name": "VLC",
        "version": musicapplet.defs.VERSION,
        "icon-name": "vlc",
        "author": "Paul Kuliniewicz <paul@kuliniewicz.org>",
        "copyright": "(C) 2008 Paul Kuliniewicz",
        "website": "http://www.kuliniewicz.org/music-applet/",
}


class VLCPlugin (musicapplet.player.MPRISPlugin):
    """
    Music Applet plugin to interface with VLC.
    """

    def __init__ (self, conf):
        musicapplet.player.MPRISPlugin.__init__ (self, conf,
                                                 INFO["name"], INFO["icon-name"],
                                                 "org.mpris.vlc")


def create_instance (conf):
    return VLCPlugin (conf)
