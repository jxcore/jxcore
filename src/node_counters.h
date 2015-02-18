// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_NODE_COUNTERS_H_
#define SRC_NODE_COUNTERS_H_

#include "node.h"

namespace node {

void InitPerfCounters(JS_HANDLE_OBJECT target);
void TermPerfCounters(JS_HANDLE_OBJECT target);
}

#ifdef HAVE_PERFCTR
#include "node_win32_perfctr_provider.h"
#else
#define NODE_COUNTER_ENABLED() (false)
#define NODE_COUNT_HTTP_SERVER_REQUEST()
#define NODE_COUNT_HTTP_SERVER_RESPONSE()
#define NODE_COUNT_HTTP_CLIENT_REQUEST()
#define NODE_COUNT_HTTP_CLIENT_RESPONSE()
#define NODE_COUNT_SERVER_CONN_OPEN()
#define NODE_COUNT_SERVER_CONN_CLOSE()
#define NODE_COUNT_NET_BYTES_SENT(bytes)
#define NODE_COUNT_NET_BYTES_RECV(bytes)
#define NODE_COUNT_GET_GC_RAWTIME()
#define NODE_COUNT_GC_PERCENTTIME()
#define NODE_COUNT_PIPE_BYTES_SENT(bytes)
#define NODE_COUNT_PIPE_BYTES_RECV(bytes)
#endif

#endif  // SRC_NODE_COUNTERS_H_
