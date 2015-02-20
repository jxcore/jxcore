// Copyright & License details are available under JXCORE_LICENSE file

#include <stdlib.h>
#include <map>
#include <list>
#include "jxcore.h"
#include "jx/extend.h"
#include "wrappers/memory_wrap.h"
#include "jx/error_definition.h"
#include "node_version.h"
#include "jx/job_store.h"
#include "string_bytes.h"

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x) * 1000)
#endif

#if HAVE_OPENSSL
#include "node_crypto.h"
#endif

namespace jxcore {

static void SignalExit(int signal) {
  uv_tty_reset_mode();
  _exit(128 + signal);
}

void RegisterSignalHandler(int signal, void (*handler)(int)) {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  sigaction(signal, &sa, NULL);
}

static char **copy_argv(int argc, char **argv) {
  size_t strlen_sum;
  char **argv_copy;
  char *argv_data;
  size_t len;
  int i;

  strlen_sum = 0;
  for (i = 0; i < argc; i++) {
    strlen_sum += strlen(argv[i]) + 1;
  }

  argv_copy = (char **)malloc(sizeof(char *) * (argc + 1) + strlen_sum);
  if (!argv_copy) {
    return NULL;
  }

  argv_data = (char *)argv_copy + sizeof(char *) * (argc + 1);

  for (i = 0; i < argc; i++) {
    argv_copy[i] = argv_data;
    len = strlen(argv[i]) + 1;
    memcpy(argv_data, argv[i], len);
    argv_data += len;
  }

  argv_copy[argc] = NULL;

  return argv_copy;
}

static char *jx_source[1] = {"@jx_source"};

static void WaitThreadExit(uv_loop_t *ev_loop) {
  if (getThreadCount() > 0) Sleep(5);

  bool busy = getThreadCount() > 0;
  int cc = 0;

  for (int i = 0; i < 20; i++) {  // ~2sec max thread cleanup wait (embedded)
    while (busy && cc < 100) {
      busy = getThreadCount() > 0;
      cc++;
      if (ev_loop)
        cc -= uv_run_jx(ev_loop, UV_RUN_NOWAIT, node::commons::CleanPinger, 0);

      if (!busy) break;

      Sleep(1);
    }

    if (!busy) break;

    cc = 0;
  }
}

#ifdef JS_ENGINE_MOZJS
void GCOnMozJS(JSRuntime *rt, JSGCStatus status, void *data) {
  // flush_console("GC called -  %s\n", status == JSGC_BEGIN ? "BEGIN" : "END");
}

bool JSEngineInterrupt(JSContext *ctx) {
  int tid = JS_GetThreadId(ctx);
  node::commons *com = node::commons::getInstanceByThreadId(tid);
  if (com->should_interrupt_) {
    com->should_interrupt_ = false;
    return false;
  }

  // TODO(obasetmur) This also triggers when CPU hits 100%. do anything ?
  return true;
}
#endif

void JXEngine::ParseDebugOpt(const char *arg) {
  const char *p = 0;

  main_node_->use_debug_agent = true;
  if (!strcmp(arg, "--debug-brk")) {
    main_node_->debug_wait_connect = true;
    return;
  } else if (!strcmp(arg, "--debug")) {
    return;
  } else if (strstr(arg, "--debug-brk=") == arg) {
    main_node_->debug_wait_connect = true;
    p = 1 + strchr(arg, '=');
    main_node_->debug_port = atoi(p);
  } else if (strstr(arg, "--debug=") == arg) {
    p = 1 + strchr(arg, '=');
    main_node_->debug_port = atoi(p);
  }
  if (p && main_node_->debug_port > 1024 && main_node_->debug_port < 65536)
    return;

  fprintf(stderr, "Bad debug option.\n");
  if (p) fprintf(stderr, "Debug port must be in range 1025 to 65535.\n");

  PrintHelp();
  exit(12);
}

void JXEngine::PrintHelp() {
  printf(
      "Usage: jx [options] [ -e script | script.js ] [arguments] \n"
      "       jx debug script.js [arguments] \n"
      "\n"
      "Options:\n"
      "  compile              compile jx file from jxp project\n"
      "  package              create jxp project from the folder (recursive)\n"
      "  install              installs a module from the repository\n"
      "  mt                   multithread the given application (no-keep)\n"
      "  mt-keep              multithread the given application (keep "
      "alive)\n\n"
      "  -jxv, --jxversion    print jxcore's version\n"
      "  -v, --version        print corresponding node's version\n"
      "  -e, --eval script    evaluate script\n"
      "  -p, --print          evaluate script and print result\n"
      "  -i, --interactive    always enter the REPL even if stdin\n"
      "                       does not appear to be a terminal\n"
      "  --no-deprecation     silence deprecation warnings\n"
      "  --trace-deprecation  show stack traces on deprecations\n"
      "  --v8-options         print v8 command line options\n"
      "  --max-stack-size=val set max v8 stack size (bytes)\n"
      "\n"
      "Environment variables:\n"
#ifdef _WIN32
      "NODE_PATH              ';'-separated list of directories\n"
#else
      "NODE_PATH              ':'-separated list of directories\n"
#endif
      "                       prefixed to the module search path.\n"
      "NODE_MODULE_CONTEXTS   Set to 1 to load modules in their own\n"
      "                       global contexts.\n"
      "NODE_DISABLE_COLORS    Set to 1 to disable colors in the REPL\n"
      "\n"
      "Documentation can be found at http://jxcore.com/\n");
}

// Parse node command line arguments.
void JXEngine::ParseArgs(int argc, char **argv) {
  int i = 1;

  std::string original("jxcore.bin(?@@?!$<$?!*)");

  if (original.c_str()[14] != '?') {
    main_node_->is_embedded_ = true;
  }

  if (!main_node_->is_embedded_) {
    for (; i < argc; i++) {
      const char *arg = argv[i];
      if (strstr(arg, "--debug") == arg) {
        ParseDebugOpt(arg);
        argv[i] = const_cast<char *>("");
      } else if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0) {
        printf("%s\n", NODE_VERSION);
        exit(0);
      } else if (strcmp(arg, "--jxversion") == 0 || strcmp(arg, "-jxv") == 0) {
        printf("%s\n", JXCORE_VERSION);
        exit(0);
      } else if (strstr(arg, "--max-stack-size=") == arg) {
        const char *p = 0;
        p = 1 + strchr(arg, '=');
        main_node_->max_stack_size = atoi(p);
        argv[i] = const_cast<char *>("");
      } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
        PrintHelp();
        exit(0);
      } else if (strcmp(arg, "--eval") == 0 || strcmp(arg, "-e") == 0 ||
                 strcmp(arg, "--print") == 0 || strcmp(arg, "-pe") == 0 ||
                 strcmp(arg, "-p") == 0) {
        bool is_eval = strchr(arg, 'e') != NULL;
        bool is_print = strchr(arg, 'p') != NULL;

        // argument to -p and --print is optional
        if (is_eval == true && i + 1 >= argc) {
          fprintf(stderr, "Error: %s requires an argument\n", arg);
          exit(13);
        }

        main_node_->print_eval = main_node_->print_eval || is_print;
        argv[i] = const_cast<char *>("");

        // --eval, -e and -pe always require an argument
        if (is_eval == true) {
          main_node_->eval_string = argv[++i];
          continue;
        }

        // next arg is the expression to evaluate unless it starts with:
        //  - a dash, then it's another switch
        //  - "\\-", then it's an escaped expression, drop the backslash
        if (argv[i + 1] == NULL) continue;
        if (argv[i + 1][0] == '-') continue;
        main_node_->eval_string = argv[++i];
        if (strncmp(main_node_->eval_string, "\\-", 2) == 0)
          ++main_node_->eval_string;
      } else if (strcmp(arg, "--interactive") == 0 || strcmp(arg, "-i") == 0) {
        main_node_->force_repl = true;
        argv[i] = const_cast<char *>("");
      } else if (strcmp(arg, "--v8-options") == 0) {
        argv[i] = const_cast<char *>("--help");
      } else if (strcmp(arg, "--no-deprecation") == 0) {
        argv[i] = const_cast<char *>("");
        node::no_deprecation = true;
      } else if (strcmp(arg, "--trace-deprecation") == 0) {
        argv[i] = const_cast<char *>("");
        main_node_->trace_deprecation = true;
      } else if (strcmp(arg, "--throw-deprecation") == 0) {
        argv[i] = const_cast<char *>("");
        main_node_->throw_deprecation = true;
      } else if (argv[i][0] != '-') {
        break;
      }
    }
  }
  main_node_->option_end_index = i;
}

char **JXEngine::Init(int argc, char *argv[], bool engine_inited_already) {
  // Initialize prog_start_time to get relative uptime.
  main_node_->prog_start_time = uv_now(main_node_->loop);

  if (!engine_inited_already) {
    // Make inherited handles noninheritable.
    uv_disable_stdio_inheritance();

    // Parse a few arguments which are specific to Node.
    ParseArgs(argc, argv);
    // Parse the rest of the args (up to the 'option_end_index' (where '--' was
    // in the command line))
    int v8argc = main_node_->option_end_index;
    char **v8argv = argv;

    if (main_node_->debug_wait_connect) {
      // v8argv is a copy of argv up to the script file argument +2 if
      // --debug-brk
      // to expose the v8 debugger js object so that node.js can set
      // a breakpoint on the first line of the startup script
      v8argc += 2;
      v8argv = new char *[v8argc];
      memcpy(v8argv, argv, sizeof(*argv) * main_node_->option_end_index);
      v8argv[main_node_->option_end_index] =
          const_cast<char *>("--expose_debug_as");
      v8argv[main_node_->option_end_index + 1] = const_cast<char *>("v8debug");
    }

// For the normal stack which moves from high to low addresses when frames
// are pushed, we can compute the limit as stack_size bytes below the
// the address of a stack variable (e.g. &stack_var) as an approximation
// of the start of the stack (we're assuming that we haven't pushed a lot
// of frames yet).
#ifdef JS_ENGINE_V8
    if (main_node_->max_stack_size != 0) {
      uint32_t stack_var;
      v8::ResourceConstraints constraints;

      uint32_t *stack_limit =
          &stack_var - (main_node_->max_stack_size / sizeof(uint32_t));
      constraints.set_stack_limit(stack_limit);
      v8::SetResourceConstraints(
          &constraints);  // Must be done before V8::Initialize
    }

    if (!main_node_->is_embedded_)
      v8::V8::SetFlagsFromCommandLine(&v8argc, v8argv, false);
#elif defined(JS_ENGINE_MOZJS)
// TODO(obastemur) set command line flags ?
#endif

#ifdef __POSIX__
    // Ignore SIGPIPE
    RegisterSignalHandler(SIGPIPE, SIG_IGN);
    RegisterSignalHandler(SIGINT, SignalExit);
    RegisterSignalHandler(SIGTERM, SignalExit);
#endif  // __POSIX__
  }

  uv_idle_t *t = main_node_->tick_spinner;
  uv_idle_init(main_node_->loop, t);

  uv_check_init(main_node_->loop, main_node_->check_immediate_watcher);
  uv_unref((uv_handle_t *)main_node_->check_immediate_watcher);
  uv_idle_init(main_node_->loop, main_node_->idle_immediate_dummy);

// Fetch a reference to the main isolate, so we have a reference to it
// even when we need it to access it from another (debugger) thread.
// com->node_isolate = Isolate::GetCurrent();

#if defined(JS_ENGINE_V8)
  main_node_->node_isolate = JS_CURRENT_ENGINE();
  if (!engine_inited_already) v8::V8::SetFatalErrorHandler(node::OnFatalError);
#elif defined(JS_ENGINE_MOZJS)
  JS_SetErrorReporter(main_node_->node_isolate->ctx_, node::OnFatalError);
#endif

  if (!engine_inited_already) {
    main_node_->setMainIsolate();

    // If the --debug flag was specified then initialize the debug thread.
    if (main_node_->use_debug_agent) {
      node::EnableDebug(main_node_->debug_wait_connect, main_node_);
    } else {
#ifdef _WIN32
      node::RegisterDebugSignalHandler();
#else   // Posix

      uv_signal_init(main_node_->loop, main_node_->signal_watcher);
      uv_signal_start(main_node_->signal_watcher,
                      node::EnableDebugSignalHandler, SIGUSR1);
      uv_unref((uv_handle_t *)main_node_->signal_watcher);
#endif  // __POSIX__
    }
  }

  return argv;
}

bool JXEngine::jxcore_was_shutdown_ = false;
bool JXEngine::JS_engine_inited_ = false;

#define jx_engine_map std::map<int, JXEngine *>
jx_engine_map jx_engine_instances;

JXEngine::JXEngine(int argc, char **argv, bool self_hosted) {
#ifdef JS_ENGINE_MOZJS
  // set only once for proxy
  MozJS::Value::SetGlobalFinalizer(node::ObjectWrap::WeakCallback);
  jscomp_ = NULL;
#endif

  assert(!jxcore_was_shutdown_);

  argc_ = argc;
  argv_ = argv;
  main_node_ = NULL;
  self_hosted_ = self_hosted;
  instance_inited_ = false;
  entry_file_name_ = "";
}

void JXEngine::Init() {
  if (main_node_ != NULL) {
    error_console("You should call Init(JXEngine) before StartEngine\n");
    exit(1);
  }

  if (instance_inited_) {
    error_console("This JXEngine instance was already Init'ed\n");
    exit(1);
  }

  instance_inited_ = true;
  jx_init_locks();
}

void JXEngine::Start() {
  if (!instance_inited_) {
    error_console(
        "This JXEngine instance wasn't Init'ed. Call 'Init' before "
        "StartEngine.\n");
    exit(1);
  }

  node::commons::process_status_ = node::JXCORE_INSTANCE_ALIVE;
  if (self_hosted_)
    InitializeEngine(argc_, argv_);
  else
    InitializeEmbeddedEngine(argc_, argv_);
}

void JXEngine::MemoryMap(const char *filename, const char *content,
                         bool entry_file) {
  if (!instance_inited_) {
    error_console(
        "This JXEngine instance wasn't Init'ed. Call 'Init' before this "
        "method.\n");
    exit(1);
  }

  if (entry_file) {
    if (entry_file_name_.length() != 0) {
      error_console(
          "You can set only 1 entry file per engine instance. You've already "
          "set %s\n",
          entry_file_name_.c_str());
      exit(1);
    }
    entry_file_name_ = filename;
  }

  node::MemoryWrap::SharedSet(filename, content);
}

void JXEngine::InitializeEngine(int argc, char **argv) {
#ifdef JS_ENGINE_V8
  const char *replaceInvalid = getenv("NODE_INVALID_UTF8");
  if (replaceInvalid == NULL)
    node::WRITE_UTF8_FLAGS |= v8::String::REPLACE_INVALID_UTF8;
#endif

  char **argv_copy;
  if (argc > 0) {
    argv_ = uv_setup_args(argc, argv);
    argv_copy = copy_argv(argc, argv_);
  } else if (node::commons::embedded_source_) {
    argv_copy = jx_source;
    argv_ = uv_setup_args(1, jx_source);
  } else {
    error_console(
        "missing startup parameters at JXEngine::Initialize*Engine.\n");
    abort();
  }
  argc_ = argc;

  const int actual_thread_id =
      node::commons::getAvailableThreadId(!jx_engine_instances.empty());
  jx_engine_instances[actual_thread_id] = this;

#ifdef JS_ENGINE_V8
#if HAVE_OPENSSL
  // V8 on Windows doesn't have a good source of entropy. Seed it from
  // OpenSSL's pool.
  v8::V8::SetEntropySource(node::crypto::EntropySource);
#endif
  if (actual_thread_id == 0) {
    main_node_ = new node::commons(0);
  } else {
    main_node_ = node::commons::newInstance(actual_thread_id);
  }
  // This needs to run *before* V8::Initialize()
  // Use copy here as to not modify the original argv:
  Init(argc, argv_copy, JS_engine_inited_);
  if (!JS_engine_inited_) {
    JS_engine_inited_ = true;
    v8::V8::Initialize();
  }
#elif defined(JS_ENGINE_MOZJS)
  bool was_inited = JS_engine_inited_;
  if (!JS_engine_inited_) {
    assert(actual_thread_id == 0);
    JS_engine_inited_ = true;
    if (!JS_Init()) {
      error_console("Couldn't initialize the SpiderMonkey JavaScript Engine\n");
      abort();
    }
  }

  if (actual_thread_id == 0) {
    main_node_ = new node::commons(0);
    main_node_->node_isolate = ENGINE_NS::Isolate::New(0);
    main_iso_ = *main_node_->node_isolate;
  } else {
    main_node_ = node::commons::newInstance(actual_thread_id);
    main_iso_ = *main_node_->node_isolate;
  }

  JSContext *ctx = main_iso_.ctx_;
  JSRuntime *rt = JS_GetRuntime(ctx);

  JS_SetInterruptCallback(rt, JSEngineInterrupt);
  JS_SetGCCallback(rt, GCOnMozJS, NULL);

  Init(argc, argv_copy, was_inited);
#endif
  {
#ifdef JS_ENGINE_V8
    v8::Locker locker;
    v8::HandleScope handle_scope;
    v8::Persistent<v8::Context> context = v8::Context::New();
    v8::Context::Scope context_scope(context);
#elif defined(JS_ENGINE_MOZJS)
    JSAutoRequest ar(ctx);
    JS::RootedObject global(ctx, jxcore::NewGlobalObject(ctx));
    assert(global != NULL);
    JSAutoCompartment ac(ctx, global);
#endif

    node::SetupProcessObject(actual_thread_id);
    JS_HANDLE_OBJECT process_l = main_node_->getProcess();
    if (entry_file_name_.length() != 0) {
      JS_DEFINE_STATE_MARKER(main_node_);
      JS_NAME_SET(process_l, JS_STRING_ID("entry_file_name_"),
                  STD_TO_STRING(entry_file_name_.c_str()));
    }

#ifdef JS_ENGINE_V8
    v8_typed_array::AttachBindings(context->Global());
#endif

    // Create all the objects, load modules, do everything.
    // so your next reading stop should be node::Load()!
    node::Load(process_l);

    // All our arguments are loaded. We've evaluated all of the scripts. We
    // might even have created TCP servers. Now we enter the main eventloop. If
    // there are no watchers on the loop (except for the ones that were
    // uv_unref'd) then this function exits. As long as there are active
    // watchers, it blocks.
    uv_run_jx(main_node_->loop, UV_RUN_DEFAULT, node::commons::CleanPinger, 0);

    node::EmitExit(process_l);

    if (main_node_->threadId == 0) WaitThreadExit(NULL);

    node::RunAtExit();

    if (main_node_->threadId == 0) {
      node::MemoryWrap::MapClear(true);
      Job::removeTaskers();
    }

#ifdef JS_ENGINE_V8
#ifndef NDEBUG
    context.Dispose();
#endif
#endif
  }
#if defined HAVE_DTRACE || defined HAVE_ETW || defined HAVE_SYSTEMTAP
  node::cleanUpDTrace();
#endif

#ifdef JS_ENGINE_V8
#ifndef NDEBUG
  // Clean up. Not strictly necessary.
  V8::Dispose();
  main_node_->Dispose();
#endif  // NDEBUG
#elif defined(JS_ENGINE_MOZJS)
  JS_DestroyContext(ctx);
  if (main_node_->threadId == 0)
    node::commons::process_status_ = node::JXCORE_INSTANCE_EXITED;
  else
    main_node_->instance_status_ = node::JXCORE_INSTANCE_EXITED;

  // SM can't do GC under GC. we need the destroy other contexts separately
  std::list<JSContext *>::iterator itc = main_node_->free_context_list_.begin();

  while (itc != main_node_->free_context_list_.end()) {
    JSContext *ctx_sub = *itc;
    JS_DestroyContext(*itc);
    itc++;
  }
  main_node_->free_context_list_.clear();
  main_node_->Dispose();

  JS_DestroyRuntime(rt);
  main_iso_.Dispose();
#endif

  // Clean up the copy:
  if (argc > 0) free(argv_copy);

  jx_engine_map::iterator it = jx_engine_instances.find(actual_thread_id);
  if (it != jx_engine_instances.end()) jx_engine_instances.erase(it);

  if (main_node_->threadId == 0) {
    node::commons::threadPoolCount = 0;
  }
  delete main_node_;
}

#define DECLARE_NATIVE_METHOD(name, method)                   \
  do {                                                        \
    JS_LOCAL_FUNCTION __temp =                                \
        JS_GET_FUNCTION(JS_NEW_FUNCTION_TEMPLATE(method));    \
    JS_NAME_SET(native_methods_, JS_STRING_ID(name), __temp); \
  } while (0)

#if defined(JS_ENGINE_MOZJS)
void JXEngine::InitializeEmbeddedEngine(int argc, char **argv) {
  char **argv_copy;
  if (argc > 0) {
    argv_ = uv_setup_args(argc, argv);
    argv_copy = copy_argv(argc, argv_);
  } else if (node::commons::embedded_source_) {
    argv_copy = jx_source;
    argv_ = uv_setup_args(1, jx_source);
  } else {
    error_console(
        "missing startup parameters at JXEngine::Initialize*Engine.\n");
    abort();
  }

  argc_ = argc;
  const int actual_thread_id =
      node::commons::getAvailableThreadId(!jx_engine_instances.empty());
  jx_engine_instances[actual_thread_id] = this;

  bool was_inited = JS_engine_inited_;
  if (!JS_engine_inited_) {
    assert(actual_thread_id == 0);
    JS_engine_inited_ = true;
    if (!JS_Init()) {
      error_console("Couldn't initialize the SpiderMonkey JavaScript Engine\n");
      abort();
    }
  }

  if (actual_thread_id == 0) {
    main_node_ = new node::commons(0);
    main_node_->node_isolate = ENGINE_NS::Isolate::New(0);
    main_iso_ = *main_node_->node_isolate;
  } else {
    main_node_ = node::commons::newInstance(actual_thread_id);
    main_iso_ = *main_node_->node_isolate;
  }

  JSContext *ctx = main_iso_.ctx_;
  JSRuntime *rt = JS_GetRuntime(ctx);

  JS_SetInterruptCallback(rt, JSEngineInterrupt);
  JS_SetGCCallback(rt, GCOnMozJS, NULL);

  Init(argc, argv_copy, was_inited);

  JS_BeginRequest(ctx);
  JS::RootedObject global(ctx, jxcore::NewGlobalObject(ctx));
  assert(global != NULL);
  jscomp_ = JS_EnterCompartment(ctx, global);

  JS_LOCAL_OBJECT global_object(global, ctx);
  global_ = JS_NEW_PERSISTENT_OBJECT(global_object);

  node::SetupProcessObject(actual_thread_id);
  JS_HANDLE_OBJECT process_l = main_node_->getProcess();
  JS_DEFINE_STATE_MARKER(main_node_);
  if (entry_file_name_.length() != 0) {
    JS_NAME_SET(process_l, JS_STRING_ID("entry_file_name_"),
                STD_TO_STRING(entry_file_name_.c_str()));
  }

  native_methods_ = JS_NEW_EMPTY_PERSISTENT_OBJECT();
  JS_NAME_SET(process_l, JS_STRING_ID("natives"), native_methods_);
  for (std::map<std::string, JS_NATIVE_METHOD>::iterator it =
           methods_to_initialize_.begin();
       it != methods_to_initialize_.end(); ++it) {
    DECLARE_NATIVE_METHOD(it->first.c_str(), it->second);
  }
  methods_to_initialize_.clear();

  node::Load(process_l);

  if (argc > 0) free(argv_copy);
}

void JXEngine::Destroy() {
  if (main_node_->threadId == 0)
    node::commons::process_status_ = node::JXCORE_INSTANCE_EXITING;
  else
    main_node_->instance_status_ = node::JXCORE_INSTANCE_EXITING;

  JS_HANDLE_OBJECT process_l = main_node_->getProcess();
  JSContext *ctx = main_iso_.ctx_;
  JSRuntime *rt = JS_GetRuntime(ctx);

  node::EmitExit(process_l);

  if (main_node_->threadId == 0 && getThreadCount() > 0)
    WaitThreadExit(main_node_->loop);

  JS_CLEAR_PERSISTENT(native_methods_);

  node::RunAtExit();

  if (main_node_->threadId == 0) {
    node::MemoryWrap::MapClear(true);
    Job::removeTaskers();
  }

  JS_DEFINE_STATE_MARKER(main_node_);

  global_->RemoveRoot();
  main_node_->Dispose();

  JS_LeaveCompartment(ctx, jscomp_);
  JS_EndRequest(ctx);

  JS_DestroyContext(ctx);
  if (main_node_->threadId == 0)
    node::commons::process_status_ = node::JXCORE_INSTANCE_EXITED;
  else
    main_node_->instance_status_ = node::JXCORE_INSTANCE_EXITED;

  // SM can't do GC under GC. we need the destroy other contexts separately
  std::list<JSContext *>::iterator itc = main_node_->free_context_list_.begin();

  while (itc != main_node_->free_context_list_.end()) {
    JSContext *ctx_sub = *itc;
    JS_DestroyContext(*itc);
    itc++;
  }
  main_node_->free_context_list_.clear();

  JS_DestroyRuntime(rt);

  jx_engine_map::iterator it = jx_engine_instances.find(main_node_->threadId);
  if (it != jx_engine_instances.end()) jx_engine_instances.erase(it);

  main_iso_.Dispose();
  delete main_node_;

  if (jx_engine_instances.size() == 0) {
    jx_destroy_locks();
    node::commons::threadPoolCount = 0;
  }
}
#endif

#if defined(JS_ENGINE_V8)
void JXEngine::InitializeEmbeddedEngine(int argc, char **argv) {
  if (!JS_engine_inited_) {
    const char *replaceInvalid = getenv("NODE_INVALID_UTF8");
    if (replaceInvalid == NULL)
      node::WRITE_UTF8_FLAGS |= v8::String::REPLACE_INVALID_UTF8;
  }

  char **argv_copy;
  if (argc > 0) {
    argv_ = uv_setup_args(argc, argv);
    argv_copy = copy_argv(argc, argv_);
  } else if (node::commons::embedded_source_) {
    argv_copy = jx_source;
    argv_ = uv_setup_args(1, jx_source);
  } else {
    error_console(
        "missing startup parameters at JXEngine::Initialize*Engine.\n");
    abort();
  }
  argc_ = argc;

  const int actual_thread_id =
      node::commons::getAvailableThreadId(!jx_engine_instances.empty());
  jx_engine_instances[actual_thread_id] = this;

  if (actual_thread_id == 0) {
    main_node_ = new node::commons(0);
  } else {
    main_node_ = node::commons::newInstance(actual_thread_id);
  }
  // This needs to run *before* V8::Initialize()
  // Use copy here as to not modify the original argv:
  Init(argc, argv_copy, JS_engine_inited_);

  if (!JS_engine_inited_) {
#if HAVE_OPENSSL
    // V8 on Windows doesn't have a good source of entropy. Seed it from
    // OpenSSL's pool.
    v8::V8::SetEntropySource(node::crypto::EntropySource);
#endif
    JS_engine_inited_ = true;
    v8::V8::Initialize();
  }

  v8::Locker locker;
  v8::HandleScope handle_scope;
  context_ = v8::Context::New();
  v8::Context::Scope context_scope(context_);

  node::SetupProcessObject(actual_thread_id);
  JS_HANDLE_OBJECT process_l = main_node_->getProcess();
  v8_typed_array::AttachBindings(context_->Global());

  JS_DEFINE_STATE_MARKER(main_node_);
  if (entry_file_name_.length() != 0) {
    JS_NAME_SET(process_l, JS_STRING_ID("entry_file_name_"),
                STD_TO_STRING(entry_file_name_.c_str()));
  }

  native_methods_ = JS_NEW_EMPTY_PERSISTENT_OBJECT();
  JS_NAME_SET(process_l, JS_STRING_ID("natives"), native_methods_);
  for (std::map<std::string, JS_NATIVE_METHOD>::iterator it =
           methods_to_initialize_.begin();
       it != methods_to_initialize_.end(); ++it) {
    DECLARE_NATIVE_METHOD(it->first.c_str(), it->second);
  }
  methods_to_initialize_.clear();

  node::Load(process_l);

  if (argc > 0) free(argv_copy);
}

void JXEngine::Destroy() {
  // need it for scope leave
  {
    if (main_node_->threadId == 0)
      node::commons::process_status_ = node::JXCORE_INSTANCE_EXITING;
    else
      main_node_->instance_status_ = node::JXCORE_INSTANCE_EXITING;
    JS_ENGINE_SCOPE();

    JS_HANDLE_OBJECT process_l = main_node_->getProcess();
    node::EmitExit(process_l);

    if (main_node_->threadId == 0 && getThreadCount() > 0)
      WaitThreadExit(main_node_->loop);

    JS_CLEAR_PERSISTENT(native_methods_);

    node::RunAtExit();

    if (main_node_->threadId == 0) {
      node::MemoryWrap::MapClear(true);
      Job::removeTaskers();
    }

    main_node_->Dispose();
  }

  if (main_node_->threadId == 0)
    node::commons::process_status_ = node::JXCORE_INSTANCE_EXITED;
  else
    main_node_->instance_status_ = node::JXCORE_INSTANCE_EXITED;

#if defined HAVE_DTRACE || defined HAVE_ETW || defined HAVE_SYSTEMTAP
  node::cleanUpDTrace();
#endif

#ifndef NDEBUG
  context_.Dispose();
#endif

  jx_engine_map::iterator it = jx_engine_instances.find(main_node_->threadId);
  if (it != jx_engine_instances.end()) jx_engine_instances.erase(it);

  delete main_node_;

  if (jx_engine_instances.size() == 0) {
    jx_destroy_locks();
    node::commons::threadPoolCount = 0;
  }
}
#endif

char *Stringify(node::commons *com, JS_HANDLE_OBJECT obj) {
  JS_ENTER_SCOPE();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_STRING str_value;
  if (!JS_IS_STRING(obj)) {
    JS_LOCAL_BOOLEAN is_function;

#ifdef JS_ENGINE_V8
    is_function = STD_TO_BOOLEAN(false);
#elif defined(JS_ENGINE_MOZJS)
    // SpiderMonkey JSON.stringify follows the ECMA 5 specs. exactly.
    // (if it's_callable -> do not -> stringify)
    is_function = STD_TO_BOOLEAN(JS_IS_FUNCTION(obj));
#endif

    JS_HANDLE_VALUE args[] = {obj, is_function};

    // Init
    if (JS_IS_EMPTY((com->JSONstringify))) {
      JS_LOCAL_FUNCTION _JSONstringify = JS_CAST_FUNCTION(
          JS_COMPILE_AND_RUN(STD_TO_STRING(
                                 "(function(obj, is_function) {\n"
                                 "try {\n"
                                 "if(is_function)\n"
                                 "{\n"
                                 "var b={};\n"
                                 "for(var o in obj){\n"
                                 "b[o] = obj[o];\n"
                                 "}\n"
                                 "obj = b;\n"
                                 "}\n"
                                 "return JSON.stringify(obj);\n"
                                 "} catch (e) {\n"
                                 "return 'undefined';\n"
                                 "}\n"
                                 "});"),
                             STD_TO_STRING("binding:stringify")));
      com->JSONstringify = JS_NEW_PERSISTENT_FUNCTION(_JSONstringify);
    }

    JS_LOCAL_VALUE result =
        JS_METHOD_CALL(com->JSONstringify, JS_GET_GLOBAL(), 2, args);
    str_value = JS_VALUE_TO_STRING(result);
  } else {
    str_value = JS_VALUE_TO_STRING(obj);
  }

  JXString str(str_value);
  str.DisableAutoGC();
  return *str;
}

bool JXEngine::Evaluate(const char *source, const char *filename,
                        JXResult *result) {
  JS_ENGINE_SCOPE();

  JS_LOCAL_STRING source_ = UTF8_TO_STRING(source);
  JS_LOCAL_STRING filename_ = STD_TO_STRING(filename);

  JS_TRY_CATCH(try_catch);

  JS_LOCAL_SCRIPT script = JS_SCRIPT_COMPILE(source_, filename_);

  if (JS_IS_EMPTY(script)) {
    // TODO(obastemur) report exception back to embedder?
    node::ReportException(try_catch, true);
    return false;
  }

  JS_LOCAL_VALUE ret_val = JS_SCRIPT_RUN(script);

  if (try_catch.HasCaught()) {
    node::ReportException(try_catch, true);
    // TODO(obastemur) report exception back to embedder?
    return false;
  }

  result->empty_ = false;

  if (JS_IS_NULL_OR_UNDEFINED(ret_val)) {
    result->type_ = RT_NullUndefined;

    return true;
  }

  if (JS_IS_BOOLEAN(ret_val)) {
    result->type_ = RT_Boolean;
    bool *data = new bool[1];
    data[0] = BOOLEAN_TO_STD(ret_val);
    result->data_ = (void *)data;

    return true;
  }

  if (JS_IS_NUMBER(ret_val)) {
    if (JS_IS_INT32(ret_val)) {
      result->type_ = RT_Int32;
      int32_t *data = new int32_t[1];
      data[0] = INT32_TO_STD(ret_val);
      result->data_ = (void *)data;
    } else {
      result->type_ = RT_Double;
      double *data = new double[1];
      data[0] = NUMBER_TO_STD(ret_val);
      result->data_ = (void *)data;
    }
    return true;
  }

  if (JS_IS_STRING(ret_val)) {
    result->type_ = RT_String;

    JXString str(ret_val);
    str.DisableAutoGC();

    result->data_ = *str;

    return true;
  }

  if (!JS_IS_OBJECT(ret_val)) {
    result->type_ = RT_NullUndefined;

    return true;
  }

  result->type_ = RT_JSON;
  result->data_ = Stringify(main_node_, JS_VALUE_TO_OBJECT(ret_val));

  return true;
}

void JXEngine::DefineNativeMethod(const char *name, JS_NATIVE_METHOD method) {
  if (main_node_ == NULL) {
    methods_to_initialize_[name] = method;
  } else {
    JS_ENGINE_SCOPE();

    DECLARE_NATIVE_METHOD(name, method);
  }
}

int JXEngine::Loop() {
  JS_ENGINE_SCOPE();

  return uv_run_jx(main_node_->loop, UV_RUN_DEFAULT, node::commons::CleanPinger,
                   0);
}

int JXEngine::LoopOnce() {
  JS_ENGINE_SCOPE();

  return uv_run_jx(main_node_->loop, UV_RUN_NOWAIT, node::commons::CleanPinger,
                   0);
}

void JXEngine::ShutDown() {
  node::commons::process_status_ = node::JXCORE_INSTANCE_EXITED;
#if defined(JS_ENGINE_MOZJS)
#elif defined(JS_ENGINE_V8) && !defined(NDEBUG)
  // Clean up. Not strictly necessary.
  V8::Dispose();
#endif
  jxcore_was_shutdown_ = true;
}

JXEngine *JXEngine::ActiveInstance() {
  if (jx_engine_instances.empty()) return NULL;

  const int actual_thread_id = node::commons::getCurrentThreadId();
  jx_engine_map::iterator it = jx_engine_instances.find(actual_thread_id);
  if (it == jx_engine_instances.end()) return NULL;

  return it->second;
}

JXEngine *JXEngine::GetInstanceByThreadId(const int threadId) {
  if (jx_engine_instances.empty()) return NULL;

  jx_engine_map::iterator it = jx_engine_instances.find(threadId);
  if (it == jx_engine_instances.end()) return NULL;

  return it->second;
}

}  // namespace jxcore
