This document focuses on jxcore's new public interface located under (`src/public`). We want this interface to be very easy to use and embedabble on any platform.  If you need to reach all the core features, you may want to check `src/public/jx.cc` to see how we have actually implemented this interface. So you can add your custom methods.

In order to embed jxcore into your application, you should first compile it as a library.

`./configure --static-library --prefix=/targetFolder --engine-mozilla` (for V8 remove --engine-mozilla)  
`./make install`

Additional useful parameters
```
--dest-cpu=ia32   ->  32 bit  (ia32, arm, armv7s, arm64, x86_64)
--dest-os=ios     ->   (ios, android, or leave empty for your current platform)
```

In the end you will have all the binaries and include files. The new public interface only expects `jx.h` and `jx_result.h` files in addition to the libraries under the `bin` folder. You can find the headers files under `include/node/src/public`

The sample below demonstrates a basic usage of the interface
```c++
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <unistd.h>

#include "jx.h"

#define flush_console(...)        \
  do {                            \
    fprintf(stdout, __VA_ARGS__); \
    fflush(stdout);               \
  } while (0)

void ConvertResult(JXResult *result, std::string &to_result) {
  switch (result->type_) {
    case RT_Null:
      to_result = "null";
      break;
    case RT_Undefined:
      to_result = "undefined";
      break;
    case RT_Boolean:
      to_result = JX_GetBoolean(result) ? "true" : "false";
      break;
    case RT_Int32: {
      std::stringstream ss;
      ss << JX_GetInt32(result);
      to_result = ss.str();
    } break;
    case RT_Double: {
      std::stringstream ss;
      ss << JX_GetDouble(result);
      to_result = ss.str();
    } break;
    case RT_Buffer: {
      to_result = JX_GetString(result);
    } break;
    case RT_JSON:
    case RT_String: {
      to_result = JX_GetString(result);
    } break;
    case RT_Error: {
      to_result = JX_GetString(result);
    } break;
    default:
      to_result = "null";
      return;
  }
}

void callback(JXResult *results, int argc) {
  // do nothing
}

void sampleMethod(JXResult *results, int argc) {
  flush_console("sampleMethod Called;\n");

  std::stringstream ss_result;
  for (int i = 0; i < argc; i++) {
    std::string str_result;
    ConvertResult(&results[i], str_result);
    ss_result << i << " : ";
    ss_result << str_result << "\n";
  }

  flush_console("%s", ss_result.str().c_str());

  // return an Array back to JS Land
  const char *str = "[1, 2, 3]";

  // results[argc] corresponds to return value
  JX_SetJSON(&results[argc], str, strlen(str));
}

int main(int argc, char **args) {
  char *path = args[0];
  JX_Initialize(path, callback);

  char *contents = "console.log('hello world');";
  JX_DefineMainFile(contents);

  JX_DefineExtension("sampleMethod", sampleMethod);
  JX_StartEngine();

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  JXResult result;
  JX_Evaluate(
      "var arr = process.natives.sampleMethod('String Parameter', {foo:1}); \n"
      "console.log('result: ', arr, 'length:', arr.length ); \n"
      "setTimeout(function() { \n"
      "  console.log('end!'); \n"
      "}, 100);",
      "myscript", &result);

  // loop for possible IO
  // or JX_Loop() without usleep / while
  while (JX_LoopOnce() != 0) usleep(1);

  JX_StopEngine();
}
```

Expected output should be;
```
hello world
sampleMethod Called;
0 : String Parameter
1 : {"foo":1}
result:  [ 1, 2, 3 ] length: 3
end!
```

In order to compile the source codes above (lets say you saved it into sample.cpp)

OSX :
```bash
g++ sample.cpp -stdlib=libstdc++ -lstdc++ -std=c++11 -O3 -I/targetFolder/include/node/public \
    /targetFolder/bin/libcares.a	/targetFolder/bin/libjx.a /targetFolder/bin/libsqlite3.a \
    /targetFolder/bin/libchrome_zlib.a /targetFolder/bin/libmozjs.a  /targetFolder/bin/libuv.a \
    /targetFolder/bin/libhttp_parser.a	/targetFolder/bin/libopenssl.a \
    -Wno-c++11-compat-deprecated-writable-strings -Wl -framework CoreServices -o sample
```

Linux:
```bash
g++ sample.cpp -lstdc++ -std=c++11 -pthread -O3 -Wno-write-strings -I/targetFolder/include/node/public \
    -fno-rtti /targetFolder/bin/libjx.a /targetFolder/bin/libsqlite3.a \
    /targetFolder/bin/libchrome_zlib.a /targetFolder/bin/libmozjs.a  /targetFolder/bin/libuv.a \
    /targetFolder/bin/libhttp_parser.a	/targetFolder/bin/libopenssl.a  \
    -ldl /targetFolder/bin/libcares.a \
    -o sample
```

Scripts above assumes you've compiled JXcore static libraries for x64 architecture. In case you did that for 32 bit, you should add `-m32` argument.

Besides the architecture, if you have compiled JXcore with V8 engine, you should replace `libmozjs.a` above to `libv8_base.a` and also add `libv8_nosnapshot.a`

That's It!
