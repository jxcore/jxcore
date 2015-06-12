/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#include <node.h>

#include "leveldown.h"
#include "database.h"
#include "iterator.h"
#include "batch.h"
#include "leveldown_async.h"

namespace leveldown {

JS_LOCAL_METHOD(DestroyDB) {
  if (args.Length() < 2 || !args.IsString(0) || !args.IsFunction(1)) {
    THROW_EXCEPTION("DestroyDB expects (string, function) parameters");
  }

  jxcore::JXString location;
  args.GetString(0, &location);

  JS_LOCAL_FUNCTION fnc = JS_TYPE_TO_LOCAL_FUNCTION(args.GetAsFunction(1));
  NanCallback* callback = new NanCallback(fnc);

  location.DisableAutoGC();
  DestroyWorker* worker = new DestroyWorker(location, callback);

  NanAsyncQueueWorker(worker);
}
JS_METHOD_END

JS_LOCAL_METHOD(RepairDB) {
  if (args.Length() < 2 || !args.IsString(0) || !args.IsFunction(1)) {
    THROW_EXCEPTION("DestroyDB expects (string, function) parameters");
  }

  jxcore::JXString location;
  args.GetString(0, &location);

  JS_LOCAL_FUNCTION fnc = JS_TYPE_TO_LOCAL_FUNCTION(args.GetAsFunction(1));
  NanCallback* callback = new NanCallback(fnc);

  location.DisableAutoGC();

  RepairWorker* worker = new RepairWorker(location, callback);

  NanAsyncQueueWorker(worker);
}
JS_METHOD_END

void RegisterModule(JS_HANDLE_OBJECT_REF target) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  Database::Initialize(target);
  leveldown::Iterator::Initialize(target);
  leveldown::Batch::Initialize(target);

  JS_LOCAL_FUNCTION leveldown =
      JS_GET_FUNCTION(JS_NEW_FUNCTION_CALL_TEMPLATE(LevelDOWN));

  JS_METHOD_SET(leveldown, "destroy", DestroyDB);
  JS_METHOD_SET(leveldown, "repair", RepairDB);

  JS_METHOD_SET(target, "leveldown", LevelDOWN);
}

}  // namespace leveldown
