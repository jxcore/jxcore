// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_V8_V8ENVIRONMENT_H_
#define SRC_JX_PROXY_V8_V8ENVIRONMENT_H_

#include "V8Types.h"

#define JS_ENGINE_MARKER v8::Isolate *

#define JS_CURRENT_CONTEXT() V8_T_CONTEXT::GetCurrent()

#define JS_CURRENT_ENGINE() v8::Isolate::GetCurrent()

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

#define JS_DEFINE_STATE_MARKER(x)       \
  JS_ENGINE_MARKER __contextORisolate = \
      (x != NULL) ? x->node_isolate : JS_CURRENT_ENGINE()
#define JS_DEFINE_STATE_MARKER_(x) JS_ENGINE_MARKER __contextORisolate = x

#define JS_DEFINE_COM_AND_MARKER()                   \
  node::commons *com = node::commons::getInstance(); \
  JS_DEFINE_STATE_MARKER(com)

#endif  // SRC_JX_PROXY_V8_V8ENVIRONMENT_H_
