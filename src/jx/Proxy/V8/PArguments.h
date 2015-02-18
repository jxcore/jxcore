// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_V8_PARGUMENTS_H_
#define SRC_JX_PROXY_V8_PARGUMENTS_H_

#include "v8.h"
#include "JXString.h"

namespace jxcore {

class PArguments {
  const v8::Arguments *args__;

#define args_ (*args__)
  int length;

 public:
  explicit PArguments(const v8::Arguments &args)
      : args__(&args), length(args.Length()) {}

  void *GetHolder() { return args_.Holder()->GetPointerFromInternalField(0); }

  const v8::Arguments *GetArgs() { return args__; }

  bool IsConstructCall() { return args_.IsConstructCall(); }

  JS_HANDLE_OBJECT This() { return args_.This(); }

  JS_HANDLE_OBJECT Holder() { return args_.Holder(); }

  int Length() const { return length; }

  v8::Isolate *GetMarker() { return args_.GetIsolate(); }

  bool IsFunction(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsFunction();
  }

  bool IsObject(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsObject();
  }

  bool IsArray(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsArray();
  }

  bool IsDate(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsDate();
  }

  bool IsRegExp(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsRegExp();
  }

  bool IsNull(const unsigned index) const {
    if (index >= length) return true;
    return args_[index]->IsNull();
  }

  bool IsUndefined(const unsigned index) const {
    if (index >= length) return true;
    return args_[index]->IsUndefined();
  }

  bool IsNumber(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsNumber();
  }

  bool IsInteger(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsInt32();
  }

  bool IsUnsigned(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsUint32();
  }

  bool IsNumberOrNull(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsNumber() || args_[index]->IsNull();
  }

  bool IsBoolean(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsBoolean();
  }

  bool IsBooleanOrNull(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsBoolean() || args_[index]->IsNull();
  }

  bool IsString(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsString();
  }

  bool IsStringOrNull(const unsigned index) const {
    if (index >= length) return false;
    return args_[index]->IsString() || args_[index]->IsNull();
  }

  bool GetBoolean(const unsigned index) { return args_[index]->BooleanValue(); }

  int64_t GetInteger(const unsigned index) {
    return args_[index]->IntegerValue();
  }

  int32_t GetInt32(const unsigned index) { return args_[index]->Int32Value(); }

  unsigned GetUInteger(const unsigned index) {
    return args_[index]->Uint32Value();
  }

  double GetNumber(const unsigned index) { return args_[index]->NumberValue(); }

  int GetString(const unsigned index, JXString *jxs) {
    jxs->set_handle(args_[index]);

    return jxs->length();
  }

  int GetUTF8Length(const unsigned index) {
    return args_[index]->ToString()->Utf8Length();
  }

  JS_HANDLE_VALUE GetItem(const unsigned index) const { return args_[index]; }

  JS_HANDLE_STRING GetAsString(const unsigned index) const {
    return args_[index].As<v8::String>();
  }

  JS_HANDLE_FUNCTION GetAsFunction(const unsigned index) const {
    return args_[index].As<v8::Function>();
  }

  JS_HANDLE_ARRAY GetAsArray(const unsigned index) const {
    return args_[index].As<v8::Array>();
  }

#undef args_
};
}  // namespace jxcore
#endif  // SRC_JX_PROXY_V8_PARGUMENTS_H_
