#!/bin/sh

usage() {
cat << _EOF
Usage:
  mateconfpkg [OPTION] [PACKAGE] 

  Help Options:
    -?, --help            Show help options

  Application Options:
    --install             Install schemas for a given package
    --uninstall           Uninstall schemas for a given package

_EOF
}

install() {
	MATECONF_CONFIG_SOURCE=`/usr/bin/mateconftool-2 --get-default-source` \
		/usr/bin/mateconftool-2 --makefile-install-rule /usr/share/mateconf/schemas/${pkgname}.schemas >/dev/null
}

uninstall() {
	if [ -f /usr/share/mateconf/schemas/${pkgname}.schemas ]; then
		schemas=/usr/share/mateconf/schemas/${pkgname}.schemas
	#elif [ -f /opt/gnome/share/mateconf/schemas/${pkgname}.schemas ]; then
	#	schemas=/opt/gnome/share/mateconf/schemas/${pkgname}.schemas
	else
		schemas=`pacman -Ql ${pkgname} | grep 'mateconf/schemas/.*schemas$' | awk '{ print $2 }'`
	fi
	MATECONF_CONFIG_SOURCE=`/usr/bin/mateconftool-2 --get-default-source` \
		/usr/bin/mateconftool-2 --makefile-uninstall-rule ${schemas} >/dev/null
}

if [ -z "$2" ]; then
	usage
else
	pkgname="$2"
	case "$1" in
		--install)
			install
		;;
		--uninstall)
			uninstall
		;;
		*)
			usage
		;;
	esac
fi
