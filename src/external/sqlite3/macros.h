#ifndef NODE_SQLITE3_SRC_MACROS_H
#define NODE_SQLITE3_SRC_MACROS_H

const char* sqlite_code_string(int code);
const char* sqlite_authorizer_string(int type);

#define REQUIRE_ARGUMENTS(n)                          \
  if (args.Length() < (n)) {                          \
    THROW_TYPE_EXCEPTION("Expected " #n "arguments"); \
  }

#define REQUIRE_ARGUMENT_EXTERNAL(i, var)               \
  if (args.Length() <= (i) || !args[i]->IsExternal()) { \
    THROW_TYPE_EXCEPTION("Argument " #i " invalid");    \
  }                                                     \
  Local<External> var = Local<External>::Cast(args[i]);

#define REQUIRE_ARGUMENT_FUNCTION(i, var)                       \
  if (args.Length() <= (i) || !args[i]->IsFunction()) {         \
    THROW_TYPE_EXCEPTION("Argument " #i " must be a function"); \
  }                                                             \
  JS_LOCAL_FUNCTION var = args.GetAsFunction(i);

#define REQUIRE_ARGUMENT_STRING(i, var)                       \
  if (args.Length() <= (i) || !args.IsString(i)) {            \
    THROW_TYPE_EXCEPTION("Argument " #i " must be a string"); \
  }                                                           \
  jxcore::JXString var;                                           \
  args.GetString(i, &var)

#define OPTIONAL_ARGUMENT_FUNCTION(i, var)                        \
  JS_HANDLE_FUNCTION var;                                          \
  if (args.Length() > i && !args.IsUndefined(i)) {                \
    if (!args.IsFunction(i)) {                                    \
      THROW_TYPE_EXCEPTION("Argument " #i " must be a function"); \
    }                                                             \
    var = args.GetAsFunction(i);                                  \
  }

#define OPTIONAL_ARGUMENT_INTEGER(i, var, default)              \
  int var;                                                      \
  if (args.Length() <= (i)) {                                   \
    var = (default);                                            \
  } else if (args.IsInteger(i)) {                               \
    var = args.GetInt32(i);                                     \
  } else {                                                      \
    THROW_TYPE_EXCEPTION("Argument " #i " must be an integer"); \
  }

#define DEFINE_CONSTANT_INTEGER(target, constant, name) \
  JS_NAME_SET(target, JS_STRING_ID(#name), STD_TO_INTEGER(constant));

#define DEFINE_CONSTANT_STRING(target, constant, name) \
  JS_NAME_SET(target, JS_STRING_ID(#name), STD_TO_STRING(constant));

#define GET_STRING(source, name, property) \
  jxcore::JXString name(JS_GET_NAME(source, JS_STRING_ID(property)));

#define GET_INTEGER(source, name, property) \
  int name = JS_GET_NAME(source, JS_STRING_ID(property))->Int32Value();

#define EXCEPTION(msg, errno, name)                                      \
  std::string str_msg = sqlite_code_string(errno);                       \
  str_msg += ":";                                                        \
  str_msg += msg;                                                        \
  JS_LOCAL_STRING str_error = STD_TO_STRING(str_msg.c_str());            \
  JS_LOCAL_OBJECT name##_obj =                                           \
      JS_VALUE_TO_OBJECT(JS_NEW_ERROR_VALUE(str_error));                 \
  JS_NAME_SET(name##_obj, JS_STRING_ID("errno"), STD_TO_INTEGER(errno)); \
  JS_NAME_SET(name##_obj, JS_STRING_ID("code"),                          \
              STD_TO_STRING(sqlite_code_string(errno)));                 \
  JS_LOCAL_VALUE name = JS_TYPE_TO_LOCAL_VALUE(name##_obj)

#define EMIT_EVENT(obj, argc, argv)                                        \
  TRY_CATCH_CALL((obj),                                                    \
                 JS_CAST_FUNCTION(JS_GET_NAME(obj, JS_STRING_ID("emit"))), \
                 argc, argv);

#define TRY_CATCH_CALL(context, callback, argc, argv) \
  {                                                   \
    JS_LOCAL_FUNCTION fnc = callback;                 \
    JS_TRY_CATCH(try_catch);                          \
    fnc->Call((context), (argc), (argv));             \
    if (try_catch.HasCaught()) {                      \
      FatalException(try_catch);                      \
    }                                                 \
  }

#define TRY_CATCH_CALL_NO_PARAM(context, callback) \
  {                                                   \
    JS_LOCAL_FUNCTION fnc = callback;                 \
    JS_TRY_CATCH(try_catch);                          \
    JS_METHOD_CALL_NO_PARAM(fnc, context);             \
    if (try_catch.HasCaught()) {                      \
      FatalException(try_catch);                      \
    }                                                 \
  }

#define WORK_DEFINITION(name)                 \
  static DEFINE_JS_METHOD(name);              \
  static void Work_Begin##name(Baton* baton); \
  static void Work_##name(uv_work_t* req);    \
  static void Work_After##name(uv_work_t* req);

#define STATEMENT_BEGIN(type)                                                 \
  assert(baton);                                                              \
  assert(baton->stmt);                                                        \
  assert(!baton->stmt->locked);                                               \
  assert(!baton->stmt->finalized);                                            \
  assert(baton->stmt->prepared);                                              \
  baton->stmt->locked = true;                                                 \
  baton->stmt->db->pending++;                                                 \
  int status = uv_queue_work(uv_default_loop(), &baton->request, Work_##type, \
                             (uv_after_work_cb)Work_After##type);             \
  assert(status == 0);

#define STATEMENT_INIT(type)                   \
  type* baton = static_cast<type*>(req->data); \
  Statement* stmt = baton->stmt;

#define STATEMENT_END()      \
  assert(stmt->locked);      \
  assert(stmt->db->pending); \
  stmt->locked = false;      \
  stmt->db->pending--;       \
  stmt->Process();           \
  stmt->db->Process();       \
  delete baton;

#define DELETE_FIELD(field)               \
  if (field != NULL) {                    \
    switch ((field)->type) {              \
      case SQLITE_INTEGER:                \
        delete (Values::Integer*)(field); \
        break;                            \
      case SQLITE_FLOAT:                  \
        delete (Values::Float*)(field);   \
        break;                            \
      case SQLITE_TEXT:                   \
        delete (Values::Text*)(field);    \
        break;                            \
      case SQLITE_BLOB:                   \
        delete (Values::Blob*)(field);    \
        break;                            \
      case SQLITE_NULL:                   \
        delete (Values::Null*)(field);    \
        break;                            \
    }                                     \
  }

#endif
