/* Copyright (c) 2012-2015 LevelDOWN contributors
 * See list at <https://github.com/level/leveldown#contributing>
 * MIT License <https://github.com/level/leveldown/blob/master/LICENSE.md>
 */
#ifndef LD_LEVELDOWN_H
#define LD_LEVELDOWN_H

#include <node.h>
#include <node_buffer.h>
#include <leveldb/slice.h>

static inline size_t StringOrBufferLength(JS_LOCAL_VALUE val) {
  if (JS_IS_EMPTY(val)) {
    return 0;
  } else {
    JS_LOCAL_OBJECT obj = JS_VALUE_TO_OBJECT(val);

    if (node::Buffer::HasInstance(obj)) {
      return node::Buffer::Length(obj);
    } else {
      JS_LOCAL_STRING str = JS_VALUE_TO_STRING(val);
      jxcore::JXString jstr(str);

      return jstr.Utf8Length();
    }
  }
}

// NOTE: this MUST be called on objects created by
// LD_STRING_OR_BUFFER_TO_SLICE
static void DisposeStringOrBufferFromSlice(JS_PERSISTENT_OBJECT& handle,
                                           leveldb::Slice slice) {
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);
  if (!slice.empty()) {
    if (JS_HAS_NAME(JS_TYPE_TO_LOCAL_OBJECT(handle), JS_STRING_ID("obj"))) {
      JS_LOCAL_VALUE obj =
          JS_GET_NAME(JS_TYPE_TO_LOCAL_OBJECT(handle), JS_STRING_ID("obj"));
      if (!node::Buffer::HasInstance(obj)) delete[] slice.data();
    } else {
      if (!node::Buffer::HasInstance(JS_TYPE_TO_LOCAL_OBJECT(handle)))
        delete[] slice.data();
    }
  }

  JS_CLEAR_PERSISTENT(handle);
}

#if defined(JS_ENGINE_V8)
static void DisposeStringOrBufferFromSlice(JS_LOCAL_VALUE handle,
                                           leveldb::Slice slice) {

  if (!slice.empty() && !node::Buffer::HasInstance(handle))
    delete[] slice.data();
}
#endif

// NOTE: must call DisposeStringOrBufferFromSlice() on objects created here
#define LD_STRING_OR_BUFFER_TO_SLICE(to, from, name)                          \
  size_t to##Sz_;                                                             \
  char* to##Ch_;                                                              \
  {                                                                           \
    JS_LOCAL_OBJECT __obj__ = JS_VALUE_TO_OBJECT(from);                       \
    if (JS_IS_NULL_OR_UNDEFINED(__obj__)) {                                   \
      to##Sz_ = 0;                                                            \
      to##Ch_ = 0;                                                            \
    } else if (!JS_IS_EMPTY(__obj__) && node::Buffer::HasInstance(__obj__)) { \
      to##Sz_ = node::Buffer::Length(__obj__);                                \
      to##Ch_ = node::Buffer::Data(__obj__);                                  \
    } else {                                                                  \
      JS_LOCAL_STRING to##Str = JS_VALUE_TO_STRING(from);                     \
      jxcore::JXString jx##Str(to##Str);                                      \
      jx##Str.DisableAutoGC();                                                \
      to##Ch_ = *jx##Str;                                                     \
      to##Sz_ = jx##Str.Utf8Length();                                         \
    }                                                                         \
  }                                                                           \
  leveldb::Slice to(to##Ch_, to##Sz_);

#define LD_RETURN_CALLBACK_OR_ERROR(callback, msg)          \
  if (!JS_IS_EMPTY(callback) && JS_IS_FUNCTION(callback)) { \
    JS_LOCAL_VALUE argv[] = {JS_NEW_ERROR_VALUE(msg)};      \
    LD_RUN_CALLBACK(callback, 1, argv)                      \
    RETURN();                                               \
  }                                                         \
  THROW_EXCEPTION(msg);

#define LD_RUN_CALLBACK(callback, argc, argv) \
  NanMakeCallback(JS_GET_GLOBAL(), callback, argc, argv);

/* LD_METHOD_SETUP_COMMON setup the following objects:
 *  - Database* database
 *  - JS_LOCAL_OBJECT optionsObj (may be empty)
 *  - v8::Persistent<v8::Function> callback (won't be empty)
 * Will throw/return if there isn't a callback in arg 0 or 1
 */
#define LD_METHOD_SETUP_COMMON(name, optionPos, callbackPos)        \
  if (args.Length() == 0)                                           \
    THROW_EXCEPTION(#name "() requires a callback argument");       \
  leveldown::Database* database =                                   \
      node::ObjectWrap::Unwrap<leveldown::Database>(args.This());   \
  JS_LOCAL_OBJECT optionsObj;                                       \
  JS_HANDLE_FUNCTION callback;                                       \
  if (optionPos == -1 && args.IsFunction(callbackPos)) {            \
    callback = args.GetAsFunction(callbackPos);                     \
  } else if (optionPos != -1 && args.IsFunction(callbackPos - 1)) { \
    callback = args.GetAsFunction(callbackPos - 1);                 \
  } else if (optionPos != -1 && args.IsObject(optionPos) &&         \
             args.IsFunction(callbackPos)) {                        \
    optionsObj = JS_VALUE_TO_OBJECT(args.GetItem(optionPos));       \
    callback = args.GetAsFunction(callbackPos);                     \
  } else {                                                          \
    THROW_EXCEPTION(#name "() requires a callback argument");       \
  }

#define LD_METHOD_SETUP_COMMON_ONEARG(name) LD_METHOD_SETUP_COMMON(name, -1, 0)

#endif
