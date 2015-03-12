// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXResult *results, int argc) {
  // do nothing
}

void sampleMethod(JXResult *results, int argc) {
  std::stringstream ss_result;
  for (int i = 0; i < argc; i++) {
    std::string str_result;
    ConvertResult(&results[i], str_result);
    ss_result << i << " : ";
    ss_result << str_result << "\n";
  }

  flush_console("%s", ss_result.str().c_str());

  // return an Array back to JS Land
  const char *str = "[1, 2, 3]";

  // results[argc] corresponds to return value
  JX_SetJSON(&results[argc], str, strlen(str));
}

const char *contents =
    "console.log('hello world from:', process.threadId); \n"
    "global.webview = {}; \n"
    "setTimeout(function(){},10);\n"
    "webview.call = function() { \n"
    "  return process.natives.sampleMethod(arguments[0], arguments[1]);\n"
    "};\n";

const char *eval_str =
    "webview.call([\"concat\",\"A\",\"B\",{\"jxcore_webview_callbackId\":1}]);";

void *create_jxcore_instance(void *_) {
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  JXResult result;
  JX_Evaluate(eval_str, "myscript", &result);

  if(!JX_ResultIsJSON(&result)) {
	flush_console("RETURN TYPE WAS: %d\n", result.type_);
  }
  assert(JX_ResultIsJSON(&result) && "expected result here is a JSON (JS array)");

  JX_FreeResultData(&result);

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
  return 0;
}

#define NUM_THREADS 20

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  pthread_t thread[NUM_THREADS];

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  for (int i = 0; i < NUM_THREADS; i++) {
    assert(pthread_create(&thread[i], NULL, create_jxcore_instance, 0) == 0);
  }

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  JXResult result;
  JX_Evaluate(eval_str, "myscript", &result);

  JX_FreeResultData(&result);

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(thread[i], NULL);
  }

  JX_StopEngine();
}
