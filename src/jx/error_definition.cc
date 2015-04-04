// Copyright & License details are available under JXCORE_LICENSE file

#include "error_definition.h"
#include "jx/extend.h"
#include "jx/memory_store.h"
#include "wrappers/handle_wrap.h"
#include "string_bytes.h"
#include <stdlib.h>
#include <string.h>

namespace node {

JS_LOCAL_VALUE ErrnoException(int errorno, const char *syscall, const char *msg,
                              const char *path) {
  JS_LOCAL_VALUE e;
  JS_DEFINE_COM_AND_MARKER();

  if (!msg[0]) {
    msg = strerror(errorno);
  }

  const char *estring = errno_string(errorno);
  std::string message(estring);
  message += ", ";
  message += msg;

  if (path) {
    message += " '";
    message += path;
    message += "'";
  }
  JS_LOCAL_STRING err_str = STD_TO_STRING(message.c_str());
  e = JS_STRING_TO_ERROR_VALUE(err_str);

  JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(e);

  JS_NAME_SET(obj, JS_STRING_ID("errno"), STD_TO_INTEGER(errorno));
  JS_NAME_SET(obj, JS_STRING_ID("code"), STD_TO_STRING(estring));
  if (path) JS_NAME_SET(obj, JS_STRING_ID("path"), STD_TO_STRING(path));
  if (syscall)
    JS_NAME_SET(obj, JS_STRING_ID("syscall"), STD_TO_STRING(syscall));
  return e;
}

const char *get_uv_errno_string(int errorno) {
  uv_err_t err;
  memset(&err, 0, sizeof err);
  err.code = (uv_err_code)errorno;
  return uv_err_name(err);
}

const char *get_uv_errno_message(int errorno) {
  uv_err_t err;
  memset(&err, 0, sizeof err);
  err.code = (uv_err_code)errorno;
  return uv_strerror(err);
}

JS_LOCAL_VALUE UVException(int errorno, const char *syscall, const char *msg,
                           const char *path) {
  if (!msg || !msg[0]) msg = get_uv_errno_message(errorno);

  JS_DEFINE_COM_AND_MARKER();

  const char *estring = get_uv_errno_string(errorno);

  std::string str(estring);
  str += ", ";
  str += msg;

  std::string path_str("");

  if (path) {
#ifdef _WIN32
    if (strncmp(path, "\\\\?\\UNC\\", 8) == 0) {
      path_str += "\\\\";
      path_str += (path + 8);
    } else if (strncmp(path, "\\\\?\\", 4) == 0) {
      path_str = (path + 4);
    } else {
      path_str = path;
    }
#else
    path_str = path;
#endif

    str += " '";
    str += path_str;
    str += "'";
  }

#ifdef JS_ENGINE_V8
  JS_LOCAL_STRING error_str = STD_TO_STRING(str.c_str());
  JS_LOCAL_VALUE e = ENGINE_NS::Exception::Error(error_str);

  JS_LOCAL_OBJECT obj = e->ToObject();
#elif defined(JS_ENGINE_MOZJS)
  JS_LOCAL_OBJECT obj = com->CreateJSObject("Error");

  JS_LOCAL_STRING message = STD_TO_STRING(str.c_str());
  JS_NAME_SET(obj, "message", message);
#endif

  JS_NAME_SET(obj, JS_STRING_ID("errno"), STD_TO_INTEGER(errorno));
  JS_NAME_SET(obj, JS_STRING_ID("code"), STD_TO_STRING(estring));
  if (path)
    JS_NAME_SET(obj, JS_STRING_ID("path"), STD_TO_STRING(path_str.c_str()));
  if (syscall)
    JS_NAME_SET(obj, JS_STRING_ID("syscall"), STD_TO_STRING(syscall));

#ifdef JS_ENGINE_V8
  return e;
#elif defined(JS_ENGINE_MOZJS)
  return obj;
#endif
}

#ifdef _WIN32
// Does about the same as strerror(),
// but supports all windows error messages
const char *winapi_strerror(const int errorno) {
  char *errmsg = NULL;

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, errorno, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) & errmsg, 0, NULL);

  if (errmsg) {
    // Remove trailing newlines
    for (int i = strlen(errmsg) - 1;
         i >= 0 && (errmsg[i] == '\n' || errmsg[i] == '\r'); i--) {
      errmsg[i] = '\0';
    }

    return errmsg;
  } else {
    // FormatMessage failed
    return "Unknown error";
  }
}

JS_LOCAL_VALUE WinapiErrnoException(int errorno, const char *syscall,
                                    const char *msg, const char *path) {
  if (!msg || !msg[0]) {
    msg = winapi_strerror(errorno);
  }
  JS_DEFINE_COM_AND_MARKER();
  std::string str(msg);

  if (path) {
    str += " '";
    str += path;
    str += "'";
  }

  JS_LOCAL_STRING err_str = STD_TO_STRING(str.c_str());
  JS_LOCAL_VALUE e;
  e = ENGINE_NS::Exception::Error(err_str);

  JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(e);

  JS_NAME_SET(obj, JS_STRING_ID("errno"), STD_TO_INTEGER(errorno));
  if (path) JS_NAME_SET(obj, JS_STRING_ID("path"), STD_TO_STRING(path));
  if (syscall)
    JS_NAME_SET(obj, JS_STRING_ID("syscall"), STD_TO_STRING(syscall));
  return e;
}
#endif

void SetCOMErrno(node::commons *com, uv_err_t err) {
  JS_ENTER_SCOPE();
  if (com == NULL) return;

  JS_DEFINE_STATE_MARKER(com);

  if (err.code == UV_UNKNOWN) {
    char errno_buf[100];
    snprintf(errno_buf, sizeof(errno_buf), "Unknown system errno %d",
             err.sys_errno_);
    JS_NAME_SET(com->getProcess(), JS_PREDEFINED_STRING(_errno),
                STD_TO_STRING(errno_buf));
  } else {
    JS_NAME_SET(com->getProcess(), JS_PREDEFINED_STRING(_errno),
                STD_TO_STRING(uv_err_name(err)));
  }
}

void SetErrno(uv_err_t err) {
  JS_ENTER_SCOPE_COM();
  if (com == NULL) return;

  JS_DEFINE_STATE_MARKER(com);

  if (err.code == UV_UNKNOWN) {
    char errno_buf[100];
    snprintf(errno_buf, sizeof(errno_buf), "Unknown system errno %d",
             err.sys_errno_);
    JS_NAME_SET(com->getProcess(), JS_PREDEFINED_STRING(_errno),
                STD_TO_STRING(errno_buf));
  } else {
    JS_NAME_SET(com->getProcess(), JS_PREDEFINED_STRING(_errno),
                STD_TO_STRING(uv_err_name(err)));
  }
}

void maybeExit(node::commons *com, const int code) {
  EmitReset(com->getProcess(), code);
}

#ifdef JS_ENGINE_V8
void OnFatalError(const char *location, const char *message) {
  if (location) {
    error_console("FATAL ERROR: %s %s\n", location, message);
  } else {
    error_console("FATAL ERROR: %s\n", message);
  }
  abort();
}
#else
std::string ConvertStack(MozJS::auto_str *str) {
  // turns SM stack info into v8/node look
  std::string stack;
  if (str->length_) {
    std::string buffer = "    at ";
    char *st = str->str_;
    bool marked_first = true;
    bool prts_mark = false;
    int nl_location = 0;
    int counter = 9;
    int line_count = 0;
    for (int i = 0; i < str->length_; i++) {
      if (st[i] == '\n') {
        line_count++;
        if (prts_mark) {
          prts_mark = false;
          buffer += ")";
        }
        if (--counter <= 0) break;
        nl_location = i;
        buffer += "\n    at ";
        marked_first = true;
      } else if (st[i] == '@' && marked_first) {
        if (st[i + 1] && i - 1 != nl_location) {
          buffer += " (";
          prts_mark = true;
        }
      } else if (marked_first) {
        buffer += st[i];
      }
    }

    int bfln = buffer.length();
    if (line_count > 1) stack = bfln > 5 ? buffer.substr(0, bfln - 5) : buffer;
  } else {
    stack = "";
  }
  return stack;
}

void OnFatalError(JSContext *JS_GET_STATE_MARKER(), const char *message,
                  JSErrorReport *report) {
  JS_TRY_CATCH(tt);
  JS_LOCAL_VALUE current_exception;
  if (tt.HasCaught()) {
    current_exception = tt.Exception();

    // check if the exception received from vm user context
    node::commons *com = node::commons::getInstance();
    if (com) {
      JSContext* real_ctx = com->node_isolate->GetRaw();
      if (__contextORisolate != real_ctx) {
    	com->temp_exception_ = current_exception;
    	return;
      }
    }
  } else if (JSREPORT_IS_WARNING(report->flags)) {
    return;  // internal warning (i.e. ASMJS)
  }

  if (!current_exception.IsEmpty() && current_exception.IsObject()) {
    tt.SetHolder(MozJS::Value(current_exception.GetRawObjectPointer(),
                              JS_GET_STATE_MARKER()));
  } else {
    MozJS::String msg_val = STD_TO_STRING(message);
    current_exception = MozJS::Exception::Error(msg_val).GetErrorObject();
    JS_NAME_SET(current_exception, "message", msg_val);

    if (report->filename != nullptr)
      JS_NAME_SET(current_exception, "fileName",
                  STD_TO_STRING(report->filename));

    JS_NAME_SET(current_exception, "lineNumber",
                STD_TO_INTEGER(report->lineno));
    JS_NAME_SET(current_exception, "columnNumber",
                STD_TO_INTEGER(report->column));

    if (report->linebuf != nullptr)
      JS_NAME_SET(current_exception, "lineBuf", STD_TO_STRING(report->linebuf));

    tt.SetHolder(current_exception);
  }

  FatalException(tt);
}
#endif

void DisplayExceptionLine(JS_TRY_CATCH_TYPE &try_catch) {
  // Prevent re-entry into this function.  For example, if there is
  // a throw from a program in vm.runInThisContext(code, filename, true),
  // then we want to show the original failure, not the secondary one.
  static bool displayed_error = false;

  if (displayed_error) {
    displayed_error = false;
    return;
  }
  displayed_error = true;

  JS_ENTER_SCOPE();

  JS_LOCAL_MESSAGE message = try_catch.Message();

  uv_tty_reset_mode();

  error_console("\n");

#ifdef JS_ENGINE_V8
  if (!message.IsEmpty()) {
    jxcore::JXString filename(message->GetScriptResourceName());
    const char *filename_string = *filename;
    int linenum = message->GetLineNumber();
    error_console("%s:%i\n", filename_string, linenum);
    // Print line of source code.
    jxcore::JXString sourceline(message->GetSourceLine());
    const char *sourceline_string = *sourceline;

    // Because of how node modules work, all scripts are wrapped with a
    // "function (module, exports, __filename, ...) {"
    // to provide script local variables.
    //
    // When reporting errors on the first line of a script, this wrapper
    // function is leaked to the user. There used to be a hack here to
    // truncate off the first 62 characters, but it caused numerous other
    // problems when vm.runIn*Context() methods were used for non-module
    // code.
    //
    // If we ever decide to re-instate such a hack, the following steps
    // must be taken:
    //
    // 1. Pass a flag around to say "this code was wrapped"
    // 2. Update the stack frame output so that it is also correct.
    //
    // It would probably be simpler to add a line rather than add some
    // number of characters to the first line, since V8 truncates the
    // sourceline to 78 characters, and we end up not providing very much
    // useful debugging info to the user if we remove 62 characters.

    int start = message->GetStartColumn();
    int end = message->GetEndColumn();

    error_console("%s\n", sourceline_string);

#ifndef __MOBILE_OS__
    // Print wavy underline (GetUnderline is deprecated).
    for (int i = 0; i < start; i++) {
      fputc((sourceline_string[i] == '\t') ? '\t' : ' ', stderr);
    }
    for (int i = start; i < end; i++) {
      fputc('^', stderr);
    }
    fputc('\n', stderr);
#endif
  }
#elif defined(JS_ENGINE_MOZJS)
  JS_THROW_EXCEPTION(message);
#endif
}

void ReportException(JS_TRY_CATCH_TYPE &try_catch, bool show_line) {
#ifdef JS_ENGINE_V8
  JS_ENTER_SCOPE_COM();

  if (show_line) DisplayExceptionLine(try_catch);

  JS_DEFINE_STATE_MARKER(com);
  JS_LOCAL_VALUE stack_trace = try_catch.StackTrace();
  jxcore::JXString trace;
  if (!JS_IS_EMPTY(stack_trace) && !stack_trace->IsUndefined()) trace.SetFromHandle(stack_trace);

  // range errors have a trace member set to undefined
  if (trace.length() > 0) {
    error_console("%s\n", *trace);
  } else {
    // this really only happens for RangeErrors, since they're the only
    // kind that won't have all this info in the trace, or when non-Error
    // objects are thrown manually.
    JS_LOCAL_VALUE er = try_catch.Exception();
    bool isErrorObject = JS_IS_OBJECT(er);
    JS_LOCAL_VALUE msg_obj;
    if (isErrorObject) {
      JS_LOCAL_OBJECT er_obj = JS_VALUE_TO_OBJECT(er);
      JS_LOCAL_STRING msg_str = STD_TO_STRING("message");
      JS_LOCAL_STRING name_str = STD_TO_STRING("name");
      JS_LOCAL_VALUE name_obj = JS_GET_NAME(er_obj, name_str);
      msg_obj = JS_GET_NAME(er_obj, msg_str);
      isErrorObject =
          !(JS_IS_UNDEFINED(msg_obj)) && !(JS_IS_UNDEFINED(name_obj));

      if (isErrorObject) {
        jxcore::JXString name(name_obj);
        error_console("%s: ", *name);
      }
    }

    jxcore::JXString msg(!isErrorObject ? er : msg_obj);
    if (msg.length() == 4) {
      const char *m = *msg;
      if (*m == 'n' && *(m + 1) == 'u' && *(m + 3) == 'l') return;
    }
    error_console("%s\n", *msg);
  }

  fflush(stderr);
#elif defined(JS_ENGINE_MOZJS)
  JS_LOCAL_VALUE msgval = try_catch.Exception();
#if !defined(__ANDROID__) && !defined(__IOS__)
  if (try_catch.HasHolder()) {
#endif
    MozJS::Value msg = msgval.Get("message");
    MozJS::auto_str str_msg;
    msg.ToSTDString(&str_msg);

    msg = msgval.Get("stack");
    MozJS::auto_str str_stack;
    if (!msg.IsEmpty()) {
      msg.ToSTDString(&str_stack);
    } else {
      str_stack.str_ = "";
      str_stack.length_ = 0;
    }
    MozJS::Value name = msgval.Get("name");
    const char *err_name = "Error";
    jxcore::JXString err_name_str;
    if (!name.IsEmpty()) {
      err_name_str.SetFromHandle(name);
      err_name = *err_name_str;
    }
    error_console("%s: ", err_name);
    error_console("%s", *str_msg);

    MozJS::Value fnval = msgval.Get("fileName");
    if (!fnval.IsUndefined()) {
      jxcore::JXString fname(fnval);
      MozJS::Value lnumval = msgval.Get("lineNumber");
      MozJS::Value cnumval = msgval.Get("columnNumber");

      jxcore::JXString lnum(lnumval);
      jxcore::JXString cnum(cnumval);

      error_console(" (%s %s:%s)\n", *fname, *lnum, *cnum);
    } else {
      error_console("\n");
    }
    std::string stack = ConvertStack(&str_stack);
    error_console("%s\n", stack.c_str());
#if !defined(__ANDROID__) && !defined(__IOS__)
  } else {
    JS_THROW_EXCEPTION(msgval);
  }
#endif
#endif
}

void FatalException(JS_TRY_CATCH_TYPE &try_catch) {
  JS_ENTER_SCOPE_COM();
  if (com == NULL || com->instance_status_ == node::JXCORE_INSTANCE_EXITED || com->expects_reset)
    return;
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_STRING ferr_str = STD_TO_STRING("_fatalException");
  JS_LOCAL_VALUE fatal_v = JS_GET_NAME(com->getProcess(), ferr_str);

  if (!JS_IS_FUNCTION(fatal_v)) {
    // failed before the process._fatalException function was added!
    // this is probably pretty bad.  Nothing to do but report and exit.
    ReportException(try_catch, true);
    maybeExit(com, 6);
    return;
  }

  JS_LOCAL_FUNCTION fatal_f = JS_CAST_FUNCTION(fatal_v);

  JS_LOCAL_VALUE error = try_catch.Exception();
  JS_LOCAL_VALUE argv[] = {error};

  JS_TRY_CATCH(fatal_try_catch);

  // this will return true if the JS layer handled it, false otherwise
  JS_LOCAL_VALUE caught =
      JS_METHOD_CALL(fatal_f, com->getProcess(), ARRAY_SIZE(argv), argv);

  if (fatal_try_catch.HasCaught()) {
    // the fatal exception function threw, so we must exit
    ReportException(fatal_try_catch, true);
    maybeExit(com, 7);
    return;
  }

  if (false == BOOLEAN_TO_STD(caught)) {
    ReportException(try_catch, true);
    maybeExit(com, 8);
    return;
  }
}

inline const char *errno_string(int errorno) {
#define ERRNO_CASE(e) \
  case e:             \
    return #e;
  switch (errorno) {
#ifdef EACCES
    ERRNO_CASE(EACCES);
#endif

#ifdef EADDRINUSE
    ERRNO_CASE(EADDRINUSE);
#endif

#ifdef EADDRNOTAVAIL
    ERRNO_CASE(EADDRNOTAVAIL);
#endif

#ifdef EAFNOSUPPORT
    ERRNO_CASE(EAFNOSUPPORT);
#endif

#ifdef EAGAIN
    ERRNO_CASE(EAGAIN);
#endif

#ifdef EWOULDBLOCK
#if EAGAIN != EWOULDBLOCK
    ERRNO_CASE(EWOULDBLOCK);
#endif
#endif

#ifdef EALREADY
    ERRNO_CASE(EALREADY);
#endif

#ifdef EBADF
    ERRNO_CASE(EBADF);
#endif

#ifdef EBADMSG
    ERRNO_CASE(EBADMSG);
#endif

#ifdef EBUSY
    ERRNO_CASE(EBUSY);
#endif

#ifdef ECANCELED
    ERRNO_CASE(ECANCELED);
#endif

#ifdef ECHILD
    ERRNO_CASE(ECHILD);
#endif

#ifdef ECONNABORTED
    ERRNO_CASE(ECONNABORTED);
#endif

#ifdef ECONNREFUSED
    ERRNO_CASE(ECONNREFUSED);
#endif

#ifdef ECONNRESET
    ERRNO_CASE(ECONNRESET);
#endif

#ifdef EDEADLK
    ERRNO_CASE(EDEADLK);
#endif

#ifdef EDESTADDRREQ
    ERRNO_CASE(EDESTADDRREQ);
#endif

#ifdef EDOM
    ERRNO_CASE(EDOM);
#endif

#ifdef EDQUOT
    ERRNO_CASE(EDQUOT);
#endif

#ifdef EEXIST
    ERRNO_CASE(EEXIST);
#endif

#ifdef EFAULT
    ERRNO_CASE(EFAULT);
#endif

#ifdef EFBIG
    ERRNO_CASE(EFBIG);
#endif

#ifdef EHOSTUNREACH
    ERRNO_CASE(EHOSTUNREACH);
#endif

#ifdef EIDRM
    ERRNO_CASE(EIDRM);
#endif

#ifdef EILSEQ
    ERRNO_CASE(EILSEQ);
#endif

#ifdef EINPROGRESS
    ERRNO_CASE(EINPROGRESS);
#endif

#ifdef EINTR
    ERRNO_CASE(EINTR);
#endif

#ifdef EINVAL
    ERRNO_CASE(EINVAL);
#endif

#ifdef EIO
    ERRNO_CASE(EIO);
#endif

#ifdef EISCONN
    ERRNO_CASE(EISCONN);
#endif

#ifdef EISDIR
    ERRNO_CASE(EISDIR);
#endif

#ifdef ELOOP
    ERRNO_CASE(ELOOP);
#endif

#ifdef EMFILE
    ERRNO_CASE(EMFILE);
#endif

#ifdef EMLINK
    ERRNO_CASE(EMLINK);
#endif

#ifdef EMSGSIZE
    ERRNO_CASE(EMSGSIZE);
#endif

#ifdef EMULTIHOP
    ERRNO_CASE(EMULTIHOP);
#endif

#ifdef ENAMETOOLONG
    ERRNO_CASE(ENAMETOOLONG);
#endif

#ifdef ENETDOWN
    ERRNO_CASE(ENETDOWN);
#endif

#ifdef ENETRESET
    ERRNO_CASE(ENETRESET);
#endif

#ifdef ENETUNREACH
    ERRNO_CASE(ENETUNREACH);
#endif

#ifdef ENFILE
    ERRNO_CASE(ENFILE);
#endif

#ifdef ENOBUFS
    ERRNO_CASE(ENOBUFS);
#endif

#ifdef ENODATA
    ERRNO_CASE(ENODATA);
#endif

#ifdef ENODEV
    ERRNO_CASE(ENODEV);
#endif

#ifdef ENOENT
    ERRNO_CASE(ENOENT);
#endif

#ifdef ENOEXEC
    ERRNO_CASE(ENOEXEC);
#endif

#ifdef ENOLINK
    ERRNO_CASE(ENOLINK);
#endif

#ifdef ENOLCK
#if ENOLINK != ENOLCK
    ERRNO_CASE(ENOLCK);
#endif
#endif

#ifdef ENOMEM
    ERRNO_CASE(ENOMEM);
#endif

#ifdef ENOMSG
    ERRNO_CASE(ENOMSG);
#endif

#ifdef ENOPROTOOPT
    ERRNO_CASE(ENOPROTOOPT);
#endif

#ifdef ENOSPC
    ERRNO_CASE(ENOSPC);
#endif

#ifdef ENOSR
    ERRNO_CASE(ENOSR);
#endif

#ifdef ENOSTR
    ERRNO_CASE(ENOSTR);
#endif

#ifdef ENOSYS
    ERRNO_CASE(ENOSYS);
#endif

#ifdef ENOTCONN
    ERRNO_CASE(ENOTCONN);
#endif

#ifdef ENOTDIR
    ERRNO_CASE(ENOTDIR);
#endif

#ifdef ENOTEMPTY
    ERRNO_CASE(ENOTEMPTY);
#endif

#ifdef ENOTSOCK
    ERRNO_CASE(ENOTSOCK);
#endif

#ifdef ENOTSUP
    ERRNO_CASE(ENOTSUP);
#else
#ifdef EOPNOTSUPP
    ERRNO_CASE(EOPNOTSUPP);
#endif
#endif

#ifdef ENOTTY
    ERRNO_CASE(ENOTTY);
#endif

#ifdef ENXIO
    ERRNO_CASE(ENXIO);
#endif

#ifdef EOVERFLOW
    ERRNO_CASE(EOVERFLOW);
#endif

#ifdef EPERM
    ERRNO_CASE(EPERM);
#endif

#ifdef EPIPE
    ERRNO_CASE(EPIPE);
#endif

#ifdef EPROTO
    ERRNO_CASE(EPROTO);
#endif

#ifdef EPROTONOSUPPORT
    ERRNO_CASE(EPROTONOSUPPORT);
#endif

#ifdef EPROTOTYPE
    ERRNO_CASE(EPROTOTYPE);
#endif

#ifdef ERANGE
    ERRNO_CASE(ERANGE);
#endif

#ifdef EROFS
    ERRNO_CASE(EROFS);
#endif

#ifdef ESPIPE
    ERRNO_CASE(ESPIPE);
#endif

#ifdef ESRCH
    ERRNO_CASE(ESRCH);
#endif

#ifdef ESTALE
    ERRNO_CASE(ESTALE);
#endif

#ifdef ETIME
    ERRNO_CASE(ETIME);
#endif

#ifdef ETIMEDOUT
    ERRNO_CASE(ETIMEDOUT);
#endif

#ifdef ETXTBSY
    ERRNO_CASE(ETXTBSY);
#endif

#ifdef EXDEV
    ERRNO_CASE(EXDEV);
#endif

    default:
      return "";
  }
}

const char *signo_string(int signo) {
#define SIGNO_CASE(e) \
  case e:             \
    return #e;
  switch (signo) {
#ifdef SIGHUP
    SIGNO_CASE(SIGHUP);
#endif

#ifdef SIGINT
    SIGNO_CASE(SIGINT);
#endif

#ifdef SIGQUIT
    SIGNO_CASE(SIGQUIT);
#endif

#ifdef SIGILL
    SIGNO_CASE(SIGILL);
#endif

#ifdef SIGTRAP
    SIGNO_CASE(SIGTRAP);
#endif

#ifdef SIGABRT
    SIGNO_CASE(SIGABRT);
#endif

#ifdef SIGIOT
#if SIGABRT != SIGIOT
    SIGNO_CASE(SIGIOT);
#endif
#endif

#ifdef SIGBUS
    SIGNO_CASE(SIGBUS);
#endif

#ifdef SIGFPE
    SIGNO_CASE(SIGFPE);
#endif

#ifdef SIGKILL
    SIGNO_CASE(SIGKILL);
#endif

#ifdef SIGUSR1
    SIGNO_CASE(SIGUSR1);
#endif

#ifdef SIGSEGV
    SIGNO_CASE(SIGSEGV);
#endif

#ifdef SIGUSR2
    SIGNO_CASE(SIGUSR2);
#endif

#ifdef SIGPIPE
    SIGNO_CASE(SIGPIPE);
#endif

#ifdef SIGALRM
    SIGNO_CASE(SIGALRM);
#endif

    SIGNO_CASE(SIGTERM);

#ifdef SIGCHLD
    SIGNO_CASE(SIGCHLD);
#endif

#ifdef SIGSTKFLT
    SIGNO_CASE(SIGSTKFLT);
#endif

#ifdef SIGCONT
    SIGNO_CASE(SIGCONT);
#endif

#ifdef SIGSTOP
    SIGNO_CASE(SIGSTOP);
#endif

#ifdef SIGTSTP
    SIGNO_CASE(SIGTSTP);
#endif

#ifdef SIGBREAK
    SIGNO_CASE(SIGBREAK);
#endif

#ifdef SIGTTIN
    SIGNO_CASE(SIGTTIN);
#endif

#ifdef SIGTTOU
    SIGNO_CASE(SIGTTOU);
#endif

#ifdef SIGURG
    SIGNO_CASE(SIGURG);
#endif

#ifdef SIGXCPU
    SIGNO_CASE(SIGXCPU);
#endif

#ifdef SIGXFSZ
    SIGNO_CASE(SIGXFSZ);
#endif

#ifdef SIGVTALRM
    SIGNO_CASE(SIGVTALRM);
#endif

#ifdef SIGPROF
    SIGNO_CASE(SIGPROF);
#endif

#ifdef SIGWINCH
    SIGNO_CASE(SIGWINCH);
#endif

#ifdef SIGIO
    SIGNO_CASE(SIGIO);
#endif

#ifdef SIGPOLL
#if SIGPOLL != SIGIO
    SIGNO_CASE(SIGPOLL);
#endif
#endif

#ifdef SIGLOST
    SIGNO_CASE(SIGLOST);
#endif

#ifdef SIGPWR
#if SIGPWR != SIGLOST
    SIGNO_CASE(SIGPWR);
#endif
#endif

#ifdef SIGSYS
    SIGNO_CASE(SIGSYS);
#endif

    default:
      return "";
  }
}
}  // namespace node
