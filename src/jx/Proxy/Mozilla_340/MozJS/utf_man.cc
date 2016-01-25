/*
Copyright (c) 2014 Nubisa, Inc.
Copyright (c) 2005 JSON.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

The Software shall be used for Good, not Evil.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

  Copyright (c) 2015 Mozilla

 * A part of this Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "utf_man.h"

static const uint32_t INVALID_UTF8 = UINT32_MAX;
static const uint32_t REPLACE_UTF8 = 0xFFFD;

uint32_t Utf8ToOneUcs4Char(const uint8_t *utf8Buffer, int utf8Length) {

  if (utf8Length == 1) {
    return *utf8Buffer;
  }

  /* from Unicode 3.1, non-shortest form is illegal */
  static const uint32_t minucs4Table[] = {0x80, 0x800, 0x10000};

  uint32_t ucs4Char = *utf8Buffer++ & ((1 << (7 - utf8Length)) - 1);
  uint32_t minucs4Char = minucs4Table[utf8Length - 2];
  while (--utf8Length) {
    ucs4Char = (ucs4Char << 6) | (*utf8Buffer++ & 0x3F);
  }

  if ((ucs4Char < minucs4Char || (ucs4Char >= 0xD800 && ucs4Char <= 0xDFFF)))
    return INVALID_UTF8;

  return ucs4Char;
}

#define ReportInvalidCharacter 1
#define ReportBufferTooSmall 2
#define ReportTooBigCharacter 2

#ifdef DEBUG
#define INVALID(report, arg, n2)                                     \
  do {                                                               \
    if (report == 2)                                                 \
      flush_console(                                                 \
          "Error while converting UTF8 (Please report this at "      \
          "https://github.com/jxcore/jxcore) - Err: %s\n",           \
          #report);                                                  \
    else                                                             \
      flush_console("!! Invalid Character while converting UTF8\n"); \
    dst[j] = jschar(REPLACE_UTF8);                                   \
    n = n2;                                                          \
    goto invalidMultiByteCodeUnit;                                   \
  } while (0)
#else
#define INVALID(report, arg, n2)   \
  do {                             \
    dst[j] = jschar(REPLACE_UTF8); \
    n = n2;                        \
    goto invalidMultiByteCodeUnit; \
  } while (0)
#endif

bool ConvertCharToChar16(const char *src, jschar *dst, size_t srclen,
                         size_t *dstlenp) {
  uint32_t j = 0;
  for (uint32_t i = 0; i < srclen; i++, j++) {
    uint32_t v = uint32_t(src[i]);
    if (!(v & 0x80)) {
      // ASCII code unit.  Simple copy.
      dst[j] = jschar(v);

    } else {
      uint32_t n = 1;
      while (v & (0x80 >> n)) n++;

      // Check the leading byte.
      if (n < 2 || n > 4) INVALID(ReportInvalidCharacter, i, 1);

      // Check that |src| is large enough to hold an n-byte code unit.
      if (i + n > srclen) INVALID(ReportBufferTooSmall, /* dummy = */ 0, 1);

      // Check the second byte.  From Unicode Standard v6.2, Table 3-7
      // Well-Formed UTF-8 Byte Sequences.
      if ((v == 0xE0 && ((uint8_t)src[i + 1] & 0xE0) != 0xA0) ||  // E0 A0~BF
          (v == 0xED && ((uint8_t)src[i + 1] & 0xE0) != 0x80) ||  // ED 80~9F
          (v == 0xF0 && ((uint8_t)src[i + 1] & 0xF0) == 0x80) ||  // F0 90~BF
          (v == 0xF4 && ((uint8_t)src[i + 1] & 0xF0) != 0x80))    // F4 80~8F
      {
        INVALID(ReportInvalidCharacter, i, 1);
      }

      // Check the continuation bytes.
      for (uint32_t m = 1; m < n; m++)
        if ((src[i + m] & 0xC0) != 0x80) INVALID(ReportInvalidCharacter, i, m);

      // Determine the code unit's length in jschars and act accordingly.
      v = Utf8ToOneUcs4Char((uint8_t *)&src[i], n);
      if (v < 0x10000) {
        // The n-byte UTF8 code unit will fit in a single jschar.
        dst[j] = jschar(v);

      } else {
        v -= 0x10000;
        if (v <= 0xFFFFF) {
          // The n-byte UTF8 code unit will fit in two jschars.
          dst[j] = jschar((v >> 10) + 0xD800);
          j++;

          dst[j] = jschar((v & 0x3FF) + 0xDC00);

        } else {
          // The n-byte UTF8 code unit won't fit in two jschars.
          INVALID(ReportTooBigCharacter, v, 1);
        }
      }

    invalidMultiByteCodeUnit:
      // Move i to the last byte of the multi-byte code unit;  the loop
      // header will do the final i++ to move to the start of the next
      // code unit.
      i += n - 1;
    }
  }

  *dstlenp = j;

  return true;
}

class utf8_decoder {
  unsigned the_index;
  unsigned the_length;
  int the_char;
  int the_byte;
  const char *the_input;

  /*
      Get the next byte. It returns UTF8_END if there are no more bytes.
  */
 public:
  utf8_decoder(const char *source, unsigned length) : the_input(source) {
    the_index = 0;
    the_char = 0;
    the_length = length;
    the_byte = 0;
  }

  int get() {
    int c;
    if (the_index >= the_length) {
      return UTF8_END;
    }
    c = the_input[the_index] & 0xFF;
    the_index += 1;
    return c;
  }

  /*
      Get the 6-bit payload of the next continuation byte.
      Return UTF8_ERROR if it is not a contination byte.
  */
  int cont() {
    const int c = get();
    return ((c & 0xC0) == 0x80) ? (c & 0x3F) : UTF8_ERROR;
  }

  /*
      Get the current byte offset. This is generally used in error reporting.
  */
  int utf8_decode_at_byte() { return the_byte; }

  /*
      Get the current character offset. This is generally used in error
     reporting.
      The character offset matches the byte offset if the text is strictly
     ASCII.
  */
  int utf8_decode_at_character() { return the_char > 0 ? the_char - 1 : 0; }

  /*
      Extract the next character.
      Returns: the character (between 0 and 1114111)
           or  UTF8_END   (the end)
           or  UTF8_ERROR (error)
  */
  int utf8_decode_next() {
    int c; /* the first byte of the character */
    int r; /* the result */

    if (the_index >= the_length) {
      return the_index == the_length ? UTF8_END : UTF8_ERROR;
    }
    the_byte = the_index;
    the_char += 1;
    c = get();
    /*
        Zero continuation (0 to 127)
    */
    if ((c & 0x80) == 0) {
      return c;
    }
    /*
        One continuation (128 to 2047)
    */
    if ((c & 0xE0) == 0xC0) {
      int c1 = cont();
      if (c1 >= 0) {
        r = ((c & 0x1F) << 6) | c1;
        return r >= 128 ? r : UTF8_ERROR;
      }

      /*
          Two continuation (2048 to 55295 and 57344 to 65535)
      */
    } else if ((c & 0xF0) == 0xE0) {
      int c1 = cont();
      int c2 = cont();
      if ((c1 | c2) >= 0) {
        r = ((c & 0x0F) << 12) | (c1 << 6) | c2;
        return r >= 2048 && (r < 55296 || r > 57343) ? r : UTF8_ERROR;
      }

      /*
          Three continuation (65536 to 1114111)
      */
    } else if ((c & 0xF8) == 0xF0) {
      int c1 = cont();
      int c2 = cont();
      int c3 = cont();
      if ((c1 | c2 | c3) >= 0) {
        return (((c & 0x0F) << 18) | (c1 << 12) | (c2 << 6) | c3) + 65536;
      }
    }
    return UTF8_ERROR;
  }
};

/*
    Very Strict UTF-8 Decoder

    UTF-8 is a multibyte character encoding of Unicode. A character can be
    represented by 1-4 bytes. The bit pattern of the first byte indicates the
    number of continuation bytes.

    Most UTF-8 decoders tend to be lenient, attempting to recover as much
    information as possible, even from badly encoded input. This UTF-8
    decoder is not lenient. It will reject input which does not include
    proper continuation bytes. It will reject aliases (or suboptimal
    codings). It will reject surrogates. (Surrogate encoding should only be
    used with UTF-16.)

    Code     Contination Minimum Maximum
    0xxxxxxx           0       0     127
    10xxxxxx       error
    110xxxxx           1     128    2047
    1110xxxx           2    2048   65535 excluding 55296 - 57343
    11110xxx           3   65536 1114111
    11111xxx       error
*/

int CheckUnicode(const char p[], unsigned length) {
  int c;
  utf8_decoder dec(p, length);
  for (unsigned the_index = 0;;) {
    c = dec.utf8_decode_next();
    if (c < 0) {
      return c == UTF8_END ? the_index : UTF8_ERROR;
    }
    if (c < 0x10000) {
      the_index += 1;
    } else {
      c -= 0x10000;
      the_index += 2;
    }
  }

  return 0;  // compiler warning
}
