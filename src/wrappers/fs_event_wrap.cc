// Copyright & License details are available under JXCORE_LICENSE file

#include "wrappers/fs_event_wrap.h"
#include <stdlib.h>
#include "jx/commons.h"

namespace node {

FSEventWrap::FSEventWrap(JS_HANDLE_OBJECT_REF object)
    : HandleWrap(object, (uv_handle_t*)&handle_) {
  handle_.data = static_cast<void*>(this);
  initialized_ = false;
}

FSEventWrap::~FSEventWrap() { assert(initialized_ == false); }

JS_METHOD(FSEventWrap, New) {
  JS_CLASS_NEW_INSTANCE(obj, FSEvent);

  new FSEventWrap(obj);

  RETURN_POINTER(obj);
}
JS_METHOD_END

JS_METHOD_NO_COM(FSEventWrap, Start) {
  ENGINE_UNWRAP(FSEventWrap);

  if (args.Length() < 1 || !args.IsString(0)) {
    THROW_EXCEPTION("FSEventWrap - Start : Bad arguments");
  }

  uv_loop_t* loop = com->loop;
  jxcore::JXString jxs;
  args.GetString(0, &jxs);
  int r = uv_fs_event_init(loop, &wrap->handle_, *jxs, OnEvent, 0);
  if (r == 0) {
    // Check for persistent argument
    if (!args.GetBoolean(1)) {
      uv_unref(reinterpret_cast<uv_handle_t*>(&wrap->handle_));
    }
    wrap->initialized_ = true;
  } else {
    SetErrno(uv_last_error(loop));
  }

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

void FSEventWrap::OnEvent(uv_fs_event_t* handle, const char* filename,
                          int events, int status) {
  JS_ENTER_SCOPE();
  JS_LOCAL_STRING eventStr;

  FSEventWrap* wrap = static_cast<FSEventWrap*>(handle->data);
  commons* com = wrap->com;
  JS_DEFINE_STATE_MARKER(com);

  assert(JS_IS_EMPTY((wrap->object_)) == false);

  // We're in a bind here. libuv can set both UV_RENAME and UV_CHANGE but
  // the Node API only lets us pass a single event to JS land.
  //
  // The obvious solution is to run the callback twice, once for each event.
  // However, since the second event is not allowed to fire if the handle is
  // closed after the first event, and since there is no good way to detect
  // closed handles, that option is out.
  //
  // For now, ignore the UV_CHANGE event if UV_RENAME is also set. Make the
  // assumption that a rename implicitly means an attribute change. Not too
  // unreasonable, right? Still, we should revisit this before v1.0.
  if (status) {
    SetErrno(uv_last_error(com->loop));
    eventStr = STD_TO_STRING("");
  } else if (events & UV_RENAME) {
    eventStr = STD_TO_STRING("rename");
  } else if (events & UV_CHANGE) {
    eventStr = STD_TO_STRING("change");
  } else {
    assert(0 && "bad fs events flag");
    abort();
  }

  JS_LOCAL_VALUE fn_value;
  if (filename)
    fn_value = static_cast<JS_LOCAL_VALUE>(STD_TO_STRING(filename));
  else
    fn_value = JS_NULL();

#ifdef JS_ENGINE_V8
  __JS_LOCAL_VALUE argv[3] = {STD_TO_INTEGER(status), eventStr, fn_value};
#elif defined(JS_ENGINE_MOZJS)
  __JS_LOCAL_VALUE argv[3] = {JS::Int32Value(status), eventStr.GetRawValue(),
                              fn_value.GetRawValue()};
#endif

  MakeCallback(com, wrap->object_, JS_PREDEFINED_STRING(onchange),
               ARRAY_SIZE(argv), argv);
}

JS_METHOD_NO_COM(FSEventWrap, Close) {
  // Unwrap manually here. The UNWRAP() macro asserts that wrap != NULL.
  // That usually indicates an error but not here: double closes are possible
  // and legal, HandleWrap::Close() deals with them the same way.
  // assert(!args.Holder().IsEmpty());
  // assert(args.Holder()->InternalFieldCount() > 0);
  // void* ptr = args.Holder()->GetPointerFromInternalField(0);
  // FSEventWrap* wrap = static_cast<FSEventWrap*>(ptr);

  ENGINE_UNWRAP(FSEventWrap);

  if (wrap == NULL || wrap->initialized_ == false) {
    RETURN();
  }
  wrap->initialized_ = false;
#ifdef JS_ENGINE_V8
  return HandleWrap::Close(p___args);
#elif defined(JS_ENGINE_MOZJS)
  return HandleWrap::Close(JS_GET_STATE_MARKER(), __argc, __jsval);
#endif
}
JS_METHOD_END

}  // namespace node

NODE_MODULE(node_fs_event_wrap, node::FSEventWrap::Initialize)
