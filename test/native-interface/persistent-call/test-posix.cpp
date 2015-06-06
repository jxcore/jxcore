// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

const char compare_base[10] = {'1', '4', 't', 't', '{',
                               'A', 'u', 'n', 'E', 'n'};

JXValue fnc;
JXValue param1, param2;

// native sampleMethod
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

  // making persistent will protect below objects from garbage collection
  JX_MakePersistent(results+9);
  JX_MakePersistent(results+3);
  JX_MakePersistent(results+4);
  
  // copy the memory since as soon as this callback returns, results mem will be free'd
  fnc = results[9];
  param1 = *(results + 3);
  param2 = *(results + 4);

  // call JS side fnc with 2 parameters and get
  // the result to 'out'
  JX_CallFunction(results+9, (results + 3), 2, &out);

  assert(JX_GetDataLength(&out) == 11 &&
         "Expected return value was 'test{\"a\":3}");
  JX_Free(&out);
  assert(out.data_ == NULL && out.size_ == 0 && "JX_FreeResultData leaks?");
}

const char *contents =
	// define a JS side function fnc
    "function fnc(x, y){return (x + JSON.stringify(y));}\n"
    "var bf = new Buffer(5);bf.fill(65);\n"
	// deliver this function 'last' parameter to the native sampleMethod above
    "process.natives.sampleMethod(1, 4.5, true, 'test', {a:3}, bf, undefined, "
    "null, new Error('error!'), fnc);";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  JXValue *params[2] = {&param1, &param2};
  JXValue out;

  // call fnc -> JS side function
  // we had made this variable persistent inside sampleMethod
  JX_CallFunction(&fnc, *params, 2, &out);

  // we need to clear persistent and then free whenever we
  // are done with persistent values to not to leak
  JX_ClearPersistent(&fnc);
  JX_Free(&fnc);
  JX_ClearPersistent(&param1);
  JX_Free(&param1);
  JX_ClearPersistent(&param2);
  JX_Free(&param2);

  assert(JX_GetDataLength(&out) == 11 &&
         "Expected return value was 'test{\"a\":3}");
  JX_Free(&out);
  assert(out.data_ == NULL && out.size_ == 0 && "JX_FreeResultData leaks?");

  JX_StopEngine();
}
