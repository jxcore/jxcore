{
  'variables': {
    'visibility%': 'hidden',         # V8's visibility setting
    'target_arch%': 'ia32',          # set v8's target architecture
    'host_arch%': 'ia32',            # set v8's host architecture
    'want_separate_host_toolset': 0, # V8 should not build target and host
    'library%': 'static_library',    # allow override to 'shared_library' for DLL/.so builds
    'component%': 'static_library',  # NB. these names match with what V8 expects
    'msvs_multi_core_compile': '0',  # we do enable multicore compiles, but not using the V8 way
    'gcc_version%': 'unknown',
    'clang%': 1,
    'python%': 'python',
    'uclibc_defined%': 0,
    
    # engines
    'node_engine_chakra%': 0,
    'node_engine_mozilla%': 0,
    'node_engine_v8%': 0,
    'v8_postmortem_support': 'false',
    
    'node_win_onecore%' : 0,

    # Enable disassembler for `--print-code` v8 options
    'v8_enable_disassembler': 1,

    # Enable V8's post-mortem debugging only on unix flavors.
    'conditions': [
      ['uclibc_defined == 1', {
        'defines':['POSIX_UCLIBC_DEFINED'],
      }], 
      ['OS == "win"', {
        'os_posix%': 0,
      }, {
        'os_posix%': 1,
      }],
      ['(GENERATOR == "ninja" or OS == "mac") and node_engine_v8==1', {
        'OBJ_DIR': '<(PRODUCT_DIR)/obj',
        'V8_BASE': '<(PRODUCT_DIR)/libv8_base.a',
      }],
      ['GENERATOR != "ninja" and OS != "mac" and node_engine_v8==1', {
        'OBJ_DIR': '<(PRODUCT_DIR)/obj.target',
        'V8_BASE': '<(PRODUCT_DIR)/obj.target/deps/v8/tools/gyp/libv8_base.a',
      }],
      # A flag for BSD platforms
      ['OS=="freebsd" or OS=="openbsd"', {
        'os_bsd%': 1,
      }, {
        'os_bsd%': 0,
      }],
    ],
  },

  'target_defaults': {
    'default_configuration': 'Release',
    'configurations': {
      'Debug': {
        'defines': [ 'DEBUG', '_DEBUG' ],
        'variables': {
          'v8_enable_handle_zapping%': 1,
        },
        'cflags': [ '-g', '-O0' ],
        'conditions': [
          ['target_arch=="x64"', {
            'msvs_configuration_platform': 'x64',
          }],
          ['target_arch=="arm"', {
            'msvs_configuration_platform': 'ARM',
          }],
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 1, # static debug
            'Optimization': 0, # /Od, no optimization
            'MinimalRebuild': 'false',
            'OmitFramePointers': 'false',
            'BasicRuntimeChecks': 3, # /RTC1
          },
          'VCLinkerTool': {
            'LinkIncremental': 2, # enable incremental linking
          },
        },
        'xcode_settings': {
          'GCC_OPTIMIZATION_LEVEL': '0', # stop gyp from defaulting to -Os
        },
      },
      'Release': {
        'cflags': [ '-O2', '-ffunction-sections', '-fdata-sections', '-fno-strict-aliasing' ],
        'cflags!': [ '-O3', '-fstrict-aliasing' ],
        'variables': {
          'v8_enable_handle_zapping%': 0,
        },
        'conditions': [
          ['target_arch=="x64"', {
            'msvs_configuration_platform': 'x64',
          }],
          ['target_arch=="arm"', {
            'msvs_configuration_platform': 'ARM',
          }],
          ['OS=="solaris"', {
            # pull in V8's postmortem metadata
            'ldflags': [ '-Wl,-z,allextract' ]
          }],
          ['clang == 0 and gcc_version >= 40', {
            'cflags': [ '-fno-tree-vrp' ],
          }],
          ['clang == 0 and gcc_version <= 44', {
            'cflags': [ '-fno-tree-sink' ],
          }],
          ['OS!="mac" and OS!="win"', {
            'cflags': [ '-fno-omit-frame-pointer' ],
          }],
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 0, # static release
            'Optimization': 3, # /Ox, full optimization
            'FavorSizeOrSpeed': 1, # /Ot, favour speed over size
            'InlineFunctionExpansion': 2, # /Ob2, inline anything eligible
            'WholeProgramOptimization': 'true', # /GL, whole program optimization, needed for LTCG
            'OmitFramePointers': 'true',
            'EnableFunctionLevelLinking': 'true',
            'EnableIntrinsicFunctions': 'true',
            'RuntimeTypeInfo': 'false',
            'ExceptionHandling': '0',
            'AdditionalOptions': [
              '/MP', # compile across multiple CPUs
            ],
          },
          'VCLibrarianTool': {
          },
          'VCLinkerTool': {
            'LinkTimeCodeGeneration': 1, # link-time code generation
            'OptimizeReferences': 2, # /OPT:REF
            'EnableCOMDATFolding': 2, # /OPT:ICF
            'LinkIncremental': 1, # disable incremental linking
          },
        },
      }
    },
    'msvs_settings': {
      'VCCLCompilerTool': {
        'StringPooling': 'true', # pool string literals
        'DebugInformationFormat': 3, # Generate a PDB
        'WarningLevel': 3,
        'BufferSecurityCheck': 'true',
        'ExceptionHandling': 1, # /EHsc
        'SuppressStartupBanner': 'true',
        'WarnAsError': 'false',
      },
      'VCLibrarianTool': {
        'AdditionalOptions': [
          '/LTCG', # link time code generation
        ],
      },
      'VCLinkerTool': {
        'conditions': [
          ['target_arch=="ia32"', {
            'TargetMachine' : 1, # /MACHINE:X86
            'target_conditions': [
              ['_type=="executable"', {
                'AdditionalOptions': [ '/SubSystem:Console,"5.01"' ],
              }],
            ],
          }],
          ['target_arch=="x64"', {
            'TargetMachine' : 17, # /MACHINE:AMD64
            'target_conditions': [
              ['_type=="executable"', {
                'AdditionalOptions': [ '/SubSystem:Console,"5.02"' ],
              }],
            ],
          }],
        ],
        'GenerateDebugInformation': 'true',
        'RandomizedBaseAddress': 2, # enable ASLR
        'DataExecutionPrevention': 2, # enable DEP
        'AllowIsolation': 'true',
        'SuppressStartupBanner': 'true',
      },
    },
    'msvs_disabled_warnings': [4351, 4355, 4800],
    'conditions': [
      ['node_win_onecore==1', {
        'defines': [ 'WINONECORE=1' ],
        'msvs_settings': {
          'VCLinkerTool': {
            'IgnoreDefaultLibraryNames' : [
              'kernel32.lib',
              'advapi32.lib',
             ],
           }
        },
        'libraries': [
          '-lonecore.lib',
        ],
      }],
      ['OS == "win"', {
        'msvs_cygwin_shell': 0, # prevent actions from trying to use cygwin
        'defines': [
          'WIN32',
          # we don't really want VC++ warning us about
          # how dangerous C functions are...
          '_CRT_SECURE_NO_DEPRECATE',
          # ... or that C implementations shouldn't use
          # POSIX names
          '_CRT_NONSTDC_NO_DEPRECATE',
          'BUILDING_V8_SHARED=1',
          'BUILDING_UV_SHARED=1',
        ],
      }],
      [ 'OS in "linux freebsd openbsd solaris"', {
        'cflags': [ '-pthread', ],
        'ldflags': [ '-pthread' ],
      }],
      [ 'OS in "linux freebsd openbsd solaris" and node_shared_library==1', {
        'cflags': [ '-fPIC' ],
        'ldflags': [ '-fPIC' ],
      }],
      [ 'OS in "linux freebsd openbsd solaris android"', {
        'cflags': [ '-Wall', '-Wextra', '-Wno-unused-parameter', 
                    '-Wno-inconsistent-missing-override' ],
        'cflags_cc': [ '-fno-rtti', '-fno-exceptions' ],
        'ldflags': [ '-rdynamic' ],
        'target_conditions': [
          ['_type=="static_library"', {
            'standalone_static_library': 1, # disable thin archive which needs binutils >= 2.19
          }],
        ],
        'conditions': [
          [ 'target_arch=="ia32"', {
            'cflags': [ '-m32' ],
            'ldflags': [ '-m32' ],
          }],
          [ 'target_arch=="x64"', {
            'cflags': [ '-m64' ],
            'ldflags': [ '-m64' ],
          }],
          [ 'OS=="solaris"', {
            'cflags': [ '-pthreads' ],
            'ldflags': [ '-pthreads' ],
            'cflags!': [ '-pthread' ],
            'ldflags!': [ '-pthread' ],
          }],
        ],
      }],
      [ 'OS=="android"', {
        'defines': ['_GLIBCXX_USE_C99_MATH'],
        'libraries': [ '-llog' ],
      }],
      [ 'OS=="android" and node_static_library==0', {
        'cflags': [ '-pie -fPIE' ],
        'ldflags': [ '-pie -fPIE' ],
      }],
      ['OS=="mac"', {
        'defines': ['_DARWIN_USE_64_BIT_INODE=1'],
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
          'MACOSX_DEPLOYMENT_TARGET': '10.5',       # -mmacosx-version-min=10.5
          'USE_HEADERMAP': 'NO',
          'OTHER_CFLAGS': [
            '-fno-strict-aliasing',
          ],
          'WARNING_CFLAGS': [
            '-Wall',
            '-Wendif-labels',
            '-W',
            '-Wno-unused-parameter',
            '-Wno-inconsistent-missing-override'
          ],
        },
        'target_conditions': [
          ['_type!="static_library"', {
            'xcode_settings': {'OTHER_LDFLAGS': ['-Wl,-search_paths_first']},
          }],
        ],
        'conditions': [
          ['target_arch=="ia32"', {
            'xcode_settings': {'ARCHS': ['i386']},
          }],
          ['target_arch=="x64"', {
            'xcode_settings': {'ARCHS': ['x86_64']},
          }],
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
            'xcode_settings': { 'SDKROOT': 'iphonesimulator' },
          }, {
            'xcode_settings': { 'SDKROOT': 'iphoneos', 'ENABLE_BITCODE': 'YES'},
          }]
        ],
      }],
      ['OS=="freebsd" and node_use_dtrace=="true"', {
        'libraries': [ '-lelf' ],
      }],
      ['OS=="freebsd"', {
        'ldflags': [
          '-Wl,--export-dynamic',
        ],
      }],
      ['target_arch in "mipsel mips"', {
        'defines' : [ '__MIPSEL__' ] #force define mipsel
      }],
      ['OS=="android"', {
        'defines': [ 'ANDROID' ] #force define ANDROID
      }],
    ],
  }
}