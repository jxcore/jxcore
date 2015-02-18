// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_MOZENVIRONMENT_H_
#define SRC_JX_PROXY_MOZILLA_MOZENVIRONMENT_H_

#define JS_ENGINE_MARKER MozJS::Isolate *

#define JS_CURRENT_ENGINE() MozJS::Isolate::GetCurrent()

#define JS_CURRENT_CONTEXT() JS_CURRENT_ENGINE()->GetRaw()  // JSContext *

#define JS_GET_GLOBAL() jxcore::getGlobal(JS_GetThreadId(__contextORisolate))

#define JS_FORCE_GC() JS_GC(JS_GetRuntime(JS_GET_STATE_MARKER()))

#define JS_TERMINATE_EXECUTION(mcom)                                      \
  do {                                                                    \
    node::commons* __mcom__ = node::commons::getInstanceByThreadId(mcom); \
    __mcom__->should_interrupt_ = true;                                   \
    JS_RequestInterruptCallback(                                          \
        JS_GetRuntime(__mcom__->node_isolate->GetRaw()));                 \
  } while (0)

#define JS_GET_UV_LOOP(index) node::commons::getInstanceByThreadId(index)->loop

#define JS_ENTER_SCOPE()

#define JS_ENTER_SCOPE_COM() node::commons* com = node::commons::getInstance()

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

#define JS_DEFINE_COM_AND_MARKER()           \
  node::commons* com = node::commons::getInstance(); \
  JS_DEFINE_STATE_MARKER(com)

#endif  // SRC_JX_PROXY_MOZILLA_MOZENVIRONMENT_H_
