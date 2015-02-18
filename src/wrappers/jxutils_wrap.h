// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_WRAPPERS_JXUTILS_WRAP_H_
#define SRC_WRAPPERS_JXUTILS_WRAP_H_

#include "jx/Proxy/JSEngine.h"
#include "jx/commons.h"

namespace node {

class JXUtilsWrap {
 private:
  static DEFINE_JS_METHOD(GetCPU);

  static DEFINE_JS_METHOD(RunLoop);

  static std::string exec(const char* cmd, int* ec);

  static DEFINE_JS_METHOD(ExecSync);

  static DEFINE_JS_METHOD(PrintLog);

  static DEFINE_JS_METHOD(PrintError);

  static DEFINE_JS_METHOD(SetMaxHeaderLength);

  static DEFINE_JS_METHOD(GetUniqueNext);

  static DEFINE_JS_METHOD(ExecSystem);

  static DEFINE_JS_METHOD(BeforeApplicationStart);

  static DEFINE_JS_METHOD(SetSourceExpiration);

  static DEFINE_JS_METHOD(Compress);

  static DEFINE_JS_METHOD(Uncompress);

 public:
  INIT_CLASS_MEMBERS() {

    SET_CLASS_METHOD("getCPU", GetCPU, 2);
    SET_CLASS_METHOD("runLoop", RunLoop, 0);
    SET_CLASS_METHOD("execSync", ExecSync, 1);
    SET_CLASS_METHOD("execSystem", ExecSystem, 1);

    SET_CLASS_METHOD("print", PrintLog, 1);
    SET_CLASS_METHOD("print_err_warn", PrintError, 1);

    SET_CLASS_METHOD("beforeApplicationStart", BeforeApplicationStart, 3);
    SET_CLASS_METHOD("setMaxHeaderLength", SetMaxHeaderLength, 1);
    SET_CLASS_METHOD("getUnique", GetUniqueNext, 0);

    SET_CLASS_METHOD("_cmp", Compress, 1);
    SET_CLASS_METHOD("_ucmp", Uncompress, 1);
    SET_CLASS_METHOD("expirationSource", SetSourceExpiration, 2);
  }
  END_INIT_MEMBERS
};
}  // namespace node

#endif  // SRC_WRAPPERS_JXUTILS_WRAP_H_
