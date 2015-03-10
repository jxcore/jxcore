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

 public:
  jsval *args_;
  bool ret_val_;

  PArguments(JSContext *ctx, int argc, JS::Value *__jsval);

  inline MozJS::Isolate *GetIsolate() {
    return MozJS::Isolate::GetByThreadId(JS_GetThreadId(ctx_));
  }

  inline JSContext *GetContext() { return ctx_; }

  inline int Length() const { return argc_; }

  int GetUTF8Length(const unsigned index);

  bool IsConstructCall();

  bool IsDate(const unsigned index) const;
  bool IsRegExp(const unsigned index) const;
  bool IsArray(const unsigned index) const;
  bool IsString(const unsigned index) const;
  bool IsInteger(const unsigned index) const;
  bool IsUnsigned(const unsigned index) const;
  bool IsNumber(const unsigned index) const;
  bool IsBoolean(const unsigned index) const;
  bool IsFunction(const unsigned index) const;
  bool IsObject(const unsigned index) const;
  bool IsNull(const unsigned index) const;
  bool IsUndefined(const unsigned index) const;
  bool IsBooleanOrNull(const unsigned index) const;
  bool IsStringOrNull(const unsigned index) const;

  JS_HANDLE_VALUE GetItem(const unsigned index) const;
  JS_HANDLE_OBJECT GetAsArray(const unsigned index) const;
  JS_HANDLE_FUNCTION GetAsFunction(const unsigned index) const;
  JS_HANDLE_STRING GetAsString(const unsigned index) const;
  int64_t GetInteger(const unsigned index);
  int32_t GetInt32(const unsigned index);
  unsigned GetUInteger(const unsigned index);
  bool GetBoolean(const unsigned index);
  int GetString(const unsigned index, jxcore::JXString *jxs);
  double GetNumber(const unsigned index);
  JS_HANDLE_OBJECT This();

  void *GetHolder();

  JS_HANDLE_OBJECT Holder() {
    return This();  // check this out
  }

  void close(JS::Value val);
  void close(MozJS::Value *val);
  void close(MozJS::Value val);
  void close(MozJS::ThrowException ex);
  void close(bool ret_val);
  void close();
};
}  // namespace jxcore
#endif  // SRC_JX_PROXY_MOZILLA_PARGUMENTS_H_
