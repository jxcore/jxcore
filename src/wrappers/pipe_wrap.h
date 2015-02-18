// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_PIPE_WRAP_H_
#define SRC_WRAPPERS_PIPE_WRAP_H_
#include "stream_wrap.h"

namespace node {

class PipeWrap : StreamWrap {
 public:
  uv_pipe_t* UVHandle();

  static JS_LOCAL_OBJECT Instantiate();
  static PipeWrap* Unwrap(JS_LOCAL_OBJECT obj);

 private:
  PipeWrap(JS_HANDLE_OBJECT object, bool ipc);

  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(Bind);
  static DEFINE_JS_METHOD(Listen);
  static DEFINE_JS_METHOD(Connect);
  static DEFINE_JS_METHOD(Open);

#ifdef _WIN32
  static JS_HANDLE_VALUE SetPendingInstances(const v8::Arguments& args);
#endif

  static void OnConnection(uv_stream_t* handle, int status);
  static void AfterConnect(uv_connect_t* req, int status);

  uv_pipe_t handle_;

  INIT_NAMED_CLASS_MEMBERS(Pipe, PipeWrap) {
    StreamWrap::Initialize(target);

#ifdef JS_ENGINE_V8
    enum v8::PropertyAttribute attributes =
        static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete);
    constructor->InstanceTemplate()->SetAccessor(
        STD_TO_STRING("fd"), StreamWrap::GetFD, NULL, JS_HANDLE_VALUE(),
        v8::DEFAULT, attributes);
#else
    JS_ACCESSOR_SET(constructor, STD_TO_STRING("fd"), StreamWrap::GetFD, NULL);
#endif

    SET_INSTANCE_METHOD("close", HandleWrap::Close, 0);
    SET_INSTANCE_METHOD("unref", HandleWrap::Unref, 1);
    SET_INSTANCE_METHOD("ref", HandleWrap::Ref, 1);

    SET_INSTANCE_METHOD("readStart", StreamWrap::ReadStart, 0);
    SET_INSTANCE_METHOD("readStop", StreamWrap::ReadStop, 0);
    SET_INSTANCE_METHOD("shutdown", StreamWrap::Shutdown, 0);

    SET_INSTANCE_METHOD("writeBuffer", StreamWrap::WriteBuffer, 0);
    SET_INSTANCE_METHOD("writeAsciiString", StreamWrap::WriteAsciiString, 0);
    SET_INSTANCE_METHOD("writeUtf8String", StreamWrap::WriteUtf8String, 0);
    SET_INSTANCE_METHOD("writeUcs2String", StreamWrap::WriteUcs2String, 0);

    SET_INSTANCE_METHOD("bind", Bind, 0);
    SET_INSTANCE_METHOD("listen", Listen, 0);
    SET_INSTANCE_METHOD("connect", Connect, 0);
    SET_INSTANCE_METHOD("open", Open, 0);

#ifdef _WIN32
    SET_INSTANCE_METHOD("setPendingInstances", SetPendingInstances, 0);
#endif

    if (!JS_IS_EMPTY(com->pipeConstructor)) {
      JS_CLEAR_PERSISTENT(com->pipeConstructor);
    }
    com->pipeConstructor =
        JS_NEW_PERSISTENT_FUNCTION(JS_GET_FUNCTION(constructor));
  }
  END_INIT_NAMED_MEMBERS(Pipe)
};

}  // namespace node

#endif  // SRC_WRAPPERS_PIPE_WRAP_H_
