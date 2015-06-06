// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

void defineMethod(JXValue *params, int argc) {
  assert(JX_IsObject(params+0));

  const char *name = "Mr. Black";
  JXValue userName;
  JX_New(&userName);
  JX_SetString(&userName, name, strlen(name));

  JX_SetNamedProperty(params+0, "name", &userName);
  JX_Free(&userName);


}

const char *contents = "process.testObj = {};\n"
                       "process.natives.defineMethod(process.testObj);\n"
                       "if (!process.testObj.hasOwnProperty('name') || process.testObj.name !== 'Mr. Black') {\n"
                       "  console.error('name property wasnt defined! or value is not matching. Going to crash..');"
                       "  process.natives.defineMethod(null); \n" // crash!
                       "}\n";

int main(int argc, char **args) {
  JX_Initialize("/test/bin/cpp/", callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("defineMethod", defineMethod);
  JX_StartEngine();

  JXValue userDefaults;
  JX_Evaluate("process.testObj[1] = 3;process.testObj", "eval", &userDefaults);

  JXValue ret;
  JX_GetIndexedProperty(&userDefaults, 1, &ret);

  assert(JX_IsInt32(&ret) && "This supposed to be a number");

  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}