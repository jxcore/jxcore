# JS_UNDEFINED / JS_NULL
They exactly correspond to their JavaScript namesakes. 

Sample Usage
```c++
JS_LOCAL_VALUE null_value = JS_NULL();
JS_LOCAL_VALUE undefined_value = JS_UNDEFINED();

// return 'null'
RETURN_PARAM(JS_NULL());

// empty RETURN() call returns JS_UNDEFINED()
RETURN();
```
***

# JS_LOCAL_*
Below is a list of variable types supported on the native side. ```LOCAL``` means the local scope. When the function exits, GC is expected to collect the variables defined locally. It's advised to use ```JS_HANDLE_*``` whenever using a native JS variable as a function parameter. Finally, ```JS_PERSISTENT_*``` variables are the ones GC doesn't collect unless you set them free.

```JS_LOCAL_STRING``` : String  
```JS_LOCAL_INTEGER``` : Integer  
```JS_LOCAL_BOOLEAN``` : Boolean  
```JS_LOCAL_NUMBER``` : Double  
```JS_LOCAL_ARRAY``` : Array  
```JS_LOCAL_OBJECT``` : Object  
```JS_LOCAL_FUNCTION``` : Function  
```JS_LOCAL_VALUE``` : Value (can hold the all defined above)

```JS_LOCAL_SCRIPT``` : Script  
```JS_LOCAL_FUNCTION_TEMPLATE``` : Function Template  
```JS_LOCAL_OBJECT_TEMPLATE``` : Object Template  

P.S. It's advised to use ```VALUE``` instead ```BOOLEAN, INTEGER, or NUMBER``` individually. 

Sample Usage
```c++
JS_LOCAL_STRING str = STD_TO_STRING("Hi!");
JS_LOCAL_INTEGER nt = STD_TO_INTEGER(44);
JS_LOCAL_BOOLEAN bl = STD_TO_BOOLEAN(true);
JS_LOCAL_NUMBER nm = STD_TO_NUMBER(12.5);

// Array
JS_LOCAL_ARRAY arr = JS_NEW_EMPTY_ARRAY();
JS_INDEX_SET(arr, 0, str);

// Object
JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
JS_NAME_SET(obj, JS_STRING_ID("my_number"), nm);

// Function
JS_LOCAL_OBJECT global = JS_GET_GLOBAL();
JS_LOCAL_OBJECT JSON = JS_GET_NAME(global, JS_STRING_ID("JSON"));
JS_LOCAL_FUNCTION fnc = JS_CAST_FUNCTION( JS_GET_NAME(JSON, JS_STRING_ID("parse")) );
JS_LOCAL_STRING str_to_parse = STD_TO_STRING("{\"name\": \"John\", \"surname\": \"doe\"}");
JS_LOCAL_VALUE result = JS_METHOD_CALL(fnc, JSON, 1, &str_to_parse);
```
***

# JS_HANDLE_*
```JS_HANDLE``` variables are beneficial when you need to return a native JS variable from a function or use it as a function parameter. Do not use ```JS_LOCAL``` members for such cases. If you need to use a native JS variable type for one of your custom ```c++ class``` type, prefer using ```JS_PERSISTENT_*``` instead.

P.S. It's advised to use ```VALUE``` instead ```BOOLEAN, INTEGER, or NUMBER``` individually. 

Sample Usage
```c++
JS_HANDLE_VALUE MyFunction(int a, int b) {
  JS_ENTER_SCOPE()
  JS_LOCAL_VALUE total = STD_TO_INTEGER(a + b);
  
  return JS_LEAVE_SCOPE(total);
}

JS_LOCAL_METHOD(MyNativeMethod) {
  if(args.IsInt32(0) && args.IsInt32(1)) {
    JS_HANDLE_VALUE result = MyFunction( args.GetInt32(0), args.GetInt32(1) );
    RETURN_PARAM(result);
  }
  else
    THROW_TYPE_EXCEPTION("This method expects 2 integers");
}
JS_METHOD_END
```
P.S. You should wrap the ```HANDLE``` variable with its corresponding ```TYPE_TO_LOCAL_*``` converter in order to assign it to a ```LOCAL`` variable. Check ```JS_PERSISTENT_*``` sample for the usage.
***

# JS_PERSISTENT_*
Whenever you need to keep a native JS variable value outside of a native JS function, it is strongly recommended to use ```JS_PERSISTENT_``` wrapper. It will protect the variable from garbage collection. Please keep in mind that, freeing a ```PERSISTENT``` object is your responsibility.

P.S. It's advised to use ```VALUE``` instead ```BOOLEAN, INTEGER, or NUMBER``` individually. 

Sample Usage:
```c++
class MyClass {
 public:
  JS_PERSISTENT_STRING version_;
  MyClass(const char *version) {
    version_ = JS_NEW_PERSISTENT_STRING(version);
  }

  ~MyClass() {
    JS_CLEAR_PERSISTENT_STRING(version_);
  }
};

MyClass clss("1.0");

JS_LOCAL_METHOD(GetVersion) {
  JS_LOCAL_STRING version = TYPE_TO_LOCAL_STRING(clss.version_);
  RETURN_PARAM(version);
}
```
As shown above, you should wrap the ```PERSISTENT``` variable with its corresponding ```TYPE_TO_LOCAL_*``` converter in order to assign it to a ```LOCAL`` variable. When it's assigned, it transfers only the value of the persistent object, not the reference. As a result ```LOCAL``` variable will be garbage collected at the end of the scope (or when it's suitable for the underlying engine).
***

# STD_TO_*
Converts ```primitive C``` types into JS native types. It transfers the value, not the reference.

Sample Usage:
```c++
// Integer, Number, Boolean
JS_LOCAL_VALUE my_int = STD_TO_INTEGER(11);
JS_LOCAL_VALUE my_double = STD_TO_NUMBER(22);
JS_LOCAL_VALUE my_boolean = STD_TO_BOOLEAN(true);

// String
JS_LOCAL_STRING native_str = STD_TO_STRING("My String");
std::string str2 = "Hello!";
JS_LOCAL_STRING native_str2 = STD_TO_STRING_WITH_LENGTH(str2.c_str(), str2.length());
JS_LOCAL_STRING unicode_str = UTF8_TO_STRING("日本語");
```
***

# *_TO_STD
You can also convert a native JS variable back to it's primitive type. 

Sample Usage
```c++
int my_int = INTEGER_TO_STD(js_int);
double my_double = NUMBER_TO_STD(js_number);
bool my_bool = BOOLEAN_TO_STD(js_bool);
std::string my_str = STRING_TO_STD(js_str); // attention: std::string copies the value! 
                                            //            STRING_TO_STD's return value is freed next line

// you could also use jxcore::JXString class
jxcore::JXString my_str(js_str);
int ln = my_str.length();
int utf8_ln = my_str.Utf8Length();
const char *cc_str = *my_str;
```
***

# TYPE_TO_LOCAL_*
```HANDLE``` and ```PERSISTENT``` wrapped native JS variables shouldn't be assigned to ```LOCAL``` variables directly. Whenever it's required to reach one of those variables, it's expected to carry their value into local scope.

Sample Usage
```c++
JS_LOCAL_VALUE val = TYPE_TO_LOCAL_VALUE(my_handle_value);

JS_LOCAL_STRING str = TYPE_TO_LOCAL_STRING(my_persistent_string);

JS_LOCAL_OBJECT obj = TYPE_TO_LOCAL_OBJECT(my_persistent_object);
```
***

# JS_IS_*
There are many ways to check the content of a native JS variable. 

Sample Usage
```c++
bool JS_IS_EMPTY(my_obj) // check if it's empty ?
bool JS_IS_NULL_OR_UNDEFINED(my_obj) // null or undefined ?
bool JS_IS_FUNCTION(obj) // is function ?
bool JS_IS_NUMBER(my_obj) // is number ?
bool JS_IS_BOOLEAN(my_obj) // is boolean ?
bool JS_IS_STRING(my_obj) // is string ?
bool JS_IS_DATE(my_obj) // is date ?
bool JS_IS_INT32(my_obj) // is 32 int ?
bool JS_IS_UINT32(my_obj) // is unsigned ?
bool JS_IS_TRUE(my_obj) // is true ?
bool JS_IS_FALSE(my_obj) // or false ?
bool JS_IS_REGEXP(my_obj) // regexp ?
bool JS_IS_NULL(my_obj) // null ?
bool JS_IS_UNDEFINED(my_obj) // or undefined ?
bool JS_IS_OBJECT(my_obj) // is it an object ?
bool JS_IS_ARRAY(my_obj) // or an array ?
```
***

## JS_VALUE_TO_*
Converts a JavaScript ```Value``` type to individual native JavaScript types.

Sample Usage:
```c++
JS_LOCAL_VALUE val = ...some JS value..

JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(val); // JS_IS_OBJECT ?
JS_LOCAL_STRING str = JS_VALUE_TO_STRING(val); // JS_IS_STRING ?
JS_LOCAL_NUMBER num = JS_VALUE_TO_NUMBER(val); // JS_IS_NUMBER ?
JS_LOCAL_INTEGER nt = JS_VALUE_TO_INTEGER(val); // JS_IS_INTEGER ?
JS_LOCAL_BOOLEAN bl = JS_VALUE_TO_BOOLEAN(val);
```
***

## JS_NEW_EMPTY_OBJECT
Creates an empty native JavaScript object.

Sample Usage
```c++
JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
JS_NAME_SET(obj, JS_STRING_ID("name"), STD_TO_STRING("John Doe"));
```
***

## JS_NEW_ARRAY
Creates an empty native JavaScript array object.

Sample Usage
```c++
JS_LOCAL_ARRAY arr = JS_NEW_ARRAY();
JS_INDEX_SET(arr, 0, STD_TO_STRING("John Doe"));
```
You may prefer ```JS_NEW_ARRAY_WITH_COUNT``` if you already know the size of the array.

Sample Usage
```c++
JS_LOCAL_ARRAY arr = JS_NEW_ARRAY_WITH_COUNT(1);
JS_INDEX_SET(arr, 0, STD_TO_STRING("John Doe"));
```
***

## JS_NEW_FUNCTION_TEMPLATE
Creates a JavaScript function representation to a native method.

Sample Usage
```c++
JS_LOCAL_METHOD(method) {
  .
  .
  .
}
JS_METHOD_END
.
.
JS_LOCAL_FUNCTION func = JS_GET_FUNCTION(JS_NEW_FUNCTION_TEMPLATE(method));
JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
JS_NAME_SET(obj, JS_STRING_ID("native_method"), func);
```
***
