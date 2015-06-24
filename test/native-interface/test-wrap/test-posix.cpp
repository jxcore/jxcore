// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

char *xx = NULL;
void sampleMethod(JXValue *results, int argc) {
  assert(argc>0 && "Test failed");
  
  if (argc == 2) {
    xx = (char*) malloc (2);
    memcpy(xx, "x\0", 2);
    JX_WrapObject(results+0, xx);
  } else {
    JXValue str;
    JX_New(&str);
    
    void *ptr = JX_UnwrapObject(results+0);
    char *dummy = (char*) ptr;
    assert(ptr && "This shouldn't be NULL");
    
    JX_SetString(&str, dummy, strlen(dummy));
    
    *(results+argc) = str;
  }
  fflush(stdout);
}

const char *contents =
    "var testObject = {};\n"
    "process.natives.sampleMethod(testObject, true);\n"
    "if (process.natives.sampleMethod(testObject) !== 'x')\n"
    "  process.natives.sampleMethod();";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
  
  if (xx) free(xx);
}