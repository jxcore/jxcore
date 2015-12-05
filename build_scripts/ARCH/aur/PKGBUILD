# Maintainer: Marc McIntosh <email@marcmcintosh.cninja>
pkgname=jxcore
pkgver=v0.3.0.6.r198.gfa2dd55
pkgrel=1
pkgdesc="Evented IO for SpiderMonkey and V8 JavaScript"
arch=('i686' 'x86_64')
url="http://jxcore.io/"
license=('common')
groups=()
depends=('openssl' 'zlib')
makedepends=('git' 'python2')
optdepends=('npm')
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=('jxcore.install')
source=('git+https://github.com/jxcore/jxcore')
noextract=()
md5sums=('SKIP')
pkgver() {
	cd "$pkgname"
	git describe --long --tags | sed 's/\([^-]*-g\)/r\1/;s/-/./g'
}
build() {
  cd "$srcdir/${pkgname}"
  msg 'Fixing for python2 name'
  find -type f -exec sed \
    -e 's_^#!/usr/bin/env python$_&2_' \
    -e 's_^\(#!/usr/bin/python2\).[45]$_\1_' \
    -e 's_^#!/usr/bin/python$_&2_' \
    -e "s_'python'_'python2'_" -i {} \;
  find test/ -type f -exec sed 's_python _python2 _' -i {} \;
  export PYTHON=python2
  ./configure \
    --prefix=/usr \
    --engine-mozilla \
    --shared-openssl \
    --shared-zlib \
    --without-npm
  make

}

package() {
  cd "$srcdir/${pkgname}"
  make DESTDIR="$pkgdir" install

  # install docs as per user request
  install -d "$pkgdir"/usr/share/doc/jxcore
  cp -r doc/api \
    "$pkgdir"/usr/share/doc/jxcore

  install -D -m644 JXCORE_LICENSE \
    "$pkgdir"/usr/share/licenses/jxcore/LICENSE
}

# vim:set ts=2 sw=2 et:
