// Copyright & License details are available under JXCORE_LICENSE file

#include "extend.h"

using node::commons;

uv_mutex_t customLocks[CUSTOMLOCKSCOUNT];
static std::queue<char *> threadQueue[MAX_JX_THREADS + 1];  // +1 for main
                                                            // thread
static std::map<int, JS_NATIVE_METHOD> external_methods;
static std::map<int, char *> external_method_names;
static int external_methods_count = 0;
uv_mutex_t threadLocks[MAX_JX_THREADS + 1];
static int threadCount = 0;

int GetExternalMethodCount() { return external_methods_count; }

JS_NATIVE_METHOD GetExternalMethodTarget(const int id) {
  assert(id >= 0 && id < external_methods_count);
  return external_methods[id];
}

const char *GetExternalMethodName(const int id) {
  assert(id >= 0 && id < external_methods_count);
  return external_method_names[id];
}

int AddExternalMethod(const char *name, JS_NATIVE_METHOD method) {
  external_methods_count++;
  external_methods[external_methods_count - 1] = method;
  char *_name = strdup(name);

  external_method_names[external_methods_count - 1] = _name;
  return external_methods_count - 1;
}

void ClearExternalMethods() {
  external_methods_count = 0;
  external_methods.clear();
  std::map<int, char *>::iterator it = external_method_names.begin();
  for (; it != external_method_names.end(); it++) {
    free(it->second);
  }
  external_method_names.clear();
}

void customLock(const int n) {
  if (n < CUSTOMLOCKSCOUNT) uv_mutex_lock(&customLocks[n]);
}

void customUnlock(const int n) {
  if (n < CUSTOMLOCKSCOUNT) uv_mutex_unlock(&customLocks[n]);
}

void threadLock(const int n) {
  if (n < MAX_JX_THREADS + 1) uv_mutex_lock(&threadLocks[n]);
}

void threadUnlock(const int n) {
  if (n < MAX_JX_THREADS + 1) uv_mutex_unlock(&threadLocks[n]);
}

bool IsThreadQueueEmpty(const int tid) {  // reader reverse!
  return threadQueue[tid].empty();
}

char *pullThreadQueue(const int tid) {  // reader reverse!
  char *str = threadQueue[tid].front();
  threadQueue[tid].pop();

  return str;
}

void pushThreadQueue(const int tid, char *str) { threadQueue[tid].push(str); }

char* cpystr(const char *src, const int ln) {
  char *dest;
  if (src != NULL) {
    dest = (char *)malloc(sizeof(char) * (ln + 1));
    memcpy(dest, src, ln);
    *(dest + ln) = '\0';
  } else {
    dest = NULL;
  }

  return dest;
}

int getThreadCount() {
  auto_lock locker_(CSLOCK_THREADCOUNT);
  return threadCount;
}

void setThreadCount(const int count) {
  auto_lock locker_(CSLOCK_THREADCOUNT);
  threadCount = count;
}

void increaseThreadCount() {
  auto_lock locker_(CSLOCK_THREADCOUNT);
  threadCount++;
}

void reduceThreadCount() {
  auto_lock locker_(CSLOCK_THREADCOUNT);
  threadCount--;
}

bool checkIncreaseThreadCount(const int count) {
  bool success = true;
  auto_lock locker_(CSLOCK_THREADCOUNT);
  if (threadCount + count <= commons::threadPoolCount)
    threadCount += count;
  else
    success = false;

  return success;
}

int getIncreaseThreadCount() {
  auto_lock locker_(CSLOCK_THREADCOUNT);
  const int count = commons::threadPoolCount - threadCount;
  threadCount = commons::threadPoolCount;

  return count;
}

#ifdef SIGPIPE
#define do_pipe_sig() signal(SIGPIPE, SIG_IGN)
#else
#define do_pipe_sig()
#endif

static bool store_inited = false;

void jx_init_locks() {
  if (store_inited) return;
  store_inited = true;
  do_pipe_sig();

  for (int i = 0; i < CUSTOMLOCKSCOUNT; i++) uv_mutex_init(&customLocks[i]);

  for (int i = 0; i < MAX_JX_THREADS + 1; i++) uv_mutex_init(&threadLocks[i]);

  if (!XSpace::StoreInit()) {
    XSpace::INITSTORE();
  }
}

void jx_destroy_locks() {
  if (!store_inited) return;
  store_inited = false;

  for (int i = 0; i < CUSTOMLOCKSCOUNT; i++) uv_mutex_destroy(&customLocks[i]);

  for (int i = 0; i < MAX_JX_THREADS + 1; i++)
    uv_mutex_destroy(&threadLocks[i]);

  if (XSpace::Store() != NULL) {
    XSpace::LOCKSTORE();
    XSpace::ClearStore();
    XSpace::UNLOCKSTORE();
    XSpace::DESTROYSTORE();
  }
}
