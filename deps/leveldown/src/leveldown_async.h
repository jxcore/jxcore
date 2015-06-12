/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#ifndef LD_LEVELDOWN_ASYNC_H
#define LD_LEVELDOWN_ASYNC_H

#include <node.h>
#include "async.h"

namespace leveldown {

class DestroyWorker : public AsyncWorker {
public:
  DestroyWorker (
      jxcore::JXString &location
    , NanCallback *callback
  );

  virtual ~DestroyWorker ();
  virtual void Execute ();

private:
  jxcore::JXString location;
};

class RepairWorker : public AsyncWorker {
public:
  RepairWorker (
      jxcore::JXString &location
    , NanCallback *callback
  );

  virtual ~RepairWorker ();
  virtual void Execute ();

private:
  jxcore::JXString location;
};

} // namespace leveldown

#endif
