// Copyright & License details are available under JXCORE_LICENSE file

#include "node.h"
#include "job_store.h"
#include "extend.h"
#include <stdint.h>
#include <map>
#include <queue>

namespace jxcore {

static long jobs = 0;
class Job;
std::queue<Job*> jobs_queue[2];
static long ops[2] = {0};
std::map<int, Job*> taskDefinitions;
std::map<int, std::queue<int> > threadTaskList;

long getJobCount() { return jobs; }

long increaseJobCount() {
  auto_lock locker_(CSLOCK_JBEND);
  jobs++;
  return jobs;
}

long decreaseJobCount() {
  auto_lock locker_(CSLOCK_JBEND);
  jobs--;
  return jobs;
}

void addNewJob(const int m, Job* j) {
  auto_lock locker_(CSLOCK_JOBS + (m));
  jobs_queue[m].push(j);
  ops[m]++;
}

Job* getJob(const int n) {
  auto_lock locker_(CSLOCK_JOBS + n);
  if (ops[n] == 0) {
    return NULL;
  }
  ops[n]--;

  Job* j = jobs_queue[n].front();
  jobs_queue[n].pop();

  return j;
}

Job* getTaskDefinition(const int n) { return taskDefinitions[n]; }

void Job::getTasks(std::queue<int>* tasks, int threadId) {
  auto_lock locker_(CSLOCK_TASKS);

  if (node::commons::process_status_ != node::JXCORE_INSTANCE_ALIVE) return;

  if (threadTaskList[threadId].empty()) {
    return;
  }

  while (!threadTaskList[threadId].empty()) {
    tasks->push(threadTaskList[threadId].front());
    threadTaskList[threadId].pop();
  }
}

int Job::getNewThreadId() {
bas:
  int q = 0;
  int tc = getThreadCount();
  customLock(CSLOCK_TASKS);
  if (node::commons::process_status_ == node::JXCORE_INSTANCE_ALIVE) {
    for (; q < tc; q++) {
      std::map<int, std::queue<int> >::iterator it = threadTaskList.find(q);
      if (it == threadTaskList.end()) break;
    }

    if (tc == q) {
      customUnlock(CSLOCK_TASKS);
      goto bas;
    }

    threadTaskList[q] = std::queue<int>();
  }
  customUnlock(CSLOCK_TASKS);

  return q;
}

void Job::fillTasks(int threadId) {
  auto_lock locker_(CSLOCK_TASKS);
  if (node::commons::process_status_ != node::JXCORE_INSTANCE_ALIVE) return;

  std::map<int, std::queue<int> >::iterator it = threadTaskList.find(threadId);
  if (it != threadTaskList.end()) {
    while (!threadTaskList[threadId].empty()) {
      threadTaskList[threadId].pop();
    }
    for (std::map<int, Job*>::iterator it = taskDefinitions.begin();
         it != taskDefinitions.end(); ++it) {
      if (!it->second->notRemember) {
        threadTaskList[threadId].push(it->first);
      }
    }
  }
}

void Job::removeTasker(int threadId) {
  auto_lock locker_(CSLOCK_TASKS);
  if (node::commons::process_status_ != node::JXCORE_INSTANCE_ALIVE) return;

  std::map<int, std::queue<int> >::iterator it = threadTaskList.find(threadId);
  if (it != threadTaskList.end()) {
    threadTaskList.erase(it);
  }
}

// use this before destroying the jx_locks
// this method is not thread safe!
void Job::removeTaskers() {
  jobs = 0;
  ops[0] = 0;
  ops[1] = 0;
  threadTaskList.clear();
  clearTaskDefinitions();
  taskDefinitions.clear();

  for (int i = 0; i < 2; i++) {
    while (!jobs_queue[i].empty()) {
      Job* jb = jobs_queue[i].front();
      delete jb;
      jobs_queue[i].pop();
    }
  }
}

// not thread safe!!
void Job::clearTaskDefinitions() {
  for (std::map<int, Job*>::iterator it = taskDefinitions.begin();
       it != taskDefinitions.end(); ++it) {
    free(it->second->script);
  }
  taskDefinitions.clear();
}

Job::Job(const char* scr, const int scrlen, const char* pr, const int paramlen,
         const int task_id, const int cb_id, bool notRem) {
  taskId = task_id;
  cbId = cb_id;
  notRemember = notRem;
  hasParam = paramlen > 0;
  hasScript = scrlen > 0;
  if (hasParam) {
    param = cpystr(pr, paramlen);
  } else {
    param = NULL;
  }

  disposed = false;

  if (hasScript) {
    assert(scr != NULL);

    script = cpystr(scr, scrlen);

    {
      auto_lock locker_(CSLOCK_TASKS);
      for (std::map<int, std::queue<int> >::iterator it =
               threadTaskList.begin();
           it != threadTaskList.end(); ++it) {
        it->second.push(taskId);
      }

      taskDefinitions[taskId] = this;
    }
  } else {
    script = NULL;
  }
}

void Job::Dispose() {
  assert(!disposed);
  disposed = true;
  if (param != NULL) {
    free(param);
    param = NULL;
  }
}
}  // namespace jxcore
