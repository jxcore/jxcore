// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_PROXY_V8_NODE_OBJECT_WRAP_H_
#define SRC_PROXY_V8_NODE_OBJECT_WRAP_H_

#include "jx/Proxy/JSEngine.h"
#include <assert.h>

// Explicitly instantiate some template classes, so we're sure they will be
// present in the binary / shared object. There isn't much doubt that they will
// be, but MSVC tends to complain about these things.
#if defined(_MSC_VER)
template class NODE_EXTERN JS_PERSISTENT_OBJECT;
template class NODE_EXTERN JS_PERSISTENT_FUNCTION_TEMPLATE;
#endif

namespace node {

#ifdef _WIN32
class NODE_EXTERN ObjectWrap {
#else
// ugly eclipse syntax error
class ObjectWrap {
#endif
 public:
  ObjectWrap() { refs_ = 0; }

  virtual ~ObjectWrap() {
    if (!JS_IS_EMPTY(handle_)) {
      assert(handle_.IsNearDeath());
      handle_.ClearWeak();
      JS_CLEAR_PERSISTENT(handle_);
    }
  }

  template <class T>
  static inline T *Unwrap(JS_HANDLE_OBJECT handle) {
    assert(!JS_IS_EMPTY(handle));
    assert(handle->InternalFieldCount() > 0);
    return static_cast<T *>(JS_GET_POINTER_DATA(handle));
  }

  JS_PERSISTENT_OBJECT handle_;  // ro

 protected:
  inline void Wrap(JS_HANDLE_OBJECT handle) {
    assert(JS_IS_EMPTY(handle_));
    assert(handle->InternalFieldCount() > 0);

    JS_DEFINE_CURRENT_MARKER();
    handle_.Reset(JS_GET_STATE_MARKER(), handle);
    JS_SET_POINTER_DATA(handle, this);
    MakeWeak();
  }

  inline void MakeWeak(void) {
    handle_.SetWeak(this, WeakCallback);
    handle_.MarkIndependent();
  }

  /* Ref() marks the object as being attached to an event loop.
   * Refed objects will not be garbage collected, even if
   * all references are lost.
   */
  virtual void Ref() {
    assert(!JS_IS_EMPTY(handle_));
    handle_.ClearWeak();
    refs_++;
  }

  /* Unref() marks an object as detached from the event loop.  This is its
   * default state.  When an object with a "weak" reference changes from
   * attached to detached state it will be freed. Be careful not to access
   * the object after making this call as it might be gone!
   * (A "weak reference" means an object that only has a
   * persistant handle.)
   *
   * DO NOT CALL THIS FROM DESTRUCTOR
   */
  virtual void Unref() {
    assert(!handle_.IsEmpty());
    assert(!handle_.IsWeak());
    assert(refs_ > 0);
    if (--refs_ == 0) {
      MakeWeak();
    }
  }

  int refs_;  // ro

 private:
  static void WeakCallback(
      const v8::WeakCallbackData<v8::Object, ObjectWrap> &data) {
    v8::Isolate *isolate = data.GetIsolate();
    v8::HandleScope scope(isolate);
    ObjectWrap* wrap = data.GetParameter();
    assert(wrap->refs_ == 0);
    assert(wrap->handle_.IsNearDeath());
    assert(
        data.GetValue() == v8::Local<v8::Object>::New(isolate, wrap->handle_));
    wrap->handle_.Reset();
    delete wrap;
  }
};

}  // namespace node
#endif  // SRC_PROXY_V8_NODE_OBJECT_WRAP_H_
