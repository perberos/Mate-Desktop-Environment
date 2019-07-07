#!/bin/bash
# Copyright © 2011 Perberos
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by the
# Free Software Foundation; either version 2.1 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
# General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

listofpackages=(
    mate-desktop/atril
    mate-desktop/caja
    mate-desktop/caja
    mate-desktop/caja-dropbox
    mate-desktop/caja-extensions
    mate-desktop/engrampa
    mate-desktop/eom
    mate-desktop/libmatekbd
    mate-desktop/libmatemixer
    mate-desktop/libmateweather
    mate-desktop/marco
    mate-desktop/mate-applets
    mate-desktop/mate-backgrounds
    mate-desktop/mate-calc
    mate-desktop/mate-desktop
    mate-desktop/mate-desktop
    mate-desktop/mate-icon-theme
    mate-desktop/mate-icon-theme
    mate-desktop/mate-menus
    mate-desktop/mate-notification-daemon
    mate-desktop/mate-panel
    mate-desktop/mate-screensaver
    mate-desktop/mate-sensors-applet
    mate-desktop/mate-session-manager
    mate-desktop/mate-settings-daemon
    mate-desktop/mate-terminal
    mate-desktop/mate-user-guide
    mate-desktop/mate-utils
    mate-desktop/mozo
    mate-desktop/pluma
    mate-desktop/python-caja
)

for i in $(seq 0 $((${#listofpackages[@]} - 1))); do
	repo=${listofpackages[$i]}
	package=${name:`expr index "$repo" /`}

	if [ -d $folder ]; then
		git clone https://github.com/$repo.git
	else
		echo Skip $folder.
	fi
done

