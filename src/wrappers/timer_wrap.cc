// Copyright & License details are available under JXCORE_LICENSE file

#include "wrappers/timer_wrap.h"
#include "jx/extend.h"

namespace node {

JS_METHOD(TimerWrap, New) {
  JS_CLASS_NEW_INSTANCE(obj, Timer);

  TimerWrap* wrap = new TimerWrap(obj);
  assert(wrap);

  RETURN_POINTER(obj);
}
JS_METHOD_END

TimerWrap::TimerWrap(JS_HANDLE_OBJECT object)
    : HandleWrap(object, (uv_handle_t*)&handle_) {
  int r = uv_timer_init(this->com->loop, &handle_);
  assert(r == 0);
  handle_.data = this;
}

JS_METHOD_NO_COM(TimerWrap, Start) {
  ENGINE_UNWRAP(TimerWrap);

  int64_t timeout = args.GetInteger(0);
  int64_t repeat = args.GetInteger(1);

  int r = uv_timer_start(&wrap->handle_, OnTimeout, timeout, repeat);

  if (r) SetErrno(uv_last_error(wrap->com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

JS_METHOD_NO_COM(TimerWrap, Stop) {
  ENGINE_UNWRAP(TimerWrap);

  int r = uv_timer_stop(&wrap->handle_);

  if (r) SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

JS_METHOD_NO_COM(TimerWrap, Again) {
  ENGINE_UNWRAP(TimerWrap);

  int r = uv_timer_again(&wrap->handle_);

  if (r) SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

JS_METHOD_NO_COM(TimerWrap, SetRepeat) {
  ENGINE_UNWRAP(TimerWrap);

  int64_t repeat = args.GetInteger(0);

  uv_timer_set_repeat(&wrap->handle_, repeat);

  RETURN_PARAM(STD_TO_INTEGER(0));
}
JS_METHOD_END

JS_METHOD_NO_COM(TimerWrap, GetRepeat) {
  ENGINE_UNWRAP(TimerWrap);

  int64_t repeat = uv_timer_get_repeat(&wrap->handle_);

  if (repeat < 0) SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));

  RETURN_PARAM(STD_TO_INTEGER(repeat));
}
JS_METHOD_END

void TimerWrap::OnTimeout(uv_timer_t* handle, int status) {
  TimerWrap* wrap = static_cast<TimerWrap*>(handle->data);
  assert(wrap);

  commons* com = wrap->com;
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);

  __JS_LOCAL_VALUE argv[1] = {
#ifdef JS_ENGINE_MOZJS
      JS::Int32Value(status)
#elif defined(JS_ENGINE_V8)
      STD_TO_INTEGER(status)
#endif
  };

  JS_LOCAL_OBJECT lobj = JS_OBJECT_FROM_PERSISTENT(wrap->object_);
  MakeCallback(wrap->com, lobj, JS_PREDEFINED_STRING(ontimeout), 1,
               argv);
}

JS_METHOD(TimerWrap, Now) {
  uv_update_time(com->loop);
  double now = static_cast<double>(uv_now(com->loop));

  RETURN_PARAM(STD_TO_NUMBER(now));
}
JS_METHOD_END

}  // namespace node

NODE_MODULE(node_timer_wrap, node::TimerWrap::Initialize)
