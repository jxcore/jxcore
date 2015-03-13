// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXResult *results, int argc) {
  // do nothing
}

// native JavaScript method we'll be calling from JS land
void sampleMethod(JXResult *results, int argc) {
  assert(argc == 4 && "number of arguments supposed to be 4");

  assert(JX_ResultIsFunction(&results[0]) && JX_ResultIsFunction(&results[1]) &&
         "both parameters supposed to be a function");

  assert(JX_ResultIsString(&results[2]) && JX_ResultIsString(&results[3]) &&
         "both parameters supposed to be a function");

  bool JX_CallFunction(JXResult * fnc, JXResult * params, const int argc,
                       JXResult * out);

  JXResult out;
  assert(JX_CallFunction(&results[0], &results[2], 1, &out) &&
         "failed while calling console.log");
  assert(JX_ResultIsUndefined(&out) &&
         "return value from console.log should be undefined");

  assert(JX_CallFunction(&results[0], &results[3], 1, &out) &&
         "failed while calling console.error");
  assert(JX_ResultIsUndefined(&out) &&
         "return value from console.error should be undefined");
}

const char *contents =
    "console.log('hello world from:', process.threadId); \n"
    "global.webview = {}; \n"
    "setTimeout(function(){},10);\n"
    "webview.call = function() { \n"
    "  process.natives.sampleMethod(arguments[0], arguments[1], 'normal', "
    "'error');\n"
    "};\n";

const char *eval_str = "webview.call(console.log, console.error);";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  JXResult result;
  JX_Evaluate(eval_str, "myscript", &result);

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}
