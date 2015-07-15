// Copyright & License details are available under JXCORE_LICENSE file

#include "node_counters.h"
#include "uv.h"
#include <string.h>
#include "external/jx_persistent_store.h"
#include "jx/commons.h"
namespace node {

#define SLURP_OBJECT(obj, member, valp)                              \
  if (!(obj)->IsObject()) {                                          \
    return (ThrowException(Exception::Error(String::New(             \
        "expected "                                                  \
        "object for " #obj " to contain object member " #member)))); \
  }                                                                  \
  *valp = JS_LOCAL_OBJECT::Cast(obj->Get(String::New(#member)));

JS_LOCAL_METHOD(COUNTER_NET_SERVER_CONNECTION) {
  NODE_COUNT_SERVER_CONN_OPEN();
}
JS_METHOD_END

JS_LOCAL_METHOD(COUNTER_NET_SERVER_CONNECTION_CLOSE) {
  NODE_COUNT_SERVER_CONN_CLOSE();
}
JS_METHOD_END

JS_LOCAL_METHOD(COUNTER_HTTP_SERVER_REQUEST) {
  NODE_COUNT_HTTP_SERVER_REQUEST();
}
JS_METHOD_END

JS_LOCAL_METHOD(COUNTER_HTTP_SERVER_RESPONSE) {
  NODE_COUNT_HTTP_SERVER_RESPONSE();
}
JS_METHOD_END

JS_LOCAL_METHOD(COUNTER_HTTP_CLIENT_REQUEST) {
  NODE_COUNT_HTTP_CLIENT_REQUEST();
}
JS_METHOD_END

JS_LOCAL_METHOD(COUNTER_HTTP_CLIENT_RESPONSE) {
  NODE_COUNT_HTTP_CLIENT_RESPONSE();
}
JS_METHOD_END

static void counter_gc_start(v8::GCType type, v8::GCCallbackFlags flags) {
  commons *com = node::commons::getInstance();
  com->counter_gc_start_time = NODE_COUNT_GET_GC_RAWTIME();

  return;
}

static void counter_gc_done(v8::GCType type, v8::GCCallbackFlags flags) {
  uint64_t endgc = NODE_COUNT_GET_GC_RAWTIME();
  if (endgc != 0) {
    commons *com = node::commons::getInstance();

    uint64_t totalperiod = endgc - com->counter_gc_end_time;
    uint64_t gcperiod = endgc - com->counter_gc_start_time;

    if (totalperiod > 0) {
      unsigned int percent =
          static_cast<unsigned int>((gcperiod * 100) / totalperiod);

      NODE_COUNT_GC_PERCENTTIME(percent);
      com->counter_gc_end_time = endgc;
    }
  }

  return;
}

struct dtabs {
  const char *name;
  JS_HANDLE_VALUE (*func)(const JS_V8_ARGUMENT &);
  JS_PERSISTENT_FUNCTION_TEMPLATE templ;
};
static jxcore::ThreadStore<dtabs[6]> tabs;
#define NODE_PROBE(a, nm) \
  a.name = #nm;           \
  a.func = nm;            \
  a.templ = JS_PERSISTENT_FUNCTION_TEMPLATE()

void InitPerfCounters(JS_HANDLE_OBJECT target) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  const int tid = tabs.getThreadId();

  NODE_PROBE(tabs.templates[tid][0], COUNTER_NET_SERVER_CONNECTION);
  NODE_PROBE(tabs.templates[tid][1], COUNTER_NET_SERVER_CONNECTION_CLOSE);
  NODE_PROBE(tabs.templates[tid][2], COUNTER_HTTP_SERVER_REQUEST);
  NODE_PROBE(tabs.templates[tid][3], COUNTER_HTTP_SERVER_RESPONSE);
  NODE_PROBE(tabs.templates[tid][4], COUNTER_HTTP_CLIENT_REQUEST);
  NODE_PROBE(tabs.templates[tid][5], COUNTER_HTTP_CLIENT_RESPONSE);

  for (int i = 0; i < 6; i++) {
    tabs.templates[tid][i].templ = JS_PERSISTENT_FUNCTION_TEMPLATE::New(
        JS_NEW_FUNCTION_TEMPLATE(tabs.templates[tid][i].func));
    JS_NAME_SET(target, JS_STRING_ID(tabs.templates[tid][i].name),
                tabs.templates[tid][i].templ->GetFunction());
  }

  // Only Windows performance counters supported
  // To enable other OS, use conditional compilation here
  InitPerfCountersWin32();

  // init times for GC percent calculation and hook callbacks
  com->counter_gc_start_time = NODE_COUNT_GET_GC_RAWTIME();
  com->counter_gc_end_time = com->counter_gc_start_time;

  v8::V8::AddGCPrologueCallback(counter_gc_start);
  v8::V8::AddGCEpilogueCallback(counter_gc_done);
}

void TermPerfCounters(JS_HANDLE_OBJECT target) {
  // Only Windows performance counters supported
  // To enable other OS, use conditional compilation here
  TermPerfCountersWin32();
}
}  // namespace node
