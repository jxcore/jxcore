// Copyright & License details are available under JXCORE_LICENSE file

#include "commons.h"
#include "job.h"
#include "extend.h"
#include "jx_instance.h"
#include "wrappers/thread_wrap.h"
#if defined(JS_ENGINE_MOZJS)
#if !defined(_MSC_VER)
#include "vm/PosixNSPR.h"
#else
#include "../deps/mozjs/incs/nss/nspr/pr/include/prthread.h"
#endif
#endif

namespace jxcore {

void SendMessage(const int threadId, const char *msg_data_, const int length,
                 bool same_thread) {
  const char *msg_data = msg_data_ == NULL ? "null" : msg_data_;
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

#ifdef JS_ENGINE_V8
int CreateThread(void (*entry)(void *arg), void *param) {
  uv_thread_t thread;
  return uv_thread_create(&thread, entry, param);
}
#elif defined(JS_ENGINE_MOZJS)
void *CreateThread(void (*entry)(void *arg), void *param) {
  void * PR_thread = (void *)PR_CreateThread(PR_USER_THREAD, entry, param,
                                 PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                                 PR_JOINABLE_THREAD, 0);

  assert(PR_thread != NULL && "couldn't create the thread");
  return PR_thread;
}

bool JoinThread(void *pth) {
  return PR_JoinThread((PRThread *)pth) == PR_SUCCESS;
}
#endif

int CreateInstances(const int count) {
  int rc = 0;
  for (int i = 0; i < count; i++) {
#ifdef JS_ENGINE_V8
    uv_thread_t thread;
    rc = uv_thread_create(&thread, JXInstance::runScript, NULL);
    if (rc != 0) break;
#elif defined(JS_ENGINE_MOZJS)
    if (CreateThread(JXInstance::runScript, NULL) == NULL) {
      rc = -1;
      break;
    }
#endif
  }

  return rc;
}

}  // namespace jxcore
