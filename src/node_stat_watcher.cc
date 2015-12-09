// Copyright & License details are available under JXCORE_LICENSE file

#include "node_stat_watcher.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "jx/commons.h"

namespace node {

static void Delete(uv_handle_t* handle) {
  delete reinterpret_cast<uv_fs_poll_t*>(handle);
}

StatWatcher::StatWatcher() : ObjectWrap(), watcher_(new uv_fs_poll_t) {
  uv_fs_poll_init(node::commons::getThreadLoop(), watcher_);
  watcher_->data = static_cast<void*>(this);
}

StatWatcher::~StatWatcher() {
  Stop();
  uv_close(reinterpret_cast<uv_handle_t*>(watcher_), Delete);
}

void StatWatcher::Callback(uv_fs_poll_t* handle, int status,
                           const uv_statbuf_t* prev, const uv_statbuf_t* curr) {
  StatWatcher* wrap = static_cast<StatWatcher*>(handle->data);
  assert(wrap->watcher_ == handle);
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_VALUE argv[3];
  argv[0] = BuildStatsObject(curr);
  argv[1] = BuildStatsObject(prev);
  argv[2] = STD_TO_INTEGER(status);
  if (status == -1) {
    SetErrno(uv_last_error(wrap->watcher_->loop));
  }

  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(wrap->handle_);
  MakeCallback(com, objl, JS_PREDEFINED_STRING(onchange), ARRAY_SIZE(argv), argv);
}

JS_METHOD(StatWatcher, New) {
  assert(args.IsConstructCall());
  JS_CLASS_NEW_INSTANCE(obj, StatWatcher);

  StatWatcher* s = new StatWatcher();
  s->Wrap(obj);
  RETURN_PARAM(obj);
}
JS_METHOD_END

JS_METHOD(StatWatcher, Start) {
  assert(args.Length() == 3);

  StatWatcher* wrap = ObjectWrap::Unwrap<StatWatcher>(args.Holder());
  jxcore::JXString path;
  args.GetString(0, &path);

  const bool persistent = args.GetBoolean(1);
  const uint32_t interval = args.GetUInteger(2);

  if (!persistent) uv_unref(reinterpret_cast<uv_handle_t*>(wrap->watcher_));

  uv_fs_poll_start(wrap->watcher_, Callback, *path, interval);
  wrap->Ref();
}
JS_METHOD_END

JS_METHOD(StatWatcher, Stop) {
  StatWatcher* wrap = ObjectWrap::Unwrap<StatWatcher>(args.Holder());

  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(wrap->handle_);
  MakeCallback(com, objl, STD_TO_STRING("onstop"), 0, NULL);
  wrap->Stop();
}
JS_METHOD_END

void StatWatcher::Stop() {
  if (!uv_is_active(reinterpret_cast<uv_handle_t*>(watcher_))) return;
  uv_fs_poll_stop(watcher_);
  Unref();
}

}  // namespace node
