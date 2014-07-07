Applies To (JXcore Beta2 and later versions)

Native C++ Node.JS modules are 100% compatible with JXcore when the application is running under a single thread. In case of a multithreaded usage of a native module, there are some basic considerations for problem free JXcore multithreading.
 
So far we have already embedded some of the most popular native modules into JXcore and applied multithreading. Besides these embedded modules, most of the native Node.JS modules are already compatible with multithreaded usage. JXcore also handles libUV eventloop targeting internally hence it is safe to use default event loop from libUV whenever it is necessary.
 
One possible issue with native code multithreading is related to global/static variables. Before JXcore, the assumption was that only one instance of the native module would be available or instantiated per process. Each JXcore thread must be considered as a separate process hence using static/global variables may cause unexpected issues with the final execution.

!! Applying below update doesn't change the behavior of the native module under Node.JS or single threaded JXcore

!! In order to compile the module for multithreading. You should call "jx install [module_name]" 


** USAGE:

** Simply replace

```static Persistent<FunctionTemplate> SOMECLASS::constructor_template;```

** To
 
```static ThreadStore<Persistent<FunctionTemplate> > SOMECLASS::c_constructor_template;```
 
**Whenever the constructor_template is needed;
 
 
```
 const int tid = c_constructor_template.getThreadId();
 c_constructor_template.templates[tid] // use instead constructor_template
``` 
