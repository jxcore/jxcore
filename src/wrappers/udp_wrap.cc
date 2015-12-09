// Copyright & License details are available under JXCORE_LICENSE file

#include "node_buffer.h"
#include "slab_allocator.h"
#include "udp_wrap.h"

#include <stdlib.h>

namespace node {

typedef ReqWrap<uv_udp_send_t> SendWrap;

JS_LOCAL_OBJECT AddressToJS(JS_STATE_MARKER, const sockaddr* addr);

void UDPWrap::DeleteSlabAllocator(void*) {
  node::commons* com = node::commons::getInstance();
  delete com->udp_slab_allocator;
  com->udp_slab_allocator = NULL;
}

UDPWrap::UDPWrap(JS_HANDLE_OBJECT object)
    : HandleWrap(object, (uv_handle_t*)&handle_) {
  uv_udp_init(this->com->loop, &handle_);
  handle_.data = reinterpret_cast<void*>(this);
}

UDPWrap::~UDPWrap() {}

JS_METHOD_NO_COM(UDPWrap, New) {
  assert(args.IsConstructCall());
  JS_CLASS_NEW_INSTANCE(obj, UDP);
  new UDPWrap(obj);

  RETURN_POINTER(obj);
}
JS_METHOD_END

JS_GETTER_CLASS_METHOD(UDPWrap, GetFD) {
#if defined(_WIN32)
  RETURN_GETTER_PARAM(JS_NULL());
#else
  UDPWrap* wrap = static_cast<UDPWrap*>(JS_GET_POINTER_DATA(caller));
  int fd = (wrap == NULL) ? -1 : wrap->handle_.io_watcher.fd;
  RETURN_GETTER_PARAM(STD_TO_INTEGER(fd));
#endif
}
JS_GETTER_METHOD_END

JS_NATIVE_RETURN_TYPE UDPWrap::DoBind(jxcore::PArguments& args, int family) {
  JS_ENTER_SCOPE_WITH(args.GetIsolate());
  int r;

  ENGINE_UNWRAP(UDPWrap);
  JS_DEFINE_STATE_MARKER(com);

  // bind(ip, port, flags)
  assert(args.Length() == 3);

  jxcore::JXString address;
  args.GetString(0, &address);
  int port = args.GetUInteger(1);
  const int flags = args.GetUInteger(2);

  int tcp = node::commons::GetTCPBoundary();

  if (tcp == -3) {
    THROW_EXCEPTION("This process is restricted and can not listen given port");
  }

  if (tcp >= 0) {
    port = tcp;
  }

  switch (family) {
    case AF_INET:
      r = uv_udp_bind(&wrap->handle_, uv_ip4_addr(*address, port), flags);
      break;
    case AF_INET6:
      r = uv_udp_bind6(&wrap->handle_, uv_ip6_addr(*address, port), flags);
      break;
    default:
      assert(0 && "unexpected address family");
      abort();
  }

  if (r) SetErrno(uv_last_error(com->loop));

  RETURN_PARAM(STD_TO_INTEGER(r));
  JS_METHOD_END

  JS_METHOD_NO_COM(UDPWrap, Bind) { RETURN_FROM(DoBind(args, AF_INET)); }
  JS_METHOD_END

  JS_METHOD_NO_COM(UDPWrap, Bind6) { RETURN_FROM(DoBind(args, AF_INET6)); }
  JS_METHOD_END

#define X(name, fn)                                       \
  JS_METHOD_NO_COM(UDPWrap, name) ENGINE_UNWRAP(UDPWrap); \
  assert(args.Length() == 1);                             \
  int flag = args.GetInteger(0);                          \
  int r = fn(&wrap->handle_, flag);                       \
  if (r) SetErrno(uv_last_error(com->loop));              \
  RETURN_PARAM(STD_TO_INTEGER(r));                        \
  JS_METHOD_END

  X(SetTTL, uv_udp_set_ttl)
  X(SetBroadcast, uv_udp_set_broadcast)
  X(SetMulticastTTL, uv_udp_set_multicast_ttl)
  X(SetMulticastLoopback, uv_udp_set_multicast_loop)

#undef X

#define UDPSETMEM                                                        \
  JS_NATIVE_RETURN_TYPE UDPWrap::SetMembership(jxcore::PArguments& args, \
                                               uv_membership membership) {
  UDPSETMEM
  JS_ENTER_SCOPE_WITH(args.GetIsolate());
  ENGINE_UNWRAP(UDPWrap);
  JS_DEFINE_STATE_MARKER(com);
  {
    assert(args.Length() == 2);

    jxcore::JXString address;
    args.GetString(0, &address);
    jxcore::JXString iface_cstr;

    if (!args.IsUndefined(1) && !args.IsNull(1)) {
      args.GetString(1, &iface_cstr);
    }

    int r = uv_udp_set_membership(&wrap->handle_, *address, *iface_cstr,
                                  membership);

    if (r) SetErrno(uv_last_error(com->loop));

    RETURN_PARAM(STD_TO_INTEGER(r));
  }
  JS_METHOD_END

  JS_METHOD_NO_COM(UDPWrap, AddMembership) {
    RETURN_FROM(SetMembership(args, UV_JOIN_GROUP));
  }
  JS_METHOD_END

  JS_METHOD_NO_COM(UDPWrap, DropMembership) {
    RETURN_FROM(SetMembership(args, UV_LEAVE_GROUP));
  }
  JS_METHOD_END

#define UDP_DOSEND() \
  JS_NATIVE_RETURN_TYPE UDPWrap::DoSend(jxcore::PArguments& args, int family) {
  UDP_DOSEND()
  JS_ENTER_SCOPE();
  ENGINE_UNWRAP(UDPWrap);
  JS_DEFINE_STATE_MARKER(com);
  {
    int r;

    assert(args.Length() == 5);

    JS_LOCAL_OBJECT buffer_obj = JS_VALUE_TO_OBJECT(args.GetItem(0));

    size_t offset = args.GetUInteger(1);
    size_t length = args.GetUInteger(2);
    assert(offset < BUFFER__LENGTH(buffer_obj));
    assert(length <= BUFFER__LENGTH(buffer_obj) - offset);

    SendWrap* req_wrap = new SendWrap(com);
    JS_LOCAL_OBJECT objr = JS_TYPE_TO_LOCAL_OBJECT(req_wrap->object_);
    JS_NAME_SET_HIDDEN(objr, JS_PREDEFINED_STRING(buffer), buffer_obj);

    uv_buf_t buf = uv_buf_init(BUFFER__DATA(buffer_obj) + offset, length);

    const unsigned short port = args.GetUInteger(3);
    jxcore::JXString address;
    args.GetString(4, &address);

    switch (family) {
      case AF_INET:
        r = uv_udp_send(&req_wrap->req_, &wrap->handle_, &buf, 1,
                        uv_ip4_addr(*address, port), OnSend);
        break;
      case AF_INET6:
        r = uv_udp_send6(&req_wrap->req_, &wrap->handle_, &buf, 1,
                         uv_ip6_addr(*address, port), OnSend);
        break;
      default:
        assert(0 && "unexpected address family");
        abort();
    }

    req_wrap->Dispatched();

    if (r) {
      SetErrno(uv_last_error(com->loop));
      delete req_wrap;
      RETURN_PARAM(JS_NULL());
    } else {
      RETURN_PARAM(objr);
    }
  }
  JS_METHOD_END

  JS_METHOD_NO_COM(UDPWrap, Send) { RETURN_FROM(DoSend(args, AF_INET)); }
  JS_METHOD_END

  JS_METHOD_NO_COM(UDPWrap, Send6) { RETURN_FROM(DoSend(args, AF_INET6)); }
  JS_METHOD_END

  JS_METHOD_NO_COM(UDPWrap, RecvStart) {
    ENGINE_UNWRAP(UDPWrap);

    // UV_EALREADY means that the socket is already bound but that's okay
    int r = uv_udp_recv_start(&wrap->handle_, OnAlloc, OnRecv);
    if (r && uv_last_error(com->loop).code != UV_EALREADY) {
      SetErrno(uv_last_error(com->loop));
      RETURN_PARAM(STD_TO_BOOLEAN(false));
    }

    RETURN_PARAM(STD_TO_BOOLEAN(true));
  }
  JS_METHOD_END

  JS_METHOD_NO_COM(UDPWrap, RecvStop) {
    ENGINE_UNWRAP(UDPWrap);

    int r = uv_udp_recv_stop(&wrap->handle_);

    RETURN_PARAM(STD_TO_INTEGER(r));
  }
  JS_METHOD_END

  JS_METHOD_NO_COM(UDPWrap, GetSockName) {
    struct sockaddr_storage address;

    ENGINE_UNWRAP(UDPWrap);

    int addrlen = sizeof(address);
    int r = uv_udp_getsockname(&wrap->handle_,
                               reinterpret_cast<sockaddr*>(&address), &addrlen);

    if (r) {
      SetErrno(uv_last_error(com->loop));
      RETURN_PARAM(JS_NULL());
    }

    const sockaddr* addr = reinterpret_cast<const sockaddr*>(&address);
    RETURN_PARAM(AddressToJS(JS_GET_STATE_MARKER(), addr));
  }
  JS_METHOD_END

  // TODO(?) share with StreamWrap::AfterWrite() in stream_wrap.cc
  void UDPWrap::OnSend(uv_udp_send_t * req, int status) {
    assert(req != NULL);

    SendWrap* req_wrap = reinterpret_cast<SendWrap*>(req->data);
    UDPWrap* wrap = reinterpret_cast<UDPWrap*>(req->handle->data);
    node::commons* com = wrap->com;

    JS_ENTER_SCOPE_WITH(com->node_isolate);
    JS_DEFINE_STATE_MARKER(com);

    assert(JS_IS_EMPTY(req_wrap->object_) == false);
    assert(JS_IS_EMPTY(wrap->object_) == false);

    if (status) {
      SetErrno(uv_last_error(com->loop));
    }

    JS_LOCAL_OBJECT objr = JS_TYPE_TO_LOCAL_OBJECT(req_wrap->object_);
    JS_LOCAL_OBJECT objl = JS_TYPE_TO_LOCAL_OBJECT(wrap->object_);

    JS_LOCAL_VALUE argv[4] = {
        STD_TO_INTEGER(status), JS_TYPE_TO_LOCAL_VALUE(objl),
        JS_TYPE_TO_LOCAL_VALUE(objr),
        JS_GET_NAME_HIDDEN(objr, JS_PREDEFINED_STRING(buffer)),
    };

    MakeCallback(wrap->com, objr, JS_PREDEFINED_STRING(oncomplete),
                 ARRAY_SIZE(argv), argv);
    delete req_wrap;
  }

  uv_buf_t UDPWrap::OnAlloc(uv_handle_t * handle, size_t suggested_size) {
    UDPWrap* wrap = static_cast<UDPWrap*>(handle->data);
    node::commons* com = wrap->com;
    JS_DEFINE_STATE_MARKER(com);

    JS_LOCAL_OBJECT objl = JS_TYPE_TO_LOCAL_OBJECT(wrap->object_);
    char* buf = com->udp_slab_allocator->Allocate(objl, suggested_size);
    return uv_buf_init(buf, suggested_size);
  }

  void UDPWrap::OnRecv(uv_udp_t * handle, ssize_t nread, uv_buf_t buf,
                       struct sockaddr * addr, unsigned flags) {
    JS_ENTER_SCOPE();

    UDPWrap* wrap = reinterpret_cast<UDPWrap*>(handle->data);
    node::commons* com = wrap->com;
    JS_DEFINE_STATE_MARKER(com);

    JS_LOCAL_OBJECT objl = JS_TYPE_TO_LOCAL_OBJECT(wrap->object_);
    JS_LOCAL_OBJECT slab =
        com->udp_slab_allocator->Shrink(objl, buf.base, nread < 0 ? 0 : nread);
    if (nread == 0) return;

    if (nread < 0) {
      JS_LOCAL_VALUE argv[] = {JS_TYPE_TO_LOCAL_OBJECT(wrap->object_)};
      SetErrno(uv_last_error(com->loop));
      MakeCallback(com, objl, JS_PREDEFINED_STRING(onmessage), ARRAY_SIZE(argv),
                   argv);
      return;
    }

    JS_LOCAL_VALUE argv[] = {
        objl, slab, STD_TO_INTEGER(buf.base - BUFFER__DATA(slab)),
        STD_TO_INTEGER(nread), AddressToJS(JS_GET_STATE_MARKER(), addr)};
    MakeCallback(com, objl, JS_PREDEFINED_STRING(onmessage), ARRAY_SIZE(argv),
                 argv);
  }

  UDPWrap* UDPWrap::Unwrap(JS_LOCAL_OBJECT obj) {
    assert(!JS_IS_EMPTY(obj));
    assert(JS_OBJECT_FIELD_COUNT(obj) > 0);
    return static_cast<UDPWrap*>(JS_GET_POINTER_DATA(obj));
  }

  JS_LOCAL_OBJECT UDPWrap::Instantiate() {
    // If this assert fires then Initialize hasn't been called yet.
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);
    assert(JS_IS_EMPTY(com->udp_constructor) == false);

    JS_LOCAL_FUNCTION objuc = JS_TYPE_TO_LOCAL_FUNCTION(com->udp_constructor);
    JS_LOCAL_OBJECT obj = JS_NEW_DEFAULT_INSTANCE(objuc);

    return JS_LEAVE_SCOPE(obj);
  }

  uv_udp_t* UDPWrap::UVHandle() { return &handle_; }
}  // namespace node

NODE_MODULE(node_udp_wrap, node::UDPWrap::Initialize)
