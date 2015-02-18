// Copyright & License details are available under JXCORE_LICENSE file

#include "node_http_parser.h"

#include "node.h"
#include "node_buffer.h"

#include <string.h> /* strdup() */
#if !defined(_MSC_VER)
#include <strings.h> /* strcasecmp() */
#else
#define strcasecmp _stricmp
#endif
#include <stdlib.h> /* free() */

#include "jx/string_ptr.h"

namespace node {

#define HTTP_CB(name)                                 \
  static int name(http_parser* p_) {                  \
    Parser* self = container_of(p_, Parser, parser_); \
    return self->name##_();                           \
  }                                                   \
  int name##_()

#define HTTP_DATA_CB(name)                                          \
  static int name(http_parser* p_, const char* at, size_t length) { \
    Parser* self = container_of(p_, Parser, parser_);               \
    return self->name##_(at, length);                               \
  }                                                                 \
  int name##_(const char* at, size_t length)

static inline JS_HANDLE_STRING method_to_str(commons* com, unsigned short m) {
  JS_DEFINE_STATE_MARKER(com);
  switch (m) {
#define X(num, name, string) \
  case HTTP_##name:          \
    return STD_TO_STRING(#string);
    HTTP_METHOD_MAP(X)
#undef X
  }
  return STD_TO_STRING("UNKNOWN_METHOD");
}

#define PRE_INITED_CACHE_COUNT 32

class Parser : public ObjectWrap {
  friend class NodeHttpParser;

 public:
  Parser(enum http_parser_type type, bool boosted) : ObjectWrap() {
    boost_performance = boosted;
    Init(type);
  }

  ~Parser() {}

  HTTP_CB(on_message_begin) {
    num_fields_ = num_values_ = 0;
    url_.Reset();
    return 0;
  }

  HTTP_DATA_CB(on_url) {
    ENGINE_LOG_THIS("HttpParser", "on_url");
    url_.Update(at, length, false);
    return 0;
  }

  HTTP_DATA_CB(on_header_field) {
    ENGINE_LOG_THIS("HttpParser", "on_header_field");
    if (num_fields_ == num_values_) {
      // start of new field name
      num_fields_++;
      if (num_fields_ == PRE_INITED_CACHE_COUNT) {
        // ran out of space - flush to javascript land
        Flush();
        num_fields_ = 1;
        num_values_ = 0;
      }
      fields_[num_fields_ - 1].Reset();
    }

    assert(num_fields_ < (int)ARRAY_SIZE(fields_));
    assert(num_fields_ == num_values_ + 1);

    fields_[num_fields_ - 1].Update(at, length, boost_performance);

    return 0;
  }

  HTTP_DATA_CB(on_header_value) {
    if (num_values_ != num_fields_) {
      // start of new header value
      num_values_++;
      values_[num_values_ - 1].Reset();
    }

    assert(num_values_ < (int)ARRAY_SIZE(values_));
    assert(num_values_ == num_fields_);

    values_[num_values_ - 1].Update(at, length, false);

    return 0;
  }

  HTTP_CB(on_headers_complete) {
    ENGINE_LOG_THIS("HttpParser", "on_headers_complete");
    if (JS_IS_EMPTY(handle_)) return 0;

    JS_DEFINE_STATE_MARKER(com);

    JS_LOCAL_VALUE cb =
        JS_GET_NAME(handle_, JS_PREDEFINED_STRING(onHeadersComplete));

    if (!JS_IS_FUNCTION(cb)) return 0;

    JS_LOCAL_OBJECT message_info = JS_NEW_EMPTY_OBJECT();

    if (have_flushed_) {
      // Slow case, flush remaining headers.
      Flush();
    } else {
      // Fast case, pass headers and URL to JS land.
      JS_NAME_SET(message_info, JS_PREDEFINED_STRING(headers), CreateHeaders());
      if (parser_.type == HTTP_REQUEST)
        JS_NAME_SET(message_info, JS_PREDEFINED_STRING(url),
                    url_.ToString(false));
    }
    num_fields_ = num_values_ = 0;

    // METHOD
    if (parser_.type == HTTP_REQUEST) {
      switch (parser_.method) {  // eliminate hot cases
        case 1: {
          JS_NAME_SET(message_info, JS_PREDEFINED_STRING(method),
                      JS_TYPE_TO_LOCAL_VALUE(com->pstr_GET));
        } break;
        case 2: {
          JS_NAME_SET(message_info, JS_PREDEFINED_STRING(method),
                      JS_TYPE_TO_LOCAL_VALUE(com->pstr_HEAD));
        } break;
        case 3: {
          JS_NAME_SET(message_info, JS_PREDEFINED_STRING(method),
                      JS_TYPE_TO_LOCAL_VALUE(com->pstr_POST));
        } break;
        default: {
          JS_NAME_SET(message_info, JS_PREDEFINED_STRING(method),
                      method_to_str(com, parser_.method));
        }
      }
    } else {
      JS_NAME_SET(message_info, JS_PREDEFINED_STRING(statusCode),
                  STD_TO_INTEGER(parser_.status_code));
    }

    // VERSION
    JS_NAME_SET(message_info, JS_PREDEFINED_STRING(versionMajor),
                STD_TO_INTEGER(parser_.http_major));
    JS_NAME_SET(message_info, JS_PREDEFINED_STRING(versionMinor),
                STD_TO_INTEGER(parser_.http_minor));

    JS_LOCAL_VALUE bvalka = http_should_keep_alive(&parser_)
                                ? STD_TO_BOOLEAN(true)
                                : STD_TO_BOOLEAN(false);
    JS_NAME_SET(message_info, JS_PREDEFINED_STRING(shouldKeepAlive), bvalka);

    JS_LOCAL_VALUE bvalu =
        parser_.upgrade ? STD_TO_BOOLEAN(true) : STD_TO_BOOLEAN(false);
    JS_NAME_SET(message_info, JS_PREDEFINED_STRING(upgrade), bvalu);

    JS_CALL_PARAMS(argv, 1, JS_CORE_REFERENCE(message_info));

    JS_LOCAL_VALUE head_response =
        JS_METHOD_CALL(JS_CAST_FUNCTION(cb), handle_, 1, argv);

    if (JS_IS_EMPTY(head_response)) {
      got_exception_ = true;
      return -1;
    }

    return JS_IS_TRUE(head_response) ? 1 : 0;
  }

  HTTP_DATA_CB(on_body) {
    ENGINE_LOG_THIS("HttpParser", "on_body");
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(com);

    if (length == 0) {
      return 0;
    }

    JS_LOCAL_VALUE cb = JS_GET_NAME(handle_, JS_PREDEFINED_STRING(onBody));
    if (!JS_IS_FUNCTION(cb)) return 0;

    if (JS_HAS_NAME(handle_, JS_PREDEFINED_STRING(incoming))) {
      if (JS_IS_NULL(JS_GET_NAME(handle_, JS_PREDEFINED_STRING(incoming)))) {
        return 0;
      }
    }

    int offset = at - com->pa_current_buffer_data;

    if (boost_performance) {
      JS_CALL_PARAMS(
          argv, 2,
          JS_CORE_REFERENCE(JS_VALUE_TO_OBJECT(Buffer::New(
              com->pa_current_buffer_data + offset, length, com)->handle_)),
          JS_CORE_REFERENCE(STD_TO_INTEGER(length)));

      JS_LOCAL_VALUE r = JS_METHOD_CALL(JS_CAST_FUNCTION(cb), handle_, 2, argv);

      if (JS_IS_EMPTY(r)) {
        got_exception_ = true;
        return -1;
      }
    } else {  // node.js compatibility for custom Parser object
      JS_CALL_PARAMS(
          argv, 3,
          JS_CORE_REFERENCE(JS_VALUE_TO_OBJECT(Buffer::New(
              com->pa_current_buffer_data + offset, length, com)->handle_)),
          JS_CORE_REFERENCE(STD_TO_INTEGER(0)),
          JS_CORE_REFERENCE(STD_TO_INTEGER(length)));
      JS_LOCAL_VALUE r = JS_METHOD_CALL(JS_CAST_FUNCTION(cb), handle_, 3, argv);

      if (JS_IS_EMPTY(r)) {
        got_exception_ = true;
        return -1;
      }
    }

    return 0;
  }

  HTTP_CB(on_message_complete) {
    ENGINE_LOG_THIS("HttpParser", "on_message_complete");
    JS_ENTER_SCOPE();

    if (num_fields_) Flush();  // Flush trailing HTTP headers.

#ifdef JS_ENGINE_V8
    JS_LOCAL_VALUE cb =
        JS_GET_NAME(handle_, JS_PREDEFINED_STRING(onMessageComplete));

    if (!JS_IS_FUNCTION(cb)) return 0;

    JS_LOCAL_VALUE r = JS_METHOD_CALL_NO_PARAM(JS_CAST_FUNCTION(cb), handle_);
#else
    JS_LOCAL_VALUE r = JS_METHOD_CALL_NO_PARAM(handle_, "onMessageComplete");
#endif

    if (JS_IS_EMPTY(r)) {
      got_exception_ = true;
      return -1;
    }

    return 0;
  }

  void Save() {
    url_.Save();

    for (int i = 0; i < num_fields_; i++) {
      fields_[i].Save();
    }

    for (int i = 0; i < num_values_; i++) {
      values_[i].Save();
    }
  }

  static JS_LOCAL_METHOD(Execute) {
    ENGINE_LOG_THIS("HttpParser", "Execute");
    Parser* parser = ObjectWrap::Unwrap<Parser>(args.This());
    commons* com = parser->com;

    if (args.Length() == 4) {
      if (!args.IsUndefined(3) && !args.IsNull(3)) {
        http_parser_type type = static_cast<http_parser_type>(args.GetInt32(3));

        if (type != HTTP_REQUEST && type != HTTP_RESPONSE) {
          THROW_EXCEPTION(
              "Argument must be HTTPParser.REQUEST or HTTPParser.RESPONSE");
        }

        parser->Init(type);
      }
    }

    assert(!com->pab_current_buffer);
    assert(!com->pa_current_buffer_data);

    if (com->pab_current_buffer) {
      THROW_TYPE_EXCEPTION("Already parsing a buffer");
    }

    JS_LOCAL_VALUE buffer_v = GET_ARG(0);

    if (!Buffer::jxHasInstance(buffer_v, com)) {
      THROW_TYPE_EXCEPTION("Argument should be a buffer");
    }

    JS_LOCAL_OBJECT buffer_obj = JS_VALUE_TO_OBJECT(buffer_v);
    char* buffer_data = BUFFER__DATA(buffer_obj);
    size_t buffer_len = BUFFER__LENGTH(buffer_obj);

    size_t off = args.GetInt32(1);
    if (off >= buffer_len) {
      THROW_EXCEPTION("Offset is out of bounds");
    }

    size_t len = args.GetInt32(2);
    if (!Buffer::IsWithinBounds(off, len, buffer_len)) {
      THROW_EXCEPTION("off + len > buffer.length");
    }

    // Assign 'buffer_' while we parse. The callbacks will access that variable.
    com->pab_current_buffer = true;
    com->pa_current_buffer_data = buffer_data;
    com->pa_current_buffer_len = buffer_len;
    parser->got_exception_ = false;

    size_t nparsed = http_parser_execute(&parser->parser_, com->parser_settings,
                                         buffer_data + off, len);

    // Unassign the 'buffer_' variable
    assert(com->pab_current_buffer);
    // com->pa_current_buffer.Dispose();
    com->pab_current_buffer = false;

    com->pa_current_buffer_data = NULL;

    // If there was an exception in one of the callbacks
    if (parser->got_exception_) RETURN();

    JS_LOCAL_INTEGER nparsed_obj = STD_TO_INTEGER(nparsed);
    // If there was a parse error in one of the callbacks
    // TODO(?) What if there is an error on EOF?
    if (!parser->parser_.upgrade && nparsed != len) {
      enum http_errno err = HTTP_PARSER_ERRNO(&parser->parser_);

      JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(
          JS_NEW_ERROR_VALUE(STD_TO_STRING("Parse Error 01")));
      JS_NAME_SET(obj, JS_STRING_ID("bytesParsed"), nparsed_obj);
      JS_NAME_SET(obj, JS_STRING_ID("code"),
                  STD_TO_STRING(http_errno_name(err)));
      RETURN_PARAM(obj);
    } else {
      RETURN_POINTER(nparsed_obj);
    }
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD_NO_COM(Finish) {
    ENGINE_LOG_THIS("HttpParser", "Finish");
    Parser* parser = ObjectWrap::Unwrap<Parser>(args.This());

    commons* com = parser->com;
    assert(!com->pab_current_buffer);
    parser->got_exception_ = false;

    int rv =
        http_parser_execute(&(parser->parser_), com->parser_settings, NULL, 0);

    if (parser->got_exception_) RETURN();

    if (rv != 0) {
      enum http_errno err = HTTP_PARSER_ERRNO(&parser->parser_);

      JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(
          JS_NEW_ERROR_VALUE(STD_TO_STRING("Parse Error 02")));
      JS_NAME_SET(obj, JS_STRING_ID("bytesParsed"), STD_TO_INTEGER(0));
      JS_NAME_SET(obj, JS_STRING_ID("code"),
                  STD_TO_STRING(http_errno_name(err)));
      RETURN_PARAM(obj);
    }
  }
  JS_METHOD_END

  static JS_LOCAL_METHOD(Reinitialize) {
    ENGINE_LOG_THIS("HttpParser", "Reinitialize");
    http_parser_type type = static_cast<http_parser_type>(args.GetInt32(0));

    if (type != HTTP_REQUEST && type != HTTP_RESPONSE) {
      THROW_EXCEPTION(
          "Argument must be HTTPParser.REQUEST or HTTPParser.RESPONSE");
    }

    Parser* parser = ObjectWrap::Unwrap<Parser>(args.This());
    parser->Init(type);
  }
  JS_METHOD_END

  template <bool should_pause>
  static JS_LOCAL_METHOD(Pause) {
    Parser* parser = ObjectWrap::Unwrap<Parser>(args.This());
    http_parser_pause(&parser->parser_, should_pause);
  }
  JS_METHOD_END

  JS_LOCAL_ARRAY CreateHeaders() {
    ENGINE_LOG_THIS("HttpParser", "CreateHeaders");
    JS_DEFINE_STATE_MARKER(com);
    // num_values_ is either -1 or the entry # of the last header
    // so num_values_ == 0 means there's a single header
    JS_LOCAL_ARRAY headers = JS_NEW_ARRAY_WITH_COUNT(2 * num_values_);

    for (int i = 0; i < num_values_; ++i) {
      JS_INDEX_SET(headers, 2 * i, fields_[i].ToString(boost_performance));
      JS_INDEX_SET(headers, 2 * i + 1, values_[i].ToString(false));
    }

    return headers;
  }

  // spill headers and request path to JS land
  void Flush() {
    ENGINE_LOG_THIS("HttpParser", "Flush");
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(com);

    JS_CALL_PARAMS(argv, 2, JS_CORE_REFERENCE(CreateHeaders()),
                   JS_CORE_REFERENCE(url_.ToString(false)));

    JS_LOCAL_VALUE cb = JS_GET_NAME(handle_, JS_PREDEFINED_STRING(onHeaders));
    if (!JS_IS_FUNCTION(cb)) return;
    JS_LOCAL_VALUE r = JS_METHOD_CALL(JS_CAST_FUNCTION(cb), handle_, 2, argv);

    if (JS_IS_EMPTY(r)) got_exception_ = true;

    url_.Reset();
    have_flushed_ = true;
  }

  void Init(enum http_parser_type type) {
    http_parser_init(&parser_, type);
    url_.Reset();
    num_fields_ = 0;
    num_values_ = 0;
    have_flushed_ = false;
    got_exception_ = false;
    is_request_ = type == HTTP_REQUEST;
  }

  bool is_request_;
  http_parser parser_;
  StringPtr fields_[PRE_INITED_CACHE_COUNT];  // header fields
  StringPtr values_[PRE_INITED_CACHE_COUNT];  // header values
  StringPtr url_;
  int num_fields_;
  int num_values_;
  bool have_flushed_;
  bool got_exception_;
  commons* com;
  bool boost_performance;
};

JS_METHOD(NodeHttpParser, New) {
  http_parser_type type = static_cast<http_parser_type>(args.GetInteger(0));

  if (type != HTTP_REQUEST && type != HTTP_RESPONSE) {
    THROW_EXCEPTION(
        "Argument must be HTTPParser.REQUEST or HTTPParser.RESPONSE");
  }

  bool boost_performance = false;
  if (args.Length() > 1) {
    boost_performance = args.GetBoolean(1);
  }

  Parser* parser = new Parser(type, boost_performance);
  parser->com = com;

  for (int i = 0; i < PRE_INITED_CACHE_COUNT; i++) {
    parser->fields_[i].com = parser->com;
    parser->values_[i].com = parser->com;
  }

  parser->url_.com = parser->com;

  JS_CLASS_NEW_INSTANCE(obj, HTTPParser);
  parser->Wrap(obj);

  RETURN_POINTER(obj);
}
JS_METHOD_END

void NodeHttpParser::InitParserMembers(
    commons* com, JS_HANDLE_FUNCTION_TEMPLATE constructor) {
  JS_DEFINE_STATE_MARKER(com);

  NODE_SET_PROTOTYPE_METHOD(constructor, "execute", Parser::Execute);
  NODE_SET_PROTOTYPE_METHOD(constructor, "finish", Parser::Finish);
  NODE_SET_PROTOTYPE_METHOD(constructor, "reinitialize", Parser::Reinitialize);
  NODE_SET_PROTOTYPE_METHOD(constructor, "pause", Parser::Pause<true>);
  NODE_SET_PROTOTYPE_METHOD(constructor, "resume", Parser::Pause<false>);

  com->parser_settings->on_message_begin = Parser::on_message_begin;
  com->parser_settings->on_url = Parser::on_url;
  com->parser_settings->on_header_field = Parser::on_header_field;
  com->parser_settings->on_header_value = Parser::on_header_value;
  com->parser_settings->on_headers_complete = Parser::on_headers_complete;
  com->parser_settings->on_body = Parser::on_body;
  com->parser_settings->on_message_complete = Parser::on_message_complete;
}

JS_HANDLE_VALUE ExecuteDirect(node::commons* com,
                              JS_HANDLE_OBJECT_REF this_parser,
                              char* buffer_data, size_t buffer_len, size_t off,
                              size_t len, int* return_value) {
  JS_ENTER_SCOPE();
  JS_DEFINE_STATE_MARKER(com);

  Parser* parser = ObjectWrap::Unwrap<Parser>(this_parser);

  JS_LOCAL_VALUE parser_status =
      JS_GET_NAME(this_parser, JS_PREDEFINED_STRING(__ptype));

  bool parser_inited = false;
  if (!JS_IS_NULL(parser_status)) {
    http_parser_type type =
        static_cast<http_parser_type>(INTEGER_TO_STD(parser_status));

    if (type != HTTP_REQUEST) {  // HTTP_RESPONSE SKIP!
      return JS_LEAVE_SCOPE(JS_UNDEFINED());
    }

    parser->Init(type);
    parser_inited = true;
  } else {
    if (!parser->is_request_)
      return JS_LEAVE_SCOPE(JS_UNDEFINED());  // Handle slow reads async
  }

  assert(!com->pab_current_buffer);
  assert(!com->pa_current_buffer_data);

  if (com->pab_current_buffer) {
    return THROW_EXCEPTION_NO_RETURN("Already parsing a buffer");
  }

  if (off >= buffer_len) {
    return THROW_RANGE_EXCEPTION_NO_RETURN("Offset is out of bounds");
  }

  if (!Buffer::IsWithinBounds(off, len, buffer_len)) {
    return THROW_RANGE_EXCEPTION_NO_RETURN("off + len > buffer.length");
  }

  com->pab_current_buffer = true;
  com->pa_current_buffer_data = buffer_data;
  com->pa_current_buffer_len = buffer_len;
  parser->got_exception_ = false;

  size_t nparsed;

  if ((len < com->max_header_size) || (com->max_header_size == 0)) {
    nparsed = http_parser_execute(&parser->parser_, com->parser_settings,
                                  buffer_data + off, len);
  } else {
    nparsed = -2;
  }
  // Unassign the 'buffer_' variable
  assert(com->pab_current_buffer);
  // com->pa_current_buffer.Dispose();
  com->pab_current_buffer = false;
  com->pa_current_buffer_data = NULL;

  // If there was an exception in one of the callbacks
  if (parser->got_exception_) {
    if (parser_inited) {
      JS_NAME_SET(this_parser, JS_PREDEFINED_STRING(__ptype), JS_NULL());
    }
    return JS_LEAVE_SCOPE(JS_UNDEFINED());
  }

  JS_LOCAL_INTEGER nparsed_obj = STD_TO_INTEGER(nparsed);
  // If there was a parse error in one of the callbacks
  // TODO(?) What if there is an error on EOF?
  if (!parser->parser_.upgrade && nparsed != len && nparsed != -2) {
    enum http_errno err = HTTP_PARSER_ERRNO(&parser->parser_);
    JS_LOCAL_OBJECT obj =
        JS_VALUE_TO_OBJECT(JS_NEW_ERROR_VALUE(STD_TO_STRING("Parse Error 03")));
    JS_NAME_SET(obj, JS_STRING_ID("bytesParsed"), nparsed_obj);
    JS_NAME_SET(obj, JS_STRING_ID("code"), STD_TO_STRING(http_errno_name(err)));
    return JS_LEAVE_SCOPE(obj);
  } else {
    *(return_value) = nparsed;
    return JS_LEAVE_SCOPE(nparsed_obj);
  }
}

}  // namespace node

NODE_MODULE(node_http_parser, node::NodeHttpParser::Initialize)
