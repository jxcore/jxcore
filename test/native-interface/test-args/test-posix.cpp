// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

JXValue fnc;
void defineMethod(JXValue *params, int argc) {
  assert(JX_IsFunction(params+0));

  // see persistent-call test for explanation on MakePersistent
  JX_MakePersistent(params+0);
  fnc = *(params+0);
}

const char *contents = "var fn = function() {\n"
                       "  return arguments.length + ' parameters received';\n"
                       "};\n"
                       "process.natives.defineMethod(fn);\n";

int main(int argc, char **args) {
  JX_Initialize("/test/bin/cpp/", callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("defineMethod", defineMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JXValue params[3];

  for(int i = 0; i < 3; i++)
    JX_New(&params[i]);

  JX_SetInt32(&params[0], 100);
  JX_SetBoolean(&params[1], true);
  JX_SetString(&params[2], "Hello", 5);

  JXValue ret_val;
  JX_CallFunction(&fnc, params, 3, &ret_val);

  std::string str;
  ConvertResult(&ret_val, str);
  printf("Result: %s \n", str.c_str());

  for(int i=0; i<3; i++)
    JX_Free(&params[i]);

  JX_Free(&ret_val);

  JX_ClearPersistent(&fnc);
  JX_Free(&fnc);

  JX_StopEngine();
}