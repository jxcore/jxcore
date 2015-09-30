// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_NODE_VERSION_H_
#define SRC_NODE_VERSION_H_

#define NODE_MAJOR_VERSION 0
#define NODE_MINOR_VERSION 10
#define NODE_PATCH_VERSION 40

#define NODE_VERSION_IS_RELEASE 1

#define JXCORE_MAJOR_VERSION 3
#define JXCORE_MINOR_VERSION 1
#define JXCORE_PATCH_VERSION 0

#ifndef xNODE_TAG
#define xNODE_TAG ""
#endif

#ifndef NODE_STRINGIFY
#define NODE_STRINGIFY(n) NODE_STRINGIFY_HELPER(n)
#define NODE_STRINGIFY_HELPER(n) #n
#endif

#if NODE_VERSION_IS_RELEASE
#define NODE_VERSION_STRING                              \
  NODE_STRINGIFY(NODE_MAJOR_VERSION) "." NODE_STRINGIFY( \
      NODE_MINOR_VERSION) "." NODE_STRINGIFY(NODE_PATCH_VERSION) xNODE_TAG
#else
#define NODE_VERSION_STRING                                                \
  NODE_STRINGIFY(NODE_MAJOR_VERSION) "." NODE_STRINGIFY(                   \
      NODE_MINOR_VERSION) "." NODE_STRINGIFY(NODE_PATCH_VERSION) xNODE_TAG \
      "-pre"
#endif

#define JXCORE_VERSION_STRING                                   \
  "0." NODE_STRINGIFY(JXCORE_MAJOR_VERSION) "." NODE_STRINGIFY( \
      JXCORE_MINOR_VERSION) "." NODE_STRINGIFY(JXCORE_PATCH_VERSION)

#define NODE_VERSION "v" NODE_VERSION_STRING

#ifdef JS_ENGINE_MOZJS
#define MOZJS_VERSION 34
#define JXCORE_VERSION "v " JXCORE_VERSION_STRING
#else
#define JXCORE_VERSION "v " JXCORE_VERSION_STRING
#endif

#define NODE_VERSION_AT_LEAST(major, minor, patch)                  \
  (((major) < NODE_MAJOR_VERSION) ||                                \
   ((major) == NODE_MAJOR_VERSION&&(minor) < NODE_MINOR_VERSION) || \
   ((major) == NODE_MAJOR_VERSION&&(minor) ==                       \
    NODE_MINOR_VERSION&&(patch) <= NODE_PATCH_VERSION))

#endif  // SRC_NODE_VERSION_H_
