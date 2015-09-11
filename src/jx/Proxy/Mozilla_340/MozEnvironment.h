// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_MOZENVIRONMENT_H_
#define SRC_JX_PROXY_MOZILLA_MOZENVIRONMENT_H_

#define ENGINE_NS MozJS

typedef bool (*JS_NATIVE_METHOD)(JSContext *ctx, unsigned argc, JS::Value *val);

#define JS_ENGINE_SCOPE(x, pass) \
  JS_ENTER_SCOPE();              \
  JS_DEFINE_STATE_MARKER(x)

#define __JS_LOCAL_STRING const char *
#define __JS_LOCAL_VALUE jsval

#define JS_ENGINE_MARKER MozJS::Isolate *

#define JS_CURRENT_ENGINE() MozJS::Isolate::GetCurrent()

#define JS_CURRENT_CONTEXT() JS_CURRENT_ENGINE()->GetRaw()  // JSContext *

#define JS_GET_GLOBAL() jxcore::getGlobal(JS_GetThreadId(__contextORisolate))

#define JS_ENGINE_WRITE_UTF8_FLAGS 1

#define JS_FORCE_GC()                                                  \
  do {                                                                 \
    int x = JS_SetRTGC(__contextORisolate, false) - 1;                 \
    for (int i = 0; i < x; i++) JS_SetRTGC(__contextORisolate, false); \
    JS_GC(JS_GetRuntime(JS_GET_STATE_MARKER()));                       \
    for (int i = -1; i < x; i++) JS_SetRTGC(__contextORisolate, true); \
  } while (0)

#define JS_TERMINATE_EXECUTION(mcom)                                      \
  do {                                                                    \
    node::commons* __mcom__ = node::commons::getInstanceByThreadId(mcom); \
    __mcom__->should_interrupt_ = true;                                   \
    JS_RequestInterruptCallback(                                          \
        JS_GetRuntime(__mcom__->node_isolate->GetRaw()));                 \
  } while (0)

#define JS_GET_UV_LOOP(index) node::commons::getInstanceByThreadId(index)->loop

#define JS_ENTER_SCOPE()

#define JS_ENTER_SCOPE_WITH(x)

#define JS_ENTER_SCOPE_COM() node::commons* com = node::commons::getInstance()

#define JS_ENTER_SCOPE_COM_WITH(x) \
  node::commons* com = node::commons : getInstanceIso(x)

#define JS_LEAVE_SCOPE(x) x

#define JS_CREATE_NEW_ENGINE(x) MozJS::Isolate::New(x)

#define JS_CURRENT_ENGINE_DATA(x) x->GetData()

#define JS_SET_ENGINE_DATA(x, data) x->SetData(data)

#define JS_STATE_MARKER JSContext* __contextORisolate

#define JS_GET_STATE_MARKER() __contextORisolate

#define JS_DEFINE_STATE_MARKER(x) \
  JSContext* __contextORisolate = \
      (x != NULL) ? x->node_isolate->GetRaw() : JS_CURRENT_CONTEXT()
#define JS_DEFINE_STATE_MARKER_(x) JSContext* __contextORisolate = x

#define JS_DEFINE_COM_AND_MARKER()                   \
  node::commons* com = node::commons::getInstance(); \
  JS_DEFINE_STATE_MARKER(com)

#endif  // SRC_JX_PROXY_MOZILLA_MOZENVIRONMENT_H_
