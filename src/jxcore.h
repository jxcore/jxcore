// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JXCORE_H_
#define SRC_JXCORE_H_
#include "jx/commons.h"
#include "public/jx_result.h"
#include "jxcore_type_wrap.h"

namespace jxcore {

class JXEngine {
  bool inside_scope_;
  bool self_hosted_;
  node::commons *main_node_;
  static bool jxcore_was_shutdown_;
  static bool JS_engine_inited_;
  std::string entry_file_name_;

  void InitializeEngine(int argc, char **argv);
  void InitializeEmbeddedEngine(int argc, char **argv);
  void ParseDebugOpt(const char *arg);
  void PrintHelp();
  void ParseArgs(int argc, char **argv);
  char **Init(int argc, char *argv[], bool engine_inited_already);

#if defined(JS_ENGINE_MOZJS)
  JS_PERSISTENT_OBJECT *global_;
  JSCompartment *jscomp_;
  ENGINE_NS::Isolate main_iso_;

 public:
  inline void ExitIsolate() {}
#elif defined(JS_ENGINE_V8)
 public:
  JS_PERSISTENT_CONTEXT pContext_;
  inline JS_HANDLE_CONTEXT getContext() {
#ifdef V8_IS_3_14
    return pContext_;
#else
    return node::PersistentToLocal(main_node_->node_isolate, pContext_);
#endif
  }

  inline void ExitIsolate() { main_node_->node_isolate->Exit(); }
#endif

  inline node::commons *getCommons() { return main_node_; }

  int argc_;
  char **argv_;
  int threadId_;

  inline void EnterScope() {
#ifdef JS_ENGINE_V8
    assert(!inside_scope_ && "JXEngine was already in a scope");
    inside_scope_ = true;
#endif
  }
  inline void LeaveScope() {
#ifdef JS_ENGINE_V8
    assert(inside_scope_ && "JXEngine was already outside of a scope");
    inside_scope_ = false;
#endif
  }
  inline bool IsInScope() {
#ifdef JS_ENGINE_V8
    return inside_scope_;
#else
    return false;
#endif
  }

  // internal
  static void InitializeProxyMethods(node::commons *com);

  JXEngine(int argc, char *argv[], bool self_hosted = true);

  // initializes the locks, memory maps etc. call it once per app.
  static void DefineGlobals();

  // initialize the engine - embedded or standalone
  void Initialize();

  // Literally starts the JavaScript execution of the entry file
  void Start();

  // Runs libuv loop (NO_WAIT)
  int LoopOnce();

  // Runs libuv loop (DEFAULT)
  int Loop();

  // Destroys the instance, clear up memory
  void Destroy();

  // !!! just before shutting down the application
  // once this called, embedder cannot create a new instance of JXcore
  static void ShutDown();

  /*
   * creates a bridge between the memory content and 'fs'
   * set 'content' to NULL in order to delete an existing match
   * use the same filename to update the content
   * file content is available all the threads.
   *
   * if you want to set entry file from the memory content, set last parameter
   * to true
   */
  void MemoryMap(const char *filename, const char *content, const size_t len,
                 bool entry_file = false);

  // Evaluates JS
  bool Evaluate(const char *script, const char *filename, JXResult *result);

  void DefineProxyMethod(JS_HANDLE_OBJECT obj, const char *name,
                         const int interface_id, JS_NATIVE_METHOD method);

  // returns the JXEngine instance for the actual thread
  static JXEngine *ActiveInstance();

  // returns a JXEngine instance for the given threadId
  // (remember, this object is not threadsafe!)
  static JXEngine *GetInstanceByThreadId(const int threadId);

  // returns threadId for this instance. returns -1 if an engine instance wasn't
  // initialized
  inline int GetThreadId() const {
    return (main_node_ != NULL) ? main_node_->threadId : -1;
  }

  ~JXEngine() { ENGINE_PRINT_LOGS(); }

  static bool ConvertToJXValue(node::commons *com, JS_HANDLE_VALUE_REF ret_val,
                               JXValue *result);
};

char *JX_Stringify(node::commons *com, JS_HANDLE_OBJECT obj,
                   size_t *data_length);
JS_HANDLE_VALUE JX_Parse(node::commons *com, const char *str,
                         const size_t length);
}  // namespace jxcore

#endif  // SRC_JXCORE_H_
