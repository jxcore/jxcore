// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_NODE_OBJECT_WRAP_H_
#define SRC_NODE_OBJECT_WRAP_H_

#include "jx/Proxy/JSEngine.h"
#include <assert.h>

#define flush_console(...)        \
  do {                            \
    fprintf(stdout, __VA_ARGS__); \
    fflush(stdout);               \
  } while (0)
// Explicitly instantiate some template classes, so we're sure they will be
// present in the binary / shared object. There isn't much doubt that they will
// be, but MSVC tends to complain about these things.
#if defined(_MSC_VER) && defined(JS_ENGINE_V8)
template class NODE_EXTERN JS_PERSISTENT_OBJECT;
template class NODE_EXTERN JS_PERSISTENT_FUNCTION_TEMPLATE;
#endif

namespace node {

class NODE_EXTERN ObjectWrap {
 public:
  ObjectWrap() { refs_ = 0; }

  virtual ~ObjectWrap() {
    if (!JS_IS_EMPTY(handle_)) {
#ifdef JS_ENGINE_V8
      assert(handle_.IsNearDeath());
      handle_.ClearWeak();
      handle_->SetPointerInInternalField(0, 0);
#elif defined(JS_ENGINE_MOZJS)
      if (!MozJS::EngineHelper::IsInstanceAlive(handle_->ctx_)) return;
#endif
      JS_CLEAR_PERSISTENT(handle_);
    }
  }

  template <class T>
  static inline T *Unwrap(JS_HANDLE_OBJECT handle) {
#ifdef JS_ENGINE_V8
    assert(!JS_IS_EMPTY(handle));
    assert(handle->InternalFieldCount() > 0);
#endif
    return static_cast<T *>(JS_GET_POINTER_DATA(handle));
  }

  JS_PERSISTENT_OBJECT handle_;  // ro

 protected:
  inline void Wrap(JS_HANDLE_OBJECT handle) {
    assert(JS_IS_EMPTY(handle_));

#ifdef JS_ENGINE_V8
    assert(handle->InternalFieldCount() > 0);
#endif

    handle_ = JS_NEW_PERSISTENT_OBJECT(handle);
    JS_SET_POINTER_DATA(handle_, this);
    MakeWeak();
  }

  inline void MakeWeak(void) {
#ifdef JS_ENGINE_V8
    handle_.MakeWeak(this, WeakCallback);
    handle_.MarkIndependent();
#elif defined(JS_ENGINE_MOZJS)
    handle_.MakeWeak();
#endif
  }

  /* Ref() marks the object as being attached to an event loop.
   * Refed objects will not be garbage collected, even if
   * all references are lost.
   */
  virtual void Ref() {
#ifdef JS_ENGINE_V8
    assert(!JS_IS_EMPTY(handle_));
#endif
    refs_++;
    handle_.ClearWeak();
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
#ifdef JS_ENGINE_V8
    assert(!handle_.IsEmpty());
    assert(!handle_.IsWeak());
#endif
    assert(refs_ > 0);
    if (--refs_ == 0) {
      MakeWeak();
    }
  }

  int refs_;  // ro

#ifdef JS_ENGINE_V8
 private:
  static void WeakCallback(JS_PERSISTENT_VALUE value, void *data) {
    JS_ENTER_SCOPE();
    ObjectWrap *obj = static_cast<ObjectWrap *>(data);
#elif defined(JS_ENGINE_MOZJS)
 public:
  static void WeakCallback(JSFreeOp *fop, JSObject *_this) {
    if (!JS_HasPrivate(_this)) return;
    void *__this = JS_GetPrivate(_this);
    if (__this == NULL) return;
    ObjectWrap *obj = static_cast<ObjectWrap *>(__this);
#endif

    assert(!obj->refs_);
#ifdef JS_ENGINE_V8
    assert(value == obj->handle_);
    assert(value.IsNearDeath());
#endif
    delete obj;
  }
};

}  // namespace node
#endif  // SRC_NODE_OBJECT_WRAP_H_
