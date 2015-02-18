// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_MOZILLA_SPIDERHELPER_H_
#define SRC_JX_PROXY_MOZILLA_SPIDERHELPER_H_

#include "MozJS/MozJS.h"

namespace jxcore {

JSObject *NewGlobalObject(JSContext *ctx);
JSObject *NewContextGlobal(JSContext *ctx);
MozJS::Value getGlobal(const int threadId);
JSObject *getGlobalObject(const int threadId);
JSObject *NewTransplantObject(JSContext *ctx);
JSScript *TransplantScript(JSContext *origContext, JSContext *newContext, JSScript *script);
JSObject *CrossCompartmentCopy(JSContext *orig_context, JSContext *new_context,
                               MozJS::Value &source, bool global_object);

}  // namespace jxcore
#endif  // SRC_JX_PROXY_MOZILLA_SPIDERHELPER_H_
