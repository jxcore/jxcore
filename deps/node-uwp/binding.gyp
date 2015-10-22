{
  'targets': [{
    'target_name': 'uwp',
    "include_dirs" : [
      "<!(node -e \"require('nan')\")"
    ],
    'sources': [
      'binding.gyp',
      'index.js',
      'package.json',
      'src/node-async.h',
      'src/uwp.h',
      'src/uwp.cc',
    ],
  }]
}
