/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2015 NAN contributors:
 *   - Rod Vagg <https://github.com/rvagg>
 *   - Benjamin Byholm <https://github.com/kkoopa>
 *   - Trevor Norris <https://github.com/trevnorris>
 *   - Nathan Rajlich <https://github.com/TooTallNate>
 *   - Brett Lawson <https://github.com/brett19>
 *   - Ben Noordhuis <https://github.com/bnoordhuis>
 *   - David Siegel <https://github.com/agnat>
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 *
 * Version 1.8.4: current Node 12: 0.12.2, Node 10: 0.10.38, io.js: 1.8.1
 *
 * See https://github.com/nodejs/nan for the latest update to this file
 **********************************************************************************/

#ifndef NAN_H_
#define NAN_H_

#include <uv.h>
#include <node.h>
#include <node_buffer.h>
#include <node_version.h>
#include <node_object_wrap.h>
#include <cstring>
#include <climits>
#include <cstdlib>
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4530)
#include <string>
#pragma warning(pop)
#else
#include <string>
#endif

#if defined(__GNUC__) && !(defined(DEBUG) && DEBUG)
#define NAN_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER) && !(defined(DEBUG) && DEBUG)
#define NAN_INLINE __forceinline
#else
#define NAN_INLINE inline
#endif

#if defined(__GNUC__) && \
    !(defined(V8_DISABLE_DEPRECATIONS) && V8_DISABLE_DEPRECATIONS)
#define NAN_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER) && \
    !(defined(V8_DISABLE_DEPRECATIONS) && V8_DISABLE_DEPRECATIONS)
#define NAN_DEPRECATED __declspec(deprecated)
#else
#define NAN_DEPRECATED
#endif

#if __cplusplus >= 201103L
#define NAN_DISALLOW_ASSIGN(CLASS) void operator=(const CLASS &) = delete;
#define NAN_DISALLOW_COPY(CLASS) CLASS(const CLASS &) = delete;
#define NAN_DISALLOW_MOVE(CLASS)                      \
  CLASS(CLASS &&) = delete; /* NOLINT(build/c++11) */ \
  void operator=(CLASS &&) = delete;
#else
#define NAN_DISALLOW_ASSIGN(CLASS) void operator=(const CLASS &);
#define NAN_DISALLOW_COPY(CLASS) CLASS(const CLASS &);
#define NAN_DISALLOW_MOVE(CLASS)
#endif

#define NAN_DISALLOW_ASSIGN_COPY(CLASS) \
  NAN_DISALLOW_ASSIGN(CLASS)            \
  NAN_DISALLOW_COPY(CLASS)

#define NAN_DISALLOW_ASSIGN_MOVE(CLASS) \
  NAN_DISALLOW_ASSIGN(CLASS)            \
  NAN_DISALLOW_MOVE(CLASS)

#define NAN_DISALLOW_COPY_MOVE(CLASS) \
  NAN_DISALLOW_COPY(CLASS)            \
  NAN_DISALLOW_MOVE(CLASS)

#define NAN_DISALLOW_ASSIGN_COPY_MOVE(CLASS) \
  NAN_DISALLOW_ASSIGN(CLASS)                 \
  NAN_DISALLOW_COPY(CLASS)                   \
  NAN_DISALLOW_MOVE(CLASS)

#define NODE_0_10_MODULE_VERSION 11
#define NODE_0_12_MODULE_VERSION 12
#define ATOM_0_21_MODULE_VERSION 41
#define IOJS_1_0_MODULE_VERSION 42
#define IOJS_1_1_MODULE_VERSION 43

// #include "nan_new.h"  // NOLINT(build/include)

// uv helpers
#ifdef UV_VERSION_MAJOR
#ifndef UV_VERSION_PATCH
#define UV_VERSION_PATCH 0
#endif
#define NAUV_UVVERSION \
  ((UV_VERSION_MAJOR << 16) | (UV_VERSION_MINOR << 8) | (UV_VERSION_PATCH))
#else
#define NAUV_UVVERSION 0x000b00
#endif

#if NAUV_UVVERSION < 0x000b17
#define NAUV_WORK_CB(func) void func(uv_async_t *async, int)
#else
#define NAUV_WORK_CB(func) void func(uv_async_t *async)
#endif

#if NAUV_UVVERSION >= 0x000b0b

typedef uv_key_t nauv_key_t;

inline int nauv_key_create(nauv_key_t *key) { return uv_key_create(key); }

inline void nauv_key_delete(nauv_key_t *key) { uv_key_delete(key); }

inline void *nauv_key_get(nauv_key_t *key) { return uv_key_get(key); }

inline void nauv_key_set(nauv_key_t *key, void *value) {
  uv_key_set(key, value);
}

#else

/* Implement thread local storage for older versions of libuv.
 * This is essentially a backport of libuv commit 5d2434bf
 * written by Ben Noordhuis, adjusted for names and inline.
 */

#ifndef WIN32

#include <pthread.h>

typedef pthread_key_t nauv_key_t;

inline int nauv_key_create(nauv_key_t *key) {
  return -pthread_key_create(key, NULL);
}

inline void nauv_key_delete(nauv_key_t *key) {
  if (pthread_key_delete(*key)) abort();
}

inline void *nauv_key_get(nauv_key_t *key) { return pthread_getspecific(*key); }

inline void nauv_key_set(nauv_key_t *key, void *value) {
  if (pthread_setspecific(*key, value)) abort();
}

#else

#include <windows.h>

typedef struct { DWORD tls_index; } nauv_key_t;

inline int nauv_key_create(nauv_key_t *key) {
  key->tls_index = TlsAlloc();
  if (key->tls_index == TLS_OUT_OF_INDEXES) return UV_ENOMEM;
  return 0;
}

inline void nauv_key_delete(nauv_key_t *key) {
  if (TlsFree(key->tls_index) == FALSE) abort();
  key->tls_index = TLS_OUT_OF_INDEXES;
}

inline void *nauv_key_get(nauv_key_t *key) {
  void *value = TlsGetValue(key->tls_index);
  if (value == NULL)
    if (GetLastError() != ERROR_SUCCESS) abort();
  return value;
}

inline void nauv_key_set(nauv_key_t *key, void *value) {
  if (TlsSetValue(key->tls_index, value) == FALSE) abort();
}

#endif
#endif

#define OPTIONS_TO_BOOLEAN(name, default_value)                          \
  bool name = default_value;                                             \
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID(#name))) {                    \
    JS_LOCAL_VALUE obj_ = JS_GET_NAME(optionsObj, JS_STRING_ID(#name)); \
    name = BOOLEAN_TO_STD(obj_);                                         \
  }

NAN_INLINE
JS_LOCAL_OBJECT NanObjectWrapHandle(const node::ObjectWrap *obj) {
  return JS_TYPE_TO_LOCAL_OBJECT(const_cast<node::ObjectWrap *>(obj)->handle_);
}

NAN_INLINE bool NanHasInstance(
    JS_PERSISTENT_FUNCTION_TEMPLATE &function_template, JS_HANDLE_VALUE value) {
  return function_template->HasInstance(value);
}

NAN_INLINE JS_LOCAL_VALUE NanMakeCallback(JS_HANDLE_OBJECT target,
                                          JS_HANDLE_FUNCTION func, int argc,
                                          JS_HANDLE_VALUE *argv) {
  return JS_TYPE_TO_LOCAL_VALUE(node::MakeCallback(target, func, argc, argv));
}

NAN_INLINE JS_LOCAL_VALUE NanMakeCallback(JS_HANDLE_OBJECT target,
                                          JS_HANDLE_STRING symbol, int argc,
                                          JS_HANDLE_VALUE *argv) {
  return JS_TYPE_TO_LOCAL_VALUE(node::MakeCallback(target, symbol, argc, argv));
}

NAN_INLINE JS_LOCAL_VALUE NanMakeCallback(JS_HANDLE_OBJECT target,
                                          const char *method, int argc,
                                          JS_HANDLE_VALUE *argv) {
  return JS_TYPE_TO_LOCAL_VALUE(node::MakeCallback(target, method, argc, argv));
}

NAN_INLINE void NanFatalException(const ENGINE_NS::TryCatch &try_catch) {
  node::FatalException(const_cast<ENGINE_NS::TryCatch &>(try_catch));
}

typedef void (*NanFreeCallback)(char *data, void *hint);

class NanCallback {
 public:
  node::commons *com_;
  NanCallback() {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);
    com_ = com;

    JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
    handle = JS_NEW_PERSISTENT_OBJECT(obj);
  }

  explicit NanCallback(JS_HANDLE_FUNCTION &fn) {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);
    com_ = com;

    JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
    handle = JS_NEW_PERSISTENT_OBJECT(obj);
    SetFunction(fn);
  }

  ~NanCallback() {
    if (JS_IS_EMPTY(handle)) return;
    JS_CLEAR_PERSISTENT(handle);
  }

  bool operator==(const NanCallback &other) const {
    JS_ENTER_SCOPE();
    JS_LOCAL_OBJECT obj = JS_TYPE_TO_LOCAL_OBJECT(handle);
    JS_LOCAL_VALUE a = JS_GET_INDEX(obj, kCallbackIndex);
    JS_LOCAL_OBJECT other_obj = JS_TYPE_TO_LOCAL_OBJECT(other.handle);
    JS_LOCAL_VALUE b = JS_GET_INDEX(other_obj, kCallbackIndex);
    return a->StrictEquals(b);
  }

  bool operator!=(const NanCallback &other) const {
    return !this->operator==(other);
  }

  NAN_INLINE void SetFunction(JS_HANDLE_FUNCTION &fn) {
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(com_);
    JS_LOCAL_OBJECT obj = JS_TYPE_TO_LOCAL_OBJECT(handle);
    JS_INDEX_SET(obj, kCallbackIndex, fn);
  }

  NAN_INLINE JS_LOCAL_FUNCTION GetFunction() {
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(com_);
    JS_LOCAL_OBJECT obj = JS_TYPE_TO_LOCAL_OBJECT(handle);
    JS_LOCAL_VALUE val = JS_GET_INDEX(obj, kCallbackIndex);
    return JS_LEAVE_SCOPE(JS_CAST_FUNCTION(val));
  }

  NAN_INLINE bool IsEmpty() const {
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(com_);
    JS_LOCAL_OBJECT obj = JS_TYPE_TO_LOCAL_OBJECT(handle);
    JS_LOCAL_VALUE val = JS_GET_INDEX(obj, kCallbackIndex);
    return JS_IS_UNDEFINED(val);
  }

  NAN_INLINE JS_HANDLE_VALUE
      Call(JS_HANDLE_OBJECT target, int argc, JS_HANDLE_VALUE argv[]) {
    return Call_(target, argc, argv);
  }

  NAN_INLINE JS_HANDLE_VALUE Call(int argc, JS_HANDLE_VALUE argv[]) {
    JS_DEFINE_STATE_MARKER(com_);
    return Call_(JS_GET_GLOBAL(), argc, argv);
  }

 private:
  NAN_DISALLOW_ASSIGN_COPY_MOVE(NanCallback)
  JS_PERSISTENT_OBJECT handle;
  static const uint32_t kCallbackIndex = 0;

  JS_HANDLE_VALUE Call_(JS_HANDLE_OBJECT target, int argc,
                        JS_HANDLE_VALUE argv[]) {
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(com_);

    JS_LOCAL_VALUE val = JS_GET_INDEX(handle, kCallbackIndex);
    JS_LOCAL_FUNCTION callback = JS_TYPE_AS_FUNCTION(val);
    return JS_LEAVE_SCOPE(node::MakeCallback(target, callback, argc, argv));
  }
};

/* abstract */ class NanAsyncWorker {
 public:
  node::commons *com_;
  explicit NanAsyncWorker(NanCallback *callback_)
      : callback(callback_), errmsg_(NULL) {
    request.data = this;

    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);
    com_ = com;
    JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
    persistentHandle = JS_NEW_PERSISTENT_OBJECT(obj);
  }

  virtual ~NanAsyncWorker() {
    JS_ENTER_SCOPE();

    if (!JS_IS_EMPTY(persistentHandle)) JS_CLEAR_PERSISTENT(persistentHandle);
    if (callback) delete callback;
    if (errmsg_) delete[] errmsg_;
  }

  virtual void WorkComplete() {
    JS_ENTER_SCOPE();

    if (errmsg_ == NULL)
      HandleOKCallback();
    else
      HandleErrorCallback();
    delete callback;
    callback = NULL;
  }

  NAN_INLINE void SaveToPersistent(const char *key, JS_LOCAL_OBJECT &obj) {
    JS_DEFINE_STATE_MARKER(com_);
    JS_LOCAL_OBJECT handle = JS_TYPE_TO_LOCAL_OBJECT(persistentHandle);
    JS_NAME_SET(handle, JS_STRING_ID(key), obj);
  }

  JS_LOCAL_OBJECT GetFromPersistent(const char *key) const {
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(com_);
    JS_LOCAL_OBJECT handle = JS_TYPE_TO_LOCAL_OBJECT(persistentHandle);
    JS_LOCAL_VALUE val = JS_GET_NAME(handle, JS_STRING_ID(key));
    return JS_LEAVE_SCOPE(JS_VALUE_TO_OBJECT(val));
  }

  virtual void Execute() = 0;

  uv_work_t request;

  virtual void Destroy() { delete this; }

 protected:
  JS_PERSISTENT_OBJECT persistentHandle;
  NanCallback *callback;

  virtual void HandleOKCallback() { callback->Call(0, NULL); }

  virtual void HandleErrorCallback() {
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(com_);

    JS_LOCAL_STRING str = STD_TO_STRING(ErrorMessage());

    JS_LOCAL_VALUE argv[] = {JS_NEW_ERROR_VALUE(str)};
    callback->Call(1, argv);
  }

  void SetErrorMessage(const char *msg) {
    if (errmsg_) {
      delete[] errmsg_;
    }

    size_t size = strlen(msg) + 1;
    errmsg_ = new char[size];
    memcpy(errmsg_, msg, size);
  }

  const char *ErrorMessage() const { return errmsg_; }

 private:
  NAN_DISALLOW_ASSIGN_COPY_MOVE(NanAsyncWorker)
  char *errmsg_;
};

/* abstract */
class NanAsyncProgressWorker : public NanAsyncWorker {
 public:
  explicit NanAsyncProgressWorker(NanCallback *callback_)
      : NanAsyncWorker(callback_), asyncdata_(NULL), asyncsize_(0) {
    async = new uv_async_t;
    uv_async_init(callback_->com_->loop, async, AsyncProgress_);
    async->data = this;

    uv_mutex_init(&async_lock);
  }

  virtual ~NanAsyncProgressWorker() {
    uv_mutex_destroy(&async_lock);

    if (asyncdata_) {
      delete[] asyncdata_;
    }
  }

  void WorkProgress() {
    uv_mutex_lock(&async_lock);
    char *data = asyncdata_;
    size_t size = asyncsize_;
    asyncdata_ = NULL;
    uv_mutex_unlock(&async_lock);

    // Dont send progress events after we've already completed.
    if (callback) {
      HandleProgressCallback(data, size);
    }
    delete[] data;
  }

  class ExecutionProgress {
    friend class NanAsyncProgressWorker;

   public:
    // You could do fancy generics with templates here.
    void Send(const char *data, size_t size) const {
      that_->SendProgress_(data, size);
    }

   private:
    explicit ExecutionProgress(NanAsyncProgressWorker *that) : that_(that) {}
    NAN_DISALLOW_ASSIGN_COPY_MOVE(ExecutionProgress)
    NanAsyncProgressWorker *const that_;
  };

  virtual void Execute(const ExecutionProgress &progress) = 0;
  virtual void HandleProgressCallback(const char *data, size_t size) = 0;

  virtual void Destroy() {
    uv_close(reinterpret_cast<uv_handle_t *>(async), AsyncClose_);
  }

 private:
  void Execute() /*final override*/ {
    ExecutionProgress progress(this);
    Execute(progress);
  }

  void SendProgress_(const char *data, size_t size) {
    char *new_data = new char[size];
    memcpy(new_data, data, size);

    uv_mutex_lock(&async_lock);
    char *old_data = asyncdata_;
    asyncdata_ = new_data;
    asyncsize_ = size;
    uv_mutex_unlock(&async_lock);

    if (old_data) {
      delete[] old_data;
    }
    uv_async_send(async);
  }

  NAN_INLINE static NAUV_WORK_CB(AsyncProgress_) {
    NanAsyncProgressWorker *worker =
        static_cast<NanAsyncProgressWorker *>(async->data);
    worker->WorkProgress();
  }

  NAN_INLINE static void AsyncClose_(uv_handle_t *handle) {
    NanAsyncProgressWorker *worker =
        static_cast<NanAsyncProgressWorker *>(handle->data);
    delete reinterpret_cast<uv_async_t *>(handle);
    delete worker;
  }

  uv_async_t *async;
  uv_mutex_t async_lock;
  char *asyncdata_;
  size_t asyncsize_;
};

NAN_INLINE void NanAsyncExecute(uv_work_t *req) {
  NanAsyncWorker *worker = static_cast<NanAsyncWorker *>(req->data);
  worker->Execute();
}

NAN_INLINE void NanAsyncExecuteComplete(uv_work_t *req) {
  NanAsyncWorker *worker = static_cast<NanAsyncWorker *>(req->data);
  worker->WorkComplete();
  worker->Destroy();
}

NAN_INLINE void NanAsyncQueueWorker(NanAsyncWorker *worker) {
  uv_queue_work(uv_default_loop(), &worker->request, NanAsyncExecute,
                (uv_after_work_cb)NanAsyncExecuteComplete);
}

//// Base 64 ////

#define _nan_base64_encoded_size(size) ((size + 2 - ((size + 2) % 3)) / 3 * 4)

// Doesn't check for padding at the end.  Can be 1-2 bytes over.
NAN_INLINE size_t _nan_base64_decoded_size_fast(size_t size) {
  size_t remainder = size % 4;

  size = (size / 4) * 3;
  if (remainder) {
    if (size == 0 && remainder == 1) {
      // special case: 1-byte input cannot be decoded
      size = 0;
    } else {
      // non-padded input, add 1 or 2 extra bytes
      size += 1 + (remainder == 3);
    }
  }

  return size;
}

template <typename T>
NAN_INLINE size_t _nan_base64_decoded_size(const T *src, size_t size) {
  if (size == 0) return 0;

  if (src[size - 1] == '=') size--;
  if (size > 0 && src[size - 1] == '=') size--;

  return _nan_base64_decoded_size_fast(size);
}

// supports regular and URL-safe base64
static const int _nan_unbase64_table[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -1, -1, -2, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 62, -1, 62, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1,
    63, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1};

#define _nan_unbase64(x) _nan_unbase64_table[(uint8_t)(x)]

template <typename T>
static size_t _nan_base64_decode(char *buf, size_t len, const T *src,
                                 const size_t srcLen) {
  char *dst = buf;
  char *dstEnd = buf + len;
  const T *srcEnd = src + srcLen;

  while (src < srcEnd && dst < dstEnd) {
    ptrdiff_t remaining = srcEnd - src;
    char a, b, c, d;

    while (_nan_unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
    if (remaining == 0 || *src == '=') break;
    a = _nan_unbase64(*src++);

    while (_nan_unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
    if (remaining <= 1 || *src == '=') break;
    b = _nan_unbase64(*src++);

    *dst++ = (a << 2) | ((b & 0x30) >> 4);
    if (dst == dstEnd) break;

    while (_nan_unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
    if (remaining <= 2 || *src == '=') break;
    c = _nan_unbase64(*src++);

    *dst++ = ((b & 0x0F) << 4) | ((c & 0x3C) >> 2);
    if (dst == dstEnd) break;

    while (_nan_unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
    if (remaining <= 3 || *src == '=') break;
    d = _nan_unbase64(*src++);

    *dst++ = ((c & 0x03) << 6) | (d & 0x3F);
  }

  return dst - buf;
}

//// HEX ////

template <typename T>
unsigned _nan_hex2bin(T c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  return static_cast<unsigned>(-1);
}

template <typename T>
static size_t _nan_hex_decode(char *buf, size_t len, const T *src,
                              const size_t srcLen) {
  size_t i;
  for (i = 0; i < len && i * 2 + 1 < srcLen; ++i) {
    unsigned a = _nan_hex2bin(src[i * 2 + 0]);
    unsigned b = _nan_hex2bin(src[i * 2 + 1]);
    if (!~a || !~b) return i;
    buf[i] = a * 16 + b;
  }

  return i;
}

#endif  // NAN_H_
