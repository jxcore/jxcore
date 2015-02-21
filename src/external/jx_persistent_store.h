// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_EXTERNAL_JX_PERSISTENT_STORE_H_
#define SRC_EXTERNAL_JX_PERSISTENT_STORE_H_

#if (NODE_MODULE_VERSION > 0x000B)
#define NODE12
#endif

#include <string.h>
#include "node.h"

namespace jxcore {
#ifdef JS_ENGINE_V8
#define JX_ISOLATE v8::Isolate *
#define JX_CURRENT_ENGINE() v8::Isolate::GetCurrent()
#ifdef NODE12
#define JX_GET_ENGINE_DATA(x) x->GetData(0)
#else
#define JX_GET_ENGINE_DATA(x) x->GetData();
#endif
#elif defined(JS_ENGINE_MOZJS)
#define JX_ISOLATE JS_ENGINE_MARKER
#define JX_CURRENT_ENGINE() JS_CURRENT_ENGINE()
#define JX_GET_ENGINE_DATA(x) JS_CURRENT_ENGINE_DATA(x)
#endif

#define MAX_JX_THREADS 64  // JXcore support max 64 SM/v8 threads per process

template <class T>
class ThreadStore {
  static bool mted;

  void initStore() {
#ifndef USES_JXCORE_ENGINE
    templates = new T[1];
    mted = false;
#else
    mted = true;
    templates = new T[MAX_JX_THREADS];
#endif
  }

  int _threadId() const {
    if (!mted) return 0;

    JX_ISOLATE iso = JX_CURRENT_ENGINE();

    if (iso == NULL) return -2;

    void *id = JX_GET_ENGINE_DATA(iso);

    if (id == NULL) {
      return -1;
    } else {
      int tid = *((int *)id);
      return tid;
    }
  }

 public:
  T *templates;

  ThreadStore() { initStore(); }

  explicit ThreadStore(T val) {
    initStore();

    const int tc = mted ? MAX_JX_THREADS : 1;

    for (int i = 0; i < tc; i++) {
      templates[i] = val;
    }
  }

  // JXcore keeps the thread id inside the isolate data slot
  int getThreadId() {
    const int tid = _threadId();
    if (tid < 0)
      return 0;
    else
      return tid;
  }
};

template <class T>
bool ThreadStore<T>::mted;
}  // namespace jxcore

#endif  // SRC_EXTERNAL_JX_PERSISTENT_STORE_H_
