// Copyright & License details are available under JXCORE_LICENSE file

// Common header file used by "public/"

#ifndef SRC_JX_JX_PRIVATE_H
#define SRC_JX_JX_PRIVATE_H

#include "commons.h"
#include "../jxcore.h"
#include "../wrappers/node_buffer.h"
#include "extend.h"
#include "job.h"

#ifdef JS_ENGINE_MOZJS
#include <limits>  // INT_MAX
#endif

class auto_state {
  jxcore::JXEngine *engine_;
  node::commons *com_;

 public:
  auto_state(jxcore::JXEngine *engine, node::commons *com)
      : engine_(engine), com_(com) {
    engine_->EnterScope();
#ifdef JS_ENGINE_V8
    com->node_isolate->Enter();
#endif
  }

  ~auto_state() {
    engine_->LeaveScope();
#ifdef JS_ENGINE_V8
    com_->node_isolate->Exit();
#endif
  }
};

#define _FREE_MEM_(x)                                             \
  jxcore::JXValueWrapper *wrap##x_ = (jxcore::JXValueWrapper *)x; \
  delete wrap##x_

#define UNWRAP_COM(arg)                            \
  assert(arg->com_ && "com_ can not be NULL");     \
  node::commons *com = (node::commons *)arg->com_; \
  JS_DEFINE_STATE_MARKER(com)

#ifdef JS_ENGINE_V8
#ifdef V8_IS_3_14
#define ENTER_ENGINE_SCOPE()         \
  JS_ENGINE_LOCKER();                \
  auto_state __state__(engine, com); \
  v8::Context::Scope context_scope(engine->getContext())
#else
#define ENTER_ENGINE_SCOPE()         \
  JS_ENGINE_LOCKER();                \
  auto_state __state__(engine, com); \
  v8::Context::Scope context_scope(engine->getContext())
#endif

#define RUN_IN_SCOPE(x)                         \
  if (engine != NULL && !engine->IsInScope()) { \
    ENTER_ENGINE_SCOPE();                       \
    x                                           \
  } else {                                      \
    x                                           \
  }
#elif JS_ENGINE_MOZJS
#define ENTER_ENGINE_SCOPE()
#define RUN_IN_SCOPE(x)                         \
  if (engine != NULL && !engine->IsInScope()) { \
    auto_state __state__(engine, com);          \
    x                                           \
  } else {                                      \
    x                                           \
  }
#endif

#define NULL_CHECK \
  if (value == NULL) return false;

#define EMPTY_CHECK(x)         \
  if (value == NULL) return x; \
  if (value->type_ == RT_Null || value->type_ == RT_Undefined) return x

#define UNWRAP_RESULT(x) \
  jxcore::JXValueWrapper *wrap = (jxcore::JXValueWrapper *)x

#endif // SRC_JX_JX_PRIVATE_H
