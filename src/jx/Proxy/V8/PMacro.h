// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_V8_PMACRO_H_
#define SRC_JX_PROXY_V8_PMACRO_H_

#include "V8Types.h"
#include "V8Environment.h"

#define JS_CLASS_NEW_INSTANCE(obj_name, js_name) \
  assert(args.IsConstructCall());                \
  JS_LOCAL_OBJECT obj_name = args.This()->ToObject()

#define ENGINE_UNWRAP(type)                                              \
  type* wrap = static_cast<type*>(args.GetHolder());                     \
  if (!wrap) {                                                           \
    fprintf(stderr, #type ": Aborting due to unwrap failure at %s:%d\n", \
            __FILE__, __LINE__);                                         \
    abort();                                                             \
  }                                                                      \
  node::commons* com = wrap->com

#define ENGINE_UNWRAP_NO_ABORT(type)                 \
  type* wrap = static_cast<type*>(args.GetHolder()); \
  node::commons* com;                                \
  if (!wrap)                                         \
    com = node::commons::getInstance();              \
  else                                               \
  node::commons* com = wrap->com

#define __JS_METHOD_BEGIN_NO_COM()                \
  JS_ENTER_SCOPE();                               \
  JS_DEFINE_STATE_MARKER_(p___args.GetIsolate()); \
  jxcore::PArguments args(p___args);

#define __JS_METHOD_BEGIN_COM()                   \
  JS_ENTER_SCOPE_COM();                           \
  JS_DEFINE_STATE_MARKER_(p___args.GetIsolate()); \
  jxcore::PArguments args(p___args);              \
  if (com->expects_reset) RETURN();

#define JS_METHOD(class_name, method_name)                                 \
  JS_HANDLE_VALUE class_name::method_name(const v8::Arguments& p___args) { \
    ENGINE_LOG_THIS(#class_name, #method_name);                            \
  __JS_METHOD_BEGIN_COM()

#define JS_LOCAL_METHOD(method_name)                           \
  JS_HANDLE_VALUE method_name(const v8::Arguments& p___args) { \
    ENGINE_LOG_THIS("LOCAL", #method_name);                    \
  __JS_METHOD_BEGIN_COM()

// IF node::commons *com is available from wrap use below
#define JS_METHOD_NO_COM(class_name, method_name)                          \
  JS_HANDLE_VALUE class_name::method_name(const v8::Arguments& p___args) { \
    ENGINE_LOG_THIS(#class_name, #method_name);                            \
  __JS_METHOD_BEGIN_NO_COM()

#define JS_LOCAL_METHOD_NO_COM(method_name)                    \
  JS_HANDLE_VALUE method_name(const v8::Arguments& p___args) { \
    ENGINE_LOG_THIS("LOCAL", #method_name);                    \
  __JS_METHOD_BEGIN_NO_COM()

#define DEFINE_JS_METHOD(name) \
  JS_HANDLE_VALUE name(const v8::Arguments& p___args)
#define DEFINE_JS_CLASS_METHOD(clss, name) \
  JS_HANDLE_VALUE clss::name(const v8::Arguments& p___args)

#define RETURN_TRUE() RETURN_PARAM(STD_TO_BOOLEAN(true))
#define RETURN_FALSE() RETURN_PARAM(STD_TO_BOOLEAN(false))

#define GET_ARG(x) p___args[x]

#define RETURN_PARAM(param)       \
  do {                            \
    return JS_LEAVE_SCOPE(param); \
  } while (0)

#define RETURN_POINTER(param) RETURN_PARAM(param)

#define RETURN()                           \
  do {                                     \
    return JS_LEAVE_SCOPE(JS_UNDEFINED()); \
  } while (0)

#define JS_METHOD_END                    \
  return JS_LEAVE_SCOPE(JS_UNDEFINED()); \
  }

#define JS_THROW_EXCEPTION_TYPE(x) v8::ThrowException(x)
#define JS_THROW_EXCEPTION(x) v8::ThrowException(x)

#define THROW_EXCEPTION(msg)                                           \
  do {                                                                 \
    JS_LOCAL_VALUE p___err = v8::Exception::Error(STD_TO_STRING(msg)); \
    RETURN_PARAM(JS_THROW_EXCEPTION_TYPE(p___err));                    \
  } while (0)

#define THROW_EXCEPTION_NO_RETURN(msg) \
  JS_THROW_EXCEPTION_TYPE(v8::Exception::Error(STD_TO_STRING(msg)))
#define THROW_RANGE_EXCEPTION_NO_RETURN(msg) \
  JS_THROW_EXCEPTION_TYPE(v8::Exception::RangeError(STD_TO_STRING(msg)))
#define THROW_TYPE_EXCEPTION_NO_RETURN(msg) \
  JS_THROW_EXCEPTION_TYPE(v8::Exception::TypeError(STD_TO_STRING(msg)))

#define THROW_EXCEPTION_OBJECT(msg) RETURN_PARAM(JS_THROW_EXCEPTION_TYPE(msg))

#define THROW_RANGE_EXCEPTION(msg)                                          \
  do {                                                                      \
    JS_LOCAL_VALUE p___err = v8::Exception::RangeError(STD_TO_STRING(msg)); \
    RETURN_PARAM(JS_THROW_EXCEPTION_TYPE(p___err));                         \
  } while (0)

#define THROW_TYPE_EXCEPTION(msg)                                          \
  do {                                                                     \
    JS_LOCAL_VALUE p___err = v8::Exception::TypeError(STD_TO_STRING(msg)); \
    RETURN_PARAM(JS_THROW_EXCEPTION_TYPE(p___err));                        \
  } while (0)

#define INIT_NAMED_CLASS_MEMBERS(name, host)                   \
 public:                                                       \
  static void Initialize(JS_HANDLE_OBJECT target) {            \
    JS_ENTER_SCOPE_COM();                                      \
    V8_T_LOCAL(V8_T_FUNCTION_TEMPLATE) constructor =           \
        v8::FunctionTemplate::New(New);                        \
    constructor->InstanceTemplate()->SetInternalFieldCount(1); \
    constructor->SetClassName(v8::String::NewSymbol(#name));   \
    JS_DEFINE_STATE_MARKER(com);

#define INIT_NAMED_GROUP_CLASS_MEMBERS(name, host, host2)       \
 public:                                                        \
  static void Initialize(JS_HANDLE_OBJECT target) {             \
    JS_ENTER_SCOPE_COM();                                       \
    V8_T_LOCAL(V8_T_FUNCTION_TEMPLATE) constructor1 =           \
        v8::FunctionTemplate::New(New);                         \
    constructor1->InstanceTemplate()->SetInternalFieldCount(1); \
    constructor1->SetClassName(v8::String::NewSymbol(#name));   \
    V8_T_LOCAL(V8_T_FUNCTION_TEMPLATE) constructor2 =           \
        v8::FunctionTemplate::New(host2);                       \
    constructor2->InstanceTemplate()->SetInternalFieldCount(1); \
    constructor2->SetClassName(v8::String::NewSymbol(#host2));  \
    JS_DEFINE_STATE_MARKER(com);

#define DECLARE_CLASS_INITIALIZER(name) void name(JS_HANDLE_OBJECT target)

#define DEFINE_CLASS_INITIALIZER(clss, name) \
  void clss::name(JS_HANDLE_OBJECT target)

#define INIT_CLASS_MEMBERS()                             \
 public:                                                 \
  static void Initialize(JS_HANDLE_OBJECT constructor) { \
    JS_ENTER_SCOPE_COM();                                \
    JS_DEFINE_STATE_MARKER(com);

#define INIT_CLASS_MEMBERS_NO_COM()                      \
 public:                                                 \
  static void Initialize(JS_HANDLE_OBJECT constructor) { \
    JS_ENTER_SCOPE();

#define END_INIT_MEMBERS }

#define END_INIT_NAMED_MEMBERS(name)                \
  JS_NAME_SET(target, v8::String::NewSymbol(#name), \
              constructor->GetFunction());          \
  }

#define END_INIT_NAMED_GROUP_MEMBERS(name, name2)    \
  JS_NAME_SET(target, v8::String::NewSymbol(#name),  \
              constructor1->GetFunction());          \
  JS_NAME_SET(target, v8::String::NewSymbol(#name2), \
              constructor2->GetFunction());          \
  }

#define __SET_CLASS_METHOD(x, name, method, pcount) \
  JS_NAME_SET(x, v8::String::NewSymbol(name),       \
              v8::FunctionTemplate::New(method)->GetFunction())

#define SET_CLASS_METHOD(name, method, pcount) \
  __SET_CLASS_METHOD(constructor, name, method, pcount)
#define JS_SET_NAMED_METHOD(target, name, method, pcount) \
  __SET_CLASS_METHOD(target, name, method, pcount)

#define SET_INSTANCE_METHOD(name, method, pcount)                              \
  do {                                                                         \
    V8_T_LOCAL(V8_T_FUNCTION_TEMPLATE) templ =                                 \
        v8::FunctionTemplate::New(method);                                     \
    JS_NAME_SET(constructor->PrototypeTemplate(), v8::String::NewSymbol(name), \
                templ);                                                        \
  } while (0)

#define JS_SETTER_CLASS_METHOD(clss, name)                        \
  void clss::name(JS_LOCAL_STRING property, JS_LOCAL_VALUE value, \
                  const v8::AccessorInfo& ___info) {              \
    JS_ENTER_SCOPE_COM();                                         \
    JS_DEFINE_STATE_MARKER(com);

#define JS_SETTER_METHOD(name)                              \
  void name(JS_LOCAL_STRING property, JS_LOCAL_VALUE value, \
            const v8::AccessorInfo& ___info) {              \
    JS_ENTER_SCOPE_COM();                                   \
    JS_HANDLE_OBJECT caller = ___info.Holder();             \
    JS_DEFINE_STATE_MARKER(com);

#define JS_SETTER_METHOD_END }

#define JS_SETADD_METHOD(name)                                         \
  JS_HANDLE_VALUE name(JS_LOCAL_STRING property, JS_LOCAL_VALUE value, \
                       const v8::AccessorInfo& ___info) {              \
    JS_ENTER_SCOPE_COM();                                              \
    JS_HANDLE_OBJECT caller = ___info.Holder();                        \
    JS_DEFINE_STATE_MARKER(com);

#define JS_SETADD_METHOD_END    \
  return JS_LEAVE_SCOPE(value); \
  }

#define JS_DEFINE_SETTER_METHOD(name)                       \
  void name(JS_LOCAL_STRING property, JS_LOCAL_VALUE value, \
            const v8::AccessorInfo& info)

#define JS_DEFINE_GETTER_METHOD(name) \
  JS_HANDLE_VALUE name(JS_LOCAL_STRING property, const v8::AccessorInfo& info)

#define DEFINE_JS_GETTER_DETAILS \
  JS_ENTER_SCOPE_COM();          \
  JS_DEFINE_STATE_MARKER(com);   \
  JS_HANDLE_OBJECT caller = ___info.Holder();

#define JS_GETTER_METHOD(name)                            \
  JS_HANDLE_VALUE name(JS_LOCAL_STRING property,          \
                       const v8::AccessorInfo& ___info) { \
  DEFINE_JS_GETTER_DETAILS

#define JS_GETTER_METHOD_(name) JS_GETTER_METHOD(name)

#define JS_GETTER_CLASS_METHOD(clss, name)                      \
  JS_HANDLE_VALUE clss::name(JS_LOCAL_STRING property,          \
                             const v8::AccessorInfo& ___info) { \
  DEFINE_JS_GETTER_DETAILS

#define JS_GETTER_METHOD_END             \
  return JS_LEAVE_SCOPE(JS_UNDEFINED()); \
  }

#define JS_GETTER_METHOD_END_ JS_GETTER_METHOD_END

#define RETURN_GETTER_PARAM(x) RETURN_PARAM(x)

#define RETURN_SETTER() return

#define JS_DELETER_METHOD(name)                             \
  JS_HANDLE_BOOLEAN name(JS_LOCAL_STRING property,          \
                         const v8::AccessorInfo& ___info) { \
    JS_ENTER_SCOPE_COM();                                   \
    JS_DEFINE_STATE_MARKER(com);                            \
    JS_HANDLE_OBJECT caller = ___info.Holder();

#define JS_DELETER_METHOD_END                   \
  return JS_LEAVE_SCOPE(STD_TO_BOOLEAN(false)); \
  }

#define RETURN_DELETER_TRUE() return JS_LEAVE_SCOPE(STD_TO_BOOLEAN(true))
#define RETURN_DELETER_FALSE() return JS_LEAVE_SCOPE(STD_TO_BOOLEAN(false))

namespace node {
#define NODE_PSYMBOL(s) JS_PERSISTENT_STRING::New(v8::String::NewSymbol(s))

#define NODE_SYMBOL(s) v8::String::New(s)
#define NODE_SYMBOL_ISO(iso, s) v8::String::New(iso, s)

/* Converts a unixtime to V8 Date */
#define NODE_UNIXTIME_V8(t) v8::Date::New(1000 * static_cast<double>(t))
#define NODE_V8_UNIXTIME(v) (static_cast<double>((v)->NumberValue()) / 1000.0);

#define NODE_DEFINE_CONSTANT(target, constant)                           \
  (target)                                                               \
      ->Set(v8::String::NewSymbol(#constant), v8::Number::New(constant), \
            static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete))

template <typename target_t>
void SetMethod(target_t obj, const char* name,
               v8::InvocationCallback callback) {
  obj->Set(v8::String::NewSymbol(name),
           v8::FunctionTemplate::New(callback)->GetFunction());
}

template <typename target_t>
void SetPrototypeMethod(target_t target, const char* name,
                        v8::InvocationCallback callback) {
  v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(callback);
  target->PrototypeTemplate()->Set(v8::String::NewSymbol(name), templ);
}

// for backwards compatibility
#define NODE_SET_METHOD node::SetMethod
#define NODE_SET_PROTOTYPE_METHOD node::SetPrototypeMethod
}  // namespace node

#define JS_METHOD_SET(obj, name, method) NODE_SET_METHOD(obj, name, method)
#endif  // SRC_JX_PROXY_V8_PMACRO_H_
