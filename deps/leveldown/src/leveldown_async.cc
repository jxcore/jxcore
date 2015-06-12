/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#include <leveldb/db.h>

#include "leveldown.h"
#include "leveldown_async.h"

namespace leveldown {

/** DESTROY WORKER **/

DestroyWorker::DestroyWorker (
    jxcore::JXString &location
  , NanCallback *callback
) : AsyncWorker(NULL, callback)
  , location(location)
{};

DestroyWorker::~DestroyWorker () {
  location.Dispose();
}

void DestroyWorker::Execute () {
  leveldb::Options options;
  SetStatus(leveldb::DestroyDB(*location, options));
}

/** REPAIR WORKER **/

RepairWorker::RepairWorker (
    jxcore::JXString &location
  , NanCallback *callback
) : AsyncWorker(NULL, callback)
  , location(location)
{};

RepairWorker::~RepairWorker () {
  location.Dispose();
}

void RepairWorker::Execute () {
  leveldb::Options options;
  SetStatus(leveldb::RepairDB(*location, options));
}

} // namespace leveldown
