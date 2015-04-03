// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_THREAD_WRAP_H_
#define SRC_WRAPPERS_THREAD_WRAP_H_

#include "jx/commons.h"

namespace node {

class ThreadWrap {
  static DEFINE_JS_METHOD(CpuCount);

  static DEFINE_JS_METHOD(Free);

  static DEFINE_JS_METHOD(Kill);

  static DEFINE_JS_METHOD(AddTask);

  static DEFINE_JS_METHOD(ShutDown);

  static DEFINE_JS_METHOD(ResetThread);

  static DEFINE_JS_METHOD(SendToThreads);

  static DEFINE_JS_METHOD(GetResults);

  static DEFINE_JS_METHOD(ThreadCount);

  static DEFINE_JS_METHOD(GetCPUCount);

  static DEFINE_JS_METHOD(SetCPUCount);

  static DEFINE_JS_METHOD(SetExiting);

  static DEFINE_JS_METHOD(JobsCount);

  static DEFINE_JS_METHOD(CPU);

 public:
  static JS_HANDLE_VALUE collectResults(node::commons* com, const int tid,
                                        bool emit_call);

  static void EmitOnMessage(const int tid);

  INIT_CLASS_MEMBERS_NO_COM() {
    SET_CLASS_METHOD("addTask", AddTask, 6);
    SET_CLASS_METHOD("resetThread", ResetThread, 1);
    SET_CLASS_METHOD("sendToAll", SendToThreads, 3);
    SET_CLASS_METHOD("getResults", GetResults, 0);
    SET_CLASS_METHOD("jobsCount", JobsCount, 0);
    SET_CLASS_METHOD("setCPUCount", SetCPUCount, 1);
    SET_CLASS_METHOD("getCPUCount", GetCPUCount, 0);
    SET_CLASS_METHOD("threadCount", ThreadCount, 0);
    SET_CLASS_METHOD("setProcessExiting", SetExiting, 2);
    SET_CLASS_METHOD("cpuCount", CpuCount, 0);
    SET_CLASS_METHOD("freeGC", Free, 0);
    SET_CLASS_METHOD("killThread", Kill, 1);
  }
  END_INIT_MEMBERS
};

}  // namespace node
#endif  // SRC_WRAPPERS_THREAD_WRAP_H_
