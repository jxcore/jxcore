// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

JXValue *fnc;
JXValue *param;

void sampleMethod(JXValue *results, int argc) {
  assert(JX_IsFunction(results + 1) && JX_IsString(results) &&
         "Function parameters do not match");
  fnc = results + 1;
  param = results;
  JX_MakePersistent(fnc);
  JX_MakePersistent(param);
}

const char *contents =
    "function fnc(x, z){return (x + z);}\n"
    "process.natives.sampleMethod('Hello ', fnc);";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JXValue value;
  JX_New(&value);
  JX_SetString(&value, "World", 5);
  JXValue params[2] = {*param, value};
  JXValue out;
  JX_CallFunction(fnc, params, 2, &out);

  JX_Free(&value);

  JX_ClearPersistent(fnc);
  JX_Free(fnc);
  JX_ClearPersistent(param);
  JX_Free(param);

  std::string str;
  ConvertResult(&out, str);
  assert(strcmp("Hello World", str.c_str()) == 0 && "Result doesn't match.");
  JX_Free(&out);

  JX_StopEngine();
}
