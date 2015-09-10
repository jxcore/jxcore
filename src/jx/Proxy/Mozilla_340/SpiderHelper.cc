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

void NewTransplantObject(JSContext *ctx, JS::MutableHandleObject ret_val) {
  ret_val.set(JS_NewObject(ctx, &transplant_class, JS::NullPtr(), JS::NullPtr()));
}

void CrossCompartmentCopy(JSContext *orig_context, JSContext *new_context,
                          MozJS::Value &source, bool global_object,
                          JS::MutableHandleObject retval) {
  JS::RootedObject cs_root(orig_context, source.GetRawObjectPointer());

  JS::RootedObject tp_obj(new_context);
  if (global_object) {
    jxcore::NewContextGlobal(new_context, &tp_obj);
  } else {
    NewTransplantObject(new_context, &tp_obj);
  }
  JS::RootedObject cs_fake_target(new_context, tp_obj);
  retval.set(JS_TransplantObject(new_context, cs_root, cs_fake_target));
}

void NewContextGlobal(JSContext *ctx, JS::MutableHandleObject ret_val) {
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

  ret_val.set(c_global);
}

MemoryScript GetScriptMemory(JSContext *ctx, JSScript *script) {
  uint32_t ln;
  JS::RootedScript rs_orig(ctx, script);
  void *data = JS_EncodeScript(ctx, rs_orig, &ln);

  return MemoryScript(data, ln);
}

JSScript *GetScript(JSContext *ctx, MemoryScript ms) {
  return JS_DecodeScript(ctx, ms.data(), ms.length(), nullptr);
}

void NewGlobalObject(JSContext *ctx, JS::MutableHandleObject ret_val) {
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

  ret_val.set(global);
}

}  // namespace jxcore
