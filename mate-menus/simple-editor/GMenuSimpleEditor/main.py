#
# Copyright (C) 2005 Red Hat, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

def main (args):
    import locale
    import gettext

    import pygtk; pygtk.require('2.0');

    import gobject
    from gobject.option import OptionParser, make_option

    import gtk

    import maindialog
    import config

    locale.setlocale (locale.LC_ALL, "")
    gettext.install (config.PACKAGE, config.LOCALEDIR)

    parser = OptionParser (
            option_list = [
		    # FIXME: remove this when we can get all the default
		    # options
                    make_option ("--version",
                                 action="store_true",
                                 dest="version",
                                 help=config.VERSION),
                ])
    parser.parse_args (args)

    if parser.values.version:
        # Translators: %s is the version number 
        print _("Simple Menu Editor %s") % (config.VERSION)
    else:
        dialog = maindialog.MenuEditorDialog (args)
        gtk.main ()
