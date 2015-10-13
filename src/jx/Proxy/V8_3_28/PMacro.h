// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_V8_PMACRO_H_
#define SRC_JX_PROXY_V8_PMACRO_H_

#include "util_node.h"
#include "V8Types.h"
#include "V8Environment.h"

#ifdef _WIN32
#define JXCORE_EXTERN(x) __declspec(dllexport) x

// for constructors
#define JXCORE_PUBLIC __declspec(dllexport)
#else
#define JXCORE_EXTERN(x) x
#define JXCORE_PUBLIC
#endif

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
  v8::HandleScope scope(p___args.GetIsolate());   \
  JS_DEFINE_STATE_MARKER_(p___args.GetIsolate()); \
  jxcore::PArguments args(p___args);

#define __JS_METHOD_BEGIN_COM()                                              \
  v8::HandleScope scope(p___args.GetIsolate());                              \
  node::commons* com = node::commons::getInstanceIso(p___args.GetIsolate()); \
  JS_DEFINE_STATE_MARKER_(p___args.GetIsolate());                            \
  jxcore::PArguments args(p___args);                                         \
  if (com->expects_reset) RETURN();

#define JS_METHOD(class_name, method_name)                       \
  void class_name::method_name(const JS_V8_ARGUMENT& p___args) { \
    ENGINE_LOG_THIS(#class_name, #method_name);                  \
  __JS_METHOD_BEGIN_COM()

#define JS_LOCAL_METHOD(method_name)                 \
  void method_name(const JS_V8_ARGUMENT& p___args) { \
    ENGINE_LOG_THIS("LOCAL", #method_name);          \
  __JS_METHOD_BEGIN_COM()

// IF node::commons *com is available from wrap use below
#define JS_METHOD_NO_COM(class_name, method_name)                \
  void class_name::method_name(const JS_V8_ARGUMENT& p___args) { \
    ENGINE_LOG_THIS(#class_name, #method_name);                  \
  __JS_METHOD_BEGIN_NO_COM()

#define JS_LOCAL_METHOD_NO_COM(method_name)          \
  void method_name(const JS_V8_ARGUMENT& p___args) { \
    ENGINE_LOG_THIS("LOCAL", #method_name);          \
  __JS_METHOD_BEGIN_NO_COM()

#define DEFINE_JS_METHOD(name) void name(const JS_V8_ARGUMENT& p___args)
#define DEFINE_JS_CLASS_METHOD(clss, name) \
  void clss::name(const JS_V8_ARGUMENT& p___args)

#define GET_ARG(x) p___args[x]

#define RETURN_PARAM(param)     \
  do {                          \
    args.SetReturnValue(param); \
    return;                     \
  } while (0)

#define RETURN_FROM(param) \
  do {                     \
    param;                 \
    return;                \
  } while (0)

#define RETURN_TRUE() RETURN_PARAM(STD_TO_BOOLEAN(true))
#define RETURN_FALSE() RETURN_PARAM(STD_TO_BOOLEAN(false))

#define RETURN_POINTER(param) RETURN_PARAM(param)

#define RETURN() \
  do {           \
    return;      \
  } while (0)

#define JS_METHOD_END }

#define JS_THROW_EXCEPTION_TYPE(x) __contextORisolate->ThrowException(x)
#define JS_THROW_EXCEPTION(x) __contextORisolate->ThrowException(x)

#define THROW_EXCEPTION(msg)                                           \
  do {                                                                 \
    JS_LOCAL_VALUE p___err = v8::Exception::Error(STD_TO_STRING(msg)); \
    JS_THROW_EXCEPTION_TYPE(p___err);                                  \
    RETURN();                                                          \
  } while (0)

#define THROW_EXCEPTION_NO_RETURN(msg) \
  JS_THROW_EXCEPTION_TYPE(v8::Exception::Error(STD_TO_STRING(msg)))
#define THROW_RANGE_EXCEPTION_NO_RETURN(msg) \
  JS_THROW_EXCEPTION_TYPE(v8::Exception::RangeError(STD_TO_STRING(msg)))
#define THROW_TYPE_EXCEPTION_NO_RETURN(msg) \
  JS_THROW_EXCEPTION_TYPE(v8::Exception::TypeError(STD_TO_STRING(msg)))

#define THROW_EXCEPTION_OBJECT(msg) \
  do {                              \
    JS_THROW_EXCEPTION_TYPE(msg);   \
    RETURN();                       \
  } while (0)

#define THROW_RANGE_EXCEPTION(msg)                                          \
  do {                                                                      \
    JS_LOCAL_VALUE p___err = v8::Exception::RangeError(STD_TO_STRING(msg)); \
    RETURN_PARAM(JS_THROW_EXCEPTION_TYPE(p___err));                         \
  } while (0)

#define THROW_TYPE_EXCEPTION(msg)                                          \
  do {                                                                     \
    JS_LOCAL_VALUE p___err = v8::Exception::TypeError(STD_TO_STRING(msg)); \
    JS_THROW_EXCEPTION_TYPE(p___err);                                      \
    RETURN();                                                              \
  } while (0)

#define INIT_NAMED_CLASS_MEMBERS(name, host)                   \
 public:                                                       \
  static void Initialize(JS_HANDLE_OBJECT target) {            \
    JS_ENTER_SCOPE_COM();                                      \
    JS_DEFINE_STATE_MARKER(com);                               \
    V8_T_LOCAL(V8_T_FUNCTION_TEMPLATE) constructor =           \
        v8::FunctionTemplate::New(__contextORisolate, New);    \
    constructor->InstanceTemplate()->SetInternalFieldCount(1); \
    constructor->SetClassName(STD_TO_STRING(#name));

#define INIT_NAMED_GROUP_CLASS_MEMBERS(name, host, host2)       \
 public:                                                        \
  static void Initialize(JS_HANDLE_OBJECT target) {             \
    JS_ENTER_SCOPE_COM();                                       \
    JS_DEFINE_STATE_MARKER(com);                                \
    V8_T_LOCAL(V8_T_FUNCTION_TEMPLATE) constructor1 =           \
        v8::FunctionTemplate::New(__contextORisolate, New);     \
    constructor1->InstanceTemplate()->SetInternalFieldCount(1); \
    constructor1->SetClassName(STD_TO_STRING(#name));           \
    V8_T_LOCAL(V8_T_FUNCTION_TEMPLATE) constructor2 =           \
        v8::FunctionTemplate::New(__contextORisolate, host2);   \
    constructor2->InstanceTemplate()->SetInternalFieldCount(1); \
    constructor2->SetClassName(STD_TO_STRING(#host2));

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
    JS_DEFINE_CURRENT_MARKER();                          \
    v8::HandleScope scope(JS_GET_STATE_MARKER());

#define END_INIT_MEMBERS }

#define END_INIT_NAMED_MEMBERS(name)                                     \
  JS_NAME_SET(target, STD_TO_STRING(#name), constructor->GetFunction()); \
  }

#define END_INIT_NAMED_GROUP_MEMBERS(name, name2)                          \
  JS_NAME_SET(target, STD_TO_STRING(#name), constructor1->GetFunction());  \
  JS_NAME_SET(target, STD_TO_STRING(#name2), constructor2->GetFunction()); \
  }

#define __SET_CLASS_METHOD(x, name, method, pcount) \
  node::NODE_SET_METHOD_(x, name, method)

#define SET_CLASS_METHOD(name, method, pcount) \
  __SET_CLASS_METHOD(constructor, name, method, pcount)
#define JS_SET_NAMED_METHOD(target, name, method, pcount) \
  __SET_CLASS_METHOD(target, name, method, pcount)

#define SET_INSTANCE_METHOD(name, method, pcount)                              \
  node::NODE_SET_PROTOTYPE_METHOD_(constructor, name, method)

#define JS_SETTER_CLASS_METHOD(clss, name)                         \
  void clss::name(JS_LOCAL_STRING property, JS_LOCAL_VALUE value,  \
                  const v8::PropertyCallbackInfo<void>& ___info) { \
    JS_ENTER_SCOPE_COM();                                          \
    JS_DEFINE_STATE_MARKER(com);

#define JS_SETTER_METHOD(name)                               \
  void name(JS_LOCAL_STRING property, JS_LOCAL_VALUE value,  \
            const v8::PropertyCallbackInfo<void>& ___info) { \
    JS_ENTER_SCOPE_COM();                                    \
    JS_HANDLE_OBJECT caller = ___info.Holder();              \
    JS_DEFINE_STATE_MARKER(com);

#define JS_SETTER_METHOD_END }

#define JS_SETADD_METHOD(name)                                    \
  void name(JS_LOCAL_STRING property, JS_LOCAL_VALUE value,       \
            const v8::PropertyCallbackInfo<v8::Value>& ___info) { \
    JS_ENTER_SCOPE_COM();                                         \
    JS_HANDLE_OBJECT caller = ___info.Holder();                   \
    JS_DEFINE_STATE_MARKER(com);

#define JS_SETADD_METHOD_END           \
  ___info.GetReturnValue().Set(value); \
  }

#define JS_DEFINE_SETTER_METHOD(name)                       \
  void name(JS_LOCAL_STRING property, JS_LOCAL_VALUE value, \
            const v8::PropertyCallbackInfo<void>& info)

#define JS_DEFINE_GETTER_METHOD(name) \
  void name(JS_LOCAL_STRING property, \
            const v8::PropertyCallbackInfo<v8::Value>& info)

#define DEFINE_JS_GETTER_DETAILS \
  JS_ENTER_SCOPE_COM();          \
  JS_DEFINE_STATE_MARKER(com);   \
  JS_HANDLE_OBJECT caller = ___info.Holder();

#define JS_GETTER_METHOD(name)                                    \
  void name(JS_LOCAL_STRING property,                             \
            const v8::PropertyCallbackInfo<v8::Value>& ___info) { \
  DEFINE_JS_GETTER_DETAILS

#define JS_GETTER_METHOD_(name) JS_GETTER_METHOD(name)

#define JS_GETTER_CLASS_METHOD(clss, name)                              \
  void clss::name(JS_LOCAL_STRING property,                             \
                  const v8::PropertyCallbackInfo<v8::Value>& ___info) { \
  DEFINE_JS_GETTER_DETAILS

#define JS_GETTER_METHOD_END }

#define JS_GETTER_METHOD_END_ JS_GETTER_METHOD_END

#define RETURN_GETTER_PARAM(x)       \
  {                                  \
    ___info.GetReturnValue().Set(x); \
    return;                          \
  }

#define RETURN_SETTER() return

#define JS_DELETER_METHOD(name)                                     \
  void name(JS_LOCAL_STRING property,                               \
            const v8::PropertyCallbackInfo<v8::Boolean>& ___info) { \
    node::commons* com = node::commons::getInstance();              \
    v8::HandleScope scope(com->node_isolate);                       \
    JS_DEFINE_STATE_MARKER(com);                                    \
    JS_HANDLE_OBJECT caller = ___info.Holder();

#define JS_DELETER_METHOD_END                          \
  ___info.GetReturnValue().Set(STD_TO_BOOLEAN(false)); \
  }

#define RETURN_DELETER_TRUE()                           \
  {                                                     \
    ___info.GetReturnValue().Set(STD_TO_BOOLEAN(true)); \
    return;                                             \
  }

#define RETURN_DELETER_FALSE()                           \
  {                                                      \
    ___info.GetReturnValue().Set(STD_TO_BOOLEAN(false)); \
    return;                                              \
  }

#define JS_WEAK_CALLBACK_METHOD(type, type2, name) \
  void name(const v8::WeakCallbackData<type, type2>& data)

#define JS_MAKE_WEAK(p, c, t) p.SetWeak(c, t)

namespace node {
#define NODE_PSYMBOL(s)  // NO USE

#define NODE_SYMBOL(s) STD_TO_STRING(s)
#define NODE_SYMBOL_ISO(iso, s) v8::String::New(iso, s)

/* Converts a unixtime to V8 Date */
#define NODE_UNIXTIME_V8(t) \
  v8::Date::New(__contextORisolate, 1000 * static_cast<double>(t))
#define NODE_V8_UNIXTIME(v) (static_cast<double>((v)->NumberValue()) / 1000.0);

#define NODE_DEFINE_CONSTANT(target, constant) \
  (target)->Set(STD_TO_STRING(#constant), STD_TO_NUMBER(constant))

inline void NODE_SET_PROTOTYPE_METHOD_(v8::Handle<v8::FunctionTemplate> recv,
                                       const char* name,
                                       v8::FunctionCallback callback) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Handle<v8::Signature> s = v8::Signature::New(isolate, recv);
  v8::Local<v8::FunctionTemplate> t =
      v8::FunctionTemplate::New(isolate, callback, v8::Handle<v8::Value>(), s);
  v8::Local<v8::Function> fn = t->GetFunction();
  recv->PrototypeTemplate()->Set(v8::String::NewFromUtf8(isolate, name), fn);
  v8::Local<v8::String> fn_name = v8::String::NewFromUtf8(isolate, name);
  fn->SetName(fn_name);
}

template <typename TypeName>
inline void NODE_SET_METHOD_(const TypeName& recv, const char* name,
                             v8::FunctionCallback callback) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::FunctionTemplate> t =
      v8::FunctionTemplate::New(isolate, callback);
  v8::Local<v8::Function> fn = t->GetFunction();
  v8::Local<v8::String> fn_name = v8::String::NewFromUtf8(isolate, name);
  fn->SetName(fn_name);
  recv->Set(fn_name, fn);
}
}  // namespace node

#define JS_METHOD_SET(obj, name, method) \
  node::NODE_SET_METHOD_(obj, name, method)

#define NODE_SET_METHOD(a, b, c) node::NODE_SET_METHOD_(a, b, c)
#define NODE_SET_PROTOTYPE_METHOD(a, b, c) \
  node::NODE_SET_PROTOTYPE_METHOD_(a, b, c)
#endif  // SRC_JX_PROXY_V8_PMACRO_H_
