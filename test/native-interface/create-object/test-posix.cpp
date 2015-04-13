// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}


void sampleMethod(JXValue *params, int argc) {
  JXValue obj;
  JX_CreateEmptyObject(&obj);
  JX_SetNamedProperty(&obj, "i1", params+0);
  JX_SetNamedProperty(&obj, "i2", params+1);

  // return
  JX_SetObject(params + argc, &obj);
  JX_Free(&obj);
}

void crashMe(JXValue *_, int argc) {
  assert(0 && "previous call to sampleMethod must be failed");
}

const char *contents =
    "var arr = process.natives.sampleMethod(1,2);"
    "if (!arr || arr.i1 !== 1 || arr.i2 !== 2) "
    "  process.natives.crashMe()";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("crashMe", crashMe);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}