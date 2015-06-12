#ifndef LD_BATCH_H
#define LD_BATCH_H

#include <vector>
#include <node.h>

#include <leveldb/write_batch.h>

#include "database.h"
#include "jx_persistent_store.h"

namespace leveldown {

class Batch : public node::ObjectWrap {
 public:
  static jxcore::ThreadStore<JS_PERSISTENT_FUNCTION_TEMPLATE> jx_persistent;

  INIT_NAMED_CLASS_MEMBERS(Batch, Batch) {
    int id = com->threadId;
    jx_persistent.templates[id] =
        JS_NEW_PERSISTENT_FUNCTION_TEMPLATE(constructor);

    SET_INSTANCE_METHOD("put", Batch::Put, 0);
    SET_INSTANCE_METHOD("del", Batch::Del, 0);
    SET_INSTANCE_METHOD("clear", Batch::Clear, 0);
    SET_INSTANCE_METHOD("write", Batch::Write, 0);
  }
  END_INIT_NAMED_MEMBERS(Batch)

  static JS_HANDLE_VALUE NewInstance(JS_HANDLE_OBJECT_REF database,
                                     JS_HANDLE_OBJECT_REF optionsObj);

  Batch(leveldown::Database* database, bool sync);
  ~Batch();
  leveldb::Status Write();

 private:
  leveldown::Database* database;
  leveldb::WriteOptions* options;
  leveldb::WriteBatch* batch;
  bool hasData;  // keep track of whether we're writing data or not

  static DEFINE_JS_METHOD(New);
  static DEFINE_JS_METHOD(Put);
  static DEFINE_JS_METHOD(Del);
  static DEFINE_JS_METHOD(Clear);
  static DEFINE_JS_METHOD(Write);
};

}  // namespace leveldown

#endif
