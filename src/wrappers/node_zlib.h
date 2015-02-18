// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_NODE_ZLIB_H_
#define SRC_WRAPPERS_NODE_ZLIB_H_

#include "node.h"
namespace node {

class NodeZlib {
  static void DefinePrivateConstants(commons *com, JS_HANDLE_OBJECT target);

  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(Close);
  static DEFINE_JS_METHOD(Write);
  static DEFINE_JS_METHOD(Init);
  static DEFINE_JS_METHOD(Reset);

  INIT_NAMED_CLASS_MEMBERS(Zlib, NodeZlib) {
    NODE_SET_PROTOTYPE_METHOD(constructor, "write", Write);
    NODE_SET_PROTOTYPE_METHOD(constructor, "init", Init);
    NODE_SET_PROTOTYPE_METHOD(constructor, "close", Close);
    NODE_SET_PROTOTYPE_METHOD(constructor, "reset", Reset);

    // valid flush values.
    NODE_DEFINE_CONSTANT(target, Z_NO_FLUSH);
    NODE_DEFINE_CONSTANT(target, Z_PARTIAL_FLUSH);
    NODE_DEFINE_CONSTANT(target, Z_SYNC_FLUSH);
    NODE_DEFINE_CONSTANT(target, Z_FULL_FLUSH);
    NODE_DEFINE_CONSTANT(target, Z_FINISH);
    NODE_DEFINE_CONSTANT(target, Z_BLOCK);

    // return/error codes
    NODE_DEFINE_CONSTANT(target, Z_OK);
    NODE_DEFINE_CONSTANT(target, Z_STREAM_END);
    NODE_DEFINE_CONSTANT(target, Z_NEED_DICT);
    NODE_DEFINE_CONSTANT(target, Z_ERRNO);
    NODE_DEFINE_CONSTANT(target, Z_STREAM_ERROR);
    NODE_DEFINE_CONSTANT(target, Z_DATA_ERROR);
    NODE_DEFINE_CONSTANT(target, Z_MEM_ERROR);
    NODE_DEFINE_CONSTANT(target, Z_BUF_ERROR);
    NODE_DEFINE_CONSTANT(target, Z_VERSION_ERROR);

    NODE_DEFINE_CONSTANT(target, Z_NO_COMPRESSION);
    NODE_DEFINE_CONSTANT(target, Z_BEST_SPEED);
    NODE_DEFINE_CONSTANT(target, Z_BEST_COMPRESSION);
    NODE_DEFINE_CONSTANT(target, Z_DEFAULT_COMPRESSION);
    NODE_DEFINE_CONSTANT(target, Z_FILTERED);
    NODE_DEFINE_CONSTANT(target, Z_HUFFMAN_ONLY);
    NODE_DEFINE_CONSTANT(target, Z_RLE);
    NODE_DEFINE_CONSTANT(target, Z_FIXED);
    NODE_DEFINE_CONSTANT(target, Z_DEFAULT_STRATEGY);
    NODE_DEFINE_CONSTANT(target, ZLIB_VERNUM);

    DefinePrivateConstants(com, target);

    JS_NAME_SET(target, JS_STRING_ID("ZLIB_VERSION"),
                STD_TO_STRING(ZLIB_VERSION));
  }
  END_INIT_NAMED_MEMBERS(Zlib)
};
}  // namespace node

#endif  // SRC_WRAPPERS_NODE_ZLIB_H_
