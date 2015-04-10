// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_MOZJS_ISOLATE_H_
#define SRC_JX_PROXY_MOZILLA_MOZJS_ISOLATE_H_
#ifndef __POSIX__
#define EXPORT_JS_API
#endif

#include "jsapi.h"
#include "jsfriendapi.h"

#ifndef JXCORE_ALOG_TAG
#define JXCORE_ALOG_TAG "jxcore-log"
#ifdef __ANDROID__  // change to EMBEDDED
#include <android/log.h>
#define ALOG_TAG "jxcore-log"
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

#define JS_OBJECT_SLOT_COUNT 3
#define JS_NATIFIED_OBJECT_SLOT_COUNT (JS_OBJECT_SLOT_COUNT + 1)
#define JS_NATIFIED_OBJECT_SLOT_INDEX JS_OBJECT_SLOT_COUNT
#define GC_SLOT_JS_CLASS (JS_OBJECT_SLOT_COUNT - 1)
#define GC_SLOT_GC_CALL (JS_OBJECT_SLOT_COUNT - 2)
#define JS_OBJECT_SLOT_MAX_INDEX (JS_OBJECT_SLOT_COUNT - 3)

namespace MozJS {
class Isolate {
  bool disposable_;
  int threadId_;

 public:
  JSContext* ctx_;

  Isolate();

  Isolate(JSContext* ctx, int threadId, bool disposable);

  static Isolate* New(int threadId = -1);

  static Isolate* GetCurrent();

  static Isolate* GetByThreadId(const int threadId);

  inline JSContext* GetRaw() { return ctx_; }

  void* GetData();

  void SetData(void* data);

  void Dispose();
};
}  // namespace MozJS
#endif  // SRC_JX_PROXY_MOZILLA_MOZJS_ISOLATE_H_
