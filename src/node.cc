// Copyright & License details are available under JXCORE_LICENSE file

#include "jx/error_definition.h"
#include "jx/extend.h"
#include "jxcore.h"
#include "jx/job.h"
#include "wrappers/memory_wrap.h"
#include "wrappers/handle_wrap.h"
#include "wrappers/thread_wrap.h"
#include "string_bytes.h"
#include "ares.h"

#ifdef JXCORE_EMBEDS_SQLITE
#include "sqlite3.h"
#endif

#include <limits.h> /* PATH_MAX */
#include <assert.h>
#if !defined(_MSC_VER)
#include <unistd.h> /* setuid, getuid */
#else
#include <direct.h>
#include <process.h>
#define getpid _getpid
#include <io.h>
#define umask _umask
typedef int mode_t;
#endif
#include <errno.h>
#include <sys/types.h>
#include "zlib.h"

#if defined(__POSIX__) || defined(__ANDROID__)
#include <pwd.h> /* getpwnam() */
#include <grp.h> /* getgrnam() */
#endif

#include "wrappers/node_buffer.h"
#include "wrappers/node_file.h"
#include "wrappers/node_http_parser.h"
#include "node_constants.h"
#include "node_javascript.h"
#include "node_version.h"
#if HAVE_OPENSSL
#include "node_crypto.h"
#endif
#if HAVE_SYSTEMTAP
#include "node_systemtap.h"
#endif
#include "node_script.h"

#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#elif !defined(_MSC_VER)
extern char** environ;
#endif

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

namespace node {

#ifdef OPENSSL_NPN_NEGOTIATED
static bool use_npn = true;
#else
static bool use_npn = false;
#endif

#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
static bool use_sni = true;
#else
static bool use_sni = false;
#endif

bool no_deprecation;

#if defined(__IOS__) || defined(__ANDROID__)
char* app_sandbox_folder = NULL;
#endif

int WRITE_UTF8_FLAGS = JS_ENGINE_WRITE_UTF8_FLAGS;

static void Spin(uv_idle_t* handle, int status) {
  node::commons* com = node::commons::getInstanceByThreadId(handle->threadId);
  if (com->instance_status_ == node::JXCORE_INSTANCE_EXITED ||
      com->expects_reset)
    return;

  uv_idle_t* t = com->tick_spinner;
  assert((uv_idle_t*)handle == t);
  assert(status == 0);

  if (!com->need_tick_cb) return;
  com->need_tick_cb = false;

  uv_idle_stop(t);

  JS_DEFINE_STATE_MARKER(com);

  if (JS_IS_EMPTY((com->process_tickFromSpinner))) {
    __JS_LOCAL_STRING tfs_str = JS_STRING_ID("_tickFromSpinner");
    JS_LOCAL_VALUE cb_v = JS_GET_NAME(com->getProcess(), tfs_str);
    if (!JS_IS_FUNCTION(cb_v)) {
      fprintf(stderr, "process._tickFromSpinner assigned to non-function\n");
      abort();
    }
    JS_LOCAL_FUNCTION cb = JS_TYPE_AS_FUNCTION(cb_v);
    JS_NEW_PERSISTENT_FUNCTION(com->process_tickFromSpinner, cb);
  }

  JS_TRY_CATCH(try_catch);

  JS_LOCAL_FUNCTION tfsl =
      JS_TYPE_TO_LOCAL_FUNCTION(com->process_tickFromSpinner);
  JS_METHOD_CALL_NO_PARAM(tfsl, com->getProcess());

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
}

static JS_LOCAL_METHOD(NeedTickCallback) {
  if (com->instance_status_ == node::JXCORE_INSTANCE_EXITED ||
      com->expects_reset)
    RETURN();

  com->need_tick_cb = true;
  uv_idle_t* t = com->tick_spinner;
  t->threadId = com->threadId;
  uv_idle_start(t, Spin);
}
JS_METHOD_END

static void CheckImmediate(uv_check_t* handle, int status) {
  node::commons* com = node::commons::getInstanceByThreadId(handle->threadId);
  if (com->instance_status_ == node::JXCORE_INSTANCE_EXITED ||
      com->expects_reset)
    return;

  assert(handle == com->check_immediate_watcher &&
         "CheckImmediate [1] assert failed at node.cc");
  assert(status == 0 && "CheckImmediate [2] assert failed at node.cc");

  JS_DEFINE_STATE_MARKER(com);
  JS_HANDLE_OBJECT process = com->getProcess();

  MakeCallback(com, process, JS_PREDEFINED_STRING(_immediateCallback), 0, NULL);
}

static void IdleImmediateDummy(uv_idle_t* handle, int status) {
  assert(status == 0 && "This should not be called!");
}

#ifdef JS_ENGINE_V8
// Just for the interface compatibility
JS_HANDLE_VALUE FromConstructorTemplate(JS_PERSISTENT_FUNCTION_TEMPLATE t,
                                        const JS_V8_ARGUMENT& args) {
  JS_ENTER_SCOPE_WITH(args.GetIsolate());
  JS_DEFINE_STATE_MARKER_(args.GetIsolate());

  JS_LOCAL_VALUE argv[32];
  unsigned argc = args.Length();
  if (argc > ARRAY_SIZE(argv)) argc = ARRAY_SIZE(argv);
  for (unsigned i = 0; i < argc; ++i) argv[i] = args[i];
  JS_LOCAL_FUNCTION_TEMPLATE ptcl = JS_TYPE_TO_LOCAL_FUNCTION_TEMPLATE(t);
  return JS_LEAVE_SCOPE(ptcl->GetFunction()->NewInstance(argc, argv));
}
#endif

JS_HANDLE_VALUE FromConstructorTemplateX(JS_HANDLE_FUNCTION_TEMPLATE t,
                                         jxcore::PArguments& args) {
  JS_ENTER_SCOPE_WITH(args.GetIsolate());
  JS_HANDLE_VALUE argv[32];
  unsigned argc = args.Length();
  if (argc > ARRAY_SIZE(argv)) argc = ARRAY_SIZE(argv);
  for (unsigned i = 0; i < argc; ++i) argv[i] = args.GetItem(i);
  return JS_LEAVE_SCOPE(JS_NEW_INSTANCE(JS_GET_CONSTRUCTOR(t), argc, argv));
}

JS_LOCAL_METHOD(UsingDomains) {
  if (com->instance_status_ == node::JXCORE_INSTANCE_EXITED ||
      com->using_domains || com->expects_reset)
    RETURN();

  com->using_domains = true;
  JS_HANDLE_OBJECT process = com->getProcess();
  __JS_LOCAL_STRING tdc_str = JS_STRING_ID("_tickDomainCallback");
  __JS_LOCAL_STRING ndt_str = JS_STRING_ID("_nextDomainTick");
  JS_LOCAL_VALUE tdc_v = JS_GET_NAME(process, tdc_str);
  JS_LOCAL_VALUE ndt_v = JS_GET_NAME(process, ndt_str);

  if (!JS_IS_FUNCTION(tdc_v)) {
    fprintf(stderr, "process._tickDomainCallback assigned to non-function\n");
    abort();
  }

  if (!JS_IS_FUNCTION(ndt_v)) {
    fprintf(stderr, "process._nextDomainTick assigned to non-function\n");
    abort();
  }

  JS_LOCAL_FUNCTION tdc = JS_TYPE_AS_FUNCTION(tdc_v);
  JS_LOCAL_FUNCTION ndt = JS_TYPE_AS_FUNCTION(ndt_v);
  __JS_LOCAL_STRING tc_str = JS_STRING_ID("_tickCallback");
  __JS_LOCAL_STRING cth_str = JS_STRING_ID("_currentTickHandler");
  JS_NAME_SET(process, tc_str, tdc);
  JS_NAME_SET(process, cth_str, ndt);

  JS_CLEAR_PERSISTENT(com->process_tickCallback);
  JS_NEW_PERSISTENT_FUNCTION(com->process_tickCallback, tdc);
}
JS_METHOD_END

// compatibility only;
JS_HANDLE_VALUE
MakeDomainCallback(const JS_HANDLE_OBJECT_REF object,
                   const JS_HANDLE_FUNCTION_REF callback, int argc,
                   JS_HANDLE_VALUE argv[]) {
  node::commons* com = node::commons::getInstance();
  return MakeDomainCallback(com, object, callback, argc, argv);
}

JS_HANDLE_VALUE
MakeDomainCallback(node::commons* com, const JS_HANDLE_OBJECT_REF object,
                   const JS_HANDLE_FUNCTION_REF callback, int argc,
                   JS_HANDLE_VALUE argv[]) {
  JS_DEFINE_STATE_MARKER(com);
  if (com == NULL || com->expects_reset) return JS_UNDEFINED();

  JS_LOCAL_VALUE domain_v;
  JS_LOCAL_OBJECT domain;
  JS_LOCAL_FUNCTION enter;
  JS_LOCAL_FUNCTION exit;

  JS_TRY_CATCH(try_catch);

  bool has_domain = false;
  if (com->using_domains) {
    domain_v = JS_GET_NAME(object, JS_PREDEFINED_STRING(domain));
    has_domain = JS_IS_OBJECT(domain_v);
    if (has_domain) {
      domain = JS_VALUE_TO_OBJECT(domain_v);
      assert(!JS_IS_EMPTY(domain));

      __JS_LOCAL_STRING d_str = JS_STRING_ID("_disposed");
      if (JS_IS_TRUE(JS_GET_NAME(domain, d_str))) {
        return JS_UNDEFINED();
      }

      __JS_LOCAL_STRING str_enter = JS_STRING_ID("enter");
      enter = JS_CAST_FUNCTION(JS_GET_NAME(domain, str_enter));
      assert(!JS_IS_EMPTY(enter));
      JS_METHOD_CALL_NO_PARAM(enter, domain);

      if (try_catch.HasCaught()) {
        FatalException(try_catch);
        return JS_UNDEFINED();
      }
    }
  }

  JS_LOCAL_VALUE ret = JS_METHOD_CALL(callback, object, argc, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
    return JS_UNDEFINED();
  }

  if (has_domain) {
    __JS_LOCAL_STRING str_exit = JS_STRING_ID("exit");
    exit = JS_CAST_FUNCTION(JS_GET_NAME(domain, str_exit));
    assert(!JS_IS_EMPTY(exit));
    JS_METHOD_CALL_NO_PARAM(exit, domain);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
      return JS_UNDEFINED();
    }
  }

  if (com->tick_infobox->length == 0) {
    com->tick_infobox->index = 0;
    com->tick_infobox->depth = 0;
    return ret;
  }

  JS_LOCAL_FUNCTION ptcl = JS_TYPE_TO_LOCAL_FUNCTION(com->process_tickCallback);
  JS_METHOD_CALL_NO_PARAM(ptcl, com->getProcess());

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
    return JS_UNDEFINED();
  }

  return ret;
}

JS_HANDLE_VALUE
MakeCallback(const JS_HANDLE_OBJECT_REF object,
             const JS_HANDLE_FUNCTION_REF callback, int argc,
             JS_HANDLE_VALUE argv[]) {
  return MakeCallback(node::commons::getInstance(), object, callback, argc,
                      argv);
}

void defineProcessCallbacks(node::commons* com) {
  JS_ENTER_SCOPE_WITH(com->node_isolate);

  JS_HANDLE_OBJECT prc = com->getProcess();
  JS_DEFINE_STATE_MARKER(com);
#ifdef JS_ENGINE_V8
  if (*prc == NULL) return;
#endif
  JS_LOCAL_VALUE cb_v = JS_GET_NAME(prc, JS_PREDEFINED_STRING(_tickCallback));
  if (!JS_IS_FUNCTION(cb_v)) {
    fprintf(stderr, "process._tickCallback assigned to non-function\n");
    abort();
  }

  JS_LOCAL_FUNCTION cb = JS_TYPE_AS_FUNCTION(cb_v);
  JS_NEW_PERSISTENT_FUNCTION(com->process_tickCallback, cb);
}

// TODO(obastemur) make this application wide
// MozJS Proxy supports the V8 MakeCallback versions without any change
// This is a hot zone, carrying/converting MozJS::Value array around is more
// expensive
#if defined(JS_ENGINE_MOZJS)
JS_HANDLE_VALUE
MakeCallback(node::commons* com, JS_HANDLE_OBJECT_REF host, const char* name,
             int argc, jsval argv[]) {
  JS_DEFINE_STATE_MARKER(com);
  if (com == NULL || com->expects_reset) return JS_UNDEFINED();

  if (com->using_domains) {
    if (argc > 0) {
      // TODO(obastemur) improve perf for domains
      MozJS::Value* args = (MozJS::Value*)malloc(sizeof(MozJS::Value) * (argc));
      for (int i = 0; i < argc; i++)
        args[i] = MozJS::Value(argv[i], JS_GET_STATE_MARKER());
      JS_HANDLE_VALUE ret_val =
          MakeCallback(com, host, STD_TO_STRING(name), argc, args);
      free(args);
      return ret_val;
    } else {
      return MakeCallback(com, host, STD_TO_STRING(name), 0, NULL);
    }
  }

  if (JS_IS_EMPTY((com->process_tickCallback))) {
    defineProcessCallbacks(com);
  }

  JS_TRY_CATCH(try_catch);

  JS_LOCAL_VALUE ret = host.Call(name, argc, argv);

  if (try_catch.HasCaught()) {
    if (!(com->expects_reset && com->threadId > 0)) {
      FatalException(try_catch);
    }
    return JS_UNDEFINED();
  }

  if (com->tick_infobox->length == 0) {
    com->tick_infobox->index = 0;
    com->tick_infobox->depth = 0;
    return ret;
  }

  JS_HANDLE_OBJECT pr = com->getProcess();
  JS::RootedValue rov(__contextORisolate);

  // We know for sure that we are in the right compartment
  jsval pr_val = pr.GetRawValue();
  jsval pt_val = com->process_tickCallback.GetRawValue();
  JS_CallFunctionValueJX(__contextORisolate, pr_val, pt_val,
                         JS::HandleValueArray::empty(), &rov);

  if (try_catch.HasCaught()) {
    if (!(com->expects_reset && com->threadId > 0)) {
      FatalException(try_catch);
    }
    return JS_UNDEFINED();
  }

  return ret;
}
#endif

JS_HANDLE_VALUE
MakeCallback(node::commons* com, const JS_HANDLE_OBJECT_REF object,
             const JS_HANDLE_FUNCTION_REF callback, int argc,
             JS_HANDLE_VALUE argv[]) {
  JS_DEFINE_STATE_MARKER(com);
  if (com == NULL || com->expects_reset) return JS_UNDEFINED();

  if (com->using_domains)
    return MakeDomainCallback(object, callback, argc, argv);

  if (JS_IS_EMPTY((com->process_tickCallback))) {
    defineProcessCallbacks(com);
  }

  JS_TRY_CATCH(try_catch);

  JS_LOCAL_VALUE ret = JS_METHOD_CALL(callback, object, argc, argv);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
    return JS_UNDEFINED();
  }

  if (com->tick_infobox->length == 0) {
    com->tick_infobox->index = 0;
    com->tick_infobox->depth = 0;
    return ret;
  }

  JS_HANDLE_OBJECT process_obj = com->getProcess();
  JS_LOCAL_FUNCTION ptc = JS_TYPE_TO_LOCAL_FUNCTION(com->process_tickCallback);
  JS_METHOD_CALL_NO_PARAM(ptc, process_obj);

  if (try_catch.HasCaught()) {
    FatalException(try_catch);
    return JS_UNDEFINED();
  }

  return ret;
}

JS_HANDLE_VALUE
MakeCallback(const JS_HANDLE_OBJECT_REF object, const JS_HANDLE_STRING symbol,
             int argc, JS_HANDLE_VALUE argv[]) {
  return MakeCallback(node::commons::getInstance(), object, symbol, argc, argv);
}

JS_HANDLE_VALUE
MakeCallback(node::commons* com, const JS_HANDLE_OBJECT_REF object,
             const char* symbol, int argc, JS_HANDLE_VALUE argv[]) {
  JS_DEFINE_STATE_MARKER(com);
  return MakeCallback(com, object, STD_TO_STRING(symbol), argc, argv);
}

JS_HANDLE_VALUE
MakeCallback(node::commons* com, const JS_HANDLE_OBJECT_REF object,
             const JS_HANDLE_STRING symbol, int argc, JS_HANDLE_VALUE argv[]) {
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);
  if (com == NULL || com->expects_reset) return JS_UNDEFINED();

  JS_LOCAL_FUNCTION callback = JS_TYPE_AS_FUNCTION(JS_GET_NAME(object, symbol));

  if (com->using_domains) {
    JS_LOCAL_VALUE objl = JS_TYPE_TO_LOCAL_VALUE(
        MakeDomainCallback(com, object, callback, argc, argv));
    return JS_LEAVE_SCOPE(objl);
  }

  return JS_HANDLE_VALUE(MakeCallback(com, object, callback, argc, argv));
}

JS_HANDLE_VALUE
MakeCallback(const JS_HANDLE_OBJECT_REF object, const char* method, int argc,
             JS_HANDLE_VALUE argv[]) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_HANDLE_VALUE ret =
      MakeCallback(com, object, STD_TO_STRING(method), argc, argv);

  return JS_HANDLE_VALUE(ret);
}

inline bool fast_compare(const char* org, const int org_length, const char* sc,
                         const char* uc, const int length) {
  if (org_length != length) return false;

  for (int i = 0; i < length; i++) {
    if (org[i] == sc[i] || org[i] == uc[i]) continue;
    return false;
  }

  return true;
}

enum encoding ParseEncoding(const char* encoding, const int length,
                            enum encoding _default) {
  if (length > 10) return _default;

  if (encoding[0] == 'u' || encoding[0] == 'U') {
    const char* _encoding = encoding + 1;
    if (fast_compare(_encoding, length - 1, "tf8", "TF8", 3)) {
      return UTF8;
    } else if (fast_compare(_encoding, length - 1, "tf-8", "TF-8", 4)) {
      return UTF8;
    } else if (fast_compare(_encoding, length - 1, "cs2", "CS2", 3)) {
      return UCS2;
    } else if (fast_compare(_encoding, length - 1, "cs-2", "CS-2", 4)) {
      return UCS2;
    } else if (fast_compare(_encoding, length - 1, "tf16le", "TF16LE", 6)) {
      return UCS2;
    } else if (fast_compare(_encoding, length - 1, "tf-16le", "TF-16LE", 7)) {
      return UCS2;
    } else {
      return _default;
    }
  }

  if (fast_compare(encoding, length, "ascii", "ASCII", 5)) {
    return ASCII;
  }

  if (encoding[0] == 'b' || encoding[0] == 'B') {
    const char* _encoding = encoding + 1;
    if (fast_compare(_encoding, length - 1, "ase64", "ASE64", 5)) {
      return BASE64;
    } else if (fast_compare(_encoding, length - 1, "inary", "INARY", 5)) {
      return BINARY;
    } else if (fast_compare(_encoding, length - 1, "uffer", "UFFER", 5)) {
      return BUFFER;
    } else {
      return _default;
    }
  }

  if (fast_compare(encoding, length, "hex", "HEX", 3)) {
    return HEX;
  } else if (fast_compare(encoding, length, "raw", "RAW", 3)) {
    if (no_deprecation) {
      fprintf(stderr,
              "'raw' (array of integers) has been removed. "
              "Use 'binary'.\n");
    }
    return BINARY;
  } else if (fast_compare(encoding, length, "raws", "RAWS", 4)) {
    if (no_deprecation) {
      fprintf(stderr,
              "'raws' encoding has been renamed to 'binary'. "
              "Please update your code.\n");
    }
    return BINARY;
  } else {
    return _default;
  }
}

enum encoding ParseEncoding(JS_HANDLE_VALUE encoding_v,
                            enum encoding _default) {
  ENGINE_LOG_THIS("node_c", "ParseEncoding");
  JS_ENTER_SCOPE();

  if (!JS_IS_STRING(encoding_v)) return _default;

  jxcore::JXString encoding(encoding_v);

  return ParseEncoding(*encoding, encoding.length(), _default);
}

JS_LOCAL_VALUE Encode(const void* buf, size_t len, enum encoding encoding) {
  ENGINE_LOG_THIS("node_cc", "Encode");
  return StringBytes::Encode(static_cast<const char*>(buf), len, encoding);
}

// Returns -1 if the handle was not valid for decoding
ssize_t DecodeBytes(JS_HANDLE_VALUE val, enum encoding encoding) {
  ENGINE_LOG_THIS("node_cc", "DecodeBytes");
  JS_ENTER_SCOPE();

  if (JS_IS_ARRAY(val)) {
    fprintf(stderr,
            "'raw' encoding (array of integers) has been removed. "
            "Use 'binary'.\n");
    assert(0);
    return -1;
  }

  return StringBytes::Size(val, encoding);
}

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

// Returns number of bytes written.
ssize_t DecodeWrite(char* buf, size_t buflen, JS_HANDLE_VALUE val,
                    enum encoding encoding) {
  ENGINE_LOG_THIS("node_cc", "DecodeWrite");
  JS_ENTER_SCOPE();

  if (JS_IS_ARRAY(val)) {
    fprintf(stderr, "'raw' encoding (array of integers) has been removed.\n");
    assert(0);
    return -1;
  }

  return StringBytes::Write(buf, buflen, val, encoding, NULL);
}

// Executes a str within the current context.
JS_LOCAL_VALUE ExecuteString(jxcore::JXString* source,
                             jxcore::JXString* filename) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_TRY_CATCH(try_catch);
  JS_LOCAL_VALUE result;
  JS_LOCAL_STRING source_ = STD_TO_STRING(*(*source));
  JS_LOCAL_STRING filename_ = STD_TO_STRING(*(*filename));

  JS_LOCAL_SCRIPT script = JS_SCRIPT_COMPILE(source_, filename_);
  if (JS_IS_EMPTY(script)) {
    ReportException(try_catch, true);
#ifndef JXCORE_EMBEDDED
    exit(3);
#else
    return JS_UNDEFINED();
#endif
  }
  result = JS_SCRIPT_RUN(script);

  if (try_catch.HasCaught()) {
    ReportException(try_catch, true);
#ifndef JXCORE_EMBEDDED
    exit(4);
#else
    return JS_UNDEFINED();
#endif
  }

  return JS_LEAVE_SCOPE(result);
}

static JS_LOCAL_METHOD(GetActiveRequests) {
  JS_LOCAL_ARRAY ary = JS_NEW_ARRAY();
  QUEUE* q = NULL;
  int i = 0;

  QUEUE* UU = &com->NQ->req;
  QUEUE_FOREACH(q, UU) {
    ReqWrap<uv_req_t>* w = container_of(q, ReqWrap<uv_req_t>, req_wrap_queue_);
    if (JS_IS_EMPTY((w->object_))) continue;
    JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(w->object_);
    JS_INDEX_SET(ary, i++, objl);
  }

  RETURN_POINTER(ary);
}
JS_METHOD_END

// Non-static, friend of HandleWrap. Could have been a HandleWrap method but
// implemented here for consistency with GetActiveRequests().
JS_LOCAL_METHOD(GetActiveHandles) {
  JS_LOCAL_ARRAY ary = JS_NEW_ARRAY();
  QUEUE* q = NULL;
  int i = 0;

  __JS_LOCAL_STRING owner_sym = JS_STRING_ID("owner");

  QUEUE* UU = &com->NQ->wrap;

  QUEUE_FOREACH(q, UU) {
    HandleWrap* w = container_of(q, HandleWrap, handle_wrap_queue_);
    if (JS_IS_EMPTY((w->object_)) || (w->flags_ & HandleWrap::kUnref)) continue;
    JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(w->object_);
    JS_LOCAL_VALUE val = JS_GET_NAME(objl, owner_sym);
    if (JS_IS_UNDEFINED(val)) val = JS_TYPE_TO_LOCAL_VALUE(objl);
    JS_INDEX_SET(ary, i++, val);
  }

  RETURN_POINTER(ary);
}
JS_METHOD_END

static JS_LOCAL_METHOD(Abort) {
#ifndef JXCORE_EMBEDDED
  abort();
#else
  THROW_EXCEPTION("You may not call abort when JXcore is embedded");
#endif
}
JS_METHOD_END

static JS_LOCAL_METHOD(Chdir) {
  if (args.Length() != 1 || !args.IsString(0)) {
    THROW_EXCEPTION("Bad argument. (expects string)");
  }

  jxcore::JXString path;
  args.GetString(0, &path);

  uv_err_t r = uv_chdir(*path);

  if (r.code != UV_OK) {
    JS_LOCAL_VALUE err = UVException(r.code, "uv_chdir", "", "");
    THROW_EXCEPTION_OBJECT(err);
  }
}
JS_METHOD_END

static JS_LOCAL_METHOD(Cwd) {
#ifdef _WIN32
  /* MAX_PATH is in characters, not bytes. Make sure we have enough headroom. */
  char buf[MAX_PATH * 4 + 1];
#else
  char buf[PATH_MAX + 1];
#endif

  size_t len = sizeof(buf);
  // keep uv_cwd from multi-thread access
  customLock(CSLOCK_UVFS);
  uv_err_t r = uv_cwd(buf, len);
  customUnlock(CSLOCK_UVFS);

  if (r.code != UV_OK) {
    JS_LOCAL_VALUE err = UVException(r.code, "uv_cwd", "", "");
    THROW_EXCEPTION_OBJECT(err);
  }

  buf[ARRAY_SIZE(buf) - 1] = '\0';

#if defined(__IOS__) || defined(__ANDROID__)
  // we do not know the embedded environment. test result

  if (strcmp(buf, "/") == 0) {
    if (args.Length() != 0)
      error_console(
          "Warning! You can not set/relate the home folder on this platform\n");

    RETURN_PARAM(UTF8_TO_STRING(app_sandbox_folder));
  }
#endif

  RETURN_PARAM(UTF8_TO_STRING(buf));
}
JS_METHOD_END

static JS_LOCAL_METHOD(Umask) {
  unsigned int old;

  if (args.IsUndefined(0)) {
    old = umask(0);
    umask((mode_t)old);

  } else if (!args.IsInteger(0) && !args.IsString(0)) {
    THROW_TYPE_EXCEPTION("argument must be an integer or octal string.");
  } else {
    int oct;
    if (args.IsInteger(0)) {
      oct = args.GetUInteger(0);
    } else {
      oct = 0;
      jxcore::JXString str_;
      int ln = args.GetString(0, &str_);
      const char* str = *str_;

      // Parse the octal string.
      for (int i = 0; i < ln; i++) {
        char c = *(str + i);
        if (c > '7' || c < '0') {
          THROW_TYPE_EXCEPTION("invalid octal string");
        }
        oct *= 8;
        oct += c - '0';
      }
    }
    old = umask(static_cast<mode_t>(oct));
  }

  RETURN_PARAM(STD_TO_INTEGER(old));
}
JS_METHOD_END

#ifdef __POSIX__

static const uid_t uid_not_found = static_cast<uid_t>(-1);
static const gid_t gid_not_found = static_cast<gid_t>(-1);

static uid_t uid_by_name(const char* name) {
  struct passwd pwd;
  struct passwd* pp;
  char buf[8192];

  errno = 0;
  pp = NULL;
#ifndef __MOBILE_OS__  // MISSING FROM BIONIC
  if (getpwnam_r(name, &pwd, buf, sizeof(buf), &pp) == 0 && pp != NULL) {
    return pp->pw_uid;
  }
#endif

  return uid_not_found;
}

static char* name_by_uid(uid_t uid) {
  struct passwd pwd;
  struct passwd* pp;
  char buf[8192];
  int rc;

  errno = 0;
  pp = NULL;
#ifndef __MOBILE_OS__  // MISSING FROM BIONIC
  if ((rc = getpwuid_r(uid, &pwd, buf, sizeof(buf), &pp)) == 0 && pp != NULL) {
    return strdup(pp->pw_name);
  }
#endif

  if (rc == 0) {
    errno = ENOENT;
  }

  return NULL;
}

static gid_t gid_by_name(const char* name) {
  struct group pwd;
  struct group* pp;
  char buf[8192];

  errno = 0;
  pp = NULL;
#ifndef __MOBILE_OS__  // MISSING FROM BIONIC
  if (getgrnam_r(name, &pwd, buf, sizeof(buf), &pp) == 0 && pp != NULL) {
    return pp->gr_gid;
  }
#endif

  return gid_not_found;
}

static uid_t uid_by_name(JS_HANDLE_VALUE value) {
  if (JS_IS_UINT32(value)) {
    return static_cast<uid_t>(UINT32_TO_STD(value));
  } else {
    jxcore::JXString name(value);
    return uid_by_name(*name);
  }
}

static gid_t gid_by_name(JS_HANDLE_VALUE value) {
  if (JS_IS_UINT32(value)) {
    return static_cast<gid_t>(UINT32_TO_STD(value));
  } else {
    jxcore::JXString name(value);
    return gid_by_name(*name);
  }
}

static JS_LOCAL_METHOD(GetUid) {
  uid_t uid = getuid();
  RETURN_PARAM(STD_TO_UNSIGNED(uid));
}
JS_METHOD_END

static JS_LOCAL_METHOD(GetGid) {
  gid_t gid = getgid();
  RETURN_PARAM(STD_TO_UNSIGNED(gid));
}
JS_METHOD_END

static JS_LOCAL_METHOD(SetGid) {
  if (!args.IsInteger(0) && !args.IsString(0)) {
    THROW_TYPE_EXCEPTION("setgid argument must be a number or a string");
  }

  gid_t gid = gid_by_name(GET_ARG(0));

  if (gid == gid_not_found) {
    THROW_EXCEPTION("setgid group id does not exist");
  }

  if (setgid(gid)) {
    JS_LOCAL_VALUE err = ErrnoException(errno, "setgid");
    THROW_EXCEPTION_OBJECT(err);
  }
}
JS_METHOD_END

static JS_LOCAL_METHOD(SetUid) {
  if (!args.IsInteger(0) && !args.IsString(0)) {
    THROW_TYPE_EXCEPTION("setuid argument must be a number or a string");
  }

  uid_t uid = uid_by_name(GET_ARG(0));

  if (uid == uid_not_found) {
    THROW_EXCEPTION("setuid user id does not exist");
  }

  if (setuid(uid)) {
    JS_LOCAL_VALUE err_val = ErrnoException(errno, "setuid");
    THROW_EXCEPTION_OBJECT(err_val);
  }
}
JS_METHOD_END

static JS_LOCAL_METHOD(GetGroups) {
  int ngroups = getgroups(0, NULL);

  if (ngroups == -1) {
    JS_LOCAL_VALUE err_val_ = ErrnoException(errno, "getgroups");
    THROW_EXCEPTION_OBJECT(err_val_);
  }

  gid_t* groups = new gid_t[ngroups];

  ngroups = getgroups(ngroups, groups);

  if (ngroups == -1) {
    delete[] groups;
    JS_LOCAL_VALUE err_val = ErrnoException(errno, "getgroups");
    THROW_EXCEPTION_OBJECT(err_val);
  }

  JS_LOCAL_ARRAY groups_list = JS_NEW_ARRAY_WITH_COUNT(ngroups);
  bool seen_egid = false;
  gid_t egid = getegid();

  for (int i = 0; i < ngroups; i++) {
    JS_INDEX_SET(groups_list, i, STD_TO_INTEGER(groups[i]));
    if (groups[i] == egid) seen_egid = true;
  }

  delete[] groups;

  if (seen_egid == false) {
    JS_INDEX_SET(groups_list, ngroups, STD_TO_INTEGER(egid));
  }

  RETURN_POINTER(groups_list);
}
JS_METHOD_END

static JS_LOCAL_METHOD(SetGroups) {
  if (!args.IsArray(0)) {
    THROW_TYPE_EXCEPTION("argument 1 must be an array");
  }

  JS_HANDLE_ARRAY groups_list = args.GetAsArray(0);
  size_t size = JS_GET_ARRAY_LENGTH(groups_list);
  gid_t* groups = new gid_t[size];

  for (size_t i = 0; i < size; i++) {
    gid_t gid = gid_by_name(JS_GET_INDEX(groups_list, i));

    if (gid == gid_not_found) {
      delete[] groups;
      THROW_EXCEPTION("group name not found");
    }

    groups[i] = gid;
  }

  int rc = setgroups(size, groups);
  delete[] groups;

  if (rc == -1) {
    JS_LOCAL_VALUE err = ErrnoException(errno, "setgroups");
    THROW_EXCEPTION_OBJECT(err);
  }
}
JS_METHOD_END

static JS_LOCAL_METHOD(InitGroups) {
  if (!args.IsUnsigned(0) && !args.IsString(0)) {
    THROW_TYPE_EXCEPTION("argument 1 must be a number or a string");
  }

  if (!args.IsUnsigned(1) && !args.IsString(1)) {
    THROW_TYPE_EXCEPTION("argument 2 must be a number or a string");
  }

  JS_LOCAL_VALUE arg0_val = GET_ARG(0);
  jxcore::JXString arg0(arg0_val);
  gid_t extra_group;
  bool must_free;
  char* user;

  if (args.IsUnsigned(0)) {
    user = name_by_uid(args.GetUInteger(0));
    must_free = true;
  } else {
    user = *arg0;
    must_free = false;
  }

  if (user == NULL) {
    THROW_EXCEPTION("initgroups user not found");
  }

  extra_group = gid_by_name(GET_ARG(1));

  if (extra_group == gid_not_found) {
    if (must_free) free(user);
    THROW_EXCEPTION("initgroups extra group not found");
  }

  int rc = initgroups(user, extra_group);

  if (must_free) {
    free(user);
  }

  if (rc) {
    JS_LOCAL_VALUE err = ErrnoException(errno, "initgroups");
    THROW_EXCEPTION_OBJECT(err);
  }
}
JS_METHOD_END

#endif  // __POSIX__

JS_LOCAL_METHOD(Exit) {
// if this is an embedded instance we shouldn't terminate the process
// TODO(obastemur) apply this rule to other places
#ifndef JXCORE_EMBEDDED
  ENGINE_PRINT_LOGS();
  exit(args.GetInt32(0));
#else
  com->expects_reset = com->threadId > 0;
  JS_TERMINATE_EXECUTION(com->threadId);
  uv_stop(com->loop);
#endif
}
JS_METHOD_END

static JS_LOCAL_METHOD(Uptime) {
  double uptime;

  node::commons* mainNode = node::commons::getInstanceByThreadId(0);
  uv_update_time(mainNode->loop);
  uptime = uv_now(mainNode->loop) - mainNode->prog_start_time;

  RETURN_PARAM(STD_TO_INTEGER(uptime / 1000));
}
JS_METHOD_END

JS_LOCAL_METHOD(MemoryUsage) {
  size_t rss;

  uv_err_t err = uv_resident_set_memory(&rss);

  if (err.code != UV_OK) {
    JS_LOCAL_VALUE err_val =
        UVException(err.code, "uv_resident_set_memory", "", "");
    THROW_EXCEPTION_OBJECT(err_val);
  }

  JS_LOCAL_OBJECT info = JS_NEW_EMPTY_OBJECT();

  JS_NAME_SET(info, JS_STRING_ID("rss"), STD_TO_NUMBER(rss));

  // V8 memory usage
  size_t total_heap_size = 0, used_heap_size = 0;
#ifdef JS_ENGINE_V8
  v8::HeapStatistics v8_heap_stats;
  JS_GET_HEAP_STATICS(&v8_heap_stats);
  total_heap_size = v8_heap_stats.total_heap_size();
  used_heap_size = v8_heap_stats.used_heap_size();
#elif defined(JS_ENGINE_MOZJS)
// TODO(obastemur) get heap statics
#endif

  JS_NAME_SET(info, JS_STRING_ID("heapTotal"),
              STD_TO_UNSIGNED(total_heap_size));
  JS_NAME_SET(info, JS_STRING_ID("heapUsed"), STD_TO_UNSIGNED(used_heap_size));

  RETURN_POINTER(info);
}
JS_METHOD_END

JS_LOCAL_METHOD(Kill) {
  if (args.Length() != 2) {
    THROW_EXCEPTION("Bad argument.");
  }

  int pid = args.GetInteger(0);
  int sig = args.GetInteger(1);
  uv_err_t err = uv_kill(pid, sig);

  if (err.code != UV_OK) {
    SetErrno(err);
    RETURN_PARAM(STD_TO_INTEGER(-1));
  }
}
JS_METHOD_END

// used in Hrtime() below
#define NANOS_PER_SEC 1000000000

// Hrtime exposes libuv's uv_hrtime() high-resolution timer.
// The value returned by uv_hrtime() is a 64-bit int representing nanoseconds,
// so this function instead returns an Array with 2 entries representing seconds
// and nanoseconds, to avoid any integer overflow possibility.
// Pass in an Array from a previous hrtime() call to instead get a time diff.
JS_LOCAL_METHOD(Hrtime) {
  uint64_t t = uv_hrtime();

  if (args.Length() > 0) {
    // return a time diff tuple
    if (!args.IsArray(0)) {
      THROW_TYPE_EXCEPTION("process.hrtime() only accepts an Array tuple.");
    }

    JS_HANDLE_ARRAY inArray = args.GetAsArray(0);
    uint64_t seconds = UINT32_TO_STD(JS_GET_INDEX(inArray, 0));
    uint64_t nanos = UINT32_TO_STD(JS_GET_INDEX(inArray, 1));
    t -= (seconds * NANOS_PER_SEC) + nanos;
  }

  JS_LOCAL_ARRAY tuple = JS_NEW_ARRAY_WITH_COUNT(2);
  JS_INDEX_SET(tuple, 0, STD_TO_UNSIGNED(t / NANOS_PER_SEC));
  JS_INDEX_SET(tuple, 1, STD_TO_UNSIGNED(t % NANOS_PER_SEC));

  RETURN_POINTER(tuple);
}
JS_METHOD_END

typedef void(UV_DYNAMIC* extInit)(JS_HANDLE_OBJECT exports);

// DLOpen is process.dlopen(module, filename).
// Used to load 'module.node' dynamically shared objects.
JS_LOCAL_METHOD(DLOpen) {
  char symbol[1024], *pos;
  uv_lib_t lib;
  int r;

  if (args.Length() < 2) {
    THROW_EXCEPTION("process.dlopen takes exactly 2 arguments.");
  }

  JS_LOCAL_OBJECT module = JS_VALUE_TO_OBJECT(GET_ARG(0));
  jxcore::JXString filename;
  int ln = args.GetString(1, &filename);

  if (!node::commons::AllowLocalNativeModules()) {
    std::string gpath(node::commons::GetGlobalModulePath());
    bool search_global = false;
    if (ln <= gpath.length()) {
      search_global = true;
    } else {
      std::string filenames(*filename);
      std::string sub_str = filenames.substr(0, gpath.length());
      search_global =
          sub_str.compare(gpath) != 0;  // TODO(obastemur) windows casing
    }

    if (search_global) RETURN_PARAM(UTF8_TO_STRING(gpath.c_str()));
  }

  JS_LOCAL_OBJECT exports =
      JS_VALUE_TO_OBJECT(JS_GET_NAME(module, JS_PREDEFINED_STRING(exports)));

  if (uv_dlopen(*filename, &lib)) {
    std::string errmsg(uv_dlerror(&lib));
#ifdef _WIN32
    // Windows needs to add the filename into the error message
    errmsg += *filename;
#endif
    THROW_EXCEPTION(errmsg.c_str());
  }

  char* _base = (char*)malloc(sizeof(char) * (ln + 1));
  memcpy(_base, *filename, ln);

  _base[ln] = '\0';

  char* base = _base;
/* Find the shared library filename within the full path. */
#ifdef __POSIX__
  pos = strrchr(base, '/');
  if (pos != NULL) {
    base = pos + 1;
  }
#else  // Windows
  for (;;) {
    pos = strpbrk(base, "\\/:");
    if (pos == NULL) {
      break;
    }
    base = pos + 1;
  }
#endif

  /* Strip the .node extension. */
  pos = strrchr(base, '.');
  if (pos != NULL) {
    *pos = '\0';
  }

  /* Add the `_module` suffix to the extension name. */
  r = snprintf(symbol, sizeof symbol, "%s_module", base);
  if (r <= 0 || static_cast<size_t>(r) >= sizeof symbol) {
    free(_base);
    THROW_EXCEPTION("Out of memory.");
  }

  /* Replace dashes with underscores. When loading foo-bar.node,
   * look for foo_bar_module, not foo-bar_module.
   */
  for (pos = symbol; *pos != '\0'; ++pos) {
    if (*pos == '-') *pos = '_';
  }

  node_module_struct* mod;
  if (uv_dlsym(&lib, symbol, reinterpret_cast<void**>(&mod))) {
    char errmsg[1024];
    snprintf(errmsg, sizeof(errmsg), "Symbol %s not found.", symbol);
    free(_base);
    THROW_EXCEPTION(errmsg);
  }

  if (mod->version != NODE_MODULE_VERSION) {
    char errmsg[1024];
    snprintf(errmsg, sizeof(errmsg),
             "Module version mismatch. Expected %d, got %d.",
             NODE_MODULE_VERSION, mod->version);
    free(_base);
    THROW_EXCEPTION(errmsg);
  }

  // Execute the C++ module
  mod->register_func(exports, module);

  free(_base);
}
JS_METHOD_END

static JS_LOCAL_METHOD(JXBinding) {
  JS_LOCAL_STRING module = JS_VALUE_TO_STRING(GET_ARG(0));
  jxcore::JXString module_v(module);
  node_module_struct* modp;

  if (JS_IS_EMPTY((com->binding_cache))) {
    JS_NEW_EMPTY_PERSISTENT_OBJECT(com->binding_cache);
  }

  JS_LOCAL_OBJECT exports;

  JS_LOCAL_OBJECT objbc = JS_OBJECT_FROM_PERSISTENT(com->binding_cache);
  if (JS_HAS_NAME(objbc, module)) {
    exports = JS_VALUE_TO_OBJECT(JS_GET_NAME(objbc, module));
    RETURN_POINTER(exports);
  }

  // Append a string to process.moduleLoadList
  char buf[1024];
  snprintf(buf, sizeof(buf), "Binding %s", *module_v);
  JS_LOCAL_ARRAY objml = JS_TYPE_TO_LOCAL_ARRAY(com->module_load_list);
  uint32_t l = JS_GET_ARRAY_LENGTH(objml);
  JS_LOCAL_STRING str_buf = STD_TO_STRING(buf);
  JS_INDEX_SET(objml, l, str_buf);

  if ((modp = get_builtin_module(*module_v)) != NULL) {
    exports = JS_NEW_EMPTY_OBJECT();
    modp->register_func(exports, JS_UNDEFINED());
    JS_NAME_SET(objbc, module, exports);

  } else if (!strcmp(*module_v, "constants")) {
    exports = JS_NEW_EMPTY_OBJECT();
    DefineConstants(exports);
    JS_NAME_SET(objbc, module, exports);

  } else if (!strcmp(*module_v, "natives")) {
    exports = JS_NEW_EMPTY_OBJECT();
    int skip = 0;
    if (args.Length() > 1) {
      skip = args.GetInteger(1);
    }

    if (skip == 1) {
      JXDefineJavaScript();
      RETURN();
    } else {
      DefineJavaScript(exports);
      JS_NAME_SET(objbc, module, exports);
    }
  } else {
    std::string message = "No such module (";
    message += *module_v;
    message += ")";
    THROW_EXCEPTION(message.c_str());
  }

  RETURN_POINTER(exports);
}
JS_METHOD_END

static JS_GETTER_METHOD(ProcessTitleGetter) {
  char buffer[512];
  uv_get_process_title(buffer, sizeof(buffer));
  RETURN_GETTER_PARAM(UTF8_TO_STRING(buffer));
}
JS_GETTER_METHOD_END

static JS_SETTER_METHOD(ProcessTitleSetter) {
  jxcore::JXString title(value);

  auto_lock locker_(CSLOCK_UVFS);
  uv_set_process_title(*title);
}
JS_SETTER_METHOD_END

static JS_GETTER_METHOD(EnvGetter) {
#ifdef __POSIX__
  jxcore::JXString key(property);
  const char* val = getenv(*key);

  if (val) {
    RETURN_GETTER_PARAM(UTF8_TO_STRING(val));
  }
#else  // _WIN32
#ifdef JS_ENGINE_V8
  v8::String::Value key(property);
  const WCHAR* key_ptr = reinterpret_cast<const WCHAR*>(*key);
#elif defined(JS_ENGINE_MOZJS)
  jxcore::JXString key(property);
  WCHAR* key_ptr = (WCHAR*)malloc(sizeof(WCHAR) * (key.length() + 1));
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, *key, -1, key_ptr,
                      key.length() + 1);
#endif
  WCHAR buffer[32767];  // The maximum size allowed for environment variables.

  DWORD result = GetEnvironmentVariableW(key_ptr, buffer, ARRAY_SIZE(buffer));
#ifdef JS_ENGINE_MOZJS
  free(key_ptr);
#endif
  // If result >= sizeof buffer the buffer was too small. That should never
  // happen. If result == 0 and result != ERROR_SUCCESS the variable was not
  // not found.
  if ((result > 0 || GetLastError() == ERROR_SUCCESS) &&
      result < ARRAY_SIZE(buffer)) {
    JS_LOCAL_STRING str_result =
        UTF8_TO_STRING_WITH_LENGTH(reinterpret_cast<uint16_t*>(buffer), result);
    RETURN_GETTER_PARAM(str_result);
  }
#endif
// Not found.  Fetch from prototype.
#ifdef JS_ENGINE_V8
  RETURN_GETTER_PARAM(___info.Data().As<v8::Object>()->Get(property));
#endif
}
JS_GETTER_METHOD_END

static JS_SETADD_METHOD(EnvSetter) {
#ifdef __POSIX__
  jxcore::JXString key(property);
  jxcore::JXString val(value);
  setenv(*key, *val, 1);
#else  // _WIN32
#ifdef JS_ENGINE_V8
  v8::String::Value key(property);
  v8::String::Value val(value);
  const WCHAR* key_ptr = reinterpret_cast<const WCHAR*>(*key);
  const WCHAR* val_ptr = reinterpret_cast<const WCHAR*>(*val);
#elif defined(JS_ENGINE_MOZJS)
  jxcore::JXString key;
  key.SetFromHandle(property, true);
  jxcore::JXString val;
  val.SetFromHandle(value, true);

  WCHAR* key_ptr = (WCHAR*)malloc(sizeof(WCHAR) * (key.length() + 1));
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, *key, -1, key_ptr,
                      key.length() + 1);

  WCHAR* val_ptr = (WCHAR*)malloc(sizeof(WCHAR) * (val.length() + 1));
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, *val, -1, val_ptr,
                      val.length() + 1);
#endif
  // Environment variables that start with '=' are read-only.
  if (key_ptr[0] != L'=') {
    SetEnvironmentVariableW(key_ptr, val_ptr);
  }
#ifdef JS_ENGINE_MOZJS
  free(key_ptr);
  free(val_ptr);
#endif
#endif
}
JS_SETADD_METHOD_END

JS_DELETER_METHOD(EnvDeleter) {
#ifdef __POSIX__
  jxcore::JXString key(property);
  bool rv = true;
  rv = getenv(*key) != NULL;
  if (rv)
    unsetenv(*key);  // can't check return value, it's void on some platforms
#else
#ifdef JS_ENGINE_V8
  v8::String::Value key(property);
  const WCHAR* key_ptr = reinterpret_cast<const WCHAR*>(*key);
#elif defined(JS_ENGINE_MOZJS)
  jxcore::JXString key(property);
  WCHAR* key_ptr = (WCHAR*)malloc(sizeof(WCHAR) * (key.length() + 1));
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, *key, -1, key_ptr,
                      key.length() + 1);
#endif
  bool rv = false;
  if (key_ptr[0] == L'=' || !SetEnvironmentVariableW(key_ptr, NULL)) {
    // Deletion failed. Return true if the key wasn't there in the first place,
    // false if it is still there.
    rv = GetEnvironmentVariableW(key_ptr, NULL, NULL) == 0 &&
         GetLastError() != ERROR_SUCCESS;
  }

#ifdef JS_ENGINE_MOZJS
  free(key_ptr);
#endif
#endif

  if (rv) RETURN_DELETER_TRUE();
}
JS_DELETER_METHOD_END

#ifdef JS_ENGINE_V8
#ifdef V8_IS_3_14
static JS_HANDLE_INTEGER EnvQuery(JS_LOCAL_STRING property,
                                  const JS_V8_PROPERTY_ARGS& ___info) {
#else
static void EnvQuery(v8::Local<v8::String> property,
                     const v8::PropertyCallbackInfo<v8::Integer>& ___info) {
#endif
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  int32_t rc = -1;
#ifdef __POSIX__
  jxcore::JXString key(property);
  if (getenv(*key)) rc = 0;
#else  // _WIN32
  v8::String::Value key(property);
  WCHAR* key_ptr = reinterpret_cast<WCHAR*>(*key);
  if (GetEnvironmentVariableW(key_ptr, NULL, 0) > 0 ||
      GetLastError() == ERROR_SUCCESS) {
    rc = 0;
    if (key_ptr[0] == L'=') {
      // Environment variables that start with '=' are hidden and read-only.
      rc = static_cast<int32_t>(v8::ReadOnly) |
           static_cast<int32_t>(v8::DontDelete) |
           static_cast<int32_t>(v8::DontEnum);
    }
  }
#endif
  // Not found
  if (rc != -1) {
    RETURN_GETTER_PARAM(STD_TO_INTEGER(rc));
  }

  RETURN_GETTER_PARAM(JS_HANDLE_INTEGER_);
}

#ifdef V8_IS_3_14
static JS_HANDLE_ARRAY EnvEnumerator(const JS_V8_PROPERTY_ARGS& ___info) {
#else
static void EnvEnumerator(const v8::PropertyCallbackInfo<v8::Array>& ___info) {
#endif
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
#ifdef __POSIX__
  int size = 0;
  while (environ[size]) size++;

  JS_LOCAL_ARRAY env = JS_NEW_ARRAY_WITH_COUNT(size);

  for (int i = 0; i < size; ++i) {
    const char* var = environ[i];
    const char* s = strchr(var, '=');
    const int length = s ? s - var : strlen(var);
    JS_LOCAL_STRING var_str = STD_TO_STRING_WITH_LENGTH(var, length);
    JS_INDEX_SET(env, i, var_str);
  }
#else  // _WIN32
  WCHAR* environment = GetEnvironmentStringsW();
  if (environment == NULL) {
    // This should not happen.
    RETURN_GETTER_PARAM(JS_NEW_ARRAY());
  }
  JS_LOCAL_ARRAY env = JS_NEW_ARRAY();
  WCHAR* p = environment;
  int i = 0;
  while (*p != NULL) {
    WCHAR* s;
    if (*p == L'=') {
      // If the key starts with '=' it is a hidden environment variable.
      p += wcslen(p) + 1;
      continue;
    } else {
      s = wcschr(p, L'=');
    }
    if (!s) {
      s = p + wcslen(p);
    }
    JS_LOCAL_STRING ps_str =
        STD_TO_STRING_WITH_LENGTH(reinterpret_cast<uint16_t*>(p), s - p);
    JS_INDEX_SET(env, i++, ps_str);
    p = s + wcslen(s) + 1;
  }
  FreeEnvironmentStringsW(environment);
#endif

  RETURN_GETTER_PARAM(env);
}

#elif defined(JS_ENGINE_MOZJS)
static bool EnvQuery(JSContext* __contextORisolate, JS::HandleObject ___obj,
                     JS::HandleId ___id) {
  JS::RootedValue rv___idval(JS_GET_STATE_MARKER());
  JS_IdToValue(JS_GET_STATE_MARKER(), ___id, &rv___idval);
  JS::Value ___idval = rv___idval;
  MozJS::Value property(___idval, JS_GET_STATE_MARKER());
  jxcore::JXString key(property);
#ifdef __POSIX__
  const char* val = getenv(*key);

  if (val) {
    MozJS::Value rval(___obj, JS_GET_STATE_MARKER());
    JS_LOCAL_STRING sval = UTF8_TO_STRING(val);
    JS_NAME_SET(rval, *key, sval);
  }
#else  // _WIN32
  WCHAR buffer[32767];  // The maximum size allowed for environment variables.
  WCHAR* temp_name = (WCHAR*)malloc(sizeof(WCHAR) * (key.length() + 1));
  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, *key, -1, temp_name,
                      key.length() + 1);

  DWORD result = GetEnvironmentVariableW(temp_name, buffer, ARRAY_SIZE(buffer));

  free(temp_name);
  // If result >= sizeof buffer the buffer was too small. That should never
  // happen. If result == 0 and result != ERROR_SUCCESS the variable was not
  // not found.
  if ((result > 0 || GetLastError() == ERROR_SUCCESS) &&
      result < ARRAY_SIZE(buffer)) {
    MozJS::Value rval(___obj, JS_GET_STATE_MARKER());
    MozJS::String sval =
        UTF8_TO_STRING_WITH_LENGTH(reinterpret_cast<uint16_t*>(buffer), result);
    JS_NAME_SET(rval, *key, sval);
  }
#endif
  return true;
}

static bool EnvEnumerator(JSContext* cx, JS::HandleObject obj) {
  JS_DEFINE_STATE_MARKER_(cx);
  MozJS::Value env(obj, cx);

#ifdef __POSIX__
  int size = 0;
  while (environ[size]) size++;

  for (int i = 0; i < size; ++i) {
    const char* var = environ[i];
    const char* s = strchr(var, '=');
    const int length = s ? s - var : strlen(var);
    JS_LOCAL_STRING key = STD_TO_STRING_WITH_LENGTH(var, length);
    jxcore::JXString jkey(key);
    const char* val = getenv(*jkey);
    JS_NAME_SET(env, key, UTF8_TO_STRING(val));
  }
  return true;
#else  // WIN
  WCHAR* environment = GetEnvironmentStringsW();
  if (environment == NULL) {
    // This should not happen.
    return true;
  }
  WCHAR* p = environment;
  int i = 0;
  while (*p != NULL) {
    WCHAR* s;
    if (*p == L'=') {
      // If the key starts with '=' it is a hidden environment variable.
      p += wcslen(p) + 1;
      continue;
    } else {
      s = wcschr(p, L'=');
    }
    if (!s) {
      s = p + wcslen(p);
    }
    JS_LOCAL_STRING ps_str =
        UTF8_TO_STRING_WITH_LENGTH(reinterpret_cast<uint16_t*>(p), s - p);

    const int tmp_len = 1 + (s - p);
    WCHAR* temp_name = (WCHAR*)malloc(tmp_len * sizeof(WCHAR));
    memcpy(temp_name, p, (tmp_len - 1) * sizeof(WCHAR));
    temp_name[tmp_len - 1] = WCHAR(0);

    WCHAR buffer[32767];  // The maximum size allowed for environment variables.
    DWORD result =
        GetEnvironmentVariableW(temp_name, buffer, ARRAY_SIZE(buffer));

    free(temp_name);
    // If result >= sizeof buffer the buffer was too small. That should never
    // happen. If result == 0 and result != ERROR_SUCCESS the variable was not
    // not found.
    if ((result > 0 || GetLastError() == ERROR_SUCCESS) &&
        result < ARRAY_SIZE(buffer)) {
      JS_NAME_SET(env, ps_str,
                  UTF8_TO_STRING_WITH_LENGTH(
                      reinterpret_cast<uint16_t*>(buffer), result));
    }
    p = s + wcslen(s) + 1;
  }
  FreeEnvironmentStringsW(environment);
  return true;
#endif
}
#endif

static JS_HANDLE_OBJECT GetFeatures() {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
  JS_LOCAL_BOOLEAN val
#if defined(DEBUG) && DEBUG
      = STD_TO_BOOLEAN(true);
#else
      = STD_TO_BOOLEAN(false);
#endif
  JS_NAME_SET(obj, JS_STRING_ID("debug"), val);

  JS_NAME_SET(obj, JS_STRING_ID("uv"), STD_TO_BOOLEAN(true));
  JS_NAME_SET(obj, JS_STRING_ID("ipv6"), STD_TO_BOOLEAN(true));
  JS_NAME_SET(obj, JS_STRING_ID("tls_npn"), STD_TO_BOOLEAN(use_npn));
  JS_NAME_SET(obj, JS_STRING_ID("tls_sni"), STD_TO_BOOLEAN(use_sni));
  JS_NAME_SET(obj, JS_STRING_ID("tls"),
              STD_TO_BOOLEAN(get_builtin_module("crypto") != NULL));

  return JS_LEAVE_SCOPE(obj);
}

static JS_GETTER_METHOD(DebugPortGetter) {
  RETURN_GETTER_PARAM(STD_TO_UNSIGNED(com->debug_port));
}
JS_GETTER_METHOD_END

static JS_SETTER_METHOD(DebugPortSetter) {
  com->debug_port = INTEGER_TO_STD(value);
}
JS_SETTER_METHOD_END

JS_GETTER_METHOD(NeedImmediateCallbackGetter) {
  RETURN_GETTER_PARAM(STD_TO_BOOLEAN(com->need_immediate_cb));
}
JS_GETTER_METHOD_END

static JS_SETTER_METHOD(NeedImmediateCallbackSetter) {
  bool bool_value = BOOLEAN_TO_STD(value);
  if (com->need_immediate_cb == bool_value) RETURN_SETTER();

  com->need_immediate_cb = bool_value;

  if (com->need_immediate_cb) {
    com->check_immediate_watcher->threadId = com->threadId;
    uv_check_start(com->check_immediate_watcher, node::CheckImmediate);
    // idle handle is needed only to maintain event loop
    uv_idle_start(com->idle_immediate_dummy, node::IdleImmediateDummy);
  } else {
    uv_check_stop(com->check_immediate_watcher);
    uv_idle_stop(com->idle_immediate_dummy);
  }
}
JS_SETTER_METHOD_END

#ifdef V8_IS_3_14
// Called from the main thread.
static void DispatchDebugMessagesAsyncCallback(uv_async_t* handle, int status) {
  v8::Debug::ProcessDebugMessages();
}

// Called from V8 Debug Agent TCP thread.
static void DispatchMessagesDebugAgentCallback() {
  node::commons* com = node::commons::getInstance();
  assert(com != NULL);
  uv_async_send(com->dispatch_debug_messages_async);
}
#elif defined(V8_IS_3_28)
void EnableDebug(bool wait_connect, node::commons* node);

// Called from V8 Debug Agent TCP thread.
static void DispatchMessagesDebugAgentCallback(node::commons* com) {
  uv_async_send(com->dispatch_debug_messages_async);
}

void StartDebug(node::commons* com, bool wait) {
  node::debugger::Agent* agent = (node::debugger::Agent*)com->agent_;
  agent->set_dispatch_handler(DispatchMessagesDebugAgentCallback);
#ifdef JS_ENGINE_CHAKRA
  com->debugger_running = v8::Debug::EnableAgent();
#else
  com->debugger_running = agent->Start(com->debug_port, wait);
#endif
  if (com->debugger_running == false) {
    fprintf(stderr, "Starting debugger on port %d failed\n", com->debug_port);
    fflush(stderr);
    return;
  }
}

// Called from the main thread.
static void DispatchDebugMessagesAsyncCallback(uv_async_t* handle, int status) {
  node::commons* com = node::commons::getInstance();
  assert(com != NULL);
  if (com->debugger_running == false) {
    fprintf(stderr, "Starting debugger agent.\n");

    jxcore::JXEngine* eng =
        jxcore::JXEngine::GetInstanceByThreadId(com->threadId);
    v8::Context::Scope context_scope(eng->getContext());

    StartDebug(com, false);
    EnableDebug(false, com);
  }
  v8::Isolate::Scope isolate_scope(com->node_isolate);
  v8::Debug::ProcessDebugMessages();
}
#endif  // V8_IS_3_28

void EnableDebug(bool wait_connect, node::commons* com) {
#if defined(JS_ENGINE_V8) || defined(JS_ENGINE_CHAKRA)
  // If we're called from another thread, make sure to enter the right
  // v8 isolate.
  if (com->node_isolate == NULL)
    com->node_isolate = v8::Isolate::GetCurrent();

  uv_async_init(com->loop, com->dispatch_debug_messages_async,
                DispatchDebugMessagesAsyncCallback);
  uv_unref((uv_handle_t*)com->dispatch_debug_messages_async);
#if defined(V8_IS_3_28)
  // Send message to enable debug in workers
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER_(com->node_isolate);

  // Enabled debugger, possibly making it wait on a semaphore
  node::debugger::Agent* agent = new node::debugger::Agent(com);
  com->agent_ = (void*)agent;
  agent->Enable();
#elif defined(V8_IS_3_14)
  com->node_isolate->Enter();

  v8::Debug::SetDebugMessageDispatchHandler(DispatchMessagesDebugAgentCallback,
                                            false);

  // Start the debug thread and it's associated TCP server on port 5858.
  if (!v8::Debug::EnableAgent("node " NODE_VERSION, com->debug_port,
                              wait_connect)) {
    flush_console("Unable to enable debugger agent\n");
    abort();
  }
  // Print out some information.
  fprintf(stderr, "debugger listening on port %d\n", com->debug_port);
  fflush(stderr);

  com->debugger_running = true;

  com->node_isolate->Exit();
#endif
#elif defined(JS_ENGINE_MOZJS)
// TODO(obastemur) DEBUG!!
#endif
}

#ifdef __POSIX__
void EnableDebugSignalHandler(uv_signal_t* handle, int) {
#ifdef JS_ENGINE_V8
  // Break once process will return execution to v8
  node::commons* com = node::commons::getInstance();
  v8::Debug::DebugBreak(com->node_isolate);

  if (!com->debugger_running) {
    fprintf(stderr, "Hit SIGUSR1 - starting debugger agent.\n");
    EnableDebug(false, com);
  }
#elif defined(JS_ENGINE_MOZJS)
// TODO(obastemur) DEBUG!!
#endif
}

JS_LOCAL_METHOD_NO_COM(DebugProcess) {
  if (args.Length() != 1) {
    THROW_EXCEPTION("Invalid number of arguments.");
  }

  pid_t pid;
  int r;

  pid = args.GetInteger(0);
  r = kill(pid, SIGUSR1);
  if (r != 0) {
    JS_LOCAL_VALUE err = ErrnoException(errno, "kill");
    THROW_EXCEPTION_OBJECT(err);
  }
}
JS_METHOD_END
#endif  // __POSIX__

#ifdef _WIN32
DWORD WINAPI EnableDebugThreadProc(void* arg) {
  // Break once process will return execution to v8
  node::commons* com = node::commons::getInstance();
  if (!com->debugger_running) {
    for (int i = 0; i < 1; i++) {
      fprintf(stderr, "Starting debugger agent.\r\n");
      fflush(stderr);
      EnableDebug(false, com);
    }
  }
#ifdef JS_ENGINE_V8
  v8::Debug::DebugBreak();
#elif defined(JS_ENGINE_MOZJS)
// TODO(obastemur) DEBUG!!
#endif

  return 0;
}

static int GetDebugSignalHandlerMappingName(DWORD pid, wchar_t* buf,
                                            size_t buf_len) {
  return _snwprintf(buf, buf_len, L"node-debug-handler-%u", pid);
}

int RegisterDebugSignalHandler() {
  wchar_t mapping_name[32];
  HANDLE mapping_handle;
  DWORD pid;
  LPTHREAD_START_ROUTINE* handler;

  pid = GetCurrentProcessId();

  if (GetDebugSignalHandlerMappingName(pid, mapping_name,
                                       ARRAY_SIZE(mapping_name)) < 0) {
    return -1;
  }

  mapping_handle =
      CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                         sizeof *handler, mapping_name);
  if (mapping_handle == NULL) {
    return -1;
  }

  handler = reinterpret_cast<LPTHREAD_START_ROUTINE*>(MapViewOfFile(
      mapping_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof *handler));
  if (handler == NULL) {
    CloseHandle(mapping_handle);
    return -1;
  }

  *handler = EnableDebugThreadProc;

  UnmapViewOfFile((void*)handler);

  return 0;
}

static JS_LOCAL_METHOD(DebugProcess) {
  JS_HANDLE_VALUE rv = JS_UNDEFINED();
  DWORD pid;
  HANDLE process = NULL;
  HANDLE thread = NULL;
  HANDLE mapping = NULL;
  wchar_t mapping_name[32];
  LPTHREAD_START_ROUTINE* handler = NULL;

  if (args.Length() != 1) {
    rv = THROW_EXCEPTION_NO_RETURN("Invalid number of arguments.");
    goto out;
  }

  pid = (DWORD)args.GetInteger(0);

  process =
      OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                      PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
                  FALSE, pid);
  if (process == NULL) {
    rv = JS_THROW_EXCEPTION_TYPE(
        WinapiErrnoException(GetLastError(), "OpenProcess", "", ""));
    goto out;
  }

  if (GetDebugSignalHandlerMappingName(pid, mapping_name,
                                       ARRAY_SIZE(mapping_name)) < 0) {
    rv = JS_THROW_EXCEPTION_TYPE(ErrnoException(errno, "sprintf", "", ""));
    goto out;
  }

  mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, mapping_name);
  if (mapping == NULL) {
    rv = JS_THROW_EXCEPTION_TYPE(
        WinapiErrnoException(GetLastError(), "OpenFileMappingW", "", ""));
    goto out;
  }

  handler = reinterpret_cast<LPTHREAD_START_ROUTINE*>(
      MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof *handler));
  if (handler == NULL || *handler == NULL) {
    rv = JS_THROW_EXCEPTION_TYPE(
        WinapiErrnoException(GetLastError(), "MapViewOfFile", "", ""));
    goto out;
  }

  thread = CreateRemoteThread(process, NULL, 0, *handler, NULL, 0, NULL);
  if (thread == NULL) {
    rv = JS_THROW_EXCEPTION_TYPE(
        WinapiErrnoException(GetLastError(), "CreateRemoteThread", "", ""));
    goto out;
  }

  // Wait for the thread to terminate
  if (WaitForSingleObject(thread, INFINITE) != WAIT_OBJECT_0) {
    rv = JS_THROW_EXCEPTION_TYPE(
        WinapiErrnoException(GetLastError(), "WaitForSingleObject", "", ""));
    goto out;
  }

out:
  if (process != NULL) {
    CloseHandle(process);
  }
  if (thread != NULL) {
    CloseHandle(thread);
  }
  if (handler != NULL) {
    UnmapViewOfFile(handler);
  }
  if (mapping != NULL) {
    CloseHandle(mapping);
  }

  RETURN_PARAM(rv);
}
JS_METHOD_END
#endif  // _WIN32

static JS_LOCAL_METHOD(DebugPause) {
#ifdef JS_ENGINE_V8
  v8::Debug::DebugBreak(JS_GET_STATE_MARKER());
#elif defined(JS_ENGINE_MOZJS)
// TODO(obastemur) DEBUG
#endif
}
JS_METHOD_END

static JS_LOCAL_METHOD(DebugEnd) {
  if (com->debugger_running) {
#ifdef JS_ENGINE_CHAKRA
#elif defined(JS_ENGINE_V8)
#ifdef V8_IS_3_14
    v8::Debug::DisableAgent();
#else
    ((node::debugger::Agent*)com->agent_)->Stop();
#endif
#elif defined(JS_ENGINE_MOZJS)
// TODO(obastemur) DEBUG
#endif
    com->debugger_running = false;
  }
}
JS_METHOD_END

static JS_LOCAL_METHOD(RawDebug) {
  assert(args.Length() == 1 && args.IsString(0) &&
         "must be called with a single string");

  jxcore::JXString message;
  args.GetString(0, &message);
  fprintf(stderr, "%s\n", *message);
  fflush(stderr);
}
JS_METHOD_END

void SetupProcessObject(const int threadId, bool debug_worker) {
  ENGINE_LOG_THIS("node", "SetupProcessObject");
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  int i, j;

// TODO(obastemur) why we need function template ?
// change made in order to get typeof process === 'object' compatibility
#ifdef JS_ENGINE_V8
  JS_LOCAL_FUNCTION_TEMPLATE process_template =
      JS_NEW_EMPTY_FUNCTION_TEMPLATE();
  JS_CLASSNAME_SET(process_template, STD_TO_STRING("process"));
  JS_HANDLE_OBJECT process =
      JS_NEW_DEFAULT_INSTANCE(JS_GET_FUNCTION(process_template));
#else
  JS_HANDLE_OBJECT process = JS_NEW_EMPTY_OBJECT();
#endif

  JS_ACCESSOR_SET(process, STD_TO_STRING("title"), ProcessTitleGetter,
                  ProcessTitleSetter);

  if (!node::commons::embedded_multithreading_) {
    JS_NAME_SET(process, JS_STRING_ID("subThread"),
                STD_TO_BOOLEAN(threadId != 0));
  } else {
    JS_NAME_SET(process, JS_STRING_ID("subThread"), STD_TO_BOOLEAN(false));
  }

  JS_NAME_SET(process, JS_STRING_ID("threadId"), STD_TO_INTEGER(threadId - 1));

  // process.version
  JS_NAME_SET(process, JS_STRING_ID("version"), STD_TO_STRING(NODE_VERSION));
  JS_NAME_SET(process, JS_STRING_ID("jxversion"),
              STD_TO_STRING(JXCORE_VERSION));

  // process.moduleLoadList
  JS_LOCAL_ARRAY objml = JS_NEW_ARRAY();
  JS_NEW_PERSISTENT_ARRAY(com->module_load_list, objml);
  JS_NAME_SET(process, JS_STRING_ID("moduleLoadList"), objml);

  // process.versions
  JS_LOCAL_OBJECT versions = JS_NEW_EMPTY_OBJECT();
  JS_NAME_SET(process, JS_STRING_ID("versions"), versions);
  JS_NAME_SET(versions, JS_STRING_ID("http_parser"),
              STD_TO_STRING(
                  NODE_STRINGIFY(HTTP_PARSER_VERSION_MAJOR) "." NODE_STRINGIFY(
                      HTTP_PARSER_VERSION_MINOR)));

  JS_NAME_SET(versions, JS_STRING_ID("node"), STD_TO_STRING(NODE_VERSION + 1));
  JS_NAME_SET(versions, JS_STRING_ID("jxcore"),
              STD_TO_STRING(JXCORE_VERSION + 1));
#ifdef JS_ENGINE_V8
#ifdef JS_ENGINE_CHAKRA
  JS_NAME_SET(versions, JS_STRING_ID("v8"), STD_TO_INTEGER(0));
  JS_NAME_SET(versions, JS_STRING_ID("ch"),
              STD_TO_STRING(v8::V8::GetVersion()));
#else
  JS_NAME_SET(versions, JS_STRING_ID("ch"), STD_TO_INTEGER(0));
  JS_NAME_SET(versions, JS_STRING_ID("v8"),
              STD_TO_STRING(v8::V8::GetVersion()));
#endif
  JS_NAME_SET(versions, JS_STRING_ID("sm"), STD_TO_INTEGER(0));
#elif defined(JS_ENGINE_MOZJS)
  JS_NAME_SET(versions, JS_STRING_ID("v8"), STD_TO_INTEGER(0));
  JS_NAME_SET(versions, JS_STRING_ID("sm"), STD_TO_INTEGER(MOZJS_VERSION));
#endif

  JS_LOCAL_OBJECT embedded = JS_NEW_EMPTY_OBJECT();
  JS_NAME_SET(versions, JS_STRING_ID("embedded"), embedded);
#ifdef JXCORE_EMBEDS_LEVELDOWN
  JS_NAME_SET(embedded, JS_STRING_ID("leveldown"), STD_TO_STRING("1.0.6"));
#endif

#ifdef JXCORE_EMBEDS_SQLITE
  JS_NAME_SET(embedded, JS_STRING_ID("sqlite"), STD_TO_STRING(SQLITE_VERSION));
#endif

  JS_NAME_SET(versions, JS_STRING_ID("ares"), STD_TO_STRING(ARES_VERSION_STR));
  JS_NAME_SET(versions, JS_STRING_ID("uv"), STD_TO_STRING(uv_version_string()));
  JS_NAME_SET(versions, JS_STRING_ID("zlib"), STD_TO_STRING(ZLIB_VERSION));
  JS_NAME_SET(versions, JS_STRING_ID("modules"),
              STD_TO_STRING(NODE_STRINGIFY(NODE_MODULE_VERSION)));
#if HAVE_OPENSSL
  // Stupid code to slice out the version string.

  int c, l = strlen(OPENSSL_VERSION_TEXT);
  for (i = j = 0; i < l; i++) {
    c = OPENSSL_VERSION_TEXT[i];
    if ('0' <= c && c <= '9') {
      for (j = i + 1; j < l; j++) {
        c = OPENSSL_VERSION_TEXT[j];
        if (c == ' ') break;
      }
      break;
    }
  }

  JS_NAME_SET(versions, JS_STRING_ID("openssl"),
              STD_TO_STRING_WITH_LENGTH(OPENSSL_VERSION_TEXT + i, j - i));
#endif

  if (threadId > 0) {
    node::commons* mainNode = node::commons::getInstanceByThreadId(0);
    com->option_end_index = mainNode->option_end_index;
  }

  // process.arch
  JS_NAME_SET(process, JS_STRING_ID("arch"), STD_TO_STRING(ARCH));

// process.platform
#ifdef WINONECORE
  JS_NAME_SET(process, JS_STRING_ID("platform"), STD_TO_STRING("winrt"));
#else
  JS_NAME_SET(process, JS_STRING_ID("platform"), STD_TO_STRING(PLATFORM));
#endif

  jxcore::JXEngine* active_engine = jxcore::JXEngine::ActiveInstance();
  if (active_engine == NULL && threadId > 0) {
    active_engine = jxcore::JXEngine::GetInstanceByThreadId(0);
  }
  assert(active_engine != NULL);
  if (debug_worker) {
    JS_LOCAL_ARRAY arguments = JS_NEW_ARRAY_WITH_COUNT(2);
    JS_INDEX_SET(arguments, 0, STD_TO_STRING("jx"));
    JS_INDEX_SET(arguments, 1, STD_TO_STRING("--debug-agent"));
    JS_NAME_SET(process, JS_STRING_ID("argv"), arguments);

    JS_LOCAL_ARRAY execArgv = JS_NEW_ARRAY_WITH_COUNT(1);
    JS_INDEX_SET(execArgv, 0, STD_TO_STRING("jx"));

    JS_NAME_SET(process, JS_STRING_ID("execArgv"), execArgv);
  } else {
    // process.argv
    JS_LOCAL_ARRAY arguments = JS_NEW_ARRAY_WITH_COUNT(
        active_engine->argc_ - com->option_end_index + 1);

    JS_INDEX_SET(arguments, 0, UTF8_TO_STRING(active_engine->argv_[0]));

    for (j = 1, i = com->option_end_index; i < active_engine->argc_; j++, i++) {
      JS_INDEX_SET(arguments, j, UTF8_TO_STRING(active_engine->argv_[i]));
    }

    // assign it
    JS_NAME_SET(process, JS_STRING_ID("argv"), arguments);

    // process.execArgv
    JS_LOCAL_ARRAY execArgv =
        JS_NEW_ARRAY_WITH_COUNT(com->option_end_index - 1);
    for (j = 1, i = 0; j < com->option_end_index; j++, i++) {
      JS_INDEX_SET(execArgv, i, UTF8_TO_STRING(active_engine->argv_[j]));
    }
    // assign it
    JS_NAME_SET(process, JS_STRING_ID("execArgv"), execArgv);
  }

// create process.env
#ifdef JS_ENGINE_V8
  JS_LOCAL_OBJECT_TEMPLATE envTemplate = JS_NEW_OBJECT_TEMPLATE();
  JS_NAMEDPROPERTYHANDLER_SET(envTemplate, EnvGetter, EnvSetter, EnvQuery,
                              EnvDeleter, EnvEnumerator, JS_NEW_EMPTY_OBJECT());
  JS_LOCAL_OBJECT env = JS_NEW_DEFAULT_INSTANCE(envTemplate);
#elif defined(JS_ENGINE_MOZJS)
  JS_LOCAL_OBJECT env(MozJS::Value::NewEmptyPropertyObject(
                          JS_GET_STATE_MARKER(), EnvGetter, EnvSetter, EnvQuery,
                          EnvEnumerator, EnvDeleter),
                      JS_GET_STATE_MARKER());
#endif
  JS_NAME_SET(process, JS_STRING_ID("env"), env);

  JS_NAME_SET(process, JS_STRING_ID("pid"), STD_TO_INTEGER(getpid()));
  JS_NAME_SET(process, JS_STRING_ID("features"), GetFeatures());
  JS_ACCESSOR_SET(process, STD_TO_STRING("_needImmediateCallback"),
                  NeedImmediateCallbackGetter, NeedImmediateCallbackSetter);

  // -e, --eval
  if (com->eval_string) {
    JS_NAME_SET(process, JS_STRING_ID("_eval"),
                STD_TO_STRING(com->eval_string));
  }

  // -p, --print
  if (com->print_eval) {
    JS_NAME_SET(process, JS_STRING_ID("_print_eval"), STD_TO_BOOLEAN(true));
  }

  // -i, --interactive
  if (com->force_repl) {
    JS_NAME_SET(process, JS_STRING_ID("_forceRepl"), STD_TO_BOOLEAN(true));
  }

  // --no-deprecation
  if (no_deprecation) {
    JS_NAME_SET(process, JS_STRING_ID("noDeprecation"), STD_TO_BOOLEAN(true));
  }

  // --throw-deprecation
  if (com->throw_deprecation) {
    JS_NAME_SET(process, JS_STRING_ID("throwDeprecation"),
                STD_TO_BOOLEAN(true));
  }

  // --trace-deprecation
  if (com->trace_deprecation) {
    JS_NAME_SET(process, JS_STRING_ID("traceDeprecation"),
                STD_TO_BOOLEAN(true));
  }

  size_t size = 2 * PATH_MAX;
  char* execPath = new char[size];
  if (uv_exepath(execPath, &size) != 0) {
    JS_NAME_SET(process, JS_STRING_ID("execPath"),
                STD_TO_STRING(active_engine->argv_[0]));
  } else {
    JS_NAME_SET(process, JS_STRING_ID("execPath"),
                STD_TO_STRING_WITH_LENGTH(execPath, size));
  }

#if defined(__IOS__) || defined(__ANDROID__)
    if (app_sandbox_folder != NULL) {
      free(app_sandbox_folder);
      app_sandbox_folder = NULL;
    }

    for (int i = size - 1; i >= 0; i--) {
      if (execPath[i] == '/') {
        app_sandbox_folder = (char*)malloc(sizeof(char) * (i + 2));
        memcpy(app_sandbox_folder, execPath, (i + 1) * sizeof(char));
        app_sandbox_folder[i + 1] = '\0';
        break;
      }
    }
    if (app_sandbox_folder == NULL) {
      app_sandbox_folder = (char*)malloc(sizeof(char) * (size + 1));
      memcpy(app_sandbox_folder, execPath, size * sizeof(char));
      app_sandbox_folder[size] = '\0';
    }
#endif

#if defined(__IOS__) || defined(__ANDROID__)
  // mobile app delivers the full path via argv[0]
  char* docs_home = NULL;
  char* docs_folder = active_engine->argv_[0];

  size_t docs_size = strlen(docs_folder);
  for (int i = docs_size - 1; i >= 0; i--) {
    if (docs_folder[i] == '/') {
      docs_home = (char*)malloc(sizeof(char) * (i + 2));
      memcpy(docs_home, docs_folder, (i + 1) * sizeof(char));
      docs_home[i + 1] = '\0';
      docs_size = i + 1;
      break;
    }
  }

  if (docs_home != NULL) {
    JS_NAME_SET(process, JS_STRING_ID("userPath"),
                STD_TO_STRING_WITH_LENGTH(docs_home, docs_size));
  } else {
    JS_NAME_SET(process, JS_STRING_ID("userPath"),
                STD_TO_STRING_WITH_LENGTH(docs_folder, docs_size));
  }

  free(docs_home);
#endif

  delete[] execPath;

  JS_ACCESSOR_SET(process, STD_TO_STRING("debugPort"), DebugPortGetter,
                  DebugPortSetter);
#if defined(JXCORE_EMBEDDED)
  JS_NAME_SET(process, JS_STRING_ID("isEmbedded"), STD_TO_BOOLEAN(true));
#else
  JS_NAME_SET(process, JS_STRING_ID("isEmbedded"), STD_TO_BOOLEAN(false));
#endif

  // Check if the process has standard stream file descriptors, needed for
  // allocating a normal console object.
  JS_NAME_SET(process, JS_STRING_ID("hasStdFds"),
              STD_TO_BOOLEAN(uv_guess_handle(1) != UV_UNKNOWN_HANDLE));

  JS_NAME_SET(process, JS_STRING_ID("isPackaged"),
              STD_TO_BOOLEAN(com->is_packaged_));

  // define various internal method
  JS_METHOD_SET(process, "_rawDebug", RawDebug);
  JS_METHOD_SET(process, "_getActiveRequests", GetActiveRequests);
  JS_METHOD_SET(process, "_getActiveHandles", GetActiveHandles);
  JS_METHOD_SET(process, "_needTickCallback", NeedTickCallback);
  JS_METHOD_SET(process, "reallyExit", Exit);
  JS_METHOD_SET(process, "abort", Abort);
  JS_METHOD_SET(process, "chdir", Chdir);
  JS_METHOD_SET(process, "cwd", Cwd);
  JS_METHOD_SET(process, "umask", Umask);

#ifdef __POSIX__
  JS_METHOD_SET(process, "getuid", GetUid);
  JS_METHOD_SET(process, "setuid", SetUid);

  JS_METHOD_SET(process, "setgid", SetGid);
  JS_METHOD_SET(process, "getgid", GetGid);

  JS_METHOD_SET(process, "getgroups", GetGroups);
  JS_METHOD_SET(process, "setgroups", SetGroups);
  JS_METHOD_SET(process, "initgroups", InitGroups);
#endif  // __POSIX__

  JS_METHOD_SET(process, "_kill", Kill);

  JS_METHOD_SET(process, "_debugProcess", DebugProcess);
  JS_METHOD_SET(process, "_debugPause", DebugPause);
  JS_METHOD_SET(process, "_debugEnd", DebugEnd);

  JS_METHOD_SET(process, "hrtime", Hrtime);

  JS_METHOD_SET(process, "_dlopen", DLOpen);

  JS_METHOD_SET(process, "uptime", Uptime);
  JS_METHOD_SET(process, "memoryUsage", MemoryUsage);

  JS_METHOD_SET(process, "binding", JXBinding);

  JS_METHOD_SET(process, "_usingDomains", UsingDomains);

  // values use to cross communicate with processNextTick
  JS_LOCAL_OBJECT info_box;
#ifdef JS_ENGINE_V8
  info_box = JS_NEW_EMPTY_OBJECT();
  JS_SET_INDEXED_EXTERNAL(info_box, com->tick_infobox,
                          ENGINE_NS::kExternalUnsignedIntArray, 3);
#elif JS_ENGINE_MOZJS
  // Above implementation also works over MozJS proxy
  // but we need a faster implementation
  JS_BIND_NEW_ARRAY_BUFFER(info_box, Uint32, uint32_t, 3, com->tick_infobox,
                           __tickbox);
#endif
  JS_NAME_SET(process, JS_STRING_ID("_tickInfoBox"), info_box);
  // pre-set _events object for faster emit checks
  JS_NAME_SET(process, JS_STRING_ID("_events"), JS_NEW_EMPTY_OBJECT());

  com->setProcess(process);
}

static void AtExit() { uv_tty_reset_mode(); }

void Load(JS_HANDLE_OBJECT process_l) {
  ENGINE_LOG_THIS("node", "Load");
  node::commons* com = node::commons::getInstance();
  JS_DEFINE_STATE_MARKER(com);

  atexit(AtExit);

  JS_TRY_CATCH(try_catch);

  jxcore::JXString str;
  MainSource(com, &str);

  jxcore::JXString fname("node.js", JS_GET_STATE_MARKER());

  JS_LOCAL_VALUE f_value = ExecuteString(&str, &fname);
  if (try_catch.HasCaught()) {
    ReportException(try_catch, true);
#ifndef JXCORE_EMBEDDED
    exit(10);
  }
  assert(JS_IS_FUNCTION(f_value));
#else
    error_console("!!!!Couldn't Initialize JXcore!!!");
    return;
  }

  if (JS_IS_UNDEFINED(f_value)) {
    return;
  }
#endif

  JS_LOCAL_FUNCTION f = JS_CAST_FUNCTION(f_value);

  // Now we call 'f' with the 'process' variable that we've built up with
  // all our bindings. Inside node.js we'll take care of assigning things to
  // their places.

  // We start the process this way in order to be more modular. Developers
  // who do not like how 'src/node.js' setups the module system but do like
  // Node's I/O bindings may want to replace 'f' with their own function.

  // Add a reference to the global object
  JS_LOCAL_OBJECT global = JS_GET_GLOBAL();
  JS_LOCAL_VALUE args[1] = {JS_TYPE_TO_LOCAL_VALUE(process_l)};

  if (com->threadId == 0) {
#if defined HAVE_DTRACE || defined HAVE_ETW || defined HAVE_SYSTEMTAP
    InitDTrace(global);
#endif

#if defined HAVE_PERFCTR
    InitPerfCounters(global);
#endif
  }

  JS_METHOD_CALL(f, global, 1, args);
  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
}

void RunAtExit() {
  ENGINE_LOG_THIS("node", "RunAtExit");
  node::commons* com = node::commons::getInstance();
  if (com != NULL) {
    AtExitCallback* p = com->at_exit_functions_;
    com->at_exit_functions_ = NULL;

    if (p == NULL) return;

    while (p) {
      AtExitCallback* q = p->next_;
      p->cb_(p->arg_);
      delete p;
      p = q;
    }
  }
}

void AtExit(void (*cb)(void* arg), void* arg) {
  ENGINE_LOG_THIS("node", "AtExit");
  node::commons* com = node::commons::getInstance();
  if (com != NULL) {
    AtExitCallback* p = new AtExitCallback;
    p->cb_ = cb;
    p->arg_ = arg;
    p->next_ = com->at_exit_functions_;
    com->at_exit_functions_ = p;
  }
}

void EmitExit(JS_HANDLE_OBJECT process_l) {
  ENGINE_LOG_THIS("node", "EmitExit");
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_NAME_SET(process_l, JS_STRING_ID("_exiting"), STD_TO_BOOLEAN(true));
  JS_LOCAL_VALUE emit_v = JS_GET_NAME(process_l, JS_STRING_ID("emit"));
  assert(JS_IS_FUNCTION(emit_v));
  JS_LOCAL_FUNCTION emit = JS_CAST_FUNCTION(emit_v);
  JS_LOCAL_VALUE args[] = {STD_TO_STRING("exit"), STD_TO_INTEGER(0)};

  JS_TRY_CATCH(try_catch);
  JS_METHOD_CALL(emit, process_l, 2, args);
  if (try_catch.HasCaught()) {
    FatalException(try_catch);
  }
}

void EmitReset(JS_HANDLE_OBJECT process_l, const int code) {
  ENGINE_LOG_THIS("node", "EmitReset");
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_TRY_CATCH(try_catch);
#ifdef JS_ENGINE_MOZJS
  if (try_catch.HasCaught()) {
    try_catch.ClearPendingException();
  }
#endif

  // if this is an embedded instance we shouldn't terminate the process
  // TODO(obastemur) apply this rule to other places
  if (node::commons::self_hosted_process_ || com->threadId > 0) {
    JS_NAME_SET(process_l, JS_STRING_ID("_exiting"), STD_TO_BOOLEAN(true));
    JS_LOCAL_VALUE emit_v = JS_GET_NAME(process_l, JS_STRING_ID("emit"));
    assert(JS_IS_FUNCTION(emit_v));
    JS_LOCAL_FUNCTION emit = JS_CAST_FUNCTION(emit_v);
    JS_LOCAL_VALUE args[] = {STD_TO_STRING("$$restart"), STD_TO_INTEGER(code)};

    JS_METHOD_CALL(emit, process_l, 2, args);
    if (try_catch.HasCaught()) {
      abort();
    }
  }
}

}  // namespace node
