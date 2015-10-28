{
  'targets': [{
    'target_name': 'uwp',
    'type': '<(library)',
    'defines': [ 'JS_ENGINE_CHAKRA=1', '_WIN32_WINNT=0x0A00', '_GNU_SOURCE',
      'JS_ENGINE_V8=1', 'V8_IS_3_28=1', 'USE_EDGEMODE_JSRT=1', 'WINONECORE=1' ],
    'sources': [
      'binding.gyp',
      'index.js',
      'src/node-async.h',
      'src/uwp.h',
      'src/uwp.cc',
    ],
    'conditions': [
      [ 'target_arch=="arm"', {
        'defines': [ '__arm__=1' ]
      }],
      [ 'node_win_onecore==1', {
        'libraries': [
          '-lchakrart.lib',
        ],
      }],
      [ 'node_win_onecore==0', {
        'libraries': [
          '-lchakrart.lib',
          '-lole32.lib',
          '-lversion.lib',
        ],
      }]
     ],
    'include_dirs': [
      '../../src',
      '../../src/jx',
      '../../src/wrappers',
      '../../src/jx/Proxy',
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
