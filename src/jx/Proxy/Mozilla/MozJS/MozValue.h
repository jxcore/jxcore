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
    int32_t length_;                                                   \
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

class SuppressGC {
  JSContext *ctx_;
  bool disposed_;

 public:
  SuppressGC(JSContext *cx) : ctx_(cx) {
    JS_SetRTGC(cx, true);
    disposed_ = false;
  }

  int Dispose() {
    disposed_ = true;
    return JS_SetRTGC(ctx_, false);
  }

  ~SuppressGC() {
    if (!disposed_) {
      JS_SetRTGC(ctx_, false);
    }
  }
};

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

struct _ValueData {
  JSContext *ctx_;
  JS::Value value_;
  bool empty_;
  bool fake_rooting_;
  bool is_exception_;
};

typedef struct _ValueData ValueData;

namespace Exception {
class Error;
}

class MozRoot {
 public:
  JS::Heap<JS::Value> value_;
  JSContext *ctx_;

  MozRoot();
  ~MozRoot();
};

class Value : protected MozRoot {
  friend class StringTools;
  friend class Script;
  friend class String;

  void Natify(JS::HandleObject object_rt, const bool force_create,
              JS::MutableHandleObject out_val);

 protected:
  bool rooted_;
  bool empty_;
  bool fake_rooting_;
  bool is_exception_;

 public:

  JXCORE_PUBLIC Value();
  JXCORE_PUBLIC Value(const JS::Value &value, JSContext *ctx, bool rooted = false);
  JXCORE_PUBLIC Value(const Value &value, bool rooted);
  JXCORE_PUBLIC Value(const Value &value);
  JXCORE_PUBLIC Value(const Exception::Error &value);
  JXCORE_PUBLIC Value(JSObject *obj, JSContext *ctx, bool rooted = false);

  // creates an empty object
  JXCORE_PUBLIC Value(JSContext *ctx);

  JXCORE_PUBLIC Value(JSNative native, bool instance, JSContext *ctx);

  JXCORE_PUBLIC Value &operator=(const Value &value);
  JXCORE_PUBLIC Value &operator=(const ValueData &value);
  JXCORE_PUBLIC Value &operator=(const Exception::Error &value);

  JXCORE_PUBLIC ~Value();

  JXCORE_PUBLIC inline bool IsNativeException() { return is_exception_;  }
  JXCORE_PUBLIC inline bool IsRooted() { return rooted_; }
  JXCORE_PUBLIC inline Value *operator->() { return this; }
  JXCORE_PUBLIC inline Value *operator->() const { return const_cast<Value *>(this); }

  JXCORE_PUBLIC inline bool IsNearDeath() const { return !rooted_; }

  JXCORE_PUBLIC void AddRoot();
  JXCORE_PUBLIC void RemoveRoot();
  JXCORE_PUBLIC ValueData RootCopy();
  JXCORE_PUBLIC void MakeWeak(void *_ = NULL, JS_FINALIZER_METHOD method = NULL);
  JXCORE_PUBLIC void ClearWeak();
  JXCORE_PUBLIC void Clear();
  JXCORE_PUBLIC void Dispose();

  JXCORE_PUBLIC bool IsArray() const;
  JXCORE_PUBLIC bool IsBoolean() const;
  JXCORE_PUBLIC bool IsBooleanObject() const;
  JXCORE_PUBLIC bool IsDate() const;
  JXCORE_PUBLIC bool IsExternal() const;
  JXCORE_PUBLIC bool IsFalse() const;
  JXCORE_PUBLIC bool IsTrue() const;
  JXCORE_PUBLIC bool IsFunction() const;
  JXCORE_PUBLIC bool IsInt32() const;
  JXCORE_PUBLIC bool IsUint32() const;
  JXCORE_PUBLIC bool IsNativeError() const;
  JXCORE_PUBLIC bool IsNull() const;
  JXCORE_PUBLIC bool IsUndefined() const;
  JXCORE_PUBLIC bool IsNumber() const;
  JXCORE_PUBLIC bool IsNumberObject() const;
  JXCORE_PUBLIC bool IsObject() const;
  JXCORE_PUBLIC bool IsRegExp() const;
  JXCORE_PUBLIC bool IsString() const;
  JXCORE_PUBLIC bool IsEmpty() const;

  JXCORE_PUBLIC bool IsStringObject() const;
  JXCORE_PUBLIC bool Equals(Value *that) const;
  JXCORE_PUBLIC int32_t Int32Value();
  JXCORE_PUBLIC uint32_t Uint32Value();
  JXCORE_PUBLIC int64_t IntegerValue();
  JXCORE_PUBLIC bool BooleanValue();
  JXCORE_PUBLIC double NumberValue();

  JXCORE_PUBLIC static void empty_finalize(JSFreeOp *fop, JSObject *obj);

  JXCORE_PUBLIC void SetPrivate(void *data);
  JXCORE_PUBLIC void *GetSelfPrivate();
  JXCORE_PUBLIC void *GetPointerFromInternalField(const int index = 0);
  JXCORE_PUBLIC int InternalFieldCount() const;  // JS_HasPrivate
  JXCORE_PUBLIC void SetInternalFieldCount(int count = 0);

  JXCORE_PUBLIC static Value FromInteger(JSContext *ctx, const int64_t n);
  JXCORE_PUBLIC static Value FromBoolean(JSContext *ctx, const bool n);
  JXCORE_PUBLIC static Value FromUnsigned(JSContext *ctx, const uint32_t n);
  JXCORE_PUBLIC static Value FromDouble(JSContext *ctx, const double n);

  JXCORE_PUBLIC static Value Undefined(JSContext *ctx);
  JXCORE_PUBLIC static Value Null(JSContext *ctx);

  JXCORE_PUBLIC inline JS::Value GetRawValue() { return value_; }

  JXCORE_PUBLIC inline JSObject *GetRawObjectPointer() const {
    if (empty_) return nullptr;
    if (value_.isObject())
      return value_.toObjectOrNull();
    else
      return nullptr;
  }

  JXCORE_PUBLIC inline JSString *GetRawStringPointer() const {
    if (empty_) return nullptr;
    JS::RootedValue rv(ctx_, value_);
    return JS::ToString(ctx_, rv);
  }

  JXCORE_PUBLIC inline JSContext *GetContext() const { return ctx_; }

  JXCORE_PUBLIC void SetIndexedPropertiesToExternalArrayData(void *data, const int data_type,
                                               const int32_t length);
  JXCORE_PUBLIC void *GetIndexedPropertiesExternalArrayData();
  JXCORE_PUBLIC int32_t GetIndexedPropertiesExternalArrayDataLength();
  JXCORE_PUBLIC int GetIndexedPropertiesExternalArrayDataType();
  JXCORE_PUBLIC bool HasBufferSignature() const;

  JXCORE_PUBLIC bool HasInstance(const Value &val);
  JXCORE_PUBLIC bool StrictEquals(const Value &val);
  JXCORE_PUBLIC void ToSTDString(auto_str *out) const;

  JXCORE_PUBLIC unsigned ArrayLength() const;

  JXCORE_PUBLIC bool Has(const String &name) const;
  JXCORE_PUBLIC bool Has(const char *name) const;

  JXCORE_PUBLIC Value Get(const String &name) const;
  JXCORE_PUBLIC Value Get(const char *name) const;
  JXCORE_PUBLIC Value GetIndex(const int index);

  JXCORE_PUBLIC Value GetPropertyNames();

  JXCORE_PUBLIC bool SetProperty(const String &name, const Value &val);
  JXCORE_PUBLIC bool SetProperty(const String &name, JS::HandleValue val);
  JXCORE_PUBLIC bool SetProperty(const char *name, JS::HandleValue val);
  JXCORE_PUBLIC bool SetProperty(const String &name, JSNative method,
                   const uint16_t parameter_count = 0);
  JXCORE_PUBLIC bool SetProperty(const char *name, JSNative method,
                   const uint16_t parameter_count = 0);

  JXCORE_PUBLIC bool SetStaticFunction(const String &_name, JSNative method,
                         const uint16_t parameter_count = 0);
  JXCORE_PUBLIC bool SetStaticFunction(const char *name, JSNative method,
                         const uint16_t parameter_count = 0);
  JXCORE_PUBLIC bool SetIndex(const int index, const Value &val);
  JXCORE_PUBLIC bool SetIndex(const int index, JS::HandleValue val);

  JXCORE_PUBLIC bool DefineGetterSetter(const String &_name, JSPropertyOp getter,
                          JSStrictPropertyOp setter,
                          const Value &initial_value);

  JXCORE_PUBLIC bool DeleteProperty(const String &_name);
  JXCORE_PUBLIC bool DeleteProperty(const char *name);

  JXCORE_PUBLIC Value Call(const Value &host, int argc = 0, jsval *args = NULL) const;
  JXCORE_PUBLIC Value Call(const String &name, const int argc, Value *args) const;
  JXCORE_PUBLIC Value Call(const Value &host, const int argc, Value *args) const;
  JXCORE_PUBLIC bool Call(const char *name, int argc, JS::Value *args,
            JS::MutableHandleValue rov) const;
  JXCORE_PUBLIC Value Call(const char *name, int argc, Value *_args) const;
  JXCORE_PUBLIC Value Call(const char *name, int argc = 0, jsval *args = NULL) const;

  JXCORE_PUBLIC JSObject *NewInstance(int argc, jsval *args);
  JXCORE_PUBLIC Value NewInstance(int argc, Value *args = NULL);

  JXCORE_PUBLIC Value GetConstructor();

  JXCORE_PUBLIC static Value NewEmptyFunction(JSContext *ctx);
  JXCORE_PUBLIC static void NewEmptyObject(JSContext *ctx, JS::MutableHandleObject out);
  JXCORE_PUBLIC static JSObject *NewEmptyPropertyObject(JSContext *ctx, JSPropertyOp add_get,
                                          JSStrictPropertyOp set,
                                          JSResolveOp resolve = NULL,
                                          JSEnumerateOp enumerate = NULL,
                                          JSDeletePropertyOp del = NULL);

  JXCORE_PUBLIC static Value CompileAndRun(JSContext *ctx_, String script, String filename,
                             MozJS::Value *_global = NULL);

  JXCORE_PUBLIC void SetFinalizer(JS_FINALIZER_METHOD method);
  JXCORE_PUBLIC static void SetGlobalFinalizer(JSFinalizeOp method);

  JXCORE_PUBLIC void SetReserved(const int index, const Value &value);
  JXCORE_PUBLIC Value GetReserved(const int index);

  JXCORE_PUBLIC String ToString();
};

class String : public Value {
 public:
  JXCORE_PUBLIC String() : Value() {}
  JXCORE_PUBLIC explicit String(const Value &val);
  JXCORE_PUBLIC String(const String &val);
  JXCORE_PUBLIC String(JSString *obj, JSContext *ctx, bool rooted = false);
  JXCORE_PUBLIC String(const JS::Value &obj, JSContext *ctx, bool rooted = false);
  JXCORE_PUBLIC String(JSContext *ctx, const char *str, const int len, bool rooted = false);

  JXCORE_PUBLIC int StringLength() const;
  JXCORE_PUBLIC int Utf8Length() const;

  JXCORE_PUBLIC inline String *operator->() { return this; }

  JXCORE_PUBLIC static String FromSTD(JSContext *ctx, const char *str, const int len);
  JXCORE_PUBLIC static String FromUTF8(JSContext *ctx, const char *str, const int len);
  JXCORE_PUBLIC static String FromUTF8(JSContext *ctx, const uint16_t *str, const int len);

  JXCORE_PUBLIC String &operator=(const String &value);
  JXCORE_PUBLIC String &operator=(const ValueData &value);
};

class Script {
  JSScript *value_;
  bool rooted_;
  bool empty_;
  bool fake_rooting_;
  JSContext *ctx_;

 public:
  JXCORE_PUBLIC Script() {
    value_ = NULL;
    ctx_ = NULL;
    rooted_ = false;
    empty_ = true;
    fake_rooting_ = false;
  }

  JXCORE_PUBLIC Script(JSScript *script, JSContext *ctx)
      : value_(script),
        rooted_(false),
        empty_(false),
        fake_rooting_(false),
        ctx_(ctx) {}

  JXCORE_PUBLIC explicit Script(Script *script)
      : value_(script->value_),
        rooted_(false),
        empty_(false),
        fake_rooting_(false),
        ctx_(script->ctx_) {}

  JXCORE_PUBLIC Script &operator=(const Script &value);

  JXCORE_PUBLIC inline bool IsRooted() { return rooted_; }

  JXCORE_PUBLIC inline Script *operator->() { return this; }

  JXCORE_PUBLIC JSScript *GetRawScriptPointer() { return value_; }

  JXCORE_PUBLIC Script RootCopy() {
    Script scr = *this;
    scr.fake_rooting_ = true;
    return scr;
  }

  JXCORE_PUBLIC static Script Compile(JSContext *ctx, const String &source,
                        const String &filename);
  JXCORE_PUBLIC static Script Compile(JSContext *ctx, const Value &host, const String &source,
                        const String &filename);
  JXCORE_PUBLIC static Script Compile(JSContext *ctx, const Value &host,
                        const auto_jschar &source, const char *filename);
  JXCORE_PUBLIC static Script Compile(JSContext *ctx, const Value &host,
                        const auto_str &source, const char *filename);

  JXCORE_PUBLIC Value Run(const Value &host);

  JXCORE_PUBLIC Value Run();

  JXCORE_PUBLIC void AddRoot();

  JXCORE_PUBLIC void RemoveRoot();

  JXCORE_PUBLIC bool IsEmpty() const { return empty_; }

  JXCORE_PUBLIC void MakeWeak() { RemoveRoot(); }

  JXCORE_PUBLIC void ClearWeak() { AddRoot(); }

  JXCORE_PUBLIC void Clear() {
    if (empty_) return;
    value_ = NULL;
    empty_ = true;
  }

  JXCORE_PUBLIC void Dispose() { RemoveRoot(); }
};
}  // namespace MozJS

#endif  // SRC_JX_PROXY_MOZILLA_MOZJS_MOZVALUE_H_
