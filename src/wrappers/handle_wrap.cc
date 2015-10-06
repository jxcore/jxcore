// Copyright & License details are available under JXCORE_LICENSE file

#include "wrappers/handle_wrap.h"
#include "jx/commons.h"

namespace node {

DEFINE_CLASS_INITIALIZER(HandleWrap, Initialize) {
  /* Doesn't do anything at the moment. */
}

JS_METHOD_NO_COM(HandleWrap, Ref) {
  ENGINE_UNWRAP_NO_ABORT(HandleWrap);

  if (wrap != NULL) wrap->ref();
}
JS_METHOD_END

JS_METHOD_NO_COM(HandleWrap, Unref) {
  ENGINE_UNWRAP_NO_ABORT(HandleWrap);

  if (wrap != NULL) wrap->unref();
}
JS_METHOD_END

void HandleWrap::unref() {
  if (handle__ != NULL) {
    uv_unref(handle__);
    flags_ |= kUnref;
  }
}

void HandleWrap::ref() {
  if (handle__ != NULL) {
    uv_ref(handle__);
    flags_ &= ~kUnref;
  }
}

JS_METHOD_NO_COM(HandleWrap, Close) {
  ENGINE_UNWRAP_NO_ABORT(HandleWrap);

  // guard against uninitialized handle or double close
  if (wrap == NULL || wrap->handle__ == NULL) {
    RETURN();
  }

  assert(!JS_IS_EMPTY(wrap->object_));
  uv_close(wrap->handle__, OnClose);
  wrap->handle__ = NULL;

  if (args.IsFunction(0)) {
    commons* com = wrap->com;
    JS_LOCAL_OBJECT lobj = JS_OBJECT_FROM_PERSISTENT(wrap->object_);
    JS_NAME_SET(lobj, JS_PREDEFINED_STRING(close), GET_ARG(0));
    wrap->flags_ |= kCloseCallback;
  }
}
JS_METHOD_END

HandleWrap::HandleWrap(JS_HANDLE_OBJECT_REF object, uv_handle_t* h) {
  ENGINE_LOG_THIS("HandleWrap", "HandleWrap");
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  this->com = com;
  flags_ = 0;
  handle__ = h;
  if (h) {
    h->data = this;
  }

  assert(JS_IS_EMPTY(object_));
#ifdef JS_ENGINE_V8
  assert(JS_OBJECT_FIELD_COUNT(object) > 0);
#endif

  JS_NEW_PERSISTENT_OBJECT(object_, object);
  JS_LOCAL_OBJECT lobj = JS_OBJECT_FROM_PERSISTENT(object_);
  JS_SET_POINTER_DATA(lobj, this);
  QUEUE_INSERT_TAIL(&com->NQ->wrap, &handle_wrap_queue_);
}

void HandleWrap::SetHandle(uv_handle_t* h) {
  ENGINE_LOG_THIS("HandleWrap", "SetHandle");
  handle__ = h;
  h->data = this;
}

HandleWrap::~HandleWrap() {
  ENGINE_LOG_THIS("HandleWrap", "~HandleWrap");
  assert(JS_IS_EMPTY(object_));
  QUEUE_REMOVE(&handle_wrap_queue_);
}

void HandleWrap::OnClose(uv_handle_t* handle) {
  ENGINE_LOG_THIS("HandleWrap", "OnClose");
  JS_ENTER_SCOPE();

  HandleWrap* wrap = static_cast<HandleWrap*>(handle->data);

  // The wrap object should still be there.
  assert(!JS_IS_EMPTY(wrap->object_));

  // But the handle pointer should be gone.
  assert(wrap->handle__ == NULL);

  JS_LOCAL_OBJECT lobj = JS_OBJECT_FROM_PERSISTENT(wrap->object_);
  if (wrap->flags_ & kCloseCallback) {
    commons *com = wrap->com;
    MakeCallback(wrap->com, lobj, JS_PREDEFINED_STRING(close), 0, NULL);
  }

  if (!JS_IS_EMPTY(wrap->object_)) {
    JS_SET_POINTER_DATA(lobj, NULL);
  }
  JS_CLEAR_PERSISTENT(wrap->object_);
  delete wrap;
}

}  // namespace node
