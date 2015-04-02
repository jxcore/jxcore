#!/bin/sh
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This shell script checks out the NSS source tree from hg and prepares
# it for Chromium.

# Make the script exit as soon as something fails.
set -ex

# We only need the nss/lib directory, but hg requires us to check out the
# complete nss source tree.
rm -rf nss
hg clone -u NSS_3_16_5_RTM https://hg.mozilla.org/projects/nss

# Rename one of the utf8.c files to avoid name conflict.
mv nss/lib/base/utf8.c nss/lib/base/nssutf8.c

rm -r nss/lib/ckfw/capi
rm -r nss/lib/ckfw/dbm
rm -r nss/lib/ckfw/nssmkey
rm -r nss/lib/crmf
rm -r nss/lib/dbm
rm -r nss/lib/freebl/ecl/tests
rm -r nss/lib/freebl/mpi/doc
rm -r nss/lib/freebl/mpi/tests
rm -r nss/lib/freebl/mpi/utils
rm -r nss/lib/jar
rm -r nss/lib/pkcs12
rm -r nss/lib/pki/doc
rm -r nss/lib/softoken/legacydb
rm -r nss/lib/sqlite
rm -r nss/lib/sysinit
rm -r nss/lib/zlib

find nss/lib -name README -print | xargs rm

# Remove the build system.
find nss/lib -name Makefile -print | xargs rm
find nss/lib -name manifest.mn -print | xargs rm
find nss/lib -name "*.mk" -print | xargs rm

# Remove files for building shared libraries/DLLs.
find nss/lib -name "*.def" -print | xargs rm
find nss/lib -name "*.rc" -print | xargs rm

# Remove obsolete files or files we don't need.
rm nss/lib/ckfw/builtins/certdata.perl
rm nss/lib/ckfw/builtins/certdata.txt
rm nss/lib/ckfw/ck.api
rm nss/lib/ckfw/ckapi.perl
rm nss/lib/util/secload.c
rm nss/lib/util/secplcy.c
rm nss/lib/util/secplcy.h
rm nss/lib/smime/*.c

find nss/lib/ssl -type f ! -name sslerr.h | xargs rm

find nss/lib/freebl -type f \
    ! -name aeskeywrap.c ! -name alg2268.c ! -name alghmac.c \
    ! -name alghmac.h ! -name arcfive.c ! -name arcfour.c \
    ! -name blapi.h ! -name blapii.h ! -name blapit.h \
    ! -name camellia.c ! -name camellia.h \
    ! -name ctr.c ! -name ctr.h ! -name cts.c ! -name cts.h \
    ! -name des.c ! -name des.h ! -name desblapi.c ! -name dh.c \
    ! -name drbg.c ! -name dsa.c ! -name ec.c \
    ! -name ec.h ! -name ec2.h ! -name ecdecode.c ! -name ecl-curve.h \
    ! -name ecl-exp.h ! -name ecl-priv.h ! -name ecl.c \
    ! -name ecl.c ! -name ecl.h ! -name ecl_curve.c \
    ! -name ecl_gf.c ! -name ecl_mult.c ! -name ecp.h \
    ! -name ecp_256.c ! -name ecp_256_32.c \
    ! -name ecp_384.c ! -name ecp_521.c \
    ! -name ecp_aff.c ! -name ecp_jac.c ! -name ecp_jm.c \
    ! -name ecp_mont.c ! -name ec_naf.c ! -name gcm.c ! -name gcm.h \
    ! -name hmacct.c ! -name hmacct.h \
    ! -name intel-aes-x64-masm.asm ! -name intel-aes-x86-masm.asm \
    ! -name intel-gcm-x64-masm.asm ! -name intel-gcm-x86-masm.asm \
    ! -name jpake.c ! -name md2.c \
    ! -name md5.c ! -name logtab.h ! -name mpcpucache.c \
    ! -name mpi-config.h \
    ! -name mpi-priv.h ! -name mpi.c ! -name mpi.h \
    ! -name mpi_amd64.c ! -name mpi_arm.c ! -name mpi_x86_asm.c \
    ! -name mplogic.c ! -name mplogic.h ! -name mpmontg.c \
    ! -name mpprime.c ! -name mpprime.h \
    ! -name mp_gf2m-priv.h ! -name mp_gf2m.c ! -name mp_gf2m.h \
    ! -name primes.c ! -name pqg.c ! -name pqg.h ! -name rawhash.c \
    ! -name rijndael.c ! -name rijndael.h ! -name rijndael32.tab \
    ! -name rsa.c ! -name rsapkcs.c ! -name secmpi.h \
    ! -name secrng.h ! -name seed.c ! -name seed.h \
    ! -name sha256.h ! -name sha512.c ! -name sha_fast.c \
    ! -name sha_fast.h ! -name shsign.h ! -name shvfy.c \
    ! -name sysrand.c ! -name tlsprfalg.c ! -name unix_rand.c \
    ! -name win_rand.c \
    | xargs rm
