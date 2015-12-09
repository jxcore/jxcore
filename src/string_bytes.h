// Copyright & License details are available under JXCORE_LICENSE file

#ifndef SRC_STRING_BYTES_H_
#define SRC_STRING_BYTES_H_

// Decodes a JS_HANDLE_STRING or Buffer to a raw char*

#include "node.h"

namespace node {

extern int WRITE_UTF8_FLAGS;

class StringBytes {
 public:
  // Does the string match the encoding? Quick but non-exhaustive.
  // Example: a HEX string must have a length that's a multiple of two.
  static bool IsValidString(JS_HANDLE_STRING string, enum encoding enc);

  // Fast, but can be 2 bytes oversized for Base64, and
  // as much as triple UTF-8 strings <= 65536 chars in length
  static size_t StorageSize(JS_HANDLE_VALUE val, enum encoding enc);
  static size_t JXStorageSize(JS_HANDLE_VALUE_REF val, enum encoding enc,
                              bool is_buffer, const size_t measured_size = 0);

  // Precise byte count, but slightly slower for Base64 and
  // very much slower for UTF-8
  static size_t Size(JS_HANDLE_VALUE val, enum encoding enc);
  static size_t JXSize(JS_HANDLE_VALUE_REF val, enum encoding enc,
                       bool is_buffer, const size_t measured_size = 0);

  // Write the bytes from the string or buffer into the char*
  // returns the number of bytes written, which will always be
  // <= buflen.  Use StorageSize/Size first to know how much
  // memory to allocate.
  static size_t Write(char* buf, size_t buflen, JS_HANDLE_VALUE val,
                      enum encoding enc, int* chars_written = NULL);
  static size_t JXWrite(char* buf, size_t buflen, JS_HANDLE_VALUE_REF val,
                        enum encoding enc, bool is_buffer,
                        int* chars_written = NULL);

  // Take the bytes in the src, and turn it into a Buffer or String.
  static JS_LOCAL_VALUE Encode(const char* buf, size_t buflen,
                               enum encoding encoding);
};

}  // namespace node

#endif  // SRC_STRING_BYTES_H_
