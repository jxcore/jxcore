// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_ERROR_DEFINITION_H_
#define SRC_JX_ERROR_DEFINITION_H_

#include "Proxy/JSEngine.h"
#if NODE_WANT_INTERNALS
#include "node_internals.h"
#endif

namespace node {

class commons;

inline const char *errno_string(int errorno);

NODE_EXTERN const char *get_uv_errno_string(int errorno);

NODE_EXTERN const char *get_uv_errno_message(int errorno);

#ifdef _WIN32
const char *winapi_strerror(const int errorno);
#endif

void SetCOMErrno(commons *com, uv_err_t err);

void maybeExit(commons *com, const int code);

#ifdef JS_ENGINE_V8
void OnFatalError(const char *location, const char *message);
#else
void OnFatalError(JSContext *JS_GET_STATE_MARKER(), const char *message,
                  JSErrorReport *report);
#endif

NODE_EXTERN void DisplayExceptionLine(JS_TRY_CATCH_TYPE &try_catch);

NODE_EXTERN void ReportException(JS_TRY_CATCH_TYPE &try_catch, bool show_line);

NODE_EXTERN void FatalException(JS_TRY_CATCH_TYPE &try_catch);

}  // namespace node

#endif  // SRC_JX_ERROR_DEFINITION_H_
