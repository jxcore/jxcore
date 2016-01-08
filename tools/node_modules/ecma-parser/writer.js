/*
 * The MIT License (MIT)
 * Copyright (c) 2015 Oguz Bastemur
 */

var colors = ['green', 'yellow'];
var printBlocks = function (bl, depth) {
  if (!depth) {
    depth = "";
  }

  var vars = bl.variables;
  var type_ = bl.type;
  bl = bl.subs;

  var show = {
    "STRING": 1,
    "COMMENT": 1,
    "WORD": 1
  };

  console.log(depth + "--->", type_, vars);
  for (var i = 0; i < bl.length; i++) {
    var str;

    var clr = colors[i % 2];

    if (bl[i].type == "DIV") clr = 'red';

    if (!show[bl[i].type] && bl[i].delimiter.length != 1) {
      jxcore.utils.console.log(depth + bl[i].type, bl[i].rowIndex + ":" + bl[i].columnIndex, clr);
    } else if (bl[i].delimiter != 'function' && bl[i].delimiter != '{') {
      str = bl[i].getData();

      if (bl[i].type == "NEW_LINE") {
        str = str.replace('\n', '');
        if (bl[i].repeats) {
          str = "[Repeats " + bl[i].repeats + " time(s)]";
        }
      }

      if (str.trim() == bl[i].delimiter || str.trim().length == 0) str = "";
      else {
        str = str.split('\n');
        for (var o in str) if (str[o].trim) str[o] = str[o].trim();
        str = str.join('\n  ' + depth);
        str = ": " + str;
      }

      if (bl[i].type == 'WORD' && bl[i].dataType) {
        str += " (" + bl[i].dataType + ")"
      }

      str += " (" + bl[i].rowIndex + ":" + bl[i].columnIndex + ")";

      jxcore.utils.console.log(depth + bl[i].type + str, clr);
    }

    if (bl[i].subs && bl[i].subs.length) {
      printBlocks(bl[i], depth + "    ");
      console.log(depth + "<---", type_);
    }
  }
};

exports.printBlocks = printBlocks;

var arr = [];
function write(filename, block) {
  if (block.type == "ROOT")
    return write(filename, block.subs);

  for (var i = 0, ln = block.length; i < ln; i++) {
    try {
      arr.push(block[i].getData());
      if (block[i].repeats) {
        for (var z = 0; z < block[i].repeats - 1; z++)
          arr.push(block[i].getData());
      }
    } catch (e) {
      console.log(i, JSON.stringify(block[i], null, 2))
      throw e;
    }
    if (block[i].subs.length) {
      write(filename, block[i].subs)
    }
  }
}

exports.blockToCode = function (block, filename) {
  arr = [];
  if (block.type == "ROOT")
    write(filename, block.subs);
  else
    write(filename, block);

  return arr.join('');
};