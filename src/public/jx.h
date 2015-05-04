// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_PUBLIC_JX_H
#define SRC_PUBLIC_JX_H

#include "jx_result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*JX_CALLBACK)(JXValue *result, int argc);

// You should call below method only once per application to
// initialize JXcore
JXCORE_EXTERN(void)
JX_Initialize(const char *home_folder, JX_CALLBACK callback);

// Per each native thread, you should initialize a new JXcore engine
JXCORE_EXTERN(void)
JX_InitializeNewEngine();

// Evaluates a JavaScript code on the fly.
// Remarks:
// 1 - returns false if compilation fails or an internal issue happens (i.e. no memory)
// 2 - result is a return value from the running code. i.e. "var x=4; x+1" returns 5
// 3 - script_name represents the script's file name
// 4 - script_code expects a JavaScript code with null ending
JXCORE_EXTERN(bool)
JX_Evaluate(const char *script_code, const char *script_name, JXValue *result);

// Define the contents of main.js file (entry file for each JXcore engine)
JXCORE_EXTERN(void)
JX_DefineMainFile(const char *data);

// Define a JavaScript file with it's contents, so you can require it from JS land
// i.e.
// native code: JX_DefineFile("test.js", "exports.x=4");
// js code: require('test.js').x -> 4
JXCORE_EXTERN(void)
JX_DefineFile(const char *name, const char *script);

// Starts JXcore engine instance
JXCORE_EXTERN(void)
JX_StartEngine();

// define a native method that can be called from the JS land
// i.e.
// native code: JX_DefineExtension("testMethod", my_callback);
// js code: process.natives.testMethod(1, true);
JXCORE_EXTERN(void)
JX_DefineExtension(const char *name, JX_CALLBACK callback);

// loop io events for once. If there is any action left to do
// this method returns 1 otherwise 0
JXCORE_EXTERN(int)
JX_LoopOnce();

// loop io events until they are finished
JXCORE_EXTERN(int)
JX_Loop();

// returns true if the underlying engine is SpiderMonkey
JXCORE_EXTERN(bool)
JX_IsSpiderMonkey();

// returns true if the underlying engine is V8
JXCORE_EXTERN(bool)
JX_IsV8();

// returns threadId for the actual instance
// -1 : there is no active instance for the current thread
// 0 to 63 threadIds. (JS side: process.threadId + 1)
JXCORE_EXTERN(int)
JX_GetThreadId();

// Stops the actual JXcore engine (under the thread)
// Call this only for the sub engines. In other words,
// When you destroy the first engine instance, you can not
// create the additional instances.
JXCORE_EXTERN(void)
JX_StopEngine();

// store JXValue and return a corresponding identifier for a future reference
// This feature is especially designed for JNI like interfaces where carrying JXValue
// type around may not be the best option. You can simply deliver the id (long) and
// using other methods, you can get the contents async.
JXCORE_EXTERN(long)
JX_StoreValue(JXValue *value);

// return stored type information
// tip: threadId for the first thread is always 0
JXCORE_EXTERN(JXValueType)
JX_GetStoredValueType(const int threadId, const long id);

// get and remove stored value
// unless you remove the stored value, it will be consuming the memory
JXCORE_EXTERN(JXValue *)
JX_RemoveStoredValue(const int threadId, const long identifier);

#ifdef __cplusplus
}
#endif

#endif  // SRC_PUBLIC_JX_H
