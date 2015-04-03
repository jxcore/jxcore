// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}


void sampleMethod(JXValue *params, int argc) {
  assert(argc == 1 && "previous call did not return correct value");
  assert(JX_IsString(params+0) && "This method expects a string parameter");

  std::string data;
  ConvertResult(params+0, data);

  char *play = strdup(data.c_str());
  int32_t ln = JX_GetDataLength(params+0);

  for (int32_t n = 0; n < ln; n++) {
    (*(play+n)) ^= 1;
  }


  JX_SetBuffer(params+argc, play, ln);
  free(play);
}

const char *contents =
    "var strTest = 'Hello World!';\n"
    "var buffer = process.natives.sampleMethod(strTest);\n"
    "buffer = process.natives.sampleMethod(buffer+'');\n"
    "if(buffer+'' !== strTest) \n"
    "  process.natives.sampleMethod('', 0);"; // intentionally crash!

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}