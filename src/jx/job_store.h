// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_JOB_STORE_H_
#define SRC_JX_JOB_STORE_H_

#include <stdlib.h>
#include <queue>

namespace jxcore {

class Job {
 public:
  static void getTasks(std::queue<int> *tasks, int threadId);

  static int getNewThreadId();

  static void fillTasks(int threadId);

  static void removeTasker(int threadId);

  static void removeTaskers();

  static void clearTaskDefinitions();

  Job(const char *scr, const int scrlen, const char *pr, const int paramlen,
      const int task_id, const int cb_id, bool notRem);

  bool notRemember;
  int taskId;
  bool hasParam, hasScript;
  char *param;
  char *script;
  int cbId;
  bool disposed;

  void Dispose();
};

Job *getJob(const int n);
void addNewJob(const int m, Job *j);

long getJobCount();
long increaseJobCount();
long decreaseJobCount();

Job *getTaskDefinition(const int n);
}  // namespace jxcore

#endif  // SRC_JX_JOB_STORE_H_
