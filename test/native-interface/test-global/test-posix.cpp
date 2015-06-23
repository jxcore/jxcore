// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

void sampleMethod(JXValue *results, int argc) {
  if (JX_GetBoolean(results + 0)) {
    JXValue value;
    JX_New(&value);
    JX_SetString(&value, "World", 5);

    JXValue global;
    JX_GetGlobalObject(&global);

    JXValue testObject;
    JX_GetNamedProperty(&global, "testObject", &testObject);

    JX_SetNamedProperty(&testObject, "sub", &value);

    JX_Free(&value);
  } else {
    assert(0 && "Something went wrong!");
  }
}

const char *contents =
    "testObject = {};\n"
    "process.natives.sampleMethod(true);\n"
    "if (!testObject.hasOwnProperty('sub'))\n"
    "  process.natives.sampleMethod(false);";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}