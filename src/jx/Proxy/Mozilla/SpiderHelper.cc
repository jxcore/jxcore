// Copyright & License details are available under JXCORE_LICENSE file

#include "SpiderHelper.h"

namespace jxcore {

/* The class of the global object. */
static JSClass global_class = {
    "global", JSCLASS_HAS_PRIVATE | JSCLASS_HAS_RESERVED_SLOTS(3) |
                  JSCLASS_IS_GLOBAL | JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub, JS_DeletePropertyStub, JS_PropertyStub,
    JS_StrictPropertyStub, JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, 0,
    0, 0, 0, JS_GlobalObjectTraceHook};

static JSClass transplant_class = {"jxcore_transplant",
                                   JSCLASS_HAS_PRIVATE |
                                       JSCLASS_HAS_RESERVED_SLOTS(3),
                                   JS_PropertyStub, JS_DeletePropertyStub,
                                   JS_PropertyStub, JS_StrictPropertyStub,
                                   JS_EnumerateStub, JS_ResolveStub,
                                   JS_ConvertStub, 0, 0, 0, 0, 0};

static JSObject *globals[65] = {NULL};

MozJS::Value getGlobal(const int threadId) {
  return MozJS::Value(globals[threadId],
                      MozJS::Isolate::GetByThreadId(threadId)->GetRaw());
}

JSObject *getGlobalObject(const int threadId) { return globals[threadId]; }

JSObject *NewTransplantObject(JSContext *ctx) {
  return JS_NewObject(ctx, &transplant_class, JS::NullPtr(), JS::NullPtr());
}

JSObject *CrossCompartmentCopy(JSContext *orig_context, JSContext *new_context,
                               MozJS::Value &source, bool global_object) {
  JS::RootedObject cs_root(orig_context, source.GetRawObjectPointer());

  JSObject *tp_obj;
  if (global_object) {
    tp_obj = jxcore::NewContextGlobal(new_context);
  } else {
    tp_obj = NewTransplantObject(new_context);
  }
  JS::RootedObject cs_fake_target(new_context, tp_obj);
  return JS_TransplantObject(new_context, cs_root, cs_fake_target);
}

JSObject *NewContextGlobal(JSContext *ctx) {
  JS::CompartmentOptions options;
  options.setVersion(JSVERSION_LATEST);

  JSObject *c_global = JS_NewGlobalObject(ctx, &global_class, nullptr,
                                          JS::DontFireOnNewGlobalHook, options);

  JS::RootedObject global(ctx, c_global);
  {
    JSAutoCompartment ac(ctx, global);
    JS_InitStandardClasses(ctx, global);
    assert(JS_InitReflect(ctx, global));
  }

  JS_FireOnNewGlobalObject(ctx, global);

  return c_global;
}

JSScript *TransplantScript(JSContext *origContext, JSContext *newContext,
                           JSScript *script) {
  uint32_t ln;
  JS::RootedScript rs_orig(origContext, script);
  void *data = JS_EncodeScript(origContext, rs_orig, &ln);

  return JS_DecodeScript(newContext, data, ln, nullptr);
}

JSObject *NewGlobalObject(JSContext *ctx) {
  JS::CompartmentOptions options;
  options.setVersion(JSVERSION_LATEST);

  const int threadId = JS_GetThreadId(ctx);
  globals[threadId] = JS_NewGlobalObject(ctx, &global_class, nullptr,
                                         JS::DontFireOnNewGlobalHook, options);

  JS::RootedObject global(ctx, globals[threadId]);
  {
    JSAutoCompartment ac(ctx, global);
    JS_InitStandardClasses(ctx, global);
    assert(JS_InitReflect(ctx, global));
  }

  JS_FireOnNewGlobalObject(ctx, global);

  return global;
}

}  // namespace jxcore
