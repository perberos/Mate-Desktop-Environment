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
    mate-desktop/libmate
    mate-desktop/libmatecanvas
    mate-desktop/libmatecomponent
    mate-desktop/libmatecomponentui
    mate-desktop/libmatekbd
    mate-desktop/libmatekeyring
    mate-desktop/libmatenotify
    mate-desktop/libmateui
    mate-desktop/libmateweather
    mate-desktop/libmatewnck
    mate-desktop/mate-backgrounds
    mate-desktop/mate-common
    mate-desktop/mate-conf
    mate-desktop/mate-control-center
    mate-desktop/mate-corba
    mate-desktop/mate-desktop
    mate-desktop/mate-dialogs
    mate-desktop/mate-doc-utils
    mate-desktop/mate-file-manager
    mate-desktop/mate-icon-theme
    mate-desktop/mate-keyring
    mate-desktop/mate-menus
    mate-desktop/mate-mime-data
    mate-desktop/mate-notification-daemon
    mate-desktop/mate-panel
    mate-desktop/mate-polkit
    mate-desktop/mate-session-manager
    mate-desktop/mate-settings-daemon
    mate-desktop/mate-vfs
    mate-desktop/mate-window-manager
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

