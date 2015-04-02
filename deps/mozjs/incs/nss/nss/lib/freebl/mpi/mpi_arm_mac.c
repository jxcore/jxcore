/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* 
 * A wrapper file for mpi_arm.c on iOS and Mac OS X.
 *
 * Xcode does not support target arch specific source files. This
 * wrapper file allows Xcode to compile mpi_arm.c only when the
 * target arch is arm.
 */

#if !defined(__APPLE__)
#error This file is intended for iOS and Mac OS X only
#endif

#if defined(__arm__)
#include "mpi_arm.c"
#endif
