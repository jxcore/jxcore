This document is not yet completed. If you have an experience on the subject, feel free to contribute!

If you want to see `jxcore` embedded in action, you might want to check `jxcore.h` and `node_main.cc` in additon to below explanations.

Embedding jxcore.h alone is enough. Depending to the engine, you should locate the below definitions into your source code.

```c++

// if it's SpiderMonkey
// #define JS_ENGINE_MOZJS 1

// else if V8
// #define JS_ENGINE_V8

// if it's SpiderMonkey and target arch is 64 bit (for both intel and arm)
// #define JS_PUNBOX64 1

// else if SpiderMonkey and 32 bit or arm7/arm7s
// #define JS_NUNBOX32 1

// if it's SpiderMonkey and target OS is osx / ios
// #define JS_HAVE_MACHINE_ENDIAN_H 1

// else if it's SpiderMonkey and linux / android / freebsd
// #define JS_HAVE_ENDIAN_H 


// you should set the right definitions above before including the below header
// do not define `if` and `else` keys together!
#include "jxcore.h"

int main() {
  // now you can create the engine instance and run
  jxcore::JXEngine engine(argc, argv, false);
  engine.Init();
  engine.Start();

  while (engine.LoopOnce() != 0) usleep(1);

  engine.Destroy();
  
  return 0;
}
```
