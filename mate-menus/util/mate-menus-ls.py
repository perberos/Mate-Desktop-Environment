# vim: set ts=4 sw=4 et:

#
# Copyright (C) 2008 Novell, Inc.
#
# Authors: Vincent Untz <vuntz@mate.org>
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

import optparse
import sys

import gmenu

def print_entry(entry, path):
    if entry.get_is_excluded():
        excluded = ' <excluded>'
    else:
        excluded = ''

    print '%s\t%s\t%s%s' % (path, entry.get_desktop_file_id(), entry.get_desktop_file_path(), excluded)

def print_directory(dir, parent_path = None):
    if not parent_path:
        path = '/'
    else:
        path = '%s%s/' % (parent_path, dir.get_name())

    for item in dir.get_contents():
        type = item.get_type()
        if type == gmenu.TYPE_ENTRY:
            print_entry(item, path)
        elif type == gmenu.TYPE_DIRECTORY:
            print_directory(item, path)
        elif type == gmenu.TYPE_ALIAS:
            aliased = item.get_item()
            if aliased.get_type() == gmenu.TYPE_ENTRY:
                print_entry(aliased, path)
        elif type in [ gmenu.TYPE_HEADER, gmenu.TYPE_SEPARATOR ]:
            pass
        else:
            print >> sys.stderr, 'Unsupported item type: %s' % type

def main(args):
    parser = optparse.OptionParser()

    parser.add_option('-f', '--file', dest='file',
                      help='Menu file')
    parser.add_option('-i', '--include-excluded', dest='exclude',
                      action='store_true', default=False,
                      help='Include <Exclude>d entries')
    parser.add_option('-n', '--include-nodisplay', dest='nodisplay',
                      action='store_true', default=False,
                      help='Include NoDisplay=true entries')

    (options, args) = parser.parse_args()

    if options.file:
        menu_file = options.file
    else:
        menu_file = 'applications.menu'

    flags = gmenu.FLAGS_NONE
    if options.exclude:
        flags |= gmenu.FLAGS_INCLUDE_EXCLUDED
    if options.nodisplay:
        flags |= gmenu.FLAGS_INCLUDE_NODISPLAY

    tree = gmenu.lookup_tree(menu_file, flags)
    root = tree.get_root_directory()

    if not root:
        print 'Menu tree is empty'
    else:
        print_directory(root)

if __name__ == '__main__':
    try:
      main(sys.argv)
    except KeyboardInterrupt:
      pass
