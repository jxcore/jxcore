// Copyright & License details are available under JXCORE_LICENSE file

#include "external/jx_persistent_store.h"

#ifdef HAVE_DTRACE
#include "node_dtrace.h"
#include <string.h>
#include "node_provider.h"
#elif HAVE_ETW
#include "node_dtrace.h"
#include <string.h>
#include "platform/win/node_win32_etw_provider.h"
#include "platform/win/node_win32_etw_provider-inl.h"
#elif HAVE_SYSTEMTAP
#include <string.h>
#include <node.h>
#include <sys/sdt.h>
#include "node_systemtap.h"
#include "node_dtrace.h"
#else
#define NODE_HTTP_SERVER_REQUEST(arg0, arg1)
#define NODE_HTTP_SERVER_REQUEST_ENABLED() (0)
#define NODE_HTTP_SERVER_RESPONSE(arg0)
#define NODE_HTTP_SERVER_RESPONSE_ENABLED() (0)
#define NODE_HTTP_CLIENT_REQUEST(arg0, arg1)
#define NODE_HTTP_CLIENT_REQUEST_ENABLED() (0)
#define NODE_HTTP_CLIENT_RESPONSE(arg0)
#define NODE_HTTP_CLIENT_RESPONSE_ENABLED() (0)
#define NODE_NET_SERVER_CONNECTION(arg0)
#define NODE_NET_SERVER_CONNECTION_ENABLED() (0)
#define NODE_NET_STREAM_END(arg0)
#define NODE_NET_STREAM_END_ENABLED() (0)
#define NODE_NET_SOCKET_READ(arg0, arg1, arg2, arg3, arg4)
#define NODE_NET_SOCKET_READ_ENABLED() (0)
#define NODE_NET_SOCKET_WRITE(arg0, arg1, arg2, arg3, arg4)
#define NODE_NET_SOCKET_WRITE_ENABLED() (0)
#define NODE_GC_START(arg0, arg1)
#define NODE_GC_DONE(arg0, arg1)
#endif

namespace node {

#define SLURP_STRING(obj, member, valp)                              \
  if (!JS_IS_OBJECT(obj)) {                                          \
    THROW_EXCEPTION(                                                 \
        "expected "                                                  \
        "object for " #obj " to contain string member " #member);    \
  }                                                                  \
  jxcore::JXString _##member;                                        \
  {                                                                  \
    JS_LOCAL_VALUE __val = JS_GET_NAME(obj, STD_TO_STRING(#member)); \
    _##member.set_handle(__val);                                     \
  }                                                                  \
  if ((*(const char **)valp = *_##member) == NULL)                   \
    *(const char **)valp = "<unknown>";

#define SLURP_INT(obj, member, valp)                               \
  if (!JS_IS_OBJECT(obj)) {                                        \
    THROW_EXCEPTION(                                               \
        "expected "                                                \
        "object for " #obj " to contain integer member " #member); \
  }                                                                \
  *valp = INTEGER_TO_STD(JS_GET_NAME(obj, STD_TO_STRING(#member)));

#define SLURP_OBJECT(obj, member, valp)                           \
  if (!JS_IS_OBJECT(obj)) {                                       \
    THROW_EXCEPTION(                                              \
        "expected "                                               \
        "object for " #obj " to contain object member " #member); \
  }                                                               \
  *valp = JS_CAST_OBJECT(JS_GET_NAME(obj, STD_TO_STRING(#member)));

#define SLURP_CONNECTION(arg, conn)                                        \
  if (!JS_IS_OBJECT(arg)) {                                                \
    THROW_EXCEPTION(                                                       \
        "expected "                                                        \
        "argument " #arg " to be a connection object");                    \
  }                                                                        \
  node_dtrace_connection_t conn;                                           \
  JS_LOCAL_OBJECT _##conn = JS_CAST_OBJECT(arg);                           \
  JS_LOCAL_VALUE _handle = JS_GET_NAME(_##conn, STD_TO_STRING("_handle")); \
  if (JS_IS_OBJECT(_handle)) {                                             \
    SLURP_INT(JS_CAST_OBJECT(_handle), fd, &conn.fd);                      \
  } else {                                                                 \
    conn.fd = -1;                                                          \
  }                                                                        \
  SLURP_STRING(_##conn, remoteAddress, &conn.remote);                      \
  SLURP_INT(_##conn, remotePort, &conn.port);                              \
  SLURP_INT(_##conn, bufferSize, &conn.buffered);

#define SLURP_CONNECTION_HTTP_CLIENT(arg, conn)         \
  if (!JS_IS_OBJECT(arg)) {                             \
    THROW_EXCEPTION(                                    \
        "expected "                                     \
        "argument " #arg " to be a connection object"); \
  }                                                     \
  node_dtrace_connection_t conn;                        \
  JS_LOCAL_OBJECT _##conn = JS_CAST_OBJECT(arg);        \
  SLURP_INT(_##conn, fd, &conn.fd);                     \
  SLURP_STRING(_##conn, host, &conn.remote);            \
  SLURP_INT(_##conn, port, &conn.port);                 \
  SLURP_INT(_##conn, bufferSize, &conn.buffered);

#define SLURP_CONNECTION_HTTP_CLIENT_RESPONSE(arg0, arg1, conn) \
  if (!(arg0)->IsObject()) {                                    \
    THROW_EXCEPTION(                                            \
        "expected "                                             \
        "argument " #arg0 " to be a connection object");        \
  }                                                             \
  if (!(arg1)->IsObject()) {                                    \
    THROW_EXCEPTION(                                            \
        "expected "                                             \
        "argument " #arg1 " to be a connection object");        \
  }                                                             \
  node_dtrace_connection_t conn;                                \
  JS_LOCAL_OBJECT _##conn = JS_CAST_OBJECT(arg0);               \
  SLURP_INT(_##conn, fd, &conn.fd);                             \
  SLURP_INT(_##conn, bufferSize, &conn.buffered);               \
  _##conn = JS_CAST_OBJECT(arg1);                               \
  SLURP_STRING(_##conn, host, &conn.remote);                    \
  SLURP_INT(_##conn, port, &conn.port);

JS_LOCAL_METHOD(DTRACE_NET_SERVER_CONNECTION) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_NET_SERVER_CONNECTION_ENABLED()) RETURN();
#endif

  SLURP_CONNECTION(GET_ARG(0), conn);
#ifdef HAVE_SYSTEMTAP
  NODE_NET_SERVER_CONNECTION(conn.fd, conn.remote, conn.port, conn.buffered);
#else
  NODE_NET_SERVER_CONNECTION(&conn, conn.remote, conn.port, conn.fd);
#endif
}
JS_METHOD_END

JS_LOCAL_METHOD(DTRACE_NET_STREAM_END) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_NET_STREAM_END_ENABLED()) RETURN();
#endif

  SLURP_CONNECTION(GET_ARG(0), conn);
#ifdef HAVE_SYSTEMTAP
  NODE_NET_STREAM_END(conn.fd, conn.remote, conn.port, conn.buffered);
#else
  NODE_NET_STREAM_END(&conn, conn.remote, conn.port, conn.fd);
#endif
}
JS_METHOD_END

JS_LOCAL_METHOD(DTRACE_NET_SOCKET_READ) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_NET_SOCKET_READ_ENABLED()) RETURN();
#endif

  SLURP_CONNECTION(GET_ARG(0), conn);

#ifdef HAVE_SYSTEMTAP
  NODE_NET_SOCKET_READ(conn.fd, conn.remote, conn.port, conn.buffered);
#else
  if (!args.IsNumber(1)) {
    THROW_EXCEPTION("expected argument 1 to be number of bytes");
  }
  int nbytes = args.GetInt32(1);
  NODE_NET_SOCKET_READ(&conn, nbytes, conn.remote, conn.port, conn.fd);
#endif
}
JS_METHOD_END

JS_LOCAL_METHOD(DTRACE_NET_SOCKET_WRITE) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_NET_SOCKET_WRITE_ENABLED()) RETURN();
#endif

  SLURP_CONNECTION(GET_ARG(0), conn);

#ifdef HAVE_SYSTEMTAP
  NODE_NET_SOCKET_WRITE(conn.fd, conn.remote, conn.port, conn.buffered);
#else
  if (!args.IsNumber(1)) {
    THROW_EXCEPTION("expected argument 1 to be number of bytes");
  }
  int nbytes = args.GetInt32(1);
  NODE_NET_SOCKET_WRITE(&conn, nbytes, conn.remote, conn.port, conn.fd);
#endif
}
JS_METHOD_END

JS_LOCAL_METHOD(DTRACE_HTTP_SERVER_REQUEST) {
  node_dtrace_http_server_request_t req;

#ifndef HAVE_SYSTEMTAP
  if (!NODE_HTTP_SERVER_REQUEST_ENABLED()) RETURN();
#endif

  JS_LOCAL_OBJECT arg0 = JS_CAST_OBJECT(GET_ARG(0));
  JS_LOCAL_OBJECT headers;

  memset(&req, 0, sizeof(req));
  req._un.version = 1;
  SLURP_STRING(arg0, url, &req.url);
  SLURP_STRING(arg0, method, &req.method);

  SLURP_OBJECT(arg0, headers, &headers);

  if (!JS_IS_OBJECT(headers))
    THROW_EXCEPTION(
        "expected object for request to contain string member headers");

  JS_LOCAL_VALUE strfwdfor =
      JS_GET_NAME(headers, STD_TO_STRING("x-forwarded-for"));
  jxcore::JXString fwdfor(strfwdfor);

  if (!JS_IS_STRING(strfwdfor) || (req.forwardedFor = *fwdfor) == NULL)
    req.forwardedFor = const_cast<char *>("");

  SLURP_CONNECTION(GET_ARG(1), conn);

#ifdef HAVE_SYSTEMTAP
  NODE_HTTP_SERVER_REQUEST(&req, conn.fd, conn.remote, conn.port,
                           conn.buffered);
#else
  NODE_HTTP_SERVER_REQUEST(&req, &conn, conn.remote, conn.port, req.method,
                           req.url, conn.fd);
#endif
}
JS_METHOD_END

JS_LOCAL_METHOD(DTRACE_HTTP_SERVER_RESPONSE) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_HTTP_SERVER_RESPONSE_ENABLED()) RETURN();
#endif

  SLURP_CONNECTION(GET_ARG(0), conn);
#ifdef HAVE_SYSTEMTAP
  NODE_HTTP_SERVER_RESPONSE(conn.fd, conn.remote, conn.port, conn.buffered);
#else
  NODE_HTTP_SERVER_RESPONSE(&conn, conn.remote, conn.port, conn.fd);
#endif
}
JS_METHOD_END

JS_LOCAL_METHOD(DTRACE_HTTP_CLIENT_REQUEST) {
  node_dtrace_http_client_request_t req;
  char *header;

#ifndef HAVE_SYSTEMTAP
  if (!NODE_HTTP_CLIENT_REQUEST_ENABLED()) RETURN();
#endif

  /*
   * For the method and URL, we're going to dig them out of the header.  This
   * is not as efficient as it could be, but we would rather not force the
   * caller here to retain their method and URL until the time at which
   * DTRACE_HTTP_CLIENT_REQUEST can be called.
   */
  JS_LOCAL_OBJECT arg0 = JS_VALUE_TO_OBJECT(GET_ARG(0));
  SLURP_STRING(arg0, _header, &header);

  req.method = header;

  while (*header != '\0' && *header != ' ') header++;

  if (*header != '\0') *header++ = '\0';

  req.url = header;

  while (*header != '\0' && *header != ' ') header++;

  *header = '\0';

  SLURP_CONNECTION_HTTP_CLIENT(GET_ARG(1), conn);
#ifdef HAVE_SYSTEMTAP
  NODE_HTTP_CLIENT_REQUEST(&req, conn.fd, conn.remote, conn.port,
                           conn.buffered);
#else
  NODE_HTTP_CLIENT_REQUEST(&req, &conn, conn.remote, conn.port, req.method,
                           req.url, conn.fd);
#endif
}
JS_METHOD_END

JS_LOCAL_METHOD(DTRACE_HTTP_CLIENT_RESPONSE) {
#ifndef HAVE_SYSTEMTAP
  if (!NODE_HTTP_CLIENT_RESPONSE_ENABLED()) RETURN();
#endif

  SLURP_CONNECTION_HTTP_CLIENT_RESPONSE(GET_ARG(0), GET_ARG(1), conn);
#ifdef HAVE_SYSTEMTAP
  NODE_HTTP_CLIENT_RESPONSE(conn.fd, conn.remote, conn.port, conn.buffered);
#else
  NODE_HTTP_CLIENT_RESPONSE(&conn, conn.remote, conn.port, conn.fd);
#endif
}
JS_METHOD_END

#ifdef JS_ENGINE_V8
static int dtrace_gc_start(v8::GCType type, v8::GCCallbackFlags flags) {
#ifdef HAVE_SYSTEMTAP
  NODE_GC_START();
#else
  NODE_GC_START(type, flags);
#endif
  /*
   * We avoid the tail-call elimination of the USDT probe (which screws up
   * args) by forcing a return of 0.
   */
  return 0;
}

static int dtrace_gc_done(v8::GCType type, v8::GCCallbackFlags flags) {
#ifdef HAVE_SYSTEMTAP
  NODE_GC_DONE();
#else
  NODE_GC_DONE(type, flags);
#endif
  return 0;
}
#endif

struct dtabs {
  const char *name;
#ifdef JS_ENGINE_V8
  JS_HANDLE_VALUE (*func)(const v8::Arguments &);
#elif defined(JS_ENGINE_MOZJS)
  bool (*func)(JSContext *JS_GET_STATE_MARKER(), unsigned __argc,
               JS::Value *__jsval);
#endif
  JS_PERSISTENT_FUNCTION_TEMPLATE templ;
};

static jxcore::ThreadStore<dtabs[8]> tabs;

#define NODE_PROBE(a, nm) \
  a.name = #nm;           \
  a.func = nm;            \
  a.templ = JS_PERSISTENT_FUNCTION_TEMPLATE()

void InitDTrace(JS_HANDLE_OBJECT target) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  const int tid = tabs.getThreadId();

  NODE_PROBE(tabs.templates[tid][0], DTRACE_NET_SERVER_CONNECTION);
  NODE_PROBE(tabs.templates[tid][1], DTRACE_NET_STREAM_END);
  NODE_PROBE(tabs.templates[tid][2], DTRACE_NET_SOCKET_READ);
  NODE_PROBE(tabs.templates[tid][3], DTRACE_NET_SOCKET_WRITE);
  NODE_PROBE(tabs.templates[tid][4], DTRACE_HTTP_SERVER_REQUEST);
  NODE_PROBE(tabs.templates[tid][5], DTRACE_HTTP_SERVER_RESPONSE);
  NODE_PROBE(tabs.templates[tid][6], DTRACE_HTTP_CLIENT_REQUEST);
  NODE_PROBE(tabs.templates[tid][7], DTRACE_HTTP_CLIENT_RESPONSE);

  for (unsigned int i = 0; i < 8; i++) {
    tabs.templates[tid][i].templ = JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(
        JS_NEW_FUNCTION_TEMPLATE(tabs.templates[tid][i].func));
    JS_NAME_SET(target, JS_STRING_ID(tabs.templates[tid][i].name),
                JS_GET_FUNCTION(tabs.templates[tid][i].templ));
  }

#ifdef HAVE_ETW
  init_etw();
#endif

#ifdef JS_ENGINE_V8
#if defined HAVE_DTRACE || defined HAVE_ETW || defined HAVE_SYSTEMTAP
  v8::V8::AddGCPrologueCallback((v8::GCPrologueCallback)dtrace_gc_start);
  v8::V8::AddGCEpilogueCallback((v8::GCEpilogueCallback)dtrace_gc_done);
#endif
#elif defined(JS_ENGINE_MOZJS)
// TODO(obastemur) implement
#endif
}

void cleanUpDTrace() {
#ifdef JS_ENGINE_V8
#if defined HAVE_DTRACE || defined HAVE_ETW || defined HAVE_SYSTEMTAP
  v8::V8::RemoveGCPrologueCallback((v8::GCPrologueCallback)dtrace_gc_start);
  v8::V8::RemoveGCEpilogueCallback((v8::GCEpilogueCallback)dtrace_gc_done);
#endif
#elif defined(JS_ENGINE_MOZJS)
// TODO(obastemur) implement
#endif
}
}  // namespace node
