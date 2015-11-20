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
*/

#include "utf_man.h"

class utf8_decoder {
  unsigned the_index;
  unsigned the_length;
  int the_char;
  int the_byte;
  const char* the_input;

  /*
      Get the next byte. It returns UTF8_END if there are no more bytes.
  */
 public:
  utf8_decoder(const char* source, unsigned length) : the_input(source) {
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

int utf8_to_utf16(char16_t w[], const char p[], unsigned length) {
  int c;
  utf8_decoder dec(p, length);
  for (unsigned the_index = 0;;) {
    c = dec.utf8_decode_next();
    if (c < 0) {
      return c == UTF8_END ? the_index : UTF8_ERROR;
    }
    if (c < 0x10000) {
      w[the_index] = (char16_t)c;
      the_index += 1;
    } else {
      c -= 0x10000;
      w[the_index] = (char16_t)(0xD800 | (c >> 10));
      the_index += 1;
      w[the_index] = (char16_t)(0xDC00 | (c & 0x3FF));
      the_index += 1;
    }
  }

  return 0;  // compiler warning
}
