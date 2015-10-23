{
  'targets': [{
    'target_name': 'uwp',
    'type': 'static_library',
    'defines': [ 'JS_ENGINE_CHAKRA=1', 'JS_ENGINE_V8=1', 'V8_IS_3_28=1' ],
    'sources': [
      'binding.gyp',
      'index.js',
      'src/node-async.h',
      'src/uwp.h',
      'src/uwp.cc',
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
      '../chakrashim/include',
    ],
  }]
}
