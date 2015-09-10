// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_ENGINEHELPER_H_
#define SRC_JX_PROXY_MOZILLA_ENGINEHELPER_H_

#include "MozJS/MozValue.h"

namespace MozJS {

class EngineHelper {
 public:
  JXCORE_PUBLIC static void CreateObject(JSContext *ctx, const char *type_name,
                           MozJS::Value *ret_val);

  JXCORE_PUBLIC static void GetPropertyNames(JSContext *ctx, MozJS::Value *from,
                               MozJS::Value *to);

  JXCORE_PUBLIC static void FromJSString(const String &str, auto_str *out,
                           bool get_ascii = false);

  JXCORE_PUBLIC static void FromJSString(const Value &str, auto_str *out,
                             bool get_ascii = false);

  JXCORE_PUBLIC static void FromJSString(JSString *str, JSContext *ctx, auto_str *out,
                           bool get_ascii = false);

  JXCORE_PUBLIC static int GetThreadId();

  JXCORE_PUBLIC static bool IsInstanceAlive(JSContext *ctx);
};
}  // namespace MozJS

#endif  // SRC_JX_PROXY_MOZILLA_ENGINEHELPER_H_
