// Copyright & License details are available under JXCORE_LICENSE file

#include "jx.h"
#include "../jxcore.h"
#include <stdio.h>
#include <string.h>
#include "jx_wrapper_internal.h"

DEFINE_WRAPPER_VARIABLES();
static int extension_id = 0;

jxcore::JXEngine *engine = NULL;
JX_CALLBACK jx_callback;
char *argv = NULL;
char *app_args[2];

JS_LOCAL_METHOD(asyncCallback) {
  CONVERT_ARG_TO_RESULT(results, __contextORisolate);
  jx_callback(results, len);
  free(results);
}
JS_METHOD_END

DEFINE_WRAPPER_HOSTS();

void JX_DefineExtension(const char *name, JX_CALLBACK callback) {
  int id = extension_id++;
  assert ( (id < MAX_WRAPPERS_COUNT) && "You have reached beyond the maximum extension slots");
  extensions[id] = callback;
  engine->DefineNativeMethod(name, wrappers[id]);
}

void JX_Initialize(const char *home_folder, JX_CALLBACK callback) {
  static bool first_initialize_ = true;
  if (first_initialize_) {
    first_initialize_ = false;
    DEFINE_WRAPPERS();
  }
  jx_callback = callback;

#if defined(__IOS__) || defined(__ANDROID__) || defined(DEBUG)
  warn_console("Initializing JXcore engine\n");
#endif

  if (engine != NULL) {
    warn_console("Destroying the previous JXcore engine instance\n");
    engine->Destroy();
    free(argv);
    delete engine;
  }

  size_t home_length = strlen(home_folder);
  argv = (char *)malloc((12 + home_length) * sizeof(char));
  memcpy(argv, home_folder, home_length * sizeof(char));
  memcpy(argv + home_length, "/jx\0main.js", 11 * sizeof(char));
  argv[home_length + 11] = '\0';

  app_args[0] = argv;
  app_args[1] = argv + home_length + 4;

  engine = new jxcore::JXEngine(2, app_args, false);
  engine->Init();

#if defined(__IOS__) || defined(__ANDROID__) || defined(DEBUG)
  warn_console("JXcore engine is ready\n");
#endif
}

bool JX_Evaluate(const char *data, const char *script_name,
                 JXResult *jxresult) {
  char *str;
  if (engine == NULL) {
    error_console("JXcore engine is not ready yet! (jx.cc:Evaluate)\n");
    str = strdup("undefined");
    return str;
  }

  const char *name = script_name == NULL ? "JX_Evaluate" : script_name;

  return engine->Evaluate(data, name, jxresult);
}

void JX_DefineMainFile(const char *data) {
  if (engine == NULL) {
    error_console("JXcore engine is not ready yet! (jx.cc:DefineMainFile)\n");
    return;
  }

  engine->MemoryMap("main.js", data, true);
}

void JX_DefineFile(const char *name, const char *file) {
  if (engine == NULL) {
    error_console("JXcore engine is not ready yet! (jx.cc:DefineFile)\n");
    return;
  }

  engine->MemoryMap(name, file);
}

void JX_StartEngine() {
  if (engine == NULL) {
    error_console("JXcore engine is not ready yet! (jx.cc:StartEngine)\n");
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
  if (engine == NULL) {
    error_console("JXcore engine is not ready yet! (jx.cc:LoopOnce)\n");
    return 0;
  }
  return engine->LoopOnce();
}

int JX_Loop() {
  if (engine == NULL) {
    error_console("JXcore engine is not ready yet! (jx.cc:Loop)\n");
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
  if (engine == NULL) {
    error_console("JXcore engine is not ready yet! (jx.cc:StopEngine)\n");
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
