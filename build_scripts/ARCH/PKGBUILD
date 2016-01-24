# Maintainer: Marc McIntosh <marc@marcmcintosh.ninja>
pkgname=jxcore
pkgver=v0.3.0.7.r347.g1638e28
pkgrel=1
pkgdesc="Evented IO for SpiderMonkey and V8 JavaScript"
arch=('arm' 'armv6h' 'armv7h' 'i686' 'x86_64')
url="http://jxcore.io/"
license=('common')
depends=()
makedepends=('python2')
optdepends=()
conflicts=()
options=()
install=
source=('git+https://github.com/jxcore/jxcore')
noextract=()
md5sums=('SKIP')
pkgver() {
  cd "$pkgname"
  git describe --long --tags | sed 's/\([^-]*-g\)/r\1/;s/-/./g'
}
set_flags() {
  
  if [ "$CARCH" == "arm" ]; then
    CONFIGFLAG="--with-arm-float-abi=soft --dest-cpu=arm"
    CXXFLAGS="$CXXFLAGS -march=armv5t -mno-unaligned-access"
    GYPFLAGS="-Darm_thumb -Darm_float_abi=soft -Darm_version=5 -Darm_fpu= -Darm_test_noprobe=on"
  fi

  if [ "$CARCH" == "armv6h" ]; then
    CONFIGFLAG="--with-arm-float-abi=hard --dest-cpu=arm"
    GYPFLAGS="-Darm_thumb -Darm_float_abi=hard -Darm_version=6 -Darm_fpu=vfpv2"
  fi

  if [ "$CARCH" == "armv7h" ]; then
    CONFIGFLAG="--with-arm-float-abi=hard --dest-cpu=arm"
    GYPFLAGS="-Darm_thumb -Darm_float_abi=hard -Darm_version=7 -Darm_fpu=vfpv3-d16"
  fi

  if [ "$CARCH" == "i686" ]; then
    CONFIGGLAG="--dest-cpu=ia32"
  fi

  if [ "$CARCH" == "x86_64" ]; then
    CONFIGFLAG="--dest-cpu=x64"
  fi

  export CXXFLAGS
  export GYPFLAGS
}
prepare(){
  cd "$srcdir/${pkgname}"
  msg 'Fixing for python2 name'
  find -type f -exec sed \
    -e 's_^#!/usr/bin/env python$_&2_' \
    -e 's_^\(#!/usr/bin/python2\).[45]$_\1_' \
    -e 's_^#!/usr/bin/python$_&2_' \
    -e "s_'python'_'python2'_" -i {} \;
  find test/ -type f -exec sed 's_python _python2 _' -i {} \;
}
build() {
  
  cd "$srcdir/${pkgname}"
  
  set_flags

  export PYTHON=python2
  $PYTHON ./configure \
    --engine-mozilla \
    --dest-os="linux" \
    ${CONFIGFLAG} 
  make

}

check(){
  cd "$srcdir/${pkgname}"
  make test || warning "Tests failed"
}

package() {
  cd "$srcdir/${pkgname}"
  make DESTDIR="$pkgdir" install

  # install docs as per user request
  install -d "$pkgdir/usr/share/doc/jxcore"
  cp -r doc/api \
    "$pkgdir/usr/share/doc/jxcore"

  install -D -m644 JXCORE_LICENSE \
    "$pkgdir/usr/share/licenses/jxcore/LICENSE"
}

# vim:set ts=2 sw=2 et:
