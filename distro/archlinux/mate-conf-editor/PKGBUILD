pkgname=mate-conf-editor
pkgver=2011.08.31
pkgrel=1
pkgdesc="A configuration database system editor"
arch=(i686 x86_64)
license=('LGPL')
depends=('mate-corba' 'libxml2' 'polkit' 'libldap' 'dbus-glib' 'gtk2' 'mate-conf')
makedepends=('pkgconfig' 'intltool' 'gtk-doc' 'gobject-introspection')
groups=('mate-extras')
install=mate-conf.install
url="http://matsusoft.com.ar/projects"
source=(http://sourceforge.net/projects/matede/files/${pkgver}/${pkgname}.tar.gz/download)
sha256sums=('453ec686dc0da845df7039c2de088ec7eda38de61bfb1e0786e88fab654cfd54')

build() {
	cd "${srcdir}/${pkgname}"

	./configure --prefix=/usr --sysconfdir=/etc \
		--libexecdir=/usr/lib/MateConf \
		--localstatedir=/var || return 1

	make || return 1
}

package() {
	cd "${srcdir}/${pkgname}"

	make MATECONF_DISABLE_MAKEFILE_SCHEMA_INSTALL=1 DESTDIR="${pkgdir}" install || return 1

	install -m755 -d ${pkgdir}/usr/share/mateconf/schemas

	mateconf-merge-schema ${pkgdir}/usr/share/mateconf/schemas/${pkgname}.schemas --domain ${pkgname} ${pkgdir}/etc/mateconf/schemas/*.schemas || return 1
	rm -f ${pkgdir}/etc/mateconf/schemas/*.schemas
}
