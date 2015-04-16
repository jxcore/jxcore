// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

const char *contents =
    "process.x = 123;";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  
  JX_StartEngine();

  JXValue ret_val;
  
  // this is not a function, it shouldn't return. 
  // exception is expected
  JX_Evaluate("return process.x", "eval", &ret_val);
  
  while (JX_LoopOnce() != 0) usleep(1);
  
  assert(JX_IsError(&ret_val) && "error was expected");
  
  JX_Free(&ret_val);

  JX_StopEngine();
  
  return 0;
}