/*
 * Copyright (c) 2014, Oguz Bastemur (oguz@bastemur.com)
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

var minimize = require('./node_modules/ecma-parser/minimize').minimize;
var fs = require('fs');
var path = require('path');

var out_file = process.argv[2];
var header_name = path.basename(out_file).replace(/[.]/g, "_");

var str = '#ifndef ' + header_name + '\n' + '#define ' + header_name + '\n'
    + 'namespace jxcore {\n\n';

var buffers = {};

// check startsWith implementation
if (!"".startsWith) {
  String.prototype.startsWith = function(x) {
    return (this.indexOf(x) === 0);
  }
}

for (var o = 3, ln = process.argv.length; o < ln; o++) {
  var file_path = process.argv[o];
  var extname = path.extname(file_path);
  if (extname == '.py')
    continue;

  var buffer = fs.readFileSync(file_path);
  var file_name = path.basename(file_path);
  var name = file_name.replace('.js', '').replace('.gypi', '');

  if (name != '_jx_marker') {
    buffer = new Buffer(minimize(file_name, buffer + '', true));
  }

  buffers[name] = buffer;
}

var stream = fs.createWriteStream(out_file);
stream.once('open',
    function(fd) {
      stream.write(str);

      for (var o in buffers) {
        var buffer = buffers[o];
        str = '  const char ' + o + '_native[]={';
        for (var o = 0, ln = buffer.length; o < ln; o++) {
          if (o)
            str += ',';
          if (o % 128 == 0)
            str += '\n\t';
          str += '(char)' + buffer[o];
          if (o % 1000 == 0) {
            stream.write(str);
            str = '';
          }
        }

        if (str.length)
          stream.write(str);

        stream.write(',0};\n\n');
      }

      stream.write('typedef struct _native {\n' + '  const char* name;\n'
          + '  const char* source;\n' + '  size_t source_len;\n'
          + '}_native;\n\n');

      stream.write('static const struct _native natives[] = {\n');

      for (var o in buffers) {
        if (o == '_jx_marker')
          str = '  {"' + o + '", ' + o + '_native, 0},\n';
        else
          str = '  {"' + o + '", ' + o + '_native, sizeof(' + o
              + '_native)-1},\n';

        stream.write(str);
      }

      stream.write('  {NULL, NULL, 0}\n\n');
      stream.write('};\n');
      stream.write('}\n#endif\n');

      stream.end();
    });

stream.once('close', function(fd) {
  // dummy
});
