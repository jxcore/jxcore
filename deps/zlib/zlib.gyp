# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'use_system_zlib%': 0
  },
  'conditions': [
    ['use_system_zlib==0', {
      'targets': [
        {
          'target_name': 'zlib',
          'type': 'static_library',
          'sources': [
            'contrib/minizip/ioapi.c',
            'contrib/minizip/ioapi.h',
            'contrib/minizip/iowin32.c',
            'contrib/minizip/iowin32.h',
            'contrib/minizip/unzip.c',
            'contrib/minizip/unzip.h',
            'contrib/minizip/zip.c',
            'contrib/minizip/zip.h',
            'adler32.c',
            'compress.c',
            'crc32.c',
            'crc32.h',
            'deflate.c',
            'deflate.h',
            'gzio.c',
            'infback.c',
            'inffast.c',
            'inffast.h',
            'inffixed.h',
            'inflate.c',
            'inflate.h',
            'inftrees.c',
            'inftrees.h',
            'mozzconf.h',
            'trees.c',
            'trees.h',
            'uncompr.c',
            'zconf.h',
            'zlib.h',
            'zutil.c',
            'zutil.h',
          ],
          'include_dirs': [
            '.',
            # For contrib/minizip
            './contrib/minizip',
          ],
          'defines!': [ 'DEBUG' ], # disable ZLib debugging (MozJS 431950)
          'direct_dependent_settings': {
            'include_dirs': [
              '.',
            ],
          },
          'conditions': [
            ['OS!="win"', {
              'product_name': 'chrome_zlib',
              'cflags!': [ '-ansi' ],
              'sources!': [
                'contrib/minizip/iowin32.c'
              ],
            }],
            ['OS=="ios"', {
              'xcode_settings': {
                'ALWAYS_SEARCH_USER_PATHS': 'NO',
                'GCC_CW_ASM_SYNTAX': 'NO',                # No -fasm-blocks
                'GCC_DYNAMIC_NO_PIC': 'NO',               # No -mdynamic-no-pic
                                                          # (Equivalent to -fPIC)
                'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',        # -fno-exceptions
                'GCC_ENABLE_CPP_RTTI': 'NO',              # -fno-rtti
                'GCC_ENABLE_PASCAL_STRINGS': 'NO',        # No -mpascal-strings
                'GCC_THREADSAFE_STATICS': 'NO',           # -fno-threadsafe-statics
                'PREBINDING': 'NO',                       # No -Wl,-prebind
                'EMBED_BITCODE': 'YES',
                'IPHONEOS_DEPLOYMENT_TARGET': '6.0',
                'GCC_GENERATE_DEBUGGING_SYMBOLS': 'NO',
          
                'USE_HEADERMAP': 'NO',
                'OTHER_CFLAGS': [
                  '-fno-strict-aliasing',
                  '-fno-standalone-debug'
                ],
                'OTHER_CPLUSPLUSFLAGS': [
                  '-fno-strict-aliasing',
                  '-fno-standalone-debug'
                ],
                'OTHER_LDFLAGS': [
                  '-s'
                ],
                'WARNING_CFLAGS': [
                  '-Wall',
                  '-Wendif-labels',
                  '-W',
                  '-Wno-unused-parameter',
                ],
              },
              'defines':[ '__IOS__' ],
              'conditions': [
                ['target_arch=="ia32"', {
                  'xcode_settings': {'ARCHS': ['i386']},
                }],
                ['target_arch=="x64"', {
                  'xcode_settings': {'ARCHS': ['x86_64']},
                }],
                [ 'target_arch in "arm64 arm armv7s"', {
                  'xcode_settings': {
                    'OTHER_CFLAGS': [
                      '-fembed-bitcode'
                    ],
                    'OTHER_CPLUSPLUSFLAGS': [
                      '-fembed-bitcode'
                    ],
                  }
                }],
                [ 'target_arch=="arm64"', {
                  'xcode_settings': {'ARCHS': ['arm64']},
                }],
                [ 'target_arch=="arm"', {
                  'xcode_settings': {'ARCHS': ['armv7']},
                }],
                [ 'target_arch=="armv7s"', {
                  'xcode_settings': {'ARCHS': ['armv7s']},
                }],
                [ 'target_arch=="x64" or target_arch=="ia32"', {
                  'xcode_settings': { 'SDKROOT': 'iphonesimulator', 'ENABLE_BITCODE': 'YES'  },
                }, {
                  'xcode_settings': { 'SDKROOT': 'iphoneos', 'ENABLE_BITCODE': 'YES'},
                }]
              ],
            }],
          ],
        },
      ],
    }, {
      'targets': [
        {
          'target_name': 'zlib',
          'type': 'static_library',
          'direct_dependent_settings': {
            'defines': [
              'USE_SYSTEM_ZLIB',
            ],
          },
          'defines': [
            'USE_SYSTEM_ZLIB',
          ],
          'sources': [
            'contrib/minizip/ioapi.c',
            'contrib/minizip/ioapi.h',
            'contrib/minizip/unzip.c',
            'contrib/minizip/unzip.h',
            'contrib/minizip/zip.c',
            'contrib/minizip/zip.h',
          ],
          'link_settings': {
            'libraries': [
              '-lz',
            ],
          },
        },
      ],
    }],
  ],
}
