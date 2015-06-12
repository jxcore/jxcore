#include <node.h>
#include <node_buffer.h>
#include "nan.h"

#include "database.h"
#include "batch_async.h"
#include "batch.h"

namespace leveldown {

jxcore::ThreadStore<JS_PERSISTENT_FUNCTION_TEMPLATE> Batch::jx_persistent;

Batch::Batch(leveldown::Database* database, bool sync) : database(database) {
  options = new leveldb::WriteOptions();
  options->sync = sync;
  batch = new leveldb::WriteBatch();
  hasData = false;
}

Batch::~Batch() {
  delete options;
  delete batch;
}

leveldb::Status Batch::Write() {
  return database->WriteBatchToDatabase(options, batch);
}

JS_METHOD(Batch, New) {
  JS_CLASS_NEW_INSTANCE(obj, Batch);

  JS_LOCAL_OBJECT objwrap = JS_VALUE_TO_OBJECT(args.GetItem(0));
  Database* database = node::ObjectWrap::Unwrap<Database>(objwrap);
  JS_LOCAL_OBJECT optionsObj;

  if (args.Length() > 1 && args.IsObject(1)) {
    optionsObj = JS_VALUE_TO_OBJECT(args.GetItem(1));
  }

  bool sync = BOOLEAN_TO_STD(JS_GET_NAME(optionsObj, JS_STRING_ID("sync")));

  Batch* batch = new Batch(database, sync);
  batch->Wrap(obj);

  RETURN_PARAM(obj);
}
JS_METHOD_END

JS_HANDLE_VALUE Batch::NewInstance(JS_HANDLE_OBJECT_REF database,
                                   JS_HANDLE_OBJECT_REF optionsObj) {
  JS_ENTER_SCOPE_COM();

  JS_LOCAL_OBJECT instance;
  JS_LOCAL_FUNCTION_TEMPLATE constructorHandle =
      JS_TYPE_TO_LOCAL_FUNCTION_TEMPLATE(
          jx_persistent.templates[com->threadId]);

  JS_LOCAL_FUNCTION fnc = JS_GET_FUNCTION(constructorHandle);
  if (JS_IS_EMPTY(optionsObj)) {
    JS_HANDLE_VALUE argv[1] = {database};
    instance = JS_NEW_INSTANCE(fnc, 1, argv);
  } else {
    JS_HANDLE_VALUE argv[2] = {database, optionsObj};
    instance = JS_NEW_INSTANCE(fnc, 2, argv);
  }

  return JS_LEAVE_SCOPE(instance);
}

JS_METHOD(Batch, Put) {
  Batch* batch = ObjectWrap::Unwrap<Batch>(args.This());
  JS_HANDLE_FUNCTION callback;  // purely for the error macros

  if (args.Length() < 2) {
    THROW_EXCEPTION("Batch::put expects two parameters");
  }

  JS_LOCAL_VALUE keyBuffer = GET_ARG(0);
  JS_LOCAL_VALUE valueBuffer = GET_ARG(1);
  LD_STRING_OR_BUFFER_TO_SLICE(key, keyBuffer, key)
  LD_STRING_OR_BUFFER_TO_SLICE(value, valueBuffer, value)

  batch->batch->Put(key, value);
  if (!batch->hasData) batch->hasData = true;

  DisposeStringOrBufferFromSlice(keyBuffer, key);
  DisposeStringOrBufferFromSlice(valueBuffer, value);

  RETURN_PARAM(args.Holder());
}
JS_METHOD_END

JS_METHOD(Batch, Del) {
  Batch* batch = ObjectWrap::Unwrap<Batch>(args.This());

  if (args.Length() == 0) {
    THROW_EXCEPTION("Batch::Del expects a parameter");
  }

  JS_HANDLE_FUNCTION callback;  // purely for the error macros

  JS_LOCAL_VALUE keyBuffer = JS_TYPE_TO_LOCAL_VALUE(args.GetItem(0));
  LD_STRING_OR_BUFFER_TO_SLICE(key, keyBuffer, key)

  batch->batch->Delete(key);
  if (!batch->hasData) batch->hasData = true;

  DisposeStringOrBufferFromSlice(keyBuffer, key);

  RETURN_PARAM(args.Holder());
}
JS_METHOD_END

JS_METHOD(Batch, Clear) {
  Batch* batch = ObjectWrap::Unwrap<Batch>(args.This());

  batch->batch->Clear();
  batch->hasData = false;

  RETURN_PARAM(args.Holder());
}
JS_METHOD_END

JS_METHOD(Batch, Write) {
  Batch* batch = ObjectWrap::Unwrap<Batch>(args.This());

  if (!args.IsFunction(0)) {
    THROW_EXCEPTION("Batch::Write expects a function");
  }

  JS_HANDLE_FUNCTION lfnc = args.GetAsFunction(0);
  if (batch->hasData) {
    NanCallback* callback = new NanCallback(lfnc);
    BatchWriteWorker* worker = new BatchWriteWorker(batch, callback);
    // persist to prevent accidental GC
    JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
    worker->SaveToPersistent("batch", _this);
    NanAsyncQueueWorker(worker);
  } else {
    LD_RUN_CALLBACK(lfnc, 0, NULL);
  }
}
JS_METHOD_END

}  // namespace leveldown
