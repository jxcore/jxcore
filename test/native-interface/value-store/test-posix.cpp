// Copyright & License details are available under JXCORE_LICENSE file
#include "../commons/common-posix.h"

void callback(JXValue *results, int argc) {
  // do nothing
}

long stored_ids[3] = {-1, -1, -1};
void sampleMethod(JXValue *params, int argc) {
  assert(JX_IsString(params+0) && "first parameter must be a string");

  stored_ids[0] = JX_StoreValue(params+0);

  JXValue obj;
  JX_CreateEmptyObject(&obj);
  stored_ids[1] = JX_StoreValue(&obj);

  JXValue arr;
  JX_CreateArrayObject(&arr);
  stored_ids[2] = JX_StoreValue(&arr);
}

void crashMe(JXValue *_, int argc) {
  assert(0 && "previous call to sampleMethod must be failed");
}

const char *contents = "process.natives.sampleMethod('hello world');";

int main(int argc, char **args) {
  JX_Initialize(args[0], callback);
  JX_InitializeNewEngine();

  JX_DefineMainFile(contents);
  JX_DefineExtension("crashMe", crashMe);
  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  while (JX_LoopOnce() != 0) usleep(1);

  int tid = JX_GetThreadId();
  assert(JX_GetStoredValueType(tid, stored_ids[0]) == RT_String && "first stored value must be a string");
  assert(JX_GetStoredValueType(tid, stored_ids[1]) == RT_Object && "second stored value must be an object");
  assert(JX_GetStoredValueType(tid, stored_ids[2]) == RT_Object && "third stored value must be an array object");

  JXValue *str = JX_RemoveStoredValue(tid, stored_ids[0]);
  assert (str->was_stored_ && "what happened to was_stored_ ?");
  assert (JX_GetDataLength(str) == strlen("hello world") && "stored string data doesn't match");
  JX_Free(str);

  // we delete str memory, and if the testing system is crazy busy
  // below hack-test may fail anyways
  assert (str->size_ == 0 && "JX_Free leaks for stored data");

  JXValue *obj = JX_RemoveStoredValue(tid, stored_ids[1]);
  assert (JX_GetDataLength(obj) == 1 && "stored object data doesn't match");
  JX_Free(obj);
  assert (obj->size_ == 0 && "JX_Free leaks for stored data");

  obj = JX_RemoveStoredValue(tid, stored_ids[2]);
  assert (JX_GetDataLength(obj) == 1 && "stored array data doesn't match");
  JX_Free(obj);
  assert (obj->size_ == 0 && "JX_Free leaks for stored data");

  JX_StopEngine();
}