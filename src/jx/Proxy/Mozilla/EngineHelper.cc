// Copyright & License details are available under JXCORE_LICENSE file

#include "../JSEngine.h"
#include "JXString.h"

// Keeping for internal engine replacement template
// #if !defined(JS_ENGINE_MOZJS)
// #define JX_TEST_ENVIRONMENT 1
// #endif

#ifndef JX_TEST_ENVIRONMENT
#include "../../commons.h"
#endif

namespace MozJS {

bool EngineHelper::IsInstanceAlive(JSContext *ctx) {
  if (node::commons::process_status_ == node::JXCORE_INSTANCE_EXITED)
    return false;
  if (ctx == nullptr) return false;

#ifndef JX_TEST_ENVIRONMENT
  node::commons *com =
      node::commons::getInstanceByThreadId(JS_GetThreadId(ctx));
  if (com == NULL) return false;
  if (com->instance_status_ == node::JXCORE_INSTANCE_EXITED) return false;
#endif

  return true;
}

int EngineHelper::GetThreadId() {
#ifndef JX_TEST_ENVIRONMENT
  return node::commons::getCurrentThreadId();
#else
  return 0;
#endif
}

void EngineHelper::FromJSString(const String &str, auto_str *out,
                                bool get_ascii) {
  jxcore::JXString jxs(str.GetRawStringPointer(), str.ctx_, false, get_ascii);
  out->str_ = *jxs;
  out->length_ = jxs.length();
  out->ctx_ = str.ctx_;
}

void EngineHelper::FromJSString(JSString *str, JSContext *ctx, auto_str *out,
                                bool get_ascii) {
  jxcore::JXString jxs(str, ctx, false, get_ascii);
  out->str_ = *jxs;
  out->length_ = jxs.length();
  out->ctx_ = ctx;
}

void EngineHelper::CreateObject(JSContext *ctx, const char *type_name,
                                MozJS::Value *ret_val) {
#ifndef JX_TEST_ENVIRONMENT
  node::commons *com =
      node::commons::getInstanceByThreadId(JS_GetThreadId(ctx));
  *ret_val = com->CreateJSObject(type_name);
#endif
}

void EngineHelper::GetPropertyNames(JSContext *ctx, MozJS::Value *from,
                                    MozJS::Value *to) {
#ifndef JX_TEST_ENVIRONMENT
  node::commons *com =
      node::commons::getInstanceByThreadId(JS_GetThreadId(ctx));
  *to = com->GetPropertyNames(from);
#endif
}
}  // namespace MozJS
