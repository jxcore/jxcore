// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_SLAB_ALLOCATOR_H_
#define SRC_SLAB_ALLOCATOR_H_

#include "node.h"
#include "jx/commons.h"

namespace node {
class commons;

class SlabAllocator {
 public:
  SlabAllocator(unsigned int size = 10485760,
                commons* com = NULL);  // default to 10M
  ~SlabAllocator();

  // allocate memory from slab, attaches the slice to `obj`
  char* Allocate(JS_HANDLE_OBJECT_REF obj, unsigned int size);

  // return excess memory to the slab, returns a handle to the parent buffer
  JS_LOCAL_OBJECT Shrink(JS_HANDLE_OBJECT_REF obj, char* ptr,
                         unsigned int size);

  commons* com_;

 private:
  void Initialize();
  bool initialized_;
  JS_PERSISTENT_OBJECT slab_;
#ifdef JS_ENGINE_V8
  JS_PERSISTENT_STRING slab_sym_;
#elif defined(JS_ENGINE_MOZJS)
  char slab_sym_[256];
#endif
  unsigned int offset_;
  unsigned int size_;
  char* last_ptr_;
};

}  // namespace node
#endif  // SRC_SLAB_ALLOCATOR_H_
