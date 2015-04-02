#!/bin/sh
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This shell script checks out the NSPR source tree from CVS and prepares
# it for Chromium.

# Make the script exit as soon as something fails.
set -ex

rm -rf nspr
hg clone -u NSPR_4_10_7_RTM https://hg.mozilla.org/projects/nspr

rm -r nspr/.hg
rm -r nspr/admin
rm -r nspr/build
rm -r nspr/config
rm -r nspr/lib/prstreams
rm -r nspr/lib/tests
rm -r nspr/pkg
rm -r nspr/pr/src/cplus
rm -r nspr/pr/tests
rm -r nspr/tools

# Remove unneeded platform-specific directories.
rm -r nspr/pr/src/bthreads
rm -r nspr/pr/src/md/beos
rm -r nspr/pr/src/md/os2

find nspr -name .cvsignore -print | xargs rm
rm nspr/.hgignore
rm nspr/.hgtags
find nspr -name README -print | xargs rm

# Remove the build system.
rm nspr/configure
rm nspr/configure.in
find nspr -name Makefile.in -print | xargs rm
find nspr -name "*.mk" -print | xargs rm

# Remove files for building shared libraries/DLLs.
find nspr -name "*.def" -print | xargs rm
find nspr -name "*.rc" -print | xargs rm
find nspr -name prvrsion.c -print | xargs rm
find nspr -name plvrsion.c -print | xargs rm

# Remove unneeded platform-specific files in nspr/pr/include/md.
find nspr/pr/include/md -name "_*" ! -name "_darwin.*" \
    ! -name "_linux.*" ! -name "_win95.*" ! -name _pth.h ! -name _pcos.h \
    ! -name _unixos.h ! -name _unix_errors.h ! -name _win32_errors.h -print \
    | xargs rm

# Remove files for unneeded Unix flavors.
find nspr/pr/src/md/unix -type f ! -name "ux*.c" ! -name unix.c \
    ! -name unix_errors.c ! -name darwin.c ! -name "os_Darwin*.s" \
    ! -name linux.c ! -name "os_Linux*.s" -print \
    | xargs rm
rm nspr/pr/src/md/unix/os_Darwin_ppc.s
rm nspr/pr/src/md/unix/os_Linux_ppc.s
rm nspr/pr/src/md/unix/os_Linux_ia64.s
rm nspr/pr/src/md/unix/uxpoll.c

# Remove files for the WINNT build configuration.
rm nspr/pr/src/md/windows/ntdllmn.c
rm nspr/pr/src/md/windows/ntio.c
rm nspr/pr/src/md/windows/ntthread.c

# Remove obsolete files or files we don't need.
rm nspr/pr/include/gencfg.c
rm nspr/pr/src/misc/compile-et.pl
rm nspr/pr/src/misc/dtoa.c
rm nspr/pr/src/misc/prerr.et
rm nspr/pr/src/misc/prerr.properties
