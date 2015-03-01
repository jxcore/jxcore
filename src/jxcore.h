// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JXCORE_H_
#define SRC_JXCORE_H_
#include "jx/commons.h"

namespace jxcore {
typedef enum JXResultType {
  RT_Int32 = 1,
  RT_Double = 2,
  RT_Boolean = 3,
  RT_NullUndefined = 4,
  RT_String = 5,
  RT_JSON = 6
};

class JXEngine;

class JXResult {
  friend class JXEngine;

  void *data_;
  JXResultType type_;
  bool empty_;

 protected:
#ifdef JS_ENGINE_MOZJS
  JSContext *__contextORisolate;
#endif

 public:
  JXResult();

  bool IsEmpty() const { return empty_; }
  bool IsInt32() const { return type_ == RT_Int32; }
  bool IsDouble() const { return type_ == RT_Double; }
  bool IsBoolean() const { return type_ == RT_Boolean; }
  bool IsNullOrUndefined() const { return type_ == RT_NullUndefined; }
  bool IsString() const { return type_ == RT_String; }
  bool IsJSON() const { return type_ == RT_JSON; }

#define __CHECKRET__(x, tp)                  \
  if (empty_ || type_ == RT_NullUndefined) { \
    return x;                                \
  }
#define __RETURN_PRIMITIVE__(tp) return *((tp *)data_)
#define __RETURN_STRING__() return (const char *)data_
#define __RET_PRI__(x, tp) \
  __CHECKRET__(x, tp); \
  __RETURN_PRIMITIVE__(tp)
#define __RET_STR__(x)       \
  __CHECKRET__(x, char); \
  __RETURN_STRING__()

  const int32_t GetInt32() { __RET_PRI__(0, int32_t); }

  const double GetDouble() { __RET_PRI__(0, double); }

  const bool GetBoolean() { __RET_PRI__(false, bool); }

  const char *GetString() { __RET_STR__(NULL); }

  const char *GetJSON() { __RET_STR__(NULL); }

#undef __CHECKRET__
#undef __RETURN_PRIMITIVE__
#undef __RETURN_STRING__
#undef __RET_PRI__
#undef __RET_STR__

  ~JXResult();
};

class JXEngine {
  bool self_hosted_;
  node::commons *main_node_;
  static bool jxcore_was_shutdown_;
  static bool JS_engine_inited_;
  bool instance_inited_;
  std::string entry_file_name_;

  std::map<std::string, JS_NATIVE_METHOD> methods_to_initialize_;
  JS_PERSISTENT_OBJECT native_methods_;

#if defined(JS_ENGINE_MOZJS)
  JS_PERSISTENT_OBJECT global_;
  JSCompartment *jscomp_;
  ENGINE_NS::Isolate main_iso_;
#elif defined(JS_ENGINE_V8)
  JS_PERSISTENT_CONTEXT context_;
#endif

  void InitializeEngine(int argc, char **argv);
  void InitializeEmbeddedEngine(int argc, char **argv);
  void ParseDebugOpt(const char *arg);
  void PrintHelp();
  void ParseArgs(int argc, char **argv);
  char **Init(int argc, char *argv[], bool engine_inited_already);

 public:
  int argc_;
  char **argv_;

  JXEngine(int argc, char *argv[], bool self_hosted = true);

  // initializes the locks, memory maps etc.
  void Init();

  // Literally starts the JavaScript engine and executes the entry file
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
   *to true
   */
  void MemoryMap(const char *filename, const char *content,
                 bool entry_file = false);

  // Evaluates JS
  bool Evaluate(const char *script, const char *filename, JXResult *result);

  // Defines a native method under process.natives
  void DefineNativeMethod(const char *name, JS_NATIVE_METHOD method);

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
};
}  // namespace jxcore

#endif  // SRC_JXCORE_H_
