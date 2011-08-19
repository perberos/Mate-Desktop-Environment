#! /bin/sh
# Copyright (C) 2006-2007 Richard Hughes <richard@hughsie.com>
#
# Licensed under the GNU General Public License Version 2
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

#$1 = keyname
print_hal_key ()
{
	udi="/org/freedesktop/Hal/devices/computer"
	ret=`hal-get-property --udi $udi --key $1 2> /dev/null`
	if [ $? -eq 0 ]; then
		echo $ret
	else
		echo "missing"
	fi 
}

#$1 = capability
print_hal_capability ()
{
	ret=`hal-find-by-capability --capability $1`
	if [ -n "$ret" ]; then
		echo "yes"
	else
		echo "no"
	fi
}

echo -n "Distro version:       "
cat /etc/*release | uniq

echo -n "Kernel version:       "
uname -r

echo -n "g-p-m version:        "
mate-power-manager --version | cut -f2 -d" "

echo -n "HAL version:          "
lshal -V | cut -f3 -d" "

echo -n "System manufacturer:  "
print_hal_key "smbios.system.manufacturer"
echo -n "System version:       "
print_hal_key "smbios.system.version"
echo -n "System product:       "
print_hal_key "smbios.system.product"

echo -n "AC adapter present:   "
print_hal_capability "ac_adapter"

echo -n "Battery present:      "
print_hal_capability "battery"

echo -n "Laptop panel present: "
print_hal_capability "laptop_panel"

echo -n "CPU scaling present:  "
print_hal_capability "cpufreq_control"

echo "Battery Information:"
lshal | grep "battery\."

OS=`uname -s`

echo "UPower data:"
upower --dump

echo "MATE Power Manager Process Information:"
if [ "$OS" = "SunOS" ]; then
	ptree -a `pgrep power`
else
	ps aux --forest | grep mate-power | grep -v grep
fi

echo "HAL Process Information:"
if [ "$OS" = "SunOS" ]; then
        ptree -a `pgrep hald`
else
	ps aux --forest | grep hald | grep -v grep
fi
