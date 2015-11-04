// Copyright & License details are available under JXCORE_LICENSE file

#include "jxcore.h"
#include "jx/extend.h"
#include "wrappers/memory_wrap.h"
#include "jx/error_definition.h"
#include "node_version.h"
#include "jx/job_store.h"
#include "string_bytes.h"
#include "jxcore_type_wrap.h"
#include "wrappers/node_crypto.h"
#include <stdlib.h>
#include <map>
#include <list>

#if defined(_MSC_VER)
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

#if HAVE_OPENSSL
#include "node_crypto.h"
#endif

namespace jxcore {

static void SignalExit(int signal) {
  uv_tty_reset_mode();
  _exit(128 + signal);
}

#ifdef __POSIX__
void RegisterSignalHandler(int signal, void (*handler)(int)) {
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = handler;
  sigfillset(&sa.sa_mask);
  sigaction(signal, &sa, NULL);
}
#endif

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
  ENGINE_LOG_THIS("jxcore.cc", "WaitThreadExit");
  if (getThreadCount() > 0) Sleep(5);

  bool busy = getThreadCount() > 0;
  int counter = 0;

  for (int i = 0; i < 20; i++) {  // ~2sec max thread cleanup wait (embedded)
    int sleep_counter = 0;
    while (busy && counter < 100 && sleep_counter++ < 100) {
      busy = getThreadCount() > 0;
      counter++;
      if (ev_loop)
        counter -=
            uv_run_jx(ev_loop, UV_RUN_NOWAIT, node::commons::CleanPinger, 0);

      if (!busy) break;

      Sleep(1);
    }

    if (!busy) break;

    counter = 0;
  }
}

#ifndef JXCORE_EMBEDDED
void JXEngine::ParseDebugOpt(const char *arg) {
#ifdef JS_ENGINE_V8
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
#elif defined(JS_ENGINE_MOZJS)
  error_console(
      "This JXcore build uses SpiderMonkey engine. For now SM build doesn't "
      "support debugging.\n");
#endif
  exit(12);
}

void JXEngine::PrintHelp() {
  printf(
      "Usage: jx [options] [ -e script | script.js ] [arguments] \n"
#ifdef JS_ENGINE_V8
      "       jx debug script.js [arguments] \n"
#endif
      "\n"
      "Options:\n"
      "  compile              compile jx file from jxp project\n"
      "  package              create jxp project from the folder (recursive)\n"
      "  install              install a module from the repository\n"
      "  mt                   multithread the given application (no-keep)\n"
      "  mt-keep              multithread the given application (keep "
      "alive)\n\n"
      "  -jxv, --jxversion    print jxcore's version\n"
      "  -jsv, --jsversion    print underlying JS engine's name and version\n"
      "  -a, --arch           print processor architecture, for which jx is "
      "compiled (process.arch)\n"
      "  -v, --version        print corresponding node's version\n"
      "  -e, --eval script    evaluate script\n"
      "  -p, --print          evaluate script and print result\n"
      "  -i, --interactive    always enter the REPL even if stdin\n"
      "                       does not appear to be a terminal\n"
      "  --no-deprecation     silence deprecation warnings\n"
      "  --trace-deprecation  show stack traces on deprecations\n"
#ifdef JS_ENGINE_V8
      "  --v8-options         print v8 command line options\n"
      "  --max-stack-size=val set max v8 stack size (bytes)\n"
#endif
      "  --enable-ssl2        enable ssl2\n"
      "  --enable-ssl3        enable ssl3\n"
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
      "Documentation can be found at http://jxcore.com/docs/\n");
}
#endif

// Parse node command line arguments.
void JXEngine::ParseArgs(int argc, char **argv) {
  ENGINE_LOG_THIS("JXEngine", "ParseArgs");
  int i = 1;

#ifndef JXCORE_EMBEDDED
  std::string original("jxcore.bin(?@@?!$<$?!*)");

  if (original.c_str()[14] != '?') {
    main_node_->is_packaged_ = true;
  }

  if (!main_node_->is_packaged_) {
    for (; i < argc; i++) {
      const char *arg = argv[i];
      if (strstr(arg, "--debug") == arg) {
        ParseDebugOpt(arg);
        argv[i] = const_cast<char *>("");
      } else if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0) {
        log_console("%s\n", NODE_VERSION);
        exit(0);
      } else if (strcmp(arg, "--jsversion") == 0 || strcmp(arg, "-jsv") == 0) {
#ifdef JS_ENGINE_V8
#ifdef JS_ENGINE_CHAKRA
        flush_console("Microsoft Chakra v%s\n", v8::V8::GetVersion());
#else
        flush_console("Google V8 v%s\n", v8::V8::GetVersion());
#endif
#elif defined(JS_ENGINE_MOZJS)
        flush_console("Mozilla SpiderMonkey v%d\n", MOZJS_VERSION);
#endif
        exit(0);
      } else if (strcmp(arg, "--arch") == 0 || strcmp(arg, "-a") == 0) {
        log_console("%s\n", ARCH);
        exit(0);
      } else if (strcmp(arg, "--jxversion") == 0 || strcmp(arg, "-jxv") == 0) {
        log_console("%s\n", JXCORE_VERSION);
        exit(0);
      } else if (strstr(arg, "--max-stack-size=") == arg) {
        const char *p = 0;
        p = 1 + strchr(arg, '=');
        main_node_->max_stack_size = atoi(p);
        argv[i] = const_cast<char *>("");
#ifndef JS_ENGINE_V8
        error_console("--max-stack-size flag has no effect for non-V8 build\n");
#endif
      } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
        PrintHelp();
        exit(0);
      } else if (strcmp(arg, "--enable-ssl2") == 0) {
        node::SSL2_ENABLE = true;
        argv[i] = const_cast<char *>("");
      } else if (strcmp(arg, "--enable-ssl3") == 0) {
        node::SSL3_ENABLE = true;
        argv[i] = const_cast<char *>("");
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
#ifndef JS_ENGINE_V8
        error_console(" --v8-options are not available for non-V8 build\n");
        exit(0);
#endif
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
#endif
  main_node_->option_end_index = i;
}

char **JXEngine::Init(int argc, char *argv[], bool engine_inited_already) {
  ENGINE_LOG_THIS("JXEngine", "Init");
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
#ifdef V8_IS_3_14
    if (main_node_->max_stack_size != 0) {
      uint32_t stack_var;
      v8::ResourceConstraints constraints;

      uint32_t *stack_limit =
          &stack_var - (main_node_->max_stack_size / sizeof(uint32_t));
      constraints.set_stack_limit(stack_limit);
      v8::SetResourceConstraints(
          &constraints);  // Must be done before V8::Initialize
    }
#endif

    if (!main_node_->is_packaged_)
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

#if defined(JS_ENGINE_MOZJS)
  JS_SetErrorReporter(main_node_->node_isolate->GetRaw(), node::OnFatalError);
#endif

  if (!engine_inited_already) {
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

static bool jxcore_first_instance = true;
bool JXEngine::jxcore_was_shutdown_ = false;
bool JXEngine::JS_engine_inited_ = false;
static std::map<std::string, JXMethod> methods_archive;

#define jx_engine_map std::map<int, JXEngine *>
jx_engine_map jx_engine_instances;

class AutoScope {
  JXEngine *engine_;
  const bool leave_isolate_;

 public:
  AutoScope(JXEngine *_, const bool li) : leave_isolate_(li) {
    engine_ = _;
    _->EnterScope();
  }

  ~AutoScope() {
    engine_->LeaveScope();
    if (leave_isolate_ && engine_->threadId_ != 0) {
      engine_->ExitIsolate();
    }
  }
};

JXEngine::JXEngine(int argc, char **argv, bool self_hosted) {
  ENGINE_LOG_THIS("JXEngine", "JXEngine");
  if (jxcore_first_instance) {
    jxcore_first_instance = false;
    node::commons::self_hosted_process_ = self_hosted;
  } else {
    assert(node::commons::self_hosted_process_ == self_hosted &&
           "Once it's set self_hosted option can not be different for other "
           "instances");
  }

  inside_scope_ = false;
  threadId_ = node::commons::getAvailableThreadId(!jx_engine_instances.empty());
  customLock(CSLOCK_JOBS);
  jx_engine_instances[threadId_] = this;
  customUnlock(CSLOCK_JOBS);
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
  entry_file_name_ = "";
}

void JXEngine::Init() {
  ENGINE_LOG_THIS("JXEngine", "Init");
  static bool instance_inited_ = false;

  if (instance_inited_) {
    error_console("JXEngine was already Init'ed\n");
#ifndef JXCORE_EMBEDDED
    exit(1);
#else
    return;
#endif
  }

  instance_inited_ = true;
  jx_init_locks();
}

void JXEngine::Start() {
  ENGINE_LOG_THIS("JXEngine", "Start");
  node::commons::process_status_ = node::JXCORE_INSTANCE_ALIVE;
  if (self_hosted_)
    InitializeEngine(argc_, argv_);
  else
    InitializeEmbeddedEngine(argc_, argv_);
}

void JXEngine::MemoryMap(const char *filename, const char *content,
                         bool entry_file) {
  ENGINE_LOG_THIS("JXEngine", "MemoryMap");
  if (entry_file) {
    if (entry_file_name_.length() != 0) {
      error_console(
          "You can set only 1 entry file per engine instance. You've already "
          "set %s\n",
          entry_file_name_.c_str());
#ifndef JXCORE_EMBEDDED
      exit(1);
#else
      return;
#endif
    }
    entry_file_name_ = filename;
  }

  node::MemoryWrap::SharedSet(filename, content);
}

#ifdef JS_ENGINE_MOZJS
void JXEngine::InitializeEngine(int argc, char **argv) {
  ENGINE_LOG_THIS("JXEngine", "InitializeEngine");
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

  const int actual_thread_id = threadId_;

  AutoScope _scope_(this, false);

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
    main_node_->setMainIsolate();
  } else {
    main_node_ = node::commons::newInstance(actual_thread_id);
    main_iso_ = *main_node_->node_isolate;
  }

  JSContext *ctx = main_iso_.GetRaw();
  JSRuntime *rt = JS_GetRuntime(ctx);
  do {
    Init(argc, argv_copy, was_inited);
    do {
      JSAutoRequest ar(ctx);
      JS::RootedObject global(ctx);
      jxcore::NewGlobalObject(ctx, &global);
      assert(global != NULL);
      JSAutoCompartment ac(ctx, global);

      node::SetupProcessObject(actual_thread_id);
      JS_HANDLE_OBJECT process_l = main_node_->getProcess();
      if (entry_file_name_.length() != 0) {
        JS_DEFINE_STATE_MARKER(main_node_);
        JS_NAME_SET(process_l, JS_STRING_ID("entry_file_name_"),
                    STD_TO_STRING(entry_file_name_.c_str()));
      }

      main_node_->loop->loopId = main_node_->threadId;

      node::Load(process_l);

      // All our arguments are loaded. We've evaluated all of the scripts. We
      // might even have created TCP servers. Now we enter the main eventloop.
      // If
      // there are no watchers on the loop (except for the ones that were
      // uv_unref'd) then this function exits. As long as there are active
      // watchers, it blocks.
      uv_run_jx(main_node_->loop, UV_RUN_DEFAULT, node::commons::CleanPinger,
                main_node_->threadId);

      node::EmitExit(process_l);

      if (main_node_->threadId == 0 && getThreadCount() > 0)
        WaitThreadExit(main_node_->loop);

      node::RunAtExit();

      if (main_node_->threadId == 0) {
        node::MemoryWrap::MapClear(true);
        Job::removeTaskers();
      }
    } while (0);

#if defined HAVE_DTRACE || defined HAVE_ETW || defined HAVE_SYSTEMTAP
    if (main_node_->threadId == 0) node::cleanUpDTrace();
#endif

    if (main_node_->threadId == 0) {
      node::commons::process_status_ = node::JXCORE_INSTANCE_EXITED;
    } else {
      main_node_->instance_status_ = node::JXCORE_INSTANCE_EXITED;
    }

    if (main_node_->threadId == 0) {
      node::commons::threadPoolCount = 0;
    }

    main_node_->Dispose();
    node::removeCommons();
  } while (0);

  // Clean up the copy:
  if (argc > 0) free(argv_copy);

  customLock(CSLOCK_JOBS);
  jx_engine_map::iterator it = jx_engine_instances.find(actual_thread_id);
  if (it != jx_engine_instances.end()) jx_engine_instances.erase(it);
  customUnlock(CSLOCK_JOBS);
}
#elif defined(JS_ENGINE_V8)
void JXEngine::InitializeEngine(int argc, char **argv) {
  ENGINE_LOG_THIS("JXEngine", "InitializeEngine");
  const char *replaceInvalid = getenv("NODE_INVALID_UTF8");
  if (replaceInvalid == NULL)
    node::WRITE_UTF8_FLAGS |= v8::String::REPLACE_INVALID_UTF8;

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

  const int actual_thread_id = threadId_;

  AutoScope _scope_(this, false);

#if HAVE_OPENSSL
  // V8 on Windows doesn't have a good source of entropy. Seed it from
  // OpenSSL's pool.
  v8::V8::SetEntropySource(node::crypto::EntropySource);
#endif
  node::commons *com = NULL;
  if (actual_thread_id == 0) {
    main_node_ = new node::commons(0);
  } else {
    main_node_ = node::commons::newInstance(actual_thread_id);
    com = main_node_;
  }
  // This needs to run *before* V8::Initialize()
  // Use copy here as to not modify the original argv:
  Init(argc, argv_copy, JS_engine_inited_);
  if (!JS_engine_inited_) {
    JS_engine_inited_ = true;
    JS_ENGINE_INITIALIZE();
  }
  do {
    do {
      JS_ENGINE_LOCKER();

      JS_NEW_CONTEXT(context, isolate, NULL);
#ifndef V8_IS_3_14
      pContext_.Reset(isolate, context);
#else
      pContext_ = context;
#endif

      v8::Context::Scope context_scope(context);
      v8::V8::SetFatalErrorHandler(node::OnFatalError);
      if (actual_thread_id == 0) {
        main_node_->node_isolate = isolate;
        main_node_->setMainIsolate();
      }

      node::SetupProcessObject(actual_thread_id);
      JS_HANDLE_OBJECT process_l = main_node_->getProcess();
      if (entry_file_name_.length() != 0) {
        JS_DEFINE_STATE_MARKER(main_node_);
        JS_NAME_SET(process_l, JS_STRING_ID("entry_file_name_"),
                    STD_TO_STRING(entry_file_name_.c_str()));
      }

#ifdef V8_IS_3_14
      v8_typed_array::AttachBindings(context->Global());
#endif

      main_node_->loop->loopId = main_node_->threadId;

      node::Load(process_l);

      // All our arguments are loaded. We've evaluated all of the scripts. We
      // might even have created TCP servers. Now we enter the main eventloop.
      // If
      // there are no watchers on the loop (except for the ones that were
      // uv_unref'd) then this function exits. As long as there are active
      // watchers, it blocks.
      uv_run_jx(main_node_->loop, UV_RUN_DEFAULT, node::commons::CleanPinger,
                main_node_->threadId);

      node::EmitExit(process_l);

      if (main_node_->threadId == 0 && getThreadCount() > 0)
        WaitThreadExit(main_node_->loop);

      node::RunAtExit();

      if (main_node_->threadId == 0) {
        node::MemoryWrap::MapClear(true);
        Job::removeTaskers();
      }

#ifndef NDEBUG
      context.Dispose();
#endif
    } while (0);

#if defined HAVE_DTRACE || defined HAVE_ETW || defined HAVE_SYSTEMTAP
    if (main_node_->threadId == 0) node::cleanUpDTrace();
#endif

    if (main_node_->threadId == 0) {
      node::commons::process_status_ = node::JXCORE_INSTANCE_EXITED;
    } else {
      main_node_->instance_status_ = node::JXCORE_INSTANCE_EXITED;
    }

    if (main_node_->threadId == 0) {
      node::commons::threadPoolCount = 0;
    }

    main_node_->Dispose();
    node::removeCommons();
  } while (0);

#ifndef NDEBUG
  // Clean up. Not strictly necessary.
  V8::Dispose();
#endif  // NDEBUG

  // Clean up the copy:
  if (argc > 0) free(argv_copy);

  customLock(CSLOCK_JOBS);
  jx_engine_map::iterator it = jx_engine_instances.find(actual_thread_id);
  if (it != jx_engine_instances.end()) jx_engine_instances.erase(it);
  customUnlock(CSLOCK_JOBS);
}
#endif

void DeclareProxy(node::commons *com, JS_HANDLE_OBJECT_REF methods,
                  const char *name, int interface_id,
                  JS_NATIVE_METHOD native_method) {
  ENGINE_LOG_THIS("JXEngine", "DeclareProxy");
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);

  char *script = (char *)malloc(1024);
  int len = sprintf(script,
                    "(function(target, host) {\n"
                    "  host.%s = function() {\n"
                    "    var arr = [];\n"
                    "    arr[0] = %d;\n"
                    "    for (var i=0,ln=arguments.length; i<ln; i++) {\n"
                    "      arr[i+1] = arguments[i];\n"
                    "    }\n"
                    "    return host.%s.ref.apply(null, arr);\n"
                    "  };\n"
                    "  host.%s.ref = target.%s;\n"
                    "})",
                    name, interface_id, name, name, name);

  assert(len < 1024);
  script[len] = '\0';

  JS_LOCAL_FUNCTION defineProxy = JS_CAST_FUNCTION(JS_COMPILE_AND_RUN(
      STD_TO_STRING(script), STD_TO_STRING("proxy_method:script")));
  free(script);

  JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
  JS_LOCAL_OBJECT temp = JS_NEW_EMPTY_OBJECT();
  NODE_SET_METHOD(obj, name, native_method);

  JS_LOCAL_VALUE args[2] = {obj, JS_TYPE_TO_LOCAL_VALUE(methods)};
  JS_METHOD_CALL(defineProxy, JS_GET_GLOBAL(), 2, args);
}

void JXEngine::InitializeProxyMethods(node::commons *com) {
  ENGINE_LOG_THIS("JXEngine", "InitializeProxyMethods");
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);
  JS_HANDLE_OBJECT process_l = com->getProcess();

  JS_LOCAL_OBJECT methods = JS_NEW_EMPTY_OBJECT();
  for (std::map<std::string, JXMethod>::iterator it = methods_archive.begin();
       it != methods_archive.end(); ++it) {
    if (it->second.is_native_method_)
      NODE_SET_METHOD(methods, it->first.c_str(), it->second.native_method_);
    else
      DeclareProxy(com, methods, it->first.c_str(), it->second.interface_id_,
                   it->second.native_method_);
  }

  JS_NAME_SET(process_l, JS_STRING_ID("natives"), methods);
}

#if defined(JS_ENGINE_MOZJS)
void JXEngine::InitializeEmbeddedEngine(int argc, char **argv) {
  ENGINE_LOG_THIS("JXEngine", "InitializeEmbeddedEngine");
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
  const int actual_thread_id = threadId_;

  bool was_inited = JS_engine_inited_;
  if (!JS_engine_inited_) {
    assert(actual_thread_id == 0);
    JS_engine_inited_ = true;
    if (!JS_Init()) {
      error_console("Couldn't initialize the SpiderMonkey JavaScript Engine\n");
      abort();
    }
  }

  AutoScope _scope_(this, false);

  if (actual_thread_id == 0) {
    main_node_ = new node::commons(0);
    main_node_->node_isolate = ENGINE_NS::Isolate::New(0);
    main_iso_ = *main_node_->node_isolate;
    main_node_->setMainIsolate();
  } else {
    main_node_ = node::commons::newInstance(actual_thread_id);
    main_iso_ = *main_node_->node_isolate;
  }

  JSContext *ctx = main_iso_.GetRaw();
  JSRuntime *rt = JS_GetRuntime(ctx);

  Init(argc, argv_copy, was_inited);

  JS_BeginRequest(ctx);
  JS::RootedObject global(ctx);
  jxcore::NewGlobalObject(ctx, &global);
  assert(global != NULL);
  jscomp_ = JS_EnterCompartment(ctx, global);
  global_ = new MozJS::Value(global, ctx);
  global_->AddRoot();

  node::SetupProcessObject(actual_thread_id);
  JS_HANDLE_OBJECT process_l = main_node_->getProcess();
  JS_DEFINE_STATE_MARKER(main_node_);
  if (entry_file_name_.length() != 0) {
    JS_NAME_SET(process_l, JS_STRING_ID("entry_file_name_"),
                STD_TO_STRING(entry_file_name_.c_str()));
  }

  for (std::map<std::string, JXMethod>::iterator it =
           methods_to_initialize_.begin();
       it != methods_to_initialize_.end(); ++it) {
    methods_archive[it->first] = it->second;
  }

  InitializeProxyMethods(main_node_);
  methods_to_initialize_.clear();

  node::Load(process_l);

  if (argc > 0) free(argv_copy);
}

void JXEngine::Destroy() {
  ENGINE_LOG_THIS("JXEngine", "Destroy");
  JSContext *ctx = main_iso_.GetRaw();
  JSRuntime *rt = JS_GetRuntime(ctx);
  {
    customLock(CSLOCK_JOBS);
    if (jx_engine_instances.size() == 1)
      node::commons::process_status_ = node::JXCORE_INSTANCE_EXITING;
    else {
      // do not destroy the initial JXcore instance before others
      // JavaScript Runtimes are related to each other and the first JSRT
      // is the parent of all the others
      assert(threadId_ != 0 &&
             "First instance can not be destroyed before others");
      main_node_->instance_status_ = node::JXCORE_INSTANCE_EXITING;
    }
    customUnlock(CSLOCK_JOBS);

    AutoScope _scope_(this, false);

    JS_HANDLE_OBJECT process_l = main_node_->getProcess();

    node::EmitExit(process_l);

    // wait for the internal task threads
    // an app can have either task threads or embedded threads
    // as a result, any thread with id bigger than 0 can't have
    // task threads.
    if (main_node_->threadId == 0 && getThreadCount() > 0)
      WaitThreadExit(main_node_->loop);

    node::RunAtExit();

    // clean task thread related data
    if (main_node_->threadId == 0) {
      node::MemoryWrap::MapClear(true);
      Job::removeTaskers();
    }

    JS_DEFINE_STATE_MARKER(main_node_);

    global_->RemoveRoot();

    // clean persistent stuff
    main_node_->Dispose();

    JS_LeaveCompartment(ctx, jscomp_);
    JS_EndRequest(ctx);

    delete global_;

    // do not destroy last context or runtime.
    // we use main runtime as a parent for all the others.
    customLock(CSLOCK_JOBS);
    if (jx_engine_instances.size() == 1)
      node::commons::process_status_ = node::JXCORE_INSTANCE_EXITED;
    else {
      main_node_->instance_status_ = node::JXCORE_INSTANCE_EXITED;
      JS_DestroyContext(ctx);
    }
    customUnlock(CSLOCK_JOBS);

    // SM can't do GC under GC. we need the destroy other contexts separately
    std::list<JSContext *>::iterator itc =
        main_node_->free_context_list_.begin();

    while (itc != main_node_->free_context_list_.end()) {
      JSContext *ctx_sub = *itc;
      JS_DestroyContext(*itc);
      itc++;
    }
    main_node_->free_context_list_.clear();

    node::removeCommons();
  }

  if (node::commons::process_status_ != node::JXCORE_INSTANCE_EXITED)
    JS_DestroyRuntime(rt);

  main_iso_.Dispose();

  customLock(CSLOCK_JOBS);
  jx_engine_map::iterator it = jx_engine_instances.find(main_node_->threadId);
  if (it != jx_engine_instances.end()) jx_engine_instances.erase(it);
  customUnlock(CSLOCK_JOBS);
}
#endif

#if defined(JS_ENGINE_V8)
void JXEngine::InitializeEmbeddedEngine(int argc, char **argv) {
  ENGINE_LOG_THIS("JXEngine", "InitializeEmbeddedEngine");
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

  const int actual_thread_id = threadId_;

  if (actual_thread_id == 0) {
    main_node_ = new node::commons(0);
  } else {
    main_node_ = node::commons::newInstance(actual_thread_id);
  }

  AutoScope _scope_(this, true);

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
    JS_ENGINE_INITIALIZE();
  }

  node::commons *com = NULL;
  if (actual_thread_id == 0) {
    main_node_->node_isolate = NULL;
  } else {
    com = main_node_;
  }

  {
    JS_ENGINE_LOCKER();
    if (actual_thread_id != 0) {
      main_node_->node_isolate->Enter();
    }

    JS_NEW_CONTEXT(context_, isolate, NULL);
#ifndef V8_IS_3_14
    pContext_.Reset(isolate, context_);
#else
    pContext_ = context_;
#endif

    v8::Context::Scope context_scope(context_);
    v8::V8::SetFatalErrorHandler(node::OnFatalError);

    if (actual_thread_id == 0) {
      main_node_->node_isolate = isolate;
      main_node_->setMainIsolate();
    }

    node::SetupProcessObject(actual_thread_id);
    JS_HANDLE_OBJECT process_l = main_node_->getProcess();
#ifdef V8_IS_3_14
    v8_typed_array::AttachBindings(context_->Global());
#endif

    JS_DEFINE_STATE_MARKER(main_node_);
    if (entry_file_name_.length() != 0) {
      JS_NAME_SET(process_l, JS_STRING_ID("entry_file_name_"),
                  STD_TO_STRING(entry_file_name_.c_str()));
    }

    for (std::map<std::string, JXMethod>::iterator it =
             methods_to_initialize_.begin();
         it != methods_to_initialize_.end(); ++it) {
      methods_archive[it->first] = it->second;
    }
    InitializeProxyMethods(main_node_);
    methods_to_initialize_.clear();

    node::Load(process_l);

    if (argc > 0) free(argv_copy);
  }
}

void JXEngine::Destroy() {
  ENGINE_LOG_THIS("JXEngine", "Destroy");
  AutoScope _scope_(this, true);
  {
    customLock(CSLOCK_JOBS);
    if (jx_engine_instances.size() == 1)
      node::commons::process_status_ = node::JXCORE_INSTANCE_EXITING;
    else
      main_node_->instance_status_ = node::JXCORE_INSTANCE_EXITING;
    customUnlock(CSLOCK_JOBS);

    JS_ENGINE_SCOPE(main_node_, threadId_ != 0);

    JS_HANDLE_OBJECT process_l = main_node_->getProcess();
    node::EmitExit(process_l);

    if (main_node_->threadId == 0 && getThreadCount() > 0)
      WaitThreadExit(main_node_->loop);

    node::RunAtExit();

    if (main_node_->threadId == 0) {
      node::MemoryWrap::MapClear(true);
      Job::removeTaskers();
    }

    main_node_->Dispose();
  }

  customLock(CSLOCK_JOBS);
  if (jx_engine_instances.size() == 1)
    node::commons::process_status_ = node::JXCORE_INSTANCE_EXITED;
  else
    main_node_->instance_status_ = node::JXCORE_INSTANCE_EXITED;
  customUnlock(CSLOCK_JOBS);

#if defined HAVE_DTRACE || defined HAVE_ETW || defined HAVE_SYSTEMTAP
  if (main_node_->threadId == 0) node::cleanUpDTrace();
#endif

#ifndef NDEBUG
  context_.Dispose();
#endif

  customLock(CSLOCK_JOBS);
  jx_engine_map::iterator it = jx_engine_instances.find(main_node_->threadId);
  if (it != jx_engine_instances.end()) jx_engine_instances.erase(it);
  customUnlock(CSLOCK_JOBS);

  node::removeCommons();
}
#endif

char *JX_Stringify(node::commons *com, JS_HANDLE_OBJECT obj,
                   size_t *data_length) {
  ENGINE_LOG_THIS("JXEngine", "JX_Stringify");
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_STRING str_value;
  if (!JS_IS_STRING(obj)) {

    JS_HANDLE_VALUE args[] = {obj};

    // Init
    if (JS_IS_EMPTY((com->JSONstringify))) {
      JS_LOCAL_FUNCTION _JSONstringify = JS_CAST_FUNCTION(
          JS_COMPILE_AND_RUN(STD_TO_STRING(
                                 "(function(obj) {\n"
                                 "  try {\n"
                                 "    if(typeof obj === 'function') {\n"
                                 "      var b={};\n"
                                 "      for (var o in obj) {\n"
                                 "        if (!obj.hasOwnProperty(o))\n"
                                 "          continue;\n"
                                 "        b[o] = obj[o];\n"
                                 "      }\n"
                                 "      obj = b;\n"
                                 "    }\n"
                                 "    return JSON.stringify(obj);\n"
                                 "  } catch (e) {\n"
                                 "    return 'undefined';\n"
                                 "  }\n"
                                 "});"),
                             STD_TO_STRING("binding:stringify")));
      JS_NEW_PERSISTENT_FUNCTION(com->JSONstringify, _JSONstringify);
    }

    JS_LOCAL_FUNCTION fncl = JS_TYPE_TO_LOCAL_FUNCTION(com->JSONstringify);
    JS_LOCAL_VALUE result = JS_METHOD_CALL(fncl, JS_GET_GLOBAL(), 1, args);
    str_value = JS_VALUE_TO_STRING(result);
  } else {
    str_value = JS_VALUE_TO_STRING(obj);
  }

  JXString str(str_value);
  str.DisableAutoGC();
  *data_length = str.Utf8Length();
  return *str;
}

JS_HANDLE_VALUE JX_Parse(node::commons *com, const char *str,
                         const size_t length) {
  ENGINE_LOG_THIS("JXEngine", "JX_Parse");
  JS_ENTER_SCOPE_WITH(com->node_isolate);
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_STRING str_value;

  if (length != 0) {
    str_value = UTF8_TO_STRING_WITH_LENGTH(str, length);
  } else {
    str_value = UTF8_TO_STRING(str);
  }
  // Init
  if (JS_IS_EMPTY((com->JSONparse))) {
    JS_LOCAL_FUNCTION _JSONparse =
        JS_CAST_FUNCTION(JS_COMPILE_AND_RUN(STD_TO_STRING(
                                                "(function(str) {\n"
                                                "  try {\n"
                                                "    return JSON.parse(str);\n"
                                                "  } catch (e) {\n"
                                                "    return e;\n"
                                                "  }\n"
                                                "});"),
                                            STD_TO_STRING("binding:parse")));
    JS_NEW_PERSISTENT_FUNCTION(com->JSONparse, _JSONparse);
  }

  JS_LOCAL_VALUE args[1] = {str_value};

  JS_LOCAL_FUNCTION fncl = JS_TYPE_TO_LOCAL_FUNCTION(com->JSONparse);
  JS_LOCAL_VALUE result = JS_METHOD_CALL(fncl, JS_GET_GLOBAL(), 1, args);

  return JS_LEAVE_SCOPE(result);
}

#define SET_UNDEFINED(to_)   \
  to_->type_ = RT_Undefined; \
  to_->data_ = NULL;         \
  to_->size_ = 0

#define MANAGE_EXCEPTION                              \
  JS_LOCAL_VALUE exception = try_catch.Exception();   \
  result->type_ = RT_Error;                           \
  result->data_ = new JXValueWrapper(com, exception); \
  result->size_ = 1;                                  \
  return true;

bool JXEngine::ConvertToJXResult(node::commons *com,
                                 JS_HANDLE_VALUE_REF ret_val,
                                 JXResult *result) {
  ENGINE_LOG_THIS("JXEngine", "ConvertToJXResult");
  assert(result->com_ && "JXResult object wasn't initialized");
  result->persistent_ = false;
  result->was_stored_ = false;

  JS_DEFINE_STATE_MARKER(com);

  if (JS_IS_NULL(ret_val)) {
    result->type_ = RT_Null;
    return true;
  }

  if (JS_IS_UNDEFINED(ret_val)) {
    result->type_ = RT_Undefined;
    return true;
  }

  result->type_ = RT_Undefined;

  if (JS_IS_BOOLEAN(ret_val)) {
    result->type_ = RT_Boolean;
    result->size_ = sizeof(bool);
  }

  if (JS_IS_NUMBER(ret_val)) {
    if (JS_IS_INT32(ret_val)) {
      result->type_ = RT_Int32;
      result->size_ = sizeof(int32_t);
    } else {
      result->type_ = RT_Double;
      result->size_ = sizeof(double);
    }
  }

  if (node::Buffer::jxHasInstance(ret_val, com)) {
    result->type_ = RT_Buffer;
    result->size_ = node::Buffer::Length(ret_val);
  }

  if (JS_IS_STRING(ret_val)) {
    result->type_ = RT_String;
    JS_LOCAL_STRING str = JS_VALUE_TO_STRING(ret_val);
    result->size_ = JS_GET_STRING_LENGTH(str);
  }

  if (result->type_ != RT_Undefined) {
    JXValueWrapper *pr_wrap = new JXValueWrapper(com, ret_val);
    result->data_ = (void *)pr_wrap;
    return true;
  }

  if (JS_IS_FUNCTION(ret_val)) {
    JS_LOCAL_VALUE ud_val = JS_UNDEFINED();
    JS_LOCAL_FUNCTION fnc = JS_CAST_FUNCTION(JS_VALUE_TO_OBJECT(ret_val));
    JXFunctionWrapper *fnc_wrap = new JXFunctionWrapper(com, fnc, ud_val);
    result->data_ = fnc_wrap;
    result->size_ = sizeof(JXFunctionWrapper);
    result->type_ = RT_Function;

    return true;
  }

  if (!JS_IS_OBJECT(ret_val)) {
    result->type_ = RT_Error;
    JS_LOCAL_STRING dummy = STD_TO_STRING("Unsupported return type.");
    result->size_ = JS_GET_STRING_LENGTH(dummy);
    result->data_ = (void *)new JXValueWrapper(com, dummy);

    return false;
  }

  result->type_ = RT_Object;
  JXValueWrapper *wrap = new JXValueWrapper(com, ret_val);
  result->data_ = (void *)wrap;
  result->size_ = 1;

  return true;
}

bool Evaluate_(const char *source, const char *filename, JXResult *result,
               node::commons *com) {
  JS_DEFINE_STATE_MARKER(com);

  SET_UNDEFINED(result);
  result->com_ = com;
  result->was_stored_ = false;

  JS_LOCAL_STRING source_ = UTF8_TO_STRING(source);
  JS_LOCAL_STRING filename_ = STD_TO_STRING(filename);

  JS_TRY_CATCH(try_catch);

  JS_LOCAL_SCRIPT script = JS_SCRIPT_COMPILE(source_, filename_);

  if (JS_IS_EMPTY(script)) {
    MANAGE_EXCEPTION
  }

  JS_LOCAL_VALUE scr_return = JS_SCRIPT_RUN(script);

  if (try_catch.HasCaught()) {
    MANAGE_EXCEPTION
  }

  return JXEngine::ConvertToJXResult(com, scr_return, result);
}

bool JXEngine::Evaluate(const char *source, const char *filename,
                        JXResult *result) {
  ENGINE_LOG_THIS("JXEngine", "Evaluate");
  bool ret_val = false;
  if (!this->IsInScope()) {
    AutoScope _scope_(this, true);
    JS_ENGINE_SCOPE(main_node_, threadId_ != 0);

    ret_val = Evaluate_(source, filename, result, main_node_);
  } else {
    ret_val = Evaluate_(source, filename, result, main_node_);
  }

  return ret_val;
}

bool JXEngine::DefineProxyMethod(const char *name, const int interface_id,
                                 JS_NATIVE_METHOD method) {
  ENGINE_LOG_THIS("JXEngine", "DefineProxyMethod");
  bool ret_val = true;
  if (main_node_ == NULL) {
    methods_to_initialize_[name].native_method_ = method;
    methods_to_initialize_[name].is_native_method_ = false;
    methods_to_initialize_[name].interface_id_ = interface_id;
  } else {
    AutoScope _scope_(this, true);
    JS_ENGINE_SCOPE(main_node_, threadId_ != 0);

    JS_HANDLE_OBJECT process = main_node_->getProcess();
    JS_LOCAL_OBJECT natives =
        JS_VALUE_TO_OBJECT(JS_GET_NAME(process, JS_STRING_ID("natives")));
    if (JS_IS_EMPTY(natives) || JS_IS_NULL_OR_UNDEFINED(natives)) {
      error_console(
          "JXEngine::DefineProxyMethod, could not find 'natives' object "
          "under "
          "process\n");
      ret_val = false;
    } else {
      DeclareProxy(main_node_, natives, name, interface_id, method);
    }
  }
exit_:
  return ret_val;
}

bool JXEngine::DefineNativeMethod(const char *name, JS_NATIVE_METHOD method) {
  ENGINE_LOG_THIS("JXEngine", "DefineNativeMethod");
  bool ret_val = true;
  if (main_node_ == NULL) {
    methods_to_initialize_[name].native_method_ = method;
    methods_to_initialize_[name].is_native_method_ = true;
  } else {
    AutoScope _scope_(this, true);
    JS_ENGINE_SCOPE(main_node_, threadId_ != 0);

    JS_HANDLE_OBJECT process = main_node_->getProcess();
    JS_LOCAL_OBJECT natives =
        JS_VALUE_TO_OBJECT(JS_GET_NAME(process, JS_STRING_ID("natives")));
    if (JS_IS_EMPTY(natives) || JS_IS_NULL_OR_UNDEFINED(natives)) {
      error_console(
          "JXEngine::DefineNativeMethod, could not find 'natives' object "
          "under "
          "process\n");
      ret_val = false;
    } else {
      NODE_SET_METHOD(natives, name, method);
    }
  }

  return ret_val;
}

int JXEngine::Loop() {
  ENGINE_LOG_THIS("JXEngine", "Loop");
  int ret_val = 0;
  {
    AutoScope _scope_(this, true);
    JS_ENGINE_SCOPE(main_node_, threadId_ != 0);

    ret_val = uv_run_jx(main_node_->loop, UV_RUN_DEFAULT,
                        node::commons::CleanPinger, main_node_->threadId);
  }

  return ret_val;
}

int JXEngine::LoopOnce() {
  ENGINE_LOG_THIS("JXEngine", "LoopOnce");
  if (this->IsInScope()) return 1;

  int ret_val = 0;
  {
    AutoScope _scope_(this, true);
    JS_ENGINE_SCOPE(main_node_, threadId_ != 0);

    ret_val = uv_run_jx(main_node_->loop, UV_RUN_NOWAIT,
                        node::commons::CleanPinger, main_node_->threadId);
  }

  return ret_val;
}

void JXEngine::ShutDown() {
  ENGINE_LOG_THIS("JXEngine", "ShutDown");
  node::commons::process_status_ = node::JXCORE_INSTANCE_EXITED;
  jx_destroy_locks();
  node::commons::threadPoolCount = 0;
#if defined(JS_ENGINE_V8) && !defined(NDEBUG)
  // Clean up. Not strictly necessary.
  V8::Dispose();
#endif
  jxcore_was_shutdown_ = true;
}

JXEngine *JXEngine::ActiveInstance() {
  ENGINE_LOG_THIS("JXEngine", "ActiveInstance");
  if (jx_engine_instances.empty()) return NULL;

  const int actual_thread_id = node::commons::getCurrentThreadId();
  if (actual_thread_id < 0) return NULL;
  jx_engine_map::iterator it = jx_engine_instances.find(actual_thread_id);
  if (it == jx_engine_instances.end()) return NULL;

  return it->second;
}

JXEngine *JXEngine::GetInstanceByThreadId(const int threadId) {
  ENGINE_LOG_THIS("JXEngine", "GetInstanceByThreadId");
  if (jx_engine_instances.empty()) return NULL;

  jx_engine_map::iterator it = jx_engine_instances.find(threadId);
  if (it == jx_engine_instances.end()) return NULL;

  return it->second;
}

}  // namespace jxcore
