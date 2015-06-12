/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#include <node.h>
#include <node_buffer.h>
#include "nan.h"

#include <leveldb/write_batch.h>
#include <leveldb/filter_policy.h>

#include "database.h"
#include "leveldown.h"
#include "async.h"
#include "database_async.h"

namespace leveldown {

/** OPEN WORKER **/

OpenWorker::OpenWorker(Database *database, NanCallback *callback,
                       leveldb::Cache *blockCache,
                       const leveldb::FilterPolicy *filterPolicy,
                       bool createIfMissing, bool errorIfExists,
                       bool compression, uint32_t writeBufferSize,
                       uint32_t blockSize, uint32_t maxOpenFiles,
                       uint32_t blockRestartInterval)
    : AsyncWorker(database, callback) {
  options = new leveldb::Options();
  options->block_cache = blockCache;
  options->filter_policy = filterPolicy;
  options->create_if_missing = createIfMissing;
  options->error_if_exists = errorIfExists;
  options->compression =
      compression ? leveldb::kSnappyCompression : leveldb::kNoCompression;
  options->write_buffer_size = writeBufferSize;
  options->block_size = blockSize;
  options->max_open_files = maxOpenFiles;
  options->block_restart_interval = blockRestartInterval;
};

OpenWorker::~OpenWorker() { delete options; }

void OpenWorker::Execute() { SetStatus(database->OpenDatabase(options)); }

/** CLOSE WORKER **/

CloseWorker::CloseWorker(Database *database, NanCallback *callback)
    : AsyncWorker(database, callback){};

CloseWorker::~CloseWorker() {}

void CloseWorker::Execute() { database->CloseDatabase(); }

void CloseWorker::WorkComplete() {
  JS_ENTER_SCOPE();
  HandleOKCallback();
  delete callback;
  callback = NULL;
}

/** IO WORKER (abstract) **/

IOWorker::IOWorker(Database *database, NanCallback *callback,
                   leveldb::Slice key, JS_LOCAL_OBJECT &keyHandle)
    : AsyncWorker(database, callback), key(key) {
  JS_ENTER_SCOPE();

  SaveToPersistent("key", keyHandle);
};

IOWorker::~IOWorker() {}

void IOWorker::WorkComplete() {
  JS_ENTER_SCOPE();

  JS_LOCAL_OBJECT obj_key = GetFromPersistent("key");
  DisposeStringOrBufferFromSlice(obj_key, key);
  AsyncWorker::WorkComplete();
}

/** READ WORKER **/

ReadWorker::ReadWorker(Database *database, NanCallback *callback,
                       leveldb::Slice key, bool asBuffer, bool fillCache,
                       JS_LOCAL_OBJECT &keyHandle)
    : IOWorker(database, callback, key, keyHandle), asBuffer(asBuffer) {
  JS_ENTER_SCOPE();

  options = new leveldb::ReadOptions();
  options->fill_cache = fillCache;
  SaveToPersistent("key", keyHandle);
};

ReadWorker::~ReadWorker() { delete options; }

void ReadWorker::Execute() {
  SetStatus(database->GetFromDatabase(options, key, value));
}

void ReadWorker::HandleOKCallback() {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_VALUE returnValue;
  if (asBuffer) {
    returnValue = JS_TYPE_TO_LOCAL_VALUE(
        node::Buffer::New((char *)value.data(), value.size(), com)->handle_);
  } else {
    returnValue =
        UTF8_TO_STRING_WITH_LENGTH((char *)value.data(), value.size());
  }
  JS_LOCAL_VALUE argv[] = {JS_NULL(), returnValue};
  callback->Call(2, argv);
}

/** DELETE WORKER **/

DeleteWorker::DeleteWorker(Database *database, NanCallback *callback,
                           leveldb::Slice key, bool sync,
                           JS_LOCAL_OBJECT &keyHandle)
    : IOWorker(database, callback, key, keyHandle) {
  JS_ENTER_SCOPE();

  options = new leveldb::WriteOptions();
  options->sync = sync;
  SaveToPersistent("key", keyHandle);
};

DeleteWorker::~DeleteWorker() { delete options; }

void DeleteWorker::Execute() {
  SetStatus(database->DeleteFromDatabase(options, key));
}

/** WRITE WORKER **/

WriteWorker::WriteWorker(Database *database, NanCallback *callback,
                         leveldb::Slice key, leveldb::Slice value, bool sync,
                         JS_LOCAL_OBJECT &keyHandle,
                         JS_LOCAL_OBJECT &valueHandle)
    : DeleteWorker(database, callback, key, sync, keyHandle), value(value) {
  JS_ENTER_SCOPE();

  SaveToPersistent("value", valueHandle);
};

WriteWorker::~WriteWorker() {}

void WriteWorker::Execute() {
  SetStatus(database->PutToDatabase(options, key, value));
}

void WriteWorker::WorkComplete() {
  JS_ENTER_SCOPE();

  JS_LOCAL_OBJECT obj_val = GetFromPersistent("value");
  DisposeStringOrBufferFromSlice(obj_val, value);
  IOWorker::WorkComplete();
}

/** BATCH WORKER **/

BatchWorker::BatchWorker(Database *database, NanCallback *callback,
                         leveldb::WriteBatch *batch, bool sync)
    : AsyncWorker(database, callback), batch(batch) {
  options = new leveldb::WriteOptions();
  options->sync = sync;
};

BatchWorker::~BatchWorker() {
  delete batch;
  delete options;
}

void BatchWorker::Execute() {
  SetStatus(database->WriteBatchToDatabase(options, batch));
}

/** APPROXIMATE SIZE WORKER **/

ApproximateSizeWorker::ApproximateSizeWorker(Database *database,
                                             NanCallback *callback,
                                             leveldb::Slice start,
                                             leveldb::Slice end,
                                             JS_LOCAL_OBJECT &startHandle,
                                             JS_LOCAL_OBJECT &endHandle)
    : AsyncWorker(database, callback), range(start, end) {
  JS_ENTER_SCOPE();

  SaveToPersistent("start", startHandle);
  SaveToPersistent("end", endHandle);
};

ApproximateSizeWorker::~ApproximateSizeWorker() {}

void ApproximateSizeWorker::Execute() {
  size = database->ApproximateSizeFromDatabase(&range);
}

void ApproximateSizeWorker::WorkComplete() {
  JS_ENTER_SCOPE();

  JS_LOCAL_OBJECT obj_start = GetFromPersistent("start");
  JS_LOCAL_OBJECT obj_end = GetFromPersistent("end");
  DisposeStringOrBufferFromSlice(obj_start, range.start);
  DisposeStringOrBufferFromSlice(obj_end, range.limit);
  AsyncWorker::WorkComplete();
}

void ApproximateSizeWorker::HandleOKCallback() {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_VALUE returnValue = STD_TO_NUMBER((double)size);
  JS_LOCAL_VALUE argv[] = {JS_NULL(), returnValue};

  callback->Call(2, argv);
}

}  // namespace leveldown
