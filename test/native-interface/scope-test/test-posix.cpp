// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

void sampleMethod(JXValue *params, int argc) {
  // returns JS return value(5) back to the caller
  JX_Evaluate("var z=4;z+1", "test_scope.js", params+argc);
}

void crashMe(JXValue *_, int argc) {
  assert(0 && "previous call to sampleMethod must be failed");
}

const char *contents = "if (process.natives.sampleMethod() != 5)\n"
                       "  process.natives.crashMe();\n";

int main(int argc, char **args) {
  JX_Initialize("/test/bin/cpp/", callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("crashMe", crashMe);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}
