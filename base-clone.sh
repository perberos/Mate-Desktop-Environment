#!/bin/sh
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
    perberos/libmate
    perberos/libmatecanvas
    perberos/libmatecomponent
    perberos/libmatecomponentui
    perberos/libmatekbd
    perberos/libmatekeyring
    perberos/libmatenotify
    perberos/libmateui
    perberos/libmateweather
    perberos/mate-backgrounds
    perberos/mate-common
    perberos/mate-conf
    perberos/mate-control-center
    perberos/mate-corba
    perberos/mate-desktop
    perberos/mate-dialogs
    perberos/mate-doc-utils
    perberos/mate-file-manager
    perberos/mate-icon-theme
    perberos/mate-keyring
    perberos/mate-menus
    perberos/mate-mime-data
    perberos/mate-notification-daemon
    perberos/mate-panel
    perberos/mate-polkit
    perberos/mate-session-manager
    perberos/mate-settings-daemon
    perberos/mate-vfs
    perberos/mate-window-manager
)

for i in $(seq 0 $((${#listofpackages[@]} - 1))); do
	repo=${listofpackages[$i]}
	package=${name:`expr index "$repo" /`}

	if [ -d $folder ]; then
		git clone git@github.com:$repo.git
	else
		echo Skip $folder.
	fi
done

