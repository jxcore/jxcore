// Copyright & License details are available under JXCORE_LICENSE file

#include "jx.h"
#include "../jxcore.h"
#include <stdio.h>
#include <string.h>

jxcore::JXEngine *engine = NULL;
JX_CALLBACK jx_callback;
char *argv = NULL;
char *app_args[2];

JS_LOCAL_METHOD(asyncCallback) {
  const int len = args.Length();
  JXResult *results = (JXResult *)malloc(sizeof(JXResult) * len);
  for (int i = 0; i < len; i++) {
    JS_HANDLE_VALUE val = args.GetItem(i);
#ifdef JS_ENGINE_MOZJS
    results[i].context_ = __contextORisolate;
#endif
    results[i].data_ = NULL;
    results[i].size_ = 0;
    results[i].type_ = RT_Undefined;

    jxcore::JXEngine::ConvertToJXResult(com, val, &results[i]);
  }

  jx_callback(results, len);
  free(results);
}
JS_METHOD_END

void JX_Initialize(const char *home_folder, JX_CALLBACK callback) {
  jx_callback = callback;

  warn_console("Initializing JXcore engine\n");
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

  warn_console("JXcore process.argv ::\n%s %s\n", app_args[0], app_args[1]);

  engine = new jxcore::JXEngine(2, app_args, false);
  engine->Init();

  warn_console("JXcore engine is ready\n");
}

bool JX_Evaluate(const char *data, const char *script_name,
                 JXResult *jxresult) {
  char *str;
  if (engine == NULL) {
    error_console(
        "JXcore engine is not ready yet! (jxcoreIOS.cc:evalEngine)\n");
    str = strdup("undefined");
    return str;
  }

  const char *name = script_name == NULL ? "IOS_Evaluate" : script_name;

  return engine->Evaluate(data, name, jxresult);
}

void JX_DefineMainFile(const char *data) {
  if (engine == NULL) {
    error_console(
        "JXcore engine is not ready yet! (jxcoreIOS.cc:defineFile)\n");
    return;
  }

  engine->MemoryMap("main.js", data, true);
}

void JX_DefineFile(const char *name, const char *file) {
  if (engine == NULL) {
    error_console(
        "JXcore engine is not ready yet! (jxcoreIOS.cc:defineFile)\n");
    return;
  }

  engine->MemoryMap(name, file);
}

void JX_StartEngine() {
  if (engine == NULL) {
    error_console(
        "JXcore engine is not ready yet! (jxcoreIOS.cc:startEngine)\n");
    return;
  }

  engine->DefineNativeMethod("asyncCallback", asyncCallback);

  warn_console("Starting JXcore engine\n");
  engine->Start();
  int res = engine->LoopOnce();
  warn_console("JXcore engine is started, loop(%d)\n", res);
}

int JX_LoopOnce() {
  if (engine == NULL) {
    error_console("JXcore engine is not ready yet! (jxcoreIOS.cc:loopOnce)\n");
    return 0;
  }
  return engine->LoopOnce();
}

void JX_StopEngine() {
  if (engine == NULL) {
    error_console(
        "JXcore engine is not ready yet! (jxcoreIOS.cc:stopEngine)\n");
    return;
  }

  warn_console("Destroying JXcore engine\n");
  if (engine->LoopOnce()) {
    warn_console("JXcore engine event loop was still handling other events\n");
  }

  engine->Destroy();
  delete engine;
  engine = NULL;
  warn_console("JXcore engine is destroyed");
}
