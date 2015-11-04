// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_V8_V8ENVIRONMENT_H_
#define SRC_JX_PROXY_V8_V8ENVIRONMENT_H_

#include "V8Types.h"
#include "../console_log.h"

#define ENGINE_NS v8

typedef JS_HANDLE_VALUE (*JS_NATIVE_METHOD)(const JS_V8_ARGUMENT &args);
typedef void (*JS_FINALIZER_METHOD)(JS_HANDLE_VALUE_REF val, void *data);

#define JS_V8_PROPERTY_ARGS v8::AccessorInfo

#define JS_ENGINE_SCOPE(x, pass)                  \
  v8::Locker locker(x->node_isolate);             \
  if (pass) {                                     \
    x->node_isolate->Enter();                     \
  }                                               \
  v8::HandleScope handle_scope;                   \
  v8::Context::Scope context_scope(getContext()); \
  v8::Isolate *__contextORisolate = x->node_isolate

#define __JS_LOCAL_STRING JS_LOCAL_STRING
#define __JS_LOCAL_VALUE JS_LOCAL_VALUE

#define JS_ENGINE_MARKER v8::Isolate *

#define JS_CURRENT_CONTEXT() V8_T_CONTEXT::GetCurrent()

#define JS_CURRENT_ENGINE() v8::Isolate::GetCurrent()

#define JS_DEFINE_CURRENT_MARKER() \
  JS_ENGINE_MARKER __contextORisolate = JS_CURRENT_ENGINE()

#define JS_GET_GLOBAL() JS_CURRENT_CONTEXT()->Global()

#define JS_ENGINE_WRITE_UTF8_FLAGS \
  v8::String::HINT_MANY_WRITES_EXPECTED | v8::String::NO_NULL_TERMINATION

#define JS_FORCE_GC() v8::V8::LowMemoryNotification()

#define JS_TERMINATE_EXECUTION(mcom)                                      \
  do {                                                                    \
    node::commons *__mcom__ = node::commons::getInstanceByThreadId(mcom); \
    v8::V8::TerminateExecution(__mcom__->node_isolate);                   \
  } while (0)

#define JS_GET_UV_LOOP(index) node::commons::getInstanceByThreadId(index)->loop

#define JS_GET_ENGINE_DATA(x) x->GetData()

#define JS_ENTER_SCOPE() v8::HandleScope scope

#define JS_ENTER_SCOPE_WITH(x) v8::HandleScope scope

#define JS_ENTER_SCOPE_COM() \
  JS_ENTER_SCOPE();          \
  node::commons *com = node::commons::getInstanceByThreadId(scope.GetThreadId())

#define JS_ENTER_SCOPE_COM_WITH(x) \
  JS_ENTER_SCOPE_WITH(x);          \
  node::commons *com = node::commons::getInstanceIso(x)

#define JS_LEAVE_SCOPE(x) scope.Close(x)

#define JS_CREATE_NEW_ENGINE(x) v8::Isolate::New()

#define JS_CURRENT_ENGINE_DATA(x) x->GetData()

#define JS_SET_ENGINE_DATA(x, data) x->SetData(data)

#define JS_STATE_MARKER JS_ENGINE_MARKER __contextORisolate

#define JS_GET_STATE_MARKER() __contextORisolate

#define JS_MAKE_WEAK(a, b, c) a.MakeWeak(b, c)

#define JS_ENGINE_LOCKER()                                         \
  v8::Isolate *isolate =                                           \
      com != NULL ? com->node_isolate : v8::Isolate::GetCurrent(); \
  v8::Locker locker(isolate);                                      \
  v8::Isolate::Scope isolateScope(isolate);                        \
  v8::HandleScope scope

#define JS_ENGINE_INITIALIZE() v8::V8::Initialize()

#define JS_GET_HEAP_STATICS(x) v8::V8::GetHeapStatistics(x)

#define JS_DEFINE_STATE_MARKER(x)       \
  JS_ENGINE_MARKER __contextORisolate = \
      (x != NULL) ? x->node_isolate : JS_CURRENT_ENGINE()
#define JS_DEFINE_STATE_MARKER_(x) JS_ENGINE_MARKER __contextORisolate = x

#define JS_DEFINE_COM_AND_MARKER()                   \
  node::commons *com = node::commons::getInstance(); \
  JS_DEFINE_STATE_MARKER(com)

namespace node {
// this would have been a template function were it not for the fact that g++
// sometimes fails to resolve it...
#define THROW_ERROR(fun)                                                   \
  do {                                                                     \
    JS_ENTER_SCOPE();                                                      \
    return ENGINE_NS::ThrowException(fun(ENGINE_NS::String::New(errmsg))); \
  } while (0)

inline static JS_HANDLE_VALUE ThrowError(const char *errmsg) {
  THROW_ERROR(ENGINE_NS::Exception::Error);
}

inline static JS_HANDLE_VALUE ThrowTypeError(const char *errmsg) {
  THROW_ERROR(ENGINE_NS::Exception::TypeError);
}

inline static JS_HANDLE_VALUE ThrowRangeError(const char *errmsg) {
  THROW_ERROR(ENGINE_NS::Exception::RangeError);
}

JS_HANDLE_VALUE FromConstructorTemplate(JS_PERSISTENT_FUNCTION_TEMPLATE t,
                                        const JS_V8_ARGUMENT &args);
}
#endif  // SRC_JX_PROXY_V8_V8ENVIRONMENT_H_
