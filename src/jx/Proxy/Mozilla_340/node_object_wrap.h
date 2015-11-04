// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_PROXY_MOZILLA_NODE_OBJECT_WRAP_H_
#define SRC_PROXY_MOZILLA_NODE_OBJECT_WRAP_H_

#include "../JSEngine.h"
#include <assert.h>

namespace node {

class NODE_EXTERN ObjectWrap {
 public:
  ObjectWrap() { refs_ = 0; }

  virtual ~ObjectWrap() {
    if (!JS_IS_EMPTY(handle_)) {
      if (!MozJS::EngineHelper::IsInstanceAlive(handle_->GetContext())) return;
      JS_CLEAR_PERSISTENT(handle_);
    }
  }

  template <class T>
  static inline T *Unwrap(JS_HANDLE_OBJECT handle) {
    return static_cast<T *>(JS_GET_POINTER_DATA(handle));
  }

  JS_PERSISTENT_OBJECT handle_;  // ro

 protected:
  inline void Wrap(JS_HANDLE_OBJECT handle) {
    assert(JS_IS_EMPTY(handle_));

    JS_NEW_PERSISTENT_OBJECT(handle_, handle);
    JS_SET_POINTER_DATA(handle_, this);
    MakeWeak();
  }

  inline void MakeWeak(void) {
    handle_.MakeWeak();
  }

  /* Ref() marks the object as being attached to an event loop.
   * Refed objects will not be garbage collected, even if
   * all references are lost.
   */
  virtual void Ref() {
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
    assert(refs_ > 0);
    if (--refs_ == 0) {
      MakeWeak();
    }
  }

  int refs_;  // ro

 public:
  static void WeakCallback(JSFreeOp *fop, JSObject *_this) {
    if (!JS_HasPrivate(_this)) return;
    void *__this = JS_GetPrivate(_this);
    if (__this == NULL) return;
    ObjectWrap *obj = static_cast<ObjectWrap *>(__this);

    assert(!obj->refs_);
    delete obj;
  }
};

}  // namespace node
#endif  // SRC_PROXY_MOZILLA_NODE_OBJECT_WRAP_H_
