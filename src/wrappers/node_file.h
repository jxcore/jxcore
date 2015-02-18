// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_NODE_FILE_H_
#define SRC_WRAPPERS_NODE_FILE_H_

#include "node.h"
#include "node_stat_watcher.h"

namespace node {

class File {
 private:
  static DEFINE_JS_METHOD(Close);
  static DEFINE_JS_METHOD(Open);
  static DEFINE_JS_METHOD(Read);
  static DEFINE_JS_METHOD(Fdatasync);
  static DEFINE_JS_METHOD(Fsync);
  static DEFINE_JS_METHOD(Rename);
  static DEFINE_JS_METHOD(FTruncate);
  static DEFINE_JS_METHOD(RMDir);
  static DEFINE_JS_METHOD(MKDir);
  static DEFINE_JS_METHOD(ReadDir);
  static DEFINE_JS_METHOD(Stat);
  static DEFINE_JS_METHOD(LStat);
  static DEFINE_JS_METHOD(FStat);
  static DEFINE_JS_METHOD(Link);
  static DEFINE_JS_METHOD(Symlink);
  static DEFINE_JS_METHOD(ReadLink);
  static DEFINE_JS_METHOD(Unlink);
  static DEFINE_JS_METHOD(Write);
  static DEFINE_JS_METHOD(Chmod);
  static DEFINE_JS_METHOD(FChmod);
  static DEFINE_JS_METHOD(Chown);
  static DEFINE_JS_METHOD(FChown);
  static DEFINE_JS_METHOD(UTimes);
  static DEFINE_JS_METHOD(FUTimes);

  INIT_CLASS_MEMBERS() {
    JS_LOCAL_FUNCTION_TEMPLATE stat_templ = JS_NEW_EMPTY_FUNCTION_TEMPLATE();
    com->nf_stats_constructor_template =
        JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(stat_templ);

#ifdef JS_ENGINE_V8
    JS_NAME_SET(constructor, JS_STRING_ID("Stats"),
                JS_GET_FUNCTION(com->nf_stats_constructor_template));
#else
    JS_NAME_SET(constructor, JS_STRING_ID("Stats"),
                com->nf_stats_constructor_template.GetConstructor());
#endif

    SET_CLASS_METHOD("close", Close, 2);
    SET_CLASS_METHOD("open", Open, 3);
    SET_CLASS_METHOD("read", Read, 2);
    SET_CLASS_METHOD("fdatasync", Fdatasync, 2);
    SET_CLASS_METHOD("fsync", Fsync, 2);
    SET_CLASS_METHOD("rename", Rename, 3);
    SET_CLASS_METHOD("ftruncate", FTruncate, 3);
    SET_CLASS_METHOD("rmdir", RMDir, 2);
    SET_CLASS_METHOD("mkdir", MKDir, 3);
    SET_CLASS_METHOD("readdir", ReadDir, 2);
    SET_CLASS_METHOD("stat", Stat, 2);
    SET_CLASS_METHOD("lstat", LStat, 2);
    SET_CLASS_METHOD("fstat", FStat, 2);
    SET_CLASS_METHOD("link", Link, 3);
    SET_CLASS_METHOD("symlink", Symlink, 4);
    SET_CLASS_METHOD("readlink", ReadLink, 2);
    SET_CLASS_METHOD("unlink", Unlink, 2);
    SET_CLASS_METHOD("write", Write, 6);

    SET_CLASS_METHOD("chmod", Chmod, 3);
    SET_CLASS_METHOD("fchmod", FChmod, 3);

    SET_CLASS_METHOD("chown", Chown, 4);
    SET_CLASS_METHOD("fchown", FChown, 4);

    SET_CLASS_METHOD("utimes", UTimes, 4);
    SET_CLASS_METHOD("futimes", FUTimes, 4);

    StatWatcher::Initialize(constructor);
  }
  END_INIT_MEMBERS
};

void InitFs(JS_HANDLE_OBJECT_REF target);

}  // namespace node
#endif  // SRC_WRAPPERS_NODE_FILE_H_
