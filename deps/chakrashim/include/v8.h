// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

// CHAKRA-TODO: winsock2.h should be included before windows.h, otherwise you
// get redefintion conflicts with winsock.h. Hack this here to avoid those
// conflicts, but should look at how to fix in core Node.js code.
#define _WINSOCKAPI_

// CHAKRA-TODO: Force the version here?
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0601   // CHAKRA-TODO: should this be win10?

#ifndef USE_EDGEMODE_JSRT
#define USE_EDGEMODE_JSRT     // Only works wtih edge JSRT
#endif

#include <jsrt.h>

#ifndef _CHAKRART_H_
// CHAKRA-TODO: Enable this check
// #error Wrong Windows SDK version
#endif

#include <stdio.h>
#include <stdint.h>
#include <memory>

// CHAKRA-TODO: This allows native modules to link against node. We should
// investigate adding an option to compile this into a standalone DLL/LIB like
// V8 does.
#define EXPORT __declspec(dllexport)


# define V8_DEPRECATED(message, declarator) declarator

namespace v8 {

class AccessorSignature;
class Array;
class Value;
class External;
class Primitive;
class Boolean;
class BooleanObject;
class Context;
class CpuProfiler;
class Function;
class FunctionTemplate;
class HeapProfiler;
class Int32;
class Integer;
class Isolate;
class Number;
class NumberObject;
class Object;
class ObjectTemplate;
class ResourceConstraints;
class RegExp;
class Script;
class Signature;
class String;
class StringObject;
class Uint32;
template <class T> class Handle;
template <class T> class Local;
template <class T> class Persistent;
template<typename T> class FunctionCallbackInfo;
template<typename T> class PropertyCallbackInfo;

class JitCodeEvent;
class RetainedObjectInfo;
struct ExternalArrayData;

enum PropertyAttribute {
  None = 0,
  ReadOnly = 1 << 0,
  DontEnum = 1 << 1,
  DontDelete = 1 << 2,
};

enum ExternalArrayType {
  kExternalInt8Array = 1,
  kExternalUint8Array,
  kExternalInt16Array,
  kExternalUint16Array,
  kExternalInt32Array,
  kExternalUint32Array,
  kExternalFloat32Array,
  kExternalFloat64Array,
  kExternalUint8ClampedArray,

  // Legacy constant names
  kExternalByteArray = kExternalInt8Array,
  kExternalUnsignedByteArray = kExternalUint8Array,
  kExternalShortArray = kExternalInt16Array,
  kExternalUnsignedShortArray = kExternalUint16Array,
  kExternalIntArray = kExternalInt32Array,
  kExternalUnsignedIntArray = kExternalUint32Array,
  kExternalFloatArray = kExternalFloat32Array,
  kExternalDoubleArray = kExternalFloat64Array,
  kExternalPixelArray = kExternalUint8ClampedArray
};

enum AccessControl {
  DEFAULT = 0,
  ALL_CAN_READ = 1,
  ALL_CAN_WRITE = 1 << 1,
  PROHIBITS_OVERWRITING = 1 << 2,
};

enum JitCodeEventOptions {
  kJitCodeEventDefault = 0,
  kJitCodeEventEnumExisting = 1,
};

typedef void (*AccessorGetterCallback)(
  Local<String> property,
  const PropertyCallbackInfo<Value>& info);
typedef void (*AccessorSetterCallback)(
  Local<String> property,
  Local<Value> value,
  const PropertyCallbackInfo<void>& info);
typedef void (*NamedPropertyGetterCallback)(
  Local<String> property, const PropertyCallbackInfo<Value>& info);
typedef void (*NamedPropertySetterCallback)(
  Local<String> property,
  Local<Value> value,
  const PropertyCallbackInfo<Value>& info);
typedef void (*NamedPropertyQueryCallback)(
  Local<String> property, const PropertyCallbackInfo<Integer>& info);
typedef void (*NamedPropertyDeleterCallback)(
  Local<String> property, const PropertyCallbackInfo<Boolean>& info);
typedef void (*NamedPropertyEnumeratorCallback)(
  const PropertyCallbackInfo<Array>& info);
typedef void (*IndexedPropertyGetterCallback)(
  uint32_t index, const PropertyCallbackInfo<Value>& info);
typedef void (*IndexedPropertySetterCallback)(
  uint32_t index, Local<Value> value, const PropertyCallbackInfo<Value>& info);
typedef void (*IndexedPropertyQueryCallback)(
  uint32_t index, const PropertyCallbackInfo<Integer>& info);
typedef void (*IndexedPropertyDeleterCallback)(
  uint32_t index, const PropertyCallbackInfo<Boolean>& info);
typedef void (*IndexedPropertyEnumeratorCallback)(
  const PropertyCallbackInfo<Array>& info);
typedef bool (*EntropySource)(unsigned char* buffer, size_t length);
typedef void (*FatalErrorCallback)(const char *location, const char *message);
typedef void (*JitCodeEventHandler)(const JitCodeEvent *event);

EXPORT Handle<Primitive> Undefined(Isolate* isolate = nullptr);
EXPORT Handle<Primitive> Null(Isolate* isolate = nullptr);
EXPORT Handle<Boolean> True(Isolate* isolate = nullptr);
EXPORT Handle<Boolean> False(Isolate* isolate = nullptr);
EXPORT bool SetResourceConstraints(ResourceConstraints *constraints);

template <class T>
class EXPORT Handle {
 protected:
  JsRef _ref;

 public:
  Handle();
  Handle(T *val);
  template <class S>
  Handle(Handle<S> that);
  bool IsEmpty() const;
  void Clear();
  T *operator->() const;
  T *operator*() const;
  template <class S>
  bool operator==(Handle<S> that) const;
  template <class S>
  bool operator!=(Handle<S> that) const;

  template <class S>
  Handle<S> As();
  template <class S>
  static Handle<T> Cast(Handle<S> that);
};

template <class T>
class EXPORT Local : public Handle<T> {
 public:
  Local();
  template <class S>
  Local(S *that);
  template <class S>
  Local(Local<S> that);
  template <class S>
  Local(Handle<S> that);
  template <class S>
  Local<S> As();
  template <class S>
  static Local<T> Cast(Local<S> that);
  static Local<T> New(Handle<T> that);
  static Local<T> New(Isolate* isolate, Handle<T> that);
  static Local<T> New(Isolate* isolate, const Persistent<T>& that);
};


template<class T, class P>
class WeakCallbackData {
 public:
  typedef void (*Callback)(const WeakCallbackData<T, P>& data);

  Isolate* GetIsolate() const { return isolate_; }
  Local<T> GetValue() const { return handle_; }
  P* GetParameter() const { return parameter_; }

  WeakCallbackData(Isolate* isolate, Local<T> handle, P* parameter)
    : isolate_(isolate), handle_(handle), parameter_(parameter) {
  }
 private:
  Isolate* isolate_;
  Local<T> handle_;
  P* parameter_;
};

namespace chakrashim {
class InternalMethods;
struct WeakReferenceCallbackWrapper;
template class EXPORT std::shared_ptr<WeakReferenceCallbackWrapper>;

// A helper method for setting an object with a WeakReferenceCallback. The
// callback will be called before the object is released.
EXPORT void SetObjectWeakReferenceCallback(
    JsValueRef object,
    WeakCallbackData<Value, void>::Callback callback,
    void* parameters,
    std::shared_ptr<WeakReferenceCallbackWrapper>* weakWrapper);
// A helper method for turning off the WeakReferenceCallback that was set using
// the previous method
EXPORT void ClearObjectWeakReferenceCallback(JsValueRef object, bool revive);

EXPORT JsValueRef MarshalJsValueRefToContext(
  JsValueRef value, JsContextRef context);
}

template <class T>
class EXPORT Persistent : public Handle<T> {
 private:
  std::shared_ptr<chakrashim::WeakReferenceCallbackWrapper> _weakWrapper;

  void SetNewRef(JsValueRef ref);

 public:
  Persistent();
  ~Persistent();
  template <class S>
  explicit Persistent(Persistent<S> that);
  template <class S>
  explicit Persistent(Isolate* isolate, Persistent<S> that);

  template <class S>
  explicit Persistent(S *that);

  template <class S>
  explicit Persistent(Handle<S> that);
  template <class S>
  explicit Persistent(Isolate* isolate, Handle<S> that);

  Persistent(const Persistent<T> &that);
  explicit Persistent(const Handle<T> &that);

  template <class S>
  Persistent<S> As();
  void Dispose();

  Local<T> get() {
    return (*this);
  }

  template<typename P>
  void SetWeak(
    P* parameter,
    typename WeakCallbackData<T, P>::Callback callback);

  void ClearWeak();
  void MarkIndependent();
  bool IsNearDeath() const;
  bool IsWeak() const;
  void SetWrapperClassId(uint16_t class_id);

  void Reset();
  template <class S>
  void Reset(Isolate* isolate, const Handle<S>& other);
  template <class S>
  void Reset(Isolate* isolate, const Persistent<S>& other);

  // CHAKRA: Addition from v8 API
  Persistent<T>& operator=(const Persistent<T> &rhs);

  // CHAKRA: Addition from v8 API
  Persistent<T>& operator=(const Handle<T> &rhs);

  // CHAKRA: Addition from v8 API
  template <class S>
  Persistent<T>& operator=(const Persistent<S> &rhs);

  // CHAKRA: Addition from v8 API
  template <class S>
  Persistent<T>& operator=(const Handle<S> &rhs);

  // CHAKRA: Addition from v8 API
  template <class S>
  Persistent<T>& operator=(S* that);

  template <class S>
  static Persistent<T> Cast(Persistent<S> that);
  static Persistent<T> New(Handle<T> that);
};

template <class T>
class EXPORT Eternal : private Persistent<T> {
 public:
  Eternal() {}
  template<class S>
  Eternal(Isolate* isolate, Local<S> handle) {
    Set(isolate, handle);
  }

  Local<T> Get(Isolate* isolate) {
    return Local<T>::New(*this);
  }
  bool IsEmpty() { return __super::IsEmpty(); }
  template<class S> void Set(Isolate* isolate, Local<S> handle) {
    Reset(isolate, handle);
  }
};

// CHAKRA: Chakra's GC behavior does not exactly match up with V8's GC behavior.
// V8 uses a HandleScope to keep Local references alive, which means that as
// long as the HandleScope is on the stack, the Local references will not be
// collected. Chakra, on the other hand, directly walks the stack and has no
// HandleScope mechanism. It requires hosts to keep "local" references on the
// stack or else turn them into "persistent" references through
// JsAddRef/JsRelease. To paper over this difference, the bridge HandleScope
// will create a JS array and will hold that reference on the stack. Any local
// values created will then be added to that array. So the GC will see the array
// on the stack and then keep those local references alive.
class EXPORT HandleScope {
  template <class T>
  friend class Local;

 private:
  JsValueRef _refs;
  int _count;
  HandleScope *_prev;
  JsContextRef _contextRef;
  struct AddRefRecord {
    JsRef _ref;
    AddRefRecord *  _next;
  } *_addRefRecordHead;

  bool AddLocal(JsValueRef value);
  bool AddLocalContext(JsContextRef value);
  bool AddLocalAddRef(JsRef value);

  static HandleScope *GetCurrent();

 public:
  HandleScope(Isolate* isolate = nullptr);
  ~HandleScope();

  template <class T>
  Local<T> Close(Handle<T> value);
};

class EXPORT EscapableHandleScope : public HandleScope {
 public:
  EscapableHandleScope(Isolate* isolate) : HandleScope(isolate) {}

  template <class T>
  Local<T> Escape(Handle<T> value) { return Close(value); }
};

class EXPORT Data {
 public:
};

class ScriptOrigin {
 public:
  explicit ScriptOrigin(Handle<Value> resource_name)
      : resource_name_(resource_name) {}
  Handle<Value> ResourceName() const { return resource_name_; }
 private:
  Handle<Value> resource_name_;
};

class EXPORT UnboundScript {
 public:
  Local<Script> BindToCurrentContext();
};

class EXPORT Script {
 public:
  static Local<Script> Compile(
    Handle<String> source, ScriptOrigin* origin = NULL);
  static Local<Script> Compile(Handle<String> source, Handle<String> file_name);
  Local<Value> Run();
  Local<UnboundScript> GetUnboundScript();
};

class EXPORT ScriptCompiler {
 public:
  struct CachedData {
    // CHAKRA-TODO: Not implemented
   private:
    CachedData();  // Make sure it is not constructed as it is not implemented.
  };

  class Source {
   public:
    Source(
      Local<String> source_string,
      const ScriptOrigin& origin,
      CachedData * cached_data = NULL)
      : source_string(source_string), resource_name(origin.ResourceName()) {
    }

    Source(Local<String> source_string, CachedData * cached_data = NULL)
      : source_string(source_string) {
    }

   private:
    friend ScriptCompiler;
    Local<String> source_string;
    Handle<Value> resource_name;
  };

  enum CompileOptions {
    kNoCompileOptions = 0,
  };

  static Local<UnboundScript> CompileUnbound(
    Isolate* isolate, Source* source,
    CompileOptions options = kNoCompileOptions);

  static Local<Script> Compile(
    Isolate* isolate, Source* source,
    CompileOptions options = kNoCompileOptions);
};

class EXPORT Message {
 public:
  Local<String> GetSourceLine() const;
  Handle<Value> GetScriptResourceName() const;
  int GetLineNumber() const;
  int GetStartColumn() const;
  int GetEndColumn() const;
};

typedef void (*MessageCallback)(Handle<Message> message, Handle<Value> error);

class EXPORT Value : public Data {
 public:
  bool IsUndefined() const;
  bool IsNull() const;
  bool IsTrue() const;
  bool IsFalse() const;
  bool IsString() const;
  bool IsFunction() const;
  bool IsArray() const;
  bool IsObject() const;
  bool IsBoolean() const;
  bool IsNumber() const;
  bool IsInt32() const;
  bool IsUint32() const;
  bool IsDate() const;
  bool IsBooleanObject() const;
  bool IsNumberObject() const;
  bool IsStringObject() const;
  bool IsNativeError() const;
  bool IsRegExp() const;
  bool IsExternal() const;
  bool IsTypedArray() const;
  Local<Boolean> ToBoolean() const;
  Local<Number> ToNumber() const;
  Local<String> ToString() const;
  Local<Object> ToObject() const;
  Local<Integer> ToInteger() const;
  Local<Uint32> ToUint32() const;
  Local<Int32> ToInt32() const;
  bool BooleanValue() const;
  double NumberValue() const;
  int64_t IntegerValue() const;
  uint32_t Uint32Value() const;
  int32_t Int32Value() const;
  bool Equals(Handle<Value> that) const;
  bool StrictEquals(Handle<Value> that) const;

  template <class T> static Value* Cast(T* value) {
    return static_cast<Value*>(value);
  }
};

class EXPORT Primitive : public Value {
 public:
};

class EXPORT Boolean : public Primitive {
 public:
  bool Value() const;

  static Handle<Boolean> New(Isolate* isolate, bool value);
};

class EXPORT String : public Primitive {
 public:
  class EXPORT AsciiValue {
   public:
    explicit AsciiValue(Handle<v8::Value> obj);
    ~AsciiValue();
    char *operator*() { return _str; }
    const char *operator*() const { return _str; }
    int length() const { return static_cast<int>(_length); }
   private:
    AsciiValue(const AsciiValue&);
    void operator=(const AsciiValue&);

    char* _str;
    size_t _length;
  };

  class EXPORT ExternalAsciiStringResource {
   public:
    virtual ~ExternalAsciiStringResource() {}
    virtual const char *data() const = 0;
    virtual size_t length() const = 0;
  };

  class EXPORT ExternalStringResource {
   public:
    virtual ~ExternalStringResource() {}
    virtual const uint16_t* data() const = 0;
    virtual size_t length() const = 0;
  };

  class EXPORT Utf8Value {
   public:
    explicit Utf8Value(Handle<v8::Value> obj);
    ~Utf8Value();
    char *operator*() { return _str; }
    const char *operator*() const { return _str; }
    int length() const { return static_cast<int>(_length); }
   private:
    Utf8Value(const Utf8Value&);
    void operator=(const Utf8Value&);

    char* _str;
    size_t _length;
  };

  class EXPORT Value {
   public:
    explicit Value(Handle<v8::Value> obj);
    ~Value();
    uint16_t *operator*() { return _str; }
    const uint16_t *operator*() const { return _str; }
    int length() const { return _length; }
   private:
    Value(const Value&);
    void operator=(const Value&);

    uint16_t* _str;
    size_t _length;
  };

  enum WriteOptions {
    NO_OPTIONS = 0,
    HINT_MANY_WRITES_EXPECTED = 1,
    NO_NULL_TERMINATION = 2,
    PRESERVE_ONE_BYTE_NULL = 4,
    // Used by WriteUtf8 to replace orphan surrogate code units with the
    // unicode replacement character. Needs to be set to guarantee valid UTF-8
    // output.
    REPLACE_INVALID_UTF8 = 8
  };

  int Length() const;
  int Utf8Length() const;
  bool inline MayContainNonAscii() const { return true; }
  int Write(
    uint16_t *buffer,
    int start = 0,
    int length = -1,
    int options = NO_OPTIONS) const;
  int WriteAscii(
    char *buffer,
    int start = 0,
    int length = -1,
    int options = NO_OPTIONS) const;
  int WriteOneByte(
    uint8_t* buffer,
    int start = 0,
    int length = -1,
    int options = NO_OPTIONS) const;
  int WriteUtf8(
    char *buffer,
    int length = -1,
    int *nchars_ref = NULL,
    int options = NO_OPTIONS) const;

  static Local<String> Empty(Isolate* isolate = nullptr);
  static String *Cast(v8::Value *obj);
  template <class ToWide> static Local<String> New(
    const ToWide& toWide, const char *data, int length = -1);
  static Local<String> New(const wchar_t *data, int length = -1);
  static Local<String> New(const uint16_t *data, int length = -1);
  static Local<String> NewSymbol(const char *data, int length = -1);
  static Local<String> NewSymbol(const wchar_t *data, int length = -1);
  static Local<String> Concat(Handle<String> left, Handle<String> right);
  static Local<String> NewExternal(
    Isolate* isolate, ExternalStringResource* resource);
  static Local<String> NewExternal(
    Isolate* isolate, ExternalAsciiStringResource *resource);

  bool IsExternal() const { return false; }
  bool IsExternalAscii() const { return false; }
  ExternalStringResource* GetExternalStringResource() const { return NULL; }
  const ExternalAsciiStringResource* GetExternalAsciiStringResource() const {
    return NULL;
  }

  enum NewStringType {
    kNormalString, kInternalizedString, kUndetectableString
  };

  static Local<String> NewFromUtf8(Isolate* isolate,
                                   const char* data,
                                   NewStringType type = kNormalString,
                                   int length = -1);

  static Local<String> NewFromOneByte(Isolate* isolate,
                                      const uint8_t* data,
                                      NewStringType type = kNormalString,
                                      int length = -1);

  static Local<String> NewFromTwoByte(Isolate* isolate,
                                      const uint16_t* data,
                                      NewStringType type = kNormalString,
                                      int length = -1);

  JsValueRef _ref;
};

class EXPORT Number : public Primitive {
 public:
  double Value() const;

  static Local<Number> New(Isolate* isolate, double value);
  static Number *Cast(v8::Value *obj);
};

class EXPORT Integer : public Number {
 public:
  static Local<Integer> New(Isolate* isolate, int32_t value);
  static Local<Integer> NewFromUnsigned(Isolate* isolate, uint32_t value);
  static Integer *Cast(v8::Value *obj);

  int64_t Value() const;
};

class EXPORT Int32 : public Integer {
 public:
  int32_t Value() const;
};

class EXPORT Uint32 : public Integer {
 public:
  uint32_t Value() const;
};

class EXPORT Object : public Value {
 public:
  bool Set(
    Handle<Value> key, Handle<Value> value, PropertyAttribute attribs = None);
  bool Set(uint32_t index, Handle<Value> value);
  bool ForceSet(
    Handle<Value> key, Handle<Value> value, PropertyAttribute attribs = None);
  Local<Value> Get(Handle<Value> key);
  Local<Value> Get(uint32_t index);
  bool Has(Handle<String> key);
  bool Delete(Handle<String> key);
  bool Delete(uint32_t index);
  bool SetAccessor(
    Handle<String> name,
    AccessorGetterCallback getter,
    AccessorSetterCallback setter = 0,
    Handle<Value> data = Handle<Value>(),
    AccessControl settings = DEFAULT,
    PropertyAttribute attribute = None);
  Local<Value> GetPrototype();
  bool SetPrototype(Handle<Value> prototype);
  Local<Value> GetConstructor();
  Local<Array> GetPropertyNames();
  Local<Array> GetOwnPropertyNames();
  bool HasOwnProperty(Handle<String> key);
  Local<String> GetConstructorName();
  int InternalFieldCount();
  bool SetHiddenValue(Handle<String> key, Handle<Value> value);
  Local<Value> GetHiddenValue(Handle<String> key);
  void SetIndexedPropertiesToExternalArrayData(
    void *data, ExternalArrayType array_type, int number_of_elements);
  bool HasIndexedPropertiesInExternalArrayData();
  void *GetIndexedPropertiesExternalArrayData();
  ExternalArrayType GetIndexedPropertiesExternalArrayDataType();
  int GetIndexedPropertiesExternalArrayDataLength();

  void* GetAlignedPointerFromInternalField(int index);
  void SetAlignedPointerInInternalField(int index, void* value);
  Local<Object> Clone();
  Local<Context> CreationContext();
  Local<Value> GetRealNamedProperty(Handle<String> key);

  static Local<Object> New(Isolate* isolate = nullptr);
  static Object *Cast(Value *obj);

 private:
  bool Set(Handle<Value> key,
           Handle<Value> value,
           PropertyAttribute attribs,
           bool force);
  JsErrorCode GetObjectData(struct ObjectData** objectData);
  JsErrorCode InternalFieldHelper(void ***externalArray, int *count);
  JsErrorCode ExternalArrayDataHelper(ExternalArrayData **data);
  bool SupportsExternalArrayData(ExternalArrayData **data);

  friend ObjectTemplate;
  bool SetAccessor(Handle<String> name,
                   AccessorGetterCallback getter,
                   AccessorSetterCallback setter,
                   Handle<Value> data,
                   AccessControl settings,
                   PropertyAttribute attribute,
                   Handle<AccessorSignature> signature);

  friend chakrashim::InternalMethods;
  ObjectTemplate* GetObjectTemplate();
};

class EXPORT Array : public Object {
 public:
  uint32_t Length() const;

  static Local<Array> New(Isolate* isolate = nullptr, int length = 0);
  static Array *Cast(Value *obj);
};


class EXPORT ArrayBuffer : public Object {
 public:
  class EXPORT Allocator {
   public:
    virtual ~Allocator() {}
    virtual void* Allocate(size_t length) = 0;
    virtual void* AllocateUninitialized(size_t length) = 0;
    virtual void Free(void* data, size_t length) = 0;
  };
};

class EXPORT BooleanObject : public Object {
 public:
  static Local<Value> New(bool value);
  bool ValueOf() const;
  static BooleanObject* Cast(Value* obj);
};


class EXPORT StringObject : public Object {
 public:
  static Local<Value> New(Handle<String> value);
  Local<String> ValueOf() const;
  static StringObject* Cast(Value* obj);
};

class EXPORT NumberObject : public Object {
 public:
  static Local<Value> New(Isolate * isolate, double value);
  double ValueOf() const;
  static NumberObject* Cast(Value* obj);
};

class EXPORT Date : public Object {
 public:
  static Local<Value> New(Isolate * isolate, double time);
  static Date *Cast(Value *obj);
};

class EXPORT RegExp : public Object {
 public:
  enum Flags {
    kNone = 0,
    kGlobal = 1,
    kIgnoreCase = 2,
    kMultiline = 4
  };

  static Local<RegExp> New(Handle<String> pattern, Flags flags);
  Local<String> GetSource() const;
  static RegExp *Cast(v8::Value *obj);
};

template<typename T>
class ReturnValue {
 public:
  // Handle setters
  template <typename S> void Set(const Persistent<S>& handle) {
    *_value = static_cast<Value *>(
      chakrashim::MarshalJsValueRefToContext(*handle, *_context));
  }
  template <typename S> void Set(const Handle<S> handle) {
    *_value = static_cast<Value *>(
      chakrashim::MarshalJsValueRefToContext(*handle, *_context));
  }
  // Fast primitive setters
  void Set(bool value) { Set(Boolean::New(Isolate::GetCurrent(), value)); }
  void Set(double i) { Set(Number::New(Isolate::GetCurrent(), i)); }
  void Set(int32_t i) { Set(Integer::New(Isolate::GetCurrent(), i)); }
  void Set(uint32_t i) {
    Set(Integer::NewFromUnsigned(Isolate::GetCurrent(), i));
  }
  // Fast JS primitive setters
  void SetNull() { Set(Null(Isolate::GetCurrent())); }
  void SetUndefined() { Set(Undefined(Isolate::GetCurrent())); }
  void SetEmptyString() { Set(String::New(L"", 0)); }
  // Convenience getter for Isolate
  Isolate* GetIsolate() { return Isolate::GetCurrent(); }

  Value* Get() const { return *_value; }
 private:
  ReturnValue(Value** value, Handle<Context> context)
    : _value(value), _context(context) {
  }

  Value** _value;
  Local<Context> _context;

  void SetCrossContext(JsValueRef valueRef);

  template <typename F> friend class FunctionCallbackInfo;
  template <typename F> friend class PropertyCallbackInfo;
};

template<typename T>
class FunctionCallbackInfo {
 public:
  int Length() const { return _length; }
  Local<Value> operator[](int i) const {
    return (i >= 0 && i < _length) ?
      Local<Value>(_args[i]) : Local<Value>(static_cast<Value*>(*Undefined()));
  }
  Local<Object> This() const { return _thisPointer; }
  Local<Object> Holder() const { return _holder; }
  Local<Function> Callee() const { return _callee; }
  bool IsConstructCall() const { return _isConstructorCall; }
  //  V8_INLINE Local<Value> Data() const;
  Isolate* GetIsolate() const { return Isolate::GetCurrent(); }
  ReturnValue<T> GetReturnValue() const {
    return ReturnValue<T>(
      &(const_cast<FunctionCallbackInfo<T>*>(this)->_returnValue), _context);
  }

  FunctionCallbackInfo(
    Value** args,
    int length,
    Local<Object> _this,
    Local<Object> holder,
    bool isConstructorCall,
    Local<Function> callee)
       : _args(args),
         _length(length),
         _thisPointer(_this),
         _holder(holder),
         _isConstructorCall(isConstructorCall),
         _callee(callee),
         _returnValue(static_cast<Value*>(JS_INVALID_REFERENCE)),
    _context(Context::GetCurrent()) {
  }

 private:
  int _length;
  Local<Object> _thisPointer;
  Local<Object> _holder;
  Local<Function> _callee;
  bool _isConstructorCall;
  Value** _args;
  Value* _returnValue;
  Local<Context> _context;
};


template<typename T>
class PropertyCallbackInfo {
 public:
  Isolate* GetIsolate() const { return Isolate::GetCurrent(); }
  Local<Value> Data() const { return _data; }
  Local<Object> This() const { return _thisObject; }
  Local<Object> Holder() const { return _holder; }
  ReturnValue<T> GetReturnValue() const {
    return ReturnValue<T>(
      &(const_cast<PropertyCallbackInfo<T>*>(this)->_returnValue), _context);
  }

  PropertyCallbackInfo(
    Local<Value> data, Local<Object> thisObject, Local<Object> holder)
       : _data(data),
         _thisObject(thisObject),
         _holder(holder),
         _returnValue(static_cast<Value*>(JS_INVALID_REFERENCE)),
         _context(Context::GetCurrent()) {
  }
 private:
  Local<Value> _data;
  Local<Object> _thisObject;
  Local<Object> _holder;
  Value* _returnValue;
  Local<Context> _context;
};

typedef void (*FunctionCallback)(const FunctionCallbackInfo<Value>& info);

class EXPORT Function : public Object {
 public:
  static Local<Function> New(
    Isolate * isolate,
    FunctionCallback callback,
    Local<Value> data = Local<Value>(),
    int length = 0);
  Local<Object> NewInstance() const;
  Local<Object> NewInstance(int argc, Handle<Value> argv[]) const;
  Local<Value> Call(Handle<Object> recv, int argc, Handle<Value> argv[]);
  void SetName(Handle<String> name);
  // Handle<Value> GetName() const;

  static Function *Cast(Value *obj);
};

enum AccessType {
  ACCESS_GET,
  ACCESS_SET,
  ACCESS_HAS,
  ACCESS_DELETE,
  ACCESS_KEYS
};

typedef bool (*NamedSecurityCallback)(
  Local<Object> host, Local<Value> key, AccessType type, Local<Value> data);
typedef bool (*IndexedSecurityCallback)(
  Local<Object> host, uint32_t index, AccessType type, Local<Value> data);

class EXPORT Template : public Data {
 public:
  void Set(Handle<String> name,
           Handle<Data> value,
           PropertyAttribute attributes = None);
  void Set(Isolate* isolate, const char* name, Handle<Data> value) {
    Set(v8::String::NewFromUtf8(isolate, name), value);
  }
 private:
  Template();
};

class EXPORT FunctionTemplate : public Template {
 public:
  static Local<FunctionTemplate> New(
    Isolate* isolate,
    FunctionCallback callback = 0,
    Handle<Value> data = Handle<Value>(),
    Handle<Signature> signature = Handle<Signature>(),
    int length = 0);

  Local<Function> GetFunction();
  Local<ObjectTemplate> InstanceTemplate();
  Local<ObjectTemplate> PrototypeTemplate();
  void SetClassName(Handle<String> name);
  void SetHiddenPrototype(bool value);
  bool HasInstance(Handle<Value> object);
};

class EXPORT ObjectTemplate : public Template {
 public:
  static Local<ObjectTemplate> New(Isolate* isolate);

  Local<Object> NewInstance();
  Local<Object> NewInstance(Handle<Object> prototype);
  void SetClassName(Handle<String> name);
  void SetSupportsOverrideToString();

  void SetAccessor(
    Handle<String> name,
    AccessorGetterCallback getter,
    AccessorSetterCallback setter = 0,
    Handle<Value> data = Handle<Value>(),
    AccessControl settings = DEFAULT,
    PropertyAttribute attribute = None,
    Handle<AccessorSignature> signature =
    Handle<AccessorSignature>());

  void SetNamedPropertyHandler(
    NamedPropertyGetterCallback getter,
    NamedPropertySetterCallback setter = 0,
    NamedPropertyQueryCallback query = 0,
    NamedPropertyDeleterCallback deleter = 0,
    NamedPropertyEnumeratorCallback enumerator = 0,
    Handle<Value> data = Handle<Value>());

  void SetIndexedPropertyHandler(
    IndexedPropertyGetterCallback getter,
    IndexedPropertySetterCallback setter = 0,
    IndexedPropertyQueryCallback query = 0,
    IndexedPropertyDeleterCallback deleter = 0,
    IndexedPropertyEnumeratorCallback enumerator = 0,
    Handle<Value> data = Handle<Value>());

  void SetAccessCheckCallbacks(
    NamedSecurityCallback named_handler,
    IndexedSecurityCallback indexed_handler,
    Handle<Value> data = Handle<Value>(),
    bool turned_on_by_default = true);

  void SetInternalFieldCount(int value);
};

class EXPORT External : public Value {
 public:
  static Local<Value> Wrap(void* data);
  static inline void* Unwrap(Handle<Value> obj);
  static bool IsExternal(const Value* obj);

  static Local<External> New(Isolate* isolate, void* value);
  static External* Cast(Value* obj);
  void* Value() const;
};

class EXPORT Signature : public Data {
 public:
  static Local<Signature> New(Isolate* isolate,
                              Handle<FunctionTemplate> receiver =
                                Handle<FunctionTemplate>(),
                              int argc = 0,
                              Handle<FunctionTemplate> argv[] = nullptr);
 private:
  Signature();
};

class EXPORT AccessorSignature : public Data {
 public:
  static Local<AccessorSignature> New(
    Isolate* isolate,
    Handle<FunctionTemplate> receiver = Handle<FunctionTemplate>());
};

class EXPORT ResourceConstraints {
 public:
  void set_stack_limit(uint32_t *value) {}
};

class EXPORT Exception {
 public:
  static Local<Value> RangeError(Handle<String> message);
  static Local<Value> TypeError(Handle<String> message);
  static Local<Value> Error(Handle<String> message);
};

enum GCType {
  kGCTypeScavenge = 1 << 0,
  kGCTypeMarkSweepCompact = 1 << 1,
  kGCTypeAll = kGCTypeScavenge | kGCTypeMarkSweepCompact
};

enum GCCallbackFlags {
  kNoGCCallbackFlags = 0,
  kGCCallbackFlagCompacted = 1 << 0,
  kGCCallbackFlagConstructRetainedObjectInfos = 1 << 1,
  kGCCallbackFlagForced = 1 << 2
};

typedef void (*GCPrologueCallback)(GCType type, GCCallbackFlags flags);
typedef void (*GCEpilogueCallback)(GCType type, GCCallbackFlags flags);


class EXPORT HeapStatistics {
 private:
  size_t heapSize;

 public:
  void set_heap_size(size_t heapSize) {
    this->heapSize = heapSize;
  }

  size_t total_heap_size() { return this->heapSize; }
  size_t total_heap_size_executable() { return 0; }
  size_t total_physical_size() { return 0; }
  size_t used_heap_size() { return this->heapSize; }
  size_t heap_size_limit() { return 0; }
};

typedef int* (*CounterLookupCallback)(const char* name);
typedef void* (*CreateHistogramCallback)(
  const char* name, int min, int max, size_t buckets);
typedef void (*AddHistogramSampleCallback)(void* histogram, int sample);

class EXPORT Isolate {
 public:
  class EXPORT Scope {
   public:
    explicit Scope(Isolate* isolate) : isolate_(isolate) { isolate->Enter(); }
    ~Scope() { isolate_->Exit(); }
   private:
    Isolate* const isolate_;
    Scope(const Scope&);
    Scope& operator=(const Scope&);
  };

  static Isolate* New();
  static Isolate* GetCurrent();

  typedef bool (*abort_on_uncaught_exception_t)();
  void SetAbortOnUncaughtException(abort_on_uncaught_exception_t callback);

  void Enter();
  void Exit();
  void Dispose();

  void GetHeapStatistics(HeapStatistics *heap_statistics);
  int64_t AdjustAmountOfExternalAllocatedMemory(int64_t change_in_bytes);
  void SetData(uint32_t slot, void* data);
  void* GetData(uint32_t slot);
  static uint32_t GetNumberOfDataSlots();
  Local<Context> GetCurrentContext();
  void RunMicrotasks();
  void SetAutorunMicrotasks(bool autorun);
  Local<Value> ThrowException(Local<Value> exception);
  HeapProfiler* GetHeapProfiler();
  CpuProfiler* GetCpuProfiler();

  typedef void (*GCPrologueCallback)(
    Isolate* isolate, GCType type, GCCallbackFlags flags);
  typedef void (*GCEpilogueCallback)(
    Isolate* isolate, GCType type, GCCallbackFlags flags);
  void AddGCPrologueCallback(
    GCPrologueCallback callback, GCType gc_type_filter = kGCTypeAll);
  void RemoveGCPrologueCallback(GCPrologueCallback callback);
  void AddGCEpilogueCallback(
    GCEpilogueCallback callback, GCType gc_type_filter = kGCTypeAll);
  void RemoveGCEpilogueCallback(GCEpilogueCallback callback);

  void SetCounterFunction(CounterLookupCallback);
  void SetCreateHistogramFunction(CreateHistogramCallback);
  void SetAddHistogramSampleFunction(AddHistogramSampleCallback);
  bool IdleNotification(int idle_time_in_ms);
  void LowMemoryNotification();
  int ContextDisposedNotification();
};

class EXPORT JitCodeEvent {
 public:
  enum EventType {
    CODE_ADDED,
    CODE_MOVED,
    CODE_REMOVED,
  };

  EventType type;
  void * code_start;
  size_t code_len;
  union {
    struct {
      const char* str;
      size_t len;
    } name;
    void* new_code_start;
  };
};

class EXPORT V8 {
 public:
  static void SetFatalErrorHandler(FatalErrorCallback that);
  static void SetArrayBufferAllocator(ArrayBuffer::Allocator* allocator);
  static bool IsDead();
  static void SetFlagsFromString(const char* str, int length);
  static void SetFlagsFromCommandLine(
    int *argc, char **argv, bool remove_flags);
  static const char *GetVersion();
  static bool Initialize();
  static void SetEntropySource(EntropySource source);
  static void SetJitCodeEventHandler(
    JitCodeEventOptions options, JitCodeEventHandler event_handler);
  static void TerminateExecution(Isolate* isolate);
  static bool IsExeuctionDisabled(Isolate* isolate = nullptr);
  static void CancelTerminateExecution(Isolate* isolate);
  static bool Dispose();
  static bool AddMessageListener(
    MessageCallback that, Handle<Value> data = Handle<Value>());
  static void RemoveMessageListeners(MessageCallback that);
};

class EXPORT TryCatch {
 private:
  void GetAndClearException();
 private:
  JsValueRef error;
  TryCatch* prev;
  bool rethrow;
  bool verbose;
 public:
  TryCatch();
  ~TryCatch();
  bool HasCaught() const;
  bool HasTerminated() const;
  Handle<Value> ReThrow();
  Local<Value> Exception() const;
  Local<Value> StackTrace() const;
  Local<v8::Message> Message() const;
  void SetVerbose(bool value);
  void CheckReportExternalException();
};

class EXPORT ExtensionConfiguration {
};

class EXPORT Context {
 public:
  class EXPORT Scope {
   private:
    Scope * previous;
    void * context;
   public:
    Scope(Handle<Context> context);
    ~Scope();
  };

  Local<Object> Global();

  static Local<Context> New(
    Isolate* isolate,
    ExtensionConfiguration* extensions = NULL,
    Handle<ObjectTemplate> global_template = Handle<ObjectTemplate>(),
    Handle<Value> global_object = Handle<Value>());
  static Local<Context> GetCurrent();

  Isolate* GetIsolate();
  void* GetAlignedPointerFromEmbedderData(int index);
  void SetAlignedPointerInEmbedderData(int index, void* value);
  void SetSecurityToken(Handle<Value> token);
  Handle<Value> GetSecurityToken();
};

class EXPORT Locker {
  // Don't need to implement this for Chakra
 public:
  explicit Locker(Isolate* isolate) {}
};

//
// Handle<T> members
//

template <class T>
Handle<T>::Handle(T *val)
    : _ref(val) {
}

template <class T>
Handle<T>::Handle()
    : _ref(JS_INVALID_REFERENCE) {
}

template <class T>
template <class S>
Handle<T>::Handle(Handle<S> that)
    : _ref(*that) {
}

template <class T>
bool Handle<T>::IsEmpty() const {
  return _ref == JS_INVALID_REFERENCE;
}

template <class T>
void Handle<T>::Clear() {
  _ref = JS_INVALID_REFERENCE;
}

template <class T>
T *Handle<T>::operator->() const {
  return static_cast<T *>(_ref);
}

template <class T>
T *Handle<T>::operator*() const {
  return static_cast<T *>(_ref);
}

template <class T>
template <class S>
bool Handle<T>::operator==(Handle<S> that) const {
  return _ref == *that;
}

template <class T>
template <class S>
bool Handle<T>::operator!=(Handle<S> that) const {
  return _ref != *that;
}

template <class T>
template <class S>
Handle<S> Handle<T>::As() {
  return Handle<S>::Cast(*this);
}

template <class T>
template <class S>
Handle<T> Handle<T>::Cast(Handle<S> that) {
  return Handle<T>(T::Cast(*that));
}

//
// Local<T> members
//

template <class T>
Local<T>::Local() {
}

// CHAKRA-TODO: This one is a little strange. It's used in a couple of places
// outside of the class to cast from a Persistent<T> to a Local<T>. This seems
// incorrect since the persistent handle could be released, and there would be
// no scope holding on to the local handle. Something to look at later.
template <class T>
template <class S>
Local<T>::Local(S *that)
    : Handle<T>(that) {
}

template <class T>
template <class S>
Local<T>::Local(Local<S> that)
    : Handle<T>(*that) {
}

template <class T>
template <class S>
Local<T>::Local(Handle<S> that)
    : Handle<T>(reinterpret_cast<T*>(*that)) {
}

template <class T>
template <class S>
Local<S> Local<T>::As() {
  return Local<S>::Cast(*this);
}

template <class T>
template <class S>
Local<T> Local<T>::Cast(Local<S> that) {
  return Local<T>(T::Cast(*that));
}

template <class T>
Local<T> Local<T>::New(Handle<T> that) {
  if (!HandleScope::GetCurrent()->AddLocal(*that)) {
    return Local<T>();
  }
  return Local<T>(*that);
}

// Context are not javascript values, so we need to specialize them
template <>
inline Local<Context> Local<Context>::New(Handle<Context> that) {
  if (!HandleScope::GetCurrent()->AddLocalContext(*that)) {
    return Local<Context>();
  }
  return Local<Context>(*that);
}

template <class T>
Local<T> Local<T>::New(Isolate* isolate, Handle<T> that) {
  return New(that);
}

template <class T>
Local<T> Local<T>::New(Isolate* isolate, const Persistent<T>& that) {
  return New(that);
}

//
// Persistent<T> members
//

template <class T>
Persistent<T>::Persistent() {
}

template <class T>
Persistent<T>::Persistent(const Persistent<T> &that)
    : Handle<T>(*that), _weakWrapper(that._weakWrapper) {
  // CONSIDER: Whether we need to do a type/inheritance check here
  // CHAKRA-TODO: Handle error?
  if (_ref != JS_INVALID_REFERENCE && !IsWeak()) {
    JsAddRef(_ref, nullptr);
  }
}

template <class T>
template <class S>
Persistent<T>::Persistent(Persistent<S> that)
    : Handle<T>(*that), _weakWrapper(that._weakWrapper) {
  // CONSIDER: Whether we need to do a type/inheritance check here
  // CHAKRA-TODO: Handle error?
  if (_ref != JS_INVALID_REFERENCE && !IsWeak()) {
    JsAddRef(_ref, nullptr);
  }
}

template <class T>
template <class S>
Persistent<T>::Persistent(Isolate* isolate, Persistent<S> that)
    : Handle<T>(*that), _weakWrapper(that._weakWrapper) {
  // CONSIDER: Whether we need to do a type/inheritance check here
  // CHAKRA-TODO: Handle error?
  if (_ref != JS_INVALID_REFERENCE && !IsWeak()) {
    JsAddRef(_ref, nullptr);
  }
}

template <class T>
template <class S>
Persistent<T>::Persistent(S *that)
    : Handle<T>(that) {
  // CHAKRA-TODO: Handle error?
  if (_ref != JS_INVALID_REFERENCE) {
    JsAddRef(_ref, nullptr);
  }
}

template <class T>
Persistent<T>::Persistent(const Handle<T> &that)
    : Handle<T>(*that) {
  // CHAKRA-TODO: Handle error?
  if (_ref != JS_INVALID_REFERENCE) {
    JsAddRef(_ref, nullptr);
  }
}

template <class T>
template <class S>
Persistent<T>::Persistent(Handle<S> that)
    : Handle<T>(*that) {
  // CHAKRA-TODO: Handle error?
  if (_ref != JS_INVALID_REFERENCE) {
    JsAddRef(_ref, nullptr);
  }
}

template <class T>
template <class S>
Persistent<T>::Persistent(Isolate* isolate, Handle<S> that)
    : Handle<T>(*that) {
  // CHAKRA-TODO: Handle error?
  if (_ref != JS_INVALID_REFERENCE) {
    JsAddRef(_ref, nullptr);
  }
}

// Note to code reviwer: We duplicate code here in order to prevent any

template <class T>
Persistent<T>::~Persistent() {
  Dispose();
}

template <class T>
Persistent<T>& Persistent<T>::operator=(const Persistent<T> &rhs) {
  if (this != &rhs) {
    if (!rhs.IsWeak()) {
      SetNewRef(*rhs);
    } else {
      Dispose();
      _ref = rhs._ref;
      _weakWrapper = rhs._weakWrapper;
    }
  }
  return *this;
}

template <class T>
Persistent<T>& Persistent<T>::operator=(const Handle<T> &rhs) {
  SetNewRef(*rhs);
  return *this;
}

template <class T>
template <class S>
Persistent<T>& Persistent<T>::operator=(const Persistent<S> &rhs) {
  if (!rhs.IsWeak()) {
    SetNewRef(*rhs);
  } else {
    Dispose();
    _ref = rhs._ref;
    _weakWrapper = rhs._weakWrapper;
  }

  return *this;
}

template <class T>
template <class S>
Persistent<T>& Persistent<T>::operator=(const Handle<S> &rhs) {
  SetNewRef(*rhs);
  return *this;
}

template <class T>
template <class S>
Persistent<T>& Persistent<T>::operator=(S* that) {
  SetNewRef(reinterpret_cast<JsValueRef>(that));
  return *this;
}

template <class T>
void Persistent<T>::SetNewRef(JsValueRef ref) {
  Dispose();

  _ref = ref;
  if (_ref != JS_INVALID_REFERENCE) {
    JsAddRef(_ref, nullptr);
  }
}

template <class T>
template <class S>
Persistent<S> Persistent<T>::As() {
  return Persistent<S>::Cast(*this);
}

template <class T>
void Persistent<T>::Dispose() {
  // CHAKRA-TODO: Handle error?
  if (_ref != JS_INVALID_REFERENCE && !V8::IsDead()) {
    if (IsWeak()) {
      if (_weakWrapper.unique()) {
        chakrashim::ClearObjectWeakReferenceCallback(_ref, /*revive*/false);
      }
      _weakWrapper.reset();
    } else {
      JsRelease(_ref, nullptr);
    }

    _ref = JS_INVALID_REFERENCE;
  }
}

template <class T>
void Persistent<T>::Reset() {
  Dispose();
}

template <class T>
template <class S>
void Persistent<T>::Reset(Isolate* isolate, const Handle<S>& other) {
  *this = other;
}

template <class T>
template <class S>
void Persistent<T>::Reset(Isolate* isolate, const Persistent<S>& other) {
  *this = other;
}

template <class T>
template<typename P>
void Persistent<T>::SetWeak(
  P* parameter,
  typename WeakCallbackData<T, P>::Callback callback) {
  if (_ref != JS_INVALID_REFERENCE) {
    bool wasStrong = !IsWeak();
    typedef typename WeakCallbackData<Value, void>::Callback Callback;
    chakrashim::SetObjectWeakReferenceCallback(
      _ref, reinterpret_cast<Callback>(callback), parameter, &_weakWrapper);
    if (wasStrong) {
      JsRelease(_ref, nullptr);
    }
  }
}

template <class T>
void Persistent<T>::ClearWeak() {
  if (_ref != JS_INVALID_REFERENCE && IsWeak()) {
    if (_weakWrapper.unique()) {
      chakrashim::ClearObjectWeakReferenceCallback(_ref, /*revive*/true);
    }
    _weakWrapper.reset();

    JsAddRef(_ref, nullptr);
  }
}

template <class T>
void Persistent<T>::MarkIndependent() {
  // CONSIDER: It's a little unclear from the documentation just what this is
  // supposed to do...
}

template <class T>
bool Persistent<T>::IsNearDeath() const {
  // CHAKRA-TODO: Always return true for an assert for now, need to implement.
  return true;
}

template <class T>
bool Persistent<T>::IsWeak() const {
  return static_cast<bool>(_weakWrapper);
}

template <class T>
void Persistent<T>::SetWrapperClassId(uint16_t class_id) {
  // CONSIDER: Ignore. We don't do anything with this.
}

template <class T>
template <class S>
Persistent<T> Persistent<T>::Cast(Persistent<S> that) {
  return Persistent<T>(T::Cast(*that));
}

template <class T>
Persistent<T> Persistent<T>::New(Handle<T> that) {
  return Persistent<T>(that);
}

//
// HandleScope template members
//

template <class T>
Local<T> HandleScope::Close(Handle<T> value) {
  if (_prev == nullptr || !_prev->AddLocal(*value)) {
    return Local<T>();
  }

  return Local<T>(*value);
}

// Context are not javascript values, so we need to specialize them
template <>
inline Local<Context> HandleScope::Close(Handle<Context> value) {
  if (_prev == nullptr || !_prev->AddLocalContext(*value)) {
    return Local<Context>();
  }

  return Local<Context>(*value);
}

}  // namespace v8