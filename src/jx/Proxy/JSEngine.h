// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_JSENGINE_H_
#define SRC_JX_PROXY_JSENGINE_H_

#include "EngineLogger.h"

#ifdef JS_IS_NOT_V8
#undef JS_ENGINE_V8  // an ugly hack for Eclipse Editor / Debugger
#endif

#ifdef V8_IS_3_14

#include "v8.h"
#include "v8-debug.h"
#include "v8-profiler.h"
#include "jx/Proxy/V8_3_14/PMacro.h"
#include "jx/Proxy/V8_3_14/PArguments.h"
#include "jx/Proxy/V8_3_14/v8_typed_array.h"

#define NODE_OBJECT_WRAP_HEADER "jx/Proxy/V8_3_14/node_object_wrap.h"

#elif defined(MOZJS_IS_3_40)

#include "Mozilla_340/PMacro.h"
#include "Mozilla_340/MozJS/MozJS.h"
#include "Mozilla_340/JXString.h"
#include "Mozilla_340/PArguments.h"
#include "Mozilla_340/EngineHelper.h"

#define NODE_OBJECT_WRAP_HEADER "jx/Proxy/Mozilla_340/node_object_wrap.h"

#endif

#if defined(JS_ENGINE_V8)

#define ENGINE_NS v8
typedef JS_HANDLE_VALUE (*JS_NATIVE_METHOD)(const JS_V8_ARGUMENT& args);
typedef void (*JS_FINALIZER_METHOD)(JS_HANDLE_VALUE_REF val, void* data);

#ifndef JXCORE_ALOG_TAG
#define JXCORE_ALOG_TAG "jxcore-log"
#if defined(__ANDROID__) && defined(JXCORE_EMBEDDED)
#include <android/log.h>
#define log_console(...) \
  __android_log_print(ANDROID_LOG_INFO, JXCORE_ALOG_TAG, __VA_ARGS__)
#define flush_console(...) \
  __android_log_print(ANDROID_LOG_INFO, JXCORE_ALOG_TAG, __VA_ARGS__)
#define error_console(...) \
  __android_log_print(ANDROID_LOG_ERROR, JXCORE_ALOG_TAG, __VA_ARGS__)
#define warn_console(...) \
  __android_log_print(ANDROID_LOG_WARN, JXCORE_ALOG_TAG, __VA_ARGS__)
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
#endif

#elif defined(JS_ENGINE_MOZJS)

#define ENGINE_NS MozJS

typedef bool (*JS_NATIVE_METHOD)(JSContext *ctx, unsigned argc, JS::Value *val);
#endif

// We can not use the JS_ENTER_SCOPE etc. here since we leave
// the whole scope every time one of the member methods exit
// This implementation is for 'embedders' only (jxcore_init)
#ifdef JS_ENGINE_V8
#define JS_ENGINE_SCOPE(x, pass)              \
  v8::Locker locker(x->node_isolate);         \
  if (pass) {                                 \
    x->node_isolate->Enter();                 \
  }                                           \
  v8::HandleScope handle_scope;               \
  v8::Context::Scope context_scope(context_); \
  v8::Isolate* __contextORisolate = x->node_isolate
#elif defined(JS_ENGINE_MOZJS)
#define JS_ENGINE_SCOPE(x, pass) \
  JS_ENTER_SCOPE();              \
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
