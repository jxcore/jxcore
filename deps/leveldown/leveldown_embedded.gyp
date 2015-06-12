#
# JXcore uses this file to embed Leveldown for mobile targets
#
{
    "targets": [{
      "target_name": "leveldown",
      'type': 'static_library',
      "conditions": [
          ["OS == 'win'", {
              "defines": [
                  "_HAS_EXCEPTIONS=0"
              ]
            , "msvs_settings": {
                  "VCCLCompilerTool": {
                      "RuntimeTypeInfo": "false"
                    , "EnableFunctionLevelLinking": "true"
                    , "ExceptionHandling": "2"
                    , "DisableSpecificWarnings": [ "4355", "4530" ,"4267", "4244", "4506" ]
                  }
              }
          }]
        , ['OS in "linux android mac ios"', {
            'cflags!': [ '-fno-tree-vrp' ],
          }],
          ['node_engine_mozilla!=1',
	      {
	        'defines': [
	          'JS_ENGINE_V8=1',
	        ],
	        'include_dirs': [
		      '../v8/include'
		    ],
	      },
	      {
	        'v8_use_snapshot%': 'false',
	        'defines': [
	          'JS_ENGINE_MOZJS=1'
	        ],
	        'include_dirs': [
		      '../mozjs/src'
		    ],
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
	          ['OS == "android"', {
	            "defines": [ 'OS_ANDROID', 'LEVELDB_PLATFORM_ANDROID' ]
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
	                  'IPHONEOS_SDK_VERSION': '8.1',
	                },
	                'defines': ['__IOS__']
	              }],
	              ['OS=="ios" and target_arch!="x64" and target_arch!="ia32"', {
	                'xcode_settings': { 'SDKROOT': 'iphoneos' },
	                'include_dirs': [
	                  '../../src/platform/ios_device', #ios device SDK doesn not have crt_externs.h
	                ],
	              }],
	              ['OS=="ios" and (target_arch=="x64" or target_arch=="ia32")', {
	                'xcode_settings': { 'SDKROOT': 'iphonesimulator' },
	              }],
	            ]
	          }],
	        ]
	      }],
        ],
        "dependencies": [
            "./deps/leveldb/leveldb.gyp:leveldb"
        ],
       "sources": [
            "src/batch.cc"
          , "src/batch_async.cc"
          , "src/database.cc"
          , "src/database_async.cc"
          , "src/iterator.cc"
          , "src/iterator_async.cc"
          , "src/leveldown.cc"
          , "src/leveldown_async.cc"
        ],
        'include_dirs': [
	      '../../src',
	      '../../src/jx',
	      '../../src/wrappers',
	      '../../src/jx/Proxy',
  	      '../../src/jx/Proxy/Mozilla',
  	      '../../src/jx/Proxy/V8',
  	      '../../src/jx/external',
	      '../uv/include',
	      '../cares/include',
	      '../http_parser',
	      '../openssl/openssl/include',
	      '../zlib',
	      '../mozjs/incs',
	    ],
    }]
}
