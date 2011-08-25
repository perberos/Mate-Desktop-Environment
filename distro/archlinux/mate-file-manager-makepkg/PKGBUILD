# Maintainer: PirateJonno <j@skurvy.no-ip.org>

pkgname=mate-file-manager-makepkg
tarname=nautilus-makepkg
pkgver=2.30.0
pkgrel=1
pkgdesc='An AUR helper for the MATE file manager context menu'
arch=('i686' 'x86_64')
url='http://www.gnome.org/'
license=('GPL')
depends=('glib2' 'mate-conf' 'mate-desktop' 'mate-file-manager' 'pacman')
makedepends=('intltool')
install="${pkgname}.install"
options=('!libtool')
group=('mate-extra')
source=("http://cloud.github.com/downloads/PirateJonno/${tarname}/${tarname}-${pkgver}.tar.bz2")
sha256sums=('a2ad3428ea3126a1916206f0091f5a85ed83308a0fccfae39ec69d6b0bf96a7f')

build() {
	cd "${srcdir}/${tarname}-${pkgver}"

	# from https://bbs.archlinux.org/viewtopic.php?pid=962336#p962336
    DIR_LIST=`ls -c`" "$(for F in `ls -c src`;do echo "src/$F ";done;)
    for FILENAME in ${DIR_LIST}; do
        # check if its a folder
        if [ -f "${FILENAME}" ]; then
            echo "patching file $FILENAME"
            sed -i "s/panel-applet/mate-panel-applet/g" ${FILENAME}
            sed -i "s/panelapplet/matepanelapplet/g" ${FILENAME}
            sed -i "s/panel_applet/mate_panel_applet/g" ${FILENAME}
            sed -i "s/PANEL_APPLET/MATE_PANEL_APPLET/g" ${FILENAME}
            sed -i "s/PanelApplet/MatePanelApplet/g" ${FILENAME}

            sed -i "s/mate-mate-panel-applet/mate-panel-applet/g" ${FILENAME}
            sed -i "s/matematepanelapplet/matepanelapplet/g" ${FILENAME}
            sed -i "s/mate_mate_panel_applet/mate_panel_applet/g" ${FILENAME}
            sed -i "s/MATE_MATE_PANEL_APPLET/MATE_PANEL_APPLET/g" ${FILENAME}
            sed -i "s/MateMatePanelApplet/MatePanelApplet/g" ${FILENAME}

            sed -i "s/gnome/mate/g" ${FILENAME}
            sed -i "s/GNOME/MATE/g" ${FILENAME}
            sed -i "s/Gnome/Mate/g" ${FILENAME}

            sed -i "s/Metacity/Marco/g" ${FILENAME}
            sed -i "s/metacity/marco/g" ${FILENAME}
            sed -i "s/METACITY/MARCO/g" ${FILENAME}

            sed -i "s/Nautilus/Caja/g" ${FILENAME}
            sed -i "s/nautilus/caja/g" ${FILENAME}
            sed -i "s/NAUTILUS/CAJA/g" ${FILENAME}

            sed -i "s/Zenity/MateDialog/g" ${FILENAME}
            sed -i "s/zenity/matedialog/g" ${FILENAME}
            sed -i "s/ZENITY/MATEDIALOG/g" ${FILENAME}

            sed -i "s/MATE|Utilities/GNOME|Utilities/g" ${FILENAME}
            sed -i "s/MATE|Desktop/GNOME|Desktop/g" ${FILENAME}
            sed -i "s/MATE|Applets/GNOME|Applets/g" ${FILENAME}
            sed -i "s/MATE|Applications/GNOME|Applications/g" ${FILENAME}
            sed -i "s/MATE|Multimedia/GNOME|Multimedia/g" ${FILENAME}

            sed -i "s/libnotify/libmatenotify/g" ${FILENAME}
            sed -i "s/LIBNOTIFY/LIBMATENOTIFY/g" ${FILENAME}
            sed -i "s/Libnotify/Libmatenotify/g" ${FILENAME}

            sed -i "s/bonobo/matecomponent/g" ${FILENAME}
            sed -i "s/Bonobo/MateComponent/g" ${FILENAME}
            sed -i "s/BONOBO/MATECOMPONENT/g" ${FILENAME}
            sed -i "s/bonoboui/matecomponentui/g" ${FILENAME}
            sed -i "s/BONOBOUI/MATECOMPONENTUI/g" ${FILENAME}

            sed -i "s/gconf/mateconf/g" ${FILENAME}
            sed -i "s/GConf/MateConf/g" ${FILENAME}
            sed -i "s/GCONF/MATECONF/g" ${FILENAME}

            sed -i "s/pkmateconfig/pkgconfig/g" ${FILENAME}
            sed -i "s/PKMATECONFIG/PKGCONFIG/g" ${FILENAME}

            sed -i "s/gweather/mateweather/g" ${FILENAME}
            sed -i "s/GWeather/MateWeather/g" ${FILENAME}
            sed -i "s/GWEATHER/MATEWEATHER/g" ${FILENAME}

            sed -i "s/ORBit/MateCORBA/g" ${FILENAME}
            sed -i "s/orbit/matecorba/g" ${FILENAME}
            sed -i "s/ORBIT/MATECORBA/g" ${FILENAME}

            sed -i "s/panel-applet/mate-panel-applet/g" ${FILENAME}
            sed -i "s/panelapplet/matepanelapplet/g" ${FILENAME}
            sed -i "s/panel_applet/mate_panel_applet/g" ${FILENAME}
            sed -i "s/PANEL_APPLET/MATE_PANEL_APPLET/g" ${FILENAME}
            sed -i "s/PanelApplet/MatePanelApplet/g" ${FILENAME}

            sed -i "s/mate-mate-panel-applet/mate-panel-applet/g" ${FILENAME}
            sed -i "s/matematepanelapplet/matepanelapplet/g" ${FILENAME}
            sed -i "s/mate_mate_panel_applet/mate_panel_applet/g" ${FILENAME}
            sed -i "s/MATE_MATE_PANEL_APPLET/MATE_PANEL_APPLET/g" ${FILENAME}
            sed -i "s/MateMatePanelApplet/MatePanelApplet/g" ${FILENAME}

            sed -i "s/soup-mate/soup-gnome/g" ${FILENAME}
            sed -i "s/SOUP_TYPE_MATE_FEATURES_2_26/SOUP_TYPE_GNOME_FEATURES_2_26/g" ${FILENAME}
            sed -i "s/mateconfaudiosink/gconfaudiosink/g" ${FILENAME}
            sed -i "s/mateconfvideosink/gconfvideosink/g" ${FILENAME}

            sed -i "s/TAMATECONFIG/TAGCONFIG/g" ${FILENAME}
        fi
    done

    mv src/nautilus-extension.c src/caja-extension.c
    mv src/nautilus-makepkg.c src/caja-makepkg.c
    mv src/nautilus-makepkg.h src/caja-makepkg.h
    mv src/nautilus-makepkg.schemas.in src/caja-makepkg.schemas.in


	./configure --prefix=/usr \
		--sysconfdir=/etc \
		--localstatedir=/var || return 1

	make -s || return 1
	make MATECONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1 DESTDIR="${pkgdir}" install || return 1

	install -m 755 -d "${pkgdir}/usr/share/mateconf/schemas"
	mateconf-merge-schema "${pkgdir}/usr/share/mateconf/schemas/${pkgname}.schemas" "${pkgdir}"/etc/mateconf/schemas/*.schemas || return 1
	rm -rf "${pkgdir}/etc"
}
