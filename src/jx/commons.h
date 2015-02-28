// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_COMMONS_H_
#define SRC_JX_COMMONS_H_

#include "node.h"
#include "ares.h"
#include "tree.h"

#if defined HAVE_DTRACE || defined HAVE_ETW || defined HAVE_SYSTEMTAP
#include "node_dtrace.h"
#endif
#if defined HAVE_PERFCTR
#include "node_counters.h"
#endif

#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_MSC_VER)
#include <strings.h>
#else
#define strcasecmp _stricmp
#endif
#include <map>
#include <list>
#include "queue.h"
#include "slab_allocator.h"
#include "http_parser.h"

#ifndef __IOS__
#include "btree_map.h"
#define MAP_HOST btree::btree_map
#else
#define MAP_HOST std::map
#endif

namespace node {

#define MAX_JX_THREADS 64

struct __tickbox {
  uint32_t length;
  uint32_t index;
  uint32_t depth;
};

struct AtExitCallback {
  AtExitCallback *next_;
  void (*cb_)(void *arg);
  void *arg_;
};

class nQueue {
 public:
  nQueue() {
    wrap[0] = &wrap;
    wrap[1] = &wrap;
    req[0] = &req;
    req[1] = &req;
  }
  QUEUE wrap;
  QUEUE req;
};

typedef enum {
  JXCORE_INSTANCE_NONE = 0,
  JXCORE_INSTANCE_ALIVE,
  JXCORE_INSTANCE_EXITING,
  JXCORE_INSTANCE_EXITED
} jxcore_instance_status;

typedef MAP_HOST<std::string, std::string> BTStore;
#define DEFINE_PERSISTENT_STRING(str) JS_PERSISTENT_STRING pstr_##str

class commons {
  JS_PERSISTENT_OBJECT process_;

  void stringOPS(bool construct);

 public:
#ifdef JS_ENGINE_MOZJS
  // JS engine execution interrupt flag
  bool should_interrupt_;
#endif

  static bool embedded_multithreading_;

  static char *embedded_source_;  // static data for entry point
  bool is_embedded_;              // marker check

  // configuration
  static int bTCP, bTCPS;
  static int64_t maxMemory;
  static int allowSysExec;
  static int allowLocalNativeModules;
  static std::string globalModulePath;
  static int maxCPU, maxCPUInterval;
  static int allowMonitoringAPI;
  static int32_t max_header_size;

  static jxcore_instance_status process_status_;
  jxcore_instance_status instance_status_;
  bool expects_reset;

  static int mapCount;
  static int threadPoolCount;
  static BTStore *mapData[];

  uv_async_t *threadPing;
  bool handle_has_symbol_;

  struct http_parser_settings *parser_settings;
  char *pa_current_buffer_data;
  size_t pa_current_buffer_len;
  bool pab_current_buffer;
  bool stream_initialized;
  node::SlabAllocator *s_slab_allocator;
  node::SlabAllocator *udp_slab_allocator;
  bool print_eval;
  bool force_repl;
  bool trace_deprecation;
  bool throw_deprecation;
  char *eval_string;
  int option_end_index;
  bool use_debug_agent;
  bool debug_wait_connect;
  double prog_start_time;
  int debug_port;
  int max_stack_size;
  bool using_domains;
  bool need_tick_cb;

  bool debugger_running;
  bool need_immediate_cb;
  int threadId;
  int waitCounter;
  int threadOnHold;
  uint64_t counter_gc_start_time;
  uint64_t counter_gc_end_time;

  __tickbox *tick_infobox;
  AtExitCallback *at_exit_functions_;

  uv_timer_t *ares_timer;
  uv_signal_t *signal_watcher;
  uv_idle_t *tick_spinner;
  uv_check_t *check_immediate_watcher;
  uv_idle_t *idle_immediate_dummy;
  uv_async_t *dispatch_debug_messages_async;

  nQueue *NQ;
  JS_ENGINE_MARKER node_isolate;
  uv_loop_t *loop;
  ares_channel _ares_channel;

  static void TriggerDummy(uv_async_t *handle, int status);
  void PingThread();
  static void CleanPinger(int threadId);
  static double GetCPUUsage(const int64_t timer, const int64_t diff);
  static bool AllowMonitoringAPI();
  static void SetMonitoringAPI(const bool allow);
  static std::string GetGlobalModulePath();
  static void SetGlobalModulePath(const std::string path);
  static int GetMaxCPU();
  static int GetMaxCPUInterval();
  static void SetMaxCPU(const int maxcpu, const int interval);
  static bool AllowLocalNativeModules();
  static void SetAllowLocalNativeModules(const int allow);

  static void SetPortBoundaries(const int tcp, const int tcps,
                                const bool allowCustom);
  static int GetTCPBoundary();
  static int GetTCPSBoundary();
  static void SetSysExec(const int sysExec);
  static bool CanSysExec();
  static void SetMaxMemory(const int64_t mem);
  static bool CheckMemoryLimit();
  static void CheckCPUUsage(const int64_t timer);

#if !defined(JS_ENGINE_MOZJS)
  DEFINE_PERSISTENT_STRING(callback);
  DEFINE_PERSISTENT_STRING(onerror);
  DEFINE_PERSISTENT_STRING(onstop);
  DEFINE_PERSISTENT_STRING(onexit);
  DEFINE_PERSISTENT_STRING(port);
  DEFINE_PERSISTENT_STRING(address);
  DEFINE_PERSISTENT_STRING(family);
  DEFINE_PERSISTENT_STRING(IPv4);
  DEFINE_PERSISTENT_STRING(IPv6);
  DEFINE_PERSISTENT_STRING(pid);
  DEFINE_PERSISTENT_STRING(uid);
  DEFINE_PERSISTENT_STRING(gid);
  DEFINE_PERSISTENT_STRING(file);
  DEFINE_PERSISTENT_STRING(args);
  DEFINE_PERSISTENT_STRING(envPairs);
  DEFINE_PERSISTENT_STRING(cwd);
  DEFINE_PERSISTENT_STRING(detached);
  DEFINE_PERSISTENT_STRING(Buffer);
  DEFINE_PERSISTENT_STRING(exports);
  DEFINE_PERSISTENT_STRING(domain);
  DEFINE_PERSISTENT_STRING(wrap);
  DEFINE_PERSISTENT_STRING(stdio);
  DEFINE_PERSISTENT_STRING(type);
  DEFINE_PERSISTENT_STRING(ignore);
  DEFINE_PERSISTENT_STRING(pipe);
  DEFINE_PERSISTENT_STRING(fd);
  DEFINE_PERSISTENT_STRING(handle);
  DEFINE_PERSISTENT_STRING(tcp);
  DEFINE_PERSISTENT_STRING(tty);
  DEFINE_PERSISTENT_STRING(udp);
  DEFINE_PERSISTENT_STRING(wrapType);
  DEFINE_PERSISTENT_STRING(windowsVerbatimArguments);
  DEFINE_PERSISTENT_STRING(bytes);
  DEFINE_PERSISTENT_STRING(_tickCallback);
  DEFINE_PERSISTENT_STRING(_errno);
  DEFINE_PERSISTENT_STRING(buffer);
  DEFINE_PERSISTENT_STRING(incoming);
  DEFINE_PERSISTENT_STRING(_charsWritten);
  DEFINE_PERSISTENT_STRING(Signal);
  DEFINE_PERSISTENT_STRING(onsignal);
  DEFINE_PERSISTENT_STRING(sessionId);
  DEFINE_PERSISTENT_STRING(length);
  DEFINE_PERSISTENT_STRING(onHeadersComplete);
  DEFINE_PERSISTENT_STRING(onHeaders);
  DEFINE_PERSISTENT_STRING(headers);
  DEFINE_PERSISTENT_STRING(url);
  DEFINE_PERSISTENT_STRING(method);
  DEFINE_PERSISTENT_STRING(statusCode);
  DEFINE_PERSISTENT_STRING(upgrade);
  DEFINE_PERSISTENT_STRING(shouldKeepAlive);
  DEFINE_PERSISTENT_STRING(versionMinor);
  DEFINE_PERSISTENT_STRING(versionMajor);
  DEFINE_PERSISTENT_STRING(onBody);
  DEFINE_PERSISTENT_STRING(onMessageComplete);
  DEFINE_PERSISTENT_STRING(owner);
  DEFINE_PERSISTENT_STRING(ondata);
  DEFINE_PERSISTENT_STRING(parser);
  DEFINE_PERSISTENT_STRING(__ptype);
#endif

  DEFINE_PERSISTENT_STRING(ontimeout);
  DEFINE_PERSISTENT_STRING(close);
  DEFINE_PERSISTENT_STRING(threadMessage);
  DEFINE_PERSISTENT_STRING(emit);
  DEFINE_PERSISTENT_STRING(oncomplete);
  DEFINE_PERSISTENT_STRING(onchange);
  DEFINE_PERSISTENT_STRING(onconnection);
  DEFINE_PERSISTENT_STRING(onmessage);
  DEFINE_PERSISTENT_STRING(onread);
  DEFINE_PERSISTENT_STRING(ondone);
  DEFINE_PERSISTENT_STRING(onhandshakestart);
  DEFINE_PERSISTENT_STRING(onnewsession);
  DEFINE_PERSISTENT_STRING(onhandshakedone);
  DEFINE_PERSISTENT_STRING(onclienthello);
  DEFINE_PERSISTENT_STRING(onselect);
  DEFINE_PERSISTENT_STRING(GET);
  DEFINE_PERSISTENT_STRING(HEAD);
  DEFINE_PERSISTENT_STRING(POST);
  DEFINE_PERSISTENT_STRING(_immediateCallback);

  JS_PERSISTENT_FUNCTION process_tickFromSpinner;
  JS_PERSISTENT_FUNCTION process_tickCallback;
  JS_PERSISTENT_FUNCTION cloneObjectMethod;

  JS_PERSISTENT_FUNCTION JSONstringify;

  JS_PERSISTENT_FUNCTION_TEMPLATE ws_constructor_template;
  JS_PERSISTENT_FUNCTION_TEMPLATE wc_constructor_template;
  JS_PERSISTENT_FUNCTION_TEMPLATE nf_stats_constructor_template;

  JS_PERSISTENT_OBJECT binding_cache;
  JS_PERSISTENT_ARRAY module_load_list;

  JS_PERSISTENT_STRING tick_callback_sym;

#ifdef JS_ENGINE_V8  // typed arrays
  int ta_ft_id, ta_ft_cc_id;
  JS_PERSISTENT_FUNCTION_TEMPLATE ft_cache[16];
  JS_PERSISTENT_FUNCTION_TEMPLATE ta_ft_cache[16];
  JS_PERSISTENT_FUNCTION_TEMPLATE dv_ft_cache;
#elif defined(JS_ENGINE_MOZJS)
  JS_PERSISTENT_FUNCTION JSObjectMaker_;
  JS_PERSISTENT_FUNCTION JSObjectLister_;
  MozJS::Value CreateJSObject(const char *type_name_);
  MozJS::Value GetPropertyNames(MozJS::Value *obj);
#endif

  JS_PERSISTENT_FUNCTION fast_buffer_constructor;
  JS_PERSISTENT_FUNCTION_TEMPLATE bf_constructor_template;
  JS_PERSISTENT_FUNCTION tcpConstructor;

  JS_PERSISTENT_FUNCTION udp_constructor;
  JS_PERSISTENT_FUNCTION_TEMPLATE secure_context_constructor;
  JS_PERSISTENT_FUNCTION pipeConstructor;

  explicit commons(const int tid);
  ~commons();

  void setProcess(JS_HANDLE_OBJECT_REF ps);

  inline JS_HANDLE_OBJECT getProcess() { return process_; }

  static commons *getInstance();
  static commons *getInstanceIso(JS_ENGINE_MARKER iso);
  static commons *newInstance(const int tid);
  static commons *getInstanceByThreadId(const int id);
  static int getCurrentThreadId();
  static int getAvailableThreadId(bool multi_threaded_);

  static uv_loop_t *getThreadLoop();

  void Dispose();
  void setMainIsolate();

#ifdef JS_ENGINE_MOZJS
  std::list<JSContext*> free_context_list_;
#endif
};

commons *setCommons(commons *com);
void removeCommons();

void SetupProcessObject(const int step);
void Load(JS_HANDLE_OBJECT process_l);
void RunAtExit();

template <typename T>
class ReqWrap {
 public:
  explicit ReqWrap(node::commons *com = NULL) {
    if (com != NULL) Init(com);
  }

  void Init(node::commons *com) {
    JS_ENTER_SCOPE();
    JS_DEFINE_STATE_MARKER(com);

    object_ = JS_NEW_EMPTY_PERSISTENT_OBJECT();

    if (com->using_domains) {
      JS_LOCAL_OBJECT process = JS_TYPE_TO_LOCAL_OBJECT(com->getProcess());
      JS_LOCAL_STRING domain_str = STD_TO_STRING("domain");
      JS_LOCAL_VALUE domain = JS_GET_NAME(process, domain_str);

      if (!JS_IS_UNDEFINED(domain)) {
        JS_NAME_SET(object_, JS_STRING_ID("domain"), domain);
      }
    }

    QUEUE_INSERT_TAIL(&com->NQ->req, &req_wrap_queue_);
  }

  ~ReqWrap() {
    QUEUE_REMOVE(&req_wrap_queue_);
    // Assert that someone has called Dispatched()
    assert(req_.data == this);

    JS_CLEAR_PERSISTENT(object_);
  }

  // Call this after the req has been dispatched.
  void Dispatched() { req_.data = this; }

  JS_PERSISTENT_OBJECT object_;
  QUEUE req_wrap_queue_;
  void *data_;

  T req_;  // *must* be last, GetActiveRequests() in node.cc depends on it
};
}  // namespace node

#endif  // SRC_JX_COMMONS_H_
