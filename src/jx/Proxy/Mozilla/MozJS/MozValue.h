// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_MOZJS_MOZVALUE_H_
#define SRC_JX_PROXY_MOZILLA_MOZJS_MOZVALUE_H_
#include "Isolate.h"
#include <assert.h>

namespace MozJS {
class Value;
class Script;
class String;

#define JX_AUTO_POINTER(name, type)                                    \
  class name {                                                         \
   public:                                                             \
    type *str_;                                                        \
    size_t length_;                                                    \
    JSContext *ctx_;                                                   \
                                                                       \
    name() {                                                           \
      str_ = NULL;                                                     \
      length_ = 0;                                                     \
      ctx_ = NULL;                                                     \
    }                                                                  \
                                                                       \
    ~name() {                                                          \
      if (length_ != 0) {                                              \
        if (!ctx_) {                                                   \
          error_console("MozValue AutoPointer: JSContext is null!\n"); \
          abort();                                                     \
        }                                                              \
        JS_free(ctx_, str_);                                           \
      }                                                                \
    }                                                                  \
                                                                       \
    type *operator*() { return str_; }                                 \
                                                                       \
    const type *operator*() const { return str_; }                     \
  }

JX_AUTO_POINTER(auto_str, char);
JX_AUTO_POINTER(auto_jschar, char16_t);

class StringTools {
 public:
  static JSString *JS_ConvertToJSString(JSContext *cx, const char *source,
                                        size_t sz_source);
  static void JS_ConvertToJSChar(JSContext *cx, const char *source,
                                 size_t sz_source, auto_jschar *out);
  static void JS_ConvertToJSChar(JSContext *cx, JSString *source,
                                 auto_jschar *out);
  static void JS_ConvertToJSChar(const String &source, auto_jschar *out);
  static JSString *FromUINT16(JSContext *ctx, const uint16_t *str,
                              const int len);
};

class Value;
typedef void (*JS_FINALIZER_METHOD)(Value &val, void *data);

class Value {
  friend class StringTools;
  friend class Script;
  friend class String;

  JSObject *Natify(JS::HandleObject obj, const bool force_create = true);

 protected:
  bool rooted_;
  bool empty_;
  bool fake_rooting_;
  JS::Value value_;

 public:
  JSContext *ctx_;
  bool is_exception_;

  Value();
  Value(const JS::Value &value, JSContext *ctx, bool rooted = false);
  Value(const Value &value, bool rooted);
  Value(const Value &value);
  Value &operator=(const Value &value);

  ~Value();

  inline Value *operator->() { return this; }
  inline Value *operator->() const { return const_cast<Value *>(this); }

  inline bool IsNearDeath() const { return !rooted_; }

  void AddRoot();
  void RemoveRoot();
  Value RootCopy();
  void MakeWeak(void *_ = NULL, JS_FINALIZER_METHOD method = NULL);
  void ClearWeak();
  void Clear();
  void Dispose();

  bool IsArray() const;
  bool IsBoolean() const;
  bool IsBooleanObject() const;
  bool IsDate() const;
  bool IsExternal() const;
  bool IsFalse() const;
  bool IsTrue() const;
  bool IsFunction() const;
  bool IsInt32() const;
  bool IsUint32() const;
  bool IsNativeError() const;
  bool IsNull() const;
  bool IsUndefined() const;
  bool IsNumber() const;
  bool IsNumberObject() const;
  bool IsObject() const;
  bool IsRegExp() const;
  bool IsString() const;
  bool IsEmpty() const;

  bool IsStringObject() const;
  bool Equals(Value *that) const;
  int32_t Int32Value();
  uint32_t Uint32Value();
  int64_t IntegerValue();
  bool BooleanValue();
  double NumberValue();

  static void empty_finalize(JSFreeOp *fop, JSObject *obj);

  void SetPrivate(void *data);
  void *GetSelfPrivate();
  void *GetPointerFromInternalField(const int index = 0);
  int InternalFieldCount() const;  // JS_HasPrivate
  void SetInternalFieldCount(int count = 0);

  static Value FromInteger(JSContext *ctx, const int32_t n);
  static Value FromBoolean(JSContext *ctx, const bool n);
  static Value FromUnsigned(JSContext *ctx, const unsigned n);
  static Value FromDouble(JSContext *ctx, const double n);

  static Value Undefined(JSContext *ctx);
  static Value Null(JSContext *ctx);

  inline JS::Value GetRawValue() { return value_; }

  inline JSObject *GetRawObjectPointer() const {
    if (value_.isObject())
      return value_.toObjectOrNull();
    else
      return nullptr;
  }

  inline JSString *GetRawStringPointer() const {
    JS::RootedValue rt_value_(ctx_, value_);

    return JS::ToString(ctx_, rt_value_);
  }

  void SetIndexedPropertiesToExternalArrayData(void *data, const int data_type,
                                               const int32_t length);
  void *GetIndexedPropertiesExternalArrayData();
  int32_t GetIndexedPropertiesExternalArrayDataLength();
  int GetIndexedPropertiesExternalArrayDataType();
  bool HasBufferSignature() const;

  bool HasInstance(const Value &val);
  bool StrictEquals(const Value &val);
  void ToSTDString(auto_str *out) const;

  // Object
  Value(JSObject *obj, JSContext *ctx, bool rooted = false);
  Value(JSNative native, bool instance, JSContext *ctx);

  unsigned ArrayLength() const;

  bool Has(const String &name) const;
  bool Has(const char *name) const;

  Value Get(const String &name) const;
  Value Get(const char *name) const;
  Value GetIndex(const int index);

  Value GetPropertyNames();

  bool SetProperty(const String &name, const Value &val);
  bool SetProperty(const String &name, JS::HandleValue val);
  bool SetProperty(const char *name, JS::HandleValue val);
  bool SetProperty(const String &name, JSNative method,
                   const uint16_t parameter_count = 0);
  bool SetProperty(const char *name, JSNative method,
                   const uint16_t parameter_count = 0);

  bool SetStaticFunction(const String &_name, JSNative method,
                         const uint16_t parameter_count = 0);
  bool SetStaticFunction(const char *name, JSNative method,
                         const uint16_t parameter_count = 0);
  bool SetIndex(const int index, const Value &val);
  bool SetIndex(const int index, JS::HandleValue val);

  bool DefineGetterSetter(const String &_name, JSPropertyOp getter,
                          JSStrictPropertyOp setter,
                          const Value &initial_value);

  bool DeleteProperty(const String &_name);
  bool DeleteProperty(const char *name);

  Value Call(const Value &host, int argc = 0, jsval *args = NULL) const;
  Value Call(const String &name, const int argc, Value *args) const;
  Value Call(const Value &host, const int argc, Value *args) const;
  bool Call(const char *name, int argc, JS::Value *args,
            JS::MutableHandleValue rov) const;
  Value Call(const char *name, int argc, Value *_args) const;
  Value Call(const char *name, int argc = 0, jsval *args = NULL) const;

  JSObject *NewInstance(int argc, jsval *args);
  Value NewInstance(int argc, Value *args = NULL);

  Value GetConstructor();

  static Value NewEmptyFunction(JSContext *ctx);
  static JSObject *NewEmptyObject(JSContext *ctx);
  static JSObject *NewEmptyPropertyObject(JSContext *ctx, JSPropertyOp add_get,
                                          JSStrictPropertyOp set,
                                          JSResolveOp resolve = NULL,
                                          JSEnumerateOp enumerate = NULL,
                                          JSDeletePropertyOp del = NULL);

  static Value CompileAndRun(JSContext *ctx_, String script, String filename,
                             MozJS::Value *_global = NULL);

  void SetFinalizer(JS_FINALIZER_METHOD method);
  static void SetGlobalFinalizer(JSFinalizeOp method);

  void SetReserved(const int index, const Value &value);
  Value GetReserved(const int index);

  String ToString();
};

class String : public Value {
 public:
  String() : Value() {}
  explicit String(const Value &val);
  String(const String &val);
  String(JSString *obj, JSContext *ctx, bool rooted = false);
  String(const JS::Value &obj, JSContext *ctx, bool rooted = false);
  String(JSContext *ctx, const char *str, const int len, bool rooted = false);

  int StringLength() const;
  int Utf8Length() const;

  inline String *operator->() { return this; }

  static String FromSTD(JSContext *ctx, const char *str, const int len);
  static String FromUTF8(JSContext *ctx, const char *str, const int len);
  static String FromSTD(JSContext *ctx, const uint16_t *str, const int len);

  String &operator=(const String &value);

  String RootCopy() {
	String str = *this;
    str.fake_rooting_ = true;
    return str;
  }
};

class Script {
  JSScript *value_;
  bool rooted_;
  bool empty_;
  bool fake_rooting_;

 public:
  JSContext *ctx_;

  Script() {
    value_ = NULL;
    ctx_ = NULL;
    rooted_ = false;
    empty_ = true;
    fake_rooting_ = false;
  }

  Script(JSScript *script, JSContext *ctx)
      : value_(script),
        rooted_(false),
        empty_(false),
        fake_rooting_(false),
        ctx_(ctx) {}

  explicit Script(Script *script)
      : value_(script->value_),
        rooted_(false),
        empty_(false),
        fake_rooting_(false),
        ctx_(script->ctx_) {}

  Script &operator=(const Script &value);

  inline Script *operator->() { return this; }

  JSScript *GetRawScriptPointer() { return value_; }

  Script RootCopy() {
	Script scr = *this;
    scr.fake_rooting_ = true;
    return scr;
  }

  static Script Compile(JSContext *ctx, const String &source,
                        const String &filename);
  static Script Compile(JSContext *ctx, const Value &host, const String &source,
                        const String &filename, bool for_asm_js = false);
  static Script Compile(JSContext *ctx, const Value &host,
                        const auto_jschar &source, const char *filename,
                        bool for_asm_js = false);
  static Script Compile(JSContext *ctx, const Value &host,
                        const auto_str &source, const char *filename);

  Value Run(const Value &host);

  Value Run();

  void AddRoot();

  void RemoveRoot();

  bool IsEmpty() const { return empty_; }

  void MakeWeak() { RemoveRoot(); }

  void ClearWeak() { AddRoot(); }

  void Clear() {
    if (empty_) return;
    value_ = NULL;
    empty_ = true;
  }

  void Dispose() { RemoveRoot(); }
};
}  // namespace MozJS

#endif  // SRC_JX_PROXY_MOZILLA_MOZJS_MOZVALUE_H_
