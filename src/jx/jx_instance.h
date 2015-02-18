// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_JX_INSTANCE_H_
#define SRC_JX_JX_INSTANCE_H_

#include "Proxy/JSEngine.h"

namespace jxcore {

class JXInstance {
 public:
  static void runScript(void *x);

  static DEFINE_JS_METHOD(sendMessage);

  static DEFINE_JS_METHOD(LoopBreaker);

  static DEFINE_JS_METHOD(Compiler);

  static DEFINE_JS_METHOD(CallBack);

  static DEFINE_JS_METHOD(refWaitCounter);

  static DEFINE_JS_METHOD(setThreadOnHold);
};
}  // namespace jxcore
#endif  // SRC_JX_JX_INSTANCE_H_
