// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_HANDLE_WRAP_H_
#define SRC_WRAPPERS_HANDLE_WRAP_H_

#include "node.h"
#include "queue.h"

namespace node {

// Rules:
//
// - Do not throw from handle methods. Set errno.
//
// - MakeCallback may only be made directly off the event loop.
//   That is there can be no JavaScript stack frames underneith it.
//   (Is there anyway to assert that?)
//
// - No use of v8::WeakReferenceCallback. The close callback signifies that
//   we're done with a handle - external resources can be freed.
//
// - Reusable?
//
// - The uv_close_cb is used to free the c++ object. The close callback
//   is not made into javascript land.
//
// - uv_ref, uv_unref counts are managed at this layer to avoid needless
//   js/c++ boundary crossing. At the javascript layer that should all be
//   taken care of.

#define UNWRAP_NO_ABORT(type)                      \
  assert(!args.Holder().IsEmpty());                \
  assert(args.Holder()->InternalFieldCount() > 0); \
  type* wrap =                                     \
      static_cast<type*>(args.Holder()->GetAlignedPointerFromInternalField(0));

class commons;

class HandleWrap {
 public:
  node::commons* com;

  static DECLARE_CLASS_INITIALIZER(Initialize);

  static DEFINE_JS_METHOD(Close);
  static DEFINE_JS_METHOD(Ref);
  static DEFINE_JS_METHOD(Unref);

  inline uv_handle_t* GetHandle() { return handle__; }

 protected:
  HandleWrap(JS_HANDLE_OBJECT_REF object, uv_handle_t* handle);
  virtual ~HandleWrap();

  virtual void SetHandle(uv_handle_t* h);
  void unref();
  void ref();

  JS_PERSISTENT_OBJECT object_;

 private:
  friend DEFINE_JS_METHOD(GetActiveHandles);
  static void OnClose(uv_handle_t* handle);
  QUEUE handle_wrap_queue_;
  // Using double underscore due to handle_ member in tcp_wrap. Probably
  // tcp_wrap should rename it's member to 'handle'.
  uv_handle_t* handle__;
  unsigned int flags_;

  static const unsigned int kUnref = 1;
  static const unsigned int kCloseCallback = 2;
};

}  // namespace node

#endif  // SRC_WRAPPERS_HANDLE_WRAP_H_
