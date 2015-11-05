// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_JX_JXP_COMPRESS_H_
#define SRC_JX_JXP_COMPRESS_H_

#include "node_buffer.h"

namespace jxcore {
typedef unsigned char mz_uint8;
node::Buffer* CompressString(node::commons* com, const char* str,
                             const long len);
node::Buffer* CompressString(node::commons *com, JS_HANDLE_VALUE *buff);

bool RaiseCache(unsigned long cache_size);

void RemoveCache();

node::Buffer* UncompressString(node::commons* com, JS_HANDLE_OBJECT obj,
                               const unsigned long ub64_len);
mz_uint8* UncompressNative(node::commons *com, const char* str, const unsigned long ub64_len);
}  // namespace jxcore

#endif  // SRC_JX_JXP_COMPRESS_H_
