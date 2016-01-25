// Copyright & License details are available under JXCORE_LICENSE file

#include "jxtimers_wrap.h"
#include "jx/extend.h"
#include "jx/memory_store.h"

#include <stdio.h>
#include <string>
#include <iostream>
#if defined(_MSC_VER)
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif
#include "jx/jxp_compress.h"
#include "node_buffer.h"

namespace node {

static bool watcher_alive_ = false;

void JXTimersWrap::checkKeys() {
  std::queue<std::string> todelete;

  if (XSpace::Timers() == NULL) return;

  XSpace::LOCKTIMERS();
  _TimerStore *timers = XSpace::Timers();
  if (timers != NULL) {
    _TimerStore::iterator it = timers->begin();
    uint64_t total = uv_hrtime();
    long counter = 0;

    while (it != timers->end()) {
      const ttlTimer &timer = it->second;
      if (node::commons::process_status_ != JXCORE_INSTANCE_ALIVE) break;

      if (timer.start + timer.slice < total) {
        todelete.push(it->first);
#ifdef HAS_BTREE_MAP
        it = timers->erase(it);
#else
        timers->erase(it++);
#endif
      } else {
        ++it;
        counter++;
      }
    }

    if (counter == 0) {
      XSpace::SetHasKey(false);
    }
  }
  XSpace::UNLOCKTIMERS();
  if (!todelete.empty()) {
    XSpace::LOCKSTORE();
    while (!todelete.empty()) {
      XSpace::Store()->erase(todelete.front());
      todelete.pop();
    }
    XSpace::UNLOCKSTORE();
  }
}

void JXTimersWrap::Watcher(void *w) {
  int timer = 25;
  int counter = 0;
  while (true) {
    commons::CheckMemoryLimit();

    if (XSpace::GetHasKey()) {
      checkKeys();
    }

    if (node::commons::process_status_ != JXCORE_INSTANCE_ALIVE) break;

    if (commons::GetMaxCPU() > 0 || commons::GetMaxCPU() < 10000) {
      commons::CheckCPUUsage(timer);
    } else {
      if (!XSpace::GetHasKey()) {
        counter++;
        if (counter > 100) {
          XSpace::LOCKTIMERS();
          watcher_alive_ = false;
          XSpace::UNLOCKTIMERS();
          return;
        }
      } else {
        counter = 0;
      }
    }

    Sleep(timer);
  }
}

JS_METHOD_NO_COM(JXTimersWrap, StartWatcher) {
  bool was_alive = watcher_alive_;
  XSpace::LOCKTIMERS();
  if (!watcher_alive_) {
    watcher_alive_ = true;
    uv_thread_t thread;
    uv_thread_create(&thread, Watcher, NULL);
  }
  XSpace::UNLOCKTIMERS();

  RETURN_PARAM(STD_TO_BOOLEAN(was_alive));
}
JS_METHOD_END

JS_METHOD_NO_COM(JXTimersWrap, ForceCheckKeys) {
  if (XSpace::GetHasKey()) {
    checkKeys();
  }
}
JS_METHOD_END

}  // namespace node

NODE_MODULE(node_jxtimers_wrap, node::JXTimersWrap::Initialize)
