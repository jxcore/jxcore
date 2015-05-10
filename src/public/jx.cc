// Copyright & License details are available under JXCORE_LICENSE file

#include "../jx/extend.h"
#include "../jxcore.h"
#include "../jx/job.h"
#include <stdio.h>
#include <string.h>
#include "jx.h"

using namespace jxcore;
JX_CALLBACK jx_callback;
char *argv = NULL;
char *app_args[2];

// allocates one extra JXResult memory at the end of the array
// Uses that one for a return value
#define CONVERT_ARG_TO_RESULT(results, context)                   \
  JXValue *results = NULL;                                        \
  const int len = args.Length() - start_arg;                      \
  {                                                               \
    results = (JXValue *)malloc(sizeof(JXValue) * (len + 1));     \
    for (int i = 0; i < len; i++) {                               \
      JS_HANDLE_VALUE val = args.GetItem(i + start_arg);          \
      results[i].com_ = context;                                  \
      results[i].data_ = NULL;                                    \
      results[i].size_ = 0;                                       \
      results[i].type_ = RT_Undefined;                            \
      results[i].was_stored_ = false;                             \
      jxcore::JXEngine::ConvertToJXResult(com, val, &results[i]); \
    }                                                             \
    results[len].com_ = context;                                  \
    results[len].data_ = NULL;                                    \
    results[len].size_ = 0;                                       \
    results[len].type_ = RT_Undefined;                            \
    results[len].was_stored_ = false;                             \
  }

JS_LOCAL_METHOD(asyncCallback) {
  const int start_arg = 0;
  CONVERT_ARG_TO_RESULT(results, com);
  jx_callback(results, len);

  for (int i = 0; i < len; i++) {
    JX_Free(&results[i]);
  }

  free(results);
}
JS_METHOD_END

static int extension_id = 0;
#define MAX_CALLBACK_ID 1024
static JX_CALLBACK callbacks[MAX_CALLBACK_ID] = {0};

JS_LOCAL_METHOD(extensionCallback) {
  if (args.Length() == 0 || !args.IsUnsigned(0)) {
    THROW_EXCEPTION("This method expects the first parameter is unsigned");
  }

  const int interface_id = args.GetInteger(0);
  if (interface_id >= MAX_CALLBACK_ID || callbacks[interface_id] == NULL)
    THROW_EXCEPTION("There is no extension method for given Id");

  const int start_arg = 1;
  CONVERT_ARG_TO_RESULT(results, com);

  callbacks[interface_id](results, len);

  for (int i = 0; i < len; i++) {
    JX_Free(&results[i]);
  }

  if (results[len].type_ != RT_Undefined) {
    assert(results[len].data_ != NULL && (results[len].size_ != 0 ||
		   results[len].type_ == RT_String) &&
           "Return value was corrupted");

    if (results[len].type_ == RT_Error) {
      std::string msg = JX_GetString(&results[len]);
      JX_Free(&results[len]);
      THROW_EXCEPTION(msg.c_str());
    } else if (results[len].type_ == RT_Function) {
      assert(sizeof(JXFunctionWrapper) == results[len].size_ &&
             "Type mixing? This can not be a Function");

      JXFunctionWrapper *fnc_wrap = (JXFunctionWrapper *)results[len].data_;
      JS_HANDLE_FUNCTION fnc = fnc_wrap->GetFunction();
      JX_Free(&results[len]);

      RETURN_PARAM(fnc);
    }

    JXValueWrapper *wrap = (JXValueWrapper *)results[len].data_;
    JS_HANDLE_VALUE ret_val = wrap->value_;
    JX_Free(&results[len]);
    RETURN_PARAM(ret_val);
  }
}
JS_METHOD_END

void JX_DefineExtension(const char *name, JX_CALLBACK callback) {
  auto_lock locker_(CSLOCK_RUNTIME);
  int id = extension_id++;
  assert(id < MAX_CALLBACK_ID &&
         "Maximum amount of extension methods reached.");
  callbacks[id] = callback;
  JXEngine *engine = JXEngine::ActiveInstance();
  if (engine == NULL) {
    warn_console(
        "(JX_DefineExtension) Did you initialize the JXEngine instance for "
        "this thread?\n");
    return;
  }
  engine->DefineProxyMethod(name, id, extensionCallback);
}

void JX_InitializeNewEngine() {
  auto_lock locker_(CSLOCK_RUNTIME);
  JXEngine *engine = JXEngine::ActiveInstance();
  const int threadId = node::commons::threadIdFromThreadPrivate();
  if (engine != NULL && threadId == engine->threadId_) {
    warn_console(
        "(JX_InitializeNewEngine) Did you forget destroying the existing "
        "JXEngine instance for this "
        "thread?\n");
    return;
  }
  engine = new jxcore::JXEngine(2, app_args, false);
}

int JX_GetThreadId() {
  JXEngine *engine = JXEngine::ActiveInstance();
  if (engine == NULL) return -1;

  return engine->threadId_;
}

void JX_Initialize(const char *home_folder, JX_CALLBACK callback) {
  jx_callback = callback;
  JXEngine::Init();
#if defined(__IOS__) || defined(__ANDROID__) || defined(DEBUG)
  warn_console("Initializing JXcore engine\n");
#endif

  size_t home_length = strlen(home_folder);
  argv = (char *)malloc((12 + home_length) * sizeof(char));
  memcpy(argv, home_folder, home_length * sizeof(char));
  if (home_length && home_folder[home_length-1] != '/' && home_folder[home_length-1] != '\\') {
    memcpy(argv + home_length, "/jx\0main.js", 11 * sizeof(char));
    argv[home_length + 11] = '\0';
    app_args[1] = argv + home_length + 4;
  } else {
	memcpy(argv + home_length, "jx\0main.js", 10 * sizeof(char));
	argv[home_length + 10] = '\0';
	app_args[1] = argv + home_length + 3;
  }

  app_args[0] = argv;

#if defined(__IOS__) || defined(__ANDROID__) || defined(DEBUG)
  warn_console("JXcore engine is ready\n");
#endif
}

bool JX_Evaluate(const char *data, const char *script_name, JXValue *jxresult) {
  JXEngine *engine = JXEngine::ActiveInstance();
  if (engine == NULL) {
    warn_console(
        "(JX_Evaluate) Did you start the JXEngine instance for this thread?\n");
    return false;
  }

  const char *name = script_name == NULL ? "JX_Evaluate" : script_name;

  return engine->Evaluate(data, name, jxresult);
}

void JX_DefineMainFile(const char *data) {
  auto_lock locker_(CSLOCK_RUNTIME);
  JXEngine *engine = JXEngine::ActiveInstance();
  if (engine == NULL) {
    warn_console(
        "(JX_DefineMainFile) Did you initialize the JXEngine instance for this "
        "thread?\n");
    return;
  }

  engine->MemoryMap("main.js", data, true);
}

void JX_DefineFile(const char *name, const char *file) {
  auto_lock locker_(CSLOCK_RUNTIME);
  JXEngine *engine = JXEngine::ActiveInstance();
  if (engine == NULL) {
    warn_console(
        "(JX_DefineFile) Did you initialize the JXEngine instance for this "
        "thread?\n");
    return;
  }

  engine->MemoryMap(name, file);
}

void JX_StartEngine() {
  auto_lock locker_(CSLOCK_RUNTIME);

  JXEngine *engine = JXEngine::ActiveInstance();
  if (engine == NULL) {
    warn_console(
        "(JX_StartEngine) Did you initialize the JXEngine instance for this "
        "thread?\n");
    return;
  }

  engine->DefineNativeMethod("asyncCallback", asyncCallback);

#if defined(__IOS__) || defined(__ANDROID__) || defined(DEBUG)
  warn_console("Starting JXcore engine\n");
#endif

  engine->Start();
  engine->LoopOnce();
#if defined(__IOS__) || defined(__ANDROID__) || defined(DEBUG)
  warn_console("JXcore engine is started\n");
#endif
}

int JX_LoopOnce() {
  JXEngine *engine = JXEngine::ActiveInstance();
  if (engine == NULL) {
    warn_console(
        "(JX_LoopOnce) Did you initialize the JXEngine instance for this "
        "thread? (ret_val: %d)\n",
        node::commons::getCurrentThreadId());
    return 0;
  }
  return engine->LoopOnce();
}

int JX_Loop() {
  JXEngine *engine = JXEngine::ActiveInstance();
  if (engine == NULL) {
    warn_console(
        "(JX_Loop) Did you initialize the JXEngine instance for this "
        "thread?\n");
    return 0;
  }
  return engine->Loop();
}

bool JX_IsV8() {
#ifdef JS_ENGINE_V8
  return true;
#else
  return false;
#endif
}

bool JX_IsSpiderMonkey() {
#ifdef JS_ENGINE_MOZJS
  return true;
#else
  return false;
#endif
}

void JX_StopEngine() {
  auto_lock locker_(CSLOCK_RUNTIME);

  JXEngine *engine = JXEngine::ActiveInstance();
  if (engine == NULL) {
    warn_console(
        "(JX_StopEngine) Did you initialize the JXEngine instance for this "
        "thread?\n");
    return;
  }

#if defined(__IOS__) || defined(__ANDROID__) || defined(DEBUG)
  warn_console("Destroying JXcore engine\n");
#endif

  if (engine->LoopOnce()) {
    warn_console("JXcore engine event loop was still handling other events\n");
  }

  engine->Destroy();

  delete engine;
  engine = NULL;
#if defined(__IOS__) || defined(__ANDROID__) || defined(DEBUG)
  warn_console("JXcore engine is destroyed");
#endif
}

// memory vs performance ?
// we have MAX number of slots allocated so we don't need locks
#define MAX_STORED_ID 1024 * 1024 * 1024
static std::map<long, JXValue *> stored_values[65];
static long stored_ids[65] = {0};
// store JXValue and return a corresponding identifier for a future reference
JXCORE_EXTERN(long)
JX_StoreValue(JXValue *value) {
  JXValue *tmp = new JXValue[1];

  JX_MakePersistent(value);
  *tmp = *value;
  tmp->was_stored_ = true;

  int threadId = JX_GetThreadIdByValue(value);
  long id = stored_ids[threadId]++;
  if (stored_values[threadId].find(id) != stored_values[threadId].end()) {
    // there is something here that we should destroy
    JX_RemoveStoredValue(threadId, id);
  }

  stored_values[threadId][id] = tmp;

  // embedders should consume a stored value before storing the
  // next billion of them
  id %= MAX_STORED_ID;

  return id;
}

// return stored type information
JXCORE_EXTERN(JXValueType)
JX_GetStoredValueType(const int threadId, const long id) {
  if (stored_values[threadId].find(id) != stored_values[threadId].end()) {
    return stored_values[threadId][id]->type_;
  } else {
    // id doesn't exist. better crash here
    assert(0 &&
           "You either use a stored identifier on a different thread or some "
           "another "
           "thing happened. A value for this id doesn't exist.");
  }

  // compiler likes this
  return RT_Undefined;
}

// get and remove stored value
JXCORE_EXTERN(JXValue *)
JX_RemoveStoredValue(const int threadId, const long id) {
  std::map<long, JXValue *>::iterator it = stored_values[threadId].find(id);
  if (it != stored_values[threadId].end()) {
    JXValue *val = it->second;
    stored_values[threadId].erase(it);
    JX_ClearPersistent(val);
    return val;
  } else {
    // id doesn't exist. better crash here
    assert(0 &&
           "You either use a stored identifier on a different thread or some "
           "another "
           "thing happened. A value for this id doesn't exist.");
  }

  // for compiler's sake
  return 0;
}
