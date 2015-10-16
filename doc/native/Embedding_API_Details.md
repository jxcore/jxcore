### JXcore Embedding API Details

#### Types

Supported types are listed below;
```c
enum JXType {
  RT_Int32 = 1,
  RT_Double = 2,
  RT_Boolean = 3,
  RT_String = 4,
  RT_Object = 5,
  RT_Buffer = 6,
  RT_Undefined = 7,
  RT_Null = 8,
  RT_Error = 9,
  RT_Function = 10
};
```

`JXValue` struct represents the common wrapper for all the supported types. Unless you know what you are doing, you shouldn't
reach the members of JXValue struct directly.

Example native String;
```c
JXValue nativeString;
JX_New(&nativeString); // => var nativeString;
JX_SetString(&nativeString, "Hello", 5); // => nativeString = "Hello";

// .
// Use the variable
// .

// Let's check if JXValue is String
assert( JX_IsString(&nativeString) ); // => typeof nativeString === "string"

// Let's release it
JX_Free(&nativeString); // You should call JX_Free on every variable you've called JX_New
```

Related methods; (Each of the methods given below also have their corresponding JX_Is.... checkers)
 - **Int32**    : void JX_SetInt32(JXValue *value, const int32_t val); 
 - **Double**   : void JX_SetDouble(JXValue *value, const double val);  
 - **Boolean**  : void JX_SetBoolean(JXValue *value, const bool val);  
 - **String**   : void JX_SetString(JXValue *value, const char *val, const int32_t length);  
 - **UCString** : void JX_SetUCString(JXValue *value, const uint16_t *val, const int32_t length);  
 - **JSON**     : void JX_SetJSON(JXValue *value, const char *val, const int32_t length);
 !! JXcore calls JSON.parse on given JSON string and delivers the resulting object
 - **Error**    : void JX_SetError(JXValue *value, const char *val, const int32_t length);
 - **Buffer**   : void JX_SetBuffer(JXValue *value, const char *val, const int32_t length);
 !! JXcore wraps char array with Node.JS Buffer and delivers the resulting object
 - **Undefined** : void JX_SetUndefined(JXValue *value);
 - **Null** : void JX_SetNull(JXValue *value);
 - **Object** : JX_SetObject(JXValue *host, JXValue *val);
 !! Object has a specical condition. See below;
 
#### Objects / Arrays

JXcore native interface (jx-ni) provides additional methods to create a Javascript Object or Array on the native side. `JX_New` 
mentioned above is not needed to create an empty object or an array. 

To create an empty object and rest;
```
JXValue js_object;
if (!JX_CreateEmptyObject(&js_object)) { // => var js_object = {};
  // something bad happened .. i.e. no memory
}

// create a number variable
JXValue number;
JX_New(&number); // => var number;
JX_SetInt32(&number, 6); // => number = 6;

// Set property
JX_SetNamedProperty(&js_object, "x", &number); // => js_object.x = 6

// Now we are done with number variable, lets free it's memory
JX_Free(&number);

// Get property 
JXValue propValue;
JX_GetNamedProperty(&js_object, "x", &propValue); // => propValue = js_object[x]

// check if the object has 'x'
if (JX_IsNullOrUndefined(&propValue)) {
 // Seems the object doesn't have 'x'
}

// check if the propValue is an int32
if (JX_IsInt32(&propValue)) {
  // Yes it is
}

//  Don't forget to free propValue and js_object memories
JX_Free(&propValue);
JX_Free(&js_object);
```

Array has a similar interface. 

 - **Create an empty array**   : bool JX_CreateArrayObject(JXValue *value)
 - **Set an indexed property** : void JX_SetIndexedProperty(JXValue *object, const unsigned index, JXValue *prop)
 - **Get an indexed property** : void JX_GetIndexedProperty(JXValue *object, const int index, JXValue *out)

#### Other Methods

##### bool JX_Evaluate(const char *script_code, const char *script_name, JXValue *result)
Evaluates a JavaScript code on the fly.

**Remarks**
 - returns false if compilation fails or an internal issue happens (i.e. no memory)
 - result is a return value from the running code. i.e. "var x=4; x+1" returns 5
 - script_name represents the script's file name
 - script_code expects a JavaScript code with null ending

##### void JX_DefineMainFile(const char *data)
Define the contents of main.js file. JXcore embedded assumes the entry file is named as 'main.js'

##### void JX_DefineFile(const char *name, const char *script)
Define a JavaScript file with it's contents, so you can require it from JS land
```
native code: JX_DefineFile("test.js", "exports.x=4");
js code: require('test.js').x -> 4
```

##### void JX_StartEngine();
Starts a JXcore engine instance. You may have multiple JXcore engine instances under a single application. However, you need 
to make sure;
 - each instance should have it's own thread. They can't share threads
 - interact with an instance only under it's thread. Remember, JS is single threaded
 - do not destroy the initial JXcore engine instance. i.e. SpiderMonkey sub runtimes are based on initial runtime.
 
##### void JX_DefineExtension(const char *name, JX_CALLBACK callback)
Define a native method that can be called from the JS land

```
native code: 
void my_callback(JXValue *args, int argc) {
   // a remark here is that; You should set return value to (args+argc)
   JX_SetNull(args+argc); // => return null; 
   // by default, the return value is undefined
}

JX_DefineExtension("testMethod", my_callback);

js code: 
process.natives.testMethod(1, true);
```

##### int JX_LoopOnce()
Loop io events for once. If there is any action left to do this method returns 1 otherwise 0

##### int JX_Loop()
Loop io events until there is nothing left to do.

##### bool JX_IsSpiderMonkey()
Returns true if the underlying engine is SpiderMonkey

##### bool JX_IsV8()
Returns true if the underlying engine is V8

##### int JX_GetThreadId()
Returns threadId for the actual instance
 - -1 : there is no active instance for the current thread
 - 0 to 63 threadIds. (JS side: process.threadId + 1)

##### void JX_StopEngine()
Stops the actual JXcore engine (under the thread). Call this only for the sub engines. In other words, when you destroy 
the first engine instance, you can not create the additional instances.

##### long JX_StoreValue(JXValue *value)
Store JXValue and return a corresponding identifier for a future reference. This feature is especially designed for 
JNI like interfaces where carrying JXValue type around may not be the best option. You can simply deliver the id (long) and
using other methods, you can get the contents async.

##### JXValueType JX_GetStoredValueType(const int threadId, const long id)
Return stored type information

 - threadId for the first thread is always 0

##### JXValue JX_RemoveStoredValue(const int threadId, const long identifier)
Get and remove stored value. Unless you remove the stored value, it will be consuming the memory

##### int JX_GetThreadIdByValue(JXValue *value)
If you have a JXValue around, this method brings threadId much faster

##### void JX_GetGlobalObject(JXValue *out)
Get global object from actual thread's main context

##### void JX_GetProcessObject(JXValue *out)
Get node's process object from actual thread's main context

##### void JX_WrapObject(JXValue *object, void *ptr)
Wrap a pointer into a JS object

##### void JX_UnwrapObject(JXValue *object)
Unwrap the pointer from a JS object (doesn't remove it)

##### void JX_GetBuffer(JXValue *object)
Returns a direct pointer to the underlying data for a Buffer. No copying involved.
Don't hold to it longer than necessary, it may be garbage collected.

##### void JX_QuitLoop()
Calls `uv_stop` on thread's uv_loop. Use `LoopOnce` if you need to control the thread.
