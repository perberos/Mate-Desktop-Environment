pkgname=python-mate
pkgver=2011.08.31
pkgrel=1
pkgdesc="Python binding for Mate"
arch=('i686' 'x86_64')
url="http://matsusoft.com.ar/projects/"
license="GPL"
depends=('python2' 'python-corba' 'libmate' 'libmateui' 'libmatecanvas' 'libmatecomponent' 'mate-conf' 'mate-vfs' 'libmatecomponentui')
makedepends=('pkgconfig')
options=('!emptydirs')
groups=('mate-extras')
source=(http://sourceforge.net/projects/matede/files/${pkgver}/${pkgname}.tar.gz/download)
sha256sums=('54aee0eb6dea1695762310073a9f64d683ffd211f2be0569e79cb982e000ef58')

build() {
	export PYTHON=python2
	cd "${srcdir}/${pkgname}"

	./configure --prefix=/usr --sysconfdir=/etc \
		--localstatedir=/var --disable-static \
		--enable-mate \
		--enable-mateui \
		--enable-matecanvas \
		--enable-matevfs \
		--enable-matevfsmatecomponent \
		--enable-pyvfsmodule \
		--enable-mateconf \
		--enable-matecomponent_activation \
		--enable-matecomponent \
		--enable-matecomponentui || return 1
	make || return 1
}

package() {
	cd "${srcdir}/${pkgname}"

	make DESTDIR="$pkgdir" install || return 1
}
