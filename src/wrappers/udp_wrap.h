// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_UDP_WRAP_H_
#define SRC_WRAPPERS_UDP_WRAP_H_

#include "node.h"
#include "jx/commons.h"
#include "handle_wrap.h"

#if defined(__ANDROID__) || defined(__IOS__) || defined(__WIN_RT__)
#define SLAB_SIZE (512 * 1024)
#elif defined(__ARM__) || defined(__ARMEB__) || defined(__ARM_EABI__)
#define SLAB_SIZE (1024 * 1024)
#else
#define SLAB_SIZE (8 * 1024 * 1024)
#endif

namespace node {

class UDPWrap : public HandleWrap {
 public:
  static JS_DEFINE_GETTER_METHOD(GetFD);

  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(Bind);
  static DEFINE_JS_METHOD(Send);
  static DEFINE_JS_METHOD(Bind6);
  static DEFINE_JS_METHOD(Send6);
  static DEFINE_JS_METHOD(RecvStart);
  static DEFINE_JS_METHOD(RecvStop);
  static DEFINE_JS_METHOD(GetSockName);
  static DEFINE_JS_METHOD(AddMembership);
  static DEFINE_JS_METHOD(DropMembership);
  static DEFINE_JS_METHOD(SetMulticastTTL);
  static DEFINE_JS_METHOD(SetMulticastLoopback);
  static DEFINE_JS_METHOD(SetBroadcast);
  static DEFINE_JS_METHOD(SetTTL);

  static UDPWrap* Unwrap(JS_LOCAL_OBJECT obj);

  static JS_LOCAL_OBJECT Instantiate();
  uv_udp_t* UVHandle();

 private:
  explicit UDPWrap(JS_HANDLE_OBJECT object);
  virtual ~UDPWrap();

  static JS_NATIVE_RETURN_TYPE DoBind(jxcore::PArguments& args, int family);
  static JS_NATIVE_RETURN_TYPE DoSend(jxcore::PArguments& args, int family);
  static JS_NATIVE_RETURN_TYPE SetMembership(jxcore::PArguments& args,
                                             uv_membership membership);

  static uv_buf_t OnAlloc(uv_handle_t* handle, size_t suggested_size);
  static void OnSend(uv_udp_send_t* req, int status);
  static void OnRecv(uv_udp_t* handle, ssize_t nread, uv_buf_t buf,
                     struct sockaddr* addr, unsigned flags);

  static void DeleteSlabAllocator(void*);

  uv_udp_t handle_;

  INIT_NAMED_CLASS_MEMBERS(UDP, UDPWrap)
  HandleWrap::Initialize(target);

  com->udp_slab_allocator = new SlabAllocator(SLAB_SIZE, com);
  AtExit(DeleteSlabAllocator, NULL);

#ifdef JS_ENGINE_V8
  enum v8::PropertyAttribute attributes =
      static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete);
  constructor->InstanceTemplate()->SetAccessor(STD_TO_STRING("fd"),
                                               UDPWrap::GetFD, NULL,
                                               JS_HANDLE_VALUE(), v8::DEFAULT,
                                               attributes);
#elif defined(JS_ENGINE_MOZJS)
  JS_ACCESSOR_SET(constructor, STD_TO_STRING("fd"), UDPWrap::GetFD, NULL);
#endif

  SET_INSTANCE_METHOD("bind", Bind, 3);
  SET_INSTANCE_METHOD("send", Send, 5);
  SET_INSTANCE_METHOD("bind6", Bind6, 3);
  SET_INSTANCE_METHOD("send6", Send6, 5);
  SET_INSTANCE_METHOD("close", Close, 0);
  SET_INSTANCE_METHOD("recvStart", RecvStart, 0);
  SET_INSTANCE_METHOD("recvStop", RecvStop, 0);
  SET_INSTANCE_METHOD("getsockname", GetSockName, 0);
  SET_INSTANCE_METHOD("addMembership", AddMembership, 2);
  SET_INSTANCE_METHOD("dropMembership", DropMembership, 2);
  SET_INSTANCE_METHOD("setMulticastTTL", SetMulticastTTL, 1);
  SET_INSTANCE_METHOD("setMulticastLoopback", SetMulticastLoopback, 1);
  SET_INSTANCE_METHOD("setBroadcast", SetBroadcast, 1);
  SET_INSTANCE_METHOD("setTTL", SetTTL, 1);

  SET_INSTANCE_METHOD("ref", HandleWrap::Ref, 0);
  SET_INSTANCE_METHOD("unref", HandleWrap::Unref, 0);

  com->udp_constructor =
      JS_NEW_PERSISTENT_FUNCTION(JS_GET_FUNCTION(constructor));

  END_INIT_NAMED_MEMBERS(UDP)
};

}  // namespace node

#endif  // SRC_WRAPPERS_UDP_WRAP_H_
