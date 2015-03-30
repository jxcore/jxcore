// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

const char compare_base[10] = {'1', '4', 't', 't', '{',
                               'A', 'u', 'n', '{', 'n'};

void sampleMethod(JXValue *results, int argc) {
  for (int i = 0; i < argc; i++) {
    std::string str;
    ConvertResult(&results[i], str);
    if (compare_base[i] != str.c_str()[0]) {
      flush_console("FAIL! Item(%d) : %s \n", i, str.c_str());
      exit(-1);
    }
  }

  JXValue out;
  JX_CallFunction(&results[9], (results + 3), 2, &out);

  assert(JX_GetDataLength(&out) == 11 &&
         "Expected return value was 'test{\"a\":3}");
  JX_Free(&out);
  assert(out.data_ == NULL && out.size_ == 0 && "JX_FreeResultData leaks?");
}

const char *contents =
    "function fnc(x, y){return (x + JSON.stringify(y));}\n"
    "var bf = new Buffer(5);bf.fill(65);\n"
    "process.natives.sampleMethod(1, 4.5, true, 'test', {a:3}, bf, undefined, "
    "null, new Error('error!'), fnc);";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
  
  return 0;
}
