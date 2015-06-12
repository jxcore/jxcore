/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#include <node.h>
#include <node_buffer.h>

#include <leveldb/db.h>
#include <leveldb/write_batch.h>

#include "leveldown.h"
#include "database.h"
#include "async.h"
#include "database_async.h"
#include "batch.h"
#include "iterator.h"
#include "jx_persistent_store.h"

namespace leveldown {

jxcore::ThreadStore<JS_PERSISTENT_FUNCTION_TEMPLATE> Database::jx_persistent;

Database::Database(const JS_HANDLE_VALUE& from)
    : db(NULL),
      currentIteratorId(0),
      pendingCloseWorker(NULL),
      blockCache(NULL),
      filterPolicy(NULL) {
  location.SetFromHandle(from);
};

Database::~Database() {
  if (db != NULL) delete db;
  location.Dispose();
};

/* Calls from worker threads, NO V8 HERE *****************************/

leveldb::Status Database::OpenDatabase(leveldb::Options* options) {
  return leveldb::DB::Open(*options, *location, &db);
}

leveldb::Status Database::PutToDatabase(leveldb::WriteOptions* options,
                                        leveldb::Slice key,
                                        leveldb::Slice value) {
  return db->Put(*options, key, value);
}

leveldb::Status Database::GetFromDatabase(leveldb::ReadOptions* options,
                                          leveldb::Slice key,
                                          std::string& value) {
  return db->Get(*options, key, &value);
}

leveldb::Status Database::DeleteFromDatabase(leveldb::WriteOptions* options,
                                             leveldb::Slice key) {
  return db->Delete(*options, key);
}

leveldb::Status Database::WriteBatchToDatabase(leveldb::WriteOptions* options,
                                               leveldb::WriteBatch* batch) {
  return db->Write(*options, batch);
}

uint64_t Database::ApproximateSizeFromDatabase(const leveldb::Range* range) {
  uint64_t size;
  db->GetApproximateSizes(range, 1, &size);
  return size;
}

void Database::GetPropertyFromDatabase(const leveldb::Slice& property,
                                       std::string* value) {

  db->GetProperty(property, value);
}

leveldb::Iterator* Database::NewIterator(leveldb::ReadOptions* options) {
  return db->NewIterator(*options);
}

const leveldb::Snapshot* Database::NewSnapshot() { return db->GetSnapshot(); }

void Database::ReleaseSnapshot(const leveldb::Snapshot* snapshot) {
  return db->ReleaseSnapshot(snapshot);
}

void Database::ReleaseIterator(uint32_t id) {
  // called each time an Iterator is End()ed, in the main thread
  // we have to remove our reference to it and if it's the last iterator
  // we have to invoke a pending CloseWorker if there is one
  // if there is a pending CloseWorker it means that we're waiting for
  // iterators to end before we can close them
  iterators.erase(id);
  if (iterators.empty() && pendingCloseWorker != NULL) {
    NanAsyncQueueWorker((AsyncWorker*)pendingCloseWorker);
    pendingCloseWorker = NULL;
  }
}

void Database::CloseDatabase() {
  delete db;
  db = NULL;
  if (blockCache) {
    delete blockCache;
    blockCache = NULL;
  }
  if (filterPolicy) {
    delete filterPolicy;
    filterPolicy = NULL;
  }
}

/* V8 exposed functions *****************************/

JS_LOCAL_METHOD(LevelDOWN) {
  JS_LOCAL_STRING location = JS_TYPE_TO_LOCAL_STRING(args.GetAsString(0));
  RETURN_PARAM(Database::NewInstance(location));
}
JS_METHOD_END

JS_METHOD(Database, New) {
  JS_CLASS_NEW_INSTANCE(obj, Database);
  JS_LOCAL_OBJECT arg0 = JS_VALUE_TO_OBJECT(args.GetItem(0));
  Database* db = new Database(arg0);
  db->Wrap(obj);

  RETURN_PARAM(obj);
}
JS_METHOD_END

JS_HANDLE_VALUE Database::NewInstance(JS_LOCAL_STRING& location) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_OBJECT instance;

  JS_LOCAL_FUNCTION_TEMPLATE constructorHandle =
      JS_TYPE_TO_LOCAL_FUNCTION_TEMPLATE(
          Database::jx_persistent.templates[com->threadId]);

  JS_HANDLE_VALUE argv[] = {location};

  JS_LOCAL_FUNCTION cons = JS_GET_FUNCTION(constructorHandle);
  instance = JS_NEW_INSTANCE(cons, 1, argv);

  return JS_LEAVE_SCOPE(instance);
}

JS_METHOD(Database, Open) {
  LD_METHOD_SETUP_COMMON(open, 0, 1)

  JS_LOCAL_VALUE obj = JS_GET_NAME(optionsObj, JS_STRING_ID("createIfMissing"));

  bool createIfMissing = true;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("createIfMissing"))) {
    obj = JS_GET_NAME(optionsObj, JS_STRING_ID("createIfMissing"));
    createIfMissing = BOOLEAN_TO_STD(obj);
  }

  bool errorIfExists = false;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("errorIfExists"))) {
    obj = JS_GET_NAME(optionsObj, JS_STRING_ID("errorIfExists"));
    errorIfExists = BOOLEAN_TO_STD(obj);
  }

  bool compression = true;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("compression"))) {
    obj = JS_GET_NAME(optionsObj, JS_STRING_ID("compression"));
    compression = BOOLEAN_TO_STD(obj);
  }

  uint32_t cacheSize = 8 << 20;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("cacheSize"))) {
    obj = JS_GET_NAME(optionsObj, JS_STRING_ID("cacheSize"));
    cacheSize = NUMBER_TO_STD(obj);
  }

  uint32_t writeBufferSize = 4 << 20;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("writeBufferSize"))) {
    obj = JS_GET_NAME(optionsObj, JS_STRING_ID("writeBufferSize"));
    writeBufferSize = NUMBER_TO_STD(obj);
  }

  uint32_t blockSize = 4096;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("blockSize"))) {
    obj = JS_GET_NAME(optionsObj, JS_STRING_ID("blockSize"));
    blockSize = NUMBER_TO_STD(obj);
  }

  uint32_t maxOpenFiles = 1000;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("maxOpenFiles"))) {
    obj = JS_GET_NAME(optionsObj, JS_STRING_ID("maxOpenFiles"));
    maxOpenFiles = NUMBER_TO_STD(obj);
  }

  uint32_t blockRestartInterval = 16;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("blockRestartInterval"))) {
    obj = JS_GET_NAME(optionsObj, JS_STRING_ID("blockRestartInterval"));
    blockRestartInterval = NUMBER_TO_STD(obj);
  }

  database->blockCache = leveldb::NewLRUCache(cacheSize);
  database->filterPolicy = leveldb::NewBloomFilterPolicy(10);

  OpenWorker* worker = new OpenWorker(
      database, new NanCallback(callback), database->blockCache,
      database->filterPolicy, createIfMissing, errorIfExists, compression,
      writeBufferSize, blockSize, maxOpenFiles, blockRestartInterval);
  // persist to prevent accidental GC
  JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
  worker->SaveToPersistent("database", _this);
  NanAsyncQueueWorker(worker);
}
JS_METHOD_END

JS_METHOD(Database, Close) {
  LD_METHOD_SETUP_COMMON_ONEARG(close)

  CloseWorker* worker = new CloseWorker(database, new NanCallback(callback));
  // persist to prevent accidental GC
  JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
  worker->SaveToPersistent("database", _this);

  if (!database->iterators.empty()) {
    // yikes, we still have iterators open! naughty naughty.
    // we have to queue up a CloseWorker and manually close each of them.
    // the CloseWorker will be invoked once they are all cleaned up
    database->pendingCloseWorker = worker;

    for (std::map<uint32_t, leveldown::Iterator*>::iterator it =
             database->iterators.begin();
         it != database->iterators.end(); ++it) {

      // for each iterator still open, first check if it's already in
      // the process of ending (ended==true means an async End() is
      // in progress), if not, then we call End() with an empty callback
      // function and wait for it to hit ReleaseIterator() where our
      // CloseWorker will be invoked

      leveldown::Iterator* iterator = it->second;

      if (!iterator->ended) {
        JS_LOCAL_OBJECT obj = NanObjectWrapHandle(iterator);
        JS_LOCAL_VALUE obj_end;
        if (JS_HAS_NAME(obj, JS_STRING_ID("end"))) {
          obj_end = JS_GET_NAME(obj, JS_STRING_ID("end"));
        }

        if (JS_IS_FUNCTION(obj_end)) {
          JS_LOCAL_FUNCTION end = JS_TYPE_AS_FUNCTION(obj_end);
          JS_LOCAL_FUNCTION_TEMPLATE tmp = JS_NEW_EMPTY_FUNCTION_TEMPLATE();

          JS_LOCAL_VALUE argv[] = {
              JS_GET_FUNCTION(tmp)
          };
          NanMakeCallback(obj, end, 1, argv);
        } else {
          THROW_EXCEPTION("database instance doesn't have end function defined");
        }
      }
    }
  } else {
    NanAsyncQueueWorker(worker);
  }
}
JS_METHOD_END

JS_METHOD(Database, Put) {
  LD_METHOD_SETUP_COMMON(put, 2, 3)

  JS_LOCAL_OBJECT keyHandle = JS_VALUE_TO_OBJECT(args.GetItem(0));
  JS_LOCAL_OBJECT valueHandle = JS_VALUE_TO_OBJECT(args.GetItem(1));
  LD_STRING_OR_BUFFER_TO_SLICE(key, keyHandle, key)
  LD_STRING_OR_BUFFER_TO_SLICE(value, valueHandle, value)

  bool sync = false;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("sync"))) {
    JS_LOCAL_VALUE obj_sync = JS_GET_NAME(optionsObj, JS_STRING_ID("sync"));
    sync = BOOLEAN_TO_STD(obj_sync);
  }

  WriteWorker* worker =
      new WriteWorker(database, new NanCallback(callback), key, value, sync,
                      keyHandle, valueHandle);

  // persist to prevent accidental GC
  JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
  worker->SaveToPersistent("database", _this);
  NanAsyncQueueWorker(worker);
}
JS_METHOD_END

JS_METHOD(Database, Get) {
  LD_METHOD_SETUP_COMMON(get, 1, 2)

  JS_LOCAL_OBJECT keyHandle = JS_VALUE_TO_OBJECT(args.GetItem(0));
  LD_STRING_OR_BUFFER_TO_SLICE(key, keyHandle, key)

  bool asBuffer = true;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("asBuffer"))) {
    JS_LOCAL_VALUE obj_sync =
        JS_GET_NAME(optionsObj, JS_STRING_ID("asBuffer"));
    asBuffer = BOOLEAN_TO_STD(obj_sync);
  }

  bool fillCache = true;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("fillCache"))) {
    JS_LOCAL_VALUE obj_sync =
        JS_GET_NAME(optionsObj, JS_STRING_ID("fillCache"));
    fillCache = BOOLEAN_TO_STD(obj_sync);
  }

  ReadWorker* worker = new ReadWorker(database, new NanCallback(callback), key,
                                      asBuffer, fillCache, keyHandle);
  // persist to prevent accidental GC
  JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
  worker->SaveToPersistent("database", _this);
  NanAsyncQueueWorker(worker);
}
JS_METHOD_END

JS_METHOD(Database, Delete) {
  LD_METHOD_SETUP_COMMON(del, 1, 2)

  JS_LOCAL_OBJECT keyHandle = JS_VALUE_TO_OBJECT(args.GetItem(0));
  LD_STRING_OR_BUFFER_TO_SLICE(key, keyHandle, key)

  bool sync = false;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("sync"))) {
    JS_LOCAL_VALUE obj_sync = JS_GET_NAME(optionsObj, JS_STRING_ID("sync"));
    sync = BOOLEAN_TO_STD(obj_sync);
  }

  DeleteWorker* worker = new DeleteWorker(database, new NanCallback(callback),
                                          key, sync, keyHandle);
  // persist to prevent accidental GC
  JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
  worker->SaveToPersistent("database", _this);
  NanAsyncQueueWorker(worker);
}
JS_METHOD_END

JS_METHOD(Database, Batch) {
  if ((args.Length() == 0 || args.Length() == 1) && !args.IsArray(0)) {
    JS_LOCAL_OBJECT optionsObj;
    if (args.Length() > 0 && args.IsObject(0)) {
      optionsObj = JS_VALUE_TO_OBJECT(args.GetItem(0));
    }

    JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
    RETURN_PARAM(Batch::NewInstance(_this, optionsObj));
  }

  LD_METHOD_SETUP_COMMON(batch, 1, 2)

  bool sync = false;
  if (JS_HAS_NAME(optionsObj, JS_STRING_ID("sync"))) {
    JS_LOCAL_VALUE obj_sync = JS_GET_NAME(optionsObj, JS_STRING_ID("sync"));
    sync = BOOLEAN_TO_STD(obj_sync);
  }

  JS_LOCAL_OBJECT arg0 = JS_VALUE_TO_OBJECT(args.GetItem(0));
  JS_LOCAL_ARRAY array = JS_TYPE_TO_LOCAL_ARRAY(arg0);

  leveldb::WriteBatch* batch = new leveldb::WriteBatch();
  bool hasData = false;

  unsigned ln = JS_GET_ARRAY_LENGTH(array);
  for (unsigned int i = 0; i < ln; i++) {
    JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(JS_GET_INDEX(array, i));
    if (!JS_IS_OBJECT(obj)) continue;

    JS_LOCAL_VALUE keyBuffer = JS_GET_NAME(obj, JS_STRING_ID("key"));
    JS_LOCAL_VALUE type = JS_GET_NAME(obj, JS_STRING_ID("type"));

    jxcore::JXString str_type(type);
    if (strcmp(*str_type, "del") == 0) {
      LD_STRING_OR_BUFFER_TO_SLICE(key, keyBuffer, key)

      batch->Delete(key);
      if (!hasData) hasData = true;

      DisposeStringOrBufferFromSlice(keyBuffer, key);
    } else if (strcmp(*str_type, "put") == 0) {
      JS_LOCAL_VALUE valueBuffer = JS_GET_NAME(obj, STD_TO_STRING("value"));

      LD_STRING_OR_BUFFER_TO_SLICE(key, keyBuffer, key)
      LD_STRING_OR_BUFFER_TO_SLICE(value, valueBuffer, value)
      batch->Put(key, value);
      if (!hasData) hasData = true;

      DisposeStringOrBufferFromSlice(keyBuffer, key);
      DisposeStringOrBufferFromSlice(valueBuffer, value);
    }
  }

  // don't allow an empty batch through
  if (hasData) {
    BatchWorker* worker =
        new BatchWorker(database, new NanCallback(callback), batch, sync);
    // persist to prevent accidental GC
    JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
    worker->SaveToPersistent("database", _this);
    NanAsyncQueueWorker(worker);
  } else {
    LD_RUN_CALLBACK(callback, 0, NULL);
  }
}
JS_METHOD_END

JS_METHOD(Database, ApproximateSize) {
  JS_LOCAL_OBJECT startHandle = JS_VALUE_TO_OBJECT(args.GetItem(0));
  JS_LOCAL_OBJECT endHandle = JS_VALUE_TO_OBJECT(args.GetItem(1));

  LD_METHOD_SETUP_COMMON(approximateSize, -1, 2)

  LD_STRING_OR_BUFFER_TO_SLICE(start, startHandle, start)
  LD_STRING_OR_BUFFER_TO_SLICE(end, endHandle, end)

  ApproximateSizeWorker* worker = new ApproximateSizeWorker(
      database, new NanCallback(callback), start, end, startHandle, endHandle);
  // persist to prevent accidental GC
  JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
  worker->SaveToPersistent("database", _this);
  NanAsyncQueueWorker(worker);
}
JS_METHOD_END

JS_METHOD(Database, GetProperty) {
  if (args.Length() < 1) {
    THROW_EXCEPTION("Database::GetProperty expects a parameter");
  }

  JS_LOCAL_VALUE propertyHandle = JS_VALUE_TO_OBJECT(args.GetItem(0));
  JS_LOCAL_FUNCTION callback;

  LD_STRING_OR_BUFFER_TO_SLICE(property, propertyHandle, property)

  leveldown::Database* database =
      node::ObjectWrap::Unwrap<leveldown::Database>(args.This());

  std::string* value = new std::string();
  database->GetPropertyFromDatabase(property, value);
  JS_LOCAL_STRING returnValue =
      UTF8_TO_STRING_WITH_LENGTH(value->c_str(), value->length());
  delete value;
  delete[] property.data();

  RETURN_PARAM(returnValue);
}
JS_METHOD_END

JS_METHOD(Database, Iterator) {
  Database* database = node::ObjectWrap::Unwrap<Database>(args.This());

  JS_LOCAL_OBJECT optionsObj;
  if (args.Length() > 0 && args.IsObject(0)) {
    optionsObj = JS_VALUE_TO_OBJECT(args.GetItem(0));
  }

  // each iterator gets a unique id for this Database, so we can
  // easily store & lookup on our `iterators` map
  uint32_t id = database->currentIteratorId++;
  JS_TRY_CATCH(try_catch);

  JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
  JS_LOCAL_OBJECT iteratorHandle =
      Iterator::NewInstance(_this, STD_TO_NUMBER(id), optionsObj);
  if (try_catch.HasCaught()) {
    // NB: node::FatalException can segfault here if there is no room on stack.
    THROW_EXCEPTION("Fatal Error in Database::Iterator!");
  }

  leveldown::Iterator* iterator =
      node::ObjectWrap::Unwrap<leveldown::Iterator>(iteratorHandle);

  database->iterators[id] = iterator;

  RETURN_PARAM(iteratorHandle);
}
JS_METHOD_END

}  // namespace leveldown
