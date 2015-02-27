// Copyright & License details are available under JXCORE_LICENSE file

#include "node.h"
#include "node_buffer.h"
#include "jx/commons.h"
#include "wrappers/handle_wrap.h"
#include "stream_wrap.h"
#include "tty_wrap.h"

namespace node {

TTYWrap* TTYWrap::Unwrap(JS_LOCAL_OBJECT obj) {
  assert(!JS_IS_EMPTY(obj));
  assert(obj->InternalFieldCount() > 0);
  return static_cast<TTYWrap*>(obj->GetPointerFromInternalField(0));
}

uv_tty_t* TTYWrap::UVHandle() { return &handle_; }

JS_METHOD(TTYWrap, GuessHandleType) {
  int fd = args.GetInt32(0);
  assert(fd >= 0);

  uv_handle_type t = uv_guess_handle(fd);

  switch (t) {
    case UV_TCP:
      RETURN_PARAM(STD_TO_STRING("TCP"));
      break;

    case UV_TTY:
      RETURN_PARAM(STD_TO_STRING("TTY"));
      break;

    case UV_UDP:
      RETURN_PARAM(STD_TO_STRING("UDP"));
      break;

    case UV_NAMED_PIPE:
      RETURN_PARAM(STD_TO_STRING("PIPE"));
      break;

    case UV_FILE:
      RETURN_PARAM(STD_TO_STRING("FILE"));
      break;

    case UV_UNKNOWN_HANDLE:
      RETURN_PARAM(STD_TO_STRING("UNKNOWN"));
      break;

    default:
      assert(0  && "Bad Handle (TTY_WRAP)");
  }
}
JS_METHOD_END

JS_METHOD(TTYWrap, IsTTY) {
  int fd = args.GetInt32(0);
  assert(fd >= 0);

  if (uv_guess_handle(fd) == UV_TTY) {
    RETURN_TRUE();
  }

  RETURN_FALSE();
}
JS_METHOD_END

JS_METHOD_NO_COM(TTYWrap, GetWindowSize) {
  ENGINE_UNWRAP(TTYWrap);

  int width, height;
  int r = uv_tty_get_winsize(&wrap->handle_, &width, &height);

  if (r) {
    SetErrno(uv_last_error(com->loop));
    RETURN();
  }

  JS_LOCAL_ARRAY arr = JS_NEW_ARRAY_WITH_COUNT(2);
  JS_INDEX_SET(arr, 0, STD_TO_INTEGER(width));
  JS_INDEX_SET(arr, 1, STD_TO_INTEGER(height));

  RETURN_POINTER(arr);
}
JS_METHOD_END

JS_METHOD_NO_COM(TTYWrap, SetRawMode) {
  ENGINE_UNWRAP(TTYWrap);

  int r = uv_tty_set_mode(&wrap->handle_, args.GetBoolean(0));

  if (r) {
    SetErrno(uv_last_error(com->loop));
  }

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

JS_METHOD(TTYWrap, New) {
  // This constructor should not be exposed to public javascript.
  // Therefore we assert that we are not trying to call this as a
  // normal function.
  assert(args.IsConstructCall());

  JS_CLASS_NEW_INSTANCE(obj, TTY);

  int fd = args.GetInt32(0);
  assert(fd >= 0);

  TTYWrap* wrap = new TTYWrap(obj, fd, args.GetBoolean(1));
  assert(wrap);
  wrap->UpdateWriteQueueSize(wrap->com);

  RETURN_POINTER(obj);
}
JS_METHOD_END

TTYWrap::TTYWrap(JS_HANDLE_OBJECT object, int fd, bool readable)
    : StreamWrap(object, (uv_stream_t*)&handle_) {
  uv_tty_init(this->com->loop, &handle_, fd, readable);
}

}  // namespace node

NODE_MODULE(node_tty_wrap, node::TTYWrap::Initialize)
