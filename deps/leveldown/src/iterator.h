/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#ifndef LD_ITERATOR_H
#define LD_ITERATOR_H

#include <node.h>
#include <vector>
#include "nan.h"

#include "leveldown.h"
#include "database.h"
#include "async.h"
#include "jx_persistent_store.h"

namespace leveldown {

class Database;
class AsyncWorker;

class Iterator : public node::ObjectWrap {
public:
  static jxcore::ThreadStore<JS_PERSISTENT_FUNCTION_TEMPLATE> jx_persistent;

  INIT_NAMED_CLASS_MEMBERS(Iterator, Iterator) {
    int id = com->threadId;
    jx_persistent.templates[id] =
        JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(constructor);

    SET_INSTANCE_METHOD("next", Iterator::Next, 0);
    SET_INSTANCE_METHOD("end", Iterator::End, 0);
  }
  END_INIT_NAMED_MEMBERS(Iterator)

  static JS_LOCAL_OBJECT NewInstance (
      JS_LOCAL_OBJECT database
    , JS_LOCAL_VALUE id
    , JS_LOCAL_OBJECT optionsObj
  );

  Iterator (
      Database* database
    , uint32_t id
    , leveldb::Slice* start
    , std::string* end
    , bool reverse
    , bool keys
    , bool values
    , int limit
    , std::string* lt
    , std::string* lte
    , std::string* gt
    , std::string* gte
    , bool fillCache
    , bool keyAsBuffer
    , bool valueAsBuffer
    , JS_LOCAL_OBJECT &startHandle
    , size_t highWaterMark
  );

  ~Iterator ();

  bool IteratorNext (std::vector<std::pair<std::string, std::string> >& result);
  leveldb::Status IteratorStatus ();
  void IteratorEnd ();
  void Release ();

private:
  Database* database;
  uint32_t id;
  leveldb::Iterator* dbIterator;
  leveldb::ReadOptions* options;
  leveldb::Slice* start;
  std::string* end;
  bool reverse;
  bool keys;
  bool values;
  int limit;
  std::string* lt;
  std::string* lte;
  std::string* gt;
  std::string* gte;
  int count;
  size_t highWaterMark;

public:
  bool keyAsBuffer;
  bool valueAsBuffer;
  bool nexting;
  bool ended;
  AsyncWorker* endWorker;

private:
  JS_PERSISTENT_OBJECT persistentHandle;

  bool Read (std::string& key, std::string& value);
  bool GetIterator ();

  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(Next);
  static DEFINE_JS_METHOD(End);
};

} // namespace leveldown

#endif
