## JS_METHOD 
Native ```Class``` method template definer. 

Sample Usage
```c++
JS_METHOD(MyClass, Sum) {
  JS_LOCAL_VALUE result = STD_TO_INTEGER(args.GetInt32(0) + args.GetInt32(1));
  RETURN_PARAM(result);
}
JS_METHOD_END
  
// JS side -> console.log(myClass.Sum(3, 4)); -> 7 
``` 
```RETURN``` or ```RETURN_PARAM``` can be used to mimic the functionality of ```return```. In order not to return anything (```undefined```) you can either call ```RETURN();``` or don't call anything. By default every native method returns ```undefined```. ```RETURN_PARAM``` expects a native JS variable. 

For example, the above method doesn't convert any native type to a JavaScript type. In such case you may use ```JS_LOCAL_METHOD_NO_COM``` name instead. 
The underlying JavaScript engine might need to know, on which context/isolate the JavaScript variable should be hosted. JXcore's macro automatically manages that by ```com``` variable (instance of commons class).
This ```com``` variable carries the actual context/isolate reference for actual instance. Remind that you can safely use the version given on above sample for any case and it shouldn't affect the performance.
***

## DEFINE_JS_METHOD 
Declares the definition of a native method. 

Sample
```c++
class MyClass {
 public:
  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(Sum);
};
```
***

## JS_LOCAL_METHOD 
Native method template definer.

Sample Usage
```c++
JS_LOCAL_METHOD(Print) {
  jxcore::JXstring str;
  args.GetString(0, &str);
  printf("%s", *str);
}
JS_METHOD_END
  
// JS side -> Print('Hello!\n'); -> Hello!  
```
```RETURN``` or ```RETURN_PARAM``` can be used to mimic the functionality of ```return```. In order not to return anything (```undefined```) you can either call ```RETURN();``` or don't call anything. By default every native method returns ```undefined```. ```RETURN_PARAM``` expects a native JS variable. 

For example, the above method doesn't convert any native type to a JavaScript type. In such case you may use ```JS_LOCAL_METHOD_NO_COM``` name instead. 
The underlying JavaScript engine might need to know, on which context/isolate the JavaScript variable should be hosted. JXcore's macro automatically manages that by ```com``` variable (instance of commons class).
This ```com``` variable carries the actual context/isolate reference for actual instance. Remind that you can safely use the version given on above sample for any case and it shouldn't affect the performance.
***

## JS_METHOD_SET
Defines a native method into a JavaScript object (static member) with a name. 

Sample Usage
```c++
JS_LOCAL_METHOD(MyMethod) {
  .
  .
  .
}
JS_METHOD_END

void DefineMethod(JS_HANDLE_OBJECT obj, const char *name, 
                                      JS_NATIVE_METHOD method) {
  JS_ENTER_SCOPE();
  JS_METHOD_SET(obj, name, method);
}

JS_LOCAL_METHOD(AnotherMethod) {
  JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
  DefineMethod(obj, "myMethod", MyMethod);
  RETURN_PARAM(obj);
}
JS_METHOD_END

//  JS:
//  var obj = anotherMethod();
//  obj.myMethod();
```
You could also use ```JS_NAME_SET``` in order to define a native method. The only difference is that you should wrap ```MyMethod``` with the ```FUNCTION_TEMPLATE``` type.

```c++
void DefineMethod(JS_HANDLE_OBJECT obj, const char *name, 
                                      JS_NATIVE_METHOD method) {
  JS_ENTER_SCOPE();
  JS_LOCAL_FUNCTION l_method =                             
        JS_GET_FUNCTION(JS_NEW_FUNCTION_TEMPLATE(method));

  JS_NAME_SET(obj, JS_STRING_ID(name), l_method);
}
```
***

## JS_CLASS_NEW_INSTANCE 
Strictly defined for a CLASS::New operation. Creates a new instance of a native class. If a native class is expected to support ```new``` calls from the JS side, it should have a method defined as ```New```. 

Sample Usage
```c++
// myclass_wrap.h file
#include "jxcore.h"
class MyClass {
 public:
  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(Sum);

  INIT_NAMED_CLASS_MEMBERS(MyClass, MyClassWrap) {
    SET_INSTANCE_METHOD("sum", Sum, 2);
  }
  END_INIT_NAMED_MEMBERS(MyClass)
};

// myclass_wrap.cc file
#include "myclass_wrap.h"
JS_METHOD(MyClass, New) {
  JS_CLASS_NEW_INSTANCE(obj, MyClass);
  
  RETURN_POINTER(obj);
}
JS_METHOD_END

JS_METHOD(MyClass, Sum) {
  JS_LOCAL_VALUE result = STD_TO_INTEGER(args.GetInt32(0) + args.GetInt32(1));
  RETURN_PARAM(result);
}
JS_METHOD_END
NODE_MODULE(node_myclass_wrap, MyClass::Initialize)
```
***

## RETURN 
```RETURN``` and it's variations are defined to help with returning the function output back to JS side. 

Sample Usage
```c++
RETURN(); // returns undefined
RETURN_TRUE(); // returns true
RETURN_FALSE(); // returns false
RETURN_PARAM(STD_TO_BOOLEAN(false)); // returns false
RETURN_PARAM(STD_TO_STRING("Hello")); // returns "Hello"
```
***

## THROW_EXCEPTION
```THROW_EXCEPTION``` and it's variations are defined to help with throwing an exception back to JS side. You may also set the type of an exception by calling either ```THROW_TYPE_EXCEPTION``` or ```THROW_RANGE_EXCEPTION```. They have the same syntax with ```THROW_EXCEPTION```

Sample Usage
```c++
JS_METHOD(MyClass, Sum) {
  if (args.Length() < 2) {
    THROW_EXCEPTION( "This method needs 2 parameters (int, int)" );
  }

  JS_LOCAL_VALUE result = STD_TO_INTEGER(args.GetInt32(0) + args.GetInt32(1));
  RETURN_PARAM(result);
}
JS_METHOD_END
```
***

## INIT_CLASS_MEMBERS
Helpers for the class ```Initialize``` method definition. If you have a native object with only static members, you don't need to define ```New``` native method. In such a case, ```INIT_CLASS_MEMBERS``` would be your choice. Otherwise, ```INIT_NAMED_CLASS_MEMBERS``` should be used.

See ```JS_CLASS_NEW_INSTANCE``` for an [INIT_NAMED_CLASS_MEMBERS](#js_class_new_instance) sample.

Sample Usage (```INIT_CLASS_MEMBERS```)
```c++
class MyClass {
  static DEFINE_JS_METHOD(SetProperty);
 public:
  INIT_CLASS_MEMBERS() {
    SET_CLASS_METHOD("setProperty", SetProperty, 2);
  }
  END_INIT_MEMBERS
};

JS_METHOD(MyClass, SetProperty) {
  JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(args.GetItem(0));
  JS_LOCAL_STRING name = args.GetAsString(1);
  JS_LOCAL_OBJECT value = JS_VALUE_TO_OBJECT(args.GetItem(2));

  JS_NAME_SET(obj, name, value);
}
JS_METHOD_END

// Usage: 
// var obj = {};
// myClass.setProperty(obj, "member", 45);
// console.log( obj.member ); -> 45
```
***

## SET_INSTANCE_METHOD
Defines a prototype method on a native class template. It's similar to JS ```object.prototype.mymethod = function..``` 
```c++
SET_INSTANCE_METHOD(String name, JS_NATIVE_METHOD method, Integer expected_parameters_count)
```
P.S. ```expected_parameters_count``` has no effect on actual functionality. You may set it to 0 if you are not sure. Sample usage for ```SET_INSTANCE_METHOD``` is available from [JS_CLASS_NEW_INSTANCE](#js_class_new_instance)
***

## SET_CLASS_METHOD
Defines a static method on a native class template. 
```c++
SET_CLASS_METHOD(String name, JS_NATIVE_METHOD method, Integer expected_parameters_count)
```
P.S. ```expected_parameters_count``` has no effect on actual functionality. You may set it to 0 if you are not sure. Sample usage for ```SET_CLASS_METHOD``` is available from [INIT_CLASS_MEMBERS](#init_class_members)
***