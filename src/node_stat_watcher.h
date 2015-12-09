// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_NODE_STAT_WATCHER_H_
#define SRC_NODE_STAT_WATCHER_H_

#include "node.h"
#include "uv.h"

namespace node {

class StatWatcher : ObjectWrap {
 public:
  INIT_NAMED_CLASS_MEMBERS(StatWatcher, StatWatcher)

  SET_INSTANCE_METHOD("start", StatWatcher::Start, 3);
  SET_INSTANCE_METHOD("stop", StatWatcher::Stop, 0);

  END_INIT_NAMED_MEMBERS(StatWatcher)

 protected:
  StatWatcher();
  virtual ~StatWatcher();

  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(Start);
  static DEFINE_JS_METHOD(Stop);

 private:
  static void Callback(uv_fs_poll_t* handle, int status,
                       const uv_statbuf_t* prev, const uv_statbuf_t* curr);
  void Stop();

  uv_fs_poll_t* watcher_;
};

}  // namespace node
#endif  // SRC_NODE_STAT_WATCHER_H_
