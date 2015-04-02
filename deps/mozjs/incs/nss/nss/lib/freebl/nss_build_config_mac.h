/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * This header defines the configuration macros used by lib/freebl files
 * for iOS and Mac OS X. It is included with the -include compiler flag.
 */
#if !defined(__APPLE__)
#error This file is intended for iOS and Mac OS X only
#endif

#if defined(__i386__)

#define NSS_X86_OR_X64 1
#define NSS_X86 1
#define i386 1

#elif defined(__x86_64__)

#define NSS_USE_64 1
#define NSS_X86_OR_X64 1
#define NSS_X64 1

#elif defined(__arm__)

#define MP_ASSEMBLY_MULTIPLY 1
#define MP_ASSEMBLY_SQUARE 1
#define MP_USE_UINT_DIGIT 1
#define SHA_NO_LONG_LONG 1

#elif defined(__aarch64__)

#define NSS_USE_64 1

#else

#error unknown processor architecture

#endif
