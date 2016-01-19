// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void crashMe(JXValue *_, int argc) {
  assert(0 && "previous call to sampleMethod must be failed");
}

const char *contents =
    "process.callMe = function callMe() {\n"
    " throw new TypeError('this is type exception');\n"
    "};";

int main(int argc, char **args) {
  JX_InitializeOnce("/test/bin/cpp/");
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_StartEngine();

  JXValue ret_val;
  JX_Evaluate("process.callMe()", "eval", &ret_val);

  while (JX_LoopOnce() != 0) usleep(1);

  assert(JX_IsError(&ret_val) && "error was expected");

  std::string str;
  ConvertResult(&ret_val, str);
  assert(!strcmp("TypeError: this is type exception", str.c_str()) &&
         "Exception output doesn't match");

  JX_Free(&ret_val);

  JX_StopEngine();
}
