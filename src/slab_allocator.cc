// Copyright & License details are available under JXCORE_LICENSE file

#include "node.h"
#include "node_buffer.h"
#include "slab_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

namespace node {

SlabAllocator::SlabAllocator(unsigned int size, commons* com) {
  if (com == NULL)
    com_ = node::commons::getInstance();
  else
    com_ = com;
  size_ = ROUND_UP(size ? size : 1, 8192);
  initialized_ = false;
  last_ptr_ = NULL;
  offset_ = 0;
}

SlabAllocator::~SlabAllocator() {
  if (!initialized_) return;
#ifdef JS_ENGINE_V8
  // TODO(obastemur) For both engines this would leak when jxcore is embedded ?
  if (v8::V8::IsDead()) return;

  JS_CLEAR_PERSISTENT(slab_sym_);
#elif defined(JS_ENGINE_MOZJS)
  slab_sym_[0] = '\0';
#endif

  JS_CLEAR_PERSISTENT(slab_);
}

void SlabAllocator::Initialize() {
  ENGINE_LOG_THIS("SlabAllocator", "Initialize");
  JS_ENTER_SCOPE_WITH(com_->node_isolate);
  JS_DEFINE_STATE_MARKER(com_);

  char sym[256];
  int ln = snprintf(sym, sizeof(sym), "slab_%p", this);  // namespace object key
  offset_ = 0;
  last_ptr_ = NULL;
  initialized_ = true;

#ifdef JS_ENGINE_V8
  JS_LOCAL_STRING str_sym = STD_TO_STRING_WITH_LENGTH(sym, ln);
  JS_NEW_PERSISTENT_STRING(slab_sym_, str_sym);
#elif defined(JS_ENGINE_MOZJS)
  memcpy(slab_sym_, sym, ln * sizeof(char));
  slab_sym_[ln] = '\0';
#endif
}

#define NewSlab(size)                                                    \
  JS_LOCAL_VALUE arg = STD_TO_INTEGER(ROUND_UP(size, 16));               \
  JS_LOCAL_FUNCTION_TEMPLATE bct =                                       \
      JS_TYPE_TO_LOCAL_FUNCTION_TEMPLATE(com_->bf_constructor_template); \
  JS_LOCAL_OBJECT buf = JS_NEW_INSTANCE(JS_GET_FUNCTION(bct), 1, &arg)

char* SlabAllocator::Allocate(JS_HANDLE_OBJECT_REF obj, unsigned int size) {
  ENGINE_LOG_THIS("SlabAllocator", "Allocate");
  JS_ENTER_SCOPE_WITH(com_->node_isolate);
  JS_DEFINE_STATE_MARKER(com_);

  assert(!JS_IS_EMPTY(obj));

  if (size == 0) return NULL;
  if (!initialized_) Initialize();

#ifdef JS_ENGINE_MOZJS
  const char* strl = (slab_sym_);
#else
  JS_LOCAL_STRING strl = JS_TYPE_TO_LOCAL_STRING(slab_sym_);
#endif
  if (size > size_) {
    NewSlab(size_);
    JS_NAME_SET_HIDDEN(obj, strl, buf);
    return BUFFER__DATA(buf);
  }

  if (JS_IS_EMPTY(slab_) || offset_ + size > size_) {
    JS_CLEAR_PERSISTENT(slab_);
    NewSlab(size_);
    JS_NEW_PERSISTENT_OBJECT(slab_, buf);
    offset_ = 0;
    last_ptr_ = NULL;
  }

  JS_LOCAL_OBJECT objl = JS_OBJECT_FROM_PERSISTENT(slab_);
  JS_NAME_SET_HIDDEN(obj, strl, objl);
  last_ptr_ = BUFFER__DATA(objl) + offset_;
  offset_ += size;

  return last_ptr_;
}

JS_LOCAL_OBJECT SlabAllocator::Shrink(JS_HANDLE_OBJECT_REF obj, char* ptr,
                                      unsigned int size) {
  ENGINE_LOG_THIS("SlabAllocator", "Shrink");
  JS_ENTER_SCOPE_WITH(com_->node_isolate);
  JS_DEFINE_STATE_MARKER(com_);

#ifdef JS_ENGINE_V8
  JS_LOCAL_STRING strl = JS_TYPE_TO_LOCAL_STRING(slab_sym_);
  JS_LOCAL_VALUE slab_v = JS_GET_NAME_HIDDEN(obj, strl);
  JS_NAME_SET_HIDDEN(obj, strl, JS_NULL());
  assert(!JS_IS_EMPTY(slab_v));
  assert(JS_IS_OBJECT(slab_v));
  JS_LOCAL_OBJECT slab = JS_VALUE_TO_OBJECT(slab_v);
#elif defined(JS_ENGINE_MOZJS)
  // both versions work for MozJS proxy, this one is faster
  // we do already check above asserts and there is no need for VALUE_TO_OBJECT
  JS_LOCAL_VALUE slab = JS_GET_NAME_HIDDEN(obj, slab_sym_);
  JS_NAME_SET_HIDDEN(obj, slab_sym_, JS_NULL());
#endif
  assert(ptr != NULL);
  if (ptr == last_ptr_) {
    last_ptr_ = NULL;
    offset_ = ptr - BUFFER__DATA(slab) + ROUND_UP(size, 16);
  }
  return JS_LEAVE_SCOPE(slab);
}

}  // namespace node
