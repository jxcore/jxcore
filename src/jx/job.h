// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_JOB_H_
#define SRC_JX_JOB_H_

#ifndef JXCORE_EXTERN
#ifdef _WIN32
#define JXCORE_EXTERN(x) __declspec(dllexport) x
#else
#define JXCORE_EXTERN(x) x
#endif
#endif

namespace jxcore {
void SendMessage(const int threadId, const char *msg_data, const int length,
                 bool same_thread);

int CreateInstances(const int count);

#ifdef JS_ENGINE_V8
// TODO(obastemur) implement libuv thread join
JXCORE_EXTERN(int)
#elif defined(JS_ENGINE_MOZJS)
JXCORE_EXTERN(bool) JoinThread(void *pth);

JXCORE_EXTERN(void *)
#endif
    CreateThread(void (*entry)(void *arg), void *param);
}

#endif  // SRC_JX_JOB_H_
