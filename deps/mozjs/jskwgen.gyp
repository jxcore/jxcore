{
  'targets': [
    {
      'target_name': 'jskwgen',
      'type': 'executable',
      'sources': ['src/jskwgen.cpp'],
      'conditions':[
        ['OS=="mac"', {
          'defines': [
            'XP_MACOSX=1',
            'DARWIN=1',
          ],
          'xcode_settings': {
            'OTHER_CPLUSPLUSFLAGS' : ['-std=c++11', '-stdlib=libc++', 
              '-Wno-mismatched-tags', '-Wno-missing-field-initializers',
              '-Wno-unused-private-field', '-Wno-invalid-offsetof', '-Wno-ignored-qualifiers',
            ],
            'OTHER_CFLAGS' : ['-std=gnu99'],
            'XX_CPU_TARGET' : 'i386',
            'MACOSX_DEPLOYMENT_TARGET': '10.7',       # -mmacosx-version-min=10.7 
          }
        }],
      ]
    },
    {
      'target_name': 'jskwgen_ios',
      'type': 'none',
      'xcode_settings': {
        'XX_CPU_TARGET' : 'i386',
      }
    }
  ],
  'actions': [
  {
    'action_name': 'jskwgen',
    'inputs': [
      '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)jskwgen<(EXECUTABLE_SUFFIX)',
    ],
    'outputs': [
      '<(SHARED_INTERMEDIATE_DIR)/jsautokw.h',
    ],
    'action': [
      '<@(_inputs)',
      '<@(_outputs)',
    ],
  },
  {
    'action_name': 'jskwgen_ios',
    'action': [ # It's pretty ugly but GYP doesn't/wasn't support(ing) multi arch compilation. 
                # We need jskwgen is compiled for the host arch
      'c++ deps/mozjs/src/jskwgen.cpp -o out/Release/jskwgen'
    ]
  }
  ], 
}