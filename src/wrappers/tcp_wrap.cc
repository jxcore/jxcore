// Copyright & License details are available under JXCORE_LICENSE file

#include "node_buffer.h"
#include "handle_wrap.h"
#include "stream_wrap.h"
#include "tcp_wrap.h"

#include <stdlib.h>

namespace node {

typedef class ReqWrap<uv_connect_t> ConnectWrap;

JS_LOCAL_OBJECT AddressToJS(JS_STATE_MARKER, const sockaddr* addr);

JS_LOCAL_OBJECT TCPWrap::InstantiateCOM(commons* _com) {
  JS_ENTER_SCOPE_COM();

  assert(JS_IS_EMPTY((com->tcpConstructor)) == false);

  JS_LOCAL_OBJECT obj = JS_NEW_DEFAULT_INSTANCE(com->tcpConstructor);

  return JS_LEAVE_SCOPE(obj);
}

JS_LOCAL_OBJECT TCPWrap::Instantiate() { return InstantiateCOM(NULL); }

TCPWrap* TCPWrap::Unwrap(JS_LOCAL_OBJECT obj) {
  assert(!JS_IS_EMPTY(obj));
  assert(JS_OBJECT_FIELD_COUNT(obj) > 0);
  return static_cast<TCPWrap*>(JS_GET_POINTER_DATA(obj));
}

uv_tcp_t* TCPWrap::UVHandle() { return &handle_; }

JS_METHOD_NO_COM(TCPWrap, New) {
  // This constructor should not be exposed to public javascript.
  // Therefore we assert that we are not trying to call this as a
  // normal function.
  assert(args.IsConstructCall());
  JS_CLASS_NEW_INSTANCE(obj, TCP);

  TCPWrap* wrap = new TCPWrap(obj);
  assert(wrap);

  RETURN_POINTER(obj);
}
JS_METHOD_END

TCPWrap::TCPWrap(JS_HANDLE_OBJECT object)
    : StreamWrap(object, (uv_stream_t*)&handle_) {

  int r = uv_tcp_init(com->loop, &handle_);
  assert(r == 0);  // How do we proxy this error up to javascript?
  // Suggestion: uv_tcp_init() returns void.
  UpdateWriteQueueSize(com);
}

TCPWrap::~TCPWrap() { assert(JS_IS_EMPTY(object_)); }

JS_METHOD_NO_COM(TCPWrap, GetSockName) {
  struct sockaddr_storage address;

  ENGINE_UNWRAP(TCPWrap);

  int addrlen = sizeof(address);
  int r = uv_tcp_getsockname(&wrap->handle_,
                             reinterpret_cast<sockaddr*>(&address), &addrlen);

  if (r) {
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
    RETURN_PARAM(JS_NULL());
  }

  const sockaddr* addr = reinterpret_cast<const sockaddr*>(&address);
  RETURN_PARAM(AddressToJS(JS_GET_STATE_MARKER(), addr));
}
JS_METHOD_END

JS_METHOD_NO_COM(TCPWrap, GetPeerName) {
  struct sockaddr_storage address;

  ENGINE_UNWRAP(TCPWrap);

  int addrlen = sizeof(address);
  int r = uv_tcp_getpeername(&wrap->handle_,
                             reinterpret_cast<sockaddr*>(&address), &addrlen);

  if (r) {
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
    RETURN_PARAM(JS_NULL());
  }

  const sockaddr* addr = reinterpret_cast<const sockaddr*>(&address);
  RETURN_PARAM(AddressToJS(JS_GET_STATE_MARKER(), addr));
}
JS_METHOD_END

JS_METHOD_NO_COM(TCPWrap, SetNoDelay) {
  ENGINE_UNWRAP(TCPWrap);

  int enable = static_cast<int>(args.GetBoolean(0));
  int r = uv_tcp_nodelay(&wrap->handle_, enable);
  if (r) SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
}
JS_METHOD_END

JS_METHOD_NO_COM(TCPWrap, SetKeepAlive) {
  ENGINE_UNWRAP(TCPWrap);

  int enable = args.GetInteger(0);
  unsigned int delay = args.GetUInteger(1);

  int r = uv_tcp_keepalive(&wrap->handle_, enable, delay);
  if (r) SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
}
JS_METHOD_END

#ifdef _WIN32
JS_METHOD_NO_COM(TCPWrap, SetSimultaneousAccepts) {
  ENGINE_UNWRAP(TCPWrap);

  bool enable = args.GetBoolean(0);

  int r = uv_tcp_simultaneous_accepts(&wrap->handle_, enable ? 1 : 0);
  if (r) SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
}
JS_METHOD_END
#endif

JS_METHOD_NO_COM(TCPWrap, Open) {
  ENGINE_UNWRAP(TCPWrap);
  int fd = args.GetInteger(0);
  uv_tcp_open(&wrap->handle_, fd);

  RETURN_PARAM(JS_NULL());
}
JS_METHOD_END

JS_METHOD_NO_COM(TCPWrap, Bind) {
  ENGINE_UNWRAP(TCPWrap);

  int port = args.GetInteger(1);

  bool secure = false;

  if (args.Length() > 2) secure = args.GetBoolean(2);

  int bport = -1;
  if (!secure)
    bport = node::commons::GetTCPBoundary();
  else
    bport = node::commons::GetTCPSBoundary();

  if (bport == -3) {
    THROW_EXCEPTION("This process is restricted and can not listen given port");
  }

  if (bport >= 0) {
    port = bport;
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  struct sockaddr_in address = uv_ip4_addr(*str_key, port);
  int r = uv_tcp_bind(&wrap->handle_, address);

  // Error starting the tcp.
  if (r) SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

JS_METHOD_NO_COM(TCPWrap, Bind6) {
  ENGINE_UNWRAP(TCPWrap);

  int port = args.GetInteger(1);

  bool secure = false;

  if (args.Length() > 2) secure = args.GetBoolean(2);

  int bport = -1;
  if (!secure)
    bport = node::commons::GetTCPBoundary();
  else
    bport = node::commons::GetTCPSBoundary();

  if (bport == -3) {
    THROW_EXCEPTION("This process is restricted and can not listen given port");
  }

  if (bport >= 0) {
    port = bport;
  }

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  struct sockaddr_in6 address = uv_ip6_addr(*str_key, port);
  int r = uv_tcp_bind6(&wrap->handle_, address);

  // Error starting the tcp.
  if (r) SetCOMErrno(wrap->com, uv_last_error(com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

JS_METHOD_NO_COM(TCPWrap, Listen) {
  ENGINE_UNWRAP(TCPWrap);

  int backlog = args.GetInteger(0);
  int port = -1;
  if (args.Length() > 1) {
    port = args.GetInteger(1);
  }

  bool secure = false;

  if (args.Length() > 2) secure = args.GetBoolean(2);

  int bport = -1;
  if (!secure)
    bport = node::commons::GetTCPBoundary();
  else
    bport = node::commons::GetTCPSBoundary();

  if (bport == -3) {
    THROW_EXCEPTION("This process is restricted and can not listen given port");
  }

  if (bport >= 0) {
    port = bport;
  }

  int r = -1;
  {
    auto_lock locker_(CSLOCK_TCP);
    r = uv_listen_jx((uv_stream_t*)&wrap->handle_, backlog, OnConnection, port);
  }

  // Error starting the tcp.
  if (r) SetCOMErrno(wrap->com, uv_last_error(com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
}
JS_METHOD_END

void TCPWrap::OnConnection(uv_stream_t* handle, int status) {
  ENGINE_LOG_THIS("TCPWrap", "OnConnection");
  JS_ENTER_SCOPE();

  TCPWrap* wrap = static_cast<TCPWrap*>(handle->data);
  assert(&wrap->handle_ == (uv_tcp_t*)handle);

  JS_DEFINE_STATE_MARKER(wrap->com);
  node::commons* com = wrap->com;
  // We should not be getting this callback if someone as already called
  // uv_close() on the handle.
  assert(JS_IS_EMPTY((wrap->object_)) == false);

  if (status == 0) {
    // Instantiate the client javascript object and handle.
    JS_LOCAL_OBJECT client_obj = InstantiateCOM(wrap->com);

    // Unwrap the client javascript object.
    assert(client_obj->InternalFieldCount() > 0);
    TCPWrap* client_wrap =
        static_cast<TCPWrap*>(JS_GET_POINTER_DATA(client_obj));

    if (uv_accept(handle, (uv_stream_t*)&client_wrap->handle_)) return;

    // Successful accept. Call the onconnection callback in JavaScript land.
    __JS_LOCAL_VALUE argv[1] = {JS_CORE_REFERENCE(client_obj)};
    MakeCallback(wrap->com, wrap->object_, JS_PREDEFINED_STRING(onconnection),
                 1, argv);
  } else {
    __JS_LOCAL_VALUE argv[1] = {
        JS_CORE_REFERENCE(JS_TYPE_TO_LOCAL_VALUE(JS_NULL()))};
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
    MakeCallback(wrap->com, wrap->object_, JS_PREDEFINED_STRING(onconnection),
                 1, argv);
  }
}

void TCPWrap::AfterConnect(uv_connect_t* req, int status) {
  ENGINE_LOG_THIS("TCPWrap", "AfterConnect");
  ConnectWrap* req_wrap = (ConnectWrap*)req->data;
  TCPWrap* wrap = (TCPWrap*)req->handle->data;

  JS_ENTER_SCOPE();

  // The wrap and request objects should still be there.
  assert(JS_IS_EMPTY((req_wrap->object_)) == false);
  assert(JS_IS_EMPTY((wrap->object_)) == false);

  if (status) {
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
  }

  JS_DEFINE_STATE_MARKER(wrap->com);

#ifdef JS_ENGINE_V8
  node::commons* com = wrap->com;
  __JS_LOCAL_VALUE argv[5] = {STD_TO_INTEGER(status),
                              JS_TYPE_TO_LOCAL_VALUE(wrap->object_),
                              JS_TYPE_TO_LOCAL_VALUE(req_wrap->object_),
                              JS_TYPE_TO_LOCAL_VALUE(STD_TO_BOOLEAN(true)),
                              JS_TYPE_TO_LOCAL_VALUE(STD_TO_BOOLEAN(true))};
#elif defined(JS_ENGINE_MOZJS)
  __JS_LOCAL_VALUE argv[5] = {
      JS::Int32Value(status),               JS_CORE_REFERENCE(wrap->object_),
      JS_CORE_REFERENCE(req_wrap->object_), JS::BooleanValue(true),
      JS::BooleanValue(true)};
#endif

  MakeCallback(wrap->com, req_wrap->object_, JS_PREDEFINED_STRING(oncomplete),
               5, argv);

  delete req_wrap;
}

JS_METHOD_NO_COM(TCPWrap, Connect) {
  ENGINE_UNWRAP(TCPWrap);

  int port = args.GetInteger(1);

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  struct sockaddr_in address = uv_ip4_addr(*str_key, port);

  ConnectWrap* req_wrap = new ConnectWrap(wrap->com);

  int r =
      uv_tcp_connect(&req_wrap->req_, &wrap->handle_, address, AfterConnect);

  req_wrap->Dispatched();

  if (r) {
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
    delete req_wrap;
    RETURN_PARAM(JS_NULL());
  } else {
    RETURN_PARAM(req_wrap->object_);
  }
}
JS_METHOD_END

JS_METHOD_NO_COM(TCPWrap, Connect6) {
  ENGINE_UNWRAP(TCPWrap);

  int port = args.GetInteger(1);

  jxcore::JXString str_key;
  args.GetString(0, &str_key);
  struct sockaddr_in6 address = uv_ip6_addr(*str_key, port);

  ConnectWrap* req_wrap = new ConnectWrap(wrap->com);

  int r =
      uv_tcp_connect6(&req_wrap->req_, &wrap->handle_, address, AfterConnect);

  req_wrap->Dispatched();

  if (r) {
    SetCOMErrno(wrap->com, uv_last_error(wrap->com->loop));
    delete req_wrap;
    RETURN_PARAM(JS_NULL());
  } else {
    RETURN_PARAM(req_wrap->object_);
  }
}
JS_METHOD_END

// also used by udp_wrap.cc
JS_LOCAL_OBJECT AddressToJS(JS_STATE_MARKER, const sockaddr* addr) {
  ENGINE_LOG_THIS("TCPWrap", "AddressToJS");
  JS_ENTER_SCOPE_COM();
  char ip[INET6_ADDRSTRLEN];
  const sockaddr_in* a4;
  const sockaddr_in6* a6;
  int port;

  JS_LOCAL_OBJECT info = JS_NEW_EMPTY_OBJECT();

  switch (addr->sa_family) {
    case AF_INET6:
      a6 = reinterpret_cast<const sockaddr_in6*>(addr);
      uv_inet_ntop(AF_INET6, &a6->sin6_addr, ip, sizeof ip);
      port = ntohs(a6->sin6_port);

      JS_NAME_SET(info, JS_PREDEFINED_STRING(address), STD_TO_STRING(ip));
      JS_NAME_SET(info, JS_PREDEFINED_STRING(family), STD_TO_STRING("IPv6"));
      JS_NAME_SET(info, JS_PREDEFINED_STRING(port), STD_TO_INTEGER(port));
      break;

    case AF_INET:
      a4 = reinterpret_cast<const sockaddr_in*>(addr);
      uv_inet_ntop(AF_INET, &a4->sin_addr, ip, sizeof ip);
      port = ntohs(a4->sin_port);

      JS_NAME_SET(info, JS_PREDEFINED_STRING(address), STD_TO_STRING(ip));
      JS_NAME_SET(info, JS_PREDEFINED_STRING(family), STD_TO_STRING("IPv4"));
      JS_NAME_SET(info, JS_PREDEFINED_STRING(port), STD_TO_INTEGER(port));
      break;

    default:
      JS_NAME_SET(info, JS_STRING_ID("address"), STD_TO_STRING(""));
  }

  return JS_LEAVE_SCOPE(info);
}

}  // namespace node

NODE_MODULE(node_tcp_wrap, node::TCPWrap::Initialize)
