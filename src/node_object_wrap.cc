// Copyright & License details are available under JXCORE_LICENSE file

#include "node_object_wrap.h"
#include "jx/commons.h"

namespace node {

#ifdef JS_ENGINE_V8
void ObjectWrap::WeakCallback(JS_PERSISTENT_VALUE value, void *data) {
  JS_ENTER_SCOPE_WITH(node::commons::getInstance()->node_isolate);
  ObjectWrap *obj = static_cast<ObjectWrap *>(data);
#elif defined(JS_ENGINE_MOZJS)
void ObjectWrap::WeakCallback(JSFreeOp *fop, JSObject *_this) {
  if (!JS_HasPrivate(_this)) return;
  void *__this = JS_GetPrivate(_this);
  if (__this == NULL) return;
  ObjectWrap *obj = static_cast<ObjectWrap *>(__this);
#endif

  assert(!obj->refs_);
#ifdef JS_ENGINE_V8
  assert(value == obj->handle_);
  assert(value.IsNearDeath());
#endif
  delete obj;
}
