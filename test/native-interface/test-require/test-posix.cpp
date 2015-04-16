// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

const char *contents =
    "process.requireGlobal = require;";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  
  JX_StartEngine();

  JXValue ret_val;
  JX_Evaluate("process.requireGlobal('./test.js').data", "eval", &ret_val);
  
  while (JX_LoopOnce() != 0) usleep(1);
  
  assert(JX_IsInt32(&ret_val) && JX_GetInt32(&ret_val) == 123 && "require failed");
  
  JX_Free(&ret_val);

  JX_StopEngine();
}