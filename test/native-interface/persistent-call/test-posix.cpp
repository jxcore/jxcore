// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXResult *results, int argc) {
  // do nothing
}

const char compare_base[10] = {'1', '4', 't', 't', '{',
                               'A', 'u', 'n', '{', 'n'};

JXResult *fnc;
JXResult *param1, *param2;

void sampleMethod(JXResult *results, int argc) {
  for (int i = 0; i < argc; i++) {
    std::string str;
    ConvertResult(&results[i], str);
    if (compare_base[i] != str.c_str()[0]) {
      flush_console("FAIL! Item(%d) : %s \n", i, str.c_str());
      exit(-1);
    }
  }

  JXResult out;

  fnc = &results[9];
  param1 = results + 3;
  param2 = results + 4;

  JX_CallFunction(fnc, (results + 3), 2, &out);

  JX_MakePersistent(fnc);
  JX_MakePersistent(param1);
  JX_MakePersistent(param2);

  assert(JX_GetDataLength(&out) == 11 &&
         "Expected return value was 'test{\"a\":3}");
  JX_FreeResultData(&out);
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

  JXResult *params[2] = {param1, param2};
  JXResult out;
  JX_CallFunction(fnc, *params, 2, &out);

  JX_ClearPersistent(fnc);
  JX_FreeResultData(fnc);
  JX_ClearPersistent(param1);
  JX_FreeResultData(param1);
  JX_ClearPersistent(param2);
  JX_FreeResultData(param2);

  assert(JX_GetDataLength(&out) == 11 &&
         "Expected return value was 'test{\"a\":3}");
  JX_FreeResultData(&out);
  assert(out.data_ == NULL && out.size_ == 0 && "JX_FreeResultData leaks?");

  JX_StopEngine();
}
