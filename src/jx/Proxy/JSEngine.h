// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_PROXY_JSENGINE_H_
#define SRC_JX_PROXY_JSENGINE_H_

#include "EngineLogger.h"

#ifdef JS_IS_NOT_V8
#undef JS_ENGINE_V8  // an ugly hack for Eclipse Editor / Debugger
#endif

// TODO(obastemur) backward compatibility. remove this after updating sub modules
#if !defined(V8_IS_3_28) && !defined(V8_IS_3_14) && !defined(MOZJS_IS_3_40)
#ifdef JS_ENGINE_V8
#define V8_IS_3_28
#else
#define MOZJS_IS_3_40
#endif
#endif

#ifdef V8_IS_3_14

#include "v8.h"
#include "v8-debug.h"
#include "v8-profiler.h"
#include "jx/Proxy/V8_3_14/PMacro.h"
#include "jx/Proxy/V8_3_14/PArguments.h"
#include "jx/Proxy/V8_3_14/v8_typed_array.h"

#define NODE_OBJECT_WRAP_HEADER "jx/Proxy/V8_3_14/node_object_wrap.h"

#elif defined(V8_IS_3_28)

#include "v8.h"
#include "v8-debug.h"
#include "v8-profiler.h"
#include "V8_3_28/PMacro.h"
#include "V8_3_28/PArguments.h"
#include "V8_3_28/extern_string.h"

#ifndef JS_ENGINE_CHAKRA
#include "debugger-agent.h"
#endif

#include "V8_3_28/util-inl.h"

#ifndef NODE_CONTEXT_EMBEDDER_DATA_INDEX
#define NODE_CONTEXT_EMBEDDER_DATA_INDEX 32
#endif

#define NODE_OBJECT_WRAP_HEADER "jx/Proxy/V8_3_28/node_object_wrap.h"

#elif defined(MOZJS_IS_3_40)

#include "Mozilla_340/JXString.h"
#include "Mozilla_340/PMacro.h"
#include "Mozilla_340/MozJS/MozJS.h"
#include "Mozilla_340/PArguments.h"
#include "Mozilla_340/EngineHelper.h"

#define NODE_OBJECT_WRAP_HEADER "jx/Proxy/Mozilla_340/node_object_wrap.h"
#endif

#endif  // SRC_JX_PROXY_JSENGINE_H_
