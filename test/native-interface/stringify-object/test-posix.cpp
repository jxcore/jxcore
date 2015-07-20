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

  char *str = JX_GetString(&obj);
  assert(strcmp(str, "{\"i1\":{\"a\":1,\"b\":\"2\",\"c\":false,\"d\":33.4},\"i2\":[1,2,3]}") == 0);
  free(str); // free stringify char* memory
  
  JX_Free(&obj); // free JS Object
}
const char *contents =
    "process.natives.sampleMethod({a:1, b:'2', c:false, d:33.4, e:function(){} }, [1,2,3]);";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}