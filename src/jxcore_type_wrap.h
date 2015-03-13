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
    pst_fnc_ = JS_NEW_PERSISTENT_FUNCTION(fnc);
    if (!JS_IS_EMPTY(obj) && !JS_IS_UNDEFINED(obj)) {
      pst_obj_ = JS_NEW_PERSISTENT_OBJECT(JS_VALUE_TO_OBJECT(obj));
    }
  }

  void Dispose() {
    JS_CLEAR_PERSISTENT(pst_fnc_);
    JS_CLEAR_PERSISTENT(pst_obj_);
  }

  ~JXFunctionWrapper() { Dispose(); }

  JS_HANDLE_VALUE Call(const int argc, JS_HANDLE_VALUE args[], bool *success) {
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(com_);

    JS_TRY_CATCH(tc);
    JS_LOCAL_VALUE res;

    if (!JS_IS_EMPTY(pst_obj_)) {
      res = JS_METHOD_CALL(pst_fnc_, pst_obj_, argc, args);
    } else {
      JS_HANDLE_OBJECT glob = JS_GET_GLOBAL();
      res = JS_METHOD_CALL(pst_fnc_, glob, argc, args);
    }

    if (tc.HasCaught()) {
      *success = false;
      return JS_LEAVE_SCOPE(tc.Exception());
    }

    *success = true;
    return JS_LEAVE_SCOPE(res);
  }
};
}  // namespace jxcore

#endif  // SRC_JXCORE_TYPE_WRAP_H_
