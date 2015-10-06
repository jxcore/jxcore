// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JXCORE_TYPE_WRAP_H_
#define SRC_JXCORE_TYPE_WRAP_H_
#include "jx/commons.h"
#include "public/jx_result.h"

namespace jxcore {

class JXMethod {
 public:
  JS_NATIVE_METHOD native_method_;
  bool is_native_method_;
  int interface_id_;

  JXMethod() : native_method_(0), is_native_method_(false), interface_id_(-1) {}
};

class JXEngine;

class JXFunctionWrapper {
  friend class JXEngine;
  node::commons *com_;
  JS_PERSISTENT_FUNCTION pst_fnc_;
  JS_PERSISTENT_OBJECT pst_obj_;

 public:
  JXFunctionWrapper(node::commons *com, JS_HANDLE_FUNCTION fnc,
                    JS_HANDLE_VALUE_REF obj) {
    com_ = com;
    JS_DEFINE_STATE_MARKER(com);
    JS_NEW_PERSISTENT_FUNCTION(pst_fnc_, fnc);
    if (!JS_IS_EMPTY(obj) && !JS_IS_UNDEFINED(obj)) {
      JS_NEW_PERSISTENT_OBJECT(pst_obj_, JS_VALUE_TO_OBJECT(obj));
    }
  }

#if defined(JS_ENGINE_MOZJS) || defined(V8_IS_3_14)
  static void dummyWeakCallback(JS_PERSISTENT_VALUE_REF value, void *data) {}
#define DWCBO_FNC dummyWeakCallback
#define DWCBO_OBJ dummyWeakCallback
#else
  static JS_WEAK_CALLBACK_METHOD(ENGINE_NS::Object, void,
                                 dummyWeakCallbackObject) {}
  static JS_WEAK_CALLBACK_METHOD(ENGINE_NS::Function, void,
                                 dummyWeakCallbackFunction) {}
#define DWCBO_OBJ dummyWeakCallbackObject
#define DWCBO_FNC dummyWeakCallbackFunction
#endif

  void Dispose() {
    if (com_ == NULL) return;
    com_ = NULL;

    if (!JS_IS_EMPTY(pst_fnc_))
      JS_MAKE_WEAK(pst_fnc_, (void*)NULL, DWCBO_FNC);

    if (!JS_IS_EMPTY(pst_obj_))
      JS_MAKE_WEAK(pst_obj_, (void*)NULL, DWCBO_OBJ);
  }

  inline JS_LOCAL_FUNCTION GetFunction() {
    assert(com_ != NULL && "JXFunctionWrapper was already disposed.");
    JS_DEFINE_STATE_MARKER(com_);
    return JS_TYPE_TO_LOCAL_FUNCTION(pst_fnc_);
  }

  ~JXFunctionWrapper() { Dispose(); }

  JS_HANDLE_VALUE Call(const int argc, JS_HANDLE_VALUE args[], bool *success) {
    assert(com_ != NULL && "JXFunctionWrapper was already disposed.");

    JS_ENTER_SCOPE_WITH(com_->node_isolate);
    JS_DEFINE_STATE_MARKER(com_);

    JS_TRY_CATCH(tc);
    JS_LOCAL_VALUE res;

    JS_LOCAL_FUNCTION l_fnc = JS_TYPE_TO_LOCAL_FUNCTION(pst_fnc_);
    if (!JS_IS_EMPTY(pst_obj_)) {
      JS_LOCAL_OBJECT l_obj = JS_TYPE_TO_LOCAL_OBJECT(pst_obj_);
      res = JS_METHOD_CALL(l_fnc, l_obj, argc, args);
    } else {
      JS_HANDLE_OBJECT glob = JS_GET_GLOBAL();
      res = JS_METHOD_CALL(l_fnc, glob, argc, args);
    }

    if (tc.HasCaught()) {
      *success = false;
      return JS_LEAVE_SCOPE(tc.Exception());
    }

    *success = true;
    return JS_LEAVE_SCOPE(res);
  }
};

class JXValueWrapper {
 public:
  JS_PERSISTENT_VALUE value_;

  JXValueWrapper() {}

  JXValueWrapper(node::commons *com, JS_HANDLE_VALUE_REF value) {
    JS_DEFINE_STATE_MARKER(com);
    JS_NEW_PERSISTENT_VALUE(value_, value);
  }

  ~JXValueWrapper() {
    if (!JS_IS_EMPTY(value_)) {
      JS_CLEAR_PERSISTENT(value_);
    }
  }
};
}  // namespace jxcore

#endif  // SRC_JXCORE_TYPE_WRAP_H_
