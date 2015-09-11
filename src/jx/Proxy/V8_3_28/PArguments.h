// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_V8_PARGUMENTS_H_
#define SRC_JX_PROXY_V8_PARGUMENTS_H_

#include "v8.h"
#include "JXString.h"

namespace jxcore {

class PArguments {
  const JS_V8_ARGUMENT *args__;

#define v8__args (*args__)
  unsigned length_;

 public:
  explicit PArguments(const JS_V8_ARGUMENT &args)
      : args__(&args), length_(args.Length()) {}

  inline v8::Local<v8::Context> GetContext() {
    return v8__args.GetIsolate()->GetCurrentContext();
  }

  inline v8::Isolate *GetIsolate() { return v8__args.GetIsolate(); }

  void *GetHolder() {
    return v8__args.Holder()->GetAlignedPointerFromInternalField(0);
  }

  const JS_V8_ARGUMENT *GetArgs() { return args__; }

  inline bool IsConstructCall() { return v8__args.IsConstructCall(); }

  inline JS_HANDLE_OBJECT This() { return v8__args.This(); }

  inline JS_HANDLE_OBJECT Holder() { return v8__args.Holder(); }

  inline int Length() const { return length_; }

  bool IsFunction(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsFunction();
  }

  bool IsObject(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsObject();
  }

  bool IsArray(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsArray();
  }

  bool IsDate(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsDate();
  }

  bool IsRegExp(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsRegExp();
  }

  bool IsNull(const unsigned index) const {
    if (index >= length_) return true;
    return v8__args[index]->IsNull();
  }

  bool IsUndefined(const unsigned index) const {
    if (index >= length_) return true;
    return v8__args[index]->IsUndefined();
  }

  bool IsNumber(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsNumber();
  }

  bool IsInteger(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsInt32();
  }

  bool IsUnsigned(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsUint32();
  }

  bool IsNumberOrNull(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsNumber() || v8__args[index]->IsNull();
  }

  bool IsBoolean(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsBoolean();
  }

  bool IsBooleanOrNull(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsBoolean() || v8__args[index]->IsNull();
  }

  bool IsString(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsString();
  }

  bool IsStringOrNull(const unsigned index) const {
    if (index >= length_) return false;
    return v8__args[index]->IsString() || v8__args[index]->IsNull();
  }

  bool GetBoolean(const unsigned index) {
    return v8__args[index]->BooleanValue();
  }

  int64_t GetInteger(const unsigned index) {
    return v8__args[index]->IntegerValue();
  }

  int32_t GetInt32(const unsigned index) {
    return v8__args[index]->Int32Value();
  }

  unsigned GetUInteger(const unsigned index) {
    return v8__args[index]->Uint32Value();
  }

  double GetNumber(const unsigned index) {
    return v8__args[index]->NumberValue();
  }

  int GetString(const unsigned index, JXString *jxs) {
    jxs->SetFromHandle(v8__args[index]);

    return (int)jxs->length();
  }

  int GetUTF8Length(const unsigned index) {
    return v8__args[index]->ToString()->Utf8Length();
  }

  JS_HANDLE_VALUE GetItem(const unsigned index) const {
    return v8__args[index];
  }

  JS_HANDLE_STRING GetAsString(const unsigned index) const {
    return v8__args[index].As<v8::String>();
  }

  JS_HANDLE_FUNCTION GetAsFunction(const unsigned index) const {
    return v8__args[index].As<v8::Function>();
  }

  JS_HANDLE_ARRAY GetAsArray(const unsigned index) const {
    return v8__args[index].As<v8::Array>();
  }

#undef v8__args
};
}  // namespace jxcore
#endif  // SRC_JX_PROXY_V8_PARGUMENTS_H_
