pkgname=mate-cursor
pkgver=2011.08.31
pkgrel=1
pkgdesc="Cursor for MATE"
arch=('any')
license=('CCPL:by-sa')
url="http://matsusoft.com.ar/projects"
makedepends=()
groups=('mate')
source=(http://sourceforge.net/projects/matede/files/${pkgver}/${pkgname}.tar.gz/download)
sha256sums=('f56ea445859a3ba15bf6c1c648daecae4b850e463d18380f6a4310fc47e9b6a1')

build() {
	cd "${srcdir}/${pkgname}"
	./configure --prefix=/usr || return 1

	make || return 1
}

package() {
    cd "$srcdir/$pkgname"

    make DESTDIR="$pkgdir" install || return 1
}
