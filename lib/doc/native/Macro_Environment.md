# JS_ENGINE_MARKER
Base type for JavaScript Engine thread. It corresponds to either native Isolate for V8 engine, or wrapper Isolate  for SpiderMonkey.

SM -> ```MozJS:Isolate```
V8 -> ```v8::Isolate```
***

# JS_CURRENT_ENGINE
Both SpiderMonkey, and V8 support multiple Javascript execution threads. ```JS_CURRENT_ENGINE()``` helps retrieving the JS engine Isolate(```JS_ENGINE_MARKER```) for the actual thread. 

Sample Usage
```c++
JS_ENGINE_MARKER iso = JS_CURRENT_ENGINE();
```
***

# JS_GET_GLOBAL
Returns the ```global``` object for the actual Javascript instance. 

Sample Usage
```c++
JS_LOCAL_OBJECT global = JS_GET_GLOBAL();
JS_NAME_SET(global, JS_STRING_ID("total"), STD_TO_INTEGER(100));
```
```js
console.log(total);
```
***

# JS_CURRENT_CONTEXT
A JavaScript execution thread may have multiple sub contexts. ```JS_CURRENT_CONTEXT()``` returns the actual JavaScript context. 

Sample Usage
```c++
JS_HANDLE_CONTEXT context = JS_CURRENT_CONTEXT();
```
***

# JS_FORCE_GC
Forces JavaScript engine garbage collection. Keep in mind, this operation may pause the actual execution thread until the GC process is completed. 

Sample Usage
```c++
JS_FORCE_GC();
```
***

# JS_TERMINATE_EXECUTION
Terminates the JavaScript engine execution on given thread. It is recommended to restart the target thread whenever the execution is terminated. 

Sample Usage
```c++
// terminate the execution on main thread
JS_TERMINATE_EXECUTION(0); 

// terminate the execution on sub thread 1
JS_TERMINATE_EXECUTION(1); 
```
P.S. You may want to stop the execution on libuv also ```uv_stop(JS_GET_UV_LOOP(1))```
***

# JS_GET_UV_LOOP
JXcore uses LibUV for IO tasks. Since JX supports multiple instances under the same process, you may need to reach the actual LibUV loop variable for the thread.

Sample Usage
```c++
uv_loop_t actual_loop = JS_GET_UV_LOOP(1);
```
In order to get the threadId for the actual JXcore thread, you may either;
```c++
// inside a native JS_METHOD
JS_METHOD(.....) {
  // there is a predefined com variable
  int threadId = com->threadId;
}
JS_METHOD_END

// inside a normal method
void MyMethod(...) {
  node::commons *com = node::commons::getInstance("");
  int threadId = com->threadId;
}
```
***

# JS_GET_STATE_MARKER
In case you need to access directly to the underlying JavaScript engine instead of using the macros / wrappers. You need the state marker to interact with the core engine features. 

State marker corresponds to ```v8::Isolate*`` for V8 engine, while it's a ```JSContext*``` for SpiderMonkey. 

This macro is available by default under ```JS_METHOD``` or ```JS_LOCAL_METHOD``` . In order to make it available to external methods, you should define the state marker by ```JS_DEFINE_STATE_MARKER``` 

Sample Usage
```c++
JS_LOCAL_METHOD(MyMethod) {
#if defined(JS_ENGINE_MOZJS)
  JSRuntime *rt = JS_GetRuntime( JS_GET_STATE_MARKER() );
#elif defined(JS_ENGINE_V8)
  v8::Isolate *current = JS_GET_STATE_MARKER();
#endif
...
...
}
JS_METHOD_END
```
***

# JS_DEFINE_STATE_MARKER
Each JavaScript execution thread has it's own state definition variable. All methods and functionality provided by the engine wrapper require that state marker is set. By default the state marker is set for ```JS_METHOD``` and ```JS_LOCAL_METHOD```

Sample Usage
```c++
JS_HANDLE_OBJECT MyCustomMethod() {
  JS_ENTER_SCOPE();

  // node::commons::getInstance("") returns the native
  // global JXcore object for the actual thread.
  JS_DEFINE_STATE_MARKER(node::commons::getInstance(""));
  return JS_LEAVE_SCOPE(STD_TO_INTEGER(100));
}
```
P.S. In case the state marker wasn't defined, and you use one of the JS engine methods, you may receive an error ```com is not defined```. For example the above sample, ```STD_TO_INTEGER``` requires the state marker is defined.

P.S.(2) You may also call ;
```JS_DEFINE_COM_AND_MARKER()``` instead ```JS_DEFINE_STATE_MARKER(node::commons::getInstance(""))```. It will simply define both ```node::commons *com``` and the state marker.
***

# JS_ENTER_SCOPE / JS_LEAVE_SCOPE
Whenever you need to return a native JavaScript variable from one of your custom methods, it is recommended to enter and leave the scope using the given macros.

Sample Usage
```c++
JS_HANDLE_OBJECT MyCustomMethod() {
  JS_ENTER_SCOPE();
  JS_DEFINE_COM_AND_MARKER();

  JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
  JS_LOCAL_STRING str = STD_TO_STRING("hello");
  JS_NAME_SET(obj, JS_STRING_ID("something"), str);
  return JS_LEAVE_SCOPE(obj);
}
```
P.S. These macros have no consistent behavior for the varying engines. The V8 version may use it (depending on the version) but SpiderMonkey doesn't. In order to make your custom implementation compatible with multiple engines, it's advised to use both ```JS_ENTER_SCOPE``` and ```JS_LEAVE_SCOPE``` under your custom helper methods.
***



