// Copyright & License details are available under JXCORE_LICENSE file

#include "jxp_compress.h"
#include "external/miniz.c"
#include "jx/extend.h"

typedef unsigned char uint8;

namespace jxcore {
node::Buffer *CompressString(node::commons *com, const char *str,
                             const long len) {
  uLong cmp_len = compressBound(len);
  uLong len2 = cmp_len + 1;
  mz_uint8 *pCmp = (mz_uint8 *)malloc((size_t)len2);

  if ((!pCmp)) {
    return NULL;
  }

  int cmp_status = compress(pCmp + 1, &cmp_len,
                            reinterpret_cast<const unsigned char *>(str), len);
  if (cmp_status != Z_OK) {
    free(pCmp);
    return NULL;
  }

  int mt = (len / cmp_len);
  if (mt > 127) mt = 127;
  pCmp[0] = mt;

  node::Buffer *buff = (node::Buffer::New(reinterpret_cast<const char *>(pCmp),
                                          cmp_len + 1, com));
  free(pCmp);

  return buff;
}

node::Buffer *CompressString(node::commons *com, JS_HANDLE_VALUE *buff) {
  char* bufferData = node::Buffer::Data(*buff);
  size_t bufferLength = node::Buffer::Length(*buff);
  return CompressString(com, bufferData, bufferLength);
}

static uint8 *compress_cache_ = NULL;
static unsigned long cache_size_ = 0;

bool RaiseCache(unsigned long cache_size) {
  auto_lock locker_(CSLOCK_COMPRESS);

  if (cache_size <= cache_size_) {
    return true;
  }

  if (compress_cache_ != NULL) {
    free(compress_cache_);
  }

  cache_size_ = 0;
  compress_cache_ = (uint8 *)malloc((size_t)cache_size);

  if (!compress_cache_) {
    return false;
  }

  cache_size_ = cache_size;

  return true;
}

void RemoveCache() {
  if (cache_size_ <= 64 * 1024) return;

  auto_lock locker_(CSLOCK_COMPRESS);

  if (compress_cache_ != NULL) {
    free(compress_cache_);
  }
  cache_size_ = 64 * 1024;
  compress_cache_ = (uint8 *)malloc((size_t)cache_size_);
}

node::Buffer *UncompressString(node::commons *com, JS_HANDLE_OBJECT obj,
                               const unsigned long ub64_len) {
  JS_ENTER_SCOPE_WITH(com->node_isolate);

  const char *str = BUFFER__DATA(obj);

  mz_ulong lenf = ub64_len * (((int)str[0]) + 1);
  // first char holds the compression ratio
  // any ratio better than 1:100 (event 1:100 though)
  // is not possible. There must be something wrong
  // return NULL (it will turn into 'Package corrupted' exception
  if (lenf < 0 || lenf > ub64_len * 100) return NULL;

  const uint8 *ucmp = reinterpret_cast<const uint8 *>(str);

  auto_lock locker_(CSLOCK_COMPRESS);

  bool cache_tried = lenf >= cache_size_;
  uint8 *pUncomp;
  int cmp_status;

retry:
  if (cache_tried) {
    pUncomp = (mz_uint8 *)malloc((size_t)lenf);
    if ((!pUncomp)) {
      return NULL;
    }

    cmp_status = uncompress(pUncomp, &lenf, ucmp + 1, ub64_len - 1);
  } else {
    lenf = cache_size_;
    cmp_status = uncompress(compress_cache_, &lenf, ucmp + 1, ub64_len - 1);
    pUncomp = compress_cache_;
  }

  if (cmp_status == MZ_BUF_ERROR) {
    if (cache_tried)
      free(pUncomp);
    else
      cache_tried = true;
    lenf *= 2;
    goto retry;
  }

  if (cmp_status != Z_OK) {
    if (cache_tried) free(pUncomp);

    return NULL;
  }

  node::Buffer *buff =
      node::Buffer::New(reinterpret_cast<const char *>(pUncomp), lenf, com);

  if (cache_tried) free(pUncomp);

  return buff;
}

mz_uint8 *UncompressNative(node::commons *com, const char *str, const unsigned long ub64_len) {
  mz_ulong lenf = ub64_len * (((int)str[0]) + 1);
  const uint8 *ucmp = reinterpret_cast<const uint8 *>(str);

  uint8 *pUncomp;
  int cmp_status;

retry:
  pUncomp = (mz_uint8 *)malloc((size_t)lenf);
  if ((!pUncomp)) {
    return NULL;
  }

  cmp_status = uncompress(pUncomp, &lenf, ucmp + 1, ub64_len - 1);

  if (cmp_status == MZ_BUF_ERROR) {
    free(pUncomp);
    lenf *= 2;
    goto retry;
  }

  if (cmp_status != Z_OK) {
    free(pUncomp);

    return NULL;
  }
  pUncomp[lenf - 1] = '\0';

  return pUncomp;
}
}  // namespace jxcore
