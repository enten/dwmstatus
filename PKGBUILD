pkgname=dwmstatus
pkgver=20160106
pkgrel=1
pkgdesc='A simple status bar for DWM.'

arch=('i686' 'x86_64')
license=('MIT')
url="http://st.suckless.org"
source=(git://github.com/enten/$pkgname)
sha256sums=('SKIP')

build() {
	cd $srcdir/$pkgname
	make X11INC=/usr/include/X11 X11LIB=/usr/lib/X11
}

package() {
	cd $srcdir/$pkgname
    make PREFIX=/usr DESTDIR=$pkgdir install
}
