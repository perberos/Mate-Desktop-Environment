#!/bin/bash

#Installation script for Caja Terminal

_install() {
	#Install Caja Terminal
	#$1 : the output path, if different of /
	echo "$_BOLD * Copy file...$_NORMAL"
	#Share
	mkdir -pv "$1"/usr/share/caja-terminal/
	cp -v ./pixmaps/* "$1"/usr/share/caja-terminal/
	cp -v ./code/*.glade "$1"/usr/share/caja-terminal/
	#Caja Python
	mkdir -pv "$1"/usr/lib/caja/extensions-2.0/python/
	cp -pv ./code/caja-terminal.py "$1"/usr/lib/caja/extensions-2.0/python/
	chmod -v 755 "$1"/usr/lib/caja/extensions-2.0/python/caja-terminal.py
	#Doc
	mkdir -pv "$1"/usr/share/doc/caja-terminal/
	cp -v README "$1"/usr/share/doc/caja-terminal/
	cp -v AUTHORS "$1"/usr/share/doc/caja-terminal/
	#locales
	echo " $_BOLD* Build and install translations...$_NORMAL"
	for file in `find ./locales -name "*.po"` ; do {
		echo -n "   * Building $file .................. "
		l10elang=`echo $file | sed -r 's#./locales/(.*).po#\1#g'`
		mkdir -pv "$1"/usr/share/locale/$l10elang/LC_MESSAGES/
		msgfmt "$file" -o "$1"/usr/share/locale/$l10elang/LC_MESSAGES/caja-terminal.mo \
			&& echo -e "$_GREEN[Ok]$_NORMAL" || echo -e "$_RED[Fail]$_NORMAL"
	} done
	#Copy this script for uninstalling, (not for --package)
	if [ "$1" == "" ] ; then {
		cp -v ./install.sh /usr/share/caja-terminal/
	} fi
}


_remove() {
	#Remove Caja Terminal
	rm -rv /usr/share/caja-terminal
	rm -v /usr/lib/caja/extensions-2.0/python/caja-terminal.py
	rm -rv /usr/share/doc/caja-terminal
	find /usr/share/locale/ -name caja-terminal.mo \
		-exec rm -v '{}' ';'
}


_locale() {
	#Make the transtation template
	mkdir -pv ./locales/
	xgettext -k_ -kN_ -o ./locales/caja-terminal.pot ./code/*.py ./code/*.glade
	for lcfile in $(find ./locales/ -name "*.po") ; do {
		echo -n "   * $lcfile"
		msgmerge --update $lcfile ./locales/caja-terminal.pot
	} done
}


_check_dep() {
	#Checks dependencies

	echo "$_BOLD * Checking dependencies...$_NORMAL"
	echo -n "   * Caja ............................ "
	test -x /usr/bin/caja && echo "$_GREEN[OK]$_NORMAL" || { echo "$_RED[Missing]$_NORMAL" ; error=1 ; }
	echo -n "   * Caja Python...................... "
	test -f /usr/lib/caja/extensions-2.0/libcaja-python.so && echo "$_GREEN[OK]$_NORMAL" || { echo "$_RED[Missing]$_NORMAL" ; }
	echo -n "   * Python .............................. "
	test -x /usr/bin/python && echo "$_GREEN[OK]$_NORMAL" || { echo "$_RED[Missing]$_NORMAL" ; error=1 ; }
	echo -n "   * Python GTK Bindings (PyGTK) ......... "
	python <<< "import gtk, pygtk" 1> /dev/null 2> /dev/null && echo "$_GREEN[OK]$_NORMAL" || { echo "$_RED[Missing]$_NORMAL" ; error=1 ; }
	echo -n "   * Python VTE .......................... "
	python <<< "import vte" 1> /dev/null 2> /dev/null && echo "$_GREEN[OK]$_NORMAL" || { echo "$_RED[Missing]$_NORMAL" ; error=1 ; }
	echo -n "   * PyXDG ............................... "
	python <<< "import xdg" 1> /dev/null 2> /dev/null && echo "$_GREEN[OK]$_NORMAL" || { echo "$_RED[Missing]$_NORMAL" ; error=1 ; }
	echo -n "   * Python gettext support .............. "
	python <<< "import gettext" 1> /dev/null 2> /dev/null && echo "$_GREEN[OK]$_NORMAL" || { echo "$_RED[Missing]$_NORMAL" ; error=1 ; }
	if [ "$error" == "1" ] ; then {
		echo "$_RED   E:$_NORMAL Some dependencies are missing."
		exit 30
	} fi
}


_check_build_dep() {
	#Checks build dependencies

	echo "$_BOLD * Checking build dependencies...$_NORMAL"
	echo -n "   * gettext ............................. "
	test -x /usr/bin/xgettext && echo "$_GREEN[OK]$_NORMAL" || { echo "$_RED[Missing]$_NORMAL" ; error=1 ; }
	if [ "$error" == "1" ] ; then {
		echo "$_RED   E:$_NORMAL Some build dependencies are missing."
		exit 31
	} fi
}


#Force english
export LANG=c
#Go to the scrip directory
cd "${0%/*}" 1> /dev/null 2> /dev/null

#Head text
echo "Caja Terminal - An integrated terminal for the Caja file browser."
echo

#Action do to
if [ "$1" == "--install" ] || [ "$1" == "-i" ] ; then {
	echo "Installing Caja Terminal..."
	_RED=$(echo -e "\033[1;31m")
	_GREEN=$(echo -e "\033[1;32m")
	_NORMAL=$(echo -e "\033[0m")
	_BOLD=$(echo -e "\033[1m")
	if [ "$(whoami)" == "root" ] ; then {
		_check_build_dep
		_check_dep
		_install
	} else {
		echo "E: Need to be root"
		exit 1
	} fi
} elif [ "$1" == "--package" ] || [ "$1" == "-p" ] ; then {
	echo "Packaging Caja Terminal..."
	if [ -d "$2" ] ; then {
		_check_build_dep
		_install "$2"
	} else {
		echo "E: '$2' is not a directory"
		exit 2
	} fi
} elif [ "$1" == "--remove" ] || [ "$1" == "-r" ] ; then {
	echo "Removing Caja Terminal..."
	if [ "$(whoami)" == "root" ] ; then {
		_remove
	} else {
		echo "E: Need to be root"
		exit 1
	} fi
} elif [ "$1" == "--locale" ] || [ "$1" == "-l" ] ; then {
	echo "Extracting strings for the translation template..."
	if [ -x /usr/bin/xgettext ] ; then {
		_locale
	} else {
		echo "E: Can't find xgettext... gettex is installed ?"
		exit 3
	} fi
} else {
	echo "Arguments :"
	echo "  --install : install Caja Terminal on your computer."
	echo "  --package <path> : install Caja Terminal in <path> (Useful for packaging)."
	echo "  --remove : remove Caja Terminal from your computer."
	echo "  --locale : extract strings to for the translation template."
} fi

exit 0


