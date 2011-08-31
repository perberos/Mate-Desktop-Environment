pkgname=python-caja
pkgver=2011.08.31
pkgrel=1
pkgdesc="Python binding for Cahja components"
arch=('i686' 'x86_64')
url="http://matsusoft.com.ar/projects/"
license="GPL"
groups=('mate-extras')
makedepends=('pkgconfig' 'python-mate' )
depends=('mate-file-manager' 'pygtk')
options=('!libtool' '!emptydirs')
source=(http://sourceforge.net/projects/matede/files/${pkgver}/${pkgname}.tar.gz/download)
sha256sums=('4c698543bebf1efd8ab097e47dd4f9ec72e492d3f482748086b271bfc1e36676')

build() {
	export PYTHON=python2
	cd "${srcdir}/${pkgname}"

	./configure --prefix=/usr --sysconfdir=/etc \
		--localstatedir=/var --disable-static || return 1
	make || return 1
}

package() {
	cd "${srcdir}/${pkgname}"

	make DESTDIR="$pkgdir" install || return 1
}
