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

// Stops windows.h from including winsock.h (conflicting with winsock2.h).
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif

#if defined(_WIN32_WINNT) && (_WIN32_WINNT < _WIN32_WINNT_WIN10)
#pragma message("warning: chakrashim requires Windows 10 SDK. "\
                "Redefine _WIN32_WINNT to _WIN32_WINNT_WIN10.")
#undef _WIN32_WINNT
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#endif

#ifndef USE_EDGEMODE_JSRT
#define USE_EDGEMODE_JSRT     // Only works with edge JSRT
#endif

#include <jsrt.h>

#ifndef _CHAKRART_H_
#error Wrong Windows SDK version
#endif

#include <stdio.h>
#include <stdint.h>
#include <memory>
#include "v8config.h"

#define V8_EXPORT __declspec(dllexport)

#define TYPE_CHECK(T, S)                                       \
  while (false) {                                              \
    *(static_cast<T* volatile*>(0)) = static_cast<S*>(0);      \
  }

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
class Platform;
class ResourceConstraints;
class RegExp;
class Script;
class Signature;
class StackFrame;
class String;
class StringObject;
class Uint32;
template <class T> class Handle;
template <class T> class Local;
template<class T> struct CopyablePersistentTraits;
template<class T> class PersistentBase;
template<class T, class M = CopyablePersistentTraits<T> > class Persistent;
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


template <class T>
class Handle {
 public:
  V8_INLINE Handle() : val_(0) {}

  template <class S>
  V8_INLINE Handle(Handle<S> that)
      : val_(reinterpret_cast<T*>(*that)) {
    TYPE_CHECK(T, S);
  }

  V8_INLINE bool IsEmpty() const { return val_ == 0; }
  V8_INLINE void Clear() { val_ = 0; }
  V8_INLINE T* operator->() const { return val_; }
  V8_INLINE T* operator*() const { return val_; }

  template <class S>
  V8_INLINE bool operator==(const Handle<S>& that) const {
    return val_ == that.val_;
  }

  template <class S>
  V8_INLINE bool operator==(const Persistent<S>& that) const {
    return val_ == that.val_;
  }

  template <class S>
  V8_INLINE bool operator!=(const Handle<S>& that) const {
    return !operator==(that);
  }

  template <class S>
  V8_INLINE bool operator!=(const Persistent<S>& that) const {
    return !operator==(that);
  }

  template <class S>
  V8_INLINE static Handle<T> Cast(Handle<S> that) {
    return Handle<T>(T::Cast(*that));
  }

  template <class S>
  V8_INLINE Handle<S> As() {
    return Handle<S>::Cast(*this);
  }

  V8_INLINE static Handle<T> New(Isolate* isolate, Handle<T> that) {
    return New(isolate, that.val_);
  }

  V8_INLINE static Handle<T> New(Isolate* isolate,
                                 const PersistentBase<T>& that) {
    return New(isolate, that.val_);
  }

 private:
  friend struct FunctionCallbackData;
  friend class FunctionTemplate;
  friend class ObjectTemplate;
  friend class Utils;
  template<class F, class M> friend class Persistent;
  template<class F> friend class PersistentBase;
  template<class F> friend class Handle;
  template<class F> friend class Local;
  friend V8_EXPORT Handle<Primitive> Undefined(Isolate* isolate);
  friend V8_EXPORT Handle<Primitive> Null(Isolate* isolate);
  friend V8_EXPORT Handle<Boolean> True(Isolate* isolate);
  friend V8_EXPORT Handle<Boolean> False(Isolate* isolate);

  V8_INLINE explicit Handle(T* val) : val_(val) {}
  V8_INLINE static Handle<T> New(Isolate* isolate, T* that) {
    return New(that);
  }

  V8_INLINE Handle(const Persistent<T>& that)
    : val_(*that) {
  }
  V8_INLINE static Handle<T> New(T* that);
  V8_INLINE static Handle<T> New(JsValueRef ref) {
    return New(static_cast<T*>(ref));
  }

  T* val_;
};


template <class T> class Local : public Handle<T> {
 public:
  V8_INLINE Local();

  template <class S>
  V8_INLINE Local(Local<S> that)
      : Handle<T>(reinterpret_cast<T*>(*that)) {
    TYPE_CHECK(T, S);
  }

  template <class S>
  V8_INLINE static Local<T> Cast(Local<S> that) {
    return Local<T>(T::Cast(*that));
  }

  template <class S>
  V8_INLINE Local(Handle<S> that)
      : Handle<T>(reinterpret_cast<T*>(*that)) {
    TYPE_CHECK(T, S);
  }

  template <class S>
  V8_INLINE Local<S> As() {
    return Local<S>::Cast(*this);
  }

  V8_INLINE static Local<T> New(Isolate* isolate, Handle<T> that);
  V8_INLINE static Local<T> New(Isolate* isolate,
                                const PersistentBase<T>& that);

 private:
  friend struct AcessorExternalDataType;
  friend class AccessorSignature;
  friend class Array;
  friend class Boolean;
  friend class BooleanObject;
  friend class Context;
  friend class Date;
  friend class External;
  friend class Function;
  friend struct FunctionCallbackData;
  friend class FunctionTemplate;
  friend struct FunctionTemplateData;
  friend class HandleScope;
  friend class Integer;
  friend class Number;
  friend class NumberObject;
  friend class Object;
  friend struct ObjectData;
  friend class ObjectTemplate;
  friend class Signature;
  friend class Script;
  friend class StackFrame;
  friend class StackTrace;
  friend class String;
  friend class StringObject;
  friend class Utils;
  friend class TryCatch;
  friend class UnboundScript;
  friend class Value;
  template <class F> friend class FunctionCallbackInfo;
  template <class F> friend class PersistentBase;
  template <class F, class M> friend class Persistent;

  template <class S>
  V8_INLINE Local(S* that) : Handle<T>(that) { }

  V8_INLINE static Local<T> New(Isolate* isolate, T* that) {
    return New(that);
  }

  V8_INLINE Local(JsValueRef that)
    : Handle<T>(static_cast<T*>(that)) {
  }
  V8_INLINE Local(const Persistent<T>& that)
    : Handle<T>(*that) {
  }
  V8_INLINE static Local<T> New(T* that);
  V8_INLINE static Local<T> New(JsValueRef ref) {
    return New(static_cast<T*>(ref));
  }
};


template<class T, class P>
class WeakCallbackData {
 public:
  typedef void (*Callback)(const WeakCallbackData<T, P>& data);

  V8_INLINE Isolate* GetIsolate() const { return isolate_; }
  V8_INLINE Local<T> GetValue() const { return handle_; }
  V8_INLINE P* GetParameter() const { return parameter_; }

 private:
  friend class Utils;
  WeakCallbackData(Isolate* isolate, Local<T> handle, P* parameter)
    : isolate_(isolate), handle_(handle), parameter_(parameter) { }
  Isolate* isolate_;
  Local<T> handle_;
  P* parameter_;
};


namespace chakrashim {
struct WeakReferenceCallbackWrapper {
  void *parameters;
  WeakCallbackData<Value, void>::Callback callback;
};
template class V8_EXPORT std::shared_ptr<WeakReferenceCallbackWrapper>;

// A helper method for setting an object with a WeakReferenceCallback. The
// callback will be called before the object is released.
V8_EXPORT void SetObjectWeakReferenceCallback(
    JsValueRef object,
    WeakCallbackData<Value, void>::Callback callback,
    void* parameters,
    std::shared_ptr<WeakReferenceCallbackWrapper>* weakWrapper);
// A helper method for turning off the WeakReferenceCallback that was set using
// the previous method
V8_EXPORT void ClearObjectWeakReferenceCallback(JsValueRef object, bool revive);

V8_EXPORT JsValueRef MarshalJsValueRefToContext(
  JsValueRef value, JsContextRef context);
}


template <class T>
class PersistentBase {
 public:
  V8_INLINE void Reset();

  template <class S>
  V8_INLINE void Reset(Isolate* isolate, const Handle<S>& other);

  V8_INLINE Local<T> get();

  template <class S>
  V8_INLINE void Reset(Isolate* isolate, const PersistentBase<S>& other);

  V8_INLINE bool IsEmpty() const { return val_ == NULL; }
  V8_INLINE void Empty() { Reset(); }

  template <class S>
  V8_INLINE bool operator==(const PersistentBase<S>& that) const {
    return val_ == that.val_;
  }

  template <class S>
  V8_INLINE bool operator==(const Handle<S>& that) const {
    return val_ == that.val_;
  }

  template <class S>
  V8_INLINE bool operator!=(const PersistentBase<S>& that) const {
    return !operator==(that);
  }

  template <class S>
  V8_INLINE bool operator!=(const Handle<S>& that) const {
    return !operator==(that);
  }

  template<typename P>
  V8_INLINE void SetWeak(
      P* parameter,
      typename WeakCallbackData<T, P>::Callback callback);

  template<typename P>
  V8_INLINE P* ClearWeak();

  V8_INLINE void ClearWeak() { ClearWeak<void>(); }
  V8_INLINE void MarkIndependent();
  // V8_INLINE void MarkPartiallyDependent();
  // V8_INLINE bool IsIndependent() const;
  V8_INLINE bool IsNearDeath() const;
  V8_INLINE bool IsWeak() const;
  V8_INLINE void SetWrapperClassId(uint16_t class_id);

 private:
  template<class F> friend class Handle;
  template<class F> friend class Local;
  template<class F1, class F2> friend class Persistent;

  explicit V8_INLINE PersistentBase(T* val) : val_(val) {}
  PersistentBase(PersistentBase& other) = delete;  // NOLINT
  void operator=(PersistentBase&) = delete;
  V8_INLINE static T* New(Isolate* isolate, T* that);

  T* val_;
  std::shared_ptr<chakrashim::WeakReferenceCallbackWrapper> _weakWrapper;
};


template<class T>
struct CopyablePersistentTraits {
  typedef Persistent<T, CopyablePersistentTraits<T> > CopyablePersistent;
  static const bool kResetInDestructor = true;
  template<class S, class M>
  static V8_INLINE void Copy(const Persistent<S, M>& source,
                             CopyablePersistent* dest) {
    // do nothing, just allow copy
  }
};


template <class T, class M>
class Persistent : public PersistentBase<T> {
 public:
  V8_INLINE Persistent() : PersistentBase<T>(0) { }

  template <class S>
  V8_INLINE Persistent(Isolate* isolate, Handle<S> that)
      : PersistentBase<T>(PersistentBase<T>::New(isolate, *that)) {
    TYPE_CHECK(T, S);
  }

  template <class S, class M2>
  V8_INLINE Persistent(Isolate* isolate, const Persistent<S, M2>& that)
    : PersistentBase<T>(PersistentBase<T>::New(isolate, *that)) {
    TYPE_CHECK(T, S);
  }

  V8_INLINE Persistent(const Persistent& that) : PersistentBase<T>(0) {
    Copy(that);
  }

  template <class S, class M2>
  V8_INLINE Persistent(const Persistent<S, M2>& that) : PersistentBase<T>(0) {
    Copy(that);
  }

  V8_INLINE Persistent& operator=(const Persistent& that) { // NOLINT
    Copy(that);
    return *this;
  }

  template <class S, class M2>
  V8_INLINE Persistent& operator=(const Persistent<S, M2>& that) { // NOLINT
    Copy(that);
    return *this;
  }

  V8_INLINE ~Persistent() {
    if (M::kResetInDestructor) this->Reset();
  }

  template <class S>
  V8_INLINE static Persistent<T>& Cast(Persistent<S>& that) { // NOLINT
    return reinterpret_cast<Persistent<T>&>(that);
  }

  template <class S>
  V8_INLINE Persistent<S>& As() { // NOLINT
    return Persistent<S>::Cast(*this);
  }

 private:
  friend class Object;
  friend struct ObjectData;
  friend class ObjectTemplate;
  friend struct ObjectTemplateData;
  friend struct TemplateData;
  friend struct FunctionCallbackData;
  friend class FunctionTemplate;
  friend struct FunctionTemplateData;
  friend class Utils;
  template<class F> friend class Handle;
  template<class F> friend class Local;
  template<class F> friend class ReturnValue;

  V8_INLINE Persistent(T* that)
    : PersistentBase<T>(PersistentBase<T>::New(nullptr, that)) { }

  V8_INLINE T* operator*() const { return this->val_; }
  V8_INLINE T* operator->() const { return this->val_; }

  template<class S, class M2>
  V8_INLINE void Copy(const Persistent<S, M2>& that);

  template <class S>
  V8_INLINE Persistent& operator=(const Handle<S>& other) {
    Reset(nullptr, other);
    return *this;
  }
  V8_INLINE Persistent& operator=(JsRef other) {
    return operator=(Local<T>(static_cast<T*>(other)));
  }
};


template <class T>
class Eternal : private Persistent<T> {
 public:
  Eternal() {}

  template<class S>
  Eternal(Isolate* isolate, Local<S> handle) {
    Set(isolate, handle);
  }

  Local<T> Get(Isolate* isolate) {
    return Local<T>::New(isolate, *this);
  }

  template<class S> void Set(Isolate* isolate, Local<S> handle) {
    Reset(isolate, handle);
  }
};


template<class T>
class UniquePersistent : public PersistentBase<T> {
  struct RValue {
    V8_INLINE explicit RValue(UniquePersistent* obj) : object(obj) {}
    UniquePersistent* object;
  };

 public:
  V8_INLINE UniquePersistent() : PersistentBase<T>(0) {}

  template <class S>
  V8_INLINE UniquePersistent(Isolate* isolate, Handle<S> that)
    : PersistentBase<T>(PersistentBase<T>::New(isolate, *that)) {
    TYPE_CHECK(T, S);
  }

  template <class S>
  V8_INLINE UniquePersistent(Isolate* isolate, const PersistentBase<S>& that)
    : PersistentBase<T>(PersistentBase<T>::New(isolate, that.val_)) {
    TYPE_CHECK(T, S);
  }

  V8_INLINE UniquePersistent(RValue rvalue)
      : PersistentBase<T>(rvalue.object->val_) {
    this._weakWrapper = rvalue.object->_weakWrapper;
    rvalue.object->val_ = 0;
    rvalue.object->_weakWrapper.reset();
  }

  V8_INLINE ~UniquePersistent() { this->Reset(); }

  template<class S>
  V8_INLINE UniquePersistent& operator=(UniquePersistent<S> rhs) {
    TYPE_CHECK(T, S);
    this->Reset();
    this->val_ = rhs.val_;
    this->_weakWrapper = rhs._weakWrapper;
    rhs.val_ = 0;
    rhs._weakWrapper.reset();
    return *this;
  }

  V8_INLINE operator RValue() { return RValue(this); }

  UniquePersistent Pass() { return UniquePersistent(RValue(this)); }

 private:
  UniquePersistent(UniquePersistent&);
  void operator=(UniquePersistent&);
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
class V8_EXPORT HandleScope {
 public:
  HandleScope(Isolate* isolate);
  ~HandleScope();

  static int NumberOfHandles(Isolate* isolate);

 private:
  friend class EscapableHandleScope;
  template <class T> friend class Handle;
  template <class T> friend class Local;
  static const int kOnStackLocals = 5;  // Arbitrary number of refs on stack

  JsValueRef _locals[kOnStackLocals];   // Save some refs on stack
  JsValueRef _refs;                     // More refs go to a JS array
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

  template <class T>
  Local<T> Close(Handle<T> value);
};

class V8_EXPORT EscapableHandleScope : public HandleScope {
 public:
  EscapableHandleScope(Isolate* isolate) : HandleScope(isolate) {}

  template <class T>
  Local<T> Escape(Handle<T> value) { return Close(value); }
};

class V8_EXPORT Data {
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

class V8_EXPORT UnboundScript {
 public:
  Local<Script> BindToCurrentContext();
};

class V8_EXPORT Script {
 public:
  static Local<Script> Compile(
    Handle<String> source, ScriptOrigin* origin = NULL);
  static Local<Script> Compile(Handle<String> source, Handle<String> file_name);
  Local<Value> Run();
  Local<UnboundScript> GetUnboundScript();
};

class V8_EXPORT ScriptCompiler {
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

class V8_EXPORT Message {
 public:
  Local<String> GetSourceLine() const;
  Handle<Value> GetScriptResourceName() const;
  int GetLineNumber() const;
  int GetStartColumn() const;
  int GetEndColumn() const;

  static const int kNoLineNumberInfo = 0;
  static const int kNoColumnInfo = 0;
  static const int kNoScriptIdInfo = 0;
};

typedef void (*MessageCallback)(Handle<Message> message, Handle<Value> error);

class V8_EXPORT StackTrace {
 public:
  enum StackTraceOptions {
    kLineNumber = 1,
    kColumnOffset = 1 << 1 | kLineNumber,
    kScriptName = 1 << 2,
    kFunctionName = 1 << 3,
    kIsEval = 1 << 4,
    kIsConstructor = 1 << 5,
    kScriptNameOrSourceURL = 1 << 6,
    kScriptId = 1 << 7,
    kExposeFramesAcrossSecurityOrigins = 1 << 8,
    kOverview = kLineNumber | kColumnOffset | kScriptName | kFunctionName,
    kDetailed = kOverview | kIsEval | kIsConstructor | kScriptNameOrSourceURL
  };

  Local<StackFrame> GetFrame(uint32_t index) const;
  int GetFrameCount() const;
  Local<Array> AsArray();

  static Local<StackTrace> CurrentStackTrace(
    Isolate* isolate,
    int frame_limit,
    StackTraceOptions options = kOverview);
};

class V8_EXPORT StackFrame {
 public:
  int GetLineNumber() const;
  int GetColumn() const;
  int GetScriptId() const;
  Local<String> GetScriptName() const;
  Local<String> GetScriptNameOrSourceURL() const;
  Local<String> GetFunctionName() const;
  bool IsEval() const;
  bool IsConstructor() const;
};

class V8_EXPORT Value : public Data {
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
  bool IsArrayBuffer() const;
  bool IsTypedArray() const;
  bool IsUint8Array() const;
  bool IsUint8ClampedArray() const;
  bool IsInt8Array() const;
  bool IsUint16Array() const;
  bool IsInt16Array() const;
  bool IsUint32Array() const;
  bool IsInt32Array() const;
  bool IsFloat32Array() const;
  bool IsFloat64Array() const;
  bool IsDataView() const;

  Local<Boolean> ToBoolean() const;
  Local<Number> ToNumber() const;
  Local<String> ToString() const;
  Local<String> ToDetailString() const;
  Local<Object> ToObject() const;
  Local<Integer> ToInteger() const;
  Local<Uint32> ToUint32() const;
  Local<Int32> ToInt32() const;

  Local<Uint32> ToArrayIndex() const;

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

class V8_EXPORT Primitive : public Value {
 public:
};

class V8_EXPORT Boolean : public Primitive {
 public:
  bool Value() const;
  static Handle<Boolean> New(Isolate* isolate, bool value);

 private:
  friend class BooleanObject;
  template <class F> friend class ReturnValue;
  static Local<Boolean> From(bool value);
};

class V8_EXPORT String : public Primitive {
 public:
  int Length() const;
  int Utf8Length() const;
  bool IsOneByte() const { return false; }
  bool ContainsOnlyOneByte() const { return false; }

  enum WriteOptions {
    NO_OPTIONS = 0,
    HINT_MANY_WRITES_EXPECTED = 1,
    NO_NULL_TERMINATION = 2,
    PRESERVE_ONE_BYTE_NULL = 4,
    REPLACE_INVALID_UTF8 = 8
  };

  int Write(uint16_t *buffer,
            int start = 0,
            int length = -1,
            int options = NO_OPTIONS) const;
  int WriteOneByte(uint8_t* buffer,
                   int start = 0,
                   int length = -1,
                   int options = NO_OPTIONS) const;
  int WriteUtf8(char *buffer,
                int length = -1,
                int *nchars_ref = NULL,
                int options = NO_OPTIONS) const;

  static Local<String> Empty(Isolate* isolate);
  bool IsExternal() const { return false; }
  bool IsExternalAscii() const { return false; }

  class V8_EXPORT ExternalAsciiStringResource {
   public:
    virtual ~ExternalAsciiStringResource() {}
    virtual const char *data() const = 0;
    virtual size_t length() const = 0;
  };

  class V8_EXPORT ExternalStringResource {
   public:
    virtual ~ExternalStringResource() {}
    virtual const uint16_t* data() const = 0;
    virtual size_t length() const = 0;
  };

  ExternalStringResource* GetExternalStringResource() const { return NULL; }
  const ExternalAsciiStringResource* GetExternalAsciiStringResource() const {
    return NULL;
  }

  static String *Cast(v8::Value *obj);

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

  static Local<String> Concat(Handle<String> left, Handle<String> right);
  static Local<String> NewExternal(
    Isolate* isolate, ExternalStringResource* resource);
  static Local<String> NewExternal(
    Isolate* isolate, ExternalAsciiStringResource *resource);

  class V8_EXPORT Utf8Value {
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
    int _length;
  };

  class V8_EXPORT Value {
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
    int _length;
  };

 private:
  template <class ToWide>
  static Local<String> New(const ToWide& toWide,
                           const char *data, int length = -1);
  static Local<String> New(const wchar_t *data, int length = -1);

  JsValueRef _ref;
};

class V8_EXPORT Number : public Primitive {
 public:
  double Value() const;
  static Local<Number> New(Isolate* isolate, double value);
  static Number *Cast(v8::Value *obj);

 private:
  friend class Integer;
  template <class F> friend class ReturnValue;
  static Local<Number> From(double value);
};

class V8_EXPORT Integer : public Number {
 public:
  static Local<Integer> New(Isolate* isolate, int32_t value);
  static Local<Integer> NewFromUnsigned(Isolate* isolate, uint32_t value);
  static Integer *Cast(v8::Value *obj);

  int64_t Value() const;

 private:
  friend class Utils;
  template <class F> friend class ReturnValue;
  static Local<Integer> From(int32_t value);
  static Local<Integer> From(uint32_t value);
};

class V8_EXPORT Int32 : public Integer {
 public:
  int32_t Value() const;
};

class V8_EXPORT Uint32 : public Integer {
 public:
  uint32_t Value() const;
};

class V8_EXPORT Object : public Value {
 public:
  bool Set(Handle<Value> key, Handle<Value> value);
  bool Set(uint32_t index, Handle<Value> value);
  bool ForceSet(Handle<Value> key, Handle<Value> value,
                PropertyAttribute attribs = None);
  Local<Value> Get(Handle<Value> key);
  Local<Value> Get(uint32_t index);
  PropertyAttribute GetPropertyAttributes(Handle<Value> key);
  Local<Value> GetOwnPropertyDescriptor(Local<String> key);
  bool Has(Handle<Value> key);
  bool Delete(Handle<Value> key);
  bool Has(uint32_t index);
  bool Delete(uint32_t index);
  bool SetAccessor(Handle<String> name,
                   AccessorGetterCallback getter,
                   AccessorSetterCallback setter = 0,
                   Handle<Value> data = Handle<Value>(),
                   AccessControl settings = DEFAULT,
                   PropertyAttribute attribute = None);
  Local<Array> GetPropertyNames();
  Local<Array> GetOwnPropertyNames();
  Local<Value> GetPrototype();
  bool SetPrototype(Handle<Value> prototype);
  Local<String> ObjectProtoToString();
  Local<String> GetConstructorName();

  int InternalFieldCount();
  Local<Value> GetInternalField(int index);
  void SetInternalField(int index, Handle<Value> value);
  void* GetAlignedPointerFromInternalField(int index);
  void SetAlignedPointerInInternalField(int index, void* value);

  bool HasOwnProperty(Handle<String> key);
  bool HasRealNamedProperty(Handle<String> key);
  bool HasRealIndexedProperty(uint32_t index);
  bool HasRealNamedCallbackProperty(Handle<String> key);

  Local<Value> GetRealNamedPropertyInPrototypeChain(Handle<String> key);
  Local<Value> GetRealNamedProperty(Handle<String> key);

  bool SetHiddenValue(Handle<String> key, Handle<Value> value);
  Local<Value> GetHiddenValue(Handle<String> key);

  Local<Object> Clone();
  Local<Context> CreationContext();

  void SetIndexedPropertiesToExternalArrayData(void *data,
                                               ExternalArrayType array_type,
                                               int number_of_elements);
  bool HasIndexedPropertiesInExternalArrayData();
  void *GetIndexedPropertiesExternalArrayData();
  ExternalArrayType GetIndexedPropertiesExternalArrayDataType();
  int GetIndexedPropertiesExternalArrayDataLength();

  Local<Value> CallAsFunction(Handle<Value> recv,
                              int argc,
                              Handle<Value> argv[]);
  Local<Value> CallAsConstructor(int argc, Handle<Value> argv[]);

  static Local<Object> New(Isolate* isolate = nullptr);
  static Object *Cast(Value *obj);

 private:
  friend struct ObjectData;
  friend class ObjectTemplate;
  friend class Utils;

  bool Set(Handle<Value> key, Handle<Value> value, PropertyAttribute attribs,
           bool force);
  bool SetAccessor(Handle<String> name,
                   AccessorGetterCallback getter,
                   AccessorSetterCallback setter,
                   Handle<Value> data,
                   AccessControl settings,
                   PropertyAttribute attribute,
                   Handle<AccessorSignature> signature);

  JsErrorCode GetObjectData(struct ObjectData** objectData);
  ObjectTemplate* GetObjectTemplate();
};

class V8_EXPORT Array : public Object {
 public:
  uint32_t Length() const;
  Local<Object> CloneElementAt(uint32_t index);

  static Local<Array> New(Isolate* isolate = nullptr, int length = 0);
  static Array *Cast(Value *obj);
};

class V8_EXPORT BooleanObject : public Object {
 public:
  static Local<Value> New(bool value);
  bool ValueOf() const;
  static BooleanObject* Cast(Value* obj);
};


class V8_EXPORT StringObject : public Object {
 public:
  static Local<Value> New(Handle<String> value);
  Local<String> ValueOf() const;
  static StringObject* Cast(Value* obj);
};

class V8_EXPORT NumberObject : public Object {
 public:
  static Local<Value> New(Isolate * isolate, double value);
  double ValueOf() const;
  static NumberObject* Cast(Value* obj);
};

class V8_EXPORT Date : public Object {
 public:
  static Local<Value> New(Isolate * isolate, double time);
  static Date *Cast(Value *obj);
};

class V8_EXPORT RegExp : public Object {
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
  void Set(bool value) { Set(Boolean::From(value)); }
  void Set(double value) { Set(Number::From(value)); }
  void Set(int32_t value) { Set(Integer::From(value)); }
  void Set(uint32_t value) { Set(Integer::From(value)); }
  // Fast JS primitive setters
  void SetNull() { Set(Null(nullptr)); }
  void SetUndefined() { Set(Undefined(nullptr)); }
  void SetEmptyString() { Set(String::Empty(nullptr)); }
  // Convenience getter for Isolate
  Isolate* GetIsolate() { return Isolate::GetCurrent(); }

  Value* Get() const { return *_value; }

 private:
  template <typename F> friend class FunctionCallbackInfo;
  template <typename F> friend class PropertyCallbackInfo;

  ReturnValue(Value** value, Handle<Context> context)
    : _value(value), _context(context) {
  }

  Value** _value;
  Local<Context> _context;
};

template<typename T>
class FunctionCallbackInfo {
 public:
  int Length() const { return _length; }
  Local<Value> operator[](int i) const {
    return (i >= 0 && i < _length) ?
      _args[i] : *Undefined(nullptr).As<Value>();
  }
  Local<Function> Callee() const { return _callee; }
  Local<Object> This() const { return _thisPointer; }
  Local<Object> Holder() const { return _holder; }
  bool IsConstructCall() const { return _isConstructorCall; }
  Local<Value> Data() const { return _data; }
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
    Local<Value> data,
    Local<Function> callee)
       : _args(args),
         _length(length),
         _thisPointer(_this),
         _holder(holder),
         _isConstructorCall(isConstructorCall),
         _data(data),
         _callee(callee),
         _returnValue(static_cast<Value*>(JS_INVALID_REFERENCE)),
         _context(Context::GetCurrent()) {
  }

 private:
  int _length;
  Local<Object> _thisPointer;
  Local<Object> _holder;
  Local<Function> _callee;
  Local<Value> _data;
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

class V8_EXPORT Function : public Object {
 public:
  static Local<Function> New(
    Isolate * isolate,
    FunctionCallback callback,
    Local<Value> data = Local<Value>(),
    int length = 0);
  Local<Object> NewInstance() const;
  Local<Object> NewInstance(int argc, Handle<Value> argv[]) const;
  Local<Value> Call(Handle<Value> recv, int argc, Handle<Value> argv[]);
  void SetName(Handle<String> name);
  // Handle<Value> GetName() const;

  static Function *Cast(Value *obj);
};

class V8_EXPORT Promise : public Object {
 public:
  class V8_EXPORT Resolver : public Object {
   public:
    static Local<Resolver> New(Isolate* isolate);
    Local<Promise> GetPromise();
    void Resolve(Handle<Value> value);
    void Reject(Handle<Value> value);
    V8_INLINE static Resolver* Cast(Value* obj);

   private:
    Resolver();
    static void CheckCast(Value* obj);
  };

  Local<Promise> Chain(Handle<Function> handler);
  Local<Promise> Catch(Handle<Function> handler);
  Local<Promise> Then(Handle<Function> handler);

  V8_INLINE static Promise* Cast(Value* obj);

 private:
  Promise();
  static void CheckCast(Value* obj);
};


class V8_EXPORT ArrayBuffer : public Object {
 public:
  class V8_EXPORT Allocator {  // NOLINT
   public:
    virtual ~Allocator() {}
    virtual void* Allocate(size_t length) = 0;
    virtual void* AllocateUninitialized(size_t length) = 0;
    virtual void Free(void* data, size_t length) = 0;
  };

  class V8_EXPORT Contents {  // NOLINT
   public:
    Contents() : data_(NULL), byte_length_(0) {}
    void* Data() const { return data_; }
    size_t ByteLength() const { return byte_length_; }

   private:
    void* data_;
    size_t byte_length_;
    friend class ArrayBuffer;
  };

  size_t ByteLength() const;
  static Local<ArrayBuffer> New(Isolate* isolate, size_t byte_length);
  static Local<ArrayBuffer> New(Isolate* isolate, void* data,
                                size_t byte_length);

  bool IsExternal() const;
  void Neuter();
  Contents Externalize();
  static ArrayBuffer* Cast(Value* obj);

 private:
  ArrayBuffer();
};

class V8_EXPORT ArrayBufferView : public Object {
 public:
  Local<ArrayBuffer> Buffer();
  size_t ByteOffset();
  size_t ByteLength();

  static ArrayBufferView* Cast(Value* obj);
 private:
  ArrayBufferView();
};

class V8_EXPORT TypedArray : public ArrayBufferView {
 public:
  size_t Length();
  static TypedArray* Cast(Value* obj);
 private:
  TypedArray();
};

class V8_EXPORT Uint8Array : public TypedArray {
 public:
  static Local<Uint8Array> New(Handle<ArrayBuffer> array_buffer,
                               size_t byte_offset, size_t length);
  static Uint8Array* Cast(Value* obj);
 private:
  Uint8Array();
};

class V8_EXPORT Uint8ClampedArray : public TypedArray {
 public:
  static Local<Uint8ClampedArray> New(Handle<ArrayBuffer> array_buffer,
                                      size_t byte_offset, size_t length);
  static Uint8ClampedArray* Cast(Value* obj);
 private:
  Uint8ClampedArray();
};

class V8_EXPORT Int8Array : public TypedArray {
 public:
  static Local<Int8Array> New(Handle<ArrayBuffer> array_buffer,
                              size_t byte_offset, size_t length);
  static Int8Array* Cast(Value* obj);
 private:
  Int8Array();
};

class V8_EXPORT Uint16Array : public TypedArray {
 public:
  static Local<Uint16Array> New(Handle<ArrayBuffer> array_buffer,
                                size_t byte_offset, size_t length);
  static Uint16Array* Cast(Value* obj);
 private:
  Uint16Array();
};

class V8_EXPORT Int16Array : public TypedArray {
 public:
  static Local<Int16Array> New(Handle<ArrayBuffer> array_buffer,
                               size_t byte_offset, size_t length);
  static Int16Array* Cast(Value* obj);
 private:
  Int16Array();
};

class V8_EXPORT Uint32Array : public TypedArray {
 public:
  static Local<Uint32Array> New(Handle<ArrayBuffer> array_buffer,
                                size_t byte_offset, size_t length);
  static Uint32Array* Cast(Value* obj);
 private:
  Uint32Array();
};

class V8_EXPORT Int32Array : public TypedArray {
 public:
  static Local<Int32Array> New(Handle<ArrayBuffer> array_buffer,
                               size_t byte_offset, size_t length);
  static Int32Array* Cast(Value* obj);
 private:
  Int32Array();
};

class V8_EXPORT Float32Array : public TypedArray {
 public:
  static Local<Float32Array> New(Handle<ArrayBuffer> array_buffer,
                                 size_t byte_offset, size_t length);
  static Float32Array* Cast(Value* obj);
 private:
  Float32Array();
};

class V8_EXPORT Float64Array : public TypedArray {
 public:
  static Local<Float64Array> New(Handle<ArrayBuffer> array_buffer,
                                 size_t byte_offset, size_t length);
  static Float64Array* Cast(Value* obj);
 private:
  Float64Array();
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

class V8_EXPORT Template : public Data {
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

class V8_EXPORT FunctionTemplate : public Template {
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
  void SetCallHandler(FunctionCallback callback,
                      Handle<Value> data = Handle<Value>());
  bool HasInstance(Handle<Value> object);
  void Inherit(Handle<FunctionTemplate> parent);
};

class V8_EXPORT ObjectTemplate : public Template {
 public:
  static Local<ObjectTemplate> New(Isolate* isolate);

  Local<Object> NewInstance();
  void SetClassName(Handle<String> name);

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
  void SetCallAsFunctionHandler(FunctionCallback callback,
                                Handle<Value> data = Handle<Value>());

 private:
  friend struct FunctionCallbackData;
  friend struct FunctionTemplateData;
  friend class Utils;

  Local<Object> NewInstance(Handle<Object> prototype);
  Handle<String> GetClassName();
};

class V8_EXPORT External : public Value {
 public:
  static Local<Value> Wrap(void* data);
  static inline void* Unwrap(Handle<Value> obj);
  static bool IsExternal(const Value* obj);

  static Local<External> New(Isolate* isolate, void* value);
  static External* Cast(Value* obj);
  void* Value() const;
};

class V8_EXPORT Signature : public Data {
 public:
  static Local<Signature> New(Isolate* isolate,
                              Handle<FunctionTemplate> receiver =
                                Handle<FunctionTemplate>(),
                              int argc = 0,
                              Handle<FunctionTemplate> argv[] = nullptr);
 private:
  Signature();
};

class V8_EXPORT AccessorSignature : public Data {
 public:
  static Local<AccessorSignature> New(
    Isolate* isolate,
    Handle<FunctionTemplate> receiver = Handle<FunctionTemplate>());
};


V8_EXPORT Handle<Primitive> Undefined(Isolate* isolate);
V8_EXPORT Handle<Primitive> Null(Isolate* isolate);
V8_EXPORT Handle<Boolean> True(Isolate* isolate);
V8_EXPORT Handle<Boolean> False(Isolate* isolate);
V8_EXPORT bool SetResourceConstraints(ResourceConstraints *constraints);


class V8_EXPORT ResourceConstraints {
 public:
  void set_stack_limit(uint32_t *value) {}
};

class V8_EXPORT Exception {
 public:
  static Local<Value> RangeError(Handle<String> message);
  static Local<Value> ReferenceError(Handle<String> message);
  static Local<Value> SyntaxError(Handle<String> message);
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


class V8_EXPORT HeapStatistics {
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

typedef void(*FunctionEntryHook)(uintptr_t function,
                                 uintptr_t return_addr_location);
typedef int* (*CounterLookupCallback)(const char* name);
typedef void* (*CreateHistogramCallback)(
  const char* name, int min, int max, size_t buckets);
typedef void (*AddHistogramSampleCallback)(void* histogram, int sample);

class V8_EXPORT Isolate {
 public:
  class V8_EXPORT Scope {
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

class V8_EXPORT JitCodeEvent {
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

class V8_EXPORT StartupData {
};

class V8_EXPORT V8 {
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
  static void InitializePlatform(Platform* platform) {}
};

class V8_EXPORT TryCatch {
 public:
  TryCatch();
  ~TryCatch();

  bool HasCaught() const;
  bool CanContinue() const { return true;  }
  bool HasTerminated() const;
  Handle<Value> ReThrow();
  Local<Value> Exception() const;
  Local<Value> StackTrace() const;
  Local<v8::Message> Message() const;
  void Reset();
  void SetVerbose(bool value);
  void SetCaptureMessage(bool value);

 private:
  friend class Function;

  void GetAndClearException();
  void CheckReportExternalException();

  JsValueRef error;
  TryCatch* prev;
  bool rethrow;
  bool verbose;
};

class V8_EXPORT ExtensionConfiguration {
};

class V8_EXPORT Context {
 public:
  class V8_EXPORT Scope {
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

class V8_EXPORT Locker {
  // Don't need to implement this for Chakra
 public:
  explicit Locker(Isolate* isolate) {}
};


//
// Local<T> members
//

template <class T>
Local<T>::Local() : Handle<T>() {}


template <class T>
Local<T> Local<T>::New(Isolate* isolate, Handle<T> that) {
  return New(isolate, that.val_);
}

template <class T>
Local<T> Local<T>::New(Isolate* isolate, const PersistentBase<T>& that) {
  return New(isolate, that.val_);
}

template <class T>
Handle<T> Handle<T>::New(T* that) {
  if (!HandleScope::GetCurrent()->AddLocal(that)) {
    return Handle<T>();
  }
  return Handle<T>(that);
}

// Context are not javascript values, so we need to specialize them
template <>
Handle<Context> Handle<Context>::New(Context* that) {
  if (!HandleScope::GetCurrent()->AddLocalContext(that)) {
    return Handle<Context>();
  }
  return Handle<Context>(that);
}

template <class T>
Local<T> Local<T>::New(T* that) {
  return Local<T>(__super::New(that));
}

//
// Persistent<T> members
//

template <class T>
T* PersistentBase<T>::New(Isolate* isolate, T* that) {
  if (that) {
    JsAddRef(static_cast<JsRef>(that), nullptr);
  }
  return that;
}

template <class T, class M>
template <class S, class M2>
void Persistent<T, M>::Copy(const Persistent<S, M2>& that) {
  TYPE_CHECK(T, S);
  this->Reset();
  if (that.IsEmpty()) return;

  this->val_ = that.val_;
  this->_weakWrapper = that._weakWrapper;
  if (val_ && !IsWeak()) {
    JsAddRef(val_, nullptr);
  }

  M::Copy(that, this);
}

template <class T>
bool PersistentBase<T>::IsNearDeath() const {
  return true;
}

template <class T>
bool PersistentBase<T>::IsWeak() const {
  return static_cast<bool>(_weakWrapper);
}

template <class T>
void PersistentBase<T>::Reset() {
  if (this->IsEmpty()) return;

  if (IsWeak()) {
    if (_weakWrapper.unique()) {
      chakrashim::ClearObjectWeakReferenceCallback(val_, /*revive*/false);
    }
    _weakWrapper.reset();
  } else {
    JsRelease(val_, nullptr);
  }

  val_ = nullptr;
}

template <class T>
template <class S>
void PersistentBase<T>::Reset(Isolate* isolate, const Handle<S>& other) {
  TYPE_CHECK(T, S);
  Reset();
  if (other.IsEmpty()) return;
  this->val_ = New(isolate, other.val_);
}

template <class T>
template <class S>
void PersistentBase<T>::Reset(Isolate* isolate,
                              const PersistentBase<S>& other) {
  TYPE_CHECK(T, S);
  Reset();
  if (other.IsEmpty()) return;
  this->val_ = New(isolate, other.val_);
}

template <class T>
Local<T> PersistentBase<T>::get() {
  return this->val_;
}

template <class T>
template <typename P>
void PersistentBase<T>::SetWeak(
    P* parameter,
    typename WeakCallbackData<T, P>::Callback callback) {
  if (this->IsEmpty()) return;

  bool wasStrong = !IsWeak();
  typedef typename WeakCallbackData<Value, void>::Callback Callback;
  chakrashim::SetObjectWeakReferenceCallback(
    val_, reinterpret_cast<Callback>(callback), parameter, &_weakWrapper);
  if (wasStrong) {
    JsRelease(val_, nullptr);
  }
}

template <class T>
template <typename P>
P* PersistentBase<T>::ClearWeak() {
  if (!IsWeak()) return nullptr;

  P* parameters = reinterpret_cast<P*>(_weakWrapper->parameters);
  if (_weakWrapper.unique()) {
    chakrashim::ClearObjectWeakReferenceCallback(val_, /*revive*/true);
  }
  _weakWrapper.reset();

  JsAddRef(val_, nullptr);
  return parameters;
}

template <class T>
void PersistentBase<T>::MarkIndependent() {
}

template <class T>
void PersistentBase<T>::SetWrapperClassId(uint16_t class_id) {
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
