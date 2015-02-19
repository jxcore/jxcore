// Copyright & License details are available under JXCORE_LICENSE file

#include "thread_wrap.h"
#include "jx/job_store.h"
#include "jx/job.h"
#include "jx/memory_store.h"
#include "jx/extend.h"

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x) * 1000)
#endif

#include <string>
#include <iostream>

namespace node {

JS_METHOD(ThreadWrap, CpuCount) {
  uv_cpu_info_t *cpu_infos;
  int count;

  uv_err_t er = uv_cpu_info(&cpu_infos, &count);
  if (er.code != UV_OK) RETURN();

  uv_free_cpu_info(cpu_infos, count);

  RETURN_PARAM(STD_TO_INTEGER(count));
}
JS_METHOD_END

JS_METHOD(ThreadWrap, Free) { JS_FORCE_GC(); }
JS_METHOD_END

#define CHECK_EMBEDDED_THREADS()                                              \
  if (node::commons::embedded_multithreading_) {                              \
    THROW_EXCEPTION(                                                          \
        "Multithreading is already enabled for embedding interface, you can " \
        "not use both");                                                      \
  }

JS_METHOD(ThreadWrap, Kill) {
  CHECK_EMBEDDED_THREADS()

  if (!args.IsInteger(0)) {
    THROW_EXCEPTION("Missing parameters (killThread) expects (int).");
  }

  node::commons *t_com =
      node::commons::getInstanceByThreadId(args.GetInteger(0) + 1);

  if (t_com == NULL) RETURN_PARAM(STD_TO_BOOLEAN(false));

  t_com->expects_reset = true;
  if (!args.IsBoolean(1) || !args.GetBoolean(1))
    JS_TERMINATE_EXECUTION(t_com->threadId);
  uv_stop(t_com->loop);

  RETURN_PARAM(STD_TO_BOOLEAN(true));
}
JS_METHOD_END

JS_METHOD(ThreadWrap, ShutDown) { uv_stop(com->loop); }
JS_METHOD_END

JS_METHOD(ThreadWrap, ResetThread) {
  CHECK_EMBEDDED_THREADS()

  if (!checkIncreaseThreadCount(1)) {
    RETURN_PARAM(STD_TO_INTEGER(0));
  }

  int rc = jxcore::CreateInstances(1);

  RETURN_PARAM(STD_TO_INTEGER(rc));
}
JS_METHOD_END

JS_METHOD(ThreadWrap, GetCPUCount) {
  CHECK_EMBEDDED_THREADS()

  RETURN_PARAM(STD_TO_INTEGER(node::commons::threadPoolCount));
}
JS_METHOD_END


JS_METHOD(ThreadWrap, ThreadCount) {
  CHECK_EMBEDDED_THREADS()

  RETURN_PARAM(STD_TO_INTEGER(getThreadCount()));
}
JS_METHOD_END


JS_METHOD(ThreadWrap, SetExiting) {
  CHECK_EMBEDDED_THREADS()

  node::commons::process_status_ = JXCORE_INSTANCE_EXITING;
}
JS_METHOD_END

JS_METHOD(ThreadWrap, JobsCount) {
  CHECK_EMBEDDED_THREADS()

  auto_lock locker_(CSLOCK_JBEND);
  int count = jxcore::getJobCount();

  RETURN_PARAM(STD_TO_INTEGER(count));
}
JS_METHOD_END

JS_METHOD(ThreadWrap, AddTask) {
  CHECK_EMBEDDED_THREADS()
  static int nth = 1;

  // taskId, method, param, cbId, notRemember
  if (!args.IsInteger(0) || !args.IsStringOrNull(1) ||
      !args.IsStringOrNull(2) || !args.IsInteger(3) ||
      !args.IsBooleanOrNull(4)) {
    THROW_TYPE_EXCEPTION(
        "Missing parameters (addTask) expects (int, string, string, int, "
        "boolean).");
  }

  bool skip_thread_creation = args.IsBoolean(5) ? args.GetBoolean(5) : false;

  int taskId = args.GetInteger(0);
  int mlen = -1, plen = -1;
  jxcore::JXString strMethod;
  jxcore::JXString strParam;

  if (!args.IsNull(1)) {
    mlen = args.GetString(1, &strMethod);
  }

  if (!args.IsNull(2)) {
    plen = args.GetString(2, &strParam);
  }

  int cbId = args.GetInteger(3);
  bool notRemember = args.GetBoolean(4);

  bool hasOps = false, newJob = false;
  int openThreads = 0;

  if (taskId == -1) {  // reset_thread
    if (!checkIncreaseThreadCount(1)) {
      RETURN_PARAM(STD_TO_INTEGER(0));
    }
    openThreads = 1;
  } else {
    jxcore::Job *j = new jxcore::Job(*strMethod, mlen, *strParam, plen, taskId,
                                     cbId, notRemember);

    if (cbId != -2) {
      const int m = nth % 2;  // JBEND
      nth %= 10000;
      nth++;

      jxcore::addNewJob(m, j);
      hasOps = jxcore::increaseJobCount() > 0;
    } else {
      hasOps = true;
      openThreads = getIncreaseThreadCount();
    }

    newJob = true;
  }

  int rc = 0;
  if (openThreads > 0 && !skip_thread_creation) {
    rc = jxcore::CreateInstances(node::commons::threadPoolCount);
  }

  for (int i = 1; i <= node::commons::threadPoolCount; i++) {  // +1 for main
    jxcore::SendMessage(i, "null", 4, false);
  }

  RETURN_PARAM(STD_TO_INTEGER(rc));
}
JS_METHOD_END

JS_HANDLE_VALUE ThreadWrap::collectResults(node::commons *com, const int tid,
                                           bool emit_call) {
  JS_ENTER_SCOPE();
  JS_DEFINE_STATE_MARKER(com);
  int i = 0;

  JS_LOCAL_ARRAY arr = JS_NEW_ARRAY();

  threadLock(tid);
  while (!IsThreadQueueEmpty(tid)) {
    char *str = pullThreadQueue(tid);
    if (str != NULL) {
      JS_INDEX_SET(arr, i++, UTF8_TO_STRING(str));
      free(str);
    }
  }
  if (emit_call) setThreadMessage(tid, 0);
  threadUnlock(tid);

  return JS_LEAVE_SCOPE(arr);
}

JS_METHOD(ThreadWrap, SendToThreads) {
  CHECK_EMBEDDED_THREADS()
  if (!args.IsNumber(0) || !args.IsString(1) || !args.IsNumber(2)) {
    THROW_EXCEPTION(
        "Missing parameters (sendToAll) expects (int, string, int).");
  }

  if (args.Length() == 3) {
    int targetThreadId = args.GetInteger(0);
    int myThreadId = args.GetInteger(2) + 1;  // js side starts from -1

    jxcore::JXString str;
    int str_len = args.GetString(1, &str);

    if (str_len > 0) {
      if (targetThreadId > -2) {
        targetThreadId++;  // js side starts from -1
        jxcore::SendMessage(targetThreadId, *str, str_len,
                            myThreadId == targetThreadId);
      } else {
        for (int i = 1; i <= node::commons::threadPoolCount;
             i++) {  // +1 for main
          jxcore::SendMessage(i, *str, str_len, myThreadId == i);
        }
      }
    }
  }
}
JS_METHOD_END

JS_METHOD(ThreadWrap, GetResults) {
  CHECK_EMBEDDED_THREADS()
  RETURN_PARAM(collectResults(com, 0, false));
}
JS_METHOD_END

JS_METHOD(ThreadWrap, SetCPUCount) {
  CHECK_EMBEDDED_THREADS()
  if (node::commons::threadPoolCount > 0) {
    RETURN_PARAM(STD_TO_INTEGER(0));
  }

  if (!args.IsNumber(0)) {
    THROW_EXCEPTION("Missing parameters (setCPUCount) expects (int).");
  }

  const int no = args.GetInteger(0);
  node::commons::threadPoolCount = no - 1;
  if (node::commons::threadPoolCount <= 1) {
    node::commons::threadPoolCount = 2;
  }

#ifndef _MSC_VER
  if (com->threadPing == NULL) {  // checks if its embedded
    com->threadPing = new uv_async_t;
    uv_async_init(com->loop, com->threadPing, node::commons::TriggerDummy);
    com->loop->fakeHandle = 1;
    com->threadPing->threadId = 0;
  }
#endif

  if (node::commons::threadPoolCount > MAX_JX_THREADS) {
    node::commons::threadPoolCount = MAX_JX_THREADS;
  }

  customLock(CSLOCK_JBEND);
  if (node::commons::mapCount < node::commons::threadPoolCount + 1) {
    for (int i = node::commons::mapCount;
         i < node::commons::threadPoolCount + 1; i++) {
      node::commons::mapData[i] = new BTStore;
    }
  }

  node::commons::mapCount = node::commons::threadPoolCount + 1;
  customUnlock(CSLOCK_JBEND);

  setThreadCount(node::commons::threadPoolCount);

  const int rc = jxcore::CreateInstances(node::commons::threadPoolCount);

  RETURN_PARAM(STD_TO_INTEGER(rc));
}
JS_METHOD_END

void ThreadWrap::EmitOnMessage(const int tid) {
  node::commons *com = node::commons::getInstanceByThreadId(tid);

  if (com->threadOnHold) return;

  JS_ENTER_SCOPE();
  JS_HANDLE_OBJECT process_object = com->getProcess();

  JS_LOCAL_OBJECT vals =
      JS_VALUE_TO_OBJECT(ThreadWrap::collectResults(com, tid, true));

#ifdef JS_ENGINE_MOZJS
  __JS_LOCAL_VALUE args[2] = {JS_CORE_REFERENCE(com->pstr_threadMessage),
                              JS_CORE_REFERENCE(vals)};
#elif defined(JS_ENGINE_V8)
  // both works for MozJS but above is faster
  __JS_LOCAL_VALUE args[2] = {*com->pstr_threadMessage, vals};
#endif

  MakeCallback(com, process_object, JS_PREDEFINED_STRING(emit), 2, args);
}

}  // namespace node

NODE_MODULE(node_thread_wrap, node::ThreadWrap::Initialize)
