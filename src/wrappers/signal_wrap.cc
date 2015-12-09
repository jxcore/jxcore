// Copyright & License details are available under JXCORE_LICENSE file

#include "node.h"
#include "signal_wrap.h"
#include "jx/commons.h"

namespace node {

JS_METHOD(SignalWrap, New) {
  assert(args.IsConstructCall());
  JS_CLASS_NEW_INSTANCE(obj, Signal);

  new SignalWrap(obj);

  RETURN_POINTER(obj);
}
JS_METHOD_END

SignalWrap::SignalWrap(JS_HANDLE_OBJECT object)
    : HandleWrap(object, reinterpret_cast<uv_handle_t*>(&handle_)) {
  assert(this->com != NULL);
  int r = uv_signal_init(this->com->loop, &handle_);
  assert(r == 0);
}

SignalWrap::~SignalWrap() {}

JS_METHOD_NO_COM(SignalWrap, Start) {
  ENGINE_UNWRAP(SignalWrap);

  int signum = args.GetInteger(0);

  int r = uv_signal_start(&wrap->handle_, OnSignal, signum);

  if (r) SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

JS_METHOD_NO_COM(SignalWrap, Stop) {
  ENGINE_UNWRAP(SignalWrap);

  int r = uv_signal_stop(&wrap->handle_);

  if (r) SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

void SignalWrap::OnSignal(uv_signal_t* handle, int signum) {
  SignalWrap* wrap = container_of(handle, SignalWrap, handle_);
  assert(wrap);
  commons* com = wrap->com;

  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);

#ifdef JS_ENGINE_V8
  __JS_LOCAL_VALUE argv[1] = {STD_TO_INTEGER(signum)};
#elif defined(JS_ENGINE_MOZJS)
  __JS_LOCAL_VALUE argv[1] = {JS::Int32Value(signum)};
#endif
  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(wrap->object_);
  MakeCallback(com, objl, JS_STRING_ID("onsignal"), ARRAY_SIZE(argv),
               argv);
}

}  // namespace node

NODE_MODULE(node_signal_wrap, node::SignalWrap::Initialize)
