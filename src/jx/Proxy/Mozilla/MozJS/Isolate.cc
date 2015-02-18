// Copyright & License details are available under JXCORE_LICENSE file

#include "Isolate.h"
#include "assert.h"
#include "../EngineHelper.h"

namespace MozJS {
#define JXCORE_MAX_THREAD_COUNT 65  // +1 for main thread
Isolate* Isolates[JXCORE_MAX_THREAD_COUNT] = {NULL};
JSRuntime* runtimes[JXCORE_MAX_THREAD_COUNT] = {NULL};

int threadIds[65] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                     13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
                     26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
                     39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
                     52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64};

Isolate::Isolate() {
  ctx_ = NULL;
  threadId_ = -1;
  disposable_ = false;
}

Isolate::Isolate(JSContext* ctx, int threadId, bool disposable) {
  ctx_ = ctx;
  threadId_ = threadId;
  disposable_ = disposable;
}

int GetThreadId() { return EngineHelper::GetThreadId(); }

Isolate* Isolate::GetCurrent() { return Isolates[GetThreadId()]; }

Isolate* Isolate::New(int threadId) {  // for_thread is true only for initial
                                       // context
  const bool for_thread = threadId != -1;

  if (for_thread) {
    if (runtimes[threadId] != NULL) {
      abort();
    }

    if (threadId == 0)
      runtimes[threadId] =
          JS_NewRuntime(64L * 1024L * 1024L, 8L * 1024L * 1024L);
    else
      runtimes[threadId] =
          JS_NewRuntime(64L * 1024L * 1024L, 8L * 1024L * 1024L, runtimes[0]);

    assert(runtimes[threadId] != NULL);

    JS_SetRuntimePrivate(runtimes[threadId], &threadIds[threadId]);

    JS_SetGCParameter(runtimes[threadId], JSGC_MAX_BYTES, 0xffffffff);
    JS_SetNativeStackQuota(runtimes[threadId], 256 * sizeof(size_t) * 1024);
    JS_SetGCParameter(runtimes[threadId], JSGC_MODE, JSGC_MODE_INCREMENTAL);
    JS_SetDefaultLocale(runtimes[threadId], "UTF-8");

#if !defined(__ANDROID__) && !defined(__IOS__)
    JS_SetGCParametersBasedOnAvailableMemory(runtimes[threadId], 513);
#endif

#ifndef __IOS__
    JS::RuntimeOptionsRef(runtimes[threadId])
        .setBaseline(true)
        .setIon(true)
        .setAsmJS(true)
        .setNativeRegExp(true);
#endif
  } else {
    threadId = GetThreadId();
  }

  JSRuntime* rt = runtimes[threadId];
  JSContext* ctx = JS_NewContext(rt, 32 * 1024);

  assert(ctx != NULL);
  JS_SetThreadId(ctx, threadId);

  if (for_thread) {
    Isolates[threadId] = new Isolate(ctx, threadId, true);
    return Isolates[threadId];
  } else {
    // TODO(obastemur) replace this to auto_ptr kind of implementation
    // to prevent mistakes(leak)
    return new Isolate(ctx, threadId, false);
  }
}

Isolate* Isolate::GetByThreadId(const int threadId) {
  assert(threadId > -1 && threadId < JXCORE_MAX_THREAD_COUNT);
  return Isolates[threadId];
}

void* Isolate::GetData() { return JS_GetContextPrivate(ctx_); }

void Isolate::SetData(void* data) { return JS_SetContextPrivate(ctx_, data); }

void Isolate::Dispose() {
  if (disposable_) {
    assert(Isolates[threadId_] != NULL);
    delete Isolates[threadId_];
    Isolates[threadId_] = NULL;
    runtimes[threadId_] = NULL;
  }
}
}  // namespace MozJS
