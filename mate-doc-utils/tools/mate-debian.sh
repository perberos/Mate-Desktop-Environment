#!/bin/sh
#
# Este programa lo he escrito con la intension de hacerme la vida un poco más
# facil a la hora de crear los paquetes debian. Y con un toque de PKGBUILD de
# archlinux.
#
# Caracteristicas faltantes:
# falta una forma de saber si se cumplen todos los requisitos
# poder instalar los paquetes para construir
#
# Copyright 2011 German Perugorria <perberos@gmail.com>
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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.

if [ ! $pkgname ]; then
	echo "You're Doing It Wrong!"
	exit 1
fi

# do not edit this!
srcdir=`pwd`
pkgdir=${srcdir}/pkg
pkgsrc=${srcdir}/src

# estas 4 funciones por lo general no necesitan editarse
pack() {
	# volvemos a la carpeta de inicio
	cd $srcdir
	# por ultimo empaqueteamos todo el contenido
	dpkg -b ${pkgdir} ${pkgname}_${pkgver}-${pkgrel}_${pkgarch}.deb
}

init() {
	

	if [ ! $pkgarch ]; then
		pkgarch=`arch`

		if [[ $pkgarch = "i686" ]]; then pkgarch=i386; fi
		if [[ $pkgarch = "x86_64" ]]; then pkgarch=amd64; fi
	fi
	# verificamos la existencia de las dependencias
	missing=""
	if [ $depends ]; then
		for index in $(seq 0 $((${#depends[@]} - 1))); do
			if [ ! -f "/var/lib/dpkg/info/${depends[index]}:${pkgarch}.list" ]; then
			if [ ! -f "/var/lib/dpkg/info/${depends[index]}.list" ]; then
				if [ "$missing" != "" ]; then
					missing="$missing "
				fi
				missing="$missing ${depends[index]}"
			fi
			fi
		done
	fi

	if [ $makedepends ]; then
		for index in $(seq 0 $((${#makedepends[@]} - 1))); do
			if [ ! -f "/var/lib/dpkg/info/${makedepends[index]}:${pkgarch}.list" ]; then
			if [ ! -f "/var/lib/dpkg/info/${makedepends[index]}.list" ]; then
				if [ -f "$missing" != "" ]; then
					missing="$missing "
				fi
				missing="$missing ${makedepends[index]}"
			fi
			fi
		done
	fi

	if [ $missing != "" ]; then
		echo "You need install:${missing}"
		return 1
	fi
	# /var/lib/dpkg/info/*.list



	# creamos la carpeta donde se compila y guarda el compilado
	if [ -d $pkgdir/ ]; then
		rm -rf $pkgdir/
	fi

	if [ ! -d $pkgdir ]; then
		mkdir $pkgdir
	fi

	if [ ! -d $pkgsrc ]; then
		mkdir $pkgsrc
	fi

	if [ -d $srcdir/${pkgname}/ ]; then
		rm -rf $srcdir/${pkgname}/
	fi

	if [ -d $pkgdir/DEBIAN ]; then
		rm -R $pkgdir/DEBIAN
	fi
}

end() {
	pkgsize=`du -s ${pkgdir} | awk -F\  '{print $1}'`

	# aveces uno se olvida de agregar el mantenedor
	if [ ! $maintainer ]; then
		maintainer=`whoami`
	fi
	if [ ! $priority ]; then
		priority=optional
	fi
	if [ ! $section ]; then
		section=mate
	fi

	if [ ! $pkgarch ]; then
		pkgarch=`arch`

		if [[ $pkgarch = "i686" ]]; then pkgarch=i386; fi
		if [[ $pkgarch = "x86_64" ]]; then pkgarch=amd64; fi
	fi

	# se hace un MD5 de cada archivo listado
	FILES=$(find "$pkgdir/" -type f | sed "s|^${pkgdir}/||")

	# esta carpeta es importante para el paquete, contiene informacion del paquete
	if [ ! -d $pkgdir/DEBIAN ]; then
		mkdir $pkgdir/DEBIAN
	fi

	# archivo nuevo
	if [ -f $pkgdir/DEBIAN/md5sums ]; then
		rm $pkgdir/DEBIAN/md5sums
	fi

	for FILE in ${FILES}; do
		content=$(md5sum "$pkgdir/$FILE" | awk '{print $1;}')
		echo "$content $FILE" >> $pkgdir/DEBIAN/md5sums
	done

	# copiamos el archivo que contiene la información del paquete
	if [ -f $srcdir/postinst ]; then
		cp $srcdir/postinst $pkgdir/DEBIAN
	fi
	if [ -f $srcdir/postrm ]; then
		cp $srcdir/postrm $pkgdir/DEBIAN
	fi
	if [ -f $srcdir/preinst ]; then
		cp $srcdir/preinst $pkgdir/DEBIAN
	fi
	if [ -f $srcdir/prerm ]; then
		cp $srcdir/prerm $pkgdir/DEBIAN
	fi


	if [ -f $srcdir/control ]; then
		cp $srcdir/control $pkgdir/DEBIAN
	fi

	# esto es importante
	find $pkgdir -type d | xargs chmod 755

	#
	# creamos el archivo control
	#
	echo "Package: %NAME%" > $pkgdir/DEBIAN/control
	echo "Version: %VERSION%" >> $pkgdir/DEBIAN/control
	echo "Architecture: %ARCH%" >> $pkgdir/DEBIAN/control
	echo "Installed-Size: %SIZE%" >> $pkgdir/DEBIAN/control

	if [ $depends ]; then
		line=""
		for index in $(seq 0 $((${#depends[@]} - 1))); do
			if [ "$line" != "" ]; then
				line="$line,"
			fi
			line="$line ${depends[index]}"
		done
		echo "Depends: $line" >> $pkgdir/DEBIAN/control
	fi

	#if [ $makedepends ]; then
	#	line=""
	#	for index in $(seq 0 $((${#makedepends[@]} - 1))); do
	#		if [ "$line" != "" ]; then
	#			line="$line,"
	#		fi
	#		line="$line ${makedepends[index]}"
	#	done
	#	echo "Build-Depends: $line" >> $pkgdir/DEBIAN/control
	#fi

	if [ $conflicts ]; then
		line=""
		for index in $(seq 0 $((${#conflicts[@]} - 1))); do
			if [ "$line" != "" ]; then
				line="$line,"
			fi
			line="$line ${conflicts[index]}"
		done
		echo "Conflicts: $line" >> $pkgdir/DEBIAN/control
	fi

	echo "Section: %SECTION%" >> $pkgdir/DEBIAN/control
	echo "Maintainer: %MAINTAINER%" >> $pkgdir/DEBIAN/control
	echo "Priority: %PRIORITY%" >> $pkgdir/DEBIAN/control
	echo "Description: %DESCRIPTION%" >> $pkgdir/DEBIAN/control
	echo "  %DESCRIPTION%" >> $pkgdir/DEBIAN/control

	# reemplazamos algunas variables dentro del archivo
	sed -i "s/%NAME%/${pkgname}/" $pkgdir/DEBIAN/control
	sed -i "s/%ARCH%/${pkgarch}/" $pkgdir/DEBIAN/control
	sed -i "s/%VERSION%/${pkgver}/" $pkgdir/DEBIAN/control
	sed -i "s/%DESCRIPTION%/${pkgdesc}/" $pkgdir/DEBIAN/control
	sed -i "s/%SIZE%/${pkgsize}/" $pkgdir/DEBIAN/control
	#sed -i "s/%DEPENDS%/${depends}/" $pkgdir/DEBIAN/control
	sed -i "s/%SECTION%/${section}/" $pkgdir/DEBIAN/control
	sed -i "s/%MAINTAINER%/${maintainer}/" $pkgdir/DEBIAN/control
	sed -i "s/%PRIORITY%/${priority}/" $pkgdir/DEBIAN/control
}

download() {
	cd $pkgsrc

	if [ ! -f $pkgsrc/download ]; then
		# descargar con la bandera resume
		wget -c http://sourceforge.net/projects/matede/files/${pkgver}/${pkgname}.tar.gz/download
	fi

	cd $srcdir
}

#construimos el paquete
init || return 1
download || return 1
build || return 1
end || return 1
pack || return 1
