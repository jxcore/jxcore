// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_MEMORY_WRAP_H_
#define SRC_WRAPPERS_MEMORY_WRAP_H_

#include "jx/commons.h"

namespace node {

class MemoryWrap {
 public:
  static void SharedSet(const char *name, const char *value);
  static void MapClear(const bool clear_blocks);

 private:
  static DEFINE_JS_METHOD(SetCPUCountMap);

  static DEFINE_JS_METHOD(MapGet);

  static DEFINE_JS_METHOD(MapExist);

  static DEFINE_JS_METHOD(MapRead);

  static DEFINE_JS_METHOD(MapRemove);

  static DEFINE_JS_METHOD(MapSet);

  static DEFINE_JS_METHOD(SourceSetIfNotExists);

  static DEFINE_JS_METHOD(SourceSetIfEq);

  static DEFINE_JS_METHOD(SourceSetIfEqOrNull);

  static DEFINE_JS_METHOD(SourceSet);

  static DEFINE_JS_METHOD(SourceExist);

  static DEFINE_JS_METHOD(SourceRead);

  static DEFINE_JS_METHOD(SourceRemove);

  static DEFINE_JS_METHOD(SourceGet);

  static DEFINE_JS_METHOD(ReadEmbeddedSource);

  INIT_CLASS_MEMBERS() {
    SET_CLASS_METHOD("readEmbeddedSource", ReadEmbeddedSource, 0);
    SET_CLASS_METHOD("setMapCount", SetCPUCountMap, 1);
    SET_CLASS_METHOD("setMap", MapSet, 3);
    SET_CLASS_METHOD("getMap", MapGet, 2);
    SET_CLASS_METHOD("readMap", MapRead, 2);
    SET_CLASS_METHOD("existMap", MapExist, 2);
    SET_CLASS_METHOD("removeMap", MapRemove, 2);

    SET_CLASS_METHOD("setSource", SourceSet, 2);
    SET_CLASS_METHOD("setSourceIfNotExists", SourceSetIfNotExists, 2);
    SET_CLASS_METHOD("setSourceIfEqualsTo", SourceSetIfEq, 3);
    SET_CLASS_METHOD("setSourceIfEqualsToOrNull", SourceSetIfEqOrNull, 2);
    SET_CLASS_METHOD("readSource", SourceRead, 1);
    SET_CLASS_METHOD("removeSource", SourceRemove, 1);
    SET_CLASS_METHOD("getSource", SourceGet, 1);
    SET_CLASS_METHOD("existsSource", SourceExist, 1);
  }
  END_INIT_MEMBERS
};

}  // namespace node
#endif  // SRC_WRAPPERS_MEMORY_WRAP_H_
