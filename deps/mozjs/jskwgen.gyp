{
  'targets': [{
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
            'MACOSX_DEPLOYMENT_TARGET': '10.7',       # -mmacosx-version-min=10.7 
          }
        }],
      ]}
  ],
  'actions': [{
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
  }], 
}