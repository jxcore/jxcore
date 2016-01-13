// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

void sampleMethod(JXValue *params, int argc) {
  JX_SetNull(params + argc);
}

const char *contents = "";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JXValue out;
  JX_Evaluate("process.natives.sampleMethod();", "", &out);
  assert(JX_IsNull(&out) && "expects null as return value");
  JX_Free(&out);

  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}
