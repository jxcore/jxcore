// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_PMACRO_H_
#define SRC_JX_PROXY_MOZILLA_PMACRO_H_

#include "MozTypes.h"
#include "MozEnvironment.h"

#ifdef _WIN32
#define JXCORE_PUBLIC __declspec(dllexport)
#else
#define JXCORE_PUBLIC
#endif

#define JS_CLASS_NEW_INSTANCE(obj_name, js_name)                             \
  JSObject *_##obj_name = JS_NewObjectForConstructorOLD(                     \
      JS_GET_STATE_MARKER(), js_name##_class_definition(), __argc, __jsval); \
  JS_LOCAL_OBJECT obj_name(_##obj_name, JS_GET_STATE_MARKER())

#define ENGINE_UNWRAP(type)                                              \
  type *wrap = static_cast<type *>(args.GetHolder());                    \
  if (!wrap) {                                                           \
    fprintf(stderr, #type ": Aborting due to unwrap failure at %s:%d\n", \
            __FILE__, __LINE__);                                         \
    abort();                                                             \
  }                                                                      \
  node::commons *com = wrap->com

#define ENGINE_UNWRAP_NO_ABORT(type)                  \
  type *wrap = static_cast<type *>(args.GetHolder()); \
  node::commons *com;                                 \
  if (!wrap)                                          \
    com = node::commons::getInstance();               \
  else                                                \
  node::commons *com = wrap->com

#define __JS_METHOD_BEGIN_NO_COM() \
  jxcore::PArguments args(JS_GET_STATE_MARKER(), __argc, __jsval);

#define __JS_METHOD_BEGIN_COM()                                    \
  node::commons *com = node::commons::getInstanceByThreadId(       \
      JS_GetThreadId(JS_GET_STATE_MARKER()));                      \
  jxcore::PArguments args(JS_GET_STATE_MARKER(), __argc, __jsval); \
  if (com->expects_reset) RETURN();

#define JS_METHOD(class_name, method_name)                                     \
  bool class_name::method_name(JSContext *__contextORisolate, unsigned __argc, \
                               JS::Value *__jsval) {                           \
    ENGINE_LOG_THIS(#class_name, #method_name);                                \
  __JS_METHOD_BEGIN_COM()

#define RETURN_TRUE() RETURN_PARAM(STD_TO_BOOLEAN(true))
#define RETURN_FALSE() RETURN_PARAM(STD_TO_BOOLEAN(false))

#define GET_ARG(x) args.GetItem(x)

#define RETURN_PARAM(param)       \
  do {                            \
    args.close(param);            \
    return args.get_ret_val_();   \
  } while (0)

#define RETURN_POINTER(param)       \
  do {                              \
    args.close(&param);             \
    return args.get_ret_val_();     \
  } while (0)

#define JS_LOCAL_METHOD(method_name)                               \
  bool method_name(JSContext *__contextORisolate, unsigned __argc, \
                   JS::Value *__jsval) {                           \
    ENGINE_LOG_THIS("LOCAL", #method_name);                        \
  __JS_METHOD_BEGIN_COM()

// IF node::commons *com is available from wrap use below
#define JS_METHOD_NO_COM(class_name, method_name)                              \
  bool class_name::method_name(JSContext *__contextORisolate, unsigned __argc, \
                               JS::Value *__jsval) {                           \
    ENGINE_LOG_THIS(#class_name, #method_name);                                \
  __JS_METHOD_BEGIN_NO_COM()

#define JS_LOCAL_METHOD_NO_COM(method_name)                        \
  bool method_name(JSContext *__contextORisolate, unsigned __argc, \
                   JS::Value *__jsval) {                           \
    ENGINE_LOG_THIS("LOCAL", #method_name);                        \
  __JS_METHOD_BEGIN_NO_COM()

#define DEFINE_JS_METHOD(name)                                 \
  bool name(JSContext *JS_GET_STATE_MARKER(), unsigned __argc, \
            JS::Value *__jsval)
#define DEFINE_JS_CLASS_METHOD(clss, name)                           \
  bool clss::name(JSContext *JS_GET_STATE_MARKER(), unsigned __argc, \
                  JS::Value *__jsval)

#define RETURN_PARAM_FALSE(param) \
  do {                            \
    args.close(param);            \
    return false;                 \
  } while (0)

#define RETURN()                \
  do {                          \
    args.close();               \
    return args.get_ret_val_(); \
  } while (0)

#define JS_METHOD_END         \
  args.close();               \
  return args.get_ret_val_(); \
  }

#define JS_THROW_EXCEPTION_TYPE(x) MozJS::ThrowException(x)
#define JS_THROW_EXCEPTION(x)            \
  do {                                   \
    JS_LOCAL_VALUE val_x = x;            \
    MozJS::ThrowException val_x_(val_x); \
  } while (0)

#define THROW_EXCEPTION(msg) \
  RETURN_PARAM_FALSE(        \
      JS_THROW_EXCEPTION_TYPE(MozJS::Exception::Error(STD_TO_STRING(msg))))

#define THROW_EXCEPTION_NO_RETURN(msg) \
  JS_THROW_EXCEPTION_TYPE(MozJS::Exception::Error(STD_TO_STRING(msg)));

#define THROW_RANGE_EXCEPTION_NO_RETURN(msg) \
  JS_THROW_EXCEPTION_TYPE(MozJS::Exception::RangeError(STD_TO_STRING(msg)));

#define THROW_TYPE_EXCEPTION_NO_RETURN(msg) \
  JS_THROW_EXCEPTION_TYPE(MozJS::Exception::TypeError(STD_TO_STRING(msg)));

#define THROW_EXCEPTION_OBJECT(msg)                                    \
  do {                                                                 \
    JS::RootedValue ____err(JS_GET_STATE_MARKER(), msg.GetRawValue()); \
    JS_SetPendingException(JS_GET_STATE_MARKER(), ____err);            \
    RETURN_PARAM_FALSE(JS_NULL());                                     \
  } while (0)

#define THROW_RANGE_EXCEPTION(msg)            \
  RETURN_PARAM_FALSE(JS_THROW_EXCEPTION_TYPE( \
      MozJS::Exception::RangeError(STD_TO_STRING(msg))))

#define THROW_TYPE_EXCEPTION(msg)             \
  RETURN_PARAM_FALSE(JS_THROW_EXCEPTION_TYPE( \
      MozJS::Exception::TypeError(STD_TO_STRING(msg))))

#define INIT_NAMED_CLASS_MEMBERS(name, host)                              \
 private:                                                                 \
  static JSClass *name##_class_definition() {                             \
    static JSClass name##_class_definition = {                            \
        #name, JSCLASS_HAS_PRIVATE |                                      \
                   JSCLASS_HAS_RESERVED_SLOTS(JS_OBJECT_SLOT_COUNT) |     \
                   JSCLASS_NEW_RESOLVE,                                   \
        JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub,          \
        JS_StrictPropertyStub, JS_EnumerateStub, JS_ResolveStub,          \
        JS_ConvertStub, MozJS::Value::empty_finalize, 0, 0, 0, 0};        \
                                                                          \
    return &name##_class_definition;                                      \
  }                                                                       \
                                                                          \
 public:                                                                  \
  static bool Initialize(JS_LOCAL_OBJECT &target) {                       \
    bool ___NO_NEW_INSTANCE = false;                                      \
    JS_DEFINE_STATE_MARKER_(MozJS::Isolate::GetCurrent()->GetRaw());      \
    MozJS::SuppressGC __gc__(__contextORisolate);                         \
    node::commons *com = node::commons::getInstanceByThreadId(            \
        JS_GetThreadId(JS_GET_STATE_MARKER()));                           \
    JS::RootedObject ___lnobject(JS_GET_STATE_MARKER(),                   \
                                 target.GetRawObjectPointer());           \
    JSObject *___target = JS_InitClass(                                   \
        JS_GET_STATE_MARKER(), ___lnobject, JS::NullPtr(),                \
        name##_class_definition(), host::New, 0, NULL, NULL, NULL, NULL); \
    JS_LOCAL_OBJECT constructor(___target, JS_GET_STATE_MARKER());

#define INIT_NAMED_GROUP_CLASS_MEMBERS(name, host, host2)                    \
 private:                                                                    \
  static JSClass *name##_class_definition() {                                \
    static JSClass name##_class_definition = {                               \
        #name, JSCLASS_HAS_PRIVATE |                                         \
                   JSCLASS_HAS_RESERVED_SLOTS(JS_OBJECT_SLOT_COUNT) |        \
                   JSCLASS_NEW_RESOLVE,                                      \
        JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub,             \
        JS_StrictPropertyStub, JS_EnumerateStub, JS_ResolveStub,             \
        JS_ConvertStub, MozJS::Value::empty_finalize, 0, 0, 0, 0};           \
                                                                             \
    return &name##_class_definition;                                         \
  }                                                                          \
                                                                             \
  static JSClass *host2##_class_definition() {                               \
    static JSClass host2##_class_definition = {                              \
        #host2, JSCLASS_HAS_PRIVATE |                                        \
                    JSCLASS_HAS_RESERVED_SLOTS(JS_OBJECT_SLOT_COUNT) |       \
                    JSCLASS_NEW_RESOLVE,                                     \
        JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub,             \
        JS_StrictPropertyStub, JS_EnumerateStub, JS_ResolveStub,             \
        JS_ConvertStub, MozJS::Value::empty_finalize, 0, 0, 0, 0};           \
                                                                             \
    return &host2##_class_definition;                                        \
  }                                                                          \
                                                                             \
 public:                                                                     \
  static bool Initialize(JS_LOCAL_OBJECT &target) {                          \
    bool ___NO_NEW_INSTANCE = false;                                         \
    JS_DEFINE_STATE_MARKER_(MozJS::Isolate::GetCurrent()->GetRaw());         \
    MozJS::SuppressGC __gc__(__contextORisolate);                            \
    node::commons *com = node::commons::getInstanceByThreadId(               \
        JS_GetThreadId(JS_GET_STATE_MARKER()));                              \
    JS::RootedObject ___lnobject(JS_GET_STATE_MARKER(),                      \
                                 target.GetRawObjectPointer());              \
    JSObject *___target1 = JS_InitClass(                                     \
        JS_GET_STATE_MARKER(), ___lnobject, JS::NullPtr(),                   \
        name##_class_definition(), host::New, 0, NULL, NULL, NULL, NULL);    \
    JS_LOCAL_OBJECT constructor1(___target1, JS_GET_STATE_MARKER());         \
    JSObject *___target2 = JS_InitClass(                                     \
        JS_GET_STATE_MARKER(), ___lnobject, JS::NullPtr(),                   \
        host2##_class_definition(), host::host2, 0, NULL, NULL, NULL, NULL); \
    JS_LOCAL_OBJECT constructor2(___target2, JS_GET_STATE_MARKER());

#define DECLARE_CLASS_INITIALIZER(name) void name(JS_HANDLE_OBJECT &target)

#define DEFINE_CLASS_INITIALIZER(clss, name) \
  void clss::name(JS_HANDLE_OBJECT &target)

#define INIT_CLASS_MEMBERS()                                         \
 public:                                                             \
  static bool Initialize(JS_LOCAL_OBJECT &constructor) {             \
    bool ___NO_NEW_INSTANCE = true;                                  \
    JS_DEFINE_STATE_MARKER_(MozJS::Isolate::GetCurrent()->GetRaw()); \
    MozJS::SuppressGC __gc__(__contextORisolate);                    \
    node::commons *com = node::commons::getInstanceByThreadId(       \
        JS_GetThreadId(JS_GET_STATE_MARKER()));

#define INIT_CLASS_MEMBERS_NO_COM()                                  \
 public:                                                             \
  static bool Initialize(JS_LOCAL_OBJECT &constructor) {             \
    bool ___NO_NEW_INSTANCE = true;                                  \
    JS_DEFINE_STATE_MARKER_(MozJS::Isolate::GetCurrent()->GetRaw()); \
    MozJS::SuppressGC __gc__(__contextORisolate);                    \
    JS_ENTER_SCOPE();

#define END_INIT_MEMBERS \
  return true;           \
  }

// we already set the member under the JS_Init call
#define END_INIT_NAMED_MEMBERS(name) \
  return true;                       \
  }

// we already set the member under the JS_Init call
#define END_INIT_NAMED_GROUP_MEMBERS(name, name2) \
  return true;                                    \
  }

#define __SET_CLASS_METHOD(x, name, method, pcount)               \
  do {                                                            \
    if (___NO_NEW_INSTANCE)                                       \
      x.SetStaticFunction(name, method, pcount);                  \
    else                                                          \
      x.GetConstructor().SetStaticFunction(name, method, pcount); \
  } while (0)

#define SET_CLASS_METHOD(name, method, pcount) \
  __SET_CLASS_METHOD(constructor, name, method, pcount)
#define JS_SET_NAMED_METHOD(mm, name, method, pcount) \
  __SET_CLASS_METHOD(mm, name, method, pcount)

#define SET_INSTANCE_METHOD(name, method, pcount)         \
  do {                                                    \
    JS_LOCAL_STRING name_string = STD_TO_STRING(name);    \
    constructor.SetProperty(name_string, method, pcount); \
  } while (0)

#define SETTER_BASE_DEFINITIONS()                            \
  node::commons *com = node::commons::getInstanceByThreadId( \
      JS_GetThreadId(JS_GET_STATE_MARKER()));                \
  JS::Value ___val = ___vp;                                  \
  JS_LOCAL_VALUE value(___val, JS_GET_STATE_MARKER());       \
  JS::RootedValue rv___idval(JS_GET_STATE_MARKER());         \
  JS_IdToValue(JS_GET_STATE_MARKER(), ___id, &rv___idval);   \
  JS::Value ___idval = rv___idval;                           \
  JS_LOCAL_OBJECT caller(___obj, JS_GET_STATE_MARKER());     \
  JS_LOCAL_VALUE property(___idval, JS_GET_STATE_MARKER());

#define JS_SETTER_CLASS_METHOD(clss, name)                               \
  bool clss::name(JSContext *JS_GET_STATE_MARKER(),                      \
                  JS::Handle<JSObject *> ___obj, JS::Handle<jsid> ___id, \
                  bool ___strict, JS::MutableHandle<JS::Value> ___vp) {  \
  SETTER_BASE_DEFINITIONS()

#define JS_SETTER_METHOD(name)                                               \
  bool name(JSContext *JS_GET_STATE_MARKER(), JS::Handle<JSObject *> ___obj, \
            JS::Handle<jsid> ___id, bool ___strict,                          \
            JS::MutableHandle<JS::Value> ___vp) {                            \
  SETTER_BASE_DEFINITIONS()

#define JS_SETTER_METHOD_END \
  return true;               \
  }

#define JS_SETADD_METHOD(name) JS_SETTER_METHOD(name)

#define JS_SETADD_METHOD_END \
  return true;               \
  }

#define JS_DEFINE_GETTER_METHOD(name)                                        \
  bool name(JSContext *JS_GET_STATE_MARKER(), JS::Handle<JSObject *> ___obj, \
            JS::Handle<jsid> ___id, JS::MutableHandle<JS::Value> ___vp)

#define JS_DEFINE_SETTER_METHOD(name)                                        \
  bool name(JSContext *JS_GET_STATE_MARKER(), JS::Handle<JSObject *> ___obj, \
            JS::Handle<jsid> ___id, bool ___strict,                          \
            JS::MutableHandle<JS::Value> ___vp)

#define GETTER_BASE_DEFINITIONS()                            \
  node::commons *com = node::commons::getInstanceByThreadId( \
      JS_GetThreadId(JS_GET_STATE_MARKER()));                \
  JS::RootedValue rv___val(JS_GET_STATE_MARKER());           \
  JS_IdToValue(JS_GET_STATE_MARKER(), ___id, &rv___val);     \
  JS::Value ___val = rv___val;                               \
  JS_LOCAL_OBJECT caller(___obj, JS_GET_STATE_MARKER());     \
  JS_LOCAL_VALUE property(___val, JS_GET_STATE_MARKER());

#define JS_GETTER_CLASS_METHOD(clss, _name)                               \
  bool clss::_name(JSContext *JS_GET_STATE_MARKER(),                      \
                   JS::Handle<JSObject *> ___obj, JS::Handle<jsid> ___id, \
                   JS::MutableHandle<JS::Value> ___vp) {                  \
  GETTER_BASE_DEFINITIONS()

#define JS_GETTER_METHOD(_name)                                               \
  bool _name(JSContext *JS_GET_STATE_MARKER(), JS::Handle<JSObject *> ___obj, \
             JS::Handle<jsid> ___id, JS::MutableHandle<JS::Value> ___vp) {    \
  GETTER_BASE_DEFINITIONS()

#define JS_GETTER_METHOD_END \
  return true;               \
  }

#define RETURN_GETTER_PARAM(x)  \
  do {                          \
    ___vp.set(x.GetRawValue()); \
    return true;                \
  } while (0)

#define RETURN_SETTER() return true

#define JS_DELETER_METHOD(name)                                               \
  static bool name(JSContext *JS_GET_STATE_MARKER(), JS::HandleObject ___obj, \
                   JS::HandleId ___id, bool *succeeded) {                     \
    JS::RootedValue rv___idval(JS_GET_STATE_MARKER());                        \
    JS_IdToValue(JS_GET_STATE_MARKER(), ___id, &rv___idval);                  \
    JS::Value ___idval = rv___idval;                                          \
    JS_LOCAL_OBJECT caller(___obj, JS_GET_STATE_MARKER());                    \
    MozJS::Value property(___idval, JS_GET_STATE_MARKER());

#define JS_DELETER_METHOD_END \
  *succeeded = true;          \
  return true;                \
  }

#define RETURN_DELETER_TRUE() \
  do {                        \
    *succeeded = true;        \
    return true;              \
  } while (0)

#define RETURN_DELETER_FALSE() \
  do {                         \
    *succeeded = false;        \
    return true;               \
  } while (0)

namespace node {
#define NODE_SYMBOL(s) STD_TO_STRING(s)
#define NODE_SYMBOL_ISO(iso, s) STD_TO_STRING(s)

// Converts a unixtime to V8 Date
#define NODE_UNIXTIME_V8(t)                                            \
  JS_LOCAL_OBJECT(JS_NewDateObjectMsec(JS_GET_STATE_MARKER(),          \
                                       1000 * static_cast<double>(t)), \
                  JS_GET_STATE_MARKER())

#define NODE_V8_UNIXTIME(v) (static_cast<double>((v).NumberValue()) / 1000.0);

#define NODE_DEFINE_CONSTANT(target, constant)                     \
  do {                                                             \
    JS::RootedValue ___##constant##_val(JS_GET_STATE_MARKER(),     \
                                        JS::Int32Value(constant)); \
    target.SetProperty(#constant, ___##constant##_val);            \
  } while (0)

//// for backwards compatibility
#define NODE_SET_PROTOTYPE_METHOD(a, b, c) a.SetProperty(b, c)
#define NODE_SET_METHOD(x, name, method) x.SetStaticFunction(name, method)

}  // namespace node

#define JS_METHOD_SET(obj, name, method) NODE_SET_METHOD(obj, name, method)
#endif  // SRC_JX_PROXY_MOZILLA_PMACRO_H_
