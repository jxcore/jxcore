// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_ERROR_DEFINITION_H_
#define SRC_JX_ERROR_DEFINITION_H_
#include "node.h"

namespace node {

class commons;

inline const char *errno_string(int errorno);

const char *signo_string(int signo);

JS_LOCAL_VALUE ErrnoException(int errorno, const char *syscall, const char *msg,
                              const char *path);

const char *get_uv_errno_string(int errorno);

const char *get_uv_errno_message(int errorno);

JS_LOCAL_VALUE UVException(int errorno, const char *syscall, const char *msg,
                           const char *path);

#ifdef _WIN32
const char *winapi_strerror(const int errorno);

JS_LOCAL_VALUE WinapiErrnoException(int errorno, const char *syscall,
                                    const char *msg, const char *path);
#endif

void SetCOMErrno(commons *com, uv_err_t err);

void SetErrno(uv_err_t err);

void maybeExit(commons *com, const int code);

#ifdef JS_ENGINE_V8
void OnFatalError(const char *location, const char *message);
#else
void OnFatalError(JSContext *JS_GET_STATE_MARKER(), const char *message,
                  JSErrorReport *report);
#endif

void DisplayExceptionLine(JS_TRY_CATCH_TYPE try_catch);

void ReportException(JS_TRY_CATCH_TYPE try_catch, bool show_line);

void FatalException(ENGINE_NS::TryCatch try_catch);

void FatalException_jx(ENGINE_NS::TryCatch try_catch,
                       const char *location = "");
}  // namespace node

#endif  // SRC_JX_ERROR_DEFINITION_H_
