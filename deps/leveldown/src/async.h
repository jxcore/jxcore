/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#ifndef LD_ASYNC_H
#define LD_ASYNC_H

#include <node.h>
#include "nan.h"
#include "database.h"

namespace leveldown {

class Database;

/* abstract */ class AsyncWorker : public NanAsyncWorker {
public:
  node::commons *com_;

  AsyncWorker (
      leveldown::Database* database
    , NanCallback *callback
  ) : NanAsyncWorker(callback), database(database) {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);
    com_ = com;
    JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
    persistentHandle = JS_NEW_PERSISTENT_OBJECT(obj);
  }

protected:
  void SetStatus(leveldb::Status status) {
    this->status = status;
    if (!status.ok())
      SetErrorMessage(status.ToString().c_str());
  }
  Database* database;
private:
  leveldb::Status status;
};

} // namespace leveldown

#endif
