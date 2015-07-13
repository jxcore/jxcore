// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_NODE_H_
#define SRC_NODE_H_

#define USES_JXCORE_ENGINE 1

#ifdef _WIN32
#ifndef BUILDING_NODE_EXTENSION
#define NODE_EXTERN __declspec(dllexport)
#else
#define NODE_EXTERN __declspec(dllimport)
#endif
#else
#define NODE_EXTERN /* nothing */
#endif

#ifdef BUILDING_NODE_EXTENSION
#undef BUILDING_V8_SHARED
#undef BUILDING_UV_SHARED
#define USING_V8_SHARED 1
#define USING_UV_SHARED 1
#endif

// This should be defined in make system.
// See issue https://github.com/joyent/node/issues/1236
#if defined(__MINGW32__) || defined(_MSC_VER)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#define NOMINMAX

#endif

#if defined(_MSC_VER)
#define PATH_MAX MAX_PATH
#endif

#ifdef _WIN32
#define SIGKILL 9
#endif

#include "uv.h"
#include "jx/Proxy/JSEngine.h"
#include "jx/error_definition.h"
#include <sys/types.h> /* struct stat */
#include <sys/stat.h>
#include <assert.h>
#include NODE_OBJECT_WRAP_HEADER

#if NODE_WANT_INTERNALS
#include "node_internals.h"
#endif

#ifndef NODE_STRINGIFY
#define NODE_STRINGIFY(n) NODE_STRINGIFY_HELPER(n)
#define NODE_STRINGIFY_HELPER(n) #n
#endif

#ifndef STATIC_ASSERT
#if defined(_MSC_VER)
#define STATIC_ASSERT(expr) static_assert(expr, "")
#else
#define STATIC_ASSERT(expr) static_cast<void>((sizeof(char[-1 + !!(expr)])))
#endif
#endif

namespace node {

extern bool inited;

NODE_EXTERN extern bool no_deprecation;

void Load(JS_HANDLE_OBJECT process);
void EmitExit(JS_HANDLE_OBJECT process);

enum encoding {
  ASCII,
  UTF8,
  BASE64,
  UCS2,
  BINARY,
  HEX,
  BUFFER
};
enum encoding ParseEncoding(JS_HANDLE_VALUE encoding_v,
                            enum encoding _default = BINARY);

enum encoding ParseEncoding(const char *encoding, const int length,
                            enum encoding _default = BINARY);

NODE_EXTERN JS_LOCAL_VALUE
    Encode(const void *buf, size_t len, enum encoding encoding = BINARY);

// Returns -1 if the handle was not valid for decoding
NODE_EXTERN ssize_t
    DecodeBytes(JS_HANDLE_VALUE, enum encoding encoding = BINARY);

// returns bytes written.
NODE_EXTERN ssize_t DecodeWrite(char *buf, size_t buflen, JS_HANDLE_VALUE,
                                enum encoding encoding = BINARY);

JS_LOCAL_OBJECT BuildStatsObject(const uv_statbuf_t *s);

NODE_EXTERN JS_LOCAL_VALUE
    ErrnoException(int errorno, const char *syscall = NULL,
                   const char *msg = "", const char *path = NULL);

NODE_EXTERN JS_LOCAL_VALUE UVException(int errorno, const char *syscall = NULL,
                                       const char *msg = NULL,
                                       const char *path = NULL);

#ifdef _WIN32
NODE_EXTERN JS_LOCAL_VALUE
    WinapiErrnoException(int errorno, const char *syscall = NULL,
                         const char *msg = "", const char *path = NULL);
#endif

const char *signo_string(int errorno);

NODE_EXTERN typedef void (*addon_register_func)(JS_HANDLE_OBJECT_REF exports,
                                                JS_HANDLE_VALUE module);

struct node_module_struct {
  int version;
  void *dso_handle;
  const char *filename;
  node::addon_register_func register_func;
  const char *modname;
};

node_module_struct *get_builtin_module(const char *name);

/**
 * When this version number is changed, node.js will refuse
 * to load older modules.  This should be done whenever
 * an API is broken in the C++ side, including in v8 or
 * other dependencies.
 */
#define NODE_MODULE_VERSION 11

#define NODE_STANDARD_MODULE_STUFF NODE_MODULE_VERSION, NULL, __FILE__

#ifdef _WIN32
#define NODE_MODULE_EXPORT __declspec(dllexport)
#else
#define NODE_MODULE_EXPORT __attribute__((visibility("default")))
#endif

#define NODE_MODULE(modname, regfunc)                                 \
  extern "C" {                                                        \
  NODE_MODULE_EXPORT node::node_module_struct modname##_module = {    \
      NODE_STANDARD_MODULE_STUFF, (node::addon_register_func)regfunc, \
      NODE_STRINGIFY(modname)};                                       \
  }

#define NODE_MODULE_DECL(modname) \
  extern "C" node::node_module_struct modname##_module;

/* Called after the event loop exits but before the VM is disposed.
 * Callbacks are run in reverse order of registration, i.e. newest first.
 */
NODE_EXTERN void AtExit(void (*cb)(void *arg), void *arg = 0);

/*
 * MakeCallback doesn't have a HandleScope. That means the callers scope
 * will retain ownership of created handles from MakeCallback and related.
 * There is by default a wrapping HandleScope before uv_run, if the caller
 * doesn't have a HandleScope on the stack the global will take ownership
 * which won't be reaped until the uv loop exits.
 *
 * If a uv callback is fired, and there is no enclosing HandleScope in the
 * cb, you will appear to leak 4-bytes for every invocation. Take heed.
 */

NODE_EXTERN JS_HANDLE_VALUE MakeCallback(const JS_HANDLE_OBJECT_REF object,
                                         const char *method, int argc,
                                         JS_HANDLE_VALUE argv[]);

NODE_EXTERN void SetErrno(uv_err_t err);

NODE_EXTERN JS_HANDLE_VALUE MakeCallback(const JS_HANDLE_OBJECT_REF object,
                                         const JS_HANDLE_STRING symbol,
                                         int argc, JS_HANDLE_VALUE argv[]);

NODE_EXTERN JS_HANDLE_VALUE MakeCallback(const JS_HANDLE_OBJECT_REF object,
                                         const JS_HANDLE_FUNCTION_REF callback,
                                         int argc, JS_HANDLE_VALUE argv[]);

}  // namespace node

#endif  // SRC_NODE_H_
