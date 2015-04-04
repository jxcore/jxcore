// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_MOZJS_EXCEPTION_H_
#define SRC_JX_PROXY_MOZILLA_MOZJS_EXCEPTION_H_

#include "MozValue.h"
#include "../SpiderHelper.h"
#include "../MozTypes.h"

#define ERROR_CLASSES(type)                                          \
 public:                                                             \
  type(MozJS::String err_msg) {                                      \
    assert(!err_msg.IsEmpty());                                      \
    ctx_ = err_msg.ctx_;                                             \
    MozJS::Value ret_val;                                            \
    EngineHelper::CreateObject(err_msg.ctx_, #type, &ret_val);       \
    JS::RootedValue rt_err_msg(err_msg.ctx_, err_msg.GetRawValue()); \
    ret_val.SetProperty("message", rt_err_msg);                      \
    err_msg_ = ret_val;                                              \
  }

namespace MozJS {
namespace Exception {
class TypeError;
class RangeError;

class Error {
  friend class TypeError;
  friend class RangeError;

 protected:
  MozJS::Value err_msg_;

  Error() { ctx_ = NULL; }

 public:
  JSContext* ctx_;

  explicit Error(MozJS::Value err_msg) {
    err_msg_ = err_msg;
    ctx_ = err_msg_.ctx_;
  }

  inline MozJS::Value GetErrorObject() { return err_msg_; }

  jsval GetRawValue() {
    assert(!err_msg_.IsEmpty());
    return err_msg_.GetRawValue();
  }

  JSContext* GetContext() { return err_msg_.ctx_; }
  ERROR_CLASSES(Error)
};

class RangeError : public Error {
 public:
  explicit RangeError(MozJS::Value err_msg) : Error(err_msg) {}
  ERROR_CLASSES(RangeError)
};

class TypeError : public Error {
 public:
  explicit TypeError(MozJS::Value err_msg) : Error(err_msg) {}
  ERROR_CLASSES(TypeError)
};
}  // namespace Exception

class ThrowException : public Value {
 public:
  explicit ThrowException(Exception::Error err);
  explicit ThrowException(Exception::TypeError err);
  explicit ThrowException(Exception::RangeError err);
  explicit ThrowException(MozJS::Value err);
};
}  // namespace MozJS

#endif  // SRC_JX_PROXY_MOZILLA_MOZJS_EXCEPTION_H_
