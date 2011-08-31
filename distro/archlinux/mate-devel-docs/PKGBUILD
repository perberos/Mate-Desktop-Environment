pkgname=mate-devel-docs
pkgver=2011.08.31
pkgrel=1
pkgdesc="Developer documentation for MATE"
arch=('any')
license=('GPL' 'LGPL')
depends=('libxslt' 'python2' 'docbook-xml' 'rarian')
makedepends=('pkgconfig' 'intltool')
url="http://matsusoft.com.ar/projects"
source=(http://sourceforge.net/projects/matede/files/${pkgver}/${pkgname}.tar.gz/download)
sha256sums=('484d28be97e17d8f8eca0c11d2817f3ff7dfcb29a17f2f149c5fc2ed388b34f3')
groups=('mate')

build() {
	cd "${srcdir}/${pkgname}"
	PYTHON=/usr/bin/python2 ./configure --prefix=/usr --sysconfdir=/etc \
		--mandir=/usr/share/man --localstatedir=/var --disable-scrollkeeper
	make
}

package() {
	cd "${srcdir}/${pkgname}"
    make DESTDIR="$pkgdir" install || return 1
}
