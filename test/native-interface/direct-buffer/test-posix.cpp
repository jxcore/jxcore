// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}


void sampleMethod(JXValue *params, int argc) {
  assert(argc == 1 && "previous call did not return correct value");
  assert(JX_IsBuffer(params+0) && "This method expects a buffer string parameter");

  JXValue *buffer = &params[0];
  assert(JX_GetDataLength(buffer) == 10000);
  unsigned char *data = (unsigned char *)JX_GetBuffer(buffer);
  for (int i = 0 ; i < 10000 ; ++i) {
    assert(data[i] == (i % 256));
  }
  //assert(false);
}

const char *contents =
    "var buffer = new Buffer(10000);\n"
    "for (var i = 0 ; i < buffer.length ; i++) { buffer[i] = i; }\n"
    "process.natives.sampleMethod(buffer);\n";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}