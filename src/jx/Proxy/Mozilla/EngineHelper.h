// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_ENGINEHELPER_H_
#define SRC_JX_PROXY_MOZILLA_ENGINEHELPER_H_

#include "MozJS/MozValue.h"

namespace MozJS {

class EngineHelper {
 public:
  static void CreateObject(JSContext *ctx, const char *type_name,
                           MozJS::Value *ret_val);

  static void GetPropertyNames(JSContext *ctx, MozJS::Value *from,
                               MozJS::Value *to);

  static void FromJSString(const String &str, auto_str *out,
                           bool get_ascii = false);

  static void FromJSString(const Value &str, auto_str *out,
                             bool get_ascii = false);

  static void FromJSString(JSString *str, JSContext *ctx, auto_str *out,
                           bool get_ascii = false);

  static int GetThreadId();

  static bool IsInstanceAlive(JSContext *ctx);
};
}  // namespace MozJS

#endif  // SRC_JX_PROXY_MOZILLA_ENGINEHELPER_H_
