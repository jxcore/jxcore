// Copyright & License details are available under JXCORE_LICENSE file

#include "module_externs.h"
#include "handle_wrap.h"
#include "jx/commons.h"

#include <stdio.h>
#include <string>
#include <iostream>

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x) * 1000)
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
  if (args.Length() < 2) {
    THROW_EXCEPTION("loadInternals takes exactly 2 arguments.");
  }

  JS_LOCAL_OBJECT module = JS_VALUE_TO_OBJECT(args.GetItem(0));
  jxcore::JXString filename;
  args.GetString(1, &filename);

  JS_LOCAL_OBJECT exports =
      JS_VALUE_TO_OBJECT(JS_GET_NAME(module, JS_STRING_ID("exports")));

  if (!strcmp(*filename, "sqlite3")) {
    node_sqlite3::RegisterModule(exports);
  }
}
JS_METHOD_END

}  // namespace node

NODE_MODULE(node_module_wrap, node::ModuleWrap::Initialize)
