// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_NODE_HTTP_PARSER_H_
#define SRC_WRAPPERS_NODE_HTTP_PARSER_H_

#include "node.h"
#include "http_parser.h"

namespace node {

class NodeHttpParser {
  static void InitParserMembers(commons *com,
                                JS_HANDLE_FUNCTION_TEMPLATE constructor);

  static DEFINE_JS_METHOD(New);

  INIT_NAMED_CLASS_MEMBERS(HTTPParser, NodeHttpParser) {
#ifdef JS_ENGINE_V8
    v8::PropertyAttribute attrib =
        (v8::PropertyAttribute)(v8::ReadOnly | v8::DontDelete);
    constructor->Set(STD_TO_STRING("REQUEST"), STD_TO_INTEGER(HTTP_REQUEST),
                     attrib);
    constructor->Set(STD_TO_STRING("RESPONSE"), STD_TO_INTEGER(HTTP_RESPONSE),
                     attrib);
#elif defined(JS_ENGINE_MOZJS)
    JS_NAME_SET(constructor.GetConstructor(), JS_STRING_ID("REQUEST"),
                STD_TO_INTEGER(HTTP_REQUEST));
    JS_NAME_SET(constructor.GetConstructor(), JS_STRING_ID("RESPONSE"),
                STD_TO_INTEGER(HTTP_RESPONSE));
#endif

    InitParserMembers(com, constructor);
  }
  END_INIT_NAMED_MEMBERS(HTTPParser)
};

// node_http_parser
JS_HANDLE_VALUE ExecuteDirect(node::commons *com,
                              JS_HANDLE_OBJECT_REF this_parser,
                              char *buffer_data, size_t buffer_len, size_t off,
                              size_t len, int *return_value);
}  // namespace node

#endif  // SRC_WRAPPERS_NODE_HTTP_PARSER_H_
