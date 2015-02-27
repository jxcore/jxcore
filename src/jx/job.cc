// Copyright & License details are available under JXCORE_LICENSE file

#include "job.h"
#include "commons.h"
#include "extend.h"
#include "jx_instance.h"
#include "wrappers/thread_wrap.h"
#if defined(JS_ENGINE_MOZJS)
#include "vm/PosixNSPR.h"
#endif

namespace jxcore {

void SendMessage(const int threadId, const char *msg_data, const int length,
                 bool same_thread) {
  char *str = cpystr(msg_data, length);

  bool hasIt = false;
  threadLock(threadId);
  pushThreadQueue(threadId, str);

  // check if thread already received a ping
  if (threadHasMessage(threadId)) hasIt = true;

  setThreadMessage(threadId, 1);
  threadUnlock(threadId);

  node::commons *com = node::commons::getInstanceByThreadId(threadId);
  if (com == NULL || com->instance_status_ != node::JXCORE_INSTANCE_ALIVE ||
      com->expects_reset)
    return;

  if (!hasIt) com->PingThread();
}

void CreateThread(void (*entry)(void *arg), void *param) {
#ifdef JS_ENGINE_V8
  uv_thread_t thread;
  int rc = uv_thread_create(&thread, entry, param);
  assert(rc == 0);
#elif defined(JS_ENGINE_MOZJS)
  PR_CreateThread(PR_USER_THREAD, entry, param,
                  PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD,
                  512 * 1024);
#endif
}

int CreateInstances(const int count) {
  int rc = 0;
  for (int i = 0; i < count; i++) {
#ifdef JS_ENGINE_V8
    uv_thread_t thread;
    rc = uv_thread_create(&thread, JXInstance::runScript, NULL);
    if (rc != 0) break;
#elif defined(JS_ENGINE_MOZJS)
    PR_CreateThread(PR_USER_THREAD, JXInstance::runScript, NULL,
                    PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD,
                    512 * 1024);
#endif
  }

  return rc;
}

}  // namespace jxcore
