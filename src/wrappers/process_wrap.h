// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_PROCESS_WRAP_H_
#define SRC_WRAPPERS_PROCESS_WRAP_H_

#include "handle_wrap.h"
#include "jx/Proxy/JSEngine.h"
#include "jx/commons.h"

namespace node {

class ProcessWrap : public HandleWrap {
 public:
  static DEFINE_JS_METHOD(New);

  explicit ProcessWrap(JS_HANDLE_OBJECT object) : HandleWrap(object, NULL) {}
  ~ProcessWrap() {}

 private:
  static DEFINE_JS_METHOD(Spawn);
  static DEFINE_JS_METHOD(Kill);

  static void ParseStdioOptions(commons* com, JS_LOCAL_OBJECT js_options,
                                uv_process_options_t* options);

  static void OnExit(uv_process_t* handle, int exit_status, int term_signal);

  uv_process_t process_;

 public:
  INIT_NAMED_CLASS_MEMBERS(Process, ProcessWrap) {
    HandleWrap::Initialize(target);

    SET_INSTANCE_METHOD("close", HandleWrap::Close, 0);
    SET_INSTANCE_METHOD("ref", HandleWrap::Ref, 1);
    SET_INSTANCE_METHOD("unref", HandleWrap::Unref, 1);

    SET_INSTANCE_METHOD("spawn", Spawn, 2);
    SET_INSTANCE_METHOD("kill", Kill, 0);
  }
  END_INIT_NAMED_MEMBERS(Process)
};

}  // namespace node

#endif  // SRC_WRAPPERS_PROCESS_WRAP_H_
