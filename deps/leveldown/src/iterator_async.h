/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#ifndef LD_ITERATOR_ASYNC_H
#define LD_ITERATOR_ASYNC_H

#include <node.h>
#include "async.h"
#include "iterator.h"

namespace leveldown {

class NextWorker : public AsyncWorker {
public:
  NextWorker (
      Iterator* iterator
    , NanCallback *callback
    , void (*localCallback)(Iterator*)
  );

  virtual ~NextWorker ();
  virtual void Execute ();
  virtual void HandleOKCallback ();

private:
  Iterator* iterator;
  void (*localCallback)(Iterator*);
  std::vector<std::pair<std::string, std::string> > result;
  bool ok;
};

class EndWorker : public AsyncWorker {
public:
  EndWorker (
      Iterator* iterator
    , NanCallback *callback
  );

  virtual ~EndWorker ();
  virtual void Execute ();
  virtual void HandleOKCallback ();

private:
  Iterator* iterator;
};

} // namespace leveldown

#endif
