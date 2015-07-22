// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_PARGUMENTS_H_
#define SRC_JX_PROXY_MOZILLA_PARGUMENTS_H_

#include "../JSEngine.h"
#include "JXString.h"

namespace jxcore {

class PArguments {
  bool close_called_;
  JSContext *ctx_;
  unsigned argc_;
  jsval *jsval_;
  jsval *args_;
  bool ret_val_;

 public:

  JXCORE_PUBLIC inline jsval* get_args_() { return args_; }
  JXCORE_PUBLIC inline bool get_ret_val_() { return ret_val_; }
  JXCORE_PUBLIC inline void set_ret_val_(bool rv) { ret_val_ = rv; }

  JXCORE_PUBLIC PArguments(JSContext *ctx, int argc, JS::Value *__jsval);

  JXCORE_PUBLIC inline MozJS::Isolate *GetIsolate() {
    return MozJS::Isolate::GetByThreadId(JS_GetThreadId(ctx_));
  }

  JXCORE_PUBLIC inline JSContext *GetContext() { return ctx_; }

  JXCORE_PUBLIC inline int Length() const { return argc_; }

  JXCORE_PUBLIC int GetUTF8Length(const unsigned index);

  JXCORE_PUBLIC bool IsConstructCall();

  JXCORE_PUBLIC bool IsDate(const unsigned index) const;
  JXCORE_PUBLIC bool IsRegExp(const unsigned index) const;
  JXCORE_PUBLIC bool IsArray(const unsigned index) const;
  JXCORE_PUBLIC bool IsString(const unsigned index) const;
  JXCORE_PUBLIC bool IsInteger(const unsigned index) const;
  JXCORE_PUBLIC bool IsUnsigned(const unsigned index) const;
  JXCORE_PUBLIC bool IsNumber(const unsigned index) const;
  JXCORE_PUBLIC bool IsBoolean(const unsigned index) const;
  JXCORE_PUBLIC bool IsFunction(const unsigned index) const;
  JXCORE_PUBLIC bool IsObject(const unsigned index) const;
  JXCORE_PUBLIC bool IsNull(const unsigned index) const;
  JXCORE_PUBLIC bool IsUndefined(const unsigned index) const;
  JXCORE_PUBLIC bool IsBooleanOrNull(const unsigned index) const;
  JXCORE_PUBLIC bool IsStringOrNull(const unsigned index) const;

  JXCORE_PUBLIC JS_HANDLE_VALUE GetItem(const unsigned index) const;
  JXCORE_PUBLIC JS_HANDLE_OBJECT GetAsArray(const unsigned index) const;
  JXCORE_PUBLIC JS_HANDLE_FUNCTION GetAsFunction(const unsigned index) const;
  JXCORE_PUBLIC JS_HANDLE_STRING GetAsString(const unsigned index) const;
  JXCORE_PUBLIC int64_t GetInteger(const unsigned index);
  JXCORE_PUBLIC int32_t GetInt32(const unsigned index);
  JXCORE_PUBLIC unsigned GetUInteger(const unsigned index);
  JXCORE_PUBLIC bool GetBoolean(const unsigned index);
  JXCORE_PUBLIC int GetString(const unsigned index, jxcore::JXString *jxs);
  JXCORE_PUBLIC double GetNumber(const unsigned index);
  JXCORE_PUBLIC JS_HANDLE_OBJECT This();

  JXCORE_PUBLIC void *GetHolder();

  JXCORE_PUBLIC JS_HANDLE_OBJECT Holder() {
    return This();  // check this out
  }

  JXCORE_PUBLIC void close(JS::Value val);
  JXCORE_PUBLIC void close(MozJS::Value *val);
  JXCORE_PUBLIC void close(MozJS::Value val);
  JXCORE_PUBLIC void close(MozJS::ThrowException ex);
  JXCORE_PUBLIC void close(bool ret_val);
  JXCORE_PUBLIC void close();
};
}  // namespace jxcore
#endif  // SRC_JX_PROXY_MOZILLA_PARGUMENTS_H_
