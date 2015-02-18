// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_JXTIMERS_WRAP_H_
#define SRC_WRAPPERS_JXTIMERS_WRAP_H_

#include "jx/Proxy/JSEngine.h"
#include "jx/commons.h"

namespace node {

class JXTimersWrap {
 private:
  static void checkKeys();

  static void Watcher(void* w);

  static DEFINE_JS_METHOD(StartWatcher);
  static DEFINE_JS_METHOD(ForceCheckKeys);

 public:
  INIT_CLASS_MEMBERS() {
    SET_CLASS_METHOD("startWatcher", StartWatcher, 0);
    SET_CLASS_METHOD("forceCheckKeys", ForceCheckKeys, 0);
  }
  END_INIT_MEMBERS
};

}  // namespace node

#endif  // SRC_WRAPPERS_JXTIMERS_WRAP_H_
