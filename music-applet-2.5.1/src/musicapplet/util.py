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
A collection of utility methods that don't quite fit anywhere else.
"""


def search_model (model, column, value):
    """
    Search a TreeModel for the first row with a particular value in one
    column.
    """

    iter = model.get_iter_first ()
    while iter is not None:
        if model.get_value (iter, column) == value:
            return iter
        iter = model.iter_next (iter)
    return None


def make_property (name):
    """
    Create a Python property that wraps a GObject property.

    Note that the setter first checks whether the value is actually changing,
    to suppress uninteresting notifications of a property change.
    """

    def getter (self):
        return self.get_property (name)
    def setter (self, value):
        if value != self.get_property (name):
            self.set_property (name, value)

    return property (getter, setter)
