// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_TTY_WRAP_H_
#define SRC_WRAPPERS_TTY_WRAP_H_

#include "wrappers/handle_wrap.h"
#include "stream_wrap.h"

namespace node {

class TTYWrap : StreamWrap {
 public:
  static TTYWrap* Unwrap(JS_LOCAL_OBJECT obj);

  uv_tty_t* UVHandle();

 private:
  TTYWrap(JS_HANDLE_OBJECT object, int fd, bool readable);

  static DEFINE_JS_METHOD(GuessHandleType);
  static DEFINE_JS_METHOD(IsTTY);
  static DEFINE_JS_METHOD(GetWindowSize);
  static DEFINE_JS_METHOD(SetRawMode);
  static DEFINE_JS_METHOD(New);

  uv_tty_t handle_;

  INIT_NAMED_CLASS_MEMBERS(TTY, TTYWrap)

  StreamWrap::Initialize(target);

#ifdef JS_ENGINE_V8
  enum v8::PropertyAttribute attributes =
      static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete);
  constructor->InstanceTemplate()->SetAccessor(STD_TO_STRING("fd"),
                                               StreamWrap::GetFD, NULL,
                                               JS_HANDLE_VALUE(), v8::DEFAULT,
                                               attributes);
#else
  JS_ACCESSOR_SET(constructor, STD_TO_STRING("fd"), StreamWrap::GetFD, NULL);
#endif

  SET_INSTANCE_METHOD("close", HandleWrap::Close, 0);
  SET_INSTANCE_METHOD("unref", HandleWrap::Unref, 1);

  SET_INSTANCE_METHOD("readStart", StreamWrap::ReadStart, 0);
  SET_INSTANCE_METHOD("readStop", StreamWrap::ReadStop, 0);
  SET_INSTANCE_METHOD("writeBuffer", StreamWrap::WriteBuffer, 0);
  SET_INSTANCE_METHOD("writeAsciiString", StreamWrap::WriteAsciiString, 0);
  SET_INSTANCE_METHOD("writeUtf8String", StreamWrap::WriteUtf8String, 0);
  SET_INSTANCE_METHOD("writeUcs2String", StreamWrap::WriteUcs2String, 0);

  SET_INSTANCE_METHOD("getWindowSize", GetWindowSize, 1);
  SET_INSTANCE_METHOD("setRawMode", SetRawMode, 0);

  JS_METHOD_SET(target, "isTTY", IsTTY);
  JS_METHOD_SET(target, "guessHandleType", GuessHandleType);

  END_INIT_NAMED_MEMBERS(TTY)
};

}  // namespace node
#endif  // SRC_WRAPPERS_TTY_WRAP_H_
