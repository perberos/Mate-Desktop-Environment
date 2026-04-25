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
    libmate
    libmatecanvas
    libmatecomponent
    libmatecomponentui
    libmatekbd
    libmatekeyring
    libmatenotify
    libmateui
    libmateweather
    libmatewnck
    mate-backgrounds
    mate-common
    mate-conf
    mate-control-center
    mate-corba
    mate-desktop
    mate-dialogs
    mate-doc-utils
    mate-file-manager
    mate-icon-theme
    mate-keyring
    mate-menus
    mate-mime-data
    mate-notification-daemon
    mate-panel
    mate-polkit
    mate-session-manager
    mate-settings-daemon
    mate-vfs
    marco
)

for i in $(seq 0 $((${#listofpackages[@]} - 1))); do
	name=${listofpackages[$i]}
	
	if [ -d $name ]; then
		echo Pull $name.
		cd $name
		git pull
		cd ..
	fi
done

