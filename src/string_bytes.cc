// Copyright & License details are available under JXCORE_LICENSE file

#include "string_bytes.h"

#include <assert.h>
#include <string.h>  // memcpy
#include <limits.h>

#include "node.h"
#include "node_buffer.h"

namespace node {

//// Base 64 ////

#define base64_encoded_size(size) ((size + 2 - ((size + 2) % 3)) / 3 * 4)

// Doesn't check for padding at the end.  Can be 1-2 bytes over.
static inline size_t base64_decoded_size_fast(size_t size) {
  size_t remainder = size % 4;

  size = (size / 4) * 3;
  if (remainder) {
    if (size == 0 && remainder == 1) {
      // special case: 1-byte input cannot be decoded
      size = 0;
    } else {
      // non-padded input, add 1 or 2 extra bytes
      size += 1 + (remainder == 3);
    }
  }

  return size;
}

static inline size_t base64_decoded_size(const char* src, size_t size) {
  if (size == 0) return 0;

  if (src[size - 1] == '=') size--;
  if (size > 0 && src[size - 1] == '=') size--;

  return base64_decoded_size_fast(size);
}

// supports regular and URL-safe base64
static const int unbase64_table[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -1, -1, -2, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, 62, -1, 62, -1, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1,
    63, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1};
#define unbase64(x) unbase64_table[(uint8_t)(x)]

static inline size_t base64_decode(char* buf, size_t len, const char* src,
                                   const size_t srcLen) {
  char a, b, c, d;
  char* dst = buf;
  char* dstEnd = buf + len;
  const char* srcEnd = src + srcLen;

  while (src < srcEnd && dst < dstEnd) {
    int remaining = srcEnd - src;

    while (unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
    if (remaining == 0 || *src == '=') break;
    a = unbase64(*src++);

    while (unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
    if (remaining <= 1 || *src == '=') break;
    b = unbase64(*src++);

    *dst++ = (a << 2) | ((b & 0x30) >> 4);
    if (dst == dstEnd) break;

    while (unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
    if (remaining <= 2 || *src == '=') break;
    c = unbase64(*src++);

    *dst++ = ((b & 0x0F) << 4) | ((c & 0x3C) >> 2);
    if (dst == dstEnd) break;

    while (unbase64(*src) < 0 && src < srcEnd) src++, remaining--;
    if (remaining <= 3 || *src == '=') break;
    d = unbase64(*src++);

    *dst++ = ((c & 0x03) << 6) | (d & 0x3F);
  }

  return dst - buf;
}

//// HEX ////

static inline unsigned hex2bin(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  return static_cast<unsigned>(-1);
}

static inline size_t hex_decode(char* buf, size_t len, const char* src,
                                const size_t srcLen) {
  size_t i;
  for (i = 0; i < len && i * 2 + 1 < srcLen; ++i) {
    unsigned a = hex2bin(src[i * 2 + 0]);
    unsigned b = hex2bin(src[i * 2 + 1]);
    if (!~a || !~b) return i;
    buf[i] = a * 16 + b;
  }

  return i;
}

size_t StringBytes::JXWrite(char* buf, size_t buflen, JS_HANDLE_VALUE_REF val,
                            enum encoding encoding, bool is_buffer,
                            int* chars_written) {
  ENGINE_LOG_THIS("StringBytes", "JXWrite");
  JS_ENTER_SCOPE();
  size_t len = 0;

  // sometimes we use 'binary' when we mean 'buffer'
  if (is_buffer && (encoding == BINARY || encoding == BUFFER)) {
    // fast path, copy buffer data
    JS_LOCAL_OBJECT valObj = JS_CAST_OBJECT(JS_TYPE_TO_LOCAL_VALUE(val));
    const char* data = (const char*)BUFFER__DATA(valObj);
    size_t size = BUFFER__LENGTH(valObj);
    size_t len = size < buflen ? size : buflen;
    memcpy(buf, data, len);

    return len;
  }

  JS_LOCAL_STRING str = JS_VALUE_TO_STRING(val);

#ifdef JS_ENGINE_V8
  int flags =
      v8::String::NO_NULL_TERMINATION | v8::String::HINT_MANY_WRITES_EXPECTED;
#endif

  switch (encoding) {
    case ASCII: {
#ifdef JS_ENGINE_V8
      len = str->WriteAscii(buf, 0, buflen, flags);
#else
      jxcore::JXString jxsa;
      jxsa.set_handle(str, true);
      len = buflen < jxsa.length() ? buflen : jxsa.length();
      memcpy(buf, *jxsa, len);
#endif
      if (chars_written != NULL) {
        *chars_written = len;
      }
    } break;

    case UTF8: {
#ifdef JS_ENGINE_V8
      len = str->WriteUtf8(buf, buflen, chars_written, WRITE_UTF8_FLAGS);
#elif defined(JS_ENGINE_MOZJS)
      jxcore::JXString jxs8(str);
      len = jxs8.WriteUtf8(buf, buflen, chars_written);
#endif
    } break;

    case UCS2: {
#ifdef JS_ENGINE_V8
      len = str->Write(reinterpret_cast<uint16_t*>(buf), 0, buflen, flags);
#else
      MozJS::auto_jschar outto;
      MozJS::StringTools::JS_ConvertToJSChar(str.ctx_,
                                             str.GetRawStringPointer(), &outto);
      len = outto.length_;
      if (len * sizeof(uint16_t) > buflen) {
        len = buflen;
      }
      memcpy(buf, (uint16_t*)outto.str_, len * sizeof(uint16_t));
#endif
      if (chars_written != NULL) {
        *chars_written = len;
      }
      len = len * sizeof(uint16_t);
    } break;

    case BASE64: {
      jxcore::JXString value;
      value.set_handle(str, true);

      len = base64_decode(buf, buflen, *value, value.length());
      if (chars_written != NULL) {
        *chars_written = len;
      }
      break;
    }

    case BINARY:
    case BUFFER: {
      uint16_t* twobytebuf = new uint16_t[buflen];
#ifdef JS_ENGINE_V8
      len = str->Write(twobytebuf, 0, buflen, flags);
#elif defined(JS_ENGINE_MOZJS)
      ENGINE_NS::auto_jschar tstr;
      ENGINE_NS::StringTools::JS_ConvertToJSChar(str, &tstr);
      len = buflen < tstr.length_ ? buflen : tstr.length_;
      memcpy(twobytebuf, reinterpret_cast<const uint16_t*>(tstr.str_),
             len * sizeof(char16_t));
#endif
      for (size_t i = 0; i < buflen && i < len; i++) {
        unsigned char* b = reinterpret_cast<unsigned char*>(&twobytebuf[i]);
        buf[i] = b[0];
      }

      if (chars_written != NULL) {
        *chars_written = len;
      }

      delete[] twobytebuf;
      break;
    }

    case HEX: {
      jxcore::JXString value(str);
      len = hex_decode(buf, buflen, *value, value.length());
      if (chars_written != NULL) {
        *chars_written = len * 2;
      }
      break;
    }

    default:
      assert(0 && "unknown encoding");
      break;
  }

  return len;
}

size_t StringBytes::Write(char* buf, size_t buflen, JS_HANDLE_VALUE val,
                          enum encoding encoding, int* chars_written) {
  JS_ENTER_SCOPE_COM();
  bool is_buffer = Buffer::jxHasInstance(val, com);
  return StringBytes::JXWrite(buf, buflen, val, encoding, is_buffer,
                              chars_written);
}

bool StringBytes::IsValidString(JS_HANDLE_STRING str, enum encoding enc) {
  return !(enc == HEX && JS_GET_STRING_LENGTH(str) % 2 != 0);
}

// Quick and dirty size calculation
// Will always be at least big enough, but may have some extra
// UTF8 can be as much as 3x the size, Base64 can have 1-2 extra bytes
size_t StringBytes::JXStorageSize(JS_HANDLE_VALUE_REF val,
                                  enum encoding encoding, bool is_buffer,
                                  const size_t measured_size) {
  ENGINE_LOG_THIS("StringBytes", "JXStorageSize");
  JS_ENTER_SCOPE();
  size_t data_size = 0;

  if (is_buffer && (encoding == BUFFER || encoding == BINARY)) {
    return BUFFER__LENGTH(val);
  }

  size_t sz;
  if (measured_size != 0) {
    sz = measured_size;
  } else {
    JS_LOCAL_STRING str = JS_VALUE_TO_STRING(val);
    sz = JS_GET_STRING_LENGTH(str);
  }

  switch (encoding) {
    case BINARY:
    case BUFFER:
    case ASCII:
      data_size = sz;
      break;

    case UTF8:
      data_size = 3 * sz;
      break;

    case UCS2:
      data_size = sz * sizeof(uint16_t);
      break;

    case BASE64:
      data_size = base64_decoded_size_fast(sz);
      break;

    case HEX:
      assert(sz % 2 == 0 && "invalid hex string length");
      data_size = sz / 2;
      break;

    default:
      assert(0 && "unknown encoding");
      break;
  }

  return data_size;
}

size_t StringBytes::StorageSize(JS_HANDLE_VALUE val, enum encoding encoding) {
  JS_ENTER_SCOPE_COM();
  return StringBytes::JXStorageSize(val, encoding,
                                    Buffer::jxHasInstance(val, com));
}

size_t StringBytes::JXSize(JS_HANDLE_VALUE_REF val, enum encoding encoding,
                           bool is_buffer, const size_t measured_size) {
  ENGINE_LOG_THIS("StringBytes", "JXSize");
  JS_ENTER_SCOPE();
  size_t data_size = 0;

  if (is_buffer && (encoding == BUFFER || encoding == BINARY)) {
    return JS_GET_EXTERNAL_ARRAY_DATA_LENGTH(val);
  }

  size_t sz;
  if (measured_size != 0) {
    sz = measured_size;
  } else {
    JS_LOCAL_STRING str = JS_VALUE_TO_STRING(val);
    sz = JS_GET_STRING_LENGTH(str);
  }

  switch (encoding) {
    case BINARY:
    case BUFFER:
    case ASCII:
      data_size = sz;
      break;

    case UTF8: {
      JS_LOCAL_STRING str = JS_VALUE_TO_STRING(val);
      data_size = str->Utf8Length();
    } break;

    case UCS2:
      data_size = sz * sizeof(uint16_t);
      break;

    case BASE64: {
      JS_LOCAL_STRING str = JS_VALUE_TO_STRING(val);
      jxcore::JXString value(str);
      data_size = base64_decoded_size(*value, value.length());
      break;
    }

    case HEX:
      data_size = sz / 2;
      break;

    default:
      assert(0 && "unknown encoding");
      break;
  }

  return data_size;
}

size_t StringBytes::Size(JS_HANDLE_VALUE val, enum encoding encoding) {
  JS_ENTER_SCOPE_COM();
  size_t data_size = 0;
  return StringBytes::JXSize(val, encoding, Buffer::jxHasInstance(val, com));
}

static bool contains_non_ascii_slow(const char* buf, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    if (buf[i] & 0x80) return true;
  }
  return false;
}

static bool contains_non_ascii(const char* src, size_t len) {
  if (len < 16) {
    return contains_non_ascii_slow(src, len);
  }

  const unsigned bytes_per_word = sizeof(void*);
  const unsigned align_mask = bytes_per_word - 1;
  const unsigned unaligned = reinterpret_cast<uintptr_t>(src) & align_mask;

  if (unaligned > 0) {
    const unsigned n = bytes_per_word - unaligned;
    if (contains_non_ascii_slow(src, n)) return true;
    src += n;
    len -= n;
  }

#if defined(__x86_64__) || defined(_WIN64)
  const uintptr_t mask = 0x8080808080808080ll;
#else
  const uintptr_t mask = 0x80808080l;
#endif

  const uintptr_t* srcw = reinterpret_cast<const uintptr_t*>(src);

  for (size_t i = 0, n = len / bytes_per_word; i < n; ++i) {
    if (srcw[i] & mask) return true;
  }

  const unsigned remainder = len & align_mask;
  if (remainder > 0) {
    const size_t offset = len - remainder;
    if (contains_non_ascii_slow(src + offset, remainder)) return true;
  }

  return false;
}

static void force_ascii_slow(const char* src, char* dst, size_t len) {
  for (size_t i = 0; i < len; ++i) {
    dst[i] = src[i] & 0x7f;
  }
}

static void force_ascii(const char* src, char* dst, size_t len) {
  if (len < 16) {
    force_ascii_slow(src, dst, len);
    return;
  }

  const unsigned bytes_per_word = sizeof(void*);
  const unsigned align_mask = bytes_per_word - 1;
  const unsigned src_unalign = reinterpret_cast<uintptr_t>(src) & align_mask;
  const unsigned dst_unalign = reinterpret_cast<uintptr_t>(dst) & align_mask;

  if (src_unalign > 0) {
    if (src_unalign == dst_unalign) {
      const unsigned unalign = bytes_per_word - src_unalign;
      force_ascii_slow(src, dst, unalign);
      src += unalign;
      dst += unalign;
      len -= src_unalign;
    } else {
      force_ascii_slow(src, dst, len);
      return;
    }
  }

#if defined(__x86_64__) || defined(_WIN64)
  const uintptr_t mask = ~0x8080808080808080ll;
#else
  const uintptr_t mask = ~0x80808080l;
#endif

  const uintptr_t* srcw = reinterpret_cast<const uintptr_t*>(src);
  uintptr_t* dstw = reinterpret_cast<uintptr_t*>(dst);

  for (size_t i = 0, n = len / bytes_per_word; i < n; ++i) {
    dstw[i] = srcw[i] & mask;
  }

  const unsigned remainder = len & align_mask;
  if (remainder > 0) {
    const size_t offset = len - remainder;
    force_ascii_slow(src + offset, dst + offset, remainder);
  }
}

static size_t base64_encode(const char* src, size_t slen, char* dst,
                            size_t dlen) {
  // We know how much we'll write, just make sure that there's space.
  assert(dlen >= base64_encoded_size(slen) &&
         "not enough space provided for base64 encode");

  dlen = base64_encoded_size(slen);

  unsigned a;
  unsigned b;
  unsigned c;
  unsigned i;
  unsigned k;
  unsigned n;

  static const char table[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";

  i = 0;
  k = 0;
  n = slen / 3 * 3;

  while (i < n) {
    a = src[i + 0] & 0xff;
    b = src[i + 1] & 0xff;
    c = src[i + 2] & 0xff;

    dst[k + 0] = table[a >> 2];
    dst[k + 1] = table[((a & 3) << 4) | (b >> 4)];
    dst[k + 2] = table[((b & 0x0f) << 2) | (c >> 6)];
    dst[k + 3] = table[c & 0x3f];

    i += 3;
    k += 4;
  }

  if (n != slen) {
    switch (slen - n) {
      case 1:
        a = src[i + 0] & 0xff;
        dst[k + 0] = table[a >> 2];
        dst[k + 1] = table[(a & 3) << 4];
        dst[k + 2] = '=';
        dst[k + 3] = '=';
        break;

      case 2:
        a = src[i + 0] & 0xff;
        b = src[i + 1] & 0xff;
        dst[k + 0] = table[a >> 2];
        dst[k + 1] = table[((a & 3) << 4) | (b >> 4)];
        dst[k + 2] = table[(b & 0x0f) << 2];
        dst[k + 3] = '=';
        break;
    }
  }

  return dlen;
}

static size_t hex_encode(const char* src, size_t slen, char* dst, size_t dlen) {
  // We know how much we'll write, just make sure that there's space.
  assert(dlen >= slen * 2 && "not enough space provided for hex encode");

  dlen = slen * 2;
  for (uint32_t i = 0, k = 0; k < dlen; i += 1, k += 2) {
    static const char hex[] = "0123456789abcdef";
    uint8_t val = static_cast<uint8_t>(src[i]);
    dst[k + 0] = hex[val >> 4];
    dst[k + 1] = hex[val & 15];
  }

  return dlen;
}

JS_LOCAL_VALUE StringBytes::Encode(const char* buf, size_t buflen,
                                   enum encoding encoding) {
  ENGINE_LOG_THIS("StringBytes", "Encode");
  JS_ENTER_SCOPE_COM();
  JS_DEFINE_STATE_MARKER(com);

  assert(buflen <= Buffer::kMaxLength);
  if (!buflen && encoding != BUFFER)
    return JS_LEAVE_SCOPE(JS_NEW_EMPTY_STRING());

  JS_LOCAL_STRING val;
  switch (encoding) {
    case BUFFER: {
      return JS_LEAVE_SCOPE(Buffer::New(buf, buflen, com)->handle_);
    } break;

    case ASCII:
      if (contains_non_ascii(buf, buflen)) {
        char* out = new char[buflen];
        force_ascii(buf, out, buflen);
        val = STD_TO_STRING_WITH_LENGTH(out, buflen);
        delete[] out;
      } else {
        val = STD_TO_STRING_WITH_LENGTH(buf, buflen);
      }
      break;

    case UTF8: {
      val = UTF8_TO_STRING_WITH_LENGTH(buf, buflen);
    } break;

    case BINARY: {
      const unsigned char* cbuf = reinterpret_cast<const unsigned char*>(buf);
      uint16_t* twobytebuf = new uint16_t[buflen];
      for (size_t i = 0; i < buflen; i++) {
        // XXX is the following line platform independent?
        twobytebuf[i] = cbuf[i];
      }
      val = STD_TO_STRING_WITH_LENGTH(twobytebuf, buflen);
      delete[] twobytebuf;
      break;
    }

    case BASE64: {
      size_t dlen = base64_encoded_size(buflen);
      char* dst = new char[dlen];

      size_t written = base64_encode(buf, buflen, dst, dlen);
      assert(written == dlen);

      val = STD_TO_STRING_WITH_LENGTH(dst, dlen);
      delete[] dst;
      break;
    }

    case UCS2: {
      const uint16_t* data = reinterpret_cast<const uint16_t*>(buf);
      val = STD_TO_STRING_WITH_LENGTH(data, buflen / 2);
      break;
    }

    case HEX: {
      size_t dlen = buflen * 2;
      char* dst = new char[dlen];
      size_t written = hex_encode(buf, buflen, dst, dlen);
      assert(written == dlen);

      val = STD_TO_STRING_WITH_LENGTH(dst, dlen);
      delete[] dst;
      break;
    }

    default:
      assert(0 && "unknown encoding");
      break;
  }

  return JS_LEAVE_SCOPE(val);
}

}  // namespace node
