{
  'variables':
  {
    'v8_use_snapshot%': 'true',
    #Turn off - Werror in V8# See http: //codereview.chromium.org/8159015
      'werror': '',
    'node_use_dtrace%': 'false',
    'node_use_etw%': 'false',
    'node_use_perfctr%': 'false',
    'node_has_winsdk%': 'false',
    'node_shared_v8%': 'false',
    'node_shared_zlib%': 'false',
    'node_shared_http_parser%': 'false',
    'node_shared_cares%': 'false',
    'node_shared_libuv%': 'false',
    'node_use_openssl%': 'true',
    'node_use_systemtap%': 'false',
    'node_shared_openssl%': 'false',
    'node_no_sqlite%': 'false',
    'node_shared_library%': 0,
    'node_embed_leveldown%': 0,
    'node_use_mdb%': 'false',
    'node_v8_options%': '',
    'library_files': [
      'lib/jx/_jx_argv.js',
      'lib/jx/_jx_subs.js',
      'lib/jx/_jx_memStore.js',
      'lib/jx/_jx_tasks.js',
      'lib/jx/_jx_utils.js',
      'lib/jx/_jx_install.js',
      'lib/jx/_jx_multiplier.js',
      'lib/jx/_jx_monitor.js',
      'lib/jx/_jx_monitorHelper.js',
      'lib/jx/_jx_loadEmbedded.js',
      'lib/jx/_jx_http_helper.js',
      'lib/jx/_jx_config.js',
      'lib/jx/_jx_incoming.js',
      'lib/jx/_jx_package.js',
      'lib/jx/_jx_parsers.js',
      'lib/jx/_jx_timers.js',
      'lib/jx/_jx_source.js',
      'lib/jx/_jx_marker.js',

      'src/node.js',
      'lib/_debugger.js',
      'lib/_linklist.js',
      'lib/assert.js',
      'lib/buffer.js',
      'lib/child_process.js',
      'lib/console.js',
      'lib/constants.js',
      'lib/crypto.js',
      'lib/cluster.js',
      'lib/dgram.js',
      'lib/dns.js',
      'lib/domain.js',
      'lib/events.js',
      'lib/freelist.js',
      'lib/fs.js',
      'lib/http.js',
      'lib/https.js',
      'lib/module.js',
      'lib/net.js',
      'lib/os.js',
      'lib/path.js',
      'lib/punycode.js',
      'lib/querystring.js',
      'lib/readline.js',
      'lib/repl.js',
      'lib/stream.js',
      'lib/_stream_readable.js',
      'lib/_stream_writable.js',
      'lib/_stream_duplex.js',
      'lib/_stream_transform.js',
      'lib/_stream_passthrough.js',
      'lib/string_decoder.js',
      'lib/sys.js',
      'lib/timers.js',
      'lib/tls.js',
      'lib/tty.js',
      'lib/url.js',
      'lib/util.js',
      'lib/vm.js',
      'lib/zlib.js',
    ],
    'conditions':[
      ['node_no_sqlite == 0', {
        'library_files':[ 'lib/external/sqlite3.js' ],
      }],
    ]
  },

  'targets': [
  {
    'target_name': 'jx',

    'include_dirs': [
      'src',
      'src/wrappers',
      'tools/msvs/genfiles',
      'deps/uv/src/ares',
      '<(SHARED_INTERMEDIATE_DIR)' #for node_natives.h
    ],

    'sources': [
      'src/jx/Proxy/EngineLogger.cc',
      'src/jx/extend.cc',
      'src/jx/commons.cc',
      'src/jx/job.cc',
      'src/jx/jx_instance.cc',
      'src/jx/job_store.cc',
      'src/jx/memory_store.cc',
      'src/jx/jxp_compress.cc',
      'src/jx/error_definition.cc',

      'src/wrappers/handle_wrap.cc',
      'src/wrappers/thread_wrap.cc',
      'src/wrappers/memory_wrap.cc',
      'src/wrappers/jxtimers_wrap.cc',
      'src/wrappers/timer_wrap.cc',
      'src/wrappers/node_os.cc',
      'src/wrappers/cares_wrap.cc',
      'src/wrappers/fs_event_wrap.cc',
      'src/wrappers/node_buffer.cc',
      'src/wrappers/tcp_wrap.cc',
      'src/wrappers/udp_wrap.cc',
      'src/wrappers/jxutils_wrap.cc',
      'src/wrappers/crypto_extension.cc',
      'src/wrappers/process_wrap.cc',
      'src/wrappers/signal_wrap.cc',
      'src/wrappers/node_file.cc',
      'src/wrappers/stream_wrap.cc',
      'src/wrappers/tty_wrap.cc',
      'src/wrappers/pipe_wrap.cc',
      'src/wrappers/node_http_parser.cc',
      'src/wrappers/node_zlib.cc',

      'src/external/module_wrap.cc',

      'src/node_constants.cc',
      'src/node_extensions.cc',
      'src/node_javascript.cc',
      
      'src/public/jx_result.cc',
      'src/public/jx.cc',

      'src/jxcore.cc',
      'src/node.cc',
      'src/node_script.cc',
      'src/string_bytes.cc',
      'src/slab_allocator.cc',
      'src/node_stat_watcher.cc'
    ],

    'defines': [
      'NODE_WANT_INTERNALS=1',
      'ARCH="<(target_arch)"',
      'PLATFORM="<(OS)"', #'NODE_TAG="<(node_tag)"',
      'NODE_V8_OPTIONS="<(node_v8_options)"',
    ],

    'cflags!': ['-ansi'],

    'conditions': [
      ['node_compress_internals!=1',
      {
        'dependencies': [
          'node_js2c#host',
        ],
      },
      {
        'dependencies': [
          'jx_js2c#cmp',
        ],
        'defines': [
          'JXCORE_SOURCES_MINIFIED'
        ]
      }],
      ['node_static_library==1',
      {
        'defines!': ['HAVE_DTRACE'],
        'defines': ['JXCORE_EMBEDDED'],
        'type': 'static_library'
      }],
      ['node_shared_library==1',
      {
        'defines!': ['HAVE_DTRACE'],
        'defines': ['JXCORE_EMBEDDED'],
        'type': 'shared_library'
      }],
      ['node_shared_library==1 and node_win_onecore==1', {
        'type': 'loadable_module',
        'defines': [ 'UWP_DLL=1' ]
      }],
      ['node_shared_library==0 and node_static_library==0',
      {
        'type': 'executable',
        'sources': [
          'src/node_main.cc',
        ],
      }],
      ['node_use_openssl=="true"',
      {
        'defines': ['HAVE_OPENSSL=1'],
        'sources': ['src/wrappers/node_crypto.cc'],
        'conditions': [
          ['node_shared_openssl=="false"',
          {
            'dependencies': ['./deps/openssl/openssl.gyp:openssl'],
          }]
        ]
      },
      {
        'defines': ['HAVE_OPENSSL=0']
      }],
      [ 'v8_is_3_28==1', {
        'defines': [
          'V8_IS_3_28=1'
        ],
        'sources': [
          'src/jx/Proxy/V8_3_28/JXString.cc',
        ],
        'conditions': [
          ['node_engine_v8==1', {
            'sources': [
              'deps/v8_3_28/v8/include/v8.h',
              'deps/v8_3_28/v8/include/v8-debug.h',
            ],
            'dependencies': ['deps/v8_3_28/v8/tools/gyp/v8.gyp:v8',
              'deps/v8_3_28/debugger-agent/debugger-agent.gyp:debugger-agent',],
          }],
          ['node_engine_chakra==1', {
            'variables': {
              'node_engine_include_dir%': 'deps/chakrashim/include'
            },
            'defines': ['JS_ENGINE_CHAKRA=1', 'JS_ENGINE_V8=1'],
            'dependencies': [ 'deps/chakrashim/chakrashim.gyp:chakrashim', 
                              'deps/node-uwp/binding.gyp:uwp'],
            'libraries': [
              '-lchakrart.lib',
              '-lole32.lib',
              '-lversion.lib',
            ],
          }]
        ],
      }],
      ['node_engine_v8==1', {
        'defines' : ['JS_ENGINE_V8=1'],
        'conditions': [
        [ 'gcc_version<=44', {
          # GCC versions <= 4.4 do not handle the aliasing in the queue
          # implementation, so disable aliasing on these platforms
          # to avoid subtle bugs
          'cflags': [ '-fno-strict-aliasing' ],
        }],
        ['v8_is_3_14==1', {
          'defines': [
              'V8_IS_3_14=1'
            ],
            'sources': [
              'src/jx/Proxy/V8_3_14/JXString.cc',
              'src/jx/Proxy/V8_3_14/v8_typed_array.cc',
              'deps/v8/include/v8.h',
              'deps/v8/include/v8-debug.h',
            ],
            'dependencies': ['deps/v8/tools/gyp/v8.gyp:v8'],
        }],
        [ 'v8_enable_i18n_support==1', {
          'defines': [ 'NODE_HAVE_I18N_SUPPORT=1' ],
          'dependencies': [
            '<(icu_gyp_path):icui18n',
            '<(icu_gyp_path):icuuc',
          ],
          'conditions': [
            [ 'icu_small=="true"', {
              'defines': [ 'NODE_HAVE_SMALL_ICU=1' ],
          }]],
        }],
        ]
      }],
      ['node_engine_mozilla==1', {
        'v8_use_snapshot%': 'false',
        'defines': [
          'JS_ENGINE_MOZJS=1', 'EXPORT_JS_API', 'MOZJS_IS_3_40=1'
        ],
        'sources': [
          'src/jx/Proxy/Mozilla_340/MozJS/Isolate.cc',
          'src/jx/Proxy/Mozilla_340/MozJS/MozValue.cc',
          'src/jx/Proxy/Mozilla_340/MozJS/Exception.cc',
          'src/jx/Proxy/Mozilla_340/MozJS/utf_man.cc',
          'src/jx/Proxy/Mozilla_340/EngineHelper.cc',
          'src/jx/Proxy/Mozilla_340/JXString.cc',
          'src/jx/Proxy/Mozilla_340/PArguments.cc',
          'src/jx/Proxy/Mozilla_340/SpiderHelper.cc',
        ],
        'include_dirs': [
          'deps/mozjs/src',
          'deps/mozjs/incs',
        ],
        'dependencies': ['deps/mozjs/mozjs.gyp:mozjs'],
        'conditions': [
          ['OS!="win"', {
            'defines': ['JS_POSIX_NSPR=1']
          }],
          ['target_arch in "arm armv7 armv7s"', {
            'defines': ['WTF_CPU_ARM_TRADITIONAL', 'JS_NUNBOX32', 'JS_CPU_ARM=1'],
          }],
          ['target_arch=="arm64"', {
            'defines': ['WTF_CPU_ARM_TRADITIONAL', 'JS_PUNBOX64', 'JS_CPU_ARM=1', '__ARM_ARCH_64__'],
          }],
          ['target_arch=="x64"', {
            'defines': ['JS_PUNBOX64', 'JS_CPU_X64=1'],
          }],
          ['target_arch=="ia32"', {
            'defines': ['JS_NUNBOX32', 'JS_CPU_X86=1'],
          }],
          ['target_arch in "mipsel mips"', {
            'defines' : [ 'JS_CODEGEN_NONE', 'JS_NUNBOX32', 'JS_CPU_MIPS' ]
          }],
          ['OS in "linux android freebsd"', {
            "cflags": [
              "-std=c++0x", '-D__STDC_LIMIT_MACROS',
              '-Wno-missing-field-initializers', '-Wno-extra',
              '-Wno-invalid-offsetof', '-Wno-ignored-qualifiers'
            ],
          }],
          ['OS in "linux android"', {
            "defines": [
              "JS_HAVE_ENDIAN_H",
            ],
          }],
          ['OS in "freebsd bsd"', {
            "defines": [
              "JS_HAVE_MACHINE_ENDIAN_H",
            ],
          }],
          ['OS=="ios" or OS=="mac"',
          {
            'defines': [
              'JS_HAVE_MACHINE_ENDIAN_H=1',
              'XP_MACOSX=1',
              'DARWIN=1',
            ],
            'xcode_settings': {
              'OTHER_CPLUSPLUSFLAGS': ['-std=c++11', '-stdlib=libstdc++',
                '-Wno-mismatched-tags', '-Wno-missing-field-initializers',
                '-Wno-unused-private-field', '-Wno-invalid-offsetof', '-Wno-ignored-qualifiers'
              ],
              'OTHER_CFLAGS': ['-std=gnu99'],
            },
            'conditions': [
              ['OS=="mac"', {
                'xcode_settings': {
                  'MACOSX_DEPLOYMENT_TARGET': '10.7',
                  #mozjs uses c++11 / libc++
                }
              }],
              ['OS=="ios"', {
                'xcode_settings': {
                  'IPHONEOS_DEPLOYMENT_TARGET': '6.0',
                },
                'defines': ['__IOS__']
              }],
              ['OS=="ios" and target_arch!="x64" and target_arch!="ia32"', {
                'xcode_settings': { 'SDKROOT': 'iphoneos' },
                'include_dirs': [
                  'src/platform/ios_device', #ios device SDK doesn not have crt_externs.h
                ],
              }],
              ['OS=="ios" and (target_arch=="x64" or target_arch=="ia32")', {
                'xcode_settings': { 'SDKROOT': 'iphonesimulator' },
              }],
            ]
          }],
        ]
      }],
      ['node_use_dtrace=="true" and node_engine_v8==1',
      {
        'defines': ['HAVE_DTRACE=1'],
        'dependencies': ['node_dtrace_header'],
        'include_dirs': ['<(SHARED_INTERMEDIATE_DIR)'],
        'sources': [
          'src/node_dtrace.cc',
        ],
        'conditions': [
          [
            'OS!="mac"',
            {
              'sources': [
                'src/node_dtrace_ustack.cc',
                'src/node_dtrace_provider.cc',
              ]
            }
          ]
        ]
      }],
      ['node_use_systemtap=="true" and node_engine_v8==1 and v8_is_3_14==1',
      {
        'defines': ['HAVE_SYSTEMTAP=1', 'STAP_SDT_V1=1'],
        'dependencies': ['node_systemtap_header'],
        'include_dirs': ['<(SHARED_INTERMEDIATE_DIR)'],
        'sources': [
          'src/node_dtrace.cc',
          '<(SHARED_INTERMEDIATE_DIR)/node_systemtap.h',
        ],
      }],
      [ 'node_use_mdb=="true" and v8_is_3_28==1', {
        'dependencies': [ 'node_mdb' ],
        'include_dirs': [ '<(SHARED_INTERMEDIATE_DIR)' ],
        'sources': [
          'src/node_mdb.cc',
        ],
      } ],
      [ 'v8_postmortem_support=="true" and node_engine_v8==1 and v8_is_3_28==1', {
         'dependencies': [ 'deps/v8_3_28/v8/tools/gyp/v8.gyp:postmortem-metadata' ],
         'xcode_settings': {
           'OTHER_LDFLAGS': [
             '-Wl,-force_load,<(V8_BASE)',
           ],
         },
      }],
      ['node_use_etw=="true" and node_engine_v8==1',
      {
        'defines': ['HAVE_ETW=1'],
        'dependencies': ['node_etw'],
        'sources': [
          'src/platform/win/node_win32_etw_provider.h',
          'src/platform/win/node_win32_etw_provider-inl.h',
          'src/platform/win/node_win32_etw_provider.cc',
          'src/node_dtrace.cc',
          'tools/msvs/genfiles/node_etw_provider.h',
          'tools/msvs/genfiles/node_etw_provider.rc',
        ]
      }],
      ['node_use_perfctr=="true" and node_engine_v8==1',
      {
        'defines': ['HAVE_PERFCTR=1'],
        'dependencies': ['node_perfctr'],
        'sources': [
          'src/platform/win/node_win32_perfctr_provider.h',
          'src/platform/win/node_win32_perfctr_provider.cc',
          'src/node_counters.cc',
          'src/node_counters.h',
          'tools/msvs/genfiles/node_perfctr_provider.rc',
        ]
      }],
      ['node_no_sqlite==0',
      {        
        'defines': [ 'JXCORE_EMBEDS_SQLITE' ],
        'sources': [
          'deps/sqlite/sqlite3.h',
          'src/external/sqlite3/database.cc',
          'src/external/sqlite3/statement.cc',
          'src/external/sqlite3/node_sqlite3.cc',
        ],
        'dependencies': ['deps/sqlite/sqlite.gyp:sqlite'],
      }],
      ['node_embed_leveldown==1',
      {
        'defines': [ 'JXCORE_EMBEDS_LEVELDOWN' ],
        'dependencies': ['deps/leveldown-mobile/leveldown_embedded.gyp:leveldown'],
      }],
      ['node_shared_zlib=="false"',
      {
        'dependencies': ['deps/zlib/zlib.gyp:zlib'],
      }],
      ['node_shared_http_parser=="false"',
      {
        'dependencies': ['deps/http_parser/http_parser.gyp:http_parser'],
      }],
      ['node_shared_cares=="false"',
      {
        'dependencies': ['deps/cares/cares.gyp:cares'],
      }],
      ['node_shared_libuv=="false"',
      {
        'dependencies': ['deps/uv/uv.gyp:libuv'],
      }],
      ['OS=="win"',
      {
        'sources': [
          'src/res/node.rc',
        ],
        'defines': [
          'FD_SETSIZE=1024', # we need to use node 's preferred "win32" 
                             # rather than gyp's preferred "win"
          'PLATFORM="win32"',
          '_UNICODE=1',
        ],
        'libraries': ['-lpsapi.lib']
      },
      {#POSIX
          'defines': ['__POSIX__'],
          'ldflags': ['-pthread'],
      }],
      ['OS=="mac"', {
        'libraries': ['-framework Carbon'],
        '!defines': [
          'PLATFORM="darwin"'
        ],
        'defines': [# we need to use node 's preferred "darwin" 
                    # rather than gyp's preferred "mac"
          'PLATFORM="darwin"',
        ],
      }],
      ['OS=="ios" or OS=="android"',
      {
        'defines': ['__MOBILE_OS__']
      }],
      ['OS=="freebsd"',
      {
        'libraries': [
          '-lutil',
          '-lkvm',
        ]
      }],
      ['OS=="solaris"',
      {
        'libraries': [
          '-lkstat',
          '-lumem',
        ],
        'defines!': [
          'PLATFORM="solaris"'
        ],
        'defines': [# we need to use node 's preferred "sunos"
                    # rather than gyp 's preferred "solaris"
          'PLATFORM="sunos"'
        ],
      }],
      ['OS in "linux freebsd" and node_shared_v8=="false" and node_engine_mozilla!=1',
        {
          'ldflags': [
            '-Wl,--whole-archive <(V8_BASE) -Wl,--no-whole-archive',
          ],
        }
      ],
    ],
    'msvs_settings':
    {
      'VCLinkerTool':
      {
        'SubSystem': 1,
        #/subsystem:console
      },
    },
  }, #generate ETW header and resource files
  {
    'target_name': 'node_etw',
    'type': 'none',
    'conditions': [
      ['node_use_etw=="true" and node_has_winsdk=="true"',
      {
        'actions': [
        {
          'action_name': 'node_etw',
          'inputs': ['src/res/node_etw_provider.man'],
          'outputs': [
            'tools/msvs/genfiles/node_etw_provider.rc',
            'tools/msvs/genfiles/node_etw_provider.h',
            'tools/msvs/genfiles/node_etw_providerTEMP.BIN',
          ],
          'action': ['mc <@(_inputs) -h tools/msvs/genfiles -r tools/msvs/genfiles']
        }]
      }]
    ]
  }, #generate perf counter header and resource files
  {
    'target_name': 'node_perfctr',
    'type': 'none',
    'conditions': [
      ['node_use_perfctr=="true" and node_has_winsdk=="true"',
      {
        'actions': [
        {
          'action_name': 'node_perfctr_man',
          'inputs': ['src/res/node_perfctr_provider.man'],
          'outputs': [
            'tools/msvs/genfiles/node_perfctr_provider.h',
            'tools/msvs/genfiles/node_perfctr_provider.rc',
            'tools/msvs/genfiles/MSG00001.BIN',
          ],
          'action': ['ctrpp <@(_inputs) '
            '-o tools/msvs/genfiles/node_perfctr_provider.h '
            '-rc tools/msvs/genfiles/node_perfctr_provider.rc'
          ]
        }, ],
      }]
    ]
  },
  {
    'target_name': 'node_js2c',
    'type': 'none',
    'toolsets': ['host'],
    'conditions': [
      ['node_compress_internals!=1',
      {
        'actions': [
        {
          'action_name': 'node_js2c',
          'inputs': [
            '<@(library_files)',
            './config.gypi',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/node_natives.h',
          ],
          'conditions': [
            ['node_use_dtrace=="false"'
              ' and node_use_etw=="false"'
              ' and node_use_systemtap=="false"',
              {
                'inputs': ['src/macros.py']
              }
            ],
            ['node_use_perfctr=="false"',
            {
              'inputs': ['src/perfctr_macros.py']
            }]
          ],
          'action': [
            '<(python)',
            'tools/js2c.py',
            '<@(_outputs)',
            '<@(_inputs)',
          ]
        }, ],
      }]
    ],
  }, #end node_js2c
  {
    'target_name': 'jx_js2c',
    'type': 'none',
    'toolsets': ['cmp'],
    'conditions': [
      ['node_compress_internals==1',
      {
        'actions': [
        {
          'action_name': 'jx_js2c',
          'inputs': [
            '<@(library_files)',
            './config.gypi',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/jx_natives.h',
          ],
          'action': [
            'jx',
            'tools/minify.js',
            '<@(_outputs)',
            '<@(_inputs)',
          ],
        }, ],
      }]
    ],
  }, #end jx_js2c
  {
    'target_name': 'node_dtrace_header',
    'type': 'none',
    'conditions': [
      ['node_use_dtrace=="true"',
      {
        'actions': [
        {
          'action_name': 'node_dtrace_header',
          'inputs': ['src/node_provider.d'],
          'outputs': ['<(SHARED_INTERMEDIATE_DIR)/node_provider.h'],
          'action': ['dtrace', '-h', '-xnolibs', '-s', '<@(_inputs)',
            '-o', '<@(_outputs)'
          ]
        }]
      }]
    ]
  },
  {
    'target_name': 'node_systemtap_header',
    'type': 'none',
    'conditions': [
      ['node_use_systemtap=="true"',
      {
        'actions': [
        {
          'action_name': 'node_systemtap_header',
          'inputs': ['src/node_systemtap.d'],
          'outputs': ['<(SHARED_INTERMEDIATE_DIR)/node_systemtap.h'],
          'action': ['dtrace', '-h', '-C', '-s', '<@(_inputs)',
            '-o', '<@(_outputs)'
          ]
        }]
      }]
    ]
  },
  {
    'target_name': 'node_mdb',
    'type': 'none',
    'conditions': [
      [ 'node_use_mdb=="true"',
        {
          'dependencies': [ 'deps/mdb_v8/mdb_v8.gyp:mdb_v8' ],
          'actions': [
            {
              'action_name': 'node_mdb',
              'inputs': [ '<(PRODUCT_DIR)/obj.target/deps/mdb_v8/mdb_v8.so' ],
              'outputs': [ '<(PRODUCT_DIR)/obj.target/node/src/node_mdb.o' ],
              'conditions': [
                [ 'target_arch=="ia32"', {
                  'action': [ 'elfwrap', '-o', '<@(_outputs)', '<@(_inputs)' ],
                } ],
                [ 'target_arch=="x64"', {
                  'action': [ 'elfwrap', '-64', '-o', '<@(_outputs)', '<@(_inputs)' ],
                } ],
              ],
            },
          ],
        },
      ],
    ],
  },
  {
    'target_name': 'node_dtrace_provider',
    'type': 'none',
    'conditions': [
      ['node_use_dtrace=="true" and OS!="mac" and OS!="linux"',
      {
        'actions': [
        {
          'action_name': 'node_dtrace_provider_o',
          'inputs': [
            'src/node_provider.d',
            '<(OBJ_DIR)/node/src/node_dtrace.o'
          ],
          'outputs': [
            '<(OBJ_DIR)/node/src/node_dtrace_provider.o'
          ],
          'action': ['dtrace', '-G', '-xnolibs', '-s', '<@(_inputs)',
            '-o', '<@(_outputs)'
          ]
        }]
      }],
      [ 'node_use_dtrace=="true" and OS=="linux"', {
        'actions': [
        {
          'action_name': 'node_dtrace_provider_o',
          'inputs': [ 'src/node_provider.d' ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/node_dtrace_provider.o'
          ],
          'action': [
            'dtrace', '-C', '-G', '-s', '<@(_inputs)', '-o', '<@(_outputs)'
          ],
         }],
      }],
    ]
  },
  {
    'target_name': 'node_dtrace_ustack',
    'type': 'none',
    'conditions': [
      ['node_use_dtrace=="true" and OS!="mac" and OS!="linux" and node_engine_v8==1',
      {
        'actions': [
        {
          'action_name': 'node_dtrace_ustack_constants',
          'inputs': [
            '<(V8_BASE)'
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/v8constants.h'
          ],
          'action': [
            'tools/genv8constants.py',
            '<@(_outputs)',
            '<@(_inputs)'
          ]
        },
        {
          'action_name': 'node_dtrace_ustack',
          'inputs': [
            'src/v8ustack.d',
            '<(SHARED_INTERMEDIATE_DIR)/v8constants.h'
          ],
          'outputs': [
            '<(OBJ_DIR)/node/src/node_dtrace_ustack.o'
          ],
          'conditions': [
            ['target_arch=="ia32"',
            {
              'action': [
                'dtrace', '-32', '-I<(SHARED_INTERMEDIATE_DIR)', '-Isrc',
                '-C', '-G', '-s', 'src/v8ustack.d', '-o', '<@(_outputs)',
              ]
            }],
            ['target_arch=="x64"',
            {
              'action': [
                'dtrace', '-64', '-I<(SHARED_INTERMEDIATE_DIR)', '-Isrc',
                '-C', '-G', '-s', 'src/v8ustack.d', '-o', '<@(_outputs)',
              ]
            }],
          ]
        }]
      }],
    ]
  }]# end targets
}
