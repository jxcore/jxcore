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
  const int tid_ctx = JS_GetThreadId(ctx);
  node::commons *com;
  if (tid_ctx < 0 || tid_ctx > 64) {
	return false;
  } else {
	com = node::commons::getInstanceByThreadId(tid_ctx);
	if (!com) return false;
	if (com->instance_status_ == node::JXCORE_INSTANCE_EXITED) return false;
  }

  JSRuntime *rt = JS_GetRuntime(ctx);
  if (!rt) return false;

  int *tid_ptr = (int *)JS_GetRuntimePrivate(rt);
  int *memref = getThreadIdsMemRef();

  if (tid_ptr < memref || tid_ptr > (memref + (sizeof(int) * 64))) return false;

  const int tid_rt = *(tid_ptr);
  return tid_rt == tid_ctx;
#else
  return true;
#endif
}

int EngineHelper::GetThreadId() {
#ifndef JX_TEST_ENVIRONMENT
  int n = node::commons::getCurrentThreadId();
  return n >= 0 ? n : 0;
#else
  return 0;
#endif
}

void EngineHelper::FromJSString(const String &str, auto_str *out,
                                bool get_ascii) {
  jxcore::JXString jxs(str.GetRawStringPointer(), str.GetContext(), false,
                       get_ascii);
  out->str_ = *jxs;
  out->length_ = jxs.length();
  out->ctx_ = str.GetContext();
}

void EngineHelper::FromJSString(const Value &str, auto_str *out,
                                bool get_ascii) {
  jxcore::JXString jxs(str.GetRawStringPointer(), str.GetContext(), false,
                       get_ascii);
  out->str_ = *jxs;
  out->length_ = jxs.length();
  out->ctx_ = str.GetContext();
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
