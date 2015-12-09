{
  "targets": [{
    "target_name": "debugger-agent",
    "type": "<(library)",
    "include_dirs": [
      "src",
      "include",
      "../../uv/include",

      # Private node.js folder and stuff needed to include from it
      "../../../src",
      "../../cares/include",
      "../../http_parser",
    ],
    "direct_dependent_settings": {
      "include_dirs": [
        "include",
      ],
    },
    'conditions': [
      [ 'gcc_version<=44', {
        # GCC versions <= 4.4 do not handle the aliasing in the queue
        # implementation, so disable aliasing on these platforms
        # to avoid subtle bugs
        'cflags': [ '-fno-strict-aliasing' ],
      }],
      [ 'node_engine_chakra==1', {
        "defines": ['JS_ENGINE_CHAKRA', 'V8_IS_3_28', 'USE_EDGEMODE_JSRT=1', 'BUILDING_CHAKRASHIM=1'],  
        "include_dirs" : [
          '../../chakrashim/include',
        ]
      }, 
      { # else if v8
        "include_dirs" : [
          "../v8/include",
        ],
      }
      ],
    ],
    "defines": [ 'JS_ENGINE_V8' ],
    "sources": [
      "src/agent.cc",
    ],
  }],
}
