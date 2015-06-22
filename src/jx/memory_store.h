// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_MEMORY_STORE_H_
#define SRC_JX_MEMORY_STORE_H_

#include <stdlib.h>
#include <string.h>
#if !defined(_MSC_VER)
#include <strings.h>
#else
#define strcasecmp _stricmp
#endif

#ifndef __IOS__
#include "btree_map.h"
#define MAP_HOST btree::btree_map
#define HAS_BTREE_MAP
#else
#include <map>
#define MAP_HOST std::map
#endif

struct ttlTimer {
  uint64_t slice;
  uint64_t start;
};

#define EXTERNAL_DATA_STRING 1
#define EXTERNAL_DATA_TIMER 2

struct externalData {
  std::string str_data;
  double number_data;
  int timer_data;
  int type;
};

typedef MAP_HOST<std::string, externalData*> _StringStore;
typedef MAP_HOST<std::string, ttlTimer> _TimerStore;

class XSpace {
 public:
  static bool StoreInit();
  static void INITSTORE();
  static void LOCKSTORE();
  static void UNLOCKSTORE();
  static void LOCKTIMERS();
  static void UNLOCKTIMERS();
  static void DESTROYSTORE();
  static void ClearStore();
  static _StringStore* Store();
  static _TimerStore* Timers();
  static void ExpirationKick(const char* key);
  static void ExpirationRemove(const char* key);
  static void SetHasKey(bool hasIt);
  static bool GetHasKey();
};

#endif  // SRC_JX_MEMORY_STORE_H_
