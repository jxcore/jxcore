// Copyright & License details are available under JXCORE_LICENSE file

#include "memory_store.h"

static uv_mutex_t orstoreLocks, ortimerLocks;
static _StringStore *StringStore;
static _timerStore *TimerStore;
static bool store_init = false;
static bool exiting = false;
static bool hasKey = false;

bool XSpace::StoreInit() {
  if (!store_init) {
    store_init = true;
    return false;
  }
  return true;
}

void XSpace::INITSTORE() {
  if (exiting) return;
  uv_mutex_init(&orstoreLocks);
  StringStore = new _StringStore;
  TimerStore = new _timerStore;
  uv_mutex_init(&ortimerLocks);
}

void XSpace::LOCKSTORE() {
  if (exiting) return;
  uv_mutex_lock(&orstoreLocks);
}
void XSpace::UNLOCKSTORE() {
  if (exiting) return;
  uv_mutex_unlock(&orstoreLocks);
}

void XSpace::LOCKTIMERS() {
  if (exiting) return;
  uv_mutex_lock(&ortimerLocks);
}
void XSpace::UNLOCKTIMERS() {
  if (exiting) return;
  uv_mutex_unlock(&ortimerLocks);
}

void XSpace::DESTROYSTORE() {
  if (!store_init) return;
  uv_mutex_lock(&orstoreLocks);
  exiting = true;
  uv_mutex_unlock(&orstoreLocks);
}

void XSpace::ClearStore() {
  XSpace::LOCKTIMERS();

  // TODO(obastemur) Clear the data allocations for StringStore ?

  StringStore->clear();
  TimerStore->clear();
  delete StringStore;
  delete TimerStore;
  StringStore = NULL;
  TimerStore = NULL;
  store_init = false;
  XSpace::UNLOCKTIMERS();
}

_StringStore *XSpace::Store() { return StringStore; }

_timerStore *XSpace::Timers() { return TimerStore; }

void XSpace::SetHasKey(bool hasIt) { hasKey = hasIt; }

bool XSpace::GetHasKey() { return hasKey; }

void XSpace::ExpirationKick(const char *ckey) {
  if (hasKey) {
    XSpace::LOCKTIMERS();

    if (TimerStore != NULL) {
      std::string key(ckey);
      _timerStore::const_iterator it = TimerStore->find(key);
      if (it != TimerStore->end()) {
        ttlTimer timer = it->second;
        TimerStore->erase(key);
        timer.start = uv_hrtime();
        TimerStore->insert(std::make_pair(key, timer));
      }
    }

    XSpace::UNLOCKTIMERS();
  }
}

void XSpace::ExpirationRemove(const char *ckey) {
  if (hasKey) {
    XSpace::LOCKTIMERS();

    if (TimerStore != NULL) {
      std::string key(ckey);
      _timerStore::const_iterator it = TimerStore->find(key);
      if (it != TimerStore->end()) {
        TimerStore->erase(key);
      }
    }

    XSpace::UNLOCKTIMERS();
  }
}
