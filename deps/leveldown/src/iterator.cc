/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */

#include <node.h>
#include <node_buffer.h>

#include "database.h"
#include "iterator.h"
#include "iterator_async.h"

namespace leveldown {

jxcore::ThreadStore<JS_PERSISTENT_FUNCTION_TEMPLATE> Iterator::jx_persistent;

Iterator::Iterator(Database* database, uint32_t id, leveldb::Slice* start,
                   std::string* end, bool reverse, bool keys, bool values,
                   int limit, std::string* lt, std::string* lte,
                   std::string* gt, std::string* gte, bool fillCache,
                   bool keyAsBuffer, bool valueAsBuffer,
                   JS_LOCAL_OBJECT& startHandle, size_t highWaterMark)
    : database(database),
      id(id),
      start(start),
      end(end),
      reverse(reverse),
      keys(keys),
      values(values),
      limit(limit),
      lt(lt),
      lte(lte),
      gt(gt),
      gte(gte),
      highWaterMark(highWaterMark),
      keyAsBuffer(keyAsBuffer),
      valueAsBuffer(valueAsBuffer) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_OBJECT obj = JS_NEW_EMPTY_OBJECT();
  if (!JS_IS_EMPTY(startHandle))
    JS_NAME_SET(obj, JS_STRING_ID("start"), startHandle);

  persistentHandle = JS_NEW_PERSISTENT_OBJECT(obj);

  options = new leveldb::ReadOptions();
  options->fill_cache = fillCache;
  // get a snapshot of the current state
  options->snapshot = database->NewSnapshot();
  dbIterator = NULL;
  count = 0;
  nexting = false;
  ended = false;
  endWorker = NULL;
};

Iterator::~Iterator() {
  delete options;
  if (!JS_IS_EMPTY(persistentHandle)) JS_CLEAR_PERSISTENT(persistentHandle);
  if (start != NULL) delete start;
  if (end != NULL) delete end;
};

bool Iterator::GetIterator() {
  if (dbIterator == NULL) {
    dbIterator = database->NewIterator(options);

    if (start != NULL) {
      dbIterator->Seek(*start);

      if (reverse) {
        if (!dbIterator->Valid()) {
          // if it's past the last key, step back
          dbIterator->SeekToLast();
        } else {
          std::string key_ = dbIterator->key().ToString();

          if (lt != NULL) {
            if (lt->compare(key_) <= 0) dbIterator->Prev();
          } else if (lte != NULL) {
            if (lte->compare(key_) < 0) dbIterator->Prev();
          } else if (start != NULL) {
            if (start->compare(key_)) dbIterator->Prev();
          }
        }

        if (dbIterator->Valid() && lt != NULL) {
          if (lt->compare(dbIterator->key().ToString()) <= 0)
            dbIterator->Prev();
        }
      } else {
        if (dbIterator->Valid() && gt != NULL &&
            gt->compare(dbIterator->key().ToString()) == 0)
          dbIterator->Next();
      }
    } else if (reverse) {
      dbIterator->SeekToLast();
    } else {
      dbIterator->SeekToFirst();
    }

    return true;
  }
  return false;
}

bool Iterator::Read(std::string& key, std::string& value) {
  // if it's not the first call, move to next item.
  if (!GetIterator()) {
    if (reverse)
      dbIterator->Prev();
    else
      dbIterator->Next();
  }

  // now check if this is the end or not, if not then return the key & value
  if (dbIterator->Valid()) {
    std::string key_ = dbIterator->key().ToString();
    int isEnd = end == NULL ? 1 : end->compare(key_);

    if ((limit < 0 || ++count <= limit) &&
        (end == NULL || (reverse && (isEnd <= 0)) ||
         (!reverse && (isEnd >= 0))) &&
        (lt != NULL ? (lt->compare(key_) > 0)
                    : lte != NULL ? (lte->compare(key_) >= 0) : true) &&
        (gt != NULL ? (gt->compare(key_) < 0)
                    : gte != NULL ? (gte->compare(key_) <= 0) : true)) {
      if (keys) key.assign(dbIterator->key().data(), dbIterator->key().size());
      if (values)
        value.assign(dbIterator->value().data(), dbIterator->value().size());
      return true;
    }
  }

  return false;
}

bool Iterator::IteratorNext(
    std::vector<std::pair<std::string, std::string> >& result) {
  size_t size = 0;
  while (true) {
    std::string key, value;
    bool ok = Read(key, value);

    if (ok) {
      result.push_back(std::make_pair(key, value));
      size = size + key.size() + value.size();

      if (size > highWaterMark) return true;

    } else {
      return false;
    }
  }
}

leveldb::Status Iterator::IteratorStatus() { return dbIterator->status(); }

void Iterator::IteratorEnd() {
  // TODO: could return it->status()
  delete dbIterator;
  dbIterator = NULL;
}

void Iterator::Release() { database->ReleaseIterator(id); }

void checkEndCallback(Iterator* iterator) {
  iterator->nexting = false;
  if (iterator->endWorker != NULL) {
    NanAsyncQueueWorker(iterator->endWorker);
    iterator->endWorker = NULL;
  }
}

JS_METHOD(Iterator, Next) {
  Iterator* iterator = node::ObjectWrap::Unwrap<Iterator>(args.This());

  JS_LOCAL_FUNCTION callback = JS_TYPE_TO_LOCAL_FUNCTION(args.GetAsFunction(0));

  NextWorker* worker =
      new NextWorker(iterator, new NanCallback(callback), checkEndCallback);
  // persist to prevent accidental GC
  JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
  worker->SaveToPersistent("iterator", _this);
  iterator->nexting = true;
  NanAsyncQueueWorker(worker);

  RETURN_PARAM(args.Holder());
}
JS_METHOD_END

JS_METHOD(Iterator, End) {
  Iterator* iterator = node::ObjectWrap::Unwrap<Iterator>(args.This());

  JS_LOCAL_FUNCTION callback = JS_TYPE_TO_LOCAL_FUNCTION(args.GetAsFunction(0));

  EndWorker* worker = new EndWorker(iterator, new NanCallback(callback));
  // persist to prevent accidental GC
  JS_LOCAL_OBJECT _this = JS_TYPE_TO_LOCAL_OBJECT(args.This());
  worker->SaveToPersistent("iterator", _this);
  iterator->ended = true;

  if (iterator->nexting) {
    // waiting for a next() to return, queue the end
    iterator->endWorker = worker;
  } else {
    NanAsyncQueueWorker(worker);
  }

  RETURN_PARAM(args.Holder());
}
JS_METHOD_END

JS_LOCAL_OBJECT Iterator::NewInstance(JS_LOCAL_OBJECT database,
                                      JS_LOCAL_VALUE id,
                                      JS_LOCAL_OBJECT optionsObj) {

  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  JS_LOCAL_OBJECT instance;

  JS_LOCAL_FUNCTION_TEMPLATE constructorHandle =
      JS_TYPE_TO_LOCAL_FUNCTION_TEMPLATE(
          Iterator::jx_persistent.templates[com->threadId]);

  if (JS_IS_EMPTY(optionsObj)) {
    JS_HANDLE_VALUE argv[2] = {database, id};
    instance = JS_NEW_INSTANCE(JS_GET_FUNCTION(constructorHandle), 2, argv);
  } else {
    JS_HANDLE_VALUE argv[3] = {database, id, optionsObj};
    instance = JS_NEW_INSTANCE(JS_GET_FUNCTION(constructorHandle), 3, argv);
  }

  return JS_LEAVE_SCOPE(instance);
}

JS_METHOD(Iterator, New) {
  JS_LOCAL_OBJECT _this = JS_VALUE_TO_OBJECT(args.GetItem(0));
  Database* database = node::ObjectWrap::Unwrap<Database>(_this);

  // TODO: remove this, it's only here to make LD_STRING_OR_BUFFER_TO_SLICE
  // happy
  JS_HANDLE_FUNCTION callback;

  JS_LOCAL_VALUE startHandle;
  leveldb::Slice* start = NULL;
  std::string* end = NULL;
  int limit = -1;
  // default highWaterMark from Readble-streams
  size_t highWaterMark = 16 * 1024;

  JS_LOCAL_VALUE id = JS_TYPE_TO_LOCAL_VALUE(args.GetItem(1));

  JS_LOCAL_OBJECT optionsObj;

  JS_LOCAL_OBJECT ltHandle;
  JS_LOCAL_OBJECT lteHandle;
  JS_LOCAL_OBJECT gtHandle;
  JS_LOCAL_OBJECT gteHandle;

  std::string* lt = NULL;
  std::string* lte = NULL;
  std::string* gt = NULL;
  std::string* gte = NULL;

  // default to forward.
  bool reverse = false;

  if (args.Length() > 1 && args.IsObject(2)) {
    optionsObj = JS_VALUE_TO_OBJECT(args.GetItem(2));

    if (JS_HAS_NAME(optionsObj, JS_STRING_ID("reverse"))) {
      JS_LOCAL_VALUE obj_ = JS_GET_NAME(optionsObj, JS_STRING_ID("reverse"));
      reverse = BOOLEAN_TO_STD(obj_);
    }

    bool pass = false;
    if (JS_HAS_NAME(optionsObj, JS_STRING_ID("start"))) {
      startHandle = JS_GET_NAME(optionsObj, JS_STRING_ID("start"));
      pass =
          JS_IS_STRING(startHandle) || node::Buffer::HasInstance(startHandle);
    }

    if (pass) {
      // ignore start if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(startHandle) > 0) {
        LD_STRING_OR_BUFFER_TO_SLICE(_start, startHandle, start)
        start = new leveldb::Slice(_start.data(), _start.size());
      }
    }

    JS_LOCAL_VALUE endBuffer;
    pass = false;
    if (JS_HAS_NAME(optionsObj, JS_STRING_ID("end"))) {
      endBuffer = JS_GET_NAME(optionsObj, JS_STRING_ID("end"));
      pass = JS_IS_STRING(endBuffer) || node::Buffer::HasInstance(endBuffer);
    }

    if (pass) {
      // ignore end if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(endBuffer) > 0) {
        LD_STRING_OR_BUFFER_TO_SLICE(_end, endBuffer, end)
        end = new std::string(_end.data(), _end.size());
      }
    }

    if (JS_HAS_NAME(optionsObj, JS_STRING_ID("limit"))) {
      JS_LOCAL_VALUE obj_ = JS_GET_NAME(optionsObj, JS_STRING_ID("limit"));
      limit = INTEGER_TO_STD(obj_);
    }

    if (JS_HAS_NAME(optionsObj, JS_STRING_ID("highWaterMark"))) {
      JS_LOCAL_VALUE obj_ =
          JS_GET_NAME(optionsObj, JS_STRING_ID("highWaterMark"));
      highWaterMark = INTEGER_TO_STD(obj_);
    }

    JS_LOCAL_VALUE ltBuffer;
    pass = false;
    if (JS_HAS_NAME(optionsObj, JS_STRING_ID("lt"))) {
      ltBuffer = JS_GET_NAME(optionsObj, JS_STRING_ID("lt"));
      pass = JS_IS_STRING(ltBuffer) || node::Buffer::HasInstance(ltBuffer);
    }

    if (pass) {
      // ignore end if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(ltBuffer) > 0) {
        LD_STRING_OR_BUFFER_TO_SLICE(_lt, ltBuffer, lt)
        lt = new std::string(_lt.data(), _lt.size());
        if (reverse) start = new leveldb::Slice(_lt.data(), _lt.size());
      }
    }

    JS_LOCAL_VALUE lteBuffer;
    pass = false;
    if (JS_HAS_NAME(optionsObj, JS_STRING_ID("lte"))) {
      lteBuffer = JS_GET_NAME(optionsObj, JS_STRING_ID("lte"));
      pass = JS_IS_STRING(lteBuffer) || node::Buffer::HasInstance(lteBuffer);
    }

    if (pass) {
      // ignore end if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(lteBuffer) > 0) {
        LD_STRING_OR_BUFFER_TO_SLICE(_lte, lteBuffer, lte)
        lte = new std::string(_lte.data(), _lte.size());
        if (reverse) start = new leveldb::Slice(_lte.data(), _lte.size());
      }
    }

    JS_LOCAL_VALUE gtBuffer;
    pass = false;
    if (JS_HAS_NAME(optionsObj, JS_STRING_ID("gt"))) {
      gtBuffer = JS_GET_NAME(optionsObj, JS_STRING_ID("gt"));
      pass = JS_IS_STRING(gtBuffer) || node::Buffer::HasInstance(gtBuffer);
    }

    if (pass) {
      // ignore end if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(gtBuffer) > 0) {
        LD_STRING_OR_BUFFER_TO_SLICE(_gt, gtBuffer, gt)
        gt = new std::string(_gt.data(), _gt.size());
        if (!reverse) start = new leveldb::Slice(_gt.data(), _gt.size());
      }
    }

    JS_LOCAL_VALUE gteBuffer;
    pass = false;
    if (JS_HAS_NAME(optionsObj, JS_STRING_ID("gte"))) {
      gteBuffer = JS_GET_NAME(optionsObj, JS_STRING_ID("gte"));
      pass = JS_IS_STRING(gteBuffer) || node::Buffer::HasInstance(gteBuffer);
    }

    if (pass) {
      // ignore end if it has size 0 since a Slice can't have length 0
      if (StringOrBufferLength(gteBuffer) > 0) {
        LD_STRING_OR_BUFFER_TO_SLICE(_gte, gteBuffer, gte)
        gte = new std::string(_gte.data(), _gte.size());
        if (!reverse) start = new leveldb::Slice(_gte.data(), _gte.size());
      }
    }
  }

  OPTIONS_TO_BOOLEAN(keys, true);
  OPTIONS_TO_BOOLEAN(values, true);
  OPTIONS_TO_BOOLEAN(keyAsBuffer, true);
  OPTIONS_TO_BOOLEAN(valueAsBuffer, true);
  OPTIONS_TO_BOOLEAN(fillCache, true);

  JS_CLASS_NEW_INSTANCE(obj, Iterator);

  JS_LOCAL_OBJECT shObj = JS_VALUE_TO_OBJECT(startHandle);
  Iterator* iterator =
      new Iterator(database, (uint32_t)id->Int32Value(), start, end, reverse,
                   keys, values, limit, lt, lte, gt, gte, fillCache,
                   keyAsBuffer, valueAsBuffer, shObj, highWaterMark);
  iterator->Wrap(obj);

  RETURN_PARAM(obj);
}
JS_METHOD_END

}  // namespace leveldown
