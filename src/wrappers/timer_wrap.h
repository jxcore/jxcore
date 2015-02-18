// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_TIMER_WRAP_H_
#define SRC_WRAPPERS_TIMER_WRAP_H_

#include "node.h"
#include "wrappers/handle_wrap.h"
#include "jx/commons.h"

namespace node {

class TimerWrap : public HandleWrap {
 public:
  explicit TimerWrap(JS_HANDLE_OBJECT object);

  static DEFINE_JS_METHOD(New);

 private:
  static DEFINE_JS_METHOD(Start);

  static DEFINE_JS_METHOD(Stop);

  static DEFINE_JS_METHOD(Again);

  static DEFINE_JS_METHOD(SetRepeat);

  static DEFINE_JS_METHOD(GetRepeat);

  static DEFINE_JS_METHOD(Now);

  static void OnTimeout(uv_timer_t* handle, int status);

  uv_timer_t handle_;

  INIT_NAMED_CLASS_MEMBERS(Timer, TimerWrap) {
    HandleWrap::Initialize(target);

    SET_INSTANCE_METHOD("close", HandleWrap::Close, 0);
    SET_INSTANCE_METHOD("ref", HandleWrap::Ref, 1);
    SET_INSTANCE_METHOD("unref", HandleWrap::Unref, 1);

    SET_INSTANCE_METHOD("start", Start, 2);
    SET_INSTANCE_METHOD("stop", Stop, 0);
    SET_INSTANCE_METHOD("setRepeat", SetRepeat, 1);
    SET_INSTANCE_METHOD("getRepeat", GetRepeat, 0);
    SET_INSTANCE_METHOD("again", Again, 0);

    SET_CLASS_METHOD("now", Now, 0);
  }
  END_INIT_NAMED_MEMBERS(Timer)
};

}  // namespace node
#endif  // SRC_WRAPPERS_TIMER_WRAP_H_
