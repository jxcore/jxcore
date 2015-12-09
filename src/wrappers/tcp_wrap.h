// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_TCP_WRAP_H_
#define SRC_WRAPPERS_TCP_WRAP_H_
#include "node.h"
#include "stream_wrap.h"
#include "jx/extend.h"

namespace node {

class TCPWrap : public StreamWrap {
 public:
  static JS_LOCAL_OBJECT InstantiateCOM(commons* com);
  static JS_LOCAL_OBJECT Instantiate();
  static TCPWrap* Unwrap(JS_LOCAL_OBJECT obj);

  uv_tcp_t* UVHandle();

 private:
  explicit TCPWrap(JS_HANDLE_OBJECT object);
  ~TCPWrap();

  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(GetSockName);
  static DEFINE_JS_METHOD(GetPeerName);
  static DEFINE_JS_METHOD(SetNoDelay);
  static DEFINE_JS_METHOD(SetKeepAlive);
  static DEFINE_JS_METHOD(Bind);
  static DEFINE_JS_METHOD(Bind6);
  static DEFINE_JS_METHOD(Listen);
  static DEFINE_JS_METHOD(Connect);
  static DEFINE_JS_METHOD(Connect6);
  static DEFINE_JS_METHOD(Open);

#ifdef _WIN32
  static DEFINE_JS_METHOD(SetSimultaneousAccepts);
#endif

  static void OnConnection(uv_stream_t* handle, int status);
  static void AfterConnect(uv_connect_t* req, int status);

  uv_tcp_t handle_;

  INIT_NAMED_CLASS_MEMBERS(TCP, TCPWrap) {
    HandleWrap::Initialize(target);
    StreamWrap::Initialize(target);

#ifdef JS_ENGINE_V8
    enum v8::PropertyAttribute attributes =
        static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete);
    constructor->InstanceTemplate()->SetAccessor(
        STD_TO_STRING("fd"), StreamWrap::GetFD, NULL, JS_HANDLE_VALUE(),
        v8::DEFAULT, attributes);
#elif defined(JS_ENGINE_MOZJS)
    JS_ACCESSOR_SET(constructor, STD_TO_STRING("fd"), StreamWrap::GetFD, NULL);
#endif

    SET_INSTANCE_METHOD("close", HandleWrap::Close, 0);

    SET_INSTANCE_METHOD("ref", HandleWrap::Ref, 0);
    SET_INSTANCE_METHOD("unref", HandleWrap::Unref, 0);

    SET_INSTANCE_METHOD("readStart", StreamWrap::ReadStart, 0);
    SET_INSTANCE_METHOD("readStop", StreamWrap::ReadStop, 0);
    SET_INSTANCE_METHOD("shutdown", StreamWrap::Shutdown, 0);

    SET_INSTANCE_METHOD("writeBuffer", StreamWrap::WriteBuffer, 0);
    SET_INSTANCE_METHOD("writeAsciiString", StreamWrap::WriteAsciiString, 0);
    SET_INSTANCE_METHOD("writeUtf8String", StreamWrap::WriteUtf8String, 0);
    SET_INSTANCE_METHOD("writeUcs2String", StreamWrap::WriteUcs2String, 0);

    SET_INSTANCE_METHOD("open", Open, 1);
    SET_INSTANCE_METHOD("bind", Bind, 3);
    SET_INSTANCE_METHOD("listen", Listen, 3);
    SET_INSTANCE_METHOD("connect", Connect, 2);
    SET_INSTANCE_METHOD("bind6", Bind6, 3);
    SET_INSTANCE_METHOD("connect6", Connect6, 2);
    SET_INSTANCE_METHOD("getsockname", GetSockName, 0);
    SET_INSTANCE_METHOD("getpeername", GetPeerName, 0);
    SET_INSTANCE_METHOD("setNoDelay", SetNoDelay, 1);
    SET_INSTANCE_METHOD("setKeepAlive", SetKeepAlive, 2);

#ifdef _WIN32
    SET_INSTANCE_METHOD("setSimultaneousAccepts", SetSimultaneousAccepts, 1);
#endif

    JS_NEW_PERSISTENT_FUNCTION(com->tcpConstructor,
                               JS_GET_FUNCTION(constructor));
  }
  END_INIT_NAMED_MEMBERS(TCP)
};

}  // namespace node

#endif  // SRC_WRAPPERS_TCP_WRAP_H_
