// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_EXTEND_H_
#define SRC_JX_EXTEND_H_

#include <queue>
#include "commons.h"
#include "memory_store.h"

// customLock / customUnlock definitions
#define CUSTOMLOCKSCOUNT 15
#define CSLOCK_TCP 0
#define CSLOCK_TRIGGER 1
#define CSLOCK_THREADCOUNT 2
#define CSLOCK_THREADMESSAGE 3
#define CSLOCK_CRYPTO 4
#define CSLOCK_UNIQUEID 5
#define CSLOCK_TASKS 6
#define CSLOCK_MAPS 7
#define CSLOCK_JBEND 8
#define CSLOCK_NEWINSTANCE 9
#define CSLOCK_UVFS 10
#define CSLOCK_JOBS 11  // && 12 (0,1)
#define CSLOCK_RESULTS 13
#define CSLOCK_COMPRESS 14

extern void customLock(const int n);
extern void customUnlock(const int n);

int AddExternalMethod(const char* name, JS_NATIVE_METHOD method);
void ClearExternalMethods();

JS_NATIVE_METHOD GetExternalMethodTarget(const int id);
const char* GetExternalMethodName(const int id);
int GetExternalMethodCount();

class auto_lock {
  int lock_id_;

 public:
  explicit auto_lock(int lock_id) : lock_id_(lock_id) { customLock(lock_id_); }
  void unlock() {
    if (lock_id_ >= 0) {
      customUnlock(lock_id_);
      lock_id_ = -1;
    }
  }
  ~auto_lock() { unlock(); }
};

extern void threadLock(const int n);
extern void threadUnlock(const int n);

int getThreadCount();
void setThreadCount(const int count);
void increaseThreadCount();
void reduceThreadCount();
bool checkIncreaseThreadCount(const int count);
int getIncreaseThreadCount();

void jx_init_locks();
void jx_destroy_locks();

bool IsThreadQueueEmpty(const int tid);
char* pullThreadQueue(const int tid);
void pushThreadQueue(const int tid, char* str);
char* cpystr(const char* src, const int ln);

#endif  // SRC_JX_EXTEND_H_
