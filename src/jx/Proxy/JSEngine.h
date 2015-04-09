// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_JSENGINE_H_
#define SRC_JX_PROXY_JSENGINE_H_

#include "EngineLogger.h"

#ifdef JS_IS_NOT_V8
#undef JS_ENGINE_V8  // an ugly hack for Eclipse Editor / Debugger
#endif

#if defined(JS_ENGINE_V8)

#define ENGINE_NS v8

#include "v8.h"
#include "v8-debug.h"
#include "v8-profiler.h"
#include "jx/Proxy/V8/PMacro.h"
#include "jx/Proxy/V8/PArguments.h"
#include "jx/Proxy/V8/v8_typed_array.h"

typedef JS_HANDLE_VALUE (*JS_NATIVE_METHOD)(const v8::Arguments& args);
typedef void (*JS_FINALIZER_METHOD)(JS_HANDLE_VALUE_REF val, void* data);

#ifdef __ANDROID__  // change to EMBEDDED
#include <android/log.h>
#define ALOG_TAG "jxcore-log"
#define log_console(...) \
  __android_log_print(ANDROID_LOG_INFO, ALOG_TAG, __VA_ARGS__)
#define flush_console(...) \
  __android_log_print(ANDROID_LOG_INFO, ALOG_TAG, __VA_ARGS__)
#define error_console(...) \
  __android_log_print(ANDROID_LOG_ERROR, ALOG_TAG, __VA_ARGS__)
#define warn_console(...) \
  __android_log_print(ANDROID_LOG_WARN, ALOG_TAG, __VA_ARGS__)
#else
#define log_console(...) fprintf(stdout, __VA_ARGS__)
#define flush_console(...)        \
  do {                            \
    fprintf(stdout, __VA_ARGS__); \
    fflush(stdout);               \
  } while (0)
#define error_console(...) fprintf(stderr, __VA_ARGS__)
#define warn_console(...) fprintf(stderr, __VA_ARGS__)
#endif

#elif defined(JS_ENGINE_MOZJS)

#define ENGINE_NS MozJS

#include "Mozilla/PMacro.h"
#include "Mozilla/MozJS/MozJS.h"
#include "Mozilla/JXString.h"
#include "Mozilla/PArguments.h"
#include "Mozilla/EngineHelper.h"

typedef bool (*JS_NATIVE_METHOD)(JSContext *ctx, unsigned argc, JS::Value *val);
#endif

// We can not use the JS_ENTER_SCOPE etc. here since we leave
// the whole scope every time one of the member methods exit
// This implementation is for 'embedders' only (jxcore_init)
#ifdef JS_ENGINE_V8
#define JS_ENGINE_SCOPE(x, pass)                      \
  v8::Locker locker(x->node_isolate); \
  if (pass) {                        \
    x->node_isolate->Enter();         \
  }                                            \
  v8::HandleScope handle_scope;                \
  v8::Context::Scope context_scope(context_);  \
  v8::Isolate* __contextORisolate = x->node_isolate
#elif defined(JS_ENGINE_MOZJS)
#define JS_ENGINE_SCOPE(x, pass) \
  JS_ENTER_SCOPE();       \
  JS_DEFINE_STATE_MARKER(x)
#endif

// Internals
#if defined(JS_ENGINE_MOZJS)
#define __JS_LOCAL_STRING const char *
#define __JS_LOCAL_VALUE jsval
#else
#define __JS_LOCAL_STRING JS_LOCAL_STRING
#define __JS_LOCAL_VALUE JS_LOCAL_VALUE
#endif
#endif  // SRC_JX_PROXY_JSENGINE_H_
