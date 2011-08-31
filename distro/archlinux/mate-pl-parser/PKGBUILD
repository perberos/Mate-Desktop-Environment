pkgname=mate-pl-parser
pkgver=2011.08.31
pkgrel=1
pkgdesc="Totem playlist parser library"
arch=(i686 x86_64)
license=('LGPL')
depends=('gmime' 'libsoup-gnome')
makedepends=('intltool' 'pkgconfig' 'gobject-introspection')
options=('!emptydirs')
url="http://matsusoft.com.ar/projects"
groups=('mate-extras')
source=(http://sourceforge.net/projects/matede/files/${pkgver}/${pkgname}.tar.gz/download)
sha256sums=('6d0a1171e47e215376cab383cdda4b48bb70b446b63146cda631d7d74f0e9305')

build() {
	cd "${srcdir}/${pkgname}"

	./configure --prefix=/usr --sysconfdir=/etc \
		--libexecdir=/usr/lib/${pkgname} \
		--localstatedir=/var --disable-static

	make
}

package() {
	cd "${srcdir}/${pkgname}"

	make DESTDIR="${pkgdir}" install
}
