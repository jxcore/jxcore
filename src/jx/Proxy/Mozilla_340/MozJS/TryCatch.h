// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_MOZJS_TRYCATCH_H_
#define SRC_JX_PROXY_MOZILLA_MOZJS_TRYCATCH_H_

#include <string>
#include "MozValue.h"
#include "../EngineHelper.h"

namespace MozJS {
class TryCatch {
  bool empty_;
  MozJS::Value holder_;

 public:
  JSContext *ctx_;

  explicit TryCatch(JSContext *ctx) {
    ctx_ = ctx;
    empty_ = true;
  }

  void ClearPendingException() { JS_ClearPendingException(ctx_); }

  ~TryCatch() {
    if (!empty_) {
      empty_ = true;  // !! before delete!
      holder_.RemoveRoot();
    }
  }

  void SetHolder(MozJS::Value val) {
    if (!empty_) {
      empty_ = true;
      holder_.RemoveRoot();
    }
    holder_ = val;
    holder_.AddRoot();
    empty_ = false;
  }

  bool HasHolder() const { return !empty_; }

  bool HasCaught() { return JS_IsExceptionPending(ctx_); }

  bool HasTerminated() { return true; }

  bool CanContinue() { return true; }

  MozJS::Value Exception() {
    if (!empty_) {
      return holder_;
    }

    if (HasCaught()) {
      JS::RootedValue value(ctx_);
      if (JS_GetPendingException(ctx_, &value)) {
        JS_ClearPendingException(ctx_);

        return Value(value.get(), ctx_);
      }
    }

    return MozJS::String::FromSTD(ctx_, "NO EXCEPTION THROWN", 0);
  }

  MozJS::Value Message() { return Exception(); }
};
}  // namespace MozJS

#endif  // SRC_JX_PROXY_MOZILLA_MOZJS_TRYCATCH_H_
