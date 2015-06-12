/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#ifndef LD_DATABASE_ASYNC_H
#define LD_DATABASE_ASYNC_H

#include <vector>
#include <node.h>

#include <leveldb/cache.h>

#include "async.h"

namespace leveldown {

class OpenWorker : public AsyncWorker {
public:
  OpenWorker (
      Database *database
    , NanCallback *callback
    , leveldb::Cache* blockCache
    , const leveldb::FilterPolicy* filterPolicy
    , bool createIfMissing
    , bool errorIfExists
    , bool compression
    , uint32_t writeBufferSize
    , uint32_t blockSize
    , uint32_t maxOpenFiles
    , uint32_t blockRestartInterval
  );

  virtual ~OpenWorker ();
  virtual void Execute ();

private:
  leveldb::Options* options;
};

class CloseWorker : public AsyncWorker {
public:
  CloseWorker (
      Database *database
    , NanCallback *callback
  );

  virtual ~CloseWorker ();
  virtual void Execute ();
  virtual void WorkComplete ();
};

class IOWorker    : public AsyncWorker {
public:
  IOWorker (
      Database *database
    , NanCallback *callback
    , leveldb::Slice key
    , JS_LOCAL_OBJECT &keyHandle
  );

  virtual ~IOWorker ();
  virtual void WorkComplete ();

protected:
  leveldb::Slice key;
};

class ReadWorker : public IOWorker {
public:
  ReadWorker (
      Database *database
    , NanCallback *callback
    , leveldb::Slice key
    , bool asBuffer
    , bool fillCache
    , JS_LOCAL_OBJECT &keyHandle
  );

  virtual ~ReadWorker ();
  virtual void Execute ();
  virtual void HandleOKCallback ();

private:
  bool asBuffer;
  leveldb::ReadOptions* options;
  std::string value;
};

class DeleteWorker : public IOWorker {
public:
  DeleteWorker (
      Database *database
    , NanCallback *callback
    , leveldb::Slice key
    , bool sync
    , JS_LOCAL_OBJECT &keyHandle
  );

  virtual ~DeleteWorker ();
  virtual void Execute ();

protected:
  leveldb::WriteOptions* options;
};

class WriteWorker : public DeleteWorker {
public:
  WriteWorker (
      Database *database
    , NanCallback *callback
    , leveldb::Slice key
    , leveldb::Slice value
    , bool sync
    , JS_LOCAL_OBJECT &keyHandle
    , JS_LOCAL_OBJECT &valueHandle
  );

  virtual ~WriteWorker ();
  virtual void Execute ();
  virtual void WorkComplete ();

private:
  leveldb::Slice value;
};

class BatchWorker : public AsyncWorker {
public:
  BatchWorker (
      Database *database
    , NanCallback *callback
    , leveldb::WriteBatch* batch
    , bool sync
  );

  virtual ~BatchWorker ();
  virtual void Execute ();

private:
  leveldb::WriteOptions* options;
  leveldb::WriteBatch* batch;
};

class ApproximateSizeWorker : public AsyncWorker {
public:
  ApproximateSizeWorker (
      Database *database
    , NanCallback *callback
    , leveldb::Slice start
    , leveldb::Slice end
    , JS_LOCAL_OBJECT &startHandle
    , JS_LOCAL_OBJECT &endHandle
  );

  virtual ~ApproximateSizeWorker ();
  virtual void Execute ();
  virtual void HandleOKCallback ();
  virtual void WorkComplete ();

  private:
    leveldb::Range range;
    uint64_t size;
};

} // namespace leveldown

#endif
