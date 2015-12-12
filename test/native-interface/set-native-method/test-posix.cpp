// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

// native JavaScript method we'll be calling from JS land
void sampleMethod(JXValue *results, int argc) {
  assert(argc == 3 && "number of arguments supposed to be 3");

  assert(JX_IsFunction(&results[0]) && JX_IsFunction(&results[1]) &&
         "both parameters supposed to be a function");

  assert(JX_IsString(&results[2]) &&
         "third parameter supposed to be a string");

  JXValue out;
  assert(JX_CallFunction(&results[0], &results[2], 1, &out) &&
         "failed while calling console.log");
  assert(JX_IsUndefined(&out) &&
         "return value from console.log should be undefined");
}

const char *contents =
    "var func = console.log;\n"
    "func2 = MyObject.sampleMethod;\n"
    "MyObject.sampleMethod(func, func2, 'text');\n";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);

  JXValue obj;
  JX_CreateEmptyObject(&obj);

  JXValue global;
  JX_GetGlobalObject(&global);

  JX_SetNamedProperty(&global, "MyObject", &obj);

  JX_SetNativeMethod(&obj, "sampleMethod", sampleMethod);

  // Free'ing object here doesn't destroy it on JS land
  JX_Free(&obj);

  JX_StartEngine();

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}
