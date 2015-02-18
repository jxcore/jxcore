// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_SIGNAL_WRAP_H_
#define SRC_WRAPPERS_SIGNAL_WRAP_H_

#include "wrappers/handle_wrap.h"

namespace node {

class SignalWrap : public HandleWrap {
 public:
  INIT_NAMED_CLASS_MEMBERS(Signal, SignalWrap) {
    HandleWrap::Initialize(target);

    SET_INSTANCE_METHOD("close", HandleWrap::Close, 0);
    SET_INSTANCE_METHOD("ref", HandleWrap::Ref, 1);
    SET_INSTANCE_METHOD("unref", HandleWrap::Unref, 1);

    SET_INSTANCE_METHOD("start", Start, 1);
    SET_INSTANCE_METHOD("stop", Stop, 0);
  }
  END_INIT_NAMED_MEMBERS(Signal)

  explicit SignalWrap(JS_HANDLE_OBJECT object);
  ~SignalWrap();

 private:
  static void OnSignal(uv_signal_t* handle, int signum);

  static DEFINE_JS_METHOD(New);

  static DEFINE_JS_METHOD(Start);

  static DEFINE_JS_METHOD(Stop);

  uv_signal_t handle_;
};

}  // namespace node

#endif  // SRC_WRAPPERS_SIGNAL_WRAP_H_
