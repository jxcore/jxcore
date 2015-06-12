/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#include <node.h>
#include <node_buffer.h>

#include "database.h"
#include "leveldown.h"
#include "async.h"
#include "iterator_async.h"

namespace leveldown {

/** NEXT-MULTI WORKER **/

NextWorker::NextWorker (
    Iterator* iterator
  , NanCallback *callback
  , void (*localCallback)(Iterator*)
) : AsyncWorker(NULL, callback)
  , iterator(iterator)
  , localCallback(localCallback)
{};

NextWorker::~NextWorker () {}

void NextWorker::Execute () {
  ok = iterator->IteratorNext(result);
  if (!ok)
    SetStatus(iterator->IteratorStatus());
}

void NextWorker::HandleOKCallback () {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  size_t idx = 0;

  size_t arraySize = result.size() * 2;
  JS_LOCAL_ARRAY returnArray = JS_NEW_ARRAY_WITH_COUNT(arraySize);

  for(idx = 0; idx < result.size(); ++idx) {
    std::pair<std::string, std::string> row = result[idx];
    std::string key = row.first;
    std::string value = row.second;

    JS_LOCAL_VALUE returnKey;
    if (iterator->keyAsBuffer) {
      returnKey = JS_TYPE_TO_LOCAL_VALUE(node::Buffer::New((char*)key.data(), key.size(), com)->handle_);
    } else {
      returnKey = UTF8_TO_STRING_WITH_LENGTH((char*)key.data(), key.size());
    }

    JS_LOCAL_VALUE returnValue;
    if (iterator->valueAsBuffer) {
      returnValue = JS_TYPE_TO_LOCAL_VALUE(node::Buffer::New((char*)value.data(), value.size(), com)->handle_);
    } else {
      returnValue = UTF8_TO_STRING_WITH_LENGTH((char*)value.data(), value.size());
    }

    // put the key & value in a descending order, so that they can be .pop:ed in javascript-land
    JS_INDEX_SET(returnArray, (static_cast<int>(arraySize - idx * 2 - 1)), returnKey);
    JS_INDEX_SET(returnArray, (static_cast<int>(arraySize - idx * 2 - 2)), returnValue);
  }

  // clean up & handle the next/end state see iterator.cc/checkEndCallback
  localCallback(iterator);

  JS_LOCAL_VALUE argv[] = {
      JS_NULL()
    , returnArray
    // when ok === false all data has been read, so it's then finished
    , STD_TO_BOOLEAN(!ok)
  };

  callback->Call(3, argv);
}

/** END WORKER **/

EndWorker::EndWorker (
    Iterator* iterator
  , NanCallback *callback
) : AsyncWorker(NULL, callback)
  , iterator(iterator)
{};

EndWorker::~EndWorker () {}

void EndWorker::Execute () {
  iterator->IteratorEnd();
}

void EndWorker::HandleOKCallback () {
  iterator->Release();
  callback->Call(0, NULL);
}

} // namespace leveldown
