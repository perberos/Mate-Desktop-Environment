pkgname=mate-user-docs
pkgver=2011.08.31
pkgrel=1
pkgdesc="User documentation for MATE"
arch=('any')
license=('FDL')
depends=()
makedepends=('mate-doc-utils')
url="http://matsusoft.com.ar/projects"
groups=('mate-extras')
source=(http://sourceforge.net/projects/matede/files/${pkgver}/${pkgname}.tar.gz/download)
sha256sums=('8a34bb2bfaacd1870ffb2b03991251da541cf16de2f047624a8b31bb3fd81e5c')

build() {
	cd "${srcdir}/${pkgname}"

	./configure --prefix=/usr --sysconfdir=/etc \
		--localstatedir=/var --disable-scrollkeeper || return 1

	make || return 1
}

package() {
	cd "${srcdir}/${pkgname}"
	make DESTDIR="${pkgdir}" install || return 1
}
