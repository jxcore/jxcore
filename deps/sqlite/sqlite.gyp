# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'use_system_sqlite%': 0,
  },
  'target_defaults': {
    'defines': [
      'SQLITE_CORE',
      'SQLITE_ENABLE_BROKEN_FTS3',
      'DSQLITE_ENABLE_FTS3_PARENTHESIS',
      'SQLITE_ENABLE_FTS3',
      'SQLITE_ENABLE_FTS4',
      'SQLITE_ENABLE_MEMORY_MANAGEMENT',
      'SQLITE_SECURE_DELETE',
      'SQLITE_SEPARATE_CACHE_POOLS',
      'THREADSAFE',
      '_HAS_EXCEPTIONS=0',
    ],
  },
 'targets': [
    {
      'target_name': 'sqlite',
      'conditions': [
        [ 'v8_is_3_28==1', {
          'defines': [
            'V8_IS_3_28=1'
          ],
        }],
        [ 'v8_is_3_14==1', {
          'defines': [
            'V8_IS_3_14=1'
          ],
        }],
        ['use_system_sqlite', {
          'type': 'none',
          'direct_dependent_settings': {
            'defines': [
              'USE_SYSTEM_SQLITE',
            ],
          },
          'conditions': [
            ['OS == "ios"', {
              'dependencies': [
                'sqlite_regexp',
              ],
              'link_settings': {
                'libraries': [
                  '$(SDKROOT)/usr/lib/libsqlite3.dylib',
                ],
              },
            }],
          ],
        }, { # !use_system_sqlite
          'product_name': 'sqlite3',
          'type': 'static_library',
          'sources': [
            'sqlite3.h',
            'sqlite3ext.h',
            'sqlite3.c',
          ],
          'include_dirs': [
            # 'amalgamation',
            # Needed for fts2 to build.
            # 'src/src',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              '.',
              '../..',
            ],
          },
          'msvs_disabled_warnings': [
            4018, 4244, 4267,
          ],
          'conditions': [
            ['OS=="linux"', {
              'link_settings': {
                'libraries': [
                  '-ldl',
                ],
              },
            }],
            ['OS == "mac" or OS == "ios"', {
              'link_settings': {
                'libraries': [
                  '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
                ],
              },
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
                  'xcode_settings': { 'SDKROOT': 'iphonesimulator' },
                }, {
                  'xcode_settings': { 'SDKROOT': 'iphoneos', 'ENABLE_BITCODE': 'YES'},
                }]
              ],
            }],
            ['OS == "android"', {
              'defines': [
                'HAVE_USLEEP=1',
                'SQLITE_DEFAULT_JOURNAL_SIZE_LIMIT=1048576',
                'SQLITE_DEFAULT_AUTOVACUUM=1',
                'SQLITE_TEMP_STORE=3',
                'SQLITE_ENABLE_FTS3_BACKWARDS',
                'DSQLITE_DEFAULT_FILE_FORMAT=4',
              ],
            }],
            ['os_posix == 1 and OS != "mac" and OS != "ios" and OS != "android"', {
              'cflags': [
                # SQLite doesn't believe in compiler warnings,
                # preferring testing.
                #   http://www.sqlite.org/faq.html#q17
                '-Wno-int-to-pointer-cast',
                '-Wno-pointer-to-int-cast',
              ],
            }],
            ['clang==1', {
              'xcode_settings': {
                'WARNING_CFLAGS': [
                  # sqlite does `if (*a++ && *b++);` in a non-buggy way.
                  '-Wno-empty-body',
                  # sqlite has some `unsigned < 0` checks.
                  '-Wno-tautological-compare',
                ],
              },
              'cflags': [
                '-Wno-empty-body',
                '-Wno-tautological-compare',
              ],
            }],
          ],
        }],
      ],
    },
  ],
}
