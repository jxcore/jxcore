// Copyright & License details are available under JXCORE_LICENSE file

#include "handle_wrap.h"
#include "jx/commons.h"

#include <stdio.h>
#include <string>
#include <iostream>
#ifdef JXCORE_EMBEDS_SQLITE
#include "sqlite3/module_externs.h"
#endif

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x) * 1000)
#endif
#ifdef JXCORE_EMBEDS_LEVELDOWN
#include "../../deps/leveldown-mobile/src/leveldown_public.h"
#endif

namespace node {

class ModuleWrap {
 public:
  INIT_CLASS_MEMBERS() { SET_CLASS_METHOD("loadInternal", LoadInternal, 0); }
  END_INIT_MEMBERS

 private:
  static DEFINE_JS_METHOD(LoadInternal);
};

JS_METHOD(ModuleWrap, LoadInternal) {
  if (args.Length() < 2 || !args.IsString(1) || !args.IsObject(0)) {
    THROW_EXCEPTION("loadInternals takes exactly 2 arguments. (object, string)");
  }

  JS_LOCAL_OBJECT module = JS_VALUE_TO_OBJECT(args.GetItem(0));
  jxcore::JXString filename;
  args.GetString(1, &filename);

#ifdef JXCORE_EMBEDS_SQLITE
  if (!strcmp(*filename, "sqlite3")) {
    node_sqlite3::RegisterModule(module);
    RETURN();
  }
#endif

#ifdef JXCORE_EMBEDS_LEVELDOWN
  if (!strcmp(*filename, "leveldown")) {
    leveldown::RegisterModule(module);
    RETURN();
  }
#endif

  THROW_EXCEPTION("Requested native module wasn't embedded.");
}
JS_METHOD_END

}  // namespace node

NODE_MODULE(node_module_wrap, node::ModuleWrap::Initialize)
