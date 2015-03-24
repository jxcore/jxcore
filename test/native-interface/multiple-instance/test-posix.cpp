// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

// native JavaScript method we'll be calling from JS land
void sampleMethod(JXValue *results, int argc) {
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
  // create a new JXcore instance
  JX_InitializeNewEngine();

  // define entry file
  JX_DefineMainFile(contents);
  
  // define native -named- method
  // we will be reaching to this method from the javascript side like this;
  // process.natives.sampleMethod( ... )
  JX_DefineExtension("sampleMethod", sampleMethod);
  
  // start the engine (executes entry file)
  JX_StartEngine();

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  JXValue result;
  // evaluate a piece of JavaScript code
  JX_Evaluate(eval_str, "myscript", &result);

  // see if the result is a JavaScript object
  if(!JX_IsJSON(&result)) {
	flush_console("RETURN TYPE WAS: %d\n", result.type_);
  }
  assert(JX_IsJSON(&result) && "expected result here is a JSON (JS array)");

  // free the memory
  JX_Free(&result);

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  // destroy the instance
  JX_StopEngine();
  return 0;
}

#define NUM_THREADS 20

int main(int argc, char **args) {
  // Call JX_Initialize only once per app	
  JX_Initialize(args[0], callback);
  
  // Creates a new engine for the current thread
  // It's our first engine instance hence it will be the
  // parent engine for all the other engine instances.
  // If you need to destroy this engine instance, you should
  // destroy everything else first. For the sake of this sample
  // we have our first instance sitting on the main thread
  // and it will be destroyed when the app exists.
  JX_InitializeNewEngine();

  pthread_t thread[NUM_THREADS];

  // define the entry file contents
  JX_DefineMainFile(contents);
  
  // define native -named- method
  // we will be reaching to this method from the javascript side like this;
  // process.natives.sampleMethod( ... )
  JX_DefineExtension("sampleMethod", sampleMethod);
  
  // start the engine (executes entry file)
  JX_StartEngine();

  // create and run other instances 
  for (int i = 0; i < NUM_THREADS; i++) {
    assert(pthread_create(&thread[i], NULL, create_jxcore_instance, 0) == 0);
  }

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  JXValue result;
  // evaluate a piece of JavaScript code
  JX_Evaluate(eval_str, "myscript", &result);

  // free the memory
  JX_Free(&result);

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(thread[i], NULL);
  }

  // finally destroy the first instance
  JX_StopEngine();
}
