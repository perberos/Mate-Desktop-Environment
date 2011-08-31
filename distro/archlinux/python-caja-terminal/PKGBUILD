pkgname=python-caja-terminal
pkgver=2011.08.31
pkgrel=1
pkgdesc="An integrated terminal for Mate File Manager"
arch=('any')
url="http://software.flogisoft.com/nautilus-terminal/"
license=('GPL')
groups=('mate-extras')
depends=('python-caja' 'vte' 'pyxdg')
source=(http://sourceforge.net/projects/matede/files/${pkgver}/${pkgname}.tar.gz/download)
sha256sums=('03c0217cae536204c4d51a001c077e4063ab8da7c17b9137befab4130a44c53a')

build() {
	cd "$srcdir/$pkgname"

	sed -i 's|^#!/usr/bin/python$|#!/usr/bin/python2|' code/caja-terminal.py
}

package() {
	cd "$srcdir/$pkgname"

	./install.sh --package "$pkgdir"
}
