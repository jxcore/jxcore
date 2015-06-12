/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#ifndef LD_DATABASE_H
#define LD_DATABASE_H

#include <map>
#include <vector>
#include <node.h>

#include <leveldb/cache.h>
#include <leveldb/db.h>
#include <leveldb/filter_policy.h>

#include "leveldown.h"
#include "iterator.h"

#include "jx_persistent_store.h"

namespace leveldown {

DEFINE_JS_METHOD(LevelDOWN);

struct Reference {
  JS_PERSISTENT_OBJECT handle;
  leveldb::Slice slice;

  Reference(JS_LOCAL_VALUE obj, leveldb::Slice slice) : slice(slice) {
    JS_ENTER_SCOPE_COM();
    JS_DEFINE_STATE_MARKER(com);

    JS_LOCAL_OBJECT _obj = JS_NEW_EMPTY_OBJECT();
    JS_NAME_SET(_obj, JS_STRING_ID("obj"), obj);
    handle = JS_NEW_PERSISTENT_OBJECT(_obj);
  };
};

static inline void ClearReferences (std::vector<Reference *> *references) {
  for (std::vector<Reference *>::iterator it = references->begin()
      ; it != references->end()
      ; ) {
    DisposeStringOrBufferFromSlice((*it)->handle, (*it)->slice);
    it = references->erase(it);
  }
  delete references;
}

class Database : public node::ObjectWrap {
public:
  static jxcore::ThreadStore<JS_PERSISTENT_FUNCTION_TEMPLATE> jx_persistent;

  INIT_NAMED_CLASS_MEMBERS(Database, Database) {
    int id = com->threadId;
    jx_persistent.templates[id] =
        JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(constructor);

    SET_INSTANCE_METHOD("open", Database::Open, 0);
    SET_INSTANCE_METHOD("close", Database::Close, 0);
    SET_INSTANCE_METHOD("put", Database::Put, 0);
    SET_INSTANCE_METHOD("get", Database::Get, 0);
    SET_INSTANCE_METHOD("del", Database::Delete, 0);
    SET_INSTANCE_METHOD("batch", Database::Batch, 0);
    SET_INSTANCE_METHOD("approximateSize", Database::ApproximateSize, 0);
    SET_INSTANCE_METHOD("getProperty", Database::GetProperty, 0);
    SET_INSTANCE_METHOD("iterator", Database::Iterator, 0);
  }
  END_INIT_NAMED_MEMBERS(Database)

  static JS_HANDLE_VALUE NewInstance (JS_LOCAL_STRING &location);

  leveldb::Status OpenDatabase (leveldb::Options* options);
  leveldb::Status PutToDatabase (
      leveldb::WriteOptions* options
    , leveldb::Slice key
    , leveldb::Slice value
  );
  leveldb::Status GetFromDatabase (
      leveldb::ReadOptions* options
    , leveldb::Slice key
    , std::string& value
  );
  leveldb::Status DeleteFromDatabase (
      leveldb::WriteOptions* options
    , leveldb::Slice key
  );
  leveldb::Status WriteBatchToDatabase (
      leveldb::WriteOptions* options
    , leveldb::WriteBatch* batch
  );
  uint64_t ApproximateSizeFromDatabase (const leveldb::Range* range);
  void GetPropertyFromDatabase (const leveldb::Slice& property, std::string* value);
  leveldb::Iterator* NewIterator (leveldb::ReadOptions* options);
  const leveldb::Snapshot* NewSnapshot ();
  void ReleaseSnapshot (const leveldb::Snapshot* snapshot);
  void CloseDatabase ();
  void ReleaseIterator (uint32_t id);

  Database (const JS_HANDLE_VALUE& from);
  ~Database ();

private:
  jxcore::JXString location;
  leveldb::DB* db;
  uint32_t currentIteratorId;
  void(*pendingCloseWorker);
  leveldb::Cache* blockCache;
  const leveldb::FilterPolicy* filterPolicy;

  std::map< uint32_t, leveldown::Iterator * > iterators;

  static void WriteDoing(uv_work_t *req);
  static void WriteAfter(uv_work_t *req);

  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(Open);
  static DEFINE_JS_METHOD(Close);
  static DEFINE_JS_METHOD(Put);
  static DEFINE_JS_METHOD(Delete);
  static DEFINE_JS_METHOD(Get);
  static DEFINE_JS_METHOD(Batch);
  static DEFINE_JS_METHOD(Write);
  static DEFINE_JS_METHOD(Iterator);
  static DEFINE_JS_METHOD(ApproximateSize);
  static DEFINE_JS_METHOD(GetProperty);
};

} // namespace leveldown

#endif
