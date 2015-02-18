// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_FS_EVENT_WRAP_H_
#define SRC_WRAPPERS_FS_EVENT_WRAP_H_

#include "node.h"
#include "wrappers/handle_wrap.h"

namespace node {

class FSEventWrap : public HandleWrap {
 public:
  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(Start);
  static DEFINE_JS_METHOD(Close);

 private:
  explicit FSEventWrap(JS_HANDLE_OBJECT_REF object);
  virtual ~FSEventWrap();

  static void OnEvent(uv_fs_event_t* handle, const char* filename, int events,
                      int status);

  uv_fs_event_t handle_;
  bool initialized_;

  INIT_NAMED_CLASS_MEMBERS(FSEvent, FSEventWrap) {
    HandleWrap::Initialize(target);

    SET_INSTANCE_METHOD("start", Start, 0);
    SET_INSTANCE_METHOD("close", Close, 0);
  }
  END_INIT_NAMED_MEMBERS(FSEvent)
};
}  // namespace node

#endif  // SRC_WRAPPERS_FS_EVENT_WRAP_H_
