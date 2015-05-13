## JS_NAME_SET
Defines a named member into a JavaScript object.

Sample Usage
```c++
JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
JS_NAME_SET(obj, JS_STRING_ID("name"), STD_TO_STRING("John Doe"));
```
***

## JS_GET_NAME
Brings a named property from a JavaScript object.

Sample Usage
```c++
JS_LOCAL_VALUE name = JS_GET_NAME(obj, JS_STRING_ID("name"));
```
***

## JS_NAME_DELETE
Deletes a property from given JavaScript object.

Sample Usage
```c++
bool is_deleted = JS_NAME_DELETE(obj, JS_STRING_ID("name"));
```
***

## JS_HAS_NAME
Checks whether a JavaScript object has a named property or not.

Sample Usage
```c++
if(JS_HAS_NAME(obj, JS_STRING_ID("name"))) {
  ..
  ..
}
```
***

#JS_STRING_ID
Represents a string based identifier for native JavaScript objects. When the performance is not the concern, ```STD_TO_STRING``` can also be used.

Usage Sample
```c++
JS_LOCAL_STRING prop_name = STD_TO_STRING("name");
JS_NAME_SET(obj, prop_name, prop_value);

// OR

JS_NAME_SET(obj, JS_STRING_ID("name"), prop_value);
```
***

## JS_SET_POINTER_DATA / JS_GET_POINTER_DATA
Sets / gets a reference to/from native variable's memory address. 

Sample Usage
```c++
void *some_data =  ...

JS_LOCAL_METHOD(MethodSet) {
  JS_HANDLE_OBJECT obj = args.GetItem(0);
  JS_SET_POINTER_DATA(obj, some_data);
}
JS_METHOD_END

JS_LOCAL_METHOD(MethodGet) {
  void *data = JS_GET_POINTER_DATA(obj);
}
JS_METHOD_END
```
***

## JS_SET_INDEXED_EXTERNAL
Makes a native memory reference reachable from JavaScript object's indexed access. 

Sample Usage
```c++
char str_data[100] = { 'A', ...... };

JS_LOCAL_METHOD(MyMethod) {
  JS_HANDLE_METHOD obj = args.GetItem(0);
  JS_SET_INDEXED_EXTERNAL(obj, str_data, ENGINE_NS::kExternalUnsignedByteArray, 100);
}
JS_METHOD_END 

// from JS:
// var obj = {};
// myMethod(obj);
// console.log(obj[0]); -> 65
```
***

## JS_GET_EXTERNAL_ARRAY_*
Gets the external array data pointer / type / length defined by ```JS_SET_INDEXED_EXTERNAL```

Sample Usage:
```c++
char str_data[100] = { 'A', ...... };

JS_LOCAL_METHOD(MyMethod) {
  JS_HANDLE_METHOD obj = args.GetItem(0);
  JS_SET_INDEXED_EXTERNAL(obj, str_data, ENGINE_NS::kExternalUnsignedByteArray, 100);

  char *str = (char*) JS_GET_EXTERNAL_ARRAY_DATA(obj);
  int type = JS_GET_EXTERNAL_ARRAY_DATA_TYPE(obj);
  size_t length = JS_GET_EXTERNAL_ARRAY_DATA_LENGTH(obj);
}
JS_METHOD_END 
```
***

## JS_NEW_INSTANCE
JavaScript ```new``` operator for native side.

Sample Usage:
```c++
// JS
// var fnc = function(x) {
//   this.value = x;
// };
// var obj = myMethod(fnc);
// console.log(obj.value); --> 100

JS_LOCAL_METHOD(MyMethod) {
  JS_HANDLE_OBJECT o = args.GetItem(0);
  JS_LOCAL_VALUE params[1] = {
                        STD_TO_INTEGER(100)
                      };
  JS_LOCAL_OBJECT new_object = JS_NEW_INSTANCE(o, 1, params);
  RETURN_PARAM(new_object);
}
JS_METHOD_END
```
Use ```JS_NEW_DEFAULT_INSTANCE``` if there is no parameter for the constructor. 
```c++
JS_LOCAL_OBJECT new_object = JS_NEW_DEFAULT_INSTANCE(o);
```
***

## JS_GET_STRING_LENGTH
Returns the length of the string for a native JavaScript object. Given parameter has to be a JavaScript string.

Sample Usage
```c++
JS_LOCAL_STRING str = STD_TO_STRING("Hello");
int ln = JS_GET_STRING_LENGTH(str); // 5
```
***

## JS_GET_ARRAY_LENGTH(a) (a)->Length()
Returns the array length of a native JavaScript array object. Given parameters is expected to be a JavaScript array.

Sample Usage
```c++
JS_LOCAL_ARRAY arr = JS_NEW_ARRAY_WITH_COUNT(3);
int ln = JS_GET_ARRAY_LENGTH(arr); // 3
```
***

## JS_TRY_CATCH
Defines a native JavaScript ```TryCatch``` interface.

Sample Usage:
```c++
JS_TRY_CATCH(tc);

JS_METHOD_CALL( .... );

if(tc.HasCaught()) {
  if (tc.CanContinue()) node::ReportException(tc, true);
  abort();
}
```
***

## JS_HAS_INSTANCE
Check whether a native JavaScript object has an instance of other.

Sample Usage
```c++
JS_HANDLE_OBJECT obj = args.GetItem(0);
JS_LOCAL_OBJECT new_object = JS_NEW_DEFAULT_INSTANCE(obj);
bool has_instance = JS_HAS_INSTANCE(obj, new_object); // true
```
***

## JS_METHOD_CALL
Calls a JavaScript function with parameters.

Sample Usage
```c++
JS_LOCAL_VALUE params[1] = {
                        STD_TO_STRING("{\"name\": \"John\", \"surname\": \"doe\"}")
                      };

JS_LOCAL_OBJECT global = JS_GET_GLOBAL();
JS_LOCAL_OBJECT JSON = JS_GET_NAME(global, JS_STRING_ID("JSON"));
JS_LOCAL_FUNCTION fnc = JS_CAST_FUNCTION( JS_GET_NAME(JSON, JS_STRING_ID("parse")) );

JS_LOCAL_VALUE result = JS_METHOD_CALL(fnc, JSON, 1, params);
```
If there is no parameter for the function call, you should prefer using ```JS_METHOD_CALL_NO_PARAM```

Sample Usage
```c++
JS_LOCAL_VALUE result = JS_METHOD_CALL_NO_PARAM(fnc, JSON);
```
***

## JS_COMPILE_AND_RUN
Compiles and runs a given script string.

Sample Usage
```c++
JS_LOCAL_FUNCTION objectMethod = JS_CAST_FUNCTION(
  JS_COMPILE_AND_RUN(STD_TO_STRING(
    "(function(type) {\n"
      "if(type == 'Error')\n"
        "return new Error();\n"
      "else if(type == 'TypeError')\n"
        "return new TypeError();\n"
      "else if(type == 'RangeError')\n"
        "return new RangeError();\n"
   "})"), STD_TO_STRING("native:jxcore_js_object")));

  JS_LOCAL_VALUE glob = JS_GET_GLOBAL();
  JS_LOCAL_STRING tp = STD_TO_STRING('TypeError');
  
  JS_LOCAL_VALUE result = JS_METHOD_CALL(objectMethod, glob, 1, &tp);
```
***
